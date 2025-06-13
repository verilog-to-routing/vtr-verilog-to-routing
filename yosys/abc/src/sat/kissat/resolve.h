#ifndef _resolve_h_INCLUDED
#define _resolve_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_generate_resolvents (struct kissat *, unsigned idx,
                                 unsigned *lit_ptr);

ABC_NAMESPACE_HEADER_END

#endif
