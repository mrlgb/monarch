/*
 * Copyright (c) 2008 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef db_io_MutatorOutputStream_H
#define db_io_MutatorOutputStream_H

#include "db/io/FilterOutputStream.h"
#include "db/io/MutationAlgorithm.h"

namespace db
{
namespace io
{

/**
 * A MutatorOutputStream uses a MutationAlgorithm to mutate data as it is
 * written to this stream.
 * 
 * @author Dave Longley
 */
class MutatorOutputStream : public FilterOutputStream
{
protected:
   /**
    * An internal buffer for storing data written to this stream that is
    * unmutated.
    */
   ByteBuffer mSource;
   
   /**
    * An internal buffer for storing mutated data.
    */
   ByteBuffer mDestination;
   
   /**
    * A ByteBuffer used as a wrapper for source bytes to improve performance
    * when input data doesn't need to be cached.
    */
   ByteBuffer mInputWrapper;
   
   /**
    * The algorithm used to mutate data.
    */
   MutationAlgorithm* mAlgorithm;
   
   /**
    * Stores the last mutation result.
    */
   MutationAlgorithm::Result mResult;
   
public:
   /**
    * Creates a new MutatorOutputStream that mutates data with the passed
    * DataMutationAlgorithm.
    * 
    * @param os the OutputStream to mutated data to.
    * @param algorithm the DataMutationAlgorithm to use.
    * @param cleanup true to clean up the passed OutputStream when destructing,
    *                false not to.
    */
   MutatorOutputStream(
      OutputStream* is, MutationAlgorithm* algorithm, bool cleanup = false);
   
   /**
    * Destructs this MutatorOutputStream.
    */
   virtual ~MutatorOutputStream();
   
   /**
    * Writes some bytes to the stream.
    * 
    * Note: Passing a length of 0 will signal the underlying mutation algorithm
    * to finish.
    * 
    * @param b the array of bytes to write.
    * @param length the number of bytes to write to the stream.
    * 
    * @return true if the write was successful, false if an IO exception
    *         occurred. 
    */
   virtual bool write(const char* b, int length);
   
   /**
    * Closes the stream and ensures the mutation algorithm finished.
    */
   virtual void close();
};

} // end namespace io
} // end namespace db
#endif