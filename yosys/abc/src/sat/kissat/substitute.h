#ifndef _substitute_h_INCLUDED
#define _substitute_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_substitute (struct kissat *, bool complete);

ABC_NAMESPACE_HEADER_END

#endif
