#ifndef ACM_H
#define ACM_H

#include "chains.h"

void   acm_init();

void   acm_solve(oplist_t &l,
		 const vector<coeff_t> &coeffs,
		 const vector<reg_t> &dests,
		 reg_t src,
		 reg_t (*tmpreg)());

void   acm_finish();

#endif
