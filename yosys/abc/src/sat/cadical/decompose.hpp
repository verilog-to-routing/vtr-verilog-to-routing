#ifndef _decompose_hpp_INCLUDED
#define _decompose_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// This implements Tarjan's algorithm for decomposing the binary implication
// graph intro strongly connected components (SCCs).  Literals in one SCC
// are equivalent and we replace them all by the literal with the smallest
// index in the SCC.  These variables are marked 'substituted' and will be
// removed from all clauses.  Their value will be fixed during 'extend'.

#define TRAVERSED UINT_MAX // mark completely traversed

struct DFS {
  unsigned idx;   // depth first search index
  unsigned min;   // minimum reachable index
  Clause *parent; // for lrat
  DFS () : idx (0), min (0), parent (0) {}
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
