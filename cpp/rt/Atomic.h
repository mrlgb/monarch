/*
 * Copyright (c) 2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef db_rt_Atomic_H
#define db_rt_Atomic_H

#include <cstdlib>
#include <inttypes.h>

#ifdef WIN32
// FIXME: get the CPU address size and use it instead of assuming 32
#define ALIGN_BYTES 4
#define ALIGN_BITS  32
typedef int8_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_int8_t;
typedef int16_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_int16_t;
typedef int32_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_int32_t;
typedef uint8_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_uint8_t;
typedef uint16_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_uint16_t;
typedef uint32_t __attribute__ ((aligned(ALIGN_BYTES))) aligned_uint32_t;
#else
typedef int8_t aligned_int8_t;
typedef int16_t aligned_int16_t;
typedef int32_t aligned_int32_t;
typedef uint8_t aligned_uint8_t;
typedef uint16_t aligned_uint16_t;
typedef uint32_t aligned_uint32_t;
#endif

namespace db
{
namespace rt
{

/**
 * The Atomic class provides methods for doing atomic operations that are
 * supported by the system's CPU.
 *
 * @author Dave Longley
 */
class Atomic
{
public:
   /**
    * Allocates memory that is aligned such that it can be safely used in
    * special atomic operations. Certain operating systems (MS Windows)
    * require that memory that is used in operations such as Compare-And-Swap
    * must be aligned on boundaries that match the address size for the CPU.
    *
    * @param size the size of the memory to allocate.
    *
    * @return a pointer to the allocated memory.
    */
   static void* mallocAligned(size_t size);

   /**
    * Frees some previously allocated aligned-memory.
    *
    * @param ptr the pointer to the aligned-memory to free.
    */
   static void freeAligned(void* ptr);

   /**
    * Performs an atomic Add-And-Fetch. Adds the given value to the value at
    * the given destination.
    *
    * @param dst the destination with the value to add to.
    * @param value the value to add.
    *
    * @return the new value.
    */
   template<typename T>
   static T addAndFetch(T* dst, T value);

   /**
    * Performs an atomic Subtract-And-Fetch. Subtracts the given value from
    * the value at the given destination.
    *
    * @param dst the destination with the value to subtract from.
    * @param value the value to subtract.
    *
    * @return the new value.
    */
   template<typename T>
   static T subtractAndFetch(T* dst, T value);

   /**
    * Performs an atomic Compare-And-Swap (CAS). The given new value will only
    * be written to the destination if it contains the given old value. If
    * the old value matches, the write will occur and the function returns
    * true. If the old value does not match, nothing will happen and the
    * function will return false.
    *
    * @param dst the destination to write the new value to.
    * @param oldVal the old value.
    * @param newVal the new value.
    *
    * @return true if successful, false if not.
    */
   template<typename T>
   static bool compareAndSwap(T* dst, T oldVal, T newVal);
};

template<typename T>
T Atomic::addAndFetch(T* dst, T value)
{
#ifdef WIN32
   return InterlockedAdd(static_cast<LONG*>(dst), static_cast<LONG*>(value));
#else
   return __sync_add_and_fetch(dst, value);
#endif
}

template<typename T>
T Atomic::subtractAndFetch(T* dst, T value)
{
#ifdef WIN32
   return addAndFetch(dst, -value);
#else
   return __sync_sub_and_fetch(dst, value);
#endif
}

template<typename T>
bool Atomic::compareAndSwap(T* dst, T oldVal, T newVal)
{
#ifdef WIN32
   return (InterlockedCompareExchange(
      static_cast<LONG*>(dst),
      static_cast<LONG*>(newVal),
      static_cast<LONG*>(oldVal)) == oldVal);
#else
   return __sync_bool_compare_and_swap(dst, oldVal, newVal);
#endif
}

} // end namespace rt
} // end namespace db
#endif