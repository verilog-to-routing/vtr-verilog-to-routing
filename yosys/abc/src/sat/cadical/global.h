#ifndef ABC_SAT_CADICAL_GLOBAL_HPP_
#define ABC_SAT_CADICAL_GLOBAL_HPP_

// comment out next line to enable cadical debug mode
#define CADICAL_NDEBUG

#define CADICAL_NBUILD
#define CADICAL_QUIET
#define CADICAL_NCONTRACTS
#define CADICAL_NTRACING
#define CADICAL_NCLOSEFROM

#ifdef CADICAL_NDEBUG
#define CADICAL_assert(ignore) ((void)0)
#else
#define CADICAL_assert(cond) assert(cond)
#endif

#include "misc/util/abc_global.h"

#endif
