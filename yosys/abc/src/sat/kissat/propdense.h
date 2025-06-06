#ifndef _propdense_h_INCLUDED
#define _propdense_h_INCLUDED

#include <limits.h>
#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_dense_propagate (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
