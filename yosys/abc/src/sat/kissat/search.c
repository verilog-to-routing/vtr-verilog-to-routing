#include "search.h"
#include "analyze.h"
#include "bump.h"
#include "classify.h"
#include "decide.h"
#include "eliminate.h"
#include "inline.h"
#include "internal.h"
#include "logging.h"
#include "lucky.h"
#include "preprocess.h"
#include "print.h"
#include "probe.h"
#include "propsearch.h"
#include "reduce.h"
#include "reluctant.h"
#include "reorder.h"
#include "rephase.h"
#include "report.h"
#include "restart.h"
#include "terminate.h"
#include "trail.h"
#include "walk.h"

#include <inttypes.h>

ABC_NAMESPACE_IMPL_START

static void init_tiers (kissat *solver) {
  for (unsigned stable = 0; stable != 2; stable++) {
    if (!solver->tier1[stable]) {
      KISSAT_assert (!solver->tier2[stable]);
      solver->tier1[stable] = GET_OPTION (tier1);
      solver->tier2[stable] = GET_OPTION (tier2);
      if (solver->tier2[stable] <= solver->tier1[stable])
        solver->tier2[stable] = solver->tier1[stable];
      LOG ("initialized tier1[%u] glue to %u", stable,
           solver->tier1[stable]);
      LOG ("initialized tier2[%u] glue to %u", stable,
           solver->tier2[stable]);
      if (!solver->limits.glue.interval)
        solver->limits.glue.interval = 2;
    }
  }
}

static void start_search (kissat *solver) {
  START (search);
  INC (searches);

  bool stable = (GET_OPTION (stable) == 2);

  solver->stable = stable;
  kissat_phase (solver, "search", GET (searches),
                "initializing %s search after %" PRIu64 " conflicts",
                (stable ? "stable" : "focus"), CONFLICTS);

  kissat_init_averages (solver, &AVERAGES);

  kissat_classify (solver);

  if (solver->stable) {
    kissat_init_reluctant (solver);
    kissat_update_scores (solver);
  }

  init_tiers (solver);

  kissat_init_limits (solver);

  unsigned seed = GET_OPTION (seed);
  solver->random = seed;
  LOG ("initialized random number generator with seed %u", seed);

#ifndef KISSAT_QUIET
  limits *limits = &solver->limits;
  limited *limited = &solver->limited;
  if (!limited->conflicts && !limited->decisions)
    kissat_very_verbose (solver, "starting unlimited search");
  else if (limited->conflicts && !limited->decisions)
    kissat_very_verbose (
        solver, "starting search with conflicts limited to %" PRIu64,
        limits->conflicts);
  else if (!limited->conflicts && limited->decisions)
    kissat_very_verbose (
        solver, "starting search with decisions limited to %" PRIu64,
        limits->decisions);
  else
    kissat_very_verbose (
        solver,
        "starting search with decisions limited to %" PRIu64
        " and conflicts limited to %" PRIu64,
        limits->decisions, limits->conflicts);
  if (stable) {
    START (stable);
    REPORT (0, '[');
  } else {
    START (focused);
    REPORT (0, '{');
  }
#endif
}

static void stop_search (kissat *solver) {
  if (solver->limited.conflicts) {
    LOG ("reset conflict limit");
    solver->limited.conflicts = false;
  }

  if (solver->limited.decisions) {
    LOG ("reset decision limit");
    solver->limited.decisions = false;
  }

  if (solver->termination.flagged) {
    kissat_very_verbose (solver, "termination forced externally");
    solver->termination.flagged = 0;
  }

  if (solver->stable) {
    REPORT (0, ']');
    STOP (stable);
    solver->stable = false;
  } else {
    REPORT (0, '}');
    STOP (focused);
  }
  STOP (search);
}

static void report_search_result (kissat *solver, int res) {
#ifndef KISSAT_QUIET
  LOG ("search result %d", res);
  char type = (res == 10 ? '1' : res == 20 ? '0' : '?');
  REPORT (0, type);
#else
  (void) solver, (void) res;
#endif
}

static void iterate (kissat *solver) {
  KISSAT_assert (solver->iterating);
  solver->iterating = false;
  REPORT (0, 'i');
}

static bool conflict_limit_hit (kissat *solver) {
  if (!solver->limited.conflicts)
    return false;
  if (solver->limits.conflicts > solver->statistics_.conflicts)
    return false;
  kissat_very_verbose (
      solver, "conflict limit %" PRIu64 " hit after %" PRIu64 " conflicts",
      solver->limits.conflicts, solver->statistics_.conflicts);
  return true;
}

static bool decision_limit_hit (kissat *solver) {
  if (!solver->limited.decisions)
    return false;
  if (solver->limits.decisions > solver->statistics_.decisions)
    return false;
  kissat_very_verbose (
      solver, "decision limit %" PRIu64 " hit after %" PRIu64 " decisions",
      solver->limits.decisions, solver->statistics_.decisions);
  return true;
}

static bool searching (kissat *solver) {
  if (!kissat_propagated (solver))
    return true;
  if (!GET_OPTION (probeinit))
    return true;
  if (!GET_OPTION (eliminateinit))
    return true;
  if (conflict_limit_hit (solver))
    return false;
  return true;
}

int kissat_search (kissat *solver) {
  REPORT (0, '*');
  int res = 0;
  if (solver->inconsistent)
    res = 20;
  if (!res && GET_OPTION (luckyearly))
    res = kissat_lucky (solver);
  if (!res && kissat_preprocessing (solver))
    res = kissat_preprocess (solver);
  if (!res && GET_OPTION (luckylate))
    res = kissat_lucky (solver);
  if (!res)
    kissat_classify (solver);
  if (!res && searching (solver)) {
    start_search (solver);
    while (!res) {
      clause *conflict = kissat_search_propagate (solver);
      if (conflict)
        res = kissat_analyze (solver, conflict);
      else if (solver->iterating)
        iterate (solver);
      else if (!solver->unassigned)
        res = 10;
      else if (TERMINATED (search_terminated_1))
        break;
      else if (kissat_reducing (solver))
        res = kissat_reduce (solver);
      else if (kissat_switching_search_mode (solver))
        kissat_switch_search_mode (solver);
      else if (kissat_restarting (solver))
        kissat_restart (solver);
      else if (kissat_reordering (solver))
        kissat_reorder (solver);
      else if (kissat_rephasing (solver))
        kissat_rephase (solver);
      else if (kissat_probing (solver))
        res = kissat_probe (solver);
      else if (kissat_eliminating (solver))
        res = kissat_eliminate (solver);
      else if (conflict_limit_hit (solver))
        break;
      else if (decision_limit_hit (solver))
        break;
      else
        kissat_decide (solver);
    }
    stop_search (solver);
  }
  report_search_result (solver, res);
  return res;
}

ABC_NAMESPACE_IMPL_END
