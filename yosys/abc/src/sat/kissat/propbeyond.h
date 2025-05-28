#ifndef _propall_h_INCLUDED
#define _propall_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
struct clause;

void kissat_propagate_beyond_conflicts (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
