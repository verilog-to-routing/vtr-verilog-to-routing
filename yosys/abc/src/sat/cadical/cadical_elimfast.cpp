#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Implements a variant of elimination with a much lower limit to be run as
// preprocessing. See elim for comments

/*------------------------------------------------------------------------*/

// Flush garbage clause, check fast elimination limits and return number of
// remaining occurrences (or 'fastelimbound + 1' if some limit was hit).

int64_t Internal::flush_elimfast_occs (int lit) {
  const int64_t occslim = opts.fastelimbound;
  const int64_t clslim = opts.fastelimocclim;
  const int64_t failed = occslim + 1;
  Occs &os = occs (lit);
  const const_occs_iterator end = os.end ();
  occs_iterator j = os.begin (), i = j;
  int64_t res = 0;
  while (i != end) {
    Clause *c = *i++;
    if (c->collect ())
      continue;
    *j++ = c;
    if (c->size > clslim) {
      res = failed;
      break;
    }
    if (++res > occslim) {
      CADICAL_assert (opts.fastelimbound < 0 || res == failed);
      break;
    }
  }
  if (i != j) {
    while (i != end)
      *j++ = *i++;
    os.resize (j - os.begin ());
    shrink_occs (os);
  }
  return res;
}

/*------------------------------------------------------------------------*/

// Check whether the number of non-tautological resolvents on 'pivot' is
// smaller or equal to the number of clauses with 'pivot' or '-pivot'.  This
// is the main criteria of bounded variable elimination.  As a side effect
// it flushes garbage clauses with that variable, sorts its occurrence lists
// (smallest clauses first) and also negates pivot if it has more positive
// than negative occurrences.

bool Internal::elimfast_resolvents_are_bounded (Eliminator &eliminator,
                                                int pivot) {
  CADICAL_assert (eliminator.gates.empty ());
  CADICAL_assert (!eliminator.definition_unit);

  stats.elimtried++;

  CADICAL_assert (!unsat);
  CADICAL_assert (active (pivot));

  const Occs &ps = occs (pivot);
  const Occs &ns = occs (-pivot);

  int64_t pos = ps.size ();
  int64_t neg = ns.size ();

  int64_t bound = opts.fastelimbound;

  if (!pos || !neg)
    return bound >= 0;

  const int64_t sum = pos + neg;
  const int64_t product = pos * neg;
  if (bound > sum)
    bound = sum;

  LOG ("checking number resolvents on %d bounded by "
       "%" PRId64 " = %" PRId64 " + %" PRId64 " + %d",
       pivot, bound, pos, neg, opts.fastelimbound);

  if (product <= bound) {
    LOG ("fast elimination occurrence limits sufficiently small enough");
    return true;
  }

  // Try all resolutions between a positive occurrence (outer loop) of
  // 'pivot' and a negative occurrence of 'pivot' (inner loop) as long the
  // bound on non-tautological resolvents is not hit and the size of the
  // generated resolvents does not exceed the resolvent clause size limit.

  int64_t resolvents = 0; // Non-tautological resolvents.

  for (const auto &c : ps) {
    CADICAL_assert (!c->redundant);
    if (c->garbage)
      continue;
    for (const auto &d : ns) {
      CADICAL_assert (!d->redundant);
      if (d->garbage)
        continue;
      if (resolve_clauses (eliminator, c, pivot, d, true)) {
        resolvents++;
        int size = clause.size ();
        clause.clear ();
        LOG ("now at least %" PRId64
             " non-tautological resolvents on pivot %d",
             resolvents, pivot);
        if (size > opts.fastelimclslim) {
          LOG ("resolvent size %d too big after %" PRId64
               " resolvents on %d",
               size, resolvents, pivot);
          return false;
        }
        if (resolvents > bound) {
          LOG ("too many non-tautological resolvents on %d", pivot);
          return false;
        }
      } else if (unsat)
        return false;
      else if (val (pivot))
        return false;
    }
  }

  LOG ("need %" PRId64 " <= %" PRId64 " non-tautological resolvents",
       resolvents, bound);

  return true;
}
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
// Add all resolvents on 'pivot' and connect them.

inline void Internal::elimfast_add_resolvents (Eliminator &eliminator,
                                               int pivot) {

  CADICAL_assert (eliminator.gates.empty ());
  CADICAL_assert (!eliminator.definition_unit);

  LOG ("adding all resolvents on %d", pivot);

  CADICAL_assert (!val (pivot));
  CADICAL_assert (!flags (pivot).eliminated ());

  const Occs &ps = occs (pivot);
  const Occs &ns = occs (-pivot);
#ifdef LOGGING
  int64_t resolvents = 0;
#endif
  for (auto &c : ps) {
    if (unsat)
      break;
    if (c->garbage)
      continue;
    for (auto &d : ns) {
      if (unsat)
        break;
      if (d->garbage)
        continue;
      if (!resolve_clauses (eliminator, c, pivot, d, false))
        continue;
      CADICAL_assert (!lrat || !lrat_chain.empty ());
      Clause *r = new_resolved_irredundant_clause ();
      elim_update_added_clause (eliminator, r);
      eliminator.enqueue (r);
      lrat_chain.clear ();
      clause.clear ();
#ifdef LOGGING
      resolvents++;
#endif
    }
  }

  LOG ("added %" PRId64 " resolvents to eliminate %d", resolvents, pivot);
}

/*------------------------------------------------------------------------*/

// Try to eliminate 'pivot' by bounded variable elimination.
void Internal::try_to_fasteliminate_variable (Eliminator &eliminator,
                                              int pivot,
                                              bool &deleted_binary_clause) {

  if (!active (pivot))
    return;
  CADICAL_assert (!frozen (pivot));

  // First flush garbage clauses and check limits.

  int64_t bound = opts.fastelimbound;

  int64_t pos = flush_elimfast_occs (pivot);
  if (pos > bound) {
    LOG ("too many occurrences thus not eliminated %d", pivot);
    CADICAL_assert (!eliminator.schedule.contains (abs (pivot)));
    return;
  }

  int64_t neg = flush_elimfast_occs (-pivot);
  if (neg > bound) {
    LOG ("too many occurrences thus not eliminated %d", -pivot);
    CADICAL_assert (!eliminator.schedule.contains (abs (pivot)));
    return;
  }

  const int64_t product = pos * neg;
  const int64_t sum = pos + neg;
  if (bound > sum)
    bound = sum;

  if (pos > neg) {
    pivot = -pivot;
    swap (pos, neg);
  }

  LOG ("pivot %d occurs positively %" PRId64
       " times and negatively %" PRId64 " times",
       pivot, pos, neg);

  CADICAL_assert (!eliminator.schedule.contains (abs (pivot)));
  CADICAL_assert (pos <= neg);

  LOG ("trying to eliminate %d", pivot);
  CADICAL_assert (!flags (pivot).eliminated ());

  // Sort occurrence lists, such that shorter clauses come first.
  Occs &ps = occs (pivot);
  stable_sort (ps.begin (), ps.end (), clause_smaller_size ());
  Occs &ns = occs (-pivot);
  stable_sort (ns.begin (), ns.end (), clause_smaller_size ());

  if (!unsat && !val (pivot)) {
    if (product <= bound ||
        elimfast_resolvents_are_bounded (eliminator, pivot)) {
      LOG ("number of resolvents on %d are bounded", pivot);
      elimfast_add_resolvents (eliminator, pivot);
      if (!unsat)
        mark_eliminated_clauses_as_garbage (eliminator, pivot,
                                            deleted_binary_clause);
      if (active (pivot))
        mark_eliminated (pivot);
    } else {
      LOG ("too many resolvents on %d so not eliminated", pivot);
    }
  }

  unmark_gate_clauses (eliminator);
  elim_backward_clauses (eliminator);
}

/*------------------------------------------------------------------------*/

// This function performs one round of bounded variable elimination and
// returns the number of eliminated variables. The additional result
// 'completed' is true if this elimination round ran to completion (all
// variables have been tried).  Otherwise it was asynchronously terminated
// or the resolution limit was hit.

int Internal::elimfast_round (bool &completed,
                              bool &deleted_binary_clause) {

  CADICAL_assert (opts.fastelim);
  CADICAL_assert (!unsat);

  START_SIMPLIFIER (fastelim, ELIM);

  stats.elimfastrounds++;

  CADICAL_assert (!level);

  int64_t resolution_limit;

  if (opts.elimlimited) {
    int64_t delta = stats.propagations.search;
    delta *= 1e-3 * opts.elimeffort;
    if (delta < opts.elimmineff)
      delta = opts.elimmineff;
    if (delta > opts.elimmaxeff)
      delta = opts.elimmaxeff;
    delta = max (delta, (int64_t) 2l * active ());

    PHASE ("fastelim-round", stats.elimfastrounds,
           "limit of %" PRId64 " resolutions", delta);

    resolution_limit = stats.elimres + delta;
  } else {
    PHASE ("fastelim-round", stats.elimfastrounds, "resolutions unlimited");
    resolution_limit = LONG_MAX;
  }

  init_noccs ();

  // First compute the number of occurrences of each literal and at the same
  // time mark satisfied clauses and update 'elim' flags of variables in
  // clauses with root level assigned literals (both false and true).
  //
  for (const auto &c : clauses) {
    if (c->garbage || c->redundant)
      continue;
    bool satisfied = false, falsified = false;
    for (const auto &lit : *c) {
      const signed char tmp = val (lit);
      if (tmp > 0)
        satisfied = true;
      else if (tmp < 0)
        falsified = true;
      else
        CADICAL_assert (active (lit));
    }
    if (satisfied)
      mark_garbage (c); // forces more precise counts
    else {
      for (const auto &lit : *c) {
        if (!active (lit))
          continue;
        if (falsified)
          mark_elim (lit); // simulate unit propagation
        noccs (lit)++;
      }
    }
  }

  init_occs ();

  Eliminator eliminator (this);
  ElimSchedule &schedule = eliminator.schedule;
  CADICAL_assert (schedule.empty ());

  // Now find elimination candidates which occurred in clauses removed since
  // the last time we ran bounded variable elimination, which in turned
  // triggered their 'elim' bit to be set.
  //
  for (auto idx : vars) {
    if (!active (idx))
      continue;
    if (frozen (idx))
      continue;
    if (!flags (idx).elim)
      continue;
    LOG ("scheduling %d for elimination initially", idx);
    schedule.push_back (idx);
  }

  schedule.shrink ();

#ifndef CADICAL_QUIET
  int64_t scheduled = schedule.size ();
#endif

  PHASE ("fastelim-round", stats.elimfastrounds,
         "scheduled %" PRId64 " variables %.0f%% for elimination",
         scheduled, percent (scheduled, active ()));

  // Connect irredundant clauses.
  //
  for (const auto &c : clauses)
    if (!c->garbage && !c->redundant)
      for (const auto &lit : *c)
        if (active (lit))
          occs (lit).push_back (c);

#ifndef CADICAL_QUIET
  const int64_t old_resolutions = stats.elimres;
#endif
  const int old_eliminated = stats.all.eliminated;
  const int old_fixed = stats.all.fixed;

  // Limit on garbage literals during variable elimination. If the limit is
  // hit a garbage collection is performed.
  //
  const int64_t garbage_limit = (2 * stats.irrlits / 3) + (1 << 20);

  // Main loops tries to eliminate variables according to the schedule. The
  // schedule is updated dynamically and variables are potentially
  // rescheduled to be tried again if they occur in a removed clause.
  //
#ifndef CADICAL_QUIET
  int64_t tried = 0;
#endif
  while (!unsat && !terminated_asynchronously () &&
         stats.elimres <= resolution_limit && !schedule.empty ()) {
    int idx = schedule.front ();
    schedule.pop_front ();
    flags (idx).elim = false;
    try_to_fasteliminate_variable (eliminator, idx, deleted_binary_clause);
#ifndef CADICAL_QUIET
    tried++;
#endif
    if (stats.garbage.literals <= garbage_limit)
      continue;
    mark_redundant_clauses_with_eliminated_variables_as_garbage ();
    garbage_collection ();
  }

  // If the schedule is empty all variables have been tried (even
  // rescheduled ones).  Otherwise asynchronous termination happened or we
  // ran into the resolution limit (or derived unsatisfiability).
  //
  completed = !schedule.size ();

  PHASE ("fastelim-round", stats.elimfastrounds,
         "tried to eliminate %" PRId64 " variables %.0f%% (%zd remain)",
         tried, percent (tried, scheduled), schedule.size ());

  schedule.erase ();

  reset_occs ();
  reset_noccs ();

  // Mark all redundant clauses with eliminated variables as garbage.
  //
  if (!unsat)
    mark_redundant_clauses_with_eliminated_variables_as_garbage ();

  int eliminated = stats.all.eliminated - old_eliminated;
  stats.all.fasteliminated += eliminated;
#ifndef CADICAL_QUIET
  int64_t resolutions = stats.elimres - old_resolutions;
  PHASE ("fastelim-round", stats.elimfastrounds,
         "eliminated %d variables %.0f%% in %" PRId64 " resolutions",
         eliminated, percent (eliminated, scheduled), resolutions);
#endif

  const int units = stats.all.fixed - old_fixed;
  report ('e', !opts.reportall && !(eliminated + units));
  STOP_SIMPLIFIER (fastelim, ELIM);

  return eliminated; // non-zero if successful
}

/*------------------------------------------------------------------------*/

void Internal::elimfast () {

  if (unsat)
    return;
  if (level)
    backtrack ();
  if (!propagate ()) {
    learn_empty_clause ();
    return;
  }

  stats.elimfastphases++;
  PHASE ("fastelim-phase", stats.elimfastphases,
         "starting at most %d elimination rounds", opts.fastelimrounds);

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = true;
  }

#ifndef CADICAL_QUIET
  int old_active_variables = active ();
  int old_eliminated = stats.all.eliminated;
#endif

  reset_watches (); // saves lots of memory

  // Alternate one round of bounded variable elimination ('elim_round') and
  // subsumption ('subsume_round'), blocked ('block') and covered clause
  // elimination ('cover') until nothing changes, or the round limit is hit.
  // The loop also aborts early if no variable could be eliminated, the
  // empty clause is resolved, it is asynchronously terminated or a
  // resolution limit is hit.

  // This variable determines whether the whole loop of this bounded
  // variable elimination phase ('elim') ran until completion.  This
  // potentially triggers an incremental increase of the elimination bound.
  //
  bool phase_complete = false, deleted_binary_clause = false;

  int round = 1;
#ifndef CADICAL_QUIET
  int eliminated = 0;
#endif

  bool round_complete = false;
  while (!unsat && !phase_complete && !terminated_asynchronously ()) {
#ifndef CADICAL_QUIET
    int eliminated =
#endif
        elimfast_round (round_complete, deleted_binary_clause);

    if (!round_complete) {
      PHASE ("fastelim-phase", stats.elimphases,
             "last round %d incomplete %s", round,
             eliminated ? "but successful" : "and unsuccessful");
      CADICAL_assert (!phase_complete);
      break;
    }

    if (round++ >= opts.fastelimrounds) {
      PHASE ("fastelim-phase", stats.elimphases, "round limit %d hit (%s)",
             round - 1,
             eliminated ? "though last round successful"
                        : "last round unsuccessful anyhow");
      CADICAL_assert (!phase_complete);
      break;
    }

    // Prioritize 'subsumption' over blocked and covered clause elimination.

    if (subsume_round ())
      continue;

    // Was not able to generate new variable elimination candidates after
    // variable elimination round, neither through subsumption, nor blocked,
    // nor covered clause elimination.
    //
    PHASE ("fastelim-phase", stats.elimphases,
           "no new variable elimination candidates");

    CADICAL_assert (round_complete);
    phase_complete = true;
  }

  for (auto idx : vars) {
    if (active (idx))
      flags (idx).elim = true;
  }

  if (phase_complete) {
    stats.elimcompleted++;
    PHASE ("fastelim-phase", stats.elimphases,
           "fully completed elimination %" PRId64
           " at elimination bound %" PRId64 "",
           stats.elimcompleted, lim.elimbound);
  } else {
    PHASE ("fastelim-phase", stats.elimphases,
           "incomplete elimination %" PRId64
           " at elimination bound %" PRId64 "",
           stats.elimcompleted + 1, lim.elimbound);
  }

  if (deleted_binary_clause)
    delete_garbage_clauses ();
  init_watches ();
  connect_watches ();

  if (unsat)
    LOG ("elimination derived empty clause");
  else if (propagated < trail.size ()) {
    LOG ("elimination produced %zd units",
         (size_t) (trail.size () - propagated));
    if (!propagate ()) {
      LOG ("propagating units after elimination results in empty clause");
      learn_empty_clause ();
    }
  }

#ifndef CADICAL_QUIET
  eliminated = stats.all.eliminated - old_eliminated;
  PHASE ("fastelim-phase", stats.elimphases,
         "eliminated %d variables %.2f%%", eliminated,
         percent (eliminated, old_active_variables));
#endif

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = false;
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
