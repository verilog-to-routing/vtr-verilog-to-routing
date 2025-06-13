#ifndef _reorder_h_INCLUDED
#define _reorder_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_reordering (struct kissat *);
void kissat_reorder (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
