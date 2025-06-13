#ifndef _restart_h_INCLUDED
#define _restart_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

#include <stdbool.h>

struct kissat;

bool kissat_restarting (struct kissat *);
void kissat_restart (struct kissat *);

void kissat_update_focused_restart_limit (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
