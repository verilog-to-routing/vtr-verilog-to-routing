#ifndef _level_hpp_INCLUDED
#define _level_hpp_INCLUDED

#include "global.h"

#include <climits>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// For each new decision we increase the decision level and push a 'Level'
// on the 'control' stack.  The information gathered here is used in
// 'reuse_trail' and for early aborts in clause minimization.

struct Level {

  int decision; // decision literal of this level
  int trail;    // trail start of this level

  struct {
    int count; // how many variables seen during 'analyze'
    int trail; // smallest trail position seen on this level
  } seen;

  void reset () {
    seen.count = 0;
    seen.trail = INT_MAX;
  }

  Level (int d, int t) : decision (d), trail (t) { reset (); }
  Level () {}
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
