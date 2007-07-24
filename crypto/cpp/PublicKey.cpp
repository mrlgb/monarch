/*
 * Copyright (c) 2007 Digital Bazaar, Inc.  All rights reserved.
 */
#include "PublicKey.h"

using namespace std;
using namespace db::crypto;

PublicKey::PublicKey(EVP_PKEY* pkey) : AsymmetricKey(pkey)
{
}

PublicKey::~PublicKey()
{
}

DigitalEnvelope* PublicKey::createEnvelope(
   const std::string& algorithm, SymmetricKey** key)
{
   DigitalEnvelope* rval = new DigitalEnvelope();
   
   // start sealing
   if(!rval->startSealing(algorithm, this, key))
   {
      // seal failed, delete envelope
      delete rval;
      rval = NULL;
   }
   
   return rval;
}

DigitalSignature* PublicKey::createSignature()
{
   DigitalSignature* rval = new DigitalSignature(this);
   return rval;
}
