#include "propbeyond.h"
#include "fastassign.h"
#include "trail.h"

ABC_NAMESPACE_IMPL_START

#define PROPAGATE_LITERAL propagate_literal_beyond_conflicts
#define CONTINUE_PROPAGATING_AFTER_CONFLICT
#define PROPAGATION_TYPE "beyond conflict"

#include "proplit.h"

static inline void
update_beyond_propagation_statistics (kissat *solver,
                                      const unsigned *saved_propagate) {
  KISSAT_assert (saved_propagate <= solver->propagate);
  const unsigned propagated = solver->propagate - saved_propagate;

  LOG ("propagated %u literals", propagated);
  LOG ("propagation took %" PRIu64 " ticks", solver->ticks);

  ADD (ticks, solver->ticks);

  ADD (propagations, propagated);
  ADD (warming_propagations, propagated);
}

static void propagate_literals_beyond_conflicts (kissat *solver) {
  unsigned *propagate = solver->propagate;
  while (propagate != END_ARRAY (solver->trail))
    if (propagate_literal_beyond_conflicts (solver, *propagate++))
      INC (warming_conflicts);
  solver->propagate = propagate;
}

void kissat_propagate_beyond_conflicts (kissat *solver) {
  KISSAT_assert (!solver->probing);
  KISSAT_assert (solver->watching);
  KISSAT_assert (solver->warming);
  KISSAT_assert (!solver->inconsistent);

  START (propagate);

  solver->ticks = 0;
  const unsigned *saved_propagate = solver->propagate;
  propagate_literals_beyond_conflicts (solver);
  update_beyond_propagation_statistics (solver, saved_propagate);

  STOP (propagate);
}

ABC_NAMESPACE_IMPL_END
