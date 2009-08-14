/*
 * Copyright (c) 2008-2009 Digital Bazaar, Inc. All rights reserved.
 */
#include "db/validation/Map.h"

using namespace db::rt;
using namespace db::validation;

Map::Map()
{
}

Map::Map(const char* key, ...)
{
   va_list ap;

   va_start(ap, key);
   addValidators(key, ap);
   va_end(ap);
}

Map::~Map()
{
   std::vector<std::pair<const char*,Validator*> >::iterator i;

   for(i = mValidators.begin();
      i != mValidators.end();
      i++)
   {
      delete i->second;
   }
}

bool Map::isValid(
   DynamicObject& obj,
   ValidatorContext* context)
{
   bool rval = true;

   if(!obj.isNull() && obj->getType() == db::rt::Map)
   {
      std::vector<std::pair<const char*,Validator*> >::iterator i;
      for(i = mValidators.begin(); i != mValidators.end(); i++)
      {
         // only add a "." if this is not a root map
         if(context->getDepth() != 0)
         {
            context->pushPath(".");
         }
         context->pushPath(i->first);
         if(obj->hasMember(i->first))
         {
            // do not short-circuit to ensure all keys tested
            if(!i->second->isValid(obj[i->first], context))
            {
               rval = false;
            }
         }
         else if(!i->second->isOptional(context))
         {
            rval = false;
            DynamicObject detail =
               context->addError("db.validation.MissingField", &obj);
            detail["validator"] = "db.validator.Map";
            detail["message"] = "A required field has not been specified.";
            detail["key"] = i->first;
         }
         context->popPath();
         if(context->getDepth() > 0)
         {
            context->popPath();
         }
      }
   }
   else
   {
      rval = false;
      DynamicObject detail =
         context->addError("db.validation.TypeError", &obj);
      detail["validator"] = "db.validator.Map";
      detail["message"] = "The given object type must a mapping (Map) type";
   }

   return rval;
}

void Map::addValidator(const char* key, Validator* validator)
{
   mValidators.push_back(std::make_pair(key, validator));
}

void Map::addValidators(const char* key, va_list ap)
{
   while(key != NULL)
   {
      Validator* v = va_arg(ap, Validator*);
      addValidator(key, v);
      key = va_arg(ap, const char*);
   }
}

void Map::addValidators(const char* key, ...)
{
   va_list ap;

   va_start(ap, key);
   addValidators(key, ap);
   va_end(ap);
}
