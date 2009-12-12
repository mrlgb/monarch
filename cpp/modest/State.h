/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef db_modest_State_H
#define db_modest_State_H

#include "monarch/modest/ImmutableState.h"
#include "monarch/rt/ExclusiveLock.h"

#include <map>
#include <cstring>

namespace db
{
namespace modest
{

/**
 * A State maintains the current information about a Modest Engine. It can
 * be modified by an Operation.
 *
 * @author Dave Longley
 */
class State : public virtual monarch::rt::ExclusiveLock, public ImmutableState
{
protected:
   /**
    * A VarNameComparator compares two state Variable names.
    */
   struct VarNameComparator
   {
      /**
       * Compares two null-terminated strings, returning true if the first is
       * less than the second, false if not.
       *
       * @param s1 the first string.
       * @param s2 the second string.
       *
       * @return true if the s1 < s2, false if not.
       */
      bool operator()(const char* s1, const char* s2) const
      {
         return strcmp(s1, s2) < 0;
      }
   };

   /**
    * A Variable is a boolean, 32-bit signed integer, or a string.
    */
   struct Variable
   {
      /**
       * The type of variable this is.
       */
      enum Type {Boolean, Integer, String} type;

      /**
       * The actual value of this variable.
       */
      union
      {
         bool b;
         int i;
         std::string* s;
      };
   };

   /**
    * The variable table.
    */
   std::map<const char*, Variable*, VarNameComparator> mVarTable;

   /**
    * Gets an existing Variable by its name.
    *
    * @param name the name of the Variable to retrieve.
    *
    * @return an existing Variable or NULL.
    */
   virtual Variable* getVariable(const char* name);

   /**
    * Creates a new Variable with the given name if it does not
    * exist.
    *
    * @param name the name of the Variable to create.
    * @param type the type of variable.
    *
    * @return the Variable.
    */
   virtual Variable* createVariable(const char* name, Variable::Type type);

   /**
    * Frees an existing Variable.
    *
    * @param var the Variable to free.
    */
   virtual void freeVariable(Variable* var);

public:
   /**
    * Creates a new State.
    */
   State();

   /**
    * Destructs this State.
    */
   virtual ~State();

   /**
    * Sets a boolean in this State.
    *
    * @param name the name of the boolean to set.
    * @param value the value of the boolean.
    */
   virtual void setBoolean(const char* name, bool value);

   /**
    * Gets a boolean from this State by its name.
    *
    * @param name the name of the boolean to retrieve.
    * @param value a boolean to set to the value of the boolean.
    *
    * @return true if the boolean exists, false if not.
    */
   virtual bool getBoolean(const char* name, bool& value);

   /**
    * Sets a 32-bit signed integer in this State.
    *
    * @param name the name of the integer to set.
    * @param value the value of the integer.
    */
   virtual void setInteger(const char* name, int32_t value);

   /**
    * Increases a 32-bit signed integer in this State.
    *
    * @param name the name of the integer to increase.
    * @param amount the amount to increase the integer by, can be negative.
    * @param value set to the integers new value (if care).
    *
    * @return true if the variable existed and was changed, false if not.
    */
   virtual bool increaseInteger(
      const char* name, int32_t amount, int32_t* value = NULL);

   /**
    * Gets a 32-bit signed integer from this State by its name.
    *
    * @param name the name of the integer to retrieve.
    * @param value an integer to set to the value of the integer.
    *
    * @return true if the integer exists, false if not.
    */
   virtual bool getInteger(const char* name, int32_t& value);

   /**
    * Gets the difference between two 32-bit signed integers from this State.
    *
    * @param name1 the name of the first integer to retrieve.
    * @param name2 the name of the second integer to retrieve.
    * @param value the difference between the two integers (name1 - name2).
    *
    * @return true if the integer exists, false if not.
    */
   virtual bool getIntegerDifference(
      const char* name1, const char* name2, int32_t& value);

   /**
    * Sets a string in this State. The string will be copied to this
    * State's internal storage.
    *
    * @param name the name of the string to set.
    * @param value the value of the string.
    */
   virtual void setString(const char* name, const std::string& value);

   /**
    * Gets a string from this State by its name.
    *
    * @param name the name of the string to retrieve.
    * @param value a string to set to the value of the string.
    *
    * @return true if the string exists, false if not.
    */
   virtual bool getString(const char* name, std::string& value);

   /**
    * Removes a variable from this state by its name.
    *
    * @param name the name of the variable to remove.
    */
   virtual void removeVariable(const char* name);
};

} // end namespace modest
} // end namespace db
#endif
