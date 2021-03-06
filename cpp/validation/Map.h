/*
 * Copyright (c) 2008-2011 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_validation_Map_H
#define monarch_validation_Map_H

#include "monarch/validation/Validator.h"
#include <vector>
#include <utility>

namespace monarch
{
namespace validation
{

/**
 * Validates specific members of a DynamicObject Map.  All keys listed are
 * checked regardless of validity of other keys.  This can be used to generate
 * error messages for all keys at once.
 *
 * For many values set up the validator with varargs:
 *
 *   Map m(
 *      "username", new Type(String),
 *      "password", new Type(String),
 *      NULL);
 *   m.isValid(dyno);
 *
 * NOTE: Checking for arbitrary extra members not yet implemented.  However,
 * you can set a validator such as: "new Optional(new NotValid())" in order
 * to fail on specific members that are not allowed.
 *
 * @author David I. Lehn
 */
class Map : public Validator
{
protected:
   struct Entry
   {
      Validator* validator;
      ValidatorRef reference;
   };
   typedef std::vector<std::pair<const char*,Entry> > Validators;
   Validators mValidators;

public:
   /**
    * Creates a new validator.
    */
   Map();

   /**
    * Creates a new validator with a NULL key terminated key:validator list.
    */
   Map(const char* key, ...);

   /**
    * Destructs this validator.
    */
   virtual ~Map();

   /**
    * Checks if an object is valid.
    *
    * @param obj the object to validate.
    * @param context context to use during validation.
    *
    * @return true if obj is valid, false and exception set otherwise.
    */
   virtual bool isValid(
      monarch::rt::DynamicObject& obj,
      ValidatorContext* context);
   using Validator::isValid;

   /**
    * Returns the number of validators in the map.
    *
    * @return the length of the validator.
    */
   virtual size_t length();

   /**
    * Adds a key:validator pair.
    *
    * @param key a map key.
    * @param validator a Validator.
    */
   virtual void addValidator(const char* key, Validator* validator);

   /**
    * Adds a key:validator pair.
    *
    * @param key a map key.
    * @param validator a Validator.
    */
   virtual void addValidatorRef(const char* key, ValidatorRef validator);

   /**
    * Adds key:validator pairs.
    *
    * @param key a map key.
    * @param ap a vararg list.
    */
   virtual void addValidators(const char* key, va_list ap);

   /**
    * Adds a NULL terminated list of key:validator pairs.
    *
    * @param key a map key.
    * @param ... more key:validator pairs.
    */
   virtual void addValidators(const char* key, ...);
};

} // end namespace validation
} // end namespace monarch
#endif
