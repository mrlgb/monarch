/*
 * Copyright (c) 2008-2011 Digital Bazaar, Inc. All rights reserved.
 */
#include "monarch/validation/Regex.h"

#include "monarch/util/Pattern.h"
#include "monarch/validation/Type.h"

#include <cstdlib>

using namespace monarch::rt;
using namespace monarch::util;
using namespace monarch::validation;

Regex::Regex(const char* regex, const char* errorMessage) :
   Validator(errorMessage)
{
   mRegex = regex ? strdup(regex) : strdup("^$");
   mStringValidator = new Type(String);
}

Regex::~Regex()
{
   free(mRegex);
   delete mStringValidator;
}

bool Regex::isValid(
   monarch::rt::DynamicObject& obj,
   ValidatorContext* context)
{
   bool rval = mStringValidator->isValid(obj, context);

   if(rval)
   {
      // FIXME compile the regex
      rval = Pattern::match(mRegex, obj->getString());
      if(!rval)
      {
         DynamicObject detail =
            context->addError("monarch.validation.ValueError", &obj);
         detail["validator"] = "monarch.validator.Regex";
         if(mErrorMessage)
         {
            detail["message"] = mErrorMessage;
         }
      }
      else
      {
         context->addSuccess();
      }
   }

   return rval;
}
