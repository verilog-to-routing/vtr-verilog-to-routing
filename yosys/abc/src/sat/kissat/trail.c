#include "trail.h"
#include "backtrack.h"
#include "inline.h"
#include "propsearch.h"

ABC_NAMESPACE_IMPL_START

void kissat_flush_trail (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->unflushed);
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (kissat_propagated (solver));
  KISSAT_assert (SIZE_ARRAY (solver->trail) == solver->unflushed);
  LOG ("flushed %zu units from trail", SIZE_ARRAY (solver->trail));
  CLEAR_ARRAY (solver->trail);
  kissat_reset_propagate (solver);
  solver->unflushed = 0;
}

void kissat_mark_reason_clauses (kissat *solver, reference start) {
  LOG ("starting marking reason clauses at clause[%" REFERENCE_FORMAT "]",
       start);
  KISSAT_assert (!solver->unflushed);
#ifdef LOGGING
  unsigned reasons = 0;
#endif
  ward *arena = BEGIN_STACK (solver->arena);
  for (all_stack (unsigned, lit, solver->trail)) {
    assigned *a = ASSIGNED (lit);
    KISSAT_assert (a->level > 0);
    if (a->binary)
      continue;
    const reference ref = a->reason;
    KISSAT_assert (ref != UNIT_REASON);
    if (ref == DECISION_REASON)
      continue;
    if (ref < start)
      continue;
    clause *c = (clause *) (arena + ref);
    KISSAT_assert (kissat_clause_in_arena (solver, c));
    c->reason = true;
#ifdef LOGGING
    reasons++;
#endif
  }
  LOG ("marked %u reason clauses", reasons);
}

bool kissat_flush_and_mark_reason_clauses (kissat *solver,
                                           reference start) {
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (kissat_propagated (solver));

  if (solver->unflushed) {
    LOG ("need to flush %u units from trail", solver->unflushed);
    kissat_backtrack_propagate_and_flush_trail (solver);
  } else {
    LOG ("no need to flush units from trail (all units already flushed)");
    kissat_mark_reason_clauses (solver, start);
  }

  return true;
}

void kissat_unmark_reason_clauses (kissat *solver, reference start) {
  LOG ("starting unmarking reason clauses at clause[%" REFERENCE_FORMAT "]",
       start);
  KISSAT_assert (!solver->unflushed);
#ifdef LOGGING
  unsigned reasons = 0;
#endif
  ward *arena = BEGIN_STACK (solver->arena);
  for (all_stack (unsigned, lit, solver->trail)) {
    assigned *a = ASSIGNED (lit);
    KISSAT_assert (a->level > 0);
    if (a->binary)
      continue;
    const reference ref = a->reason;
    KISSAT_assert (ref != UNIT_REASON);
    if (ref == DECISION_REASON)
      continue;
    if (ref < start)
      continue;
    clause *c = (clause *) (arena + ref);
    KISSAT_assert (kissat_clause_in_arena (solver, c));
    KISSAT_assert (c->reason);
    c->reason = false;
#ifdef LOGGING
    reasons++;
#endif
  }
  LOG ("unmarked %u reason clauses", reasons);
}

ABC_NAMESPACE_IMPL_END
