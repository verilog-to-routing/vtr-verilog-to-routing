#ifndef _smooth_h_INCLUDED
#define _smooth_h_INCLUDED

#include <stdint.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct smooth smooth;

struct smooth {
  double value, biased, alpha, beta, exp;
#ifdef LOGGING
  const char *name;
  uint64_t updated;
#endif
};

struct kissat;

void kissat_init_smooth (struct kissat *, smooth *, int window,
                         const char *);
void kissat_update_smooth (struct kissat *, smooth *, double);

ABC_NAMESPACE_HEADER_END

#endif
