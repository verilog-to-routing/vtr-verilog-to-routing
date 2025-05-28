#ifndef _proprobe_h_INCLUDED
#define _proprobe_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
struct clause;

struct clause *kissat_probing_propagate (struct kissat *, struct clause *,
                                         bool flush);

ABC_NAMESPACE_HEADER_END

#endif
