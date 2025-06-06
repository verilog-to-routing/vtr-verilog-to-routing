#ifndef _watch_hpp_INCLUDED
#define _watch_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// Watch lists for CDCL search.  The blocking literal (see also comments
// related to 'propagate') is a must and thus combining that with a 64 bit
// pointer will give a 16 byte (8 byte aligned) structure anyhow, which
// means the additional 4 bytes for the size come for free.  As alternative
// one could use a 32-bit reference instead of the pointer which would
// however limit the number of clauses to '2^32 - 1'.  One would also need
// to use at least one more bit (either taken away from the variable space
// or the clauses) to denote whether the watch is binary.

// in fashion of Intel Sat 10.4230/LIPIcs.SAT.2022.8 we try to
// guarantee the following invariant:
// For both watches:
// if     the watched literal is negatively assigned
// either it will be propagated in the future
// or     the corresponding blocking literal is positively assigned
// and    its level is smaller than the level of the watched literal
//

struct Clause;

struct Watch {

  Clause *clause;
  int blit;
  int size;

  Watch (int b, Clause *c) : clause (c), blit (b), size (c->size) {}
  Watch () {}

  bool binary () const { return size == 2; }
};

typedef vector<Watch> Watches; // of one literal

typedef Watches::iterator watch_iterator;
typedef Watches::const_iterator const_watch_iterator;

inline void remove_watch (Watches &ws, Clause *clause) {
  const auto end = ws.end ();
  auto i = ws.begin ();
  for (auto j = i; j != end; j++) {
    const Watch &w = *i++ = *j;
    if (w.clause == clause)
      i--;
  }
  CADICAL_assert (i + 1 == end);
  ws.resize (i - ws.begin ());
}

// search for the clause and updates the size marked in the watch lists
inline void update_watch_size (Watches &ws, int blit, Clause *conflict) {
  bool found = false;
  const int size = conflict->size;
  for (Watch &w : ws) {
    if (w.clause == conflict)
      w.size = size, w.blit = blit, found = true;
    CADICAL_assert (w.clause->garbage || w.size == 2 || w.clause->size != 2);
  }
  CADICAL_assert (found), (void) found;
}

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
