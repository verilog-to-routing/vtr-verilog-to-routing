#ifndef _reduce_h_INCLUDED
#define _reduce_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_reducing (struct kissat *);
int kissat_reduce (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
