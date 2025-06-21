#ifndef _limits_h_INCLUDED
#define _limits_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct bounds bounds;
typedef struct changes changes;
typedef struct delays delays;
typedef struct delay delay;
typedef struct remember remember;
typedef struct enabled enabled;
typedef struct limited limited;
typedef struct limits limits;

struct bounds {
  struct {
    uint64_t max_bound_completed;
    unsigned additional_clauses;
  } eliminate;
};

struct limits {
  uint64_t conflicts;
  uint64_t decisions;
  uint64_t reports;

  struct {
    uint64_t count;
    uint64_t ticks;
    uint64_t conflicts;
  } mode;

  struct {
    struct {
      uint64_t eliminate;
      uint64_t subsume;
    } variables;
    uint64_t conflicts;
  } eliminate;

  struct {
    uint64_t marked;
  } factor;

  struct {
    uint64_t conflicts;
  } probe, randec, reduce, reorder, rephase, restart;

  struct {
    uint64_t conflicts;
    uint64_t interval;
  } glue;
};

struct limited {
  bool conflicts;
  bool decisions;
};

struct enabled {
  bool eliminate;
  bool focus;
  bool mode;
  bool probe;
};

struct delay {
  unsigned count;
  unsigned current;
};

struct delays {
  delay bumpreasons;
  delay congruence;
  delay sweep;
  delay vivifyirr;
};

struct remember {
  struct {
    uint64_t eliminate;
    uint64_t probe;
  } ticks;
  struct {
    uint64_t reduce;
  } conflicts;
};

struct kissat;

changes kissat_changes (struct kissat *);

bool kissat_changed (changes before, changes after);

void kissat_init_limits (struct kissat *);

uint64_t kissat_scale_delta (struct kissat *, const char *, uint64_t);

double kissat_nlogpown (uint64_t, unsigned);
double kissat_sqrt (uint64_t);
double kissat_logn (uint64_t);

#define LOGN(COUNT) kissat_logn (COUNT)
#define LINEAR(COUNT) (COUNT)
#define NLOGN(COUNT) kissat_nlogpown (COUNT, 1)
#define NLOG2N(COUNT) kissat_nlogpown (COUNT, 2)
#define NLOG3N(COUNT) kissat_nlogpown (COUNT, 3)

#define SQRT(COUNT) kissat_sqrt (COUNT)

#define UPDATE_CONFLICT_LIMIT(NAME, COUNT, SCALE_COUNT_FUNCTION, \
                              SCALE_DELTA) \
  do { \
    if (solver->inconsistent) \
      break; \
    const struct statistics *statistics = &solver->statistics_; \
    KISSAT_assert (statistics->COUNT > 0); \
    struct limits *limits = &solver->limits; \
    uint64_t DELTA = GET_OPTION (NAME##int); \
    const double SCALING = SCALE_COUNT_FUNCTION (statistics->COUNT); \
    KISSAT_assert (SCALING >= 1); \
    DELTA *= SCALING; \
    const uint64_t SCALED = \
        !(SCALE_DELTA) ? DELTA \
                       : kissat_scale_delta (solver, #NAME, DELTA); \
    limits->NAME.conflicts = CONFLICTS + SCALED; \
    kissat_phase ( \
        solver, #NAME, GET (COUNT), "new limit of %s after %s conflicts", \
        FORMAT_COUNT (limits->NAME.conflicts), FORMAT_COUNT (SCALED)); \
  } while (0)

#include <inttypes.h>

#define SET_EFFORT_LIMIT(LIMIT, NAME, START) \
  uint64_t LIMIT; \
  do { \
    const uint64_t OLD_LIMIT = solver->statistics_.START; \
    const uint64_t TICKS = solver->statistics_.search_ticks; \
    const uint64_t LAST = solver->probing ? solver->last.ticks.probe \
                                          : solver->last.ticks.eliminate; \
    uint64_t REFERENCE = TICKS - LAST; \
    const uint64_t MINEFFORT = 1e6 * GET_OPTION (mineffort); \
    if (REFERENCE < MINEFFORT) { \
      REFERENCE = MINEFFORT; \
      kissat_extremely_verbose ( \
          solver, #NAME " effort reference %s set to 'mineffort'", \
          FORMAT_COUNT (REFERENCE)); \
    } else { \
      kissat_extremely_verbose ( \
          solver, #NAME " effort reference %s = %s - %s 'search_ticks'", \
          FORMAT_COUNT (REFERENCE), FORMAT_COUNT (TICKS), \
          FORMAT_COUNT (LAST)); \
    } \
    const double EFFORT = (double) GET_OPTION (NAME##effort) * 1e-3; \
    const uint64_t DELTA = EFFORT * REFERENCE; \
\
    kissat_extremely_verbose ( \
        solver, #NAME " effort delta %s = %g * %s '" #START "'", \
        FORMAT_COUNT (DELTA), EFFORT, FORMAT_COUNT (REFERENCE)); \
\
    const uint64_t NEW_LIMIT = OLD_LIMIT + DELTA; \
    kissat_very_verbose (solver, \
                         #NAME " effort limit %s = %s + %s '" #START "'", \
                         FORMAT_COUNT (NEW_LIMIT), \
                         FORMAT_COUNT (OLD_LIMIT), FORMAT_COUNT (DELTA)); \
\
    LIMIT = NEW_LIMIT; \
\
  } while (0)

struct kissat;

bool kissat_delaying (struct kissat *, delay *);
void kissat_bump_delay (struct kissat *, delay *);
void kissat_reduce_delay (struct kissat *, delay *);

#define DELAYING(NAME) kissat_delaying (solver, &solver->delays.NAME)

#define BUMP_DELAY(NAME) kissat_bump_delay (solver, &solver->delays.NAME)

#define REDUCE_DELAY(NAME) \
  kissat_reduce_delay (solver, &solver->delays.NAME)

ABC_NAMESPACE_HEADER_END

#endif
