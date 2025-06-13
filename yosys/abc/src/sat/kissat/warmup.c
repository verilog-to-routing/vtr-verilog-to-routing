#include "warmup.h"
#include "backtrack.h"
#include "decide.h"
#include "internal.h"
#include "print.h"
#include "propbeyond.h"
#include "terminate.h"

ABC_NAMESPACE_IMPL_START

void kissat_warmup (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (GET_OPTION (warmup));
  START (warmup);
  KISSAT_assert (!solver->warming);
  solver->warming = true;
  INC (warmups);
#ifndef KISSAT_QUIET
  const statistics *stats = &solver->statistics;
  uint64_t propagations = stats->warming_propagations;
  uint64_t decisions = stats->warming_decisions;
#endif
  while (solver->unassigned) {
    if (TERMINATED (warmup_terminated_1))
      break;
    kissat_decide (solver);
    kissat_propagate_beyond_conflicts (solver);
  }
  KISSAT_assert (!solver->inconsistent);
#ifndef KISSAT_QUIET
  decisions = stats->warming_decisions - decisions;
  propagations = stats->warming_propagations - propagations;

  kissat_very_verbose (solver,
                       "warming-up needed %" PRIu64
                       " decisions and %" PRIu64 " propagations",
                       decisions, propagations);

  if (solver->unassigned)
    kissat_verbose (solver,
                    "reached decision level %u "
                    "during warming-up saved phases",
                    solver->level);
  else
    kissat_verbose (solver,
                    "all variables assigned at decision level %u "
                    "during warming-up saved phases",
                    solver->level);
#endif
  kissat_backtrack_without_updating_phases (solver, 0);
  KISSAT_assert (solver->warming);
  solver->warming = false;
  STOP (warmup);
}

ABC_NAMESPACE_IMPL_END
