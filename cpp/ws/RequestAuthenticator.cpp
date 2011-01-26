/*
 * Copyright (c) 2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/ws/RequestAuthenticator.h"

using namespace monarch::rt;
using namespace monarch::ws;

RequestAuthenticator::RequestAuthenticator()
{
}

RequestAuthenticator::~RequestAuthenticator()
{
}

bool RequestAuthenticator::checkAuthentication(ServiceChannel* ch)
{
   // do anonymous authentication
   ch->setAuthenticationMethod(NULL, NULL);
   return true;
}