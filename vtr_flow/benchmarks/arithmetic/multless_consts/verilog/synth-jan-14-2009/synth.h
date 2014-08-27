#ifndef SYNTH_H
#define SYNTH_H

#include "chains.h"

void init_synth(int argc, char **argv);
void create_problem(cfset_t &coeffs, reg_t input_reg = 1);
void create_random_problem(int size);
void set_bits(int b);
void solve(oplist_t *l, regmap_t *regs, reg_t (*tmpreg)());

void dump_state(ostream &os);
void state();

extern bool SLOW_C1;
extern bool SLOW_C2;
extern bool USE_TABLE;
extern bool GEN_UNIQUE;
extern bool GEN_ADDERS;
extern bool GEN_DAG;
extern bool PRINT_TARGETS;
extern bool USE_AC1;
extern bool IMPROVE_AC1;
extern bool MAX_BENEFIT;
extern int VERBOSE;

extern cfvec_t SIZES;
extern cfvec_t SPEC_ALL_TARGETS; /* all specified targets (including duplicates) */
extern cfset_t SPEC_TARGETS;     /* duplicates/evens/negatives are omitted here */
extern cfset_t TARGETS;          /* not yet synthesized coefficients */
extern full_addmap_t READY; /* synthesized coefficients */

extern adag * ADAG;

#endif
