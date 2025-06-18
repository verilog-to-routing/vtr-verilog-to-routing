#ifndef _probe_h_INCLUDED
#define _probe_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

bool kissat_probing (struct kissat *);
int kissat_probe (struct kissat *);
int kissat_probe_initially (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
