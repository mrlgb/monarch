/*
 * Copyright (c) 2007-2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/net/DatagramService.h"

#include "monarch/net/Server.h"

using namespace monarch::modest;
using namespace monarch::net;

DatagramService::DatagramService(
   Server* server, InternetAddress* address, DatagramServicer* servicer,
   const char* name) :
   PortService(server, address, name),
   mServicer(servicer),
   mSocket(NULL)
{
}

DatagramService::~DatagramService()
{
   // ensure service is stopped
   DatagramService::stop();
}

void DatagramService::run()
{
   // service datagrams
   mServicer->serviceDatagrams(mSocket, mOperation);

   // close socket
   mSocket->close();
}

Operation DatagramService::initialize()
{
   Operation rval(NULL);

   // create datagram socket
   mSocket = new DatagramSocket();

   // bind socket to the address
   if(mSocket->bind(getAddress()) && mServicer->initialize(mSocket))
   {
      // create Operation for running service
      rval = *this;
   }

   return rval;
}

void DatagramService::cleanup()
{
   delete mSocket;
   mSocket = NULL;
}
