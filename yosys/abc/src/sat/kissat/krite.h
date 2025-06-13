#ifndef _krite_h_INCLUDED
#define _krite_h_INCLUDED

#include <stdio.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
void kissat_write_dimacs (struct kissat *, FILE *);

ABC_NAMESPACE_HEADER_END

#endif
