#ifndef _phases_hpp_INCLUDED
#define _phases_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Phases {

  vector<signed char> best;   // The current largest trail phase.
  vector<signed char> forced; // Forced through 'phase'.
  vector<signed char> min;    // The current minimum unsatisfied phase.
  vector<signed char> prev;   // Previous during local search.
  vector<signed char> saved;  // The actual saved phase.
  vector<signed char> target; // The current target phase.
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
