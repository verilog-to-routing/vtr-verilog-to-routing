#include "backtrack.h"
#include "analyze.h"
#include "inline.h"
#include "inlineheap.h"
#include "inlinequeue.h"
#include "print.h"
#include "proprobe.h"
#include "propsearch.h"
#include "trail.h"

ABC_NAMESPACE_IMPL_START

static inline void unassign (kissat *solver, value *values, unsigned lit) {
  LOG ("unassign %s", LOGLIT (lit));
  KISSAT_assert (values[lit] > 0);
  const unsigned not_lit = NOT (lit);
  values[lit] = values[not_lit] = 0;
  KISSAT_assert (solver->unassigned < VARS);
  solver->unassigned++;
}

static inline void add_unassigned_variable_back_to_queue (kissat *solver,
                                                          links *links,
                                                          unsigned lit) {
  KISSAT_assert (!solver->stable);
  const unsigned idx = IDX (lit);
  if (links[idx].stamp > solver->queue.search.stamp)
    kissat_update_queue (solver, links, idx);
}

static inline void add_unassigned_variable_back_to_heap (kissat *solver,
                                                         heap *scores,
                                                         unsigned lit) {
  KISSAT_assert (solver->stable);
  const unsigned idx = IDX (lit);
  if (!kissat_heap_contains (scores, idx))
    kissat_push_heap (solver, scores, idx);
}

static void kissat_update_target_and_best_phases (kissat *solver) {
  if (solver->probing)
    return;

  if (!solver->stable)
    return;

  const unsigned assigned = kissat_assigned (solver);
#ifdef LOGGING
  LOG ("updating target and best phases");
  LOG ("currently %u variables assigned", assigned);
#endif

  if (solver->target_assigned < assigned) {
    kissat_extremely_verbose (solver,
                              "updating target assigned "
                              "trail height from %u to %u",
                              solver->target_assigned, assigned);
    solver->target_assigned = assigned;
    kissat_save_target_phases (solver);
    INC (target_saved);
  }

  if (solver->best_assigned < assigned) {
    kissat_extremely_verbose (solver,
                              "updating best assigned "
                              "trail height from %u to %u",
                              solver->best_assigned, assigned);
    solver->best_assigned = assigned;
    kissat_save_best_phases (solver);
    INC (best_saved);
  }
}

void kissat_backtrack_without_updating_phases (kissat *solver,
                                               unsigned new_level) {
  KISSAT_assert (solver->level >= new_level);
  if (solver->level == new_level)
    return;

  LOG ("backtracking to decision level %u", new_level);

  frame *new_frame = &FRAME (new_level + 1);
  SET_END_OF_STACK (solver->frames, new_frame);

  value *values = solver->values;
  unsigned *trail = BEGIN_ARRAY (solver->trail);
  unsigned *new_end = trail + new_frame->trail;
  assigned *assigned = solver->assigned;

  unsigned *old_end = END_ARRAY (solver->trail);
  unsigned unassigned = 0, reassigned = 0;

  unsigned *q = new_end;
  if (solver->stable) {
    heap *scores = SCORES;
    for (const unsigned *p = q; p != old_end; p++) {
      const unsigned lit = *p;
      const unsigned idx = IDX (lit);
      KISSAT_assert (idx < VARS);
      struct assigned *a = assigned + idx;
      const unsigned level = a->level;
      if (level <= new_level) {
        const unsigned new_trail = q - trail;
        KISSAT_assert (new_trail <= a->trail);
        a->trail = new_trail;
        *q++ = lit;
        LOG ("reassign %s", LOGLIT (lit));
        reassigned++;
      } else {
        unassign (solver, values, lit);
        add_unassigned_variable_back_to_heap (solver, scores, lit);
        unassigned++;
      }
    }
  } else {
    links *links = solver->links;
    for (const unsigned *p = q; p != old_end; p++) {
      const unsigned lit = *p;
      const unsigned idx = IDX (lit);
      KISSAT_assert (idx < VARS);
      struct assigned *a = assigned + idx;
      const unsigned level = a->level;
      if (level <= new_level) {
        const unsigned new_trail = q - trail;
        KISSAT_assert (new_trail <= a->trail);
        a->trail = new_trail;
        *q++ = lit;
        LOG ("reassign %s", LOGLIT (lit));
        reassigned++;
      } else {
        unassign (solver, values, lit);
        add_unassigned_variable_back_to_queue (solver, links, lit);
        unassigned++;
      }
    }
  }
  SET_END_OF_ARRAY (solver->trail, q);

  solver->level = new_level;
  LOG ("unassigned %u literals", unassigned);
  LOG ("reassigned %u literals", reassigned);
  (void) unassigned, (void) reassigned;

  KISSAT_assert (new_end <= END_ARRAY (solver->trail));
  LOG ("propagation will resume at trail position %zu",
       (size_t) (new_end - trail));
  solver->propagate = new_end;

  KISSAT_assert (!solver->extended);
}

void kissat_backtrack_in_consistent_state (kissat *solver,
                                           unsigned new_level) {
  kissat_update_target_and_best_phases (solver);
  kissat_backtrack_without_updating_phases (solver, new_level);
}

void kissat_backtrack_after_conflict (kissat *solver, unsigned new_level) {
  if (solver->level)
    kissat_backtrack_without_updating_phases (solver, solver->level - 1);
  kissat_update_target_and_best_phases (solver);
  kissat_backtrack_without_updating_phases (solver, new_level);
}

void kissat_backtrack_propagate_and_flush_trail (kissat *solver) {
  if (solver->level) {
    KISSAT_assert (solver->watching);
    kissat_backtrack_in_consistent_state (solver, 0);
#ifndef KISSAT_NDEBUG
    clause *conflict =
#endif
        solver->probing ? kissat_probing_propagate (solver, 0, true)
                        : kissat_search_propagate (solver);
    KISSAT_assert (!conflict);
  }

  KISSAT_assert (kissat_propagated (solver));
  KISSAT_assert (kissat_trail_flushed (solver));
}

ABC_NAMESPACE_IMPL_END
