#ifndef _averages_h_INCLUDED
#define _averages_h_INCLUDED

#include "smooth.h"

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct averages averages;

struct averages {
  bool initialized;
  smooth fast_glue, slow_glue;
#ifndef KISSAT_QUIET
  smooth level, size, trail;
#endif
  smooth decision_rate;
  uint64_t saved_decisions;
};

struct kissat;

void kissat_init_averages (struct kissat *, averages *);

#define AVERAGES (solver->averages[solver->stable])

#define EMA(NAME) (AVERAGES.NAME)

#define AVERAGE(NAME) (EMA (NAME).value)

#define UPDATE_AVERAGE(NAME, VALUE) \
  kissat_update_smooth (solver, &EMA (NAME), VALUE)

ABC_NAMESPACE_HEADER_END

#endif
