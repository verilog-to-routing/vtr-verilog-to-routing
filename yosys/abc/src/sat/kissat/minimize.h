#ifndef _minimize_h_INCLUDED
#define _minimize_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_reset_poisoned (struct kissat *);

void kissat_minimize_clause (struct kissat *);
bool kissat_minimize_literal (struct kissat *, unsigned,
                              bool lit_in_clause);

ABC_NAMESPACE_HEADER_END

#endif
