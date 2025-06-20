#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Implements a variant of bounded variable elimination as originally
// described in our SAT'05 paper introducing 'SATeLite'.  This is an
// inprocessing version, i.e., it is interleaved with search and triggers
// subsumption and strengthening, blocked and covered clause elimination
// during elimination rounds.  It focuses only those variables which
// occurred in removed irredundant clauses since the last time an
// elimination round was run.  By bounding the maximum resolvent size we can
// run each elimination round until completion.  See the code of 'elim' for
// how elimination rounds are interleaved with blocked clause elimination
// and subsumption (which in turn also calls vivification and transitive
// reduction of the binary implication graph).

/*------------------------------------------------------------------------*/

inline double Internal::compute_elim_score (unsigned lit) {
  CADICAL_assert (1 <= lit), CADICAL_assert (lit <= (unsigned) max_var);
  const unsigned uidx = 2 * lit;
  const double pos = internal->ntab[uidx];
  const double neg = internal->ntab[uidx + 1];
  if (!pos)
    return -neg;
  if (!neg)
    return -pos;
  double sum = 0, prod = 0;
  if (opts.elimsum)
    sum = opts.elimsum * (pos + neg);
  if (opts.elimprod)
    prod = opts.elimprod * (pos * neg);
  return prod + sum;
}

/*------------------------------------------------------------------------*/

bool elim_more::operator() (unsigned a, unsigned b) {
  const auto s = internal->compute_elim_score (a);
  const auto t = internal->compute_elim_score (b);
  if (s > t)
    return true;
  if (s < t)
    return false;
  return a > b;
}

/*------------------------------------------------------------------------*/

// Note that the new fast subsumption algorithm implemented in 'subsume'
// does not distinguish between irredundant and redundant clauses and is
// also run during search to strengthen and remove 'sticky' redundant
// clauses but also irredundant ones.  So beside learned units during search
// or as consequence of other preprocessors, these subsumption rounds during
// search can remove (irredundant) clauses (and literals), which in turn
// might make new bounded variable elimination possible.  This is tested
// in the 'bool ineliminating ()' guard.

bool Internal::ineliminating () {

  if (!opts.elim)
    return false;
  if (!preprocessing && !opts.inprocessing)
    return false;
  if (preprocessing)
    CADICAL_assert (lim.preprocessing);

  // Respect (increasing) conflict limit.
  //
  if (lim.elim >= stats.conflicts)
    return false;

  // Wait until there are new units or new removed variables
  // (in removed or shrunken irredundant clauses and thus marked).
  //
  if (last.elim.fixed < stats.all.fixed)
    return true;
  if (last.elim.marked < stats.mark.elim)
    return true;

  // VERBOSE (3, "elim not scheduled due to fixpoint");
  return false;
}

/*------------------------------------------------------------------------*/

// Update the global elimination schedule after adding or removing a clause.

void Internal::elim_update_added_clause (Eliminator &eliminator,
                                         Clause *c) {
  CADICAL_assert (!c->redundant);
  ElimSchedule &schedule = eliminator.schedule;
  for (const auto &lit : *c) {
    if (!active (lit))
      continue;
    occs (lit).push_back (c);
    if (frozen (lit))
      continue;
    noccs (lit)++;
    const int idx = abs (lit);
    if (schedule.contains (idx))
      schedule.update (idx);
  }
}

void Internal::elim_update_removed_lit (Eliminator &eliminator, int lit) {
  if (!active (lit))
    return;
  if (frozen (lit))
    return;
  int64_t &score = noccs (lit);
  CADICAL_assert (score > 0);
  score--;
  const int idx = abs (lit);
  ElimSchedule &schedule = eliminator.schedule;
  if (schedule.contains (idx))
    schedule.update (idx);
  else {
    LOG ("rescheduling %d for elimination after removing clause", idx);
    schedule.push_back (idx);
  }
}

void Internal::elim_update_removed_clause (Eliminator &eliminator,
                                           Clause *c, int except) {
  CADICAL_assert (!c->redundant);
  for (const auto &lit : *c) {
    if (lit == except)
      continue;
    CADICAL_assert (lit != -except);
    elim_update_removed_lit (eliminator, lit);
  }
}

/*------------------------------------------------------------------------*/

// Since we do not have watches we have to do our own unit propagation
// during elimination as soon we find a unit clause.  This finds new units
// and also marks clauses satisfied by those units as garbage immediately.

void Internal::elim_propagate (Eliminator &eliminator, int root) {
  CADICAL_assert (val (root) > 0);
  vector<int> work;
  size_t i = 0;
  work.push_back (root);
  while (i < work.size ()) {
    int lit = work[i++];
    LOG ("elimination propagation of %d", lit);
    CADICAL_assert (val (lit) > 0);
    const Occs &ns = occs (-lit);
    for (const auto &c : ns) {
      if (c->garbage)
        continue;
      int unit = 0, satisfied = 0;
      for (const auto &other : *c) {
        const signed char tmp = val (other);
        if (tmp < 0)
          continue;
        if (tmp > 0) {
          satisfied = other;
          break;
        }
        if (unit)
          unit = INT_MIN;
        else
          unit = other;
      }
      if (satisfied) {
        LOG (c, "elimination propagation of %d finds %d satisfied", lit,
             satisfied);
        elim_update_removed_clause (eliminator, c, satisfied);
        mark_garbage (c);
      } else if (!unit) {
        LOG ("empty clause during elimination propagation of %d", lit);
        // need to set conflict = c for lrat
        conflict = c;
        learn_empty_clause ();
        conflict = 0;
        break;
      } else if (unit != INT_MIN) {
        LOG ("new unit %d during elimination propagation of %d", unit, lit);
        build_chain_for_units (unit, c, 0);
        assign_unit (unit);
        work.push_back (unit);
      }
    }
    if (unsat)
      break;
    const Occs &ps = occs (lit);
    for (const auto &c : ps) {
      if (c->garbage)
        continue;
      LOG (c, "elimination propagation of %d produces satisfied", lit);
      elim_update_removed_clause (eliminator, c, lit);
      mark_garbage (c);
    }
  }
}

/*------------------------------------------------------------------------*/

// On-the-fly self-subsuming resolution during variable elimination is due
// to HyoJung Han, Fabio Somenzi, SAT'09.  Basically while resolving two
// clauses we test the resolvent to be smaller than one of the antecedents.
// If this is the case the pivot can be removed from the antecedent
// on-the-fly and the resolution can be skipped during elimination.

void Internal::elim_on_the_fly_self_subsumption (Eliminator &eliminator,
                                                 Clause *c, int pivot) {
  LOG (c, "pivot %d on-the-fly self-subsuming resolution", pivot);
  stats.elimotfstr++;
  stats.strengthened++;
  CADICAL_assert (clause.empty ());
  for (const auto &lit : *c) {
    if (lit == pivot)
      continue;
    const signed char tmp = val (lit);
    CADICAL_assert (tmp <= 0);
    if (tmp < 0)
      continue;
    clause.push_back (lit);
  }
  Clause *r = new_resolved_irredundant_clause ();
  elim_update_added_clause (eliminator, r);
  clause.clear ();
  lrat_chain.clear ();
  elim_update_removed_clause (eliminator, c, pivot);
  mark_garbage (c);
}

/*------------------------------------------------------------------------*/

// Resolve two clauses on the pivot literal 'pivot', which is assumed to
// occur in opposite phases in 'c' and 'd'.  The actual resolvent is stored
// in the temporary global 'clause' if it is not redundant.  It is
// considered redundant if one of the clauses is already marked as garbage
// it is root level satisfied, the resolvent is empty, a unit, or produces a
// self-subsuming resolution, which results in the pivot to be removed from
// at least one of the antecedents.

// Note that current root level assignments are taken into account, i.e., by
// removing root level falsified literals.  The function returns true if the
// resolvent is not redundant and for instance has to be taken into account
// during bounded variable elimination.

// Detected units are immediately assigned and in case the last argument is
// true also propagated eagerly in a elimination specific propagation
// routine, which not only finds units but also updates the schedule.

// When this function is called during computation of the number of
// non-trivial (or non-satisfied) resolvents we can eagerly propagate units.
// But during actually adding the resolvents this results in problems as we
// found one rare test case '../test/trace/reg0056.trace' (out of billions),
// where the pivot itself was assigned during such a propagation while
// adding resolvents and lead to pushing a clause to the reconstruction
// stack that later flipped the value of the pivot (while all other literals
// in that clause were unit implied too).  Not pushing the pivot clauses to
// the reconstruction stack produced a wrong model too.  Our fix is to only
// eagerly propagate during computation of the number of resolvents and
// otherwise delay propagation until the end of elimination (which is less
// precise regarding scheduling but very rarely happens).

bool Internal::resolve_clauses (Eliminator &eliminator, Clause *c,
                                int pivot, Clause *d,
                                const bool propagate_eagerly) {

  CADICAL_assert (!c->redundant);
  CADICAL_assert (!d->redundant);

  stats.elimres++;

  if (c->garbage || d->garbage)
    return false;
  if (c->size > d->size) {
    pivot = -pivot;
    swap (c, d);
  }

  CADICAL_assert (!level);
  CADICAL_assert (clause.empty ());

  int satisfied = 0;    // Contains this satisfying literal.
  int tautological = 0; // Clashing literal if tautological.

  int s = 0; // Actual literals from 'c'.
  int t = 0; // Actual literals from 'd'.

  // First determine whether the first antecedent is satisfied, add its
  // literals to 'clause' and mark them (except for 'pivot').
  //
  for (const auto &lit : *c) {
    if (lit == pivot) {
      s++;
      continue;
    }
    CADICAL_assert (lit != -pivot);
    const signed char tmp = val (lit);
    if (tmp > 0) {
      satisfied = lit;
      break;
    } else if (tmp < 0) {
      if (!lrat)
        continue;
      Flags &f = flags (lit);
      if (f.seen)
        continue;
      analyzed.push_back (lit);
      f.seen = true;
      int64_t id = unit_id (-lit);
      lrat_chain.push_back (id);
      continue;
    } else
      mark (lit), clause.push_back (lit), s++;
  }
  if (satisfied) {
    LOG (c, "satisfied by %d antecedent", satisfied);
    elim_update_removed_clause (eliminator, c, satisfied);
    mark_garbage (c);
    clause.clear ();
    lrat_chain.clear ();
    clear_analyzed_literals ();
    unmark (c);
    return false;
  }

  // Then determine whether the second antecedent is satisfied, add its
  // literal to 'clause' and check whether a clashing literal is found, such
  // that the resolvent would be tautological.
  //
  for (const auto &lit : *d) {
    if (lit == -pivot) {
      t++;
      continue;
    }
    CADICAL_assert (lit != pivot);
    signed char tmp = val (lit);
    if (tmp > 0) {
      satisfied = lit;
      break;
    } else if (tmp < 0) {
      if (!lrat)
        continue;
      Flags &f = flags (lit);
      if (f.seen)
        continue;
      analyzed.push_back (lit);
      f.seen = true;
      int64_t id = unit_id (-lit);
      lrat_chain.push_back (id);
      continue;
    } else if ((tmp = marked (lit)) < 0) {
      tautological = lit;
      break;
    } else if (!tmp)
      clause.push_back (lit), t++;
    else
      CADICAL_assert (tmp > 0), t++;
  }

  clear_analyzed_literals ();
  unmark (c);
  const int64_t size = clause.size ();

  if (lrat) {
    lrat_chain.push_back (d->id);
    lrat_chain.push_back (c->id);
  }

  if (satisfied) {
    LOG (d, "satisfied by %d antecedent", satisfied);
    elim_update_removed_clause (eliminator, d, satisfied);
    mark_garbage (d);
    clause.clear ();
    lrat_chain.clear ();
    return false;
  }

  LOG (c, "first antecedent");
  LOG (d, "second antecedent");

  if (tautological) {
    clause.clear ();
    LOG ("resolvent tautological on %d", tautological);
    lrat_chain.clear ();
    return false;
  }

  if (!size) {
    clause.clear ();
    LOG ("empty resolvent");
    learn_empty_clause (); // already clears lrat_chain.
    return false;
  }

  if (size == 1) {
    int unit = clause[0];
    LOG ("unit resolvent %d", unit);
    clause.clear ();
    assign_unit (unit); // already clears lrat_chain.
    if (propagate_eagerly)
      elim_propagate (eliminator, unit);
    return false;
  }

  LOG (clause, "resolvent");
  CADICAL_assert (!lrat || !lrat_chain.empty ());

  // Double self-subsuming resolution.  The clauses 'c' and 'd' are
  // identical except for the pivot which occurs in different phase.  The
  // resolvent subsumes both antecedents.

  if (s > size && t > size) {
    CADICAL_assert (s == size + 1);
    CADICAL_assert (t == size + 1);
    clause.clear ();
    // LRAT is c + d (+ eventual units)
    elim_on_the_fly_self_subsumption (eliminator, c, pivot);
    LOG (d, "double pivot %d on-the-fly self-subsuming resolution", -pivot);
    stats.elimotfsub++;
    stats.subsumed++;
    elim_update_removed_clause (eliminator, d, -pivot);
    mark_garbage (d);
    return false;
  }

  // Single self-subsuming resolution:  The pivot can be removed from 'c',
  // which is implemented by adding a clause which is the same as 'c' but
  // with 'pivot' removed and then marking 'c' as garbage.

  if (s > size) {
    CADICAL_assert (s == size + 1);
    clause.clear ();
    // LRAT is c + d (+ eventual units)
    elim_on_the_fly_self_subsumption (eliminator, c, pivot);
    return false;
  }

  // Same single self-subsuming resolution situation, but only for 'd'.

  if (t > size) {
    CADICAL_assert (t == size + 1);
    clause.clear ();
    // LRAT is c + d (+ eventual units) -> same.
    elim_on_the_fly_self_subsumption (eliminator, d, -pivot);
    return false;
  }
  if (propagate_eagerly)
    lrat_chain.clear ();
  return true;
}

/*------------------------------------------------------------------------*/

// Check whether the number of non-tautological resolvents on 'pivot' is
// smaller or equal to the number of clauses with 'pivot' or '-pivot'.  This
// is the main criteria of bounded variable elimination.  As a side effect
// it flushes garbage clauses with that variable, sorts its occurrence lists
// (smallest clauses first) and also negates pivot if it has more positive
// than negative occurrences.

bool Internal::elim_resolvents_are_bounded (Eliminator &eliminator,
                                            int pivot) {
  const bool substitute = !eliminator.gates.empty ();
  const bool resolve_gates = eliminator.definition_unit;
  if (substitute)
    LOG ("trying to substitute %d", pivot);

  stats.elimtried++;

  CADICAL_assert (!unsat);
  CADICAL_assert (active (pivot));

  const Occs &ps = occs (pivot);
  const Occs &ns = occs (-pivot);
  const int64_t pos = ps.size ();
  const int64_t neg = ns.size ();
  if (!pos || !neg)
    return lim.elimbound >= 0;
  const int64_t bound = pos + neg + lim.elimbound;

  LOG ("checking number resolvents on %d bounded by "
       "%" PRId64 " = %" PRId64 " + %" PRId64 " + %" PRId64,
       pivot, bound, pos, neg, lim.elimbound);

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
      if (!resolve_gates && substitute && c->gate == d->gate)
        continue;
      stats.elimrestried++;
      if (resolve_clauses (eliminator, c, pivot, d, true)) {
        resolvents++;
        int size = clause.size ();
        clause.clear ();
        LOG ("now at least %" PRId64
             " non-tautological resolvents on pivot %d",
             resolvents, pivot);
        if (size > opts.elimclslim) {
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
// Add all resolvents on 'pivot' and connect them.

inline void Internal::elim_add_resolvents (Eliminator &eliminator,
                                           int pivot) {

  const bool substitute = !eliminator.gates.empty ();
  const bool resolve_gates = eliminator.definition_unit;
  if (substitute) {
    LOG ("substituting pivot %d by resolving with %zd gate clauses", pivot,
         eliminator.gates.size ());
    stats.elimsubst++;
  }
  switch (eliminator.gatetype) {
  case EQUI:
    stats.eliminated_equi++;
    break;
  case AND:
    stats.eliminated_and++;
    break;
  case ITE:
    stats.eliminated_ite++;
    break;
  case XOR:
    stats.eliminated_xor++;
    break;
  case DEF:
    stats.eliminated_def++;
    break;
  default:
    CADICAL_assert (eliminator.gatetype == NO);
  }

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
      if (!resolve_gates && substitute && c->gate == d->gate)
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

// Remove clauses with 'pivot' and '-pivot' by marking them as garbage and
// push them on the extension stack.

void Internal::mark_eliminated_clauses_as_garbage (
    Eliminator &eliminator, int pivot, bool &deleted_binary_clause) {
  CADICAL_assert (!unsat);

  LOG ("marking irredundant clauses with %d as garbage", pivot);

  const int64_t substitute = eliminator.gates.size ();
  if (substitute)
    LOG ("pushing %" PRId64 " gate clauses on extension stack", substitute);
#ifndef CADICAL_NDEBUG
  int64_t pushed = 0;
#endif
  Occs &ps = occs (pivot);
  for (const auto &c : ps) {
    if (c->garbage)
      continue;
    CADICAL_assert (!c->redundant);
    if (!substitute || c->gate) {
      if (proof)
        proof->weaken_minus (c);
      if (c->size == 2)
        deleted_binary_clause = true;
      external->push_clause_on_extension_stack (c, pivot);
#ifndef CADICAL_NDEBUG
      pushed++;
#endif
    }
    mark_garbage (c);
    elim_update_removed_clause (eliminator, c, pivot);
  }
  erase_occs (ps);

  LOG ("marking irredundant clauses with %d as garbage", -pivot);

  Occs &ns = occs (-pivot);
  for (const auto &d : ns) {
    if (d->garbage)
      continue;
    CADICAL_assert (!d->redundant);
    if (!substitute || d->gate) {
      if (proof)
        proof->weaken_minus (d);
      if (d->size == 2)
        deleted_binary_clause = true;
      external->push_clause_on_extension_stack (d, -pivot);
#ifndef CADICAL_NDEBUG
      pushed++;
#endif
    }
    mark_garbage (d);
    elim_update_removed_clause (eliminator, d, -pivot);
  }
  erase_occs (ns);

  if (substitute)
    CADICAL_assert (pushed <= substitute);

  // Unfortunately, we can not use the trick by Niklas Soerensson anymore,
  // which avoids saving all clauses on the extension stack.  This would
  // break our new incremental 'restore' logic.
}

/*------------------------------------------------------------------------*/

// Try to eliminate 'pivot' by bounded variable elimination.
void Internal::try_to_eliminate_variable (Eliminator &eliminator, int pivot,
                                          bool &deleted_binary_clause) {

  if (!active (pivot))
    return;
  CADICAL_assert (!frozen (pivot));

  // First flush garbage clauses.
  //
  int64_t pos = flush_occs (pivot);
  int64_t neg = flush_occs (-pivot);

  if (pos > neg) {
    pivot = -pivot;
    swap (pos, neg);
  }
  LOG ("pivot %d occurs positively %" PRId64
       " times and negatively %" PRId64 " times",
       pivot, pos, neg);
  CADICAL_assert (!eliminator.schedule.contains (abs (pivot)));
  CADICAL_assert (pos <= neg);

  if (pos && neg > opts.elimocclim) {
    LOG ("too many occurrences thus not eliminated %d", pivot);
    CADICAL_assert (!eliminator.schedule.contains (abs (pivot)));
    return;
  }

  LOG ("trying to eliminate %d", pivot);
  CADICAL_assert (!flags (pivot).eliminated ());

  // Sort occurrence lists, such that shorter clauses come first.
  Occs &ps = occs (pivot);
  stable_sort (ps.begin (), ps.end (), clause_smaller_size ());
  Occs &ns = occs (-pivot);
  stable_sort (ns.begin (), ns.end (), clause_smaller_size ());

  if (pos)
    find_gate_clauses (eliminator, pivot);

  if (!unsat && !val (pivot)) {
    if (elim_resolvents_are_bounded (eliminator, pivot)) {
      LOG ("number of resolvents on %d are bounded", pivot);
      elim_add_resolvents (eliminator, pivot);
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

void Internal::
    mark_redundant_clauses_with_eliminated_variables_as_garbage () {
  for (const auto &c : clauses) {
    if (c->garbage || !c->redundant)
      continue;
    bool clean = true;
    for (const auto &lit : *c) {
      Flags &f = flags (lit);
      if (f.eliminated ()) {
        clean = false;
        break;
      }
      if (f.pure ()) {
        clean = false;
        break;
      }
    }
    if (!clean)
      mark_garbage (c);
  }
}

/*------------------------------------------------------------------------*/

// This function performs one round of bounded variable elimination and
// returns the number of eliminated variables. The additional result
// 'completed' is true if this elimination round ran to completion (all
// variables have been tried).  Otherwise it was asynchronously terminated
// or the resolution limit was hit.

int Internal::elim_round (bool &completed, bool &deleted_binary_clause) {

  CADICAL_assert (opts.elim);
  CADICAL_assert (!unsat);

  START_SIMPLIFIER (elim, ELIM);
  stats.elimrounds++;

  int64_t marked_before = last.elim.marked;
  last.elim.marked = stats.mark.elim;
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

    PHASE ("elim-round", stats.elimrounds,
           "limit of %" PRId64 " resolutions", delta);

    resolution_limit = stats.elimres + delta;
  } else {
    PHASE ("elim-round", stats.elimrounds, "resolutions unlimited");
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

  PHASE ("elim-round", stats.elimrounds,
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
    try_to_eliminate_variable (eliminator, idx, deleted_binary_clause);
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

  if (!completed)
    last.elim.marked = marked_before;

  PHASE ("elim-round", stats.elimrounds,
         "tried to eliminate %" PRId64 " variables %.0f%% (%zd remain)",
         tried, percent (tried, scheduled), schedule.size ());

  schedule.erase ();

  // Collect potential literal clause instantiation pairs, which needs full
  // occurrence lists and thus we have it here before resetting them.
  //
  Instantiator instantiator;
  if (!unsat && !terminated_asynchronously () && opts.instantiate)
    collect_instantiation_candidates (instantiator);

  reset_occs ();
  reset_noccs ();

  // Mark all redundant clauses with eliminated variables as garbage.
  //
  if (!unsat)
    mark_redundant_clauses_with_eliminated_variables_as_garbage ();

  int eliminated = stats.all.eliminated - old_eliminated;
#ifndef CADICAL_QUIET
  int64_t resolutions = stats.elimres - old_resolutions;
  PHASE ("elim-round", stats.elimrounds,
         "eliminated %d variables %.0f%% in %" PRId64 " resolutions",
         eliminated, percent (eliminated, scheduled), resolutions);
#endif

  last.elim.subsumephases = stats.subsumephases;
  const int units = stats.all.fixed - old_fixed;
  report ('e', !opts.reportall && !(eliminated + units));
  STOP_SIMPLIFIER (elim, ELIM);

  if (!unsat && !terminated_asynchronously () &&
      instantiator) // Do we have candidate pairs?
    instantiate (instantiator);

  return eliminated; // non-zero if successful
}

/*------------------------------------------------------------------------*/

// Increase elimination bound (additional clauses allowed during variable
// elimination), which is triggered if elimination with last bound completed
// (including no new subsumptions).  This was pioneered by GlueMiniSAT in
// the SAT Race 2015 and then picked up Chanseok Oh in his COMinisatPS
// solver, which in turn is used in the Maple series of SAT solvers.
// The bound is no increased if the maximum bound is reached.

void Internal::increase_elimination_bound () {

  if (lim.elimbound >= opts.elimboundmax)
    return;

  if (lim.elimbound < 0)
    lim.elimbound = 0;
  else if (!lim.elimbound)
    lim.elimbound = 1;
  else
    lim.elimbound *= 2;

  if (lim.elimbound > opts.elimboundmax)
    lim.elimbound = opts.elimboundmax;

  PHASE ("elim-phase", stats.elimphases,
         "new elimination bound %" PRId64 "", lim.elimbound);

  // Now reschedule all active variables for elimination again.
  //
#ifdef LOGGING
  int count = 0;
#endif
  for (auto idx : vars) {
    if (!active (idx))
      continue;
    if (flags (idx).elim)
      continue;
    mark_elim (idx);
#ifdef LOGGING
    count++;
#endif
  }
  LOG ("marked %d variables as elimination candidates", count);

  report ('^');
}

void Internal::init_citten () {
  if (!opts.elimdef)
    return;
  CADICAL_assert (!citten);
  citten = cadical_kitten_init ();
}

void Internal::reset_citten () {
  if (citten) {
    cadical_kitten_release (citten);
    citten = 0;
  }
}

/*------------------------------------------------------------------------*/

void Internal::elim (bool update_limits) {

  if (unsat)
    return;
  if (level)
    backtrack ();
  if (!propagate ()) {
    learn_empty_clause ();
    return;
  }

  stats.elimphases++;
  PHASE ("elim-phase", stats.elimphases,
         "starting at most %d elimination rounds", opts.elimrounds);

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = true;
  }

#ifndef CADICAL_QUIET
  int old_active_variables = active ();
  int old_eliminated = stats.all.eliminated;
#endif

  // Make sure there was a complete subsumption phase since last
  // elimination including vivification etc.
  //
  if (last.elim.subsumephases == stats.subsumephases)
    subsume ();

  reset_watches (); // saves lots of memory

  init_citten ();

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
        elim_round (round_complete, deleted_binary_clause);

    if (!round_complete) {
      PHASE ("elim-phase", stats.elimphases, "last round %d incomplete %s",
             round, eliminated ? "but successful" : "and unsuccessful");
      CADICAL_assert (!phase_complete);
      break;
    }

    if (round++ >= opts.elimrounds) {
      PHASE ("elim-phase", stats.elimphases, "round limit %d hit (%s)",
             round - 1,
             eliminated ? "though last round successful"
                        : "last round unsuccessful anyhow");
      CADICAL_assert (!phase_complete);
      break;
    }

    // Prioritize 'subsumption' over blocked and covered clause elimination.

    if (subsume_round ())
      continue;
    if (block ())
      continue;
    if (cover ())
      continue;

    // Was not able to generate new variable elimination candidates after
    // variable elimination round, neither through subsumption, nor blocked,
    // nor covered clause elimination.
    //
    PHASE ("elim-phase", stats.elimphases,
           "no new variable elimination candidates");

    CADICAL_assert (round_complete);
    phase_complete = true;
  }

  if (phase_complete) {
    stats.elimcompleted++;
    PHASE ("elim-phase", stats.elimphases,
           "fully completed elimination %" PRId64
           " at elimination bound %" PRId64 "",
           stats.elimcompleted, lim.elimbound);
  } else {
    PHASE ("elim-phase", stats.elimphases,
           "incomplete elimination %" PRId64
           " at elimination bound %" PRId64 "",
           stats.elimcompleted + 1, lim.elimbound);
  }

  reset_citten ();
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

  // If we ran variable elimination until completion we increase the
  // variable elimination bound and reschedule elimination of all variables.
  //
  if (phase_complete)
    increase_elimination_bound ();

#ifndef CADICAL_QUIET
  eliminated = stats.all.eliminated - old_eliminated;
  PHASE ("elim-phase", stats.elimphases, "eliminated %d variables %.2f%%",
         eliminated, percent (eliminated, old_active_variables));
#endif

  if (external_prop) {
    CADICAL_assert (!level);
    private_steps = false;
  }

  if (!update_limits)
    return;

  int64_t delta = scale (opts.elimint * (stats.elimphases + 1));
  lim.elim = stats.conflicts + delta;

  PHASE ("elim-phase", stats.elimphases,
         "new limit at %" PRId64 " conflicts after %" PRId64 " conflicts",
         lim.elim, delta);

  last.elim.fixed = stats.all.fixed;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
