#ifndef _forward_h_INCLUDED
#define _forward_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_forward_subsume_during_elimination (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
