/*
 * Copyright (c) 2007-2008 Digital Bazaar, Inc.  All rights reserved.
 */
#include "db/event/Observable.h"

using namespace std;
using namespace db::event;
using namespace db::modest;
using namespace db::rt;

Observable::Observable()
{
   // no events to dispatch yet
   mDispatch = false;
}

Observable::~Observable()
{
   // ensure event dispatching is stopped
   stop();
}

void Observable::dispatchEvent(
   Event& e, EventId id, OperationList& opList)
{
   // go through the list of EventId taps
   EventIdMap::iterator ti = mTaps.find(id);
   if(ti != mTaps.end())
   {
      EventIdMap::iterator end = mTaps.upper_bound(id);
      for(; ti != end; ti++)
      {
         // dispatch event if the tap is the EventId itself
         if(ti->second == id)
         {
            // go through the list of observers for the EventId tap
            ObserverMap::iterator oi = mObservers.find(id);
            if(oi != mObservers.end())
            {
               ObserverMap::iterator oend = mObservers.upper_bound(id);
               for(; oi != oend; oi++)
               {
                  // create and run event dispatcher for each observable
                  RunnableRef ed = new EventDispatcher(oi->second, &e);
                  Operation op(ed);
                  mOpRunner->runOperation(op);
                  opList.add(op);
               }
            }
         }
         else
         {
            // dispatch event to tap
            dispatchEvent(e, ti->second, opList);
         }
      }
   }
}

void Observable::dispatchEvent(Event& e)
{
   // create an operation list
   OperationList opList;
   
   // get the EventId for the event and dispatch it
   EventId id = e["id"]->getUInt64();
   dispatchEvent(e, id, opList);
   
   if(!opList.isEmpty())
   {
      // unlock, wait for dispatch operations to complete, relock
      unlock();
      if(!opList.waitFor())
      {
         // dispatch thread interrupted, so interrupt all
         // event dispatches and wait for them to complete
         opList.interrupt();
         opList.waitFor(false);
      }
      lock();
   }
}

void Observable::dispatchEvents()
{
   // lock while dispatching
   lock();
   {
      // dispatch
      while(!mEventQueue.empty() && !mOperation->isInterrupted())
      {
         // dispatch the next event
         Event e = mEventQueue.front();
         mEventQueue.pop_front();
         dispatchEvent(e);
      }
      
      // turn off dispatching
      mDispatch = false;
   }
   unlock();
}

void Observable::registerObserver(Observer* observer, EventId id)
{
   lock();
   {
      // add tap to self if EventId doesn't exist yet
      EventIdMap::iterator i = mTaps.find(id);
      if(i == mTaps.end())
      {
         mTaps.insert(make_pair(id, id));
      }
      
      // add observer
      mObservers.insert(make_pair(id, observer));
   }
   unlock();
}

void Observable::unregisterObserver(Observer* observer, EventId id)
{
   lock();
   {
      ObserverMap::iterator i = mObservers.find(id);
      if(i != mObservers.end())
      {
         ObserverMap::iterator end = mObservers.upper_bound(id);
         for(; i != end; i++)
         {
            if(i->second == observer)
            {
               // remove observer and break
               mObservers.erase(i);
               break;
            }
         }
      }
   }
   unlock();
}

void Observable::addTap(EventId id, EventId tap)
{
   lock();
   {
      // add tap to id-self if EventId doesn't exist yet
      EventIdMap::iterator i = mTaps.find(id);
      if(i == mTaps.end())
      {
         mTaps.insert(make_pair(id, id));
      }
      
      // insert tap for id
      mTaps.insert(make_pair(id, tap));
      
      // add tap to tap-self if EventId doesn't exist yet
      i = mTaps.find(tap);
      if(i == mTaps.end())
      {
         mTaps.insert(make_pair(tap, tap));
      }
   }
   unlock();
}

void Observable::removeTap(EventId id, EventId tap)
{
   lock();
   {
      // look for tap in the range of taps
      EventIdMap::iterator i = mTaps.find(id);
      if(i != mTaps.end())
      {
         EventIdMap::iterator end = mTaps.upper_bound(id);
         for(; i != end; i++)
         {
            if(i->second == tap)
            {
               // remove tap and break
               mTaps.erase(i);
               break;
            }
         }
      }
   }
   unlock();
}

void Observable::schedule(Event e, EventId id, bool async)
{
   lock();
   {
      // set the event's ID
      e["id"] = id;
      
      if(async)
      {
         // lock to set dispatch condition
         mDispatchLock.lock();
         mDispatch = true;
         
         // add event to event queue
         mEventQueue.push_back(e);
         
         // notify on dispatch lock and release
         mDispatchLock.notifyAll();
         mDispatchLock.unlock();
      }
      else
      {
         // dispatch the event immediately
         dispatchEvent(e);
      }
   }
   unlock();
}

void Observable::start(OperationRunner* opRunner)
{
   lock();
   {
      if(mOperation.isNull())
      {
         // store operation runner, activate dispatching,
         // create and run operation
         mOpRunner = opRunner;
         mDispatch = true;
         mOperation = *this;
         opRunner->runOperation(mOperation);
      }
   }
   unlock();
}

void Observable::stop()
{
   lock();
   {
      if(!mOperation.isNull())
      {
         // interrupt dispatch operation
         mOperation->interrupt();
         
         // Note: We only care about locking in this method to prevent
         // start() from running while we are shutting down -- we don't
         // care if more events are scheduled because that won't cause
         // any conflicts. Since this is the case, and start() will
         // check mOperation.isNull() before starting anything, then
         // we don't need to worry about it starting here since mOperation
         // can't be NULL until we relock and clear it below.
         // 
         // We need to unlock() while we wait for the dispatch operation
         // to complete because it needs to lock after dispatching each
         // event and won't finish if we are holding the lock here waiting
         // for it to finish.
         
         // unlock, wait for dispatch operation to finish, relock
         unlock();
         mOperation->waitFor();
         lock();
         
         // clean up operation
         mOperation.setNull();
      }
   }
   unlock();
}

void Observable::run()
{
   // keep dispatching until interrupted
   while(!mOperation->isInterrupted())
   {
      // lock dispatch lock and check to see if we can dispatch
      mDispatchLock.lock();
      if(mDispatch)
      {
         mDispatchLock.unlock();
         dispatchEvents();
      }
      else
      {
         // wait until dispatch condition is marked true and we are notified
         mDispatchLock.wait();
         mDispatchLock.unlock();
      }
   }
}
