#include "vtr_assert.h"

#include "ace.h"
#include "sim.h"

#include "bdd/cudd/cudd.h"
#include "bdd/cudd/cuddInt.h"

void get_pi_values(Abc_Ntk_t * ntk, Vec_Ptr_t * nodes, int cycle);
int * getFaninValues(Abc_Obj_t * obj_ptr);
ace_status_t getFaninStatus(Abc_Obj_t * obj_ptr);
void evaluate_circuit(Abc_Ntk_t * ntk, Vec_Ptr_t * node_vec, int cycle);
void update_FFs(Abc_Ntk_t * ntk);

void get_pi_values(Abc_Ntk_t * ntk, Vec_Ptr_t * /*nodes*/, int cycle) {
	Abc_Obj_t * obj;
	Ace_Obj_Info_t * info;
	int i;
	double prob0to1, prob1to0, rand_num;

	//Vec_PtrForEachEntry(Abc_Obj_t *, nodes, obj, i)
	Abc_NtkForEachObj(ntk, obj, i)
	{
		info = Ace_ObjInfo(obj);
		if (Abc_ObjType(obj) == ABC_OBJ_PI) {
			if (info->values) {
				if (info->status == ACE_UNDEF) {
					info->status = ACE_NEW;
					if (info->values[cycle] == 1) {
						info->value = 1;
						info->num_toggles = 1;
						info->num_ones = 1;
					} else {
						info->value = 0;
						info->num_toggles = 0;
						info->num_ones = 0;
					}
				} else {
					switch (info->value) {
					case 0:
						if (info->values[cycle] == 1) {
							info->value = 1;
							info->status = ACE_NEW;
							info->num_toggles++;
							info->num_ones++;
						} else {
							info->status = ACE_OLD;
						}
						break;

					case 1:
						if (info->values[cycle] == 0) {
							info->value = 0;
							info->status = ACE_NEW;
							info->num_toggles++;
						} else {
							info->num_ones++;
							info->status = ACE_OLD;
						}
						break;

					default:
						printf("Bad Value\n");
						VTR_ASSERT(0);
						break;
					}
				}
			} else {
				prob0to1 = ACE_P0TO1(info->static_prob, info->switch_prob);
				prob1to0 = ACE_P1TO0(info->static_prob, info->switch_prob);

                //We don't need a cryptographically secure random number
                //generator so suppress warning in coverity
                //
                //coverity[dont_call]
				rand_num = (double) rand() / (double) RAND_MAX;

				if (info->status == ACE_UNDEF) {
					info->status = ACE_NEW;
					if (rand_num < prob0to1) {
						info->value = 1;
						info->num_toggles = 1;
						info->num_ones = 1;
					} else {
						info->value = 0;
						info->num_toggles = 0;
						info->num_ones = 0;
					}
				} else {
					switch (info->value) {
					case 0:
						if (rand_num < prob0to1) {
							info->value = 1;
							info->status = ACE_NEW;
							info->num_toggles++;
							info->num_ones++;
						} else {
							info->status = ACE_OLD;
						}
						break;

					case 1:
						if (rand_num < prob1to0) {
							info->value = 0;
							info->status = ACE_NEW;
							info->num_toggles++;
						} else {
							info->num_ones++;
							info->status = ACE_OLD;
						}
						break;

					default:
						printf("Bad value\n");
						VTR_ASSERT(FALSE);
						break;
					}
				}
			}
		}
	}
}

int * getFaninValues(Abc_Obj_t * obj_ptr) {
	Abc_Obj_t * fanin;
	int i;
	Ace_Obj_Info_t * info;
	int * faninValues;

	Abc_ObjForEachFanin(obj_ptr, fanin, i)
	{
		info = Ace_ObjInfo(fanin);
		if (info->status == ACE_UNDEF) {
			printf("Fan-in is undefined\n");
			VTR_ASSERT(FALSE);
		} else if (info->status == ACE_NEW) {
			break;
		}
	}

	if (i >= Abc_ObjFaninNum(obj_ptr)) {
		// inputs haven't changed
		return NULL;
	}

	faninValues = (int*) malloc(Abc_ObjFaninNum(obj_ptr) * sizeof(int));
	Abc_ObjForEachFanin(obj_ptr, fanin, i)
	{
		info = Ace_ObjInfo(fanin);
		faninValues[i] = info->value;
	}

	return faninValues;
}

ace_status_t getFaninStatus(Abc_Obj_t * obj_ptr) {
	Abc_Obj_t * fanin;
	int i;
	Ace_Obj_Info_t * info;

	Abc_ObjForEachFanin(obj_ptr, fanin, i)
	{
		info = Ace_ObjInfo(fanin);
		if (info->status == ACE_UNDEF) {
			return ACE_UNDEF;
		}
	}

	Abc_ObjForEachFanin(obj_ptr, fanin, i)
	{
		info = Ace_ObjInfo(fanin);
		if (info->status == ACE_NEW || info->status == ACE_SIM) {
			return ACE_NEW;
		}
	}

	return ACE_OLD;
}

void evaluate_circuit(Abc_Ntk_t * ntk, Vec_Ptr_t * node_vec, int /*cycle*/) {
	Abc_Obj_t * obj;
	Ace_Obj_Info_t * info;
	int i;
	int value = -1;
	int * faninValues;
	ace_status_t status;
	DdNode * dd_node;

	Vec_PtrForEachEntry(Abc_Obj_t*, node_vec, obj, i)
	{
		info = Ace_ObjInfo(obj);

		switch (Abc_ObjType(obj)) {
		case ABC_OBJ_PI:
		case ABC_OBJ_BO:
			break;

		case ABC_OBJ_PO:
		case ABC_OBJ_BI:
		case ABC_OBJ_LATCH:
		case ABC_OBJ_NODE:
			status = getFaninStatus(obj);
			switch (status) {
			case ACE_UNDEF:
				info->status = ACE_UNDEF;
				break;
			case ACE_OLD:
				info->status = ACE_OLD;
				info->num_ones += info->value;
				break;
			case ACE_NEW:
				if (Abc_ObjIsNode(obj)) {
					faninValues = getFaninValues(obj);
					VTR_ASSERT(faninValues);
					dd_node = Cudd_Eval((DdManager*) ntk->pManFunc, (DdNode*) obj->pData, faninValues);
					VTR_ASSERT(Cudd_IsConstant(dd_node));
					if (dd_node == Cudd_ReadOne((DdManager*) ntk->pManFunc)) {
						value = 1;
					} else if (dd_node == Cudd_ReadLogicZero((DdManager*) ntk->pManFunc)) {
						value = 0;
					} else {
						VTR_ASSERT(0);
					}
					free(faninValues);
				} else {
					Ace_Obj_Info_t * fanin_info = Ace_ObjInfo(
							Abc_ObjFanin0(obj));
					value = fanin_info->value;
				}

				if (info->value != value || info->status == ACE_UNDEF) {
					info->value = value;
					if (info->status != ACE_UNDEF) {
						/* Don't count the first value as a toggle */
						info->num_toggles++;
					}
					info->status = ACE_NEW;
				} else {
					info->status = ACE_OLD;
				}
				info->num_ones += info->value;
				break;
			default:
				VTR_ASSERT(0);
				break;
			}
			break;
		default:
			VTR_ASSERT(0);
			break;
		}
	}
}

void update_FFs(Abc_Ntk_t * ntk) {
	Abc_Obj_t * obj;
	int i;
	Ace_Obj_Info_t * bi_fanin_info;
	Ace_Obj_Info_t * bi_info;
	Ace_Obj_Info_t * latch_info;
	Ace_Obj_Info_t * bo_info;

	Abc_NtkForEachLatch(ntk, obj, i)
	{
		bi_fanin_info = Ace_ObjInfo(Abc_ObjFanin0(Abc_ObjFanin0(obj)));
		bi_info = Ace_ObjInfo(Abc_ObjFanin0(obj));
		bo_info = Ace_ObjInfo(Abc_ObjFanout0(obj));
		latch_info = Ace_ObjInfo(obj);

		// Value
		bi_info->value = bi_fanin_info->value;
		latch_info->value = bi_fanin_info->value;
		bo_info->value = bi_fanin_info->value;

		// Status
		bi_info->status = bi_fanin_info->status;
		latch_info->status = bi_fanin_info->status;
		bo_info->status = bi_fanin_info->status;

		// Ones
		bi_info->num_ones = bi_fanin_info->num_ones;
		latch_info->num_ones = bi_fanin_info->num_ones;
		bo_info->num_ones = bi_fanin_info->num_ones;

		// Toggles
		bi_info->num_toggles = bi_fanin_info->num_toggles;
		latch_info->num_toggles = bi_fanin_info->num_toggles;
		bo_info->num_toggles = bi_fanin_info->num_toggles;
	}
}

void ace_sim_activities(Abc_Ntk_t * ntk, Vec_Ptr_t * nodes, int max_cycles,
		double threshold) {
	Abc_Obj_t * obj;
	Ace_Obj_Info_t * info;
	int i;

	VTR_ASSERT(max_cycles > 0);
	VTR_ASSERT(threshold > 0.0);

//	srand((unsigned) time(NULL));

	//Vec_PtrForEachEntry(Abc_Obj_t *, nodes, obj, i)
	Abc_NtkForEachObj(ntk, obj, i)
	{
		info = Ace_ObjInfo(obj);
		info->value = 0;

		if (Abc_ObjType(obj) == ABC_OBJ_BO) {
			info->status = ACE_NEW;
		} else {
			info->status = ACE_UNDEF;
		}
		info->num_ones = 0;
		info->num_toggles = 0;
	}

	Vec_Ptr_t * logic_nodes = Abc_NtkDfs(ntk, TRUE);
	for (i = 0; i < max_cycles; i++) {
		get_pi_values(ntk, nodes, i);
		evaluate_circuit(ntk, logic_nodes, i);
		update_FFs(ntk);
	}

	//Vec_PtrForEachEntry(Abc_Obj_t *, nodes, obj, i)
	Abc_NtkForEachObj(ntk, obj, i)
	{
		info = Ace_ObjInfo(obj);
		info->static_prob = info->num_ones / (double) max_cycles;
		VTR_ASSERT(info->static_prob >= 0.0 && info->static_prob <= 1.0);
		info->switch_prob = info->num_toggles / (double) max_cycles;
		VTR_ASSERT(info->switch_prob >= 0.0 && info->switch_prob <= 1.0);

		VTR_ASSERT(info->switch_prob - EPSILON <= 2.0 * (1.0 - info->static_prob));
		VTR_ASSERT(info->switch_prob - EPSILON <= 2.0 * (info->static_prob));

		info->status = ACE_SIM;
	}
    Vec_PtrFree(logic_nodes);
}
