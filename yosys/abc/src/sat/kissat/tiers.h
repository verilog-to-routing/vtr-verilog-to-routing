#ifndef _tiers_h_INCLUDED
#define _tiers_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_compute_and_set_tier_limits (struct kissat *);
void kissat_print_tier_usage_statistics (struct kissat *solver,
                                         bool stable);

ABC_NAMESPACE_HEADER_END

#endif
