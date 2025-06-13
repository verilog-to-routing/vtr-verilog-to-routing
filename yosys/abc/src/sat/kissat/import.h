#ifndef _import_h_INLCUDED
#define _import_h_INLCUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

unsigned kissat_import_literal (struct kissat *solver, int lit);
unsigned kissat_fresh_literal (struct kissat *solver);

ABC_NAMESPACE_HEADER_END

#endif
