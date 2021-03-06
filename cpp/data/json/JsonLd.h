/*
 * Copyright (c) 2010-2012 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_data_json_JsonLd_H
#define monarch_data_json_JsonLd_H

#include "monarch/rt/DynamicObject.h"

#include <string>

namespace monarch
{
namespace data
{
namespace json
{

/**
 * The JsonLd class provides APIs for working with JSON-LD objects.
 *
 * @author Dave Longley
 */
class JsonLd
{
protected:
   /**
    * Creates a new JsonLd.
    */
   JsonLd();

public:
   /**
    * Destructs this JsonLd.
    */
   virtual ~JsonLd();

   /**
    * Performs JSON-LD compaction.
    *
    * @param input the JSON-LD object to compact.
    * @param ctx the context to compact with.
    * @param options compaction options:
    *           base: the base IRI to use.
    *           optimize: true to turn on optimization (default: false).
    * @param output to be set to the JSON-LD compacted output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool compact(
      monarch::rt::DynamicObject input,
      monarch::rt::DynamicObject ctx,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Performs JSON-LD expansion.
    *
    * @param input the JSON-LD object to expand.
    * @param options expansion options.
    *           base: the base IRI to use.
    * @param output to be set to the JSON-LD expanded output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool expand(
      monarch::rt::DynamicObject input,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Performs JSON-LD framing.
    *
    * @param input the JSON-LD object to frame.
    * @param frame the JSON-LD frame to use.
    * @param options the framing options.
    *           base: the base IRI to use.
    * @param output to be set to the JSON-LD framed output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool frame(
      monarch::rt::DynamicObject input,
      monarch::rt::DynamicObject frame,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Performs RDF normalization on the given JSON-LD input.
    *
    * @param input the JSON-LD object to normalize.
    * @param options normalization options.
    *           base: the base IRI to use.
    *           format: the format if output is a string:
    *              "application/nquads" for N-Quads (default).
    * @param output to be set to the normalized output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool normalize(
      monarch::rt::DynamicObject input,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Converts RDF statements into JSON-LD.
    *
    * @param statements a serialized string of RDF statements in a format
    *           specified by the format option or an array of the RDF
    *           statements to convert.
    * @param options the options to use:
    *           format: the format if input is not an array:
    *              "application/nquads" for N-Quads (default).
    *           notType: true to use rdf:type, false to use @type (default).
    * @param the JSON-LD output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool fromRdf(
      monarch::rt::DynamicObject statements,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Outputs the RDF statements found in the given JSON-LD object.
    *
    * @param input the JSON-LD input.
    * @param options toRdf options.
    *           base: the base IRI to use.
    *           format: the format to use to output a string:
    *              "applications/nquads" for N-Quads (default).
    * @param output to be set to the RDF statement output.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool toRdf(
      monarch::rt::DynamicObject input,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Processes a local context, resolving any URLs as necessary, and returns a
    * new active context.
    *
    * @param activeCtx the current active context.
    * @param localCtx the local context to process.
    * @param options processing options.
    * @param output to be set to the merged context.
    *
    * @return true on success, false on failure with exception set.
    */
   static bool processContext(
      monarch::rt::DynamicObject activeCtx,
      monarch::rt::DynamicObject localCtx,
      monarch::rt::DynamicObject options,
      monarch::rt::DynamicObject& output);

   /**
    * Returns true if the given subject has the given property.
    *
    * @param subject the subject to check.
    * @param property the property to look for.
    *
    * @return true if the subject has the given property, false if not.
    */
   static bool hasProperty(
      monarch::rt::DynamicObject& subject, const char* property);

   /**
    * Determines if the given value is a property of the given subject.
    *
    * @param subject the subject to check.
    * @param property the property to check.
    * @param value the value to check.
    *
    * @return true if the value exists, false if not.
    */
   static bool hasValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      monarch::rt::DynamicObject value);
   static bool hasValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      const char* value);

   /**
    * Adds a value to a subject. If the subject already has the value, it will
    * not be added. If the value is an array, all values in the array will be
    * added.
    *
    * Note: If the value is a subject that already exists as a property of the
    * given subject, this method makes no attempt to deeply merge properties.
    * Instead, the value will not be added.
    *
    * @param subject the subject to add the value to.
    * @param property the property that relates the value to the subject.
    * @param value the value to add.
    * @param [propertyIsArray] true if the property is always an array, false
    *           if not (default: false).
    * @param [propertyIsList] true if the property is a @list, false
    *           if not (default: false).
    */
   static void addValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      monarch::rt::DynamicObject value,
      bool propertyIsArray = false,
      bool propertyIsList = false);
   static void addValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      const char* value,
      bool propertyIsArray = false,
      bool propertyIsList = false);

   /**
    * Gets all of the values for a subject's property as an array.
    *
    * @param subject the subject.
    * @param property the property.
    *
    * @return all of the values for a subject's property as an array.
    */
   static monarch::rt::DynamicObject getValues(
      monarch::rt::DynamicObject& subject, const char* property);

   /**
    * Removes a property from a subject.
    *
    * @param subject the subject.
    * @param property the property.
    */
   static void removeProperty(
      monarch::rt::DynamicObject& subject, const char* property);

   /**
    * Removes a value from a subject.
    *
    * @param subject the subject.
    * @param property the property that relates the value to the subject.
    * @param value the value to remove.
    * @param [propertyIsArray] true if the property is always an array, false
    *           if not (default: false).
    */
   static void removeValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      monarch::rt::DynamicObject value,
      bool propertyIsArray = false);
   static void removeValue(
      monarch::rt::DynamicObject& subject,
      const char* property,
      const char* value,
      bool propertyIsArray = false);

   /**
    * Compares two JSON-LD values for equality. Two JSON-LD values will be
    * considered equal if:
    *
    * 1. They are both primitives of the same type and value.
    * 2. They are both @values with the same @value, @type, and @language, OR
    * 3. They both have @ids they are the same.
    *
    * @param v1 the first value.
    * @param v2 the second value.
    *
    * @return true if v1 and v2 are considered equal, false if not.
    */
   static bool compareValues(
      monarch::rt::DynamicObject& v1, monarch::rt::DynamicObject& v2);

   /**
    * Gets the value for the given @context key and type, null if none is set.
    *
    * @param ctx the active context.
    * @param key the context key.
    * @param type the type of value to get (eg: '@id', '@type'), if NULL
    *           gets the entire entry for a key, NULL if not found.
    *
    * @return the value.
    */
   static monarch::rt::DynamicObject getContextValue(
      monarch::rt::DynamicObject ctx,
      const char* key, const char* type);
};

} // end namespace json
} // end namespace data
} // end namespace monarch
#endif
