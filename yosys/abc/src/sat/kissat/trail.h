#ifndef _trail_h_INLCUDED
#define _trail_h_INLCUDED

#include "reference.h"

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_flush_trail (struct kissat *);
bool kissat_flush_and_mark_reason_clauses (struct kissat *,
                                           reference start);
void kissat_unmark_reason_clauses (struct kissat *, reference start);
void kissat_mark_reason_clauses (struct kissat *, reference start);

ABC_NAMESPACE_HEADER_END

#endif
