/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/compress/deflate/Deflater.h"

#include "monarch/rt/Exception.h"

using namespace monarch::compress::deflate;
using namespace monarch::io;
using namespace monarch::rt;

#define EXCEPTION_DEFLATE "monarch.compress.deflate"

Deflater::Deflater()
{
   mShouldFinish = false;
   mFinished = false;
}

Deflater::~Deflater()
{
   Deflater::cleanupStream();
}

bool Deflater::startDeflating(int level, bool raw)
{
   // clean up previous stream
   cleanupStream();

   int ret;

   if(raw)
   {
      // windowBits is negative to indicate a raw stream
      // 8 is the default memLevel, use default strategy as well
      ret = deflateInit2(
         &mZipStream, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
   }
   else
   {
      // windowBits set to use zlib header/trailer
      // 8 is the default memLevel, use default strategy as well
      ret = deflateInit2(
         &mZipStream, level, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
   }

   mDeflating = true;
   mShouldFinish = false;
   mFinished = false;

   return !createException(ret, NULL);
}

bool Deflater::startInflating(bool raw)
{
   // clean up previous stream
   cleanupStream();

   int ret;

   if(raw)
   {
      // windowBits is negative to indicate a raw stream
      ret = inflateInit2(&mZipStream, -15);
   }
   else
   {
      // add 32 to windowBits to support gzip or zlib decoding
      ret = inflateInit2(&mZipStream, 15 + 32);
   }

   mDeflating = false;
   mShouldFinish = false;
   mFinished = false;

   return !createException(ret, NULL);
}

void Deflater::setInput(const char* b, int length, bool finish)
{
   // set zip stream input buffer
   mZipStream.next_in = (unsigned char*)b;
   mZipStream.avail_in = length;
   mShouldFinish = finish;
}

int Deflater::process(ByteBuffer* dst, bool resize)
{
   int rval = 0;

   if(!mFinished)
   {
      // when deflating, let zlib determine flushing to maximize compression
      // when inflating, flush output whenever possible
      int flush = (mShouldFinish ?
         Z_FINISH : (mDeflating ? Z_NO_FLUSH : Z_SYNC_FLUSH));

      // keep processing while no finished, no error, no output data, while
      // there is input data or processing should finish, and while there is
      // room to store the output data or if the destination buffer can be
      // resized
      while(!mFinished && rval == 0 &&
            (mZipStream.avail_in > 0 || mShouldFinish) &&
            (resize || !dst->isFull()))
      {
         if(resize && dst->isFull())
         {
            // allocate space for output
            dst->allocateSpace(1024, resize);
         }

         // set output buffer, store old free space
         int freeSpace = dst->freeSpace();
         dst->allocateSpace(freeSpace, false);
         mZipStream.next_out = dst->uend();
         mZipStream.avail_out = freeSpace;

         // perform deflation/inflation
         int ret = (mDeflating) ?
            ::deflate(&mZipStream, flush) :
            ::inflate(&mZipStream, flush);

         // extend destination buffer by data written to it
         int length = freeSpace - mZipStream.avail_out;
         dst->extend(length);

         // handle potential exception
         if(createException(ret, dst))
         {
            rval = -1;
         }
         else
         {
            // return number of output bytes
            rval = length;
         }

         if(ret == Z_STREAM_END)
         {
            // finished
            mFinished = true;
         }
      }
   }

   return rval;
}

MutationAlgorithm::Result Deflater::mutateData(
   ByteBuffer* src, ByteBuffer* dst, bool finish)
{
   MutationAlgorithm::Result rval = MutationAlgorithm::Stepped;

   if(!isFinished())
   {
      if(inputAvailable() == 0)
      {
         if(src->isEmpty() && !finish)
         {
            // more data required
            rval = MutationAlgorithm::NeedsData;
         }
         else
         {
            // set input
            setInput(src->data(), src->length(), finish);
         }
      }

      // keep processing while no output data and algorithm stepped
      while(dst->isEmpty() && rval == MutationAlgorithm::Stepped)
      {
         // try to process existing input
         int ret = process(dst, false);
         if(ret == 0)
         {
            // either request more data or algorithm is complete
            rval = (!isFinished()) ?
               MutationAlgorithm::NeedsData :
               MutationAlgorithm::CompleteTruncate;
         }
         else if(ret == -1)
         {
            // exception occurred
            rval = MutationAlgorithm::Error;
         }
         else if(isFinished())
         {
            // algorithm complete
            rval = MutationAlgorithm::CompleteTruncate;
         }

         // clear source buffer of data that has been consumed
         src->clear(src->length() - inputAvailable());
      }
   }
   else
   {
      // algorithm completed
      rval = MutationAlgorithm::CompleteTruncate;
   }

   return rval;
}

unsigned int Deflater::inputAvailable()
{
   return mZipStream.avail_in;
}

bool Deflater::isFinished()
{
   return mFinished;
}

unsigned int Deflater::getTotalInputBytes()
{
   return mZipStream.total_in;
}

unsigned int Deflater::getTotalOutputBytes()
{
   return mZipStream.total_out;
}

void Deflater::cleanupStream()
{
   // clean up previous stream
   if(mFinished)
   {
      if(mDeflating)
      {
         deflateEnd(&mZipStream);
      }
      else
      {
         inflateEnd(&mZipStream);
      }
   }

   // re-initialize stream (use defaults via Z_NULL)
   mZipStream.zalloc = Z_NULL;
   mZipStream.zfree = Z_NULL;
   mZipStream.opaque = Z_NULL;
   mZipStream.next_in = Z_NULL;
   mZipStream.avail_in = 0;
}

bool Deflater::createException(int ret, ByteBuffer* dst)
{
   bool rval = false;

   Exception* e = NULL;

   switch(ret)
   {
      case Z_OK:
         // no error
      case Z_STREAM_END:
         // no error, returned by last call to deflate()/inflate()
         break;
      case Z_BUF_ERROR:
         // returned when output buffer is too small or when there is no
         // more input ... which is only an error if we requested the
         // zip stream to finish but it couldn't even though our destination
         // buffer is not full (missing inflation data)
         if(mShouldFinish && dst != NULL && !dst->isFull())
         {
            e = new Exception(
               "Not enough source data to finish inflation.",
               EXCEPTION_DEFLATE ".MissingData");
         }
         break;
      case Z_MEM_ERROR:
         // not enough memory
         e = new Exception(
            "Not enough memory for inflation/deflation.",
            EXCEPTION_DEFLATE ".InsufficientMemory");
         break;
      case Z_VERSION_ERROR:
         // zlib library version incompatible
         e = new Exception(
            "Incompatible zlib library version.",
            EXCEPTION_DEFLATE ".IncompatibleVersion");
         break;
      case Z_STREAM_ERROR:
         // invalid stream parameters
         e = new Exception(
            "Invalid zip stream parameters. Null pointer?",
            EXCEPTION_DEFLATE ".InvalidZipStreamParams");
         break;
      default:
         // something else went wrong
         e = new Exception(
            "Could not inflate/deflate.",
            EXCEPTION_DEFLATE ".Error");
         break;
   }

   if(e != NULL)
   {
      rval = true;

      if(mZipStream.msg != NULL)
      {
         // use zlib stream error message as cause
         ExceptionRef cause = new Exception(mZipStream.msg);
         e->setCause(cause);
      }

      ExceptionRef ref = e;
      Exception::set(ref);
   }

   return rval;
}
