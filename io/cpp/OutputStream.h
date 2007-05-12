/*
 * Copyright (c) 2007 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef OutputStream_H
#define OutputStream_H

#include "Object.h"
#include "IOException.h"

namespace db
{
namespace io
{

/**
 * An OutputStream is the abstract base class for all classes that represent an
 * output stream of bytes.
 * 
 * @author Dave Longley
 */
class OutputStream : public virtual db::rt::Object
{
public:
   /**
    * Creates a new OutputStream.
    */
   OutputStream() {};
   
   /**
    * Destructs this OutputStream.
    */
   virtual ~OutputStream() {};
   
   /**
    * Writes a single byte to the stream.
    * 
    * @param b the byte to write.
    * 
    * @exception IOException thrown if an IO error occurs. 
    */
   virtual void write(const char& b) throw(IOException) = 0;
   
   /**
    * Writes some bytes to the stream.
    * 
    * @param b the array of bytes to write.
    * @param length the number of bytes to write to the stream.
    * 
    * @exception IOException thrown if an IO error occurs. 
    */
   virtual void write(const char* b, unsigned int length)
   throw(IOException);
   
   /**
    * Closes the stream.
    * 
    * @exception IOException thrown if an IO error occurs.
    */
   virtual void close() throw(IOException);
};

inline void OutputStream::write(const char* b, unsigned int length)
throw(IOException)
{
   for(unsigned int i = 0; i < length; i++)
   {
      write(b[i]);
   }
}

inline void OutputStream::close() throw(IOException)
{
   // nothing to do in base class
}

} // end namespace io
} // end namespace db
#endif
