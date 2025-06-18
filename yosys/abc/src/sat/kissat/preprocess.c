#include "preprocess.h"
#include "collect.h"
#include "fastel.h"
#include "internal.h"
#include "print.h"
#include "probe.h"
#include "propinitially.h"
#include "report.h"
#include "sweep.h"
#include "terminate.h"

#include <inttypes.h>

ABC_NAMESPACE_IMPL_START

bool kissat_preprocessing (struct kissat *solver) {
  KISSAT_assert (!solver->level);
  KISSAT_assert (!solver->inconsistent);
  if (!GET_OPTION (preprocess))
    return false;
  if (!GET_OPTION (probe))
    return false;
  if (!GET_OPTION (preprocessprobe))
    return false;
#if defined(KISSAT_NDEBUG) && defined(KISSAT_NOPTIONS)
  (void) solver;
#endif
  return true;
}

int kissat_preprocess (struct kissat *solver) {
  KISSAT_assert (kissat_preprocessing (solver));
  if (!kissat_initially_propagate (solver)) {
    KISSAT_assert (solver->inconsistent);
    return 20;
  }
  START (preprocess);
  KISSAT_assert (!solver->preprocessing);
  solver->preprocessing = true;
  REPORT (0, '(');
  const unsigned max_rounds = GET_OPTION (preprocessrounds);
#ifndef KISSAT_QUIET
  const unsigned variables_initially = solver->active;
  const uint64_t clauses_initially = BINIRR_CLAUSES;
  const unsigned variables_originally = SIZE_STACK (solver->import);
  const uint64_t clauses_originally = solver->statistics.clauses_original;
  kissat_verbose (solver,
                  "[preprocess] running at most %u preprocesing rounds",
                  max_rounds);
  kissat_verbose (
      solver,
      "[preprocess] initially %u variables %.0f%% and %" PRIu64
      " clauses %.0f%%",
      variables_initially,
      kissat_percent (variables_initially, variables_originally),
      clauses_initially,
      kissat_percent (clauses_initially, clauses_originally));
#endif
  kissat_initial_sparse_collect (solver);
  unsigned round = 1;
  for (;;) {
    if (solver->inconsistent)
      break;
    if (TERMINATED (preprocess_terminated_1))
      break;
    const unsigned variables_before = solver->active;
    const uint64_t clauses_before = BINIRR_CLAUSES;
#ifndef KISSAT_QUIET
    kissat_verbose (solver,
                    "[preprocess-%u] started preprocessing round %u", round,
                    round);
#endif
    if (GET_OPTION (preprocessprobe))
      kissat_probe_initially (solver);
    if (GET_OPTION (fastel))
      kissat_fast_variable_elimination (solver);
    const unsigned variables_after = solver->active;
    const uint64_t clauses_after = BINIRR_CLAUSES;
#ifndef KISSAT_QUIET
    if (variables_after < variables_before) {
      unsigned removed = variables_before - variables_after;
      kissat_verbose (
          solver, "[preprocess-%u] removed %u variables %.0f%% in round %u",
          round, removed, kissat_percent (removed, variables_before),
          round);
    } else if (variables_after > variables_before) {
      unsigned added = variables_after - variables_before;
      kissat_verbose (
          solver, "[preprocess-%u] added %u variables %.0f%% in round %u",
          round, added, kissat_percent (added, variables_before), round);
    } else
      kissat_verbose (
          solver,
          "[preprocess-%u] number variables %u unchanged in round %u",
          round, variables_before, round);
    if (clauses_after < clauses_before) {
      uint64_t removed = clauses_before - clauses_after;
      kissat_verbose (solver,
                      "[preprocess-%u] removed %" PRIu64
                      " irredundant and binary clauses %.0f%% in round %u",
                      round, removed,
                      kissat_percent (removed, clauses_before), round);
    } else if (clauses_after > clauses_before) {
      uint64_t added = clauses_after - clauses_before;
      kissat_verbose (solver,
                      "[preprocess-%u] added %" PRIu64
                      " irredundant and binary clauses %.0f%% in round %u",
                      round, added, kissat_percent (added, clauses_before),
                      round);
    } else
      kissat_verbose (
          solver,
          "[preprocess-%u] number irredundant and binary clauses %" PRIu64
          " unchanged in round %u",
          round, clauses_before, round);
#endif
    if (round == max_rounds)
      break;
    if (variables_before == variables_after &&
        clauses_before == clauses_after)
      break;
    round++;
  }
#ifndef KISSAT_QUIET
  const unsigned variables_finally = solver->active;
  const uint64_t clauses_finally = BINIRR_CLAUSES;
  kissat_verbose (solver, "[preprocess] finished after %u rounds", round);
  if (variables_finally < variables_initially) {
    unsigned removed = variables_initially - variables_finally;
    kissat_verbose (
        solver,
        "[preprocess] removed %u variables %.0f%% (%u remain %.0f%%)",
        removed, kissat_percent (removed, variables_initially),
        variables_finally,
        kissat_percent (variables_finally, variables_originally));
  } else if (variables_finally > variables_initially) {
    unsigned added = variables_finally - variables_initially;
    kissat_verbose (
        solver, "[preprocess] added %u variables %.0f%% (%u remain %.0f%%)",
        added, kissat_percent (added, variables_initially),
        variables_finally,
        kissat_percent (variables_finally, variables_originally));
  } else
    kissat_verbose (
        solver,
        "[preprocess] number variables %u unchanged (%u remain %.0f%%)",
        variables_initially, variables_finally,
        kissat_percent (variables_finally, variables_originally));
  if (clauses_finally < clauses_initially) {
    uint64_t removed = clauses_initially - clauses_finally;
    kissat_verbose (solver,
                    "[preprocess] removed %" PRIu64
                    " irredundant and binary clauses %.0f%% (%" PRIu64
                    " remain %.0f%%)",
                    removed, kissat_percent (removed, clauses_initially),
                    clauses_finally,
                    kissat_percent (clauses_finally, clauses_originally));
  } else if (clauses_finally > clauses_initially) {
    uint64_t added = clauses_finally - clauses_initially;
    kissat_verbose (solver,
                    "[preprocess] added %" PRIu64
                    " irredundant and binary clauses %.0f%% (%" PRIu64
                    " remain %.0f%%)",
                    added, kissat_percent (added, clauses_initially),
                    clauses_finally,
                    kissat_percent (clauses_finally, clauses_originally));
  } else
    kissat_verbose (
        solver,
        "[preprocess] number irredundant and binary clauses %" PRIu64
        " unchanged (%" PRIu64 " remain %.0f%%)",
        clauses_initially, clauses_finally,
        kissat_percent (clauses_finally, clauses_originally));
#endif
  REPORT (0, ')');
  KISSAT_assert (solver->preprocessing);
  solver->preprocessing = false;
  STOP (preprocess);
  return solver->inconsistent ? 20 : 0;
}

ABC_NAMESPACE_IMPL_END
