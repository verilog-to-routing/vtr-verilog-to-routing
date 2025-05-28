#include "propinitially.h"
#include "analyze.h"
#include "fastassign.h"
#include "print.h"
#include "trail.h"

ABC_NAMESPACE_IMPL_START

#define PROPAGATE_LITERAL initially_propagate_literal
#define PROPAGATION_TYPE "initially"

#include "proplit.h"

static inline void
update_initial_propagation_statistics (kissat *solver,
                                       const unsigned *saved_propagate) {
  KISSAT_assert (saved_propagate <= solver->propagate);
  const unsigned propagated = solver->propagate - saved_propagate;

  LOG ("propagated %u literals", propagated);
  LOG ("propagation took %" PRIu64 " ticks", solver->ticks);

  ADD (propagations, propagated);
  ADD (ticks, solver->ticks);
}

static clause *initially_propagate (kissat *solver) {
  clause *res = 0;
  unsigned *propagate = solver->propagate;
  while (!res && propagate != END_ARRAY (solver->trail))
    res = initially_propagate_literal (solver, *propagate++);
  solver->propagate = propagate;
  return res;
}

bool kissat_initially_propagate (kissat *solver) {
  KISSAT_assert (!solver->probing);
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);

  START (propagate);

  solver->ticks = 0;
  const unsigned *saved_propagate = solver->propagate;
  clause *conflict = initially_propagate (solver);
  update_initial_propagation_statistics (solver, saved_propagate);
  kissat_update_conflicts_and_trail (solver, conflict, true);
  if (conflict) {
    int res = kissat_analyze (solver, conflict);
    KISSAT_assert (solver->inconsistent);
    KISSAT_assert (res == 20);
    (void) res;
  }

  STOP (propagate);

  return !conflict;
}

ABC_NAMESPACE_IMPL_END
