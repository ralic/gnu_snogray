// trace.h -- Tracing path
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __TRACE_H__
#define __TRACE_H__

#include "medium.h"


namespace snogray {


class TraceContext;
class TraceCache;


// A node in a tracing path.
//
// A Trace object corresponds to a single segment in the tracing path,
// starting from a vertex where it continues from the previous segment.
//
// The Trace::type field describes the type of the originating vertex of
// the segment, usually describing the mechanism by which the direction
// change happened (or SPONTANEOUS meaning there is no previous segment).
//
class Trace
{
public:

  // The type of transition from one Trace node to another.
  //
  enum Type {
    SPONTANEOUS,
    REFLECTION,
    REFRACTION_IN,		// entering a transparent surface
    REFRACTION_OUT,		// exiting it
    NUM_TRACE_TYPES
  };


  // Constructor for a root (camera/eye) Trace.
  //
  Trace (TraceContext &_context, TraceCache &_root_cache);

  // Constructor for sub-traces
  //
  Trace (float branch_factor, Type type, const Medium &_medium,
	 const Trace &_source);

  // Searches back through the trace history to find the enclosing medium.
  //
  const Medium &enclosing_medium () const;

  // Return the depth of tracing at this trace, i.e., the number of
  // segments from the first segment to this one, inclusive.
  // The value will be 1 for a camera/eye ray.
  //
  unsigned depth () const
  {
    unsigned d = 1;
    for (const Trace *t = source; t; t = t->source)
      d++;
    return d;
  }


  // The previous segment in the trace path, or zero if this is the first
  // segment (in which case, the type should be SPONTANEOUS).
  //
  const Trace *source;

  // Stuff that's only allocated once.  The context is generally
  // modifiable (e.g., for memory allocation or stats) even when a Trace
  // is const, so mark it as mutable.
  //
  mutable TraceContext &context;

  // The type of transition from the previous segment in the trace path
  // (pointed to by the "source" field) to this segment.
  //
  Type type;

  // This is a very rough guess at the number of paths will reach this
  // point in the rendering tree.  It is computed simply by multiplying
  // by the branching factor with each recursive trace (and so would
  // only be truly accurate if all paths reached the same recursion
  // depth).
  //
  float complexity;

  // The medium this trace is through.
  //
  const Medium &medium;

  // Downward tree of cached information at this trace point.  The cache
  // is generally modifiable even when a Trace is const, so mark it as
  // mutable.
  //
  mutable TraceCache &cache;
};


}


#endif /* __TRACE_H__ */


// arch-tag: 7ae04357-d63f-4119-9e79-a63d0e5a5e7f
