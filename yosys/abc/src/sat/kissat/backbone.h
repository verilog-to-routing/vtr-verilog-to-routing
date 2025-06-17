#ifndef _backbone_h_INCLUDED
#define _backbone_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
void kissat_binary_clauses_backbone (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
