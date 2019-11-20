#ifndef __ACE_BDD_H__
#define __ACE_BDD_H__

#include "ace.h"
#include "cube.h"

#include "cudd.h"
#include "st.h"

double calc_cube_switch_prob_recur(DdManager * mgr, DdNode * bdd,
		ace_cube_t * cube, Vec_Ptr_t * inputs, st_table * visited, int phase);

void ace_bdd_get_literals(Abc_Ntk_t * ntk, st_table ** lit_st_table,
		Vec_Ptr_t ** literals);

int ace_bdd_build_network_bdds(Abc_Ntk_t * ntk, st_table * leaves,
		Vec_Ptr_t * inputs, int max_size, double min_Prob);

double ace_bdd_calc_switch_act(DdManager * mgr, Abc_Obj_t * obj,
		Vec_Ptr_t * fanins);

int node_error(int code);

#endif
