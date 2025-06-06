#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::constrain (int lit) {
  if (lit)
    constraint.push_back (lit);
  else {
    if (level)
      backtrack ();
    LOG (constraint, "shrinking constraint");
    bool satisfied_constraint = false;
    const vector<int>::const_iterator end = constraint.end ();
    vector<int>::iterator i = constraint.begin ();
    for (vector<int>::const_iterator j = i; j != end; j++) {
      int tmp = marked (*j);
      if (tmp > 0) {
        LOG ("removing duplicated literal %d from constraint", *j);
      } else if (tmp < 0) {
        LOG ("tautological since both %d and %d occur in constraint", -*j,
             *j);
        satisfied_constraint = true;
        break;
      } else {
        tmp = val (*j);
        if (tmp < 0) {
          LOG ("removing falsified literal %d from constraint clause", *j);
        } else if (tmp > 0) {
          LOG ("satisfied constraint with literal %d", *j);
          satisfied_constraint = true;
          break;
        } else {
          *i++ = *j;
          mark (*j);
        }
      }
    }
    constraint.resize (i - constraint.begin ());
    for (const auto &lit : constraint)
      unmark (lit);
    if (satisfied_constraint)
      constraint.clear ();
    else if (constraint.empty ()) {
      unsat_constraint = true;
      if (!conflict_id)
        marked_failed = false; // allow to trigger failing ()
    } else
      for (const auto lit : constraint)
        freeze (lit);
  }
}

bool Internal::failed_constraint () { return unsat_constraint; }

void Internal::reset_constraint () {
  for (auto lit : constraint)
    melt (lit);
  LOG ("cleared %zd constraint literals", constraint.size ());
  constraint.clear ();
  unsat_constraint = false;
  marked_failed = true;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
