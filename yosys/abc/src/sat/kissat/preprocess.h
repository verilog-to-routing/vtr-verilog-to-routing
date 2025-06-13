#ifndef _kissat_preprocess_h_INCLUDED
#define _kissat_preprocess_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;
bool kissat_preprocessing (struct kissat *);
int kissat_preprocess (struct kissat *);

ABC_NAMESPACE_HEADER_END

#endif
