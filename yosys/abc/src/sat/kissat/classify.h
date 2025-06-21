#ifndef _classify_h_INCLUDED
#define _classify_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

struct classification {
  bool small;
  bool bigbig;
};

typedef struct classification classification;

void kissat_classify (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
