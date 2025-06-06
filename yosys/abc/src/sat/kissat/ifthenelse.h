#ifndef _ifthenelse_h_INCLUDED
#define _ifthenelse_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_find_if_then_else_gate (struct kissat *, unsigned lit,
                                    unsigned negative);

ABC_NAMESPACE_HEADER_END

#endif
