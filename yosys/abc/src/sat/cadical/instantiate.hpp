#ifndef _instantiate_hpp_INCLUDED
#define _instantiate_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// We are trying to remove literals in clauses, which occur in few clauses
// and further restrict this removal to variables for which variable
// elimination failed.  Thus if for instance we succeed in removing the
// single occurrence of a literal, pure literal elimination can
// eliminate the corresponding variable in the next variable elimination
// round.  The set of such literal clause candidate pairs is collected at
// the end of a variable elimination round and tried before returning.  The
// name of this technique is inspired by 'variable instantiation' as
// described in [AnderssonBjesseCookHanna-DAC'02] and apparently
// successfully used in the 'Oepir' SAT solver.

struct Clause;
struct Internal;

class Instantiator {

  friend struct Internal;

  struct Candidate {
    int lit;
    int size;
    size_t negoccs;
    Clause *clause;
    Candidate (int l, Clause *c, int s, size_t n)
        : lit (l), size (s), negoccs (n), clause (c) {}
  };

  vector<Candidate> candidates;

public:
  void candidate (int l, Clause *c, int s, size_t n) {
    candidates.push_back (Candidate (l, c, s, n));
  }

  operator bool () const { return !candidates.empty (); }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
