#ifndef _strengthen_h_INCLUDED
#define _strengthen_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct clause;
struct kissat;

struct clause *kissat_on_the_fly_strengthen (struct kissat *,
                                             struct clause *, unsigned lit);

void kissat_on_the_fly_subsume (struct kissat *, struct clause *,
                                struct clause *);

bool issat_strengthen_clause (struct kissat *, struct clause *, unsigned);

ABC_NAMESPACE_HEADER_END

#endif
