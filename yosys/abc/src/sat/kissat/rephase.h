#ifndef _rephase_h_INCLUDED
#define _rephase_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_rephasing (struct kissat *);
void kissat_rephase (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
