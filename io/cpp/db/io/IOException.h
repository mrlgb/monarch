/*
 * Copyright (c) 2007 Digital Bazaar, Inc.  All rights reserved.
 */
#ifndef db_io_IOException_H
#define db_io_IOException_H

#include "db/rt/Exception.h"

namespace db
{
namespace io
{

/**
 * An IOException is raised when some kind of IO error occurs.
 *
 * @author Dave Longley
 */
class IOException : public db::rt::Exception
{
protected:
   /**
    * Stores the number of used bytes (read/written) during an IO operation.
    */
   int mUsedBytes;
   
   /**
    * Stores the number of unused bytes (unread/unwritten) from an IO
    * operation.
    */
   int mUnusedBytes;
   
public:
   /**
    * Creates a new IOException.
    *
    * A message and code may be optionally specified.
    *
    * @param message the message for this Exception.
    * @param code the code for this Exception.
    */
   IOException(const char* message = NULL, const char* code = NULL);
   
   /**
    * Destructs this IOException.
    */
   virtual ~IOException();
   
   /**
    * Sets the number of used bytes (read/written) from an IO operation.
    * 
    * @param used the number of used bytes (read or written).
    */
   virtual void setUsedBytes(int used);
   
   /**
    * Gets the number of used bytes (read/written) from an IO operation.
    * 
    * @return the number of used bytes (read or written).
    */
   virtual int getUsedBytes();
   
   /**
    * Sets the number of unused bytes (unread/unwritten) from an IO operation.
    * 
    * @param unused the number of unused bytes (unread or unwritten).
    */
   virtual void setUnusedBytes(int unused);
   
   /**
    * Gets the number of unused bytes (unread/unwritten) from an IO operation.
    * 
    * @return the number of unused bytes (unread or unwritten).
    */
   virtual int getUnusedBytes();
};

} // end namespace io
} // end namespace db
#endif
