#include "internal.h"
#include "logging.h"
#include "mode.h"
#include "print.h"
#include "reduce.h"
#include "rephase.h"
#include "resources.h"
#include "restart.h"

#include <inttypes.h>
#include <math.h>

ABC_NAMESPACE_IMPL_START

double kissat_logn (uint64_t count) {
  KISSAT_assert (count > 0);
  const double res = log10 (count + 9);
  KISSAT_assert (res >= 1);
  return res;
}

double kissat_sqrt (uint64_t count) {
  KISSAT_assert (count > 0);
  const double res = sqrt (count);
  KISSAT_assert (res >= 1);
  return res;
}

double kissat_nlogpown (uint64_t count, unsigned exponent) {
  KISSAT_assert (count > 0);
  const double tmp = log10 (count + 9);
  double factor = 1;
  while (exponent--)
    factor *= tmp;
  KISSAT_assert (factor >= 1);
  const double res = count * factor;
  KISSAT_assert (res >= 1);
  return res;
}

uint64_t kissat_scale_delta (kissat *solver, const char *pretty,
                             uint64_t delta) {
  const uint64_t C = BINIRR_CLAUSES;
  double f = kissat_logn (C + 1) - 5;
  const double ff = f * f;
  KISSAT_assert (ff >= 0);
  const double fff = 4.5 * ff + 25;
  uint64_t scaled = fff * delta;
  KISSAT_assert (delta <= scaled);
  // clang-format off
  kissat_very_verbose (solver,
    "scaled %s delta %" PRIu64
    " = %g * %" PRIu64
    " = (4.5 (log10(%" PRIu64 ") - 5)^2 + 25) * %" PRIu64,
    pretty, scaled, fff, delta, C, delta);
  // clang-format on
  (void) pretty;
  return scaled;
}

static void init_enabled (kissat *solver) {
  bool probe;
  if (!GET_OPTION (simplify))
    probe = false;
  else if (!GET_OPTION (probe))
    probe = false;
  else if (GET_OPTION (substitute))
    probe = true;
  else if (GET_OPTION (sweep))
    probe = true;
  else if (GET_OPTION (vivify))
    probe = true;
  else
    probe = false;
  kissat_very_verbose (solver, "probing %sabled", probe ? "en" : "dis");
  solver->enabled.probe = probe;

  bool eliminate;
  if (!GET_OPTION (simplify))
    eliminate = false;
  else if (!GET_OPTION (eliminate))
    eliminate = false;
  else
    eliminate = true;
  kissat_very_verbose (solver, "eliminate %sabled",
                       eliminate ? "en" : "dis");
  solver->enabled.eliminate = eliminate;
}

#define INIT_CONFLICT_LIMIT(NAME, SCALE) \
  do { \
    const uint64_t DELTA = GET_OPTION (NAME##init); \
    const uint64_t SCALED = \
        !(SCALE) ? DELTA : kissat_scale_delta (solver, #NAME, DELTA); \
    limits->NAME.conflicts = CONFLICTS + SCALED; \
    kissat_very_verbose (solver, \
                         "initial " #NAME " limit of %s conflicts", \
                         FORMAT_COUNT (limits->NAME.conflicts)); \
  } while (0)

void kissat_init_limits (kissat *solver) {
  KISSAT_assert (solver->statistics_.searches == 1);

  init_enabled (solver);

  limits *limits = &solver->limits;

  if (GET_OPTION (randec))
    INIT_CONFLICT_LIMIT (randec, false);

  if (GET_OPTION (reduce))
    INIT_CONFLICT_LIMIT (reduce, false);

  if (GET_OPTION (reorder))
    INIT_CONFLICT_LIMIT (reorder, false);

  if (GET_OPTION (rephase))
    INIT_CONFLICT_LIMIT (rephase, false);

  if (!solver->stable)
    kissat_update_focused_restart_limit (solver);

  kissat_init_mode_limit (solver);

  if (solver->enabled.eliminate) {
    INIT_CONFLICT_LIMIT (eliminate, true);
    solver->bounds.eliminate.max_bound_completed = 0;
    solver->bounds.eliminate.additional_clauses = 0;
    kissat_very_verbose (solver, "reset elimination bound to zero");
  }

  if (solver->enabled.probe)
    INIT_CONFLICT_LIMIT (probe, true);
}

#ifndef KISSAT_QUIET

static const char *delay_description (kissat *solver, delay *delay) {
  delays *delays = &solver->delays;
  if (delay == &delays->bumpreasons)
    return "bumping reason side literals";
  else if (delay == &delays->congruence)
    return "congruence closure";
  else if (delay == &delays->sweep)
    return "sweeping";
  else {
    KISSAT_assert (delay == &delays->vivifyirr);
    return "vivifying irredundant clauses";
  }
}

#endif

#define VERY_VERBOSE_IF_NOT_BUMPREASONS(...) \
  VERY_VERBOSE_OR_LOG (delay == &solver->delays.bumpreasons, __VA_ARGS__)

void kissat_reduce_delay (kissat *solver, delay *delay) {
  if (!delay->current)
    return;
  delay->current /= 2;
  VERY_VERBOSE_IF_NOT_BUMPREASONS (
      solver, "%s delay interval decreased to %u",
      delay_description (solver, delay), delay->current);
  delay->count = delay->current;
#ifdef KISSAT_QUIET
  (void) solver;
#endif
}

void kissat_bump_delay (kissat *solver, delay *delay) {
  delay->current += delay->current < UINT_MAX;
  VERY_VERBOSE_IF_NOT_BUMPREASONS (
      solver, "%s delay interval increased to %u",
      delay_description (solver, delay), delay->current);
  delay->count = delay->current;
#ifdef KISSAT_QUIET
  (void) solver;
#endif
}

bool kissat_delaying (kissat *solver, delay *delay) {
  if (delay->count) {
    delay->count--;
    VERY_VERBOSE_IF_NOT_BUMPREASONS (
        solver, "%s still delayed (%u more times)",
        delay_description (solver, delay), delay->current);
    return true;
  } else {
    VERY_VERBOSE_IF_NOT_BUMPREASONS (solver, "%s not delayed",
                                     delay_description (solver, delay));
    return false;
  }
#ifdef KISSAT_QUIET
  (void) solver;
#endif
}

ABC_NAMESPACE_IMPL_END
