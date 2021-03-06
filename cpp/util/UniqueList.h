/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_util_UniqueList_H
#define monarch_util_UniqueList_H

#include "monarch/rt/ListIterator.h"

namespace monarch
{
namespace util
{

/**
 * A UniqueList consists of a list of unique objects. The objects will be
 * compared on their equality operator: operator==.
 *
 * @author Dave Longley
 */
template<typename T>
class UniqueList
{
protected:
   /**
    * The underlying list data structure.
    */
   std::list<T> mList;

public:
   /**
    * Creates a new UniqueList.
    */
   UniqueList() {};

   /**
    * Destructs this UniqueList.
    */
   virtual ~UniqueList() {};

   /**
    * Adds an object to this list, if it isn't already in the list.
    *
    * @param obj the object to add.
    *
    * @return true if the object was added, false if it was not.
    */
   virtual bool add(const T& obj);

   /**
    * Removes an object from this list, if it is in the list.
    *
    * @param obj the object to remove.
    *
    * @return true if the object was removed, false if it not.
    */
   virtual bool remove(const T& obj);

   /**
    * Concatenates another list onto this one.
    *
    * @param rhs another list to concatenate onto this one.
    *
    * @return true if all items were added, false if at least one was not.
    */
   virtual bool concat(UniqueList<T>& rhs);

   /**
    * Clears this list.
    */
   virtual void clear();

   /**
    * The number of items in this list.
    *
    * @return the number of items in this list.
    */
   virtual unsigned int count();

   /**
    * Gets the Iterator for this list. It must be deleted after use.
    *
    * @return the Iterator for the list.
    */
   virtual monarch::rt::Iterator<T>* getIterator();
};

template<typename T>
bool UniqueList<T>::add(const T& obj)
{
   bool rval = true;

   monarch::rt::Iterator<T>* i = getIterator();
   while(rval && i->hasNext())
   {
      rval = !(i->next() == obj);
   }
   delete i;

   if(rval)
   {
      mList.push_back(obj);
   }

   return rval;
}

template<typename T>
bool UniqueList<T>::remove(const T& obj)
{
   bool rval = false;

   monarch::rt::Iterator<T>* i = getIterator();
   while(!rval && i->hasNext())
   {
      T& t = i->next();
      if(t == obj)
      {
         i->remove();
         rval = true;
      }
   }
   delete i;

   return rval;
}

template<typename T>
bool UniqueList<T>::concat(UniqueList<T>& rhs)
{
   bool rval = true;

   monarch::rt::Iterator<T>* i = rhs.getIterator();
   while(i->hasNext())
   {
      if(!add(i->next()))
      {
         rval = false;
      }
   }
   delete i;

   return rval;
}

template<typename T>
void UniqueList<T>::clear()
{
   // clear list
   mList.clear();
}

template<typename T>
unsigned int UniqueList<T>::count()
{
   // return size of list
   return mList.size();
}

template<typename T>
monarch::rt::Iterator<T>* UniqueList<T>::getIterator()
{
   return new monarch::rt::ListIterator<T>(mList);
}

} // end namespace util
} // end namespace monarch
#endif
