/*
 * Copyright (c) 2007 Digital Bazaar, Inc.  All rights reserved.
 */
#include "SslContext.h"

using namespace db::net;

SslContext::SslContext(const std::string& protocol)
{
   // FIXME: handle protocol "SSLv2/SSLv3/TLS"
   
   // create SSL context object
   mContext = SSL_CTX_new(SSLv23_method());
   
   // turn on all options (this enables a bunch of bug fixes for various
   // SSL implementations that may communicate with sockets created in
   // this context)
   SSL_CTX_set_options(mContext, SSL_OP_ALL);
}

SslContext::~SslContext()
{
   // free context
   if(mContext != NULL)
   {
      SSL_CTX_free(mContext);
   }
}

SSL* SslContext::createSSL(TcpSocket* socket, bool client)
{
   SSL* ssl = SSL_new(mContext);
   
   // set connect state on SSL
   if(client)
   {
      SSL_set_connect_state(ssl);
   }
   else
   {
      SSL_set_accept_state(ssl);
   }
   
   return ssl;
}
