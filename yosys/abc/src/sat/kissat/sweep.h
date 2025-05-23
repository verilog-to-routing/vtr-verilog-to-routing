#ifndef _sweep_h_INCLUDED
#define _sweep_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
bool kissat_sweep (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
