#ifndef _occs_h_INCLUDED
#define _occs_h_INCLUDED

#include "global.h"

#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// Full occurrence lists used in a one-watch scheme for all clauses in
// subsumption checking and for irredundant clauses in variable elimination.

struct Clause;
using namespace std;

typedef vector<Clause *> Occs;

inline void shrink_occs (Occs &os) { shrink_vector (os); }
inline void erase_occs (Occs &os) { erase_vector (os); }

inline void remove_occs (Occs &os, Clause *c) {
  const auto end = os.end ();
  auto i = os.begin ();
  for (auto j = i; j != end; j++) {
    const Clause *d = *i++ = *j;
    if (c == d)
      i--;
  }
  CADICAL_assert (i + 1 == end);
  os.resize (i - os.begin ());
}

typedef Occs::iterator occs_iterator;
typedef Occs::const_iterator const_occs_iterator;

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
