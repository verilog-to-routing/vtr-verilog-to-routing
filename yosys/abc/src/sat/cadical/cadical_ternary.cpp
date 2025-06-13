#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// This procedure can be used to produce all possible hyper ternary
// resolvents from ternary clauses.  In contrast to hyper binary resolution
// we would only try to produce ternary clauses from ternary clauses, i.e.,
// do not consider quaternary clauses as antecedents.  Of course if a binary
// clause is generated we keep it too.  In any case we have to make sure
// though that we do not add clauses which are already in the formula or are
// subsumed by a binary clause in the formula.  This procedure simulates
// structural hashing for multiplexer (if-then-else) and binary XOR gates in
// combination with equivalent literal substitution ('decompose') if run
// until to completion (which in the current implementation is too costly
// though and would need to be interleaved more eagerly with equivalent
// literal substitution).  For more information see our CPAIOR'13 paper.

/*------------------------------------------------------------------------*/

// Check whether a binary clause consisting of the permutation of the given
// literals already exists.

bool Internal::ternary_find_binary_clause (int a, int b) {
  CADICAL_assert (occurring ());
  CADICAL_assert (active (a));
  CADICAL_assert (active (b));
  size_t s = occs (a).size ();
  size_t t = occs (b).size ();
  int lit = s < t ? a : b;
  if (opts.ternaryocclim < (int) occs (lit).size ())
    return true;
  for (const auto &c : occs (lit)) {
    if (c->size != 2)
      continue;
    const int *lits = c->literals;
    if (lits[0] == a && lits[1] == b)
      return true;
    if (lits[0] == b && lits[1] == a)
      return true;
  }
  return false;
}

/*------------------------------------------------------------------------*/

// Check whether a ternary clause consisting of the permutation of the given
// literals already exists or is subsumed by an existing binary clause.

bool Internal::ternary_find_ternary_clause (int a, int b, int c) {
  CADICAL_assert (occurring ());
  CADICAL_assert (active (a));
  CADICAL_assert (active (b));
  CADICAL_assert (active (c));
  size_t r = occs (a).size ();
  size_t s = occs (b).size ();
  size_t t = occs (c).size ();
  int lit;
  if (r < s)
    lit = (t < r) ? c : a;
  else
    lit = (t < s) ? c : b;
  if (opts.ternaryocclim < (int) occs (lit).size ())
    return true;
  for (const auto &d : occs (lit)) {
    const int *lits = d->literals;
    if (d->size == 2) {
      if (lits[0] == a && lits[1] == b)
        return true;
      if (lits[0] == b && lits[1] == a)
        return true;
      if (lits[0] == a && lits[1] == c)
        return true;
      if (lits[0] == c && lits[1] == a)
        return true;
      if (lits[0] == b && lits[1] == c)
        return true;
      if (lits[0] == c && lits[1] == b)
        return true;
    } else {
      CADICAL_assert (d->size == 3);
      if (lits[0] == a && lits[1] == b && lits[2] == c)
        return true;
      if (lits[0] == a && lits[1] == c && lits[2] == b)
        return true;
      if (lits[0] == b && lits[1] == a && lits[2] == c)
        return true;
      if (lits[0] == b && lits[1] == c && lits[2] == a)
        return true;
      if (lits[0] == c && lits[1] == a && lits[2] == b)
        return true;
      if (lits[0] == c && lits[1] == b && lits[2] == a)
        return true;
    }
  }
  return false;
}

/*------------------------------------------------------------------------*/

// Try to resolve the two ternary clauses on the given pivot (assumed to
// occur positively in the first clause, negatively in the second).  If the
// resolvent has four literals, is tautological, already exists or in the
// case of a ternary resolvent is subsumed by an existing binary clause then
// 'false' is returned.  The global 'clause' contains the resolvent and
// needs to be cleared in any case.

bool Internal::hyper_ternary_resolve (Clause *c, int pivot, Clause *d) {
  LOG ("hyper binary resolving on pivot %d", pivot);
  LOG (c, "1st antecedent");
  LOG (d, "2nd antecedent");
  stats.ternres++;
  CADICAL_assert (c->size == 3);
  CADICAL_assert (d->size == 3);
  CADICAL_assert (clause.empty ());
  for (const auto &lit : *c)
    if (lit != pivot)
      clause.push_back (lit);
  for (const auto &lit : *d) {
    if (lit == -pivot)
      continue;
    if (lit == clause[0])
      continue;
    if (lit == -clause[0])
      return false;
    if (lit == clause[1])
      continue;
    if (lit == -clause[1])
      return false;
    clause.push_back (lit);
  }
  size_t size = clause.size ();
  if (size > 3)
    return false;
  if (size == 2 && ternary_find_binary_clause (clause[0], clause[1]))
    return false;
  if (size == 3 &&
      ternary_find_ternary_clause (clause[0], clause[1], clause[2]))
    return false;
  return true;
}

/*------------------------------------------------------------------------*/

// Produce all ternary resolvents on literal 'pivot' and increment the
// 'steps' counter by the number of clauses containing 'pivot' which are
// used during this process.  The reason for choosing this metric to measure
// the effort spent in 'ternary' is that it should be similar to one
// propagation step during search.

void Internal::ternary_lit (int pivot, int64_t &steps, int64_t &htrs) {
  LOG ("starting hyper ternary resolutions on pivot %d", pivot);
  steps -= 1 + cache_lines (occs (pivot).size (), sizeof (Clause *));
  for (const auto &c : occs (pivot)) {
    if (steps < 0)
      break;
    if (htrs < 0)
      break;
    if (c->garbage)
      continue;
    if (c->size != 3) {
      CADICAL_assert (c->size == 2);
      continue;
    }
    if (--steps < 0)
      break;
    bool assigned = false;
    for (const auto &lit : *c)
      if (val (lit)) {
        assigned = true;
        break;
      }
    if (assigned)
      continue;
    steps -= 1 + cache_lines (occs (-pivot).size (), sizeof (Clause *));
    for (const auto &d : occs (-pivot)) {
      if (htrs < 0)
        break;
      if (--steps < 0)
        break;
      if (d->garbage)
        continue;
      if (d->size != 3) {
        CADICAL_assert (d->size == 2);
        continue;
      }
      for (const auto &lit : *d)
        if (val (lit)) {
          assigned = true;
          break;
        }
      if (assigned)
        continue;
      CADICAL_assert (clause.empty ());
      htrs--;
      if (hyper_ternary_resolve (c, pivot, d)) {
        size_t size = clause.size ();
        bool red = (size == 3 || (c->redundant && d->redundant));
        if (lrat) {
          CADICAL_assert (lrat_chain.empty ());
          lrat_chain.push_back (c->id);
          lrat_chain.push_back (d->id);
        }
        Clause *r = new_hyper_ternary_resolved_clause (red);
        if (red)
          r->hyper = true;
        lrat_chain.clear ();
        clause.clear ();
        LOG (r, "hyper ternary resolved");
        stats.htrs++;
        for (const auto &lit : *r)
          occs (lit).push_back (r);
        if (size == 2) {
          LOG ("hyper ternary resolvent subsumes both antecedents");
          mark_garbage (c);
          mark_garbage (d);
          stats.htrs2++;
          break;
        } else {
          CADICAL_assert (r->size == 3);
          stats.htrs3++;
        }
      } else {
        LOG (clause, "ignoring size %zd resolvent", clause.size ());
        clause.clear ();
      }
    }
  }
}

/*------------------------------------------------------------------------*/

// Same as 'ternary_lit' but pick the phase of the variable based on the
// number of positive and negative occurrence.

void Internal::ternary_idx (int idx, int64_t &steps, int64_t &htrs) {
  CADICAL_assert (0 < idx);
  CADICAL_assert (idx <= max_var);
  steps -= 3;
  if (!active (idx))
    return;
  if (!flags (idx).ternary)
    return;
  int pos = occs (idx).size ();
  int neg = occs (-idx).size ();
  if (pos <= opts.ternaryocclim && neg <= opts.ternaryocclim) {
    LOG ("index %d has %zd positive and %zd negative occurrences", idx,
         occs (idx).size (), occs (-idx).size ());
    int pivot = (neg < pos ? -idx : idx);
    ternary_lit (pivot, steps, htrs);
  }
  flags (idx).ternary = false;
}

/*------------------------------------------------------------------------*/

// One round of ternary resolution over all variables.  As in 'block' and
// 'elim' we maintain a persistent global flag 'ternary' for each variable,
// which records, whether a ternary clause containing it was added.  Then we
// can focus on those variables for which we have not tried ternary
// resolution before and nothing changed for them since then.  This works
// across multiple calls to 'ternary' as well as 'ternary_round' since this
// 'ternary' variable flag is updated during adding (ternary) resolvents.
// This function goes over each variable just once.

bool Internal::ternary_round (int64_t &steps_limit, int64_t &htrs_limit) {

  CADICAL_assert (!unsat);

#ifndef CADICAL_QUIET
  int64_t bincon = 0;
  int64_t terncon = 0;
#endif

  init_occs ();

  steps_limit -= 1 + cache_lines (clauses.size (), sizeof (Clause *));
  for (const auto &c : clauses) {
    steps_limit--;
    if (c->garbage)
      continue;
    if (c->size > 3)
      continue;
    bool assigned = false, marked = false;
    for (const auto &lit : *c) {
      if (val (lit)) {
        assigned = true;
        break;
      }
      if (flags (lit).ternary)
        marked = true;
    }
    if (assigned)
      continue;
    if (c->size == 2) {
#ifndef CADICAL_QUIET
      bincon++;
#endif
    } else {
      CADICAL_assert (c->size == 3);
      if (!marked)
        continue;
#ifndef CADICAL_QUIET
      terncon++;
#endif
    }

    for (const auto &lit : *c)
      occs (lit).push_back (c);
  }

  PHASE ("ternary", stats.ternary,
         "connected %" PRId64 " ternary %.0f%% "
         "and %" PRId64 " binary clauses %.0f%%",
         terncon, percent (terncon, clauses.size ()), bincon,
         percent (bincon, clauses.size ()));

  // Try ternary resolution on all variables once.
  //
  for (auto idx : vars) {
    if (terminated_asynchronously ())
      break;
    if (steps_limit < 0)
      break;
    if (htrs_limit < 0)
      break;
    ternary_idx (idx, steps_limit, htrs_limit);
  }

  // Gather some statistics for the verbose messages below and also
  // determine whether new variables have been marked and it would make
  // sense to run another round of ternary resolution over those variables.
  //
  int remain = 0;
  for (auto idx : vars) {
    if (!active (idx))
      continue;
    if (!flags (idx).ternary)
      continue;
    remain++;
  }
  if (remain)
    PHASE ("ternary", stats.ternary, "%d variables remain %.0f%%", remain,
           percent (remain, max_var));
  else
    PHASE ("ternary", stats.ternary, "completed hyper ternary resolution");

  reset_occs ();
  CADICAL_assert (!unsat);

  return remain; // Are there variables that should be tried again?
}

/*------------------------------------------------------------------------*/

bool Internal::ternary () {

  if (!opts.ternary)
    return false;
  if (unsat)
    return false;
  if (terminated_asynchronously ())
    return false;

  // No new ternary clauses added since last time?
  //
  if (last.ternary.marked == stats.mark.ternary)
    return false;

  SET_EFFORT_LIMIT (limit, ternary, true);

  START_SIMPLIFIER (ternary, TERNARY);
  stats.ternary++;

  CADICAL_assert (!level);

  CADICAL_assert (!unsat);
  if (watching ())
    reset_watches ();

  // The number of clauses derived through ternary resolution can grow
  // substantially, particularly for random formulas.  Thus we limit the
  // number of added clauses too (actually the number of 'htrs').
  //
  int64_t htrs_limit = stats.current.redundant + stats.current.irredundant;
  htrs_limit *= opts.ternarymaxadd;
  htrs_limit /= 100;

  // approximation of ternary ticks.
  // TODO: count with ternary.ticks directly.
  int64_t steps_limit = stats.ticks.ternary - limit;
  stats.ticks.ternary = limit;

  // With 'stats.ternary' we actually count the number of calls to
  // 'ternary_round' and not the number of calls to 'ternary'. But before
  // the first round we want to show the limit on the number of steps and
  // thus we increase counter for the first round here and skip increasing
  // it in the loop below.
  //
  PHASE ("ternary", stats.ternary,
         "will run a maximum of %d rounds "
         "limited to %" PRId64 " steps and %" PRId64 " clauses",
         opts.ternaryrounds, steps_limit, htrs_limit);

  bool resolved_binary_clause = false;
  bool completed = false;

  for (int round = 0;
       !terminated_asynchronously () && round < opts.ternaryrounds;
       round++) {
    if (htrs_limit < 0)
      break;
    if (steps_limit < 0)
      break;
    if (round)
      stats.ternary++;
    int old_htrs2 = stats.htrs2;
    int old_htrs3 = stats.htrs3;
    completed = ternary_round (steps_limit, htrs_limit);
    int delta_htrs2 = stats.htrs2 - old_htrs2;
    int delta_htrs3 = stats.htrs3 - old_htrs3;
    PHASE ("ternary", stats.ternary,
           "derived %d ternary and %d binary resolvents", delta_htrs3,
           delta_htrs2);
    report ('3', !opts.reportall && !(delta_htrs2 + delta_htrs2));
    if (delta_htrs2)
      resolved_binary_clause = true;
    if (!delta_htrs3)
      break;
  }

  CADICAL_assert (!occurring ());
  CADICAL_assert (!unsat);
  init_watches ();
  connect_watches ();
  if (!propagate ()) {
    LOG ("propagation after connecting watches results in inconsistency");
    learn_empty_clause ();
  }

  if (completed)
    last.ternary.marked = stats.mark.ternary;

  STOP_SIMPLIFIER (ternary, TERNARY);

  return resolved_binary_clause;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
