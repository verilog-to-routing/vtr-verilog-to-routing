#ifndef _equivs_h_INCLUDED
#define _equivs_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_find_equivalence_gate (struct kissat *, unsigned lit);

ABC_NAMESPACE_HEADER_END

#endif
