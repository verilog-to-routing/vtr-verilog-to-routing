#ifndef _propsearch_h_INCLUDED
#define _propsearch_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
struct clause;

struct clause *kissat_search_propagate (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
