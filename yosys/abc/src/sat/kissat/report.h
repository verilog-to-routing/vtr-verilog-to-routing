#ifndef _report_h_INCLUDED
#define _report_h_INCLUDED

#include "global.h"

#ifdef KISSAT_QUIET

ABC_NAMESPACE_HEADER_START

#define REPORT(...) \
  do { \
  } while (0)

#else

#include <stdbool.h>

ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_report (struct kissat *, bool verbose, char type);

#define REPORT(LEVEL, TYPE) kissat_report (solver, (LEVEL), (TYPE))

#endif

ABC_NAMESPACE_HEADER_END

#endif
