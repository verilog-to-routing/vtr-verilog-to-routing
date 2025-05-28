#ifndef _var_hpp_INCLUDED
#define _var_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Clause;

// This structure captures data associated with an assigned variable.

struct Var {

  // Note that none of these members is valid unless the variable is
  // assigned.  During unassigning a variable we do not reset it.

  int level;      // decision level
  int trail;      // trail height at assignment
  Clause *reason; // implication graph edge during search
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
