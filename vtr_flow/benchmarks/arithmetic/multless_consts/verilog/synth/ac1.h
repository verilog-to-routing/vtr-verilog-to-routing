#ifndef AC1_H
#define AC1_H

#include "chains.h"

void   ac1_init(const char *);
chain *ac1_get_chain(coeff_t c);
void   ac1_finish();

#endif
