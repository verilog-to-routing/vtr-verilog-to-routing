#include "proprobe.h"
#include "fastassign.h"
#include "trail.h"

ABC_NAMESPACE_IMPL_START

#define PROPAGATE_LITERAL probing_propagate_literal
#define PROPAGATION_TYPE "probing"
#define PROBING_PROPAGATION

#include "proplit.h"

static void update_probing_propagation_statistics (kissat *solver,
                                                   unsigned propagated) {
  const uint64_t ticks = solver->ticks;
  LOG (PROPAGATION_TYPE " propagation took %u propagations", propagated);
  LOG (PROPAGATION_TYPE " propagation took %" PRIu64 " ticks", ticks);

  ADD (propagations, propagated);
  ADD (probing_propagations, propagated);

#if defined(METRICS)
  if (solver->backbone_computing) {
    ADD (backbone_propagations, propagated);
    ADD (backbone_ticks, ticks);
  }
  if (solver->vivifying) {
    ADD (vivify_propagations, propagated);
    ADD (vivify_ticks, ticks);
  }
#endif

  ADD (probing_ticks, ticks);
  ADD (ticks, ticks);
}

clause *kissat_probing_propagate (kissat *solver, clause *ignore,
                                  bool flush) {
  KISSAT_assert (solver->probing);
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);

  START (propagate);

  clause *conflict = 0;
  unsigned *propagate = solver->propagate;
  solver->ticks = 0;
  while (!conflict && propagate != END_ARRAY (solver->trail)) {
    const unsigned lit = *propagate++;
    conflict = probing_propagate_literal (solver, ignore, lit);
  }

  const unsigned propagated = propagate - solver->propagate;
  solver->propagate = propagate;
  update_probing_propagation_statistics (solver, propagated);
  kissat_update_conflicts_and_trail (solver, conflict, flush);

  STOP (propagate);

  return conflict;
}

ABC_NAMESPACE_IMPL_END
