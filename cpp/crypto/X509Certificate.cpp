/*
 * Copyright (c) 2010 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/crypto/X509Certificate.h"

#include <openssl/evp.h>
#include <openssl/x509v3.h>

using namespace monarch::crypto;
using namespace monarch::rt;

X509Certificate::X509Certificate(X509* x509) :
   mX509(x509),
   mPublicKey(NULL)
{
}

X509Certificate::~X509Certificate()
{
   // free the X509 data structure
   X509_free(mX509);
}

X509* X509Certificate::getX509()
{
   return mX509;
}

int64_t X509Certificate::getVersion()
{
   return X509_get_version(mX509);
}

PublicKeyRef& X509Certificate::getPublicKey()
{
   if(mPublicKey.isNull())
   {
      // get EVP_PKEY from x509
      EVP_PKEY* pkey = X509_get_pubkey(mX509);
      if(pkey != NULL)
      {
         mPublicKey = new PublicKey(pkey);
      }
   }

   return mPublicKey;
}

/**
 * Gets the field names and values for a particular X509_NAME.
 *
 * For instance, if the subject name is passed, then the "CN" (common name)
 * value, "C" (country) value, etc. will be added to the output map.
 *
 * @param name the X509_name, i.e. X509_get_subject_name(mX509).
 * @param output the map to populate.
 */
static void _getX509NameValues(X509_NAME* name, DynamicObject& output)
{
   output->setType(Map);

   unsigned char* value;
   X509_NAME_ENTRY* entry;
   int count = X509_NAME_entry_count(name);
   for(int i = 0; i < count; i++)
   {
      entry = X509_NAME_get_entry(name, i);

      // get entry name (object) and value (data)
      ASN1_OBJECT* obj = X509_NAME_ENTRY_get_object(entry);
      ASN1_STRING* str = X509_NAME_ENTRY_get_data(entry);

      // convert name and value to strings
      int nid = OBJ_obj2nid(obj);
      const char* sn = OBJ_nid2sn(nid);
      if(ASN1_STRING_to_UTF8(&value, str) != -1)
      {
         output[sn] = value;
         OPENSSL_free(value);
      }
   }
}

DynamicObject X509Certificate::getSubject()
{
   // build subject
   DynamicObject rval;
   _getX509NameValues(X509_get_subject_name(mX509), rval);
   return rval;
}

DynamicObject X509Certificate::getIssuer()
{
   // build issuer
   DynamicObject rval;
   _getX509NameValues(X509_get_issuer_name(mX509), rval);
   return rval;
}

DynamicObject X509Certificate::getExtensions()
{
   DynamicObject rval;
   rval->setType(Map);

   int count = X509_get_ext_count(mX509);
   for(int i = 0; i < count; i++)
   {
      // get extension and v3 extension method
      X509_EXTENSION* ext = X509_get_ext(mX509, i);
      X509V3_EXT_METHOD* method = X509V3_EXT_get(ext);
      if(method != NULL)
      {
         // get extension name
         ASN1_OBJECT* obj = X509_EXTENSION_get_object(ext);
         int nid = OBJ_obj2nid(obj);
         const char* name = OBJ_nid2sn(nid);

         // convert data into extension stack pointer
         void* stackPtr;
         const unsigned char* data = ext->value->data;

         // see if the item pointer is set
         if(method->it != NULL)
         {
            stackPtr = ASN1_item_d2i(
               NULL, &data, ext->value->length, ASN1_ITEM_ptr(method->it));
         }
         else
         {
            stackPtr = method->d2i(NULL, &data, ext->value->length);
         }

         // get extension value stack
         DynamicObject values;
         STACK_OF(CONF_VALUE)* stack = method->i2v(method, stackPtr, NULL);
         for(int n = 0; n < sk_CONF_VALUE_num(stack); n++)
         {
            CONF_VALUE* nval = sk_CONF_VALUE_value(stack, n);
            DynamicObject d;
            if(nval->section)
            {
               d["section"] = nval->section;
            }
            if(nval->name)
            {
               d["name"] = nval->name;
            }
            if(nval->value)
            {
               d["value"] = nval->value;
            }
            values->append(d);
         }

         // add values
         rval[name] = values;
      }
   }

   return rval;
}
