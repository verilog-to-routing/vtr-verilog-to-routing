#ifndef _witness_h_INCLUDED
#define _witness_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_print_witness (struct kissat *, int max_var, bool partial);

ABC_NAMESPACE_HEADER_END

#endif
