#ifndef _ands_h_INCLUDED
#define _ands_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_find_and_gate (struct kissat *, unsigned lit,
                           unsigned negative);

ABC_NAMESPACE_HEADER_END

#endif
