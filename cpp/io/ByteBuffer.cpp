/*
 * Copyright (c) 2007-2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/io/ByteBuffer.h"

#include "monarch/rt/Exception.h"

#include <cstdlib>
#include <cstring>

using namespace monarch::io;
using namespace monarch::rt;

ByteBuffer::ByteBuffer(int capacity)
{
   // create the byte buffer
   mCapacity = capacity;
   mBuffer = (capacity > 0) ? (unsigned char*)malloc(mCapacity) : NULL;
   mOffset = mBuffer;
   mLength = 0;
   mCleanup = true;
}

ByteBuffer::ByteBuffer(
   char* b, int offset, int length, int capacity, bool cleanup)
{
   // set the byte buffer
   mCleanup = false;
   capacity = (capacity == -1) ? length : capacity;
   ByteBuffer::setBytes(b, offset, length, capacity, cleanup);
}

ByteBuffer::ByteBuffer(const ByteBuffer& copy)
{
   // copy bytes
   mCapacity = copy.capacity();
   mBuffer = (mCapacity > 0) ? (unsigned char*)malloc(mCapacity) : NULL;
   memcpy(mBuffer, copy.bytes(), copy.capacity());
   mOffset = mBuffer + copy.offset();
   mLength = copy.length();
   mCleanup = true;
}

ByteBuffer::~ByteBuffer()
{
   // clean up byte buffer
   cleanupBytes();
}

void ByteBuffer::cleanupBytes()
{
   if(mCleanup)
   {
      ::free(mBuffer);
      mBuffer = NULL;
   }
}

void ByteBuffer::free()
{
   cleanupBytes();
   mBuffer = mOffset = NULL;
   mCapacity = 0;
   mLength = 0;
   mCleanup = true;
}

void ByteBuffer::allocateSpace(int length, bool resize)
{
   if(resize)
   {
      // determine if the buffer needs to be resized
      int overflow = length - freeSpace();
      if(overflow > 0)
      {
         // resize the buffer by the overflow amount
         this->resize(mCapacity + overflow);
      }
   }

   // determine if the data needs to be shifted
   if(mOffset > mBuffer)
   {
      int overflow = length - freeSpace() + (mOffset - mBuffer);
      if(overflow > 0)
      {
         if(mLength > 0)
         {
            // shift the data in the buffer
            memmove(mBuffer, mOffset, mLength);
         }

         mOffset = mBuffer;
      }
   }
}

void ByteBuffer::resize(int capacity)
{
   if(capacity != mCapacity)
   {
      if(mCleanup && mBuffer != NULL)
      {
         // move existing data to front of buffer
         if(mOffset > mBuffer)
         {
            memmove(mBuffer, mOffset, mLength);
            mOffset = mBuffer;
         }

         // store old offset difference
         int offset = mOffset - mBuffer;

         // reallocate buffer
         mBuffer = (unsigned char*)realloc(mBuffer, capacity);
         mCapacity = capacity;
         mLength = (mCapacity < mLength) ? mCapacity : mLength;
         mOffset = (offset < mLength) ? mBuffer + offset : mBuffer + mLength;
      }
      else
      {
         // create a new buffer
         unsigned char* newBuffer = (unsigned char*)malloc(capacity);

         // copy the data into the new buffer, truncate old count as necessary
         mCapacity = capacity;
         mLength = (mCapacity < mLength) ? mCapacity : mLength;
         memcpy(newBuffer, mOffset, mLength);

         // clean up old buffer
         cleanupBytes();

         // memory management now on regardless of previous setting
         mBuffer = mOffset = newBuffer;
         mCleanup = true;
      }
   }
}

void ByteBuffer::reAllocate(int capacity, bool copy)
{
   // save old data info
   unsigned char* data = mBuffer;
   char* offset = (char*)mOffset;
   int length = mLength;

   // create the new byte buffer
   mCapacity = capacity;
   mBuffer = (capacity > 0) ? (unsigned char*)malloc(mCapacity) : NULL;
   mOffset = mBuffer;
   mLength = 0;

   if(copy && capacity > 0)
   {
      // put old data into buffer
      put(offset, length, false);
   }

   if(mCleanup)
   {
      // free old data
      ::free(data);
   }
   else
   {
      // now must cleanup
      mCleanup = true;
   }
}

int ByteBuffer::putByte(unsigned char b, int n, bool resize)
{
   // allocate space for the data
   allocateSpace(n, resize);

   int fs = freeSpace();
   n = (fs < n ? fs : n);
   if(n > 0)
   {
      // set data in buffer
      memset(mOffset + mLength, b, n);
      mLength += n;
   }

   return n;
}

int ByteBuffer::put(const char* b, int length, bool resize)
{
   // allocate space for the data
   allocateSpace(length, resize);

   // copy data into the buffer
   int fs = freeSpace();
   length = (length < fs) ? length : fs;
   memcpy(mOffset + mLength, b, length);
   mLength += length;

   return length;
}

int ByteBuffer::put(ByteBuffer* b, int length, bool resize)
{
   length = (length < b->length()) ? length : b->length();
   return put(b->bytes() + b->offset(), length, resize);
}

int ByteBuffer::put(InputStream* is, int length)
{
   int rval = 0;

   // if the buffer is not full, do a read
   int fs = freeSpace();
   if(fs != 0)
   {
      // allocate free space
      allocateSpace(fs, false);

      // determine how much to read
      length = (length > 0 && length < fs ? length : fs);

      // read some bytes
      rval = is->read(end(), length);
      if(rval > 0)
      {
         // update buffer length
         mLength += rval;
      }
   }

   return rval;
}

int ByteBuffer::fill(InputStream* is, int length)
{
   int rval = 0;

   // if the buffer is not full, do a read
   int fs = freeSpace();
   if(fs != 0)
   {
      // allocate free space
      allocateSpace(fs, false);

      // determine how much to read
      length = (length > 0 && length < fs ? length : fs);

      // read until amount received or stream empty/buffer full
      int numBytes = 1;
      while(length > 0 && numBytes > 0)
      {
         numBytes = is->read(end(), length);
         if(numBytes >= 0)
         {
            // update by bytes read
            mLength += numBytes;
            rval += numBytes;
            length -= numBytes;
         }
         else
         {
            rval = -1;
         }
      }
   }

   return rval;
}

int ByteBuffer::getByte(unsigned char& b)
{
   int rval = 0;

   if(mLength > 0)
   {
      // get byte
      b = mOffset[0];

      // move internal pointer
      ++mOffset;
      --mLength;
      ++rval;
   }

   return rval;
}

int ByteBuffer::get(char* b, int length)
{
   length = (length < mLength) ? length : mLength;
   memcpy(b, data(), length);

   // move internal pointer
   mOffset += length;
   mLength -= length;

   return length;
}

int ByteBuffer::get(ByteBuffer* b, int length, bool resize)
{
   // put data into passed buffer
   length = (length < mLength) ? length : mLength;
   int rval = b->put(data(), length, resize);

   // move internal pointer and change length
   mOffset += rval;
   mLength -= rval;

   return rval;
}

int ByteBuffer::get(OutputStream* os)
{
   int rval = 0;

   if(os->write(data(), mLength))
   {
      rval = mLength;
      mOffset = mBuffer;
      mLength = 0;
   }
   else
   {
      // determine if output stream would block
      ExceptionRef e = Exception::get();
      if(e->getDetails()->hasMember("wouldBlock"))
      {
         // move internal pointer and change length by amount written
         rval = e->getDetails()["written"]->getInt32();
         mOffset += rval;
         mLength -= rval;
      }

      // exception
      rval = -1;
   }

   return rval;
}

int ByteBuffer::clear(int length)
{
   // ensure that the maximum cleared is existing length
   int rval = (length > 0) ? ((mLength < length) ? mLength : length) : 0;

   // set new length and offset
   mLength -= rval;
   mOffset = (mLength == 0) ? mBuffer : mOffset + rval;

   return rval;
}

inline int ByteBuffer::clear()
{
   return clear(mLength);
}

int ByteBuffer::advanceOffset(int length)
{
   // ensure that the maximum cleared is existing length
   int rval = (length > 0) ? ((mLength < length) ? mLength : length) : 0;

   // set new length and offset
   mLength -= rval;
   mOffset = mOffset + rval;

   return rval;
}

int ByteBuffer::reset(int length)
{
   // ensure that the most the offset is moved back is to the beginning
   // of the buffer
   int max = mOffset - mBuffer;
   int rval = (length > 0 ? (max < length ? max : length) : 0);

   // set new offset and length
   mOffset -= rval;
   mLength += rval;

   return rval;
}

int ByteBuffer::trim(int length)
{
   // ensure that the maximum trimmed is existing length
   int rval = (length > 0) ? ((mLength < length) ? mLength : length) : 0;

   // set new length
   mLength -= rval;

   return rval;
}

int ByteBuffer::extend(int length)
{
   // ensure that the maximum extended is (free space - offset)
   int max = freeSpace() - (mOffset - mBuffer);
   int rval = (length > 0) ? ((max < length) ? max : length) : 0;

   // set new length
   mLength += rval;

   return rval;
}

unsigned char ByteBuffer::next()
{
   --mLength;
   ++mOffset;
   return (mOffset - 1)[0];
}

inline int ByteBuffer::capacity() const
{
   return mCapacity;
}

inline void ByteBuffer::setBytes(ByteBuffer* b, bool cleanup)
{
   // set the byte buffer
   setBytes(b->bytes(), b->offset(), b->length(), b->capacity(), cleanup);
}

void ByteBuffer::setBytes(
   char* b, int offset, int length, int capacity, bool cleanup)
{
   // cleanup old buffer
   cleanupBytes();

   mCapacity = capacity;
   mBuffer = (unsigned char*)b;
   mOffset = mBuffer + offset;
   mLength = length;
   mCleanup = cleanup;
}

inline char* ByteBuffer::bytes() const
{
   return (char*)mBuffer;
}

inline unsigned char* ByteBuffer::ubytes() const
{
   return mBuffer;
}

inline char* ByteBuffer::data() const
{
   return (char*)(mOffset);
}

inline unsigned char* ByteBuffer::udata() const
{
   return mOffset;
}

inline char* ByteBuffer::end() const
{
   return (char*)(mOffset + mLength);
}

inline unsigned char* ByteBuffer::uend() const
{
   return mOffset + mLength;
}

inline int ByteBuffer::offset() const
{
   return mOffset - mBuffer;
}

inline int ByteBuffer::length() const
{
   return mLength;
}

inline int ByteBuffer::freeSpace() const
{
   return mCapacity - mLength;
}

inline bool ByteBuffer::isFull() const
{
   return freeSpace() == 0;
}

inline bool ByteBuffer::isEmpty() const
{
   return mLength == 0;
}

inline bool ByteBuffer::isManaged() const
{
   return mCleanup;
}
