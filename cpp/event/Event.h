/*
 * Copyright (c) 2007-2009 Digital Bazaar, Inc. All rights reserved.
 */
#ifndef db_event_Event_H
#define db_event_Event_H

#include "monarch/rt/DynamicObject.h"

namespace db
{
namespace event
{

/**
 * An Event is an object that is generated by an Observable when something
 * significant occurs. An Event may be dispatched to the Observable's
 * registered Observers so they can take whatever action they deem appropriate.
 *
 * Reserved fields:
 *
 * Event["id"]:
 *    Refers to an EventId for the Event's type.
 * Event["sequenceId"]:
 *    Refers to a global sequence in which an event occurred on a given
 *    Observable -- it increases by 1 per event.
 * Event["serial"]:
 *    Used to determine if an event must be distributed before
 *    events that follow it.
 * Event["parallel"]:
 *    Used to determine if an event can be distributed at any moment,
 *    regardless of other events.
 * Event["details"]:
 *    A map that should contain any user details for the event.
 *
 * @author Dave Longley
 */
typedef monarch::rt::DynamicObject Event;
typedef uint64_t EventId;

/**
 * An EventFilter can be used to filter events that are received by an
 * Observer. It is a DynamicObject map that is a subset of data that must
 * be present in an Event in order for an Observer to receive it.
 *
 * @author Dave Longley
 */
typedef monarch::rt::DynamicObject EventFilter;

} // end namespace event
} // end namespace db
#endif
