#ifndef _walk_h_INCLUDED
#define _walk_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_walking (struct kissat *);
void kissat_walk (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
