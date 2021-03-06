/*
 * Copyright (c) 2007-2010 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_crypto_CryptoHashAlgorithm_H
#define monarch_crypto_CryptoHashAlgorithm_H

#include "monarch/util/HashAlgorithm.h"

#include <openssl/evp.h>

namespace monarch
{
namespace crypto
{

/**
 * The CryptoHashAlgorithm class provides an abstract base class for
 * cryptographic HashAlgorithms. It uses OpenSSL's implementations for
 * crypographic hash algorithms.
 *
 * @author Dave Longley
 */
class CryptoHashAlgorithm : public monarch::util::HashAlgorithm
{
protected:
   /**
    * The message digest context.
    */
   EVP_MD_CTX mMessageDigestContext;

   /**
    * A pointer to the hash function.
    */
   const EVP_MD* mHashFunction;

public:
   /**
    * Creates a new CryptoHashAlgorithm.
    */
   CryptoHashAlgorithm();

   /**
    * Destructs this CryptoHashAlgorithm.
    */
   virtual ~CryptoHashAlgorithm();

protected:
   /**
    * Gets the hash function for this algorithm.
    *
    * @return the hash function to use.
    */
   virtual const EVP_MD* getHashFunction() = 0;
};

} // end namespace crypto
} // end namespace monarch
#endif
