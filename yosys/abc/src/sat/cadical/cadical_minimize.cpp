#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// Functions for learned clause minimization. We only have the recursive
// version, which actually really is implemented recursively.  We also
// played with a derecursified version, which however was more complex and
// slower.  The trick to keep potential stack exhausting recursion under
// guards is to explicitly limit the recursion depth.

// Instead of signatures as in the original implementation in MiniSAT and
// our corresponding paper, we use the 'poison' idea of Allen Van Gelder to
// mark unsuccessful removal attempts, then Donald Knuth's idea to abort
// minimization if only one literal was seen on the level and a new idea of
// also aborting if the earliest seen literal was assigned afterwards.

bool Internal::minimize_literal (int lit, int depth) {
  LOG ("attempt to minimize lit %d at depth %d", lit, depth);
  CADICAL_assert (val (lit) > 0);
  Flags &f = flags (lit);
  Var &v = var (lit);
  if (!v.level || f.removable || f.keep)
    return true;
  if (!v.reason || f.poison || v.level == level)
    return false;
  const Level &l = control[v.level];
  if (!depth && l.seen.count < 2)
    return false; // Don Knuth's idea
  if (v.trail <= l.seen.trail)
    return false; // new early abort
  if (depth > opts.minimizedepth)
    return false;
  bool res = true;
  CADICAL_assert (v.reason);
  if (opts.minimizeticks)
    stats.ticks.search[stable]++;
  if (v.reason == external_reason) {
    CADICAL_assert (!opts.exteagerreasons);
    v.reason = learn_external_reason_clause (lit, 0, true);
    if (!v.reason) {
      CADICAL_assert (!v.level);
      return true;
    }
  }
  CADICAL_assert (v.reason != external_reason);
  const const_literal_iterator end = v.reason->end ();
  const_literal_iterator i;
  for (i = v.reason->begin (); res && i != end; i++) {
    const int other = *i;
    if (other == lit)
      continue;
    res = minimize_literal (-other, depth + 1);
  }
  if (res)
    f.removable = true;
  else
    f.poison = true;
  minimized.push_back (lit);
  if (!depth) {
    LOG ("minimizing %d %s", lit, res ? "succeeded" : "failed");
  }
  return res;
}

// Sorting the clause before minimization with respect to the trail order
// (literals with smaller trail height first) is necessary but natural and
// might help to minimize the required recursion depth too.

struct minimize_trail_positive_rank {
  Internal *internal;
  minimize_trail_positive_rank (Internal *s) : internal (s) {}
  typedef unsigned Type;
  Type operator() (const int &a) const {
    CADICAL_assert (internal->val (a));
    return (unsigned) internal->var (a).trail;
  }
};

struct minimize_trail_smaller {
  Internal *internal;
  minimize_trail_smaller (Internal *s) : internal (s) {}
  bool operator() (const int &a, const int &b) const {
    return internal->var (a).trail < internal->var (b).trail;
  }
};

struct minimize_trail_level_positive_rank {
  Internal *internal;
  minimize_trail_level_positive_rank (Internal *s) : internal (s) {}
  typedef uint64_t Type;
  Type operator() (const int &a) const {
    CADICAL_assert (internal->val (a));
    Var &v = internal->var (a);
    uint64_t res = v.level;
    res <<= 32;
    res |= v.trail;
    return res;
  }
};

struct minimize_trail_level_smaller {
  Internal *internal;
  minimize_trail_level_smaller (Internal *s) : internal (s) {}
  bool operator() (const int &a, const int &b) const {
    return minimize_trail_level_positive_rank (internal) (a) <
           minimize_trail_level_positive_rank (internal) (b);
  }
};

void Internal::minimize_clause () {
  START (minimize);
  LOG (clause, "minimizing first UIP clause");

  external->check_learned_clause (); // check 1st UIP learned clause first
  minimize_sort_clause ();

  CADICAL_assert (minimized.empty ());
  CADICAL_assert (minimize_chain.empty ());
  const auto end = clause.end ();
  auto j = clause.begin (), i = j;
  std::vector<int> stack;
  for (; i != end; i++) {
    if (minimize_literal (-*i)) {
      if (lrat) {
        CADICAL_assert (mini_chain.empty ());
        calculate_minimize_chain (-*i, stack);
        for (auto p : mini_chain) {
          minimize_chain.push_back (p);
        }
        mini_chain.clear ();
      }
      stats.minimized++;
    } else
      flags (*j++ = *i).keep = true;
  }
  LOG ("minimized %zd literals", (size_t) (clause.end () - j));
  if (j != end)
    clause.resize (j - clause.begin ());
  clear_minimized_literals ();
  for (auto p = minimize_chain.rbegin (); p != minimize_chain.rend ();
       p++) {
    lrat_chain.push_back (*p);
  }
  minimize_chain.clear ();
  STOP (minimize);
}

// go backwards in reason graph and add ids
// mini_chain is in correct order so we have to add it to minimize_chain
// and then reverse when we put it on lrat_chain
//
// We have to use the non-recursive as we cannot limit the depth like the
// minimize version. Unlike the minimize version, we have to keep literals
// on the stack in order to push its reason later.
void Internal::calculate_minimize_chain (int lit, std::vector<int> &stack) {
  CADICAL_assert (stack.empty ());
  stack.push_back (vidx (lit));

  while (!stack.empty ()) {
    const int idx = stack.back ();
    CADICAL_assert (idx);
    stack.pop_back ();
    if (idx < 0) {
      Var &v = var (idx);
      mini_chain.push_back (v.reason->id);
      continue;
    }
    CADICAL_assert (idx);
    Flags &f = flags (idx);
    Var &v = var (idx);
    if (f.keep || f.added || f.poison) {
      continue;
    }
    if (!v.level) {
      if (f.seen)
        continue;
      f.seen = true;
      unit_analyzed.push_back (idx);
      const int lit = val (idx) > 0 ? idx : -idx;
      int64_t id = unit_id (lit);
      unit_chain.push_back (id);
      continue;
    }
    f.added = true;
    CADICAL_assert (v.reason && f.removable);
    const const_literal_iterator end = v.reason->end ();
    const_literal_iterator i;
    LOG (v.reason, "LRAT chain for lit %d at depth %zd by going over", lit,
         stack.size ());
    stack.push_back (-idx);
    for (i = v.reason->begin (); i != end; i++) {
      const int other = *i;
      if (other == idx)
        continue;
      stack.push_back (vidx (other));
    }
  }
  CADICAL_assert (stack.empty ());
}

// Sort the literals in reverse assignment order (thus trail order) to
// establish the base case of the recursive minimization algorithm in the
// positive case (where a literal with 'keep' true is hit).
//
void Internal::minimize_sort_clause () {
  MSORT (opts.radixsortlim, clause.begin (), clause.end (),
         minimize_trail_positive_rank (this),
         minimize_trail_smaller (this));
}

void Internal::clear_minimized_literals () {
  LOG ("clearing %zd minimized literals", minimized.size ());
  for (const auto &lit : minimized) {
    Flags &f = flags (lit);
    f.poison = f.removable = f.shrinkable = f.added = false;
  }
  for (const auto &lit : clause)
    CADICAL_assert (!flags (lit).shrinkable), flags (lit).keep =
                                          flags (lit).shrinkable =
                                              flags (lit).added = false;
  minimized.clear ();
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
