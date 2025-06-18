#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*----------------------------------------------------------------------------*/
//
// Mark a variable as an observed one. It can be a new variable. It is
// assumed to be clean (not eliminated by previous simplifications).
//
void Internal::add_observed_var (int ilit) {
  int idx = vidx (ilit);
  if (idx >= (int64_t) relevanttab.size ())
    relevanttab.resize (1 + (size_t) idx, 0);
  unsigned &ref = relevanttab[idx];
  if (ref < UINT_MAX) {
    ref++;
    LOG ("variable %d is observed %u times", idx, ref);
  } else
    LOG ("variable %d remains observed forever", idx);
  // TODO: instead of actually backtracking, it would be enough to notify
  // backtrack and re-play again every levels' notification to the
  // propagator
  if (val (ilit) && level && !fixed (ilit)) {
    // The variable is already assigned, but we can not send a notification
    // about it because it happened on an earlier decision level.
    // To not break the stack-like view of the trail, we simply backtrack to
    // undo this unnotifiable assignment.
    const int assignment_level = var (ilit).level;
    backtrack (assignment_level - 1);
  } else if (level && fixed (ilit)) {
    backtrack (0);
  }
}

/*----------------------------------------------------------------------------*/
//
// Removing an observed variable should happen only once it is ensured
// that there is no unexplained propagation in the implication
// graph involving this variable.
//
void Internal::remove_observed_var (int ilit) {
  if (!fixed (ilit) && level) {
    backtrack ();
  }

  CADICAL_assert (fixed (ilit) || !level);

  const int idx = vidx (ilit);
  CADICAL_assert ((size_t) idx < relevanttab.size ());
  unsigned &ref = relevanttab[idx];
  CADICAL_assert (fixed (ilit) || ref > 0);
  if (fixed (ilit))
    ref = 0;
  else if (ref < UINT_MAX) {
    if (!--ref) {
      LOG ("variable %d is not observed anymore", idx);
    } else
      LOG ("variable %d is unobserved once but remains observed %u times",
           ilit, ref);
  } else
    LOG ("variable %d remains observed forever", idx);
}

/*----------------------------------------------------------------------------*/
//
// Supposed to be used only by mobical.
//
bool Internal::observed (int ilit) const {
  CADICAL_assert ((size_t) vidx (ilit) < relevanttab.size ());
  return relevanttab[vidx (ilit)] > 0;
}

/*----------------------------------------------------------------------------*/
//
// Check for unexplained propagations upon disconnecting external propagator
//
void Internal::set_tainted_literal () {
  if (!opts.ilb) {
    return;
  }
  for (auto idx : vars) {
    if (!val (idx))
      continue;
    if (var (idx).reason != external_reason)
      continue;
    if (!tainted_literal) {
      tainted_literal = idx;
      continue;
    }
    CADICAL_assert (val (tainted_literal));
    if (var (idx).level < var (tainted_literal).level) {
      tainted_literal = idx;
    }
  }
}

void Internal::renotify_trail_after_ilb () {
  if (!external_prop || external_prop_is_lazy || !trail.size () ||
      !opts.ilb) {
    return;
  }
  LOG ("notify external propagator about new assignments (after ilb)");
#ifndef CADICAL_NDEBUG
  LOG ("(decision level: %d, trail size: %zd, notified %zd)", level,
       trail.size (), notified);
#endif
  renotify_full_trail ();
}

void Internal::renotify_trail_after_local_search () {
  if (!external_prop || external_prop_is_lazy || !trail.size ()) {
    return;
  }
  LOG ("notify external propagator about new assignments (after local "
       "search)");
#ifndef CADICAL_NDEBUG
  LOG ("(decision level: %d, trail size: %zd, notified %zd)", level,
       trail.size (), notified);
#endif
  renotify_full_trail ();
}

// It repeats ALL assignments of the trail, so the already notified
// root-level assignments will be notified multiple times.

void Internal::renotify_full_trail () {
  const size_t end_of_trail = trail.size ();
  if (level) {
    notified = 0; // TODO: save the last notified root-level position
                  // somewhere and use it here
    notify_backtrack (0);
  }
  std::vector<int> assigned;

  int prev_max_level = 0;
  int current_level = 0;
  int propagator_level = 0;

  while (notified < end_of_trail) {
    int ilit = trail[notified++];
    // In theory, 0 ilit can happen due to pseudo-decision levels
    if (!ilit)
      current_level = prev_max_level + 1;
    else
      current_level = var (ilit).level;

    if (current_level > propagator_level) {
      if (assigned.size ())
        external->propagator->notify_assignment (assigned);
      while (current_level > propagator_level) {
        external->propagator->notify_new_decision_level ();
        propagator_level++;
      }
      assigned.clear ();
    }
    // Current level can be smaller than prev_max_level due to chrono
    if (current_level > prev_max_level)
      prev_max_level = current_level;

    if (!observed (ilit))
      continue;

    int elit = externalize (ilit); // TODO: double-check tainting
    CADICAL_assert (elit);
    // Fixed variables might get mapped (during compact) to another
    // non-observed but fixed variable.
    // This happens on root level, so notification about their assignment is
    // already done.
    CADICAL_assert (external->observed (elit) || fixed (ilit));
    if (!external->ervars[abs (elit)])
      assigned.push_back (elit);
  }
  if (assigned.size ())
    external->propagator->notify_assignment (assigned);
  assigned.clear ();

  // In case there are some left over empty levels on the top of the trail,
  // the external propagtor must be notified about them so the levels are
  // synced
  while (level > propagator_level) {
    external->propagator->notify_new_decision_level ();
    propagator_level++;
  }

  return;
}

/*----------------------------------------------------------------------------*/
//
// Check if the variable is assigned by decision.
//
bool Internal::is_decision (int ilit) {
  if (!level || fixed (ilit) || !val (ilit))
    return false;

  const int idx = vidx (ilit);
  Var &v = var (idx);
#ifndef CADICAL_NDEBUG
  LOG (v.reason,
       "is_decision: i%d (current level: %d, is_fixed: %d, v.level: %d, "
       "is_external_reason: %d, v.reason: )",
       ilit, level, fixed (ilit), v.level, v.reason == external_reason);
#endif
  if (!v.level || v.reason)
    return false;
  CADICAL_assert (!v.reason);
  return true;
}

void Internal::force_backtrack (size_t new_level) {
  if (!forced_backt_allowed || level <= 0 || new_level >= (size_t) level)
    return;

#ifndef CADICAL_NDEBUG
  LOG ("external propagator forces backtrack to decision level"
       "%zd (from level %d)",
       new_level, level);
#endif
  backtrack (new_level);
}

/*----------------------------------------------------------------------------*/
//
// Call external propagator to check if there is a literal to be propagated.
// The reason of the propagation is not necessarily asked at that point.
//
// In case the externally propagated literal is already falsified, the
// reason is asked and conflict analysis starts. In case the externally
// propagated literal is already satisfied, nothing happens.
//
bool Internal::external_propagate () {
  if (level)
    require_mode (SEARCH);
  CADICAL_assert (!unsat);

  size_t before = num_assigned;
  bool cb_repropagate_needed = true;
  while (cb_repropagate_needed && !conflict && external_prop &&
         !external_prop_is_lazy && !private_steps) {
#ifndef CADICAL_NDEBUG
    LOG ("external propagation starts (decision level: %d, trail size: "
         "%zd, notified %zd)",
         level, trail.size (), notified);
#endif
    cb_repropagate_needed = false;
    // external->reset_extended (); //TODO for inprocessing

    notify_assignments ();

    int elit = external->propagator->cb_propagate ();
    stats.ext_prop.ext_cb++;
    stats.ext_prop.eprop_call++;
    while (elit) {
      CADICAL_assert (external->is_observed[abs (elit)]);
      int ilit = external->e2i[abs (elit)];
      if (elit < 0)
        ilit = -ilit;
      int tmp = val (ilit);
#ifndef CADICAL_NDEBUG
      CADICAL_assert (fixed (ilit) || observed (ilit));
      LOG ("External propagation of e%d (i%d val: %d)", elit, ilit, tmp);
#endif
      if (!tmp) {
        // variable is not assigned, it can be propagated
        if (!level) {
          Clause *res = learn_external_reason_clause (ilit, elit);
#ifndef LOGGING
          LOG (res, "reason clause of external propagation of %d:", elit);
#endif
          (void) res;
        } else
          search_assign_external (ilit);
        stats.ext_prop.eprop_prop++;

        if (unsat || conflict)
          break;
        propagate ();
        if (unsat || conflict)
          break;
        notify_assignments ();
      } else if (tmp < 0) {
        LOG ("External propgation of %d is falsified under current trail",
             ilit);
        stats.ext_prop.eprop_conf++;
        int level_before = level;
        size_t assigned = num_assigned;
        Clause *res = learn_external_reason_clause (ilit, elit);
#ifndef LOGGING
        LOG (res, "reason clause of external propagation of %d:", elit);
#endif
        (void) res;
        bool trail_changed =
            (num_assigned != assigned || level != level_before ||
             propagated < trail.size ());

        if (unsat || conflict)
          break;

        if (trail_changed) {
          propagate ();
          if (unsat || conflict)
            break;
          notify_assignments ();
        }
      } // else (tmp > 0) -> the case of a satisfied literal is ignored
      elit = external->propagator->cb_propagate ();
      stats.ext_prop.ext_cb++;
      stats.ext_prop.eprop_call++;
    }

#ifndef CADICAL_NDEBUG
    LOG ("External propagation ends (decision level: %d, trail size: %zd, "
         "notified %zd)",
         level, trail.size (), notified);
#endif
    if (!unsat && !conflict) {
      int level_before = level;
      size_t assigned = num_assigned;
      bool has_external_clause = ask_external_clause ();
      // New observed variable might have triggered a backtrack during this
      // ask_external_clause call, so we need to propagate before continuing
      stats.ext_prop.ext_cb++;
      stats.ext_prop.elearn_call++;

      bool trail_changed =
          (num_assigned != assigned || level != level_before ||
           propagated < trail.size ());
      if (trail_changed) {
        propagate (); // unsat or conflict will be caught later
        if (!unsat || !conflict)
          notify_assignments ();
      }

#ifndef CADICAL_NDEBUG
      if (has_external_clause)
        LOG ("New external clauses are to be added.");
      else
        LOG ("No external clauses to be added.");
#endif

      while (has_external_clause) {
        level_before = level;
        assigned = num_assigned;

        add_external_clause (0);
        trail_changed =
            (num_assigned != assigned || level != level_before ||
             propagated < trail.size ());
        cb_repropagate_needed = true;

        if (unsat || conflict) {
          cb_repropagate_needed = false;
          break;
        }

        if (trail_changed) {
          propagate ();
          if (unsat || conflict) {
            cb_repropagate_needed = false;
            break;
          }

          notify_assignments ();
        }
        has_external_clause = ask_external_clause ();
        stats.ext_prop.ext_cb++;
        stats.ext_prop.elearn_call++;
      }
    }
#ifndef CADICAL_NDEBUG
    LOG ("External clause addition ends on decision level %d at trail "
         "size "
         "%zd (notified %zd)",
         level, trail.size (), notified);
#endif
  }
  if (before < num_assigned)
    did_external_prop = true;
  return !conflict;
}

/*----------------------------------------------------------------------------*/
//
// Helper function, calls 'cb_has_external_clause', while maintains the
// related redundancy type of the clause.
//

bool Internal::ask_external_clause () {
  ext_clause_forgettable = false;
  bool res =
      external->propagator->cb_has_external_clause (ext_clause_forgettable);

  return res;
}
/*----------------------------------------------------------------------------*/
//
// Literals of the externally learned clause must be reordered based on the
// assignment levels of the literals.
//
void Internal::move_literals_to_watch () {
  if (clause.size () < 2)
    return;
  if (!level)
    return;

  for (int i = 0; i < 2; i++) {
    int highest_position = i;
    int highest_literal = clause[i];

    int highest_level = var (highest_literal).level;
    int highest_value = val (highest_literal);

    for (size_t j = i + 1; j < clause.size (); j++) {
      const int other = clause[j];
      const int other_level = var (other).level;
      const int other_value = val (other);

      if (other_value < 0) {
        if (highest_value >= 0)
          continue;
        if (other_level <= highest_level)
          continue;
      } else if (other_value > 0) {
        if (highest_value > 0 && other_level >= highest_level)
          continue;
      } else {
        if (highest_value >= 0)
          continue;
      }

      highest_position = j;
      highest_literal = other;
      highest_level = other_level;
      highest_value = other_value;
    }
#ifndef CADICAL_NDEBUG
    LOG ("highest position: %d highest level: %d highest value: %d",
         highest_position, highest_level, highest_value);
#endif

    if (highest_position == i)
      continue;
    if (highest_position > i) {
      std::swap (clause[i], clause[highest_position]);
    }
  }
}

/*----------------------------------------------------------------------------*/
//
// Reads out from the external propagator the lemma/proapgation reason
// clause literal by literal. In case propagated_elit is 0, it is about an
// external clause via 'cb_add_external_clause_lit'. Otherwise, it is about
// learning the reason of 'propagated_elit' via 'cb_add_reason_clause_lit'.
// The learned clause is simplified by the current root-level assignment
// (i.e. root-level falsified literals are removed, root satisfied clauses
// are skipped). Duplicate literals are removed, tauotologies are detected
// and skipped. It always adds the original (un-simplified) external clause
// to the proof as an input clause and
// the simplified version of it (except exceptions below) as a derived
// clause.
//
// In case the external clause, after simplifications, is satisfied, no
// clause is constructed, and the function returns 0. In case the external
// clause, after simplifications, is empty, no clause is constructed, unsat
// is set true and the function returns 0. In case the external clause,
// after simplifications, is unit, no clause is constructed,
// 'Internal::clause' has the unit literal (without 0) and the function
// returns 0.
//
// In every other cases a new clause is constructed and the pointer is in
// newest_clause
//
void Internal::add_external_clause (int propagated_elit,
                                    bool no_backtrack) {
  CADICAL_assert (original.empty ());
  int elit = 0;

  if (propagated_elit) {
    // Propagation reason clauses are by default assumed to be forgettable
    // irredundant. In case they would be unforgettably important, the
    // propagator can add them as an explicit unforgettable external clause
    // or set 'are_reasons_forgettable' to false.
    ext_clause_forgettable = external->propagator->are_reasons_forgettable;
#ifndef CADICAL_NDEBUG
    LOG ("add external reason of propagated lit: %d", propagated_elit);
#endif
    elit = external->propagator->cb_add_reason_clause_lit (propagated_elit);
  } else
    elit = external->propagator->cb_add_external_clause_lit ();

  // we need to be build a new LRAT chain if we are already in the middle of
  // the analysis (like during failed assumptions)
  LOG (lrat_chain, "lrat chain before");
  std::vector<int64_t> lrat_chain_ext = std::move (lrat_chain);
  lrat_chain.clear ();
  clause.clear ();

  // Read out the external lemma into original and simplify it into clause
  CADICAL_assert (clause.empty ());
  CADICAL_assert (original.empty ());

  CADICAL_assert (!force_no_backtrack);
  CADICAL_assert (!from_propagator);
  force_no_backtrack = no_backtrack;
  from_propagator = true;
  while (elit) {
    CADICAL_assert (external->is_observed[abs (elit)]);
    external->add (elit);
    if (propagated_elit)
      elit =
          external->propagator->cb_add_reason_clause_lit (propagated_elit);
    else
      elit = external->propagator->cb_add_external_clause_lit ();
  }
  external->add (elit);
  CADICAL_assert (original.empty ());
  CADICAL_assert (clause.empty ());
  force_no_backtrack = false;
  from_propagator = false;
  lrat_chain = std::move (lrat_chain_ext);
  LOG (lrat_chain, "lrat chain after");
}

/*----------------------------------------------------------------------------*/
//
// Recursively calls 'learn_external_reason_clause' to explain every
// backward reachable externally propagated literal starting from 'ilit'.
//
void Internal::explain_reason (int ilit, Clause *reason, int &open) {
  if (!opts.exteagerreasons)
    return;
#ifndef CADICAL_NDEBUG
  LOG (reason, "explain_reason of %d (open: %d)", ilit, open);
#endif
  CADICAL_assert (reason);
  CADICAL_assert (reason != external_reason);
  for (const auto &other : *reason) {
    if (other == ilit)
      continue;
    Flags &f = flags (other);
    if (f.seen)
      continue;
    Var &v = var (other);
    if (!v.level)
      continue;
    CADICAL_assert (val (other) < 0);
    CADICAL_assert (v.level <= level);
    if (v.reason == external_reason) {
      v.reason = learn_external_reason_clause (-other, 0, true);
    }
    if (v.level && v.reason) {
      f.seen = true;
      open++;
    }
  }
}

/*----------------------------------------------------------------------------*/
//
// In case external propagation was used, the reason clauses of the relevant
// propagations must be learned lazily before/during conflict analysis.
// While conflict analysis needs to analyze only the current level, lazy
// clause learning must check every clause on every level that is backward
// reachable from the conflicting clause to guarantee that the assignment
// levels of the variables are accurate. So this explanation round is
// separated from the conflict analysis, thereby guranteeing that the flags
// and datastructures can be properly used later.
//
// This function must be called before the conflict analysis, in order to
// guarantee that every relevant reason clause is indeed learned already and
// to be sure that the levels of assignments are set correctly.
//
// Later TODO: experiment with bounded explanation (explain only those
// literals that are directly used during conflict analysis +
// minimizing/shrinking). The assignment levels are then only
// over-approximated.
//
void Internal::explain_external_propagations () {
  CADICAL_assert (conflict);
  CADICAL_assert (clause.empty ());

  Clause *reason = conflict;
  std::vector<int> seen_lits;
  int open = 0; // Seen but not explained literal

  explain_reason (0, reason, open); // marks conflict clause lits as seen
  int i = trail.size ();            // Start at end-of-trail
  while (i > 0) {
    const int lit = trail[--i];
    if (!flags (lit).seen)
      continue;
    seen_lits.push_back (lit);
    Var &v = var (lit);
    if (!v.level)
      continue;
    if (v.reason) {
      open--;
      explain_reason (lit, v.reason, open);
    }
    if (!open)
      break;
  }
  CADICAL_assert (!open);

  if (!opts.exteagerrecalc) {
    for (auto lit : seen_lits) {
      Flags &f = flags (lit);
      f.seen = false;
    }
#ifndef CADICAL_NDEBUG
    for (auto idx : vars) {
      CADICAL_assert (!flags (idx).seen);
    }
#endif
  }

  // Traverse now in the opposite direction (from lower to higher levels)
  // and calculate the actual assignment level for the seen assignments.
  for (auto it = seen_lits.rbegin (); it != seen_lits.rend (); ++it) {
    const int lit = *it;
    Flags &f = flags (lit);
    Var &v = var (lit);
    if (v.reason) {
      int real_level = 0;
      for (const auto &other : *v.reason) {
        if (other == lit)
          continue;
        int tmp = var (other).level;
        if (tmp > real_level)
          real_level = tmp;
      }
      if (v.level && !real_level) {
        build_chain_for_units (lit, v.reason, 1);
        learn_unit_clause (lit);
        lrat_chain.clear ();
        v.reason = 0;
      }
      CADICAL_assert (v.level >= real_level);
      if (v.level > real_level) {
        v.level = real_level;
      }
    }
    f.seen = false;
  }

#if 0 // has been fuzzed extensively
  for (auto idx : vars) {
    CADICAL_assert (!flags (idx).seen);
  }
#endif
}

/*----------------------------------------------------------------------------*/
//
// Learns the reason clause of the propagation of ilit from the
// external propagator via 'add_external_clause'.
// In case of falsified propagation steps, if the propagated literal is
// already fixed to the opposite value, externalize will not necessarily
// give back the original elit (but an equivalent one). To avoid that, in
// falsified propagation cases the propagated elit is added as a second
// argument.
//
Clause *Internal::learn_external_reason_clause (int ilit,
                                                int falsified_elit,
                                                bool no_backtrack) {
  CADICAL_assert (external->propagator);
  // we cannot modify clause during analysis
  auto clause_tmp = std::move (clause);

  CADICAL_assert (clause.empty ());
  CADICAL_assert (original.empty ());

  stats.ext_prop.eprop_expl++;

  int elit = 0;
  if (!falsified_elit) {
    CADICAL_assert (!fixed (ilit));
    elit = externalize (ilit);
  } else
    elit = falsified_elit;

  LOG ("ilit: %d, elit: %d", ilit, elit);
  add_external_clause (elit, no_backtrack);

#ifndef CADICAL_NDEBUG
  if (!falsified_elit && newest_clause) {
    // Check if external propagation is correct wrt to the topological order
    // defined by the trail. In case it is a falsified external propagation
    // step, the order does not matter, the reason simply supposed to be a
    // falsified clause.
    const int propagated_ilit = ilit;
    for (auto const reason_ilit : *newest_clause) {
      CADICAL_assert (var (reason_ilit).trail <= var (propagated_ilit).trail);
    }
  }
#endif

  clause = std::move (clause_tmp);
  return newest_clause;
}

/*----------------------------------------------------------------------------*/
//
// Helper function to be able to call learn_external_reason_clause when the
// internal clause is already used in the caller side (for example during
// proof checking). These calls are assumed to be without a falsified elit.
// Dont use it in general instead of learn_external_reason_clause because it
// does not support the corner cases where a literal remains in clause.
//
Clause *Internal::wrapped_learn_external_reason_clause (int ilit) {
  Clause *res;
  std::vector<int64_t> chain_tmp{std::move (lrat_chain)};
  lrat_chain.clear ();
  if (clause.empty ()) {
    res = learn_external_reason_clause (ilit, 0, true);
  } else {
    std::vector<int> clause_tmp{std::move (clause)};
    clause.clear ();
    res = learn_external_reason_clause (ilit, 0, true);
    // The learn_external_reason clause can leave a literal in clause when
    // there is a falsified elit arg. Here it is not allowed to
    // happen.
    CADICAL_assert (clause.empty ());

    clause = std::move (clause_tmp);
    clause_tmp.clear ();
  }
  CADICAL_assert (lrat_chain.empty ());
  lrat_chain = std::move (chain_tmp);
  chain_tmp.clear ();
  return res;
}

/*----------------------------------------------------------------------------*/
//
// Checks if the new clause forces backtracking, new assignments or conflict
// analysis
//
void Internal::handle_external_clause (Clause *res) {
  if (from_propagator)
    stats.ext_prop.elearned++;
  // at level 0 we have to do nothing...
  if (!level)
    return;
  if (!res) {
    if (from_propagator)
      stats.ext_prop.elearn_prop++;
    // new unit clause. For now just backtrack.
    CADICAL_assert (!force_no_backtrack);
    CADICAL_assert (level);
    // if (!opts.chrono) {
    backtrack ();
    // }
    return;
  }
  if (from_propagator)
    stats.ext_prop.elearned++;
  CADICAL_assert (res->size >= 2);
  const int pos0 = res->literals[0];
  const int pos1 = res->literals[1];
  if (force_no_backtrack) {
    CADICAL_assert (val (pos1) < 0);
    CADICAL_assert (val (pos0) >= 0);
    return;
    // TODO: maybe fix levels
  }
  const int l1 = var (pos1).level;
  if (val (pos0) < 0) { // conflicting or propagating clause
    CADICAL_assert (0 < l1 && l1 <= var (pos0).level);
    if (!opts.chrono) {
      backtrack (l1);
    }
    if (val (pos0) < 0) {
      conflict = res;
      if (!from_propagator) {
        // its better to backtrack instead of analyze
        backtrack (l1 - 1);
        conflict = 0;
        CADICAL_assert (!val (pos0) && !val (pos1));
      }
    } else {
      search_assign_driving (pos0, res);
    }
    if (from_propagator)
      stats.ext_prop.elearn_conf++;
    return;
  }
  if (val (pos1) < 0 && !val (pos0)) { // propagating clause
    if (!opts.chrono) {
      backtrack (l1);
    }
    search_assign_driving (pos0, res);
    if (from_propagator)
      stats.ext_prop.elearn_conf++;
    return;
  }
}

/*----------------------------------------------------------------------------*/
//
// Asks the external propagator if the current solution is OK
// by calling 'cb_check_found_model (model)'.
//
// The checked model is built up after everything is restored
// from the reconstruction stack and every variable is reactivated
// and so it is not just simply the trail (i.e. it can be expensive).
//
// If the external propagator approves the model, the function
// returns true.
//
// If the propagator does not approve the model, the solver asks
// the propagator to add an external clause.
// This external clause, however, does NOT have to be falsified by
// the current model. The possible cases and reactions are described
// below in the function. The possible states after that function:
// - A solution was found and accepted by the external propagator
// - A conflicting clause was learned from the external propagator
// - The empty clause was learned due to something new learned from
// the external propagator.
//
// In case only new variables were introduced, but no new clauses were
// added, the function will return without a conflict to the outer CDCL
// loop, where the new (not yet satisfied) variables are recognized and
// the search continues.
bool Internal::external_check_solution () {
  if (!external_prop)
    return true;

  bool trail_changed = true;
  bool added_new_clauses = false;
  while (trail_changed || added_new_clauses) {
    notify_assignments ();
    if (!satisfied ())
      break;
    trail_changed = false; // to be on the safe side
    added_new_clauses = false;
    LOG ("Final check by external propagator is invoked.");
    stats.ext_prop.echeck_call++;
    external->reset_extended ();
    external->extend ();

    std::vector<int> etrail;

    // Here the variables must be filtered by external->is_observed,
    // because fixed variables are internally not necessarily observed
    // anymore.
    for (int idx = 1; idx <= external->max_var; idx++) {
      if (!external->is_observed[idx])
        continue;
      const int lit = external->ival (idx);
      etrail.push_back (lit);
#ifndef CADICAL_NDEBUG
#ifdef LOGGING
      bool p = external->vals[idx];
      LOG ("evals[%d]: %d ival(%d): %d", idx, p, idx, lit);
#endif
#endif
    }

    bool is_consistent =
        external->propagator->cb_check_found_model (etrail);
    stats.ext_prop.ext_cb++;
    if (is_consistent) {
      LOG ("Found solution is approved by external propagator.");
      return true;
    }

    bool has_external_clause = ask_external_clause ();

    stats.ext_prop.ext_cb++;
    stats.ext_prop.elearn_call++;

    if (has_external_clause)
      LOG (
          "Found solution triggered new clauses from external propagator.");

    while (has_external_clause) {
      int level_before = level;
      size_t assigned = num_assigned;
      add_external_clause (0);
      bool trail_changed =
          (num_assigned != assigned || level != level_before ||
           propagated < trail.size ());
      added_new_clauses = true;
      //
      // There are many possible scenarios here:
      // - Learned conflicting clause: return to CDCL loop (conflict true)
      // - Learned conflicting unit clause that after backtrack+BCP leads to
      //   a new complete solution: force the outer loop to check the new
      //   model (trail_changed is true, but (conflict & unsat) is false)
      // - Learned empty clause: return to CDCL loop (unsat true)
      // - Learned a non-conflicting unit clause:
      //   Though it does not invalidate the current solution, the solver
      //   will backtrack to the root level and will repropagate it. The
      //   search will start again (saved phases hopefully make it quick),
      //   but it is needed in order to guarantee that every fixed variable
      //   is properly handled+notified (important for incremental use
      //   cases).
      // - Otherwise: the solution is considered approved and the CDCL-loop
      //   can return with res = 10.
      //
      if (unsat || conflict || trail_changed)
        break;
      has_external_clause = ask_external_clause ();
      stats.ext_prop.ext_cb++;
      stats.ext_prop.elearn_call++;
    }
    LOG ("No more external clause to add.");
    if (unsat || conflict)
      break;
  }

  if (!unsat && conflict) {
    const int conflict_level = var (conflict->literals[0]).level;
    if (conflict_level != level) {
      backtrack (conflict_level);
    }
  }

  return !conflict;
}

/*----------------------------------------------------------------------------*/
//
// Notify the external propagator that an observed variable got assigned.
//
void Internal::notify_assignments () {
  if (!external_prop || external_prop_is_lazy || private_steps)
    return;

  const size_t end_of_trail = trail.size ();

  if (notified >= end_of_trail)
    return;

  LOG ("notify external propagator about new assignments");
  std::vector<int> assigned;

  while (notified < end_of_trail) {
    int ilit = trail[notified++];
    if (!observed (ilit))
      continue;

    int elit = externalize (ilit); // TODO: double-check tainting
    CADICAL_assert (elit);
    if (external->ervars[abs (elit)])
      continue;
    // Fixed variables might get mapped (during compact) to another
    // non-observed but fixed variable.
    // This happens on root level, so notification about their assignment is
    // already done.
    CADICAL_assert (external->observed (elit) ||
            (fixed (ilit) && !external->ervars[abs (elit)]));
    assigned.push_back (elit);
  }

  external->propagator->notify_assignment (assigned);
  return;
}

/*----------------------------------------------------------------------------*/

void Internal::connect_propagator () {
  if (level)
    backtrack ();
}

/*----------------------------------------------------------------------------*/
//
// Notify the external propagator that a new decision level is started.
//
void Internal::notify_decision () {
  if (!external_prop || external_prop_is_lazy || private_steps)
    return;
  external->propagator->notify_new_decision_level ();
}

/*----------------------------------------------------------------------------*/
//
// Notify the external propagator that backtrack to new_level.
//
void Internal::notify_backtrack (size_t new_level) {
  if (!external_prop || external_prop_is_lazy || private_steps)
    return;
  external->propagator->notify_backtrack (new_level);
}

/*----------------------------------------------------------------------------*/
//
// Ask the external propagator if there is a suggested literal as next
// decision.
//
int Internal::ask_decision () {
  if (!external_prop || external_prop_is_lazy || private_steps)
    return 0;

  CADICAL_assert (!unsat);
  CADICAL_assert (!conflict);
  notify_assignments ();
  int level_before = level;
  forced_backt_allowed = true;
  int elit = external->propagator->cb_decide ();
  forced_backt_allowed = false;
  stats.ext_prop.ext_cb++;

  if (level_before != level) {

    propagate ();
    CADICAL_assert (!unsat);
    CADICAL_assert (!conflict);
    notify_assignments ();

    // In case the external propagator forced to backtrack below the
    // pseduo decision levels, we must go back to the CDCL loop instead of
    // making a decision.
    if ((size_t) level < assumptions.size () ||
        ((size_t) level == assumptions.size () && constraint.size ())) {
      return 0;
    }
  }

  if (!elit)
    return 0;
  LOG ("external propagator proposes decision: %d", elit);
  CADICAL_assert (external->is_observed[abs (elit)]);
  if (!external->is_observed[abs (elit)])
    return 0;

  int ilit = external->e2i[abs (elit)];
  if (elit < 0)
    ilit = -ilit;

  CADICAL_assert (fixed (ilit) || observed (ilit));

  LOG ("Asking external propagator for decision returned: %d (internal: "
       "%d, fixed: %d, val: %d)",
       elit, ilit, fixed (ilit), val (ilit));

  if (fixed (ilit) || val (ilit)) {
    LOG ("Proposed decision variable is already assigned, falling back to "
         "internal decision.");
    return 0;
  }

  return ilit;
}

/*----------------------------------------------------------------------------*/
//
// Check if the clause is a forgettable clause coming from the external
// propagator.
//
bool Internal::is_external_forgettable (int64_t id) {
  CADICAL_assert (opts.check);
  return (external->forgettable_original.find (id) !=
          external->forgettable_original.end ());
}

/*----------------------------------------------------------------------------*/
//
// When an external forgettable clause is deleted, it is marked in the
// 'forgettable_original' hash, so that the internal model checking can
// ignore it.
//
void Internal::mark_garbage_external_forgettable (int64_t id) {
  CADICAL_assert (opts.check);
  CADICAL_assert (is_external_forgettable (id));

  LOG (external->forgettable_original[id],
       "forgettable external lemma is deleted:");
  // Mark as removed by flipping the first flag to false.
  external->forgettable_original[id][0] = 0;
}

/*----------------------------------------------------------------------------*/
//
// Check that the literals in the clause are properly ordered. Used only
// internally for debug purposes.
//
void Internal::check_watched_literal_invariants () {
#ifndef CADICAL_NDEBUG
  int v0 = 0;
  int v1 = 0;

  if (val (clause[0]) > 0)
    v0 = 1;
  else if (val (clause[0]) < 0)
    v0 = -1;

  if (val (clause[1]) > 0)
    v1 = 1;
  else if (val (clause[1]) < 0)
    v1 = -1;
  CADICAL_assert (v0 >= v1);
#endif
  if (val (clause[0]) > 0) {
    if (val (clause[1]) > 0) { // Case 1: Both literals are satisfied
      // They are ordered by lower to higher decision level
      CADICAL_assert (var (clause[0]).level <= var (clause[1]).level);

      // Every other literal of the clause is either
      //    - satisfied at higher level
      //    - unassigned
      //    - falsified
      for (size_t i = 2; i < clause.size (); i++)
        CADICAL_assert (val (clause[i]) <= 0 ||
                (var (clause[1]).level <= var (clause[i]).level));

    } else if (val (clause[1]) ==
               0) { // Case 2: First satisfied, next unassigned

      // Every other literal of the clause is either
      //    - unassigned
      //    - falsified
      for (size_t i = 2; i < clause.size (); i++)
        CADICAL_assert (val (clause[i]) <= 0);

    } else { // Case 3: First satisfied, next falsified -> could have been a
             // reason of a previous propagation
      // Every other literal of the clause is falsified but at a lower
      // decision level
      for (size_t i = 2; i < clause.size (); i++)
        CADICAL_assert (val (clause[i]) < 0 &&
                (var (clause[1]).level >= var (clause[i]).level));
    }
  } else if (val (clause[0]) == 0) {
    if (val (clause[1]) == 0) { // Case 4: Both literals are unassigned

      // Every other literal of the clause is either
      //    - unassigned
      //    - falsified
      for (size_t i = 2; i < clause.size (); i++)
        CADICAL_assert (val (clause[i]) <= 0);

    } else { // Case 5: First unassigned, next falsified -> PROPAGATE
      // Every other literal of the clause is falsified but at a lower
      // decision level
      for (size_t i = 2; i < clause.size (); i++)
        CADICAL_assert (val (clause[i]) < 0 &&
                (var (clause[1]).level >= var (clause[i]).level));
    }
  } else {
    CADICAL_assert (val (clause[0]) < 0 &&
            val (clause[1]) < 0); // Case 6: Both literals are falsified

    // They are ordered by higher to lower decision level
    CADICAL_assert (var (clause[0]).level >= var (clause[1]).level);

    // Every other literal of the clause is falsified, but at a lower level
    for (size_t i = 2; i < clause.size (); i++)
      CADICAL_assert (val (clause[i]) < 0 &&
              (var (clause[1]).level >= var (clause[i]).level));
  }
}

#ifndef CADICAL_NDEBUG

/*----------------------------------------------------------------------------*/
//
// An expensive function that can be used for deep-debug trail-related
// issues in mobical. Do not use it unless it is really unavoidable.
//
// eq_class contains all the merged external literals that are currently
// compacted to the internal literal of trail[0] and return true.
//
// In case trail[0] does not exists or is not on the root level, the
// function returns false (indicating that there was no merger literal
// found).
//
bool Internal::get_merged_literals (std::vector<int> &eq_class) {
  eq_class.clear ();

  if (!trail.size ())
    return false;

  int ilit = trail[0];
  size_t lit_level = var (ilit).level;

  if (!lit_level) {
    // Collect all the variables that are merged and mapped to that ilit
    size_t e2i_size = external->e2i.size ();
    int ivar = abs (ilit);
    for (size_t i = 0; i < e2i_size; i++) {
      int other = abs (external->e2i[i]);
      if (other == ivar) {
        if (external->e2i[i] == ilit)
          eq_class.push_back (i);
        else
          eq_class.push_back (-1 * i);
      }
    }

    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/
//
// Collect all external variables that are FIXED internally. Again an
// expensive function that should be called only for debugging in mobical.
//
// Do not use it unless it is really unavoidable.
//
void Internal::get_all_fixed_literals (std::vector<int> &fixed_lits) {
  fixed_lits.clear ();
  if (!trail.size ())
    return;

  int e2i_size = external->e2i.size ();
  int ilit;
  for (int eidx = 1; eidx < e2i_size; eidx++) {
    ilit = external->e2i[eidx];
    if (ilit && !external->ervars[eidx]) {
      Flags &f = flags (ilit);
      if (f.status == Flags::FIXED) {
        fixed_lits.push_back (vals[abs (ilit)] * eidx);
      }
    }
  }
}
#endif

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
