/*
 * Copyright (c) 2007-2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/io/MutatorInputStream.h"

#include "monarch/rt/Exception.h"

using namespace monarch::io;
using namespace monarch::rt;

MutatorInputStream::MutatorInputStream(
   InputStream* is, bool cleanupStream,
   MutationAlgorithm* algorithm, bool cleanupAlgorithm,
   ByteBuffer* src, ByteBuffer* dst) :
   FilterInputStream(is, cleanupStream),
   mAlgorithm(algorithm),
   mCleanupAlgorithm(cleanupAlgorithm),
   mResult(MutationAlgorithm::NeedsData),
   mSourceEmpty(false)
{
   // set source buffer
   if(src == NULL)
   {
      mSource = new ByteBuffer(2048);
      mCleanupSource = true;
   }
   else
   {
      mSource = src;
      mCleanupSource = false;
   }

   // set destination buffer
   if(dst == NULL)
   {
      mDestination = new ByteBuffer(4096);
      mCleanupDestination = true;
   }
   else
   {
      mDestination = src;
      mCleanupDestination = false;
   }
}

MutatorInputStream::~MutatorInputStream()
{
   if(mCleanupSource)
   {
      delete mSource;
   }

   if(mCleanupDestination)
   {
      delete mDestination;
   }

   if(mCleanupAlgorithm)
   {
      delete mAlgorithm;
   }
}

int MutatorInputStream::read(char* b, int length)
{
   int rval = 0;

   // mutate while no data is available and algorithm not complete
   while(rval == 0 && mResult < MutationAlgorithm::CompleteAppend)
   {
      // try to mutate data
      mResult = mAlgorithm->mutateData(mSource, mDestination, mSourceEmpty);
      switch(mResult)
      {
         case MutationAlgorithm::NeedsData:
            if(mSource->isFull() || mSourceEmpty)
            {
               // no more data available for algorithm
               mResult = MutationAlgorithm::Error;
               ExceptionRef e = new Exception(
                  "Insufficient data for mutation algorithm.",
                  "monarch.io.MutationException");
               Exception::set(e);
               rval = -1;
            }
            else
            {
               // read more data from underlying stream
               int numBytes = mSource->fill(mInputStream);
               mSourceEmpty = (numBytes == 0);
               if(numBytes < 0)
               {
                  // error reading from underlying stream
                  mResult = MutationAlgorithm::Error;
                  rval = -1;
               }
            }
            break;
         case MutationAlgorithm::Error:
            rval = -1;
            break;
         default:
            // set rval to available data
            rval = mDestination->length();
            break;
      }
   }

   // if the algorithm has completed, handle any excess source data
   if(mResult >= MutationAlgorithm::CompleteAppend)
   {
      if(mResult == MutationAlgorithm::CompleteAppend)
      {
         // empty source into destination
         mSource->get(mDestination, mSource->length(), true);

         // get bytes from destination
         rval = mDestination->get(b, length);

         if(rval == 0 && !mSourceEmpty)
         {
            // read bytes directly into passed buffer
            rval = mInputStream->read(b, length);
         }
      }
      else
      {
         // get remaining data from destination, do not populate source again
         rval = mDestination->get(b, length);
      }
   }
   else if(mResult != MutationAlgorithm::Error)
   {
      // get data from destination buffer
      rval = mDestination->get(b, length);
   }

   return rval;
}

void MutatorInputStream::setAlgorithm(MutationAlgorithm* ma, bool cleanup)
{
   if(mCleanupAlgorithm)
   {
      delete mAlgorithm;
   }
   mAlgorithm = ma;
   mCleanupAlgorithm = cleanup;
   mResult = MutationAlgorithm::NeedsData;
   mSourceEmpty = false;
}

MutationAlgorithm* MutatorInputStream::getAlgorithm()
{
   return mAlgorithm;
}
