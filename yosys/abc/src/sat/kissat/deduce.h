#ifndef _deduce_h_INCLUDED
#define _deduce_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct clause;
struct kissat;

struct clause *kissat_deduce_first_uip_clause (struct kissat *,
                                               struct clause *);

bool kissat_recompute_and_promote (struct kissat *, struct clause *);

ABC_NAMESPACE_HEADER_END

#endif
