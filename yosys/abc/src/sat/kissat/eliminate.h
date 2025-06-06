#ifndef _eliminate_hpp_INCLUDED
#define _eliminate_hpp_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
struct clause;
struct heap;

void kissat_flush_units_while_connected (struct kissat *);

bool kissat_eliminating (struct kissat *);
int kissat_eliminate (struct kissat *);

void kissat_eliminate_binary (struct kissat *, unsigned, unsigned);
void kissat_eliminate_clause (struct kissat *, struct clause *, unsigned);
void kissat_update_variable_score (struct kissat *, unsigned idx);

ABC_NAMESPACE_HEADER_END

#endif
