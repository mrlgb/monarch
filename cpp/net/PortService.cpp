/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/net/PortService.h"

#include "monarch/net/Server.h"

#include <cstdlib>

using namespace monarch::modest;
using namespace monarch::net;
using namespace monarch::rt;

PortService::PortService(
   Server* server, InternetAddress* address, const char* name) :
   mServer(server),
   mAddress(address),
   mOperation(NULL)
{
   mName = strdup(name);
}

PortService::~PortService()
{
   free(mName);
}

bool PortService::start()
{
   if(!mOperation.isNull())
   {
      // stop service
      stop();
   }

   // initialize service
   mOperation = initialize();
   if(!mOperation.isNull())
   {
      // run service
      mServer->getOperationRunner()->runOperation(mOperation);
   }
   else
   {
      // set exception
      ExceptionRef e = new Exception(
         "Port service failed to start.",
         "monarch.net.PortService.StartFailed");
      e->getDetails()["name"] = mName;
      Exception::push(e);

      // clean up service
      cleanup();
   }

   return !mOperation.isNull();
}

void PortService::interrupt()
{
   if(!mOperation.isNull())
   {
      // interrupt service
      mOperation->interrupt();
   }
}

void PortService::stop()
{
   if(!mOperation.isNull())
   {
      // stop service
      mOperation->interrupt();
      mOperation->waitFor(false);
      mOperation.setNull();
   }

   // clean up service
   cleanup();
}

inline InternetAddress* PortService::getAddress()
{
   return mAddress;
}
