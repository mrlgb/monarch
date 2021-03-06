/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef monarch_modest_Operation_H
#define monarch_modest_Operation_H

#include "monarch/rt/Collectable.h"
#include "monarch/modest/OperationImpl.h"

namespace monarch
{
namespace modest
{

/**
 * An Operation is some set of instructions that can be executed by an
 * Engine provided that its current State is compatible with this Operation's
 * OperationGuard. An Operation may change the current State of the
 * Engine that executes it.
 *
 * Operations running on the same Engine share its State information and can
 * therefore specify the conditions underwhich they can execute. This provides
 * developers with an easy yet powerful way to protect resources and restrict
 * the flow of their code such that it only executes in the specific manner
 * desired.
 *
 * The Operation type functions as a reference-counting pointer for a
 * heap-allocated Operation implementation (a OperationImpl). When no more
 * references to a given heap-allocated OperationImpl exist, it will be freed.
 *
 * @author Dave Longley
 */
class Operation : public monarch::rt::Collectable<OperationImpl>
{
public:
   /**
    * Creates a new Operation with the given Runnable.
    *
    * @param r the Runnable to use (which can be NULL to only mutate state).
    */
   Operation(monarch::rt::Runnable& r);
   Operation(monarch::rt::RunnableRef& r);

   /**
    * Creates a new Operation reference to the passed OperationImpl.
    *
    * @param impl the OperationImpl to reference.
    */
   Operation(OperationImpl* impl = NULL);

   /**
    * Destructs this Operation.
    */
   virtual ~Operation();
};

} // end namespace modest
} // end namespace monarch
#endif
