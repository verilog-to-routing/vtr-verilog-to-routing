#ifndef ABC_SAT_KISSAT_GLOBAL_H_
#define ABC_SAT_KISSAT_GLOBAL_H_

// comment out next line to enable kissat debug mode
#define KISSAT_NDEBUG

#define KISSAT_COMPACT
#define KISSAT_NOPTIONS
#define KISSAT_NPROOFS
#define KISSAT_QUIET

#ifdef KISSAT_NDEBUG
#define KISSAT_assert(ignore) ((void)0)
#else
#define KISSAT_assert(cond) assert(cond)
#endif

#include "misc/util/abc_global.h"

#endif
