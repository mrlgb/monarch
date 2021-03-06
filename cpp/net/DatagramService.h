/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_net_DatagramService_H
#define monarch_net_DatagramService_H

#include "monarch/net/PortService.h"
#include "monarch/net/DatagramServicer.h"

namespace monarch
{
namespace net
{

/**
 * A DatagramService binds to an address to communicate using Datagrams.
 *
 * @author Dave Longley
 */
class DatagramService : public PortService
{
protected:
   /**
    * The DatagramServicer to use.
    */
   DatagramServicer* mServicer;

   /**
    * The Socket for this service.
    */
   DatagramSocket* mSocket;

public:
   /**
    * Creates a new DatagramService that uses the passed address for
    * communication.
    *
    * @param server the Server this service is for.
    * @param address the address to bind to.
    * @param servicer the DatagramServicer to service datagrams with.
    * @param name a name for this service.
    */
   DatagramService(
      Server* server,
      InternetAddress* address,
      DatagramServicer* servicer,
      const char* name = "unnamed");

   /**
    * Destructs this DatagramService.
    */
   virtual ~DatagramService();

   /**
    * Runs this DatagramService.
    */
   virtual void run();

protected:
   /**
    * Initializes this service and creates the Operation for running it,
    * typically through the Server's OperationRunner. If the service could
    * not be initialized, an exception should be set on the current thread
    * indicating the reason why the service could not be initialized.
    *
    * @return the Operation for running this service, or NULL if the
    *         service could not be initialized.
    */
   virtual monarch::modest::Operation initialize();

   /**
    * Called to clean up resources for this service that were created or
    * obtained via a call to initialize(). If there are no resources to
    * clean up, then this method should have no effect.
    */
   virtual void cleanup();
};

} // end namespace net
} // end namespace monarch
#endif
