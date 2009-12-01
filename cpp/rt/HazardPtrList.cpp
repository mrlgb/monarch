/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "db/rt/HazardPtrList.h"

#include "db/rt/Atomic.h"

using namespace db::rt;

HazardPtrList::HazardPtrList() :
   mHead(NULL)
{
}

HazardPtrList::~HazardPtrList()
{
}

HazardPtr* HazardPtrList::acquire()
{
   volatile HazardPtr* rval = NULL;

   // find a hazard pointer that is inactive
   for(HazardPtr* ptr = (HazardPtr*)mHead; ptr != NULL; ptr = ptr->next)
   {
      // try to mark an inactive pointer as active
      if(!ptr->active && Atomic::compareAndSwap(&ptr->active, false, true))
      {
         // successfully acquired
         rval = ptr;
      }
   }

   // no inactive pointer found, create a new one
   if(rval == NULL)
   {
      rval = new HazardPtr;
      rval->active = true;
      rval->ptr = NULL;

      // atomically push the pointer onto the head of the list
      volatile HazardPtr* old;
      do
      {
         old = mHead;
         rval->next = (HazardPtr*)old;
      }
      while(!Atomic::compareAndSwap(&mHead, old, rval));
   }

   return const_cast<HazardPtr*>(rval);
}

void HazardPtrList::release(HazardPtr* ptr)
{
   ptr->ptr = NULL;
   ptr->active = false;
}

const HazardPtr* HazardPtrList::first()
{
   return const_cast<HazardPtr*>(mHead);
}
