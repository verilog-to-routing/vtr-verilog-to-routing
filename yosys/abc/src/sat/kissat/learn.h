#ifndef _learn_h_INCLUDED
#define _learn_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_learn_clause (struct kissat *);
void kissat_update_learned (struct kissat *, unsigned glue, unsigned size);
unsigned kissat_determine_new_level (struct kissat *, unsigned jump);

ABC_NAMESPACE_HEADER_END

#endif
