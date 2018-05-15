#include <inttypes.h>

#include "vtr_assert.h"

#include "ace.h"
#include "misc/vec/vecPtr.h"
#include "bdd.h"
#include "cube.h"
#include "bdd/cudd/cuddInt.h"

//#include "vecPtr.h"
//#include "cudd.h"

int check_pi_status(Abc_Obj_t * obj);
void ace_bdd_count_paths(DdManager * mgr, DdNode * bdd, int * num_one_paths,
		int * num_zero_paths);
double calc_cube_switch_prob(DdManager * mgr, DdNode * bdd, ace_cube_t * cube,
		Vec_Ptr_t * inputs, int phase);
double calc_switch_prob_recur(DdManager * mgr, DdNode * bdd_next, DdNode * bdd,
		ace_cube_t * cube, Vec_Ptr_t * inputs, double P1, int phase);

void ace_bdd_get_literals(Abc_Ntk_t * ntk, st__table ** lit_st_table,
		Vec_Ptr_t ** literals) {
	Abc_Obj_t * obj;
	int i;

	*literals = Vec_PtrAlloc(0);
	*lit_st_table = st__init_table(st__ptrcmp, st__ptrhash);

	Abc_NtkForEachObj(ntk, obj, i)
	{
		if (Abc_ObjIsCi(obj)) {
			st__insert(*lit_st_table, (char*) obj,
					(char*) Vec_PtrSize( *literals));
			Vec_PtrPush(*literals, obj);
		}
	}
}

int check_pi_status(Abc_Obj_t * obj) {
	int i;
	Abc_Obj_t * fanin;

	Abc_ObjForEachFanin(obj, fanin, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
		if (info->status == ACE_UNDEF) {
			return 0;
		}
	}

	return 1;
}

void ace_bdd_count_paths(DdManager * mgr, DdNode * bdd, int * num_one_paths,
		int * num_zero_paths) {
	DdNode * then_bdd;
	DdNode * else_bdd;

	if (bdd == Cudd_ReadLogicZero(mgr)) {
		*num_zero_paths = *num_zero_paths + 1;
		return;
	} else if (bdd == Cudd_ReadOne(mgr)) {
		*num_one_paths = *num_one_paths + 1;
		return;
	}

	then_bdd = Cudd_T(bdd);
	ace_bdd_count_paths(mgr, then_bdd, num_one_paths, num_zero_paths);
	//bdd_free(then_bdd);

	else_bdd = Cudd_E(bdd);
	ace_bdd_count_paths(mgr, else_bdd, num_one_paths, num_zero_paths);
	//bdd_free(else_bdd);

}

#if 0
int ace_bdd_build_network_bdds(
		Abc_Ntk_t * ntk,
		st_table * leaves,
		Vec_Ptr_t * inputs,
		int max_size,
		double min_Prob)
{
	Abc_Obj_t * obj;
	double * P1s;
	int i;
	Vec_Ptr_t * nodes;

	VTR_ASSERT(Vec_PtrSize(inputs) > 0);

	nodes = Abc_NtkDfsSeq(ntk);

	P1s = malloc(sizeof(double) * Vec_PtrSize(inputs));

	Vec_PtrForEachEntry (Abc_Obj_t *, inputs, obj, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
		P1s[i] = info->static_prob;
		info->prob0to1 = ACE_P0TO1(info->static_prob, info->switch_prob);
		info->prob1to0 = ACE_P1TO0(info->static_prob, info->switch_prob);
	}

	Vec_PtrForEachEntry(Abc_Obj_t *, nodes, obj, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
		if (Abc_ObjIsCi(obj))
		{
			continue;
		}

		switch (info->status)
		{
			case ACE_SIM:
			VTR_ASSERT (info->static_prob >= 0.0 && info->static_prob <= 1.0);
			VTR_ASSERT (info->switch_prob >= 0.0 && info->switch_prob <= 1.0);

			if (!st_lookup(leaves, (char*) obj, NULL))
			{
				st_insert(leaves, (char*) obj, (char*) Vec_PtrSize(inputs));
				Vec_PtrPush(inputs, obj);

				P1s = realloc(P1s, sizeof(double) * Vec_PtrSize(inputs));
				P1s[Vec_PtrSize(inputs) - 1] = info->static_prob;

				info->prob0to1 = ACE_P0TO1(info->static_prob, info->switch_prob);
				info->prob1to0 = ACE_P1TO0(info->static_prob, info->switch_prob);
			}
			break;

			case ACE_UNDEF:
			VTR_ASSERT(0);
			if (check_pi_status(obj))
			{
				while(1)
				{
					DdNode * bdd = obj->pData;
					int n0;
					int n1;
					ace_bdd_count_paths(ntk->pManFunc, bdd, &n1, &n0);
					if (n1 <= max_size && n0 <= max_size)
					{
						break;
					}

					//m == choose_root_fanin
					//st_insert(leaves, (char*) )
				}
			}
			break;

			case ACE_DEF:
			VTR_ASSERT(info->static_prob >= 0 && info->static_prob <= 1.0);
			VTR_ASSERT(info->switch_prob >= 0 && info->switch_prob <= 1.0);
			break;

			case ACE_NEW:
			case ACE_OLD:
			default:
			VTR_ASSERT(0);
		}
	}

	free(P1s);
	Vec_PtrFree(nodes);

	return 0;
}
#endif

double calc_cube_switch_prob_recur(DdManager * mgr, DdNode * bdd,
		ace_cube_t * cube, Vec_Ptr_t * inputs, st__table * visited, int phase) {
	double * current_prob;
	short i;
	Abc_Obj_t * pi;
	DdNode * bdd_if1, *bdd_if0;
	double then_prob, else_prob;

	if (bdd == Cudd_ReadLogicZero(mgr)) {
		if (phase == 0)
			return (1.0);
		if (phase == 1)
			return (0.0);
	} else if (bdd == Cudd_ReadOne(mgr)) {
		if (phase == 1)
			return (1.0);
		if (phase == 0)
			return (0.0);
	}

	if (st__lookup(visited, (char *) bdd, (char **) &current_prob)) {
		return (*current_prob);
	}

	/* Get literal index for this bdd node. */
	//VTR_ASSERT(0);
	i = Cudd_Regular(bdd)->index;
	pi = (Abc_Obj_t*) Vec_PtrEntry((Vec_Ptr_t*) inputs, i);

	current_prob = (double*) malloc(sizeof(double));

	if (Cudd_IsComplement(bdd)) {
		bdd_if1 = Cudd_E(bdd);
		bdd_if0 = Cudd_T(bdd);
	} else {
		bdd_if1 = Cudd_T(bdd);
		bdd_if0 = Cudd_E(bdd);
	}

	Ace_Obj_Info_t * fanin_info = Ace_ObjInfo(pi);

	then_prob = calc_cube_switch_prob_recur(mgr, bdd_if1, cube, inputs, visited,
			phase);
	VTR_ASSERT(then_prob + EPSILON >= 0 && then_prob - EPSILON <= 1);

	else_prob = calc_cube_switch_prob_recur(mgr, bdd_if0, cube, inputs, visited,
			phase);
	VTR_ASSERT(else_prob + EPSILON >= 0 && else_prob - EPSILON <= 1);

	switch (node_get_literal (cube->cube, i)) {
	case ZERO:
		*current_prob = fanin_info->prob0to1 * then_prob
				+ (1.0 - fanin_info->prob0to1) * else_prob;
		break;
	case ONE:
		*current_prob = (1.0 - fanin_info->prob1to0) * then_prob
				+ fanin_info->prob1to0 * else_prob;
		break;
	case TWO:
		*current_prob = fanin_info->static_prob * then_prob
				+ (1.0 - fanin_info->static_prob) * else_prob;
		break;
	default:
		fail("Bad literal.");
	}

	st__insert(visited, (char *) bdd, (char *) current_prob);

	VTR_ASSERT(*current_prob + EPSILON >= 0 && *current_prob - EPSILON < 1.0);
	return (*current_prob);
}

double calc_cube_switch_prob(DdManager * mgr, DdNode * bdd, ace_cube_t * cube,
		Vec_Ptr_t * inputs, int phase) {
	double sp;
	st__table * visited;

	visited = st__init_table(st__ptrcmp, st__ptrhash);

	sp = calc_cube_switch_prob_recur(mgr, bdd, cube, inputs, visited, phase);

	st__free_table(visited);

	VTR_ASSERT(sp + EPSILON >= 0. && sp - EPSILON <= 1.0);
	return (sp);
}

double calc_switch_prob_recur(DdManager * mgr, DdNode * bdd_next, DdNode * bdd,
		ace_cube_t * cube, Vec_Ptr_t * inputs, double P1, int phase) {
	short i;
	Abc_Obj_t * pi;
	double switch_prob_t, switch_prob_e;
	double prob;
	DdNode * bdd_if1, *bdd_if0;
	ace_cube_t * cube0, *cube1;
	Ace_Obj_Info_t * info;

	VTR_ASSERT(inputs != NULL);
	VTR_ASSERT(Vec_PtrSize(inputs) > 0);
	VTR_ASSERT(P1 >= 0);

	if (bdd == Cudd_ReadLogicZero(mgr)) {
		if (phase != 1)
			return (0.0);
		prob = calc_cube_switch_prob(mgr, bdd_next, cube, inputs, phase);
		prob *= P1;

		VTR_ASSERT(prob + EPSILON >= 0. && prob - EPSILON <= 1.);
		return (prob * P1);
	} else if (bdd == Cudd_ReadOne(mgr)) {
		if (phase != 0)
			return (0.0);
		prob = calc_cube_switch_prob(mgr, bdd_next, cube, inputs, phase);
		prob *= P1;

		VTR_ASSERT(prob + EPSILON >= 0. && prob - EPSILON <= 1.);
		return (prob * P1);
	}

	/* Get literal index for this bdd node. */
	i = Cudd_Regular(bdd)->index;
	pi = (Abc_Obj_t*) Vec_PtrEntry((Vec_Ptr_t*) inputs, i);
	info = Ace_ObjInfo(pi);

	if (Cudd_IsComplement(bdd)) {
		bdd_if1 = Cudd_E(bdd);
		bdd_if0 = Cudd_T(bdd);
	} else {
		bdd_if1 = Cudd_T(bdd);
		bdd_if0 = Cudd_E(bdd);
	}

	/* Recursive call down the THEN branch */
	cube1 = ace_cube_dup(cube);
	set_remove(cube1->cube, 2 * i);
	set_insert(cube1->cube, 2 * i + 1);
	switch_prob_t = calc_switch_prob_recur(mgr, bdd_next, bdd_if1, cube1,
			inputs, P1 * info->static_prob, phase);
	ace_cube_free(cube1);

	/* Recursive call down the ELSE branch */
	cube0 = ace_cube_dup(cube);
	set_insert(cube0->cube, 2 * i);
	set_remove(cube0->cube, 2 * i + 1);
	switch_prob_e = calc_switch_prob_recur(mgr, bdd_next, bdd_if0, cube0,
			inputs, P1 * (1.0 - info->static_prob), phase);
	ace_cube_free(cube0);

	VTR_ASSERT(switch_prob_t + EPSILON >= 0. && switch_prob_t - EPSILON <= 1.);
	VTR_ASSERT(switch_prob_e + EPSILON >= 0. && switch_prob_e - EPSILON <= 1.);

	return (switch_prob_t + switch_prob_e);
}

double ace_bdd_calc_switch_act(DdManager * mgr, Abc_Obj_t * obj,
		Vec_Ptr_t * fanins) {
	int d;
	int n0, n1;
	Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
	Abc_Obj_t * fanin;
	ace_cube_t * cube;
	double switch_act;
	int i;
	DdNode * bdd;

	d = info->depth;
	VTR_ASSERT(d > 0);
	d = (int) d * 0.4;
	if (d < 1) {
		d = 1;
	}

	if (Vec_PtrSize(fanins) < 1) {
		return 0.5;
	}

	bdd = (DdNode*) obj->pData;
	n0 = n1 = 0;
	ace_bdd_count_paths(mgr, bdd, &n1, &n0);

	Vec_PtrForEachEntry(Abc_Obj_t*, fanins, fanin, i)
	//#define Vec_PtrForEachEntry( vVec, pEntry, i ) for ( i = 0; (i < Vec_PtrSize(vVec)) && (((pEntry) = Vec_PtrEntry(vVec, i)), 1); i++ )
	//for ( i = 0; (i < Vec_PtrSize(fanins)) && (((fanin) = Vec_PtrEntry(fanins, i)), 1); i++ )
	{
		Ace_Obj_Info_t * fanin_info = Ace_ObjInfo(fanin);

		fanin_info->prob0to1 =
				ACE_P0TO1 (fanin_info->static_prob, fanin_info->switch_prob / (double) d);
		fanin_info->prob1to0 =
				ACE_P1TO0 (fanin_info->static_prob, fanin_info->switch_prob / (double) d);

		prob_epsilon_fix(&fanin_info->prob0to1);
		prob_epsilon_fix(&fanin_info->prob1to0);

		VTR_ASSERT(
				fanin_info->prob0to1 + EPSILON >= 0.
						&& fanin_info->prob0to1 - EPSILON <= 1.0);
		VTR_ASSERT(
				fanin_info->prob1to0 + EPSILON >= 0.
						&& fanin_info->prob1to0 - EPSILON <= 1.0);
	}
	cube = ace_cube_new_dc(Vec_PtrSize(fanins));

	switch_act = 2.0
			* calc_switch_prob_recur(mgr, bdd, bdd, cube, fanins, 1.0, n1 > n0)
			* (double) d;
	//switch_act = 2.0 * calc_switch_prob_recur (mgr, bdd, bdd, cube, fanins, 1.0, 1) * (double) d;

	return switch_act;
}

int node_error(int code) {
	switch (code) {
	case 0:
		fail("node_get_cube: node does not have a function");
		/* NOTREACHED */
	case 1:
		fail("node_get_cube: cube index out of bounds");
		/* NOTREACHED */
	case 2:
		fail("node_get_literal: bad cube");
		/* NOTREACHED */
	case 4:
		fail("foreach_fanin: node changed during generation");
		/* NOTREACHED */
	case 5:
		fail("foreach_fanout: node changed during generation");
		/* NOTREACHED */
	default:
		fail("error code unused");
	}
	return 0;
}
