#include "probe.h"
#include "backbone.h"
#include "backtrack.h"
#include "congruence.h"
#include "factor.h"
#include "internal.h"
#include "print.h"
#include "substitute.h"
#include "sweep.h"
#include "terminate.h"
#include "transitive.h"
#include "vivify.h"

#include <inttypes.h>

ABC_NAMESPACE_IMPL_START

bool kissat_probing (kissat *solver) {
  if (!solver->enabled.probe)
    return false;
  statistics *statistics = &solver->statistics_;
  const uint64_t conflicts = statistics->conflicts;
  if (solver->last.conflicts.reduce == conflicts)
    return false;
  return solver->limits.probe.conflicts < conflicts;
}

static void probe (kissat *solver) {
  kissat_backtrack_propagate_and_flush_trail (solver);
  KISSAT_assert (!solver->inconsistent);
  STOP_SEARCH_AND_START_SIMPLIFIER (probe);
  kissat_phase (solver, "probe", GET (probings),
                "probing limit hit after %" PRIu64 " conflicts",
                solver->limits.probe.conflicts);
  kissat_congruence (solver);
  kissat_substitute (solver, false);
  kissat_binary_clauses_backbone (solver);
  kissat_vivify (solver);
  kissat_sweep (solver);
  kissat_substitute (solver, false);
  kissat_transitive_reduction (solver);
  kissat_binary_clauses_backbone (solver);
  kissat_factor (solver);
  STOP_SIMPLIFIER_AND_RESUME_SEARCH (probe);
}

static void probe_initially (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->inconsistent);
  kissat_phase (solver, "probe", GET (probings), "initial probing");
  bool substitute_at_the_end = true;
  if (GET_OPTION (preprocesscongruence)) {
    if (kissat_congruence (solver)) {
      kissat_substitute (solver, true);
      substitute_at_the_end = false;
    }
  }
  if (GET_OPTION (preprocessbackbone))
    kissat_binary_clauses_backbone (solver);
  if (GET_OPTION (preprocessweep)) {
    if (kissat_sweep (solver)) {
      kissat_substitute (solver, true);
      substitute_at_the_end = false;
    }
  }
  if (substitute_at_the_end)
    kissat_substitute (solver, false);
  if (GET_OPTION (preprocessfactor))
    kissat_factor (solver);
}

int kissat_probe (kissat *solver) {
  KISSAT_assert (!solver->inconsistent);
  INC (probings);
  KISSAT_assert (!solver->probing);
  solver->probing = true;
  const unsigned max_rounds = GET_OPTION (proberounds);
  for (unsigned round = 0; round != max_rounds; round++) {
    unsigned before = solver->active;
    probe (solver);
    if (solver->inconsistent)
      break;
    if (before == solver->active)
      break;
  }
  kissat_classify (solver);
  UPDATE_CONFLICT_LIMIT (probe, probings, NLOGN, true);
  solver->last.ticks.probe = solver->statistics_.search_ticks;
  KISSAT_assert (solver->probing);
  solver->probing = false;
  return solver->inconsistent ? 20 : 0;
}

int kissat_probe_initially (kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->inconsistent);
  INC (probings);
  START (probe);
  KISSAT_assert (!solver->probing);
  solver->probing = true;
  probe_initially (solver);
  KISSAT_assert (solver->probing);
  solver->probing = false;
  STOP (probe);
  return solver->inconsistent ? 20 : 0;
}

ABC_NAMESPACE_IMPL_END
