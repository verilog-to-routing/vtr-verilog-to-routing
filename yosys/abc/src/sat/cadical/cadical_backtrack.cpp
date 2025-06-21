#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// The global assignment stack can only be (partially) reset through
// 'backtrack' which is the only function using 'unassign' (inlined and thus
// local to this file).  It turns out that 'unassign' does not need a
// specialization for 'probe' nor 'vivify' and thus it is shared.

inline void Internal::unassign (int lit) {
  CADICAL_assert (val (lit) > 0);
  set_val (lit, 0);

  int idx = vidx (lit);
  LOG ("unassign %d @ %d", lit, var (idx).level);
  num_assigned--;

  // In the standard EVSIDS variable decision heuristic of MiniSAT, we need
  // to push variables which become unassigned back to the heap.
  //
  if (!scores.contains (idx))
    scores.push_back (idx);

  // For VMTF we need to update the 'queue.unassigned' pointer in case this
  // variable sits after the variable to which 'queue.unassigned' currently
  // points.  See our SAT'15 paper for more details on this aspect.
  //
  if (queue.bumped < btab[idx])
    update_queue_unassigned (idx);
}

/*------------------------------------------------------------------------*/

// Update the current target maximum assignment and also the very best
// assignment.  Whether a trail produces a conflict is determined during
// propagation.  Thus that all functions in the 'search' loop after
// propagation can assume that 'no_conflict_until' is valid.  If a conflict
// is found then the trail before the last decision is used (see the end of
// 'propagate').  During backtracking we can then save this largest
// propagation conflict free assignment.  It is saved as both 'target'
// assignment for picking decisions in 'stable' mode and if it is the
// largest ever such assignment also as 'best' assignment. This 'best'
// assignment can then be used in future stable decisions after the next
// 'rephase_best' overwrites saved phases with it.

void Internal::update_target_and_best () {

  bool reset = (rephased && stats.conflicts > last.rephase.conflicts);

  if (reset) {
    target_assigned = 0;
    if (rephased == 'B')
      best_assigned = 0; // update it again
  }

  if (no_conflict_until > target_assigned) {
    copy_phases (phases.target);
    target_assigned = no_conflict_until;
    LOG ("new target trail level %zu", target_assigned);
  }

  if (no_conflict_until > best_assigned) {
    copy_phases (phases.best);
    best_assigned = no_conflict_until;
    LOG ("new best trail level %zu", best_assigned);
  }

  if (reset) {
    report (rephased);
    rephased = 0;
  }
}

/*------------------------------------------------------------------------*/

void Internal::backtrack (int new_level) {
  CADICAL_assert (new_level <= level);
  if (new_level == level)
    return;

  update_target_and_best ();
  backtrack_without_updating_phases (new_level);
}

void Internal::backtrack_without_updating_phases (int new_level) {

  CADICAL_assert (new_level <= level);
  if (new_level == level)
    return;

  stats.backtracks++;

  CADICAL_assert (num_assigned == trail.size ());

  const size_t assigned = control[new_level + 1].trail;

  LOG ("backtracking to decision level %d with decision %d and trail %zd",
       new_level, control[new_level].decision, assigned);

  const size_t end_of_trail = trail.size ();
  size_t i = assigned, j = i;

#ifdef LOGGING
  int unassigned = 0;
#endif
  int reassigned = 0;

  notify_backtrack (new_level);
  if (external_prop && !external_prop_is_lazy && !private_steps &&
      notified > assigned) {
    LOG ("external propagator is notified about some unassignments (trail: "
         "%zd, notified: %zd).",
         trail.size (), notified);
    notified = assigned;
  }

  while (i < end_of_trail) {
    int lit = trail[i++];
    Var &v = var (lit);
    if (v.level > new_level) {
      unassign (lit);
#ifdef LOGGING
      unassigned++;
#endif
    } else {
      // This is the essence of the SAT'18 paper on chronological
      // backtracking.  It is possible to just keep out-of-order assigned
      // literals on the trail without breaking the solver (after some
      // modifications to 'analyze' - see 'opts.chrono' guarded code there).
      CADICAL_assert (opts.chrono || external_prop || did_external_prop);
#ifdef LOGGING
      if (!v.level)
        LOG ("reassign %d @ 0 unit clause %d", lit, lit);
      else
        LOG (v.reason, "reassign %d @ %d", lit, v.level);
#endif
      trail[j] = lit;
      v.trail = j++;
      reassigned++;
    }
  }
  trail.resize (j);
  LOG ("unassigned %d literals %.0f%%", unassigned,
       percent (unassigned, unassigned + reassigned));
  LOG ("reassigned %d literals %.0f%%", reassigned,
       percent (reassigned, unassigned + reassigned));

  if (propagated > assigned)
    propagated = assigned;
  if (propagated2 > assigned)
    propagated2 = assigned;
  if (no_conflict_until > assigned)
    no_conflict_until = assigned;

  propergated = 0; // Always go back to root-level.

  CADICAL_assert (notified <= assigned + reassigned);
  if (reassigned) {
    notify_assignments ();
  }

  control.resize (new_level + 1);
  level = new_level;
  if (tainted_literal) {
    CADICAL_assert (opts.ilb);
    if (!val (tainted_literal)) {
      tainted_literal = 0;
    }
  }
  CADICAL_assert (num_assigned == trail.size ());
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
