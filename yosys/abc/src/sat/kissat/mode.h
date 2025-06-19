#ifndef _mode_h_INCLUDED
#define _mode_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

typedef struct mode mode;

struct mode {
  uint64_t ticks;
#ifndef KISSAT_QUIET
  double entered;
  uint64_t conflicts;
#ifdef METRICS
  uint64_t propagations;
  uint64_t visits;
#endif
#endif
};

void kissat_init_mode_limit (struct kissat *);
bool kissat_switching_search_mode (struct kissat *);
void kissat_switch_search_mode (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
