#include "vtr_assert.h"
#include "vtr_time.h" //For some reason this causes compilation errors if included below the std headers on with g++-5
#include "vtr_assert.h"

#include <stdio.h>
#include <inttypes.h>

#include "ace.h"
#include "io_ace.h"
#include "blif.h"
#include "cycle.h"
#include "sim.h"
#include "bdd.h"
#include "depth.h"
#include "cube.h"

// ABC Headers
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/io/ioAbc.h"
//#include "vecInt.h"

void print_status(Abc_Ntk_t * ntk);
void alloc_and_init_activity_info(Abc_Ntk_t * ntk);
void ace_update_latch_probs(Abc_Ntk_t * ntk);
void print_node_bdd(Abc_Ntk_t * ntk);
void print_nodes(Vec_Ptr_t * nodes);
int ace_calc_activity(Abc_Ntk_t * ntk, int num_vectors, char * clk_name);

st__table * ace_info_hash_table;

void print_status(Abc_Ntk_t * ntk) {
	int i;
	Abc_Obj_t * obj;

	Abc_NtkForEachNode(ntk, obj, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);
		switch (info->status) {
		case ACE_UNDEF:
			printf("%d: UNDEFINED\n", i);
			break;
		case ACE_DEF:
			printf("%d: DEFINED\n", i);
			break;
		case ACE_SIM:
			printf("%d: SIM\n", i);
			break;
		case ACE_NEW:
			printf("%d: NEW\n", i);
			break;
		case ACE_OLD:
			printf("%d: OLD\n", i);
			break;
        default:
            VTR_ASSERT_MSG(false, "Invalid ABC object info status");
		}
	}
}

void alloc_and_init_activity_info(Abc_Ntk_t * ntk) {
	Vec_Ptr_t * node_vec;
	Abc_Obj_t * obj_ptr;
	int i;

	node_vec = Abc_NtkDfsSeq(ntk);
	Vec_PtrForEachEntry(Abc_Obj_t*, node_vec, obj_ptr, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj_ptr);
		info->values = NULL;
		info->status = ACE_UNDEF;
		info->num_toggles = 0;
		info->num_ones = 0;
	}
	Vec_PtrFree(node_vec);
}

void ace_update_latch_probs(Abc_Ntk_t * ntk) {
	Abc_Obj_t * obj_ptr;
	Abc_Obj_t * fanin_ptr;
	Abc_Obj_t * fanout_ptr;
	Ace_Obj_Info_t * fanin_info;
	Ace_Obj_Info_t * fanout_info;
	int i;

	Abc_NtkForEachLatch(ntk, obj_ptr, i)
	{
		fanin_ptr = Abc_ObjFanin0(obj_ptr);
		fanout_ptr = Abc_ObjFanout0(obj_ptr);

		fanin_info = Ace_ObjInfo(fanin_ptr);
		fanout_info = Ace_ObjInfo(fanout_ptr);

		fanout_info->static_prob = fanin_info->static_prob;
		fanout_info->switch_prob = fanin_info->switch_prob;
		fanout_info->status = fanin_info->status;
	}
}

void print_node_bdd(Abc_Ntk_t * ntk) {
	Abc_Obj_t * obj;
	int i;

	Abc_NtkForEachNode(ntk, obj, i)
	{
		DdNode * node = (DdNode*) obj->pData;

		printf("Object: %d\n", obj->Id);
		fflush(0);
		//printf("Fanin: %d\n", Abc_ObjFaninNum(obj)); fflush(0);
		while (1) {
			if (node == Cudd_ReadOne((DdManager*)ntk->pManFunc)) {
				//printf("one!\n");
				break;
			} else if (node == Cudd_ReadLogicZero((DdManager*)ntk->pManFunc)) {
				//printf("zero!\n");
				break;
			}

			printf("\tVar: %hd (%08" PRIXPTR ")\n", Cudd_Regular(node)->index,
					(uintptr_t) node);
			fflush(0);

			DdNode * first_node;
			DdGen* gen = Cudd_FirstNode((DdManager*) ntk->pManFunc, node, &first_node);
            Cudd_GenFree(gen);
			node = Cudd_E(node);

		}
	}
}

void print_nodes(Vec_Ptr_t * nodes) {
	Abc_Obj_t * obj;
	int i;

	printf("Printing Nodes\n");
	Vec_PtrForEachEntry(Abc_Obj_t*, nodes, obj, i)
	{
		printf("\t%d. %d-%d-%s\n", i, Abc_ObjId(obj), Abc_ObjType(obj),
				Abc_ObjName(obj));
	}
	fflush(0);
}

int ace_calc_activity(Abc_Ntk_t * ntk, int num_vectors, char * clk_name) {
	int error = 0;
	Vec_Ptr_t * nodes_all;
	Vec_Ptr_t * nodes_logic;
	Vec_Ptr_t * next_state_node_vec;
	Vec_Ptr_t * latches_in_cycles_vec;
	Abc_Obj_t * obj;
	int i, j;
	Ace_Obj_Info_t * info;

	//Build BDD
	Abc_NtkSopToBdd(ntk);

	nodes_all = Abc_NtkDfsSeq(ntk);
	nodes_logic = Abc_NtkDfs(ntk, TRUE);

	//print_nodes(nodes_logic);

	Vec_PtrForEachEntry(Abc_Obj_t*, nodes_all, obj, i)
	{
		info = Ace_ObjInfo(obj);
		info->status = ACE_UNDEF;
	}

	Abc_NtkForEachPi(ntk, obj, i)
	{
		info = Ace_ObjInfo(obj);
		if (strcmp(Abc_ObjName(obj), clk_name) != 0) {
			VTR_ASSERT(info->static_prob >= 0 && info->static_prob <= 1.0);
			VTR_ASSERT(info->switch_prob >= 0 && info->switch_prob <= 1.0);
			VTR_ASSERT(info->switch_act >= 0 && info->switch_act <= 1.0);
			VTR_ASSERT(info->switch_prob <= 2.0 * (1.0 - info->static_prob));
			VTR_ASSERT(info->switch_prob <= 2.0 * info->static_prob);
		}
		info->status = ACE_DEF;
	}

	latches_in_cycles_vec = latches_in_cycles(ntk);
	printf("%d/%d latches are part of cycle(s)\n", latches_in_cycles_vec->nSize,
			Abc_NtkLatchNum(ntk));
	fflush(0);

	//if (latches_in_cycles_vec->nSize)
	if (TRUE) {
		//print_status(ntk);

		printf("Stage 1: Simulating Probabilities...\n");
		fflush(0);

		next_state_node_vec = Abc_NtkDfsSeq(ntk);

		//print_nodes(next_state_node_vec);

		ace_sim_activities(ntk, next_state_node_vec, num_vectors, 0.05);
		//ace_sim_activities(ntk, nodes_logic, num_vectors, 0.05);

		ace_update_latch_probs(ntk);

		Vec_PtrFree(next_state_node_vec);
	}

	//print_status(ntk);
	printf("Stage 2: Computing Probabilities...\n");
	fflush(0);
	// Currently this stage does nothing

#if 0
	ace_bdd_get_literals (ntk, &leaves, &literals);

	i = 0;
	while(1)
	{
		//printf("Calc Iteration = %d\n", i++); fflush(0);
		if (ace_bdd_build_network_bdds(ntk, leaves, literals, ACE_MAX_BDD_SIZE, ACE_MIN_BDD_PROB) < 1)
		{
			break;
		}
		ace_update_latch_static_probs(ntk);
		ace_update_latch_switch_probs(ntk);
	}
	st__free_table(leaves);
	Vec_PtrFree(literals);
#endif

	/*------------- Computing Register Output Activities. ---------------------*/
	printf("Stage 3: Computing Register Output Activities...\n");
	fflush(0);
	Abc_NtkForEachLatchOutput(ntk, obj, i)
	{
		Ace_Obj_Info_t * info2 = Ace_ObjInfo(obj);

		info2->switch_act = info2->switch_prob;
		VTR_ASSERT(info2->switch_act >= 0.0);
	}
	Abc_NtkForEachPi(ntk, obj, i)
	{
		VTR_ASSERT(Ace_ObjInfo(obj)->switch_act >= 0.0);
	}

	/*------------- Calculate switching activities. ---------------------*/
	printf("Stage 4: Computing Switching Activities...\n");
	fflush(0);

	/* Do latches first, then logic after */
	Vec_PtrForEachEntry(Abc_Obj_t*, nodes_all, obj, i)
	{
		Ace_Obj_Info_t * info2 = Ace_ObjInfo(obj);

		switch (Abc_ObjType(obj)) {
		case ABC_OBJ_PI:
			if (strcmp(Abc_ObjName(obj), clk_name) == 0) {
				info2->switch_act = 2;
				info2->switch_prob = 1;
				info2->static_prob = 0.5;
			} else {
				info2->switch_act = info2->switch_prob;
			}
			break;

		case ABC_OBJ_BO:
		case ABC_OBJ_LATCH:
			info2->switch_act = info2->switch_prob;
			break;

		default:
			break;
		}
	}

	Vec_PtrForEachEntry(Abc_Obj_t*, nodes_logic, obj, i)
	{
		Ace_Obj_Info_t * info2 = Ace_ObjInfo(obj);
		//Ace_Obj_Info_t * fanin_info2;

		VTR_ASSERT(Abc_ObjType(obj) == ABC_OBJ_NODE);

		if (Abc_ObjFaninNum(obj) < 1) {
			info2->switch_act = 0.0;
			continue;
		} else {
			Vec_Ptr_t * literals = Vec_PtrAlloc(0);
			Abc_Obj_t * fanin;

			VTR_ASSERT(obj->Type == ABC_OBJ_NODE);

			Abc_ObjForEachFanin(obj, fanin, j)
			{
				Vec_PtrPush(literals, fanin);
			}
			info2->switch_act = ace_bdd_calc_switch_act((DdManager*)ntk->pManFunc, obj,
					literals);
			Vec_PtrFree(literals);
		}
		VTR_ASSERT(info2->switch_act >= 0);
	}
    Vec_PtrFree(nodes_logic);
    Vec_PtrFree(latches_in_cycles_vec);

	return error;
}

Ace_Obj_Info_t * Ace_ObjInfo(Abc_Obj_t * obj) {
	Ace_Obj_Info_t * info;

	if (st__lookup(ace_info_hash_table, (char *) obj, (char **) &info)) {
		return info;
	}
	VTR_ASSERT(0);
    return NULL;
}

void prob_epsilon_fix(double * d) {
	if (*d < 0) {
		VTR_ASSERT(*d > 0 - EPSILON);
		*d = 0;
	} else if (*d > 1) {
		VTR_ASSERT(*d < 1 + EPSILON);
		*d = 1.;
	}
}

int main(int argc, char * argv[]) {
    vtr::ScopedFinishTimer t("Ace");
	FILE * BLIF = NULL;
	FILE * IN_ACT = NULL;
	FILE * OUT_ACT = stdout;
	ace_pi_format_t pi_format = ACE_CODED;
	double p, d;
	int i;
	int depth;
	int error = 0;
	Abc_Frame_t * pAbc;
	Abc_Ntk_t * ntk;
	Abc_Obj_t * obj;
	int seed = 0;

	p = ACE_PI_STATIC_PROB;
	d = ACE_PI_SWITCH_PROB;

	char blif_file_name[BLIF_FILE_NAME_LEN];
	char new_blif_file_name[BLIF_FILE_NAME_LEN];
    char* clk_name = NULL;
	ace_io_parse_argv(argc, argv, &BLIF, &IN_ACT, &OUT_ACT, blif_file_name,
			new_blif_file_name, &pi_format, &p, &d, &seed, &clk_name);

	srand(seed);

	pAbc = Abc_FrameGetGlobalFrame();

	ntk = Io_Read(blif_file_name, IO_FILE_BLIF, 1, 0);

    VTR_ASSERT(ntk);

	printf("Objects in network: %d\n", Abc_NtkObjNum(ntk));
	printf("PIs in network: %d\n", Abc_NtkPiNum(ntk));

	printf("POs in network: %d\n", Abc_NtkPoNum(ntk));

	printf("Nodes in network: %d\n", Abc_NtkNodeNum(ntk));

	printf("Latches in network: %d\n", Abc_NtkLatchNum(ntk));

	if (!Abc_NtkIsAcyclic(ntk)) {
		printf("Circuit has combinational loops\n");
		exit(0);
	}

	// Alloc Aux Info Array

	// Full Allocation
	Ace_Obj_Info_t * info = (Ace_Obj_Info_t*) calloc(Abc_NtkObjNum(ntk), sizeof(Ace_Obj_Info_t));
	ace_info_hash_table = st__init_table(st__ptrcmp, st__ptrhash);

	int objNum = 0;
	Abc_NtkForEachObj(ntk, obj, i)
	{
		st__insert(ace_info_hash_table, (char *) obj, (char *) &info[objNum]);
		objNum++;
	}

	// Check Depth
	depth = ace_calc_network_depth(ntk);
	printf("Max Depth: %d\n", depth);
	VTR_ASSERT(depth > 0);

	alloc_and_init_activity_info(ntk);

	switch (pi_format) {
	case ACE_CODED:
		printf("Input activities will be assumed (%f, %f, %f)...\n",
		ACE_PI_STATIC_PROB, ACE_PI_SWITCH_PROB, ACE_PI_SWITCH_ACT);
		break;
	case ACE_PD:
		printf("Input activities will be (%f, %f, %f)...\n", p, d, d);
		fflush(0);
		break;
	case ACE_ACT:
		printf("Input activities will be read from an activity file...\n");
		break;
	case ACE_VEC:
		printf("Input activities will be read from a vector file...\n");
		break;
	default:
		printf("Error reading activities.\n");
		error = ACE_ERROR;
		break;
	}

	if (!error) {
		if (clk_name == NULL) {
			// No clocks
			printf(
					"No clocks detected in blif file.  This is not supported.\n");
			error = ACE_ERROR;
		} else {
			printf("Clock detected: %s\n", clk_name);
		}
	}

	// Read Activities
	if (!error) {
		error = ace_io_read_activity(ntk, IN_ACT, pi_format, p, d, clk_name);
	}

	if (!error) {
		error = ace_calc_activity(ntk, ACE_NUM_VECTORS, clk_name);
	}

	//Abc_NtkToSop(ntk, 0);
	Abc_Ntk_t * new_ntk;
	new_ntk = Abc_NtkToNetlist(ntk);

	if (!error) {
		ace_io_print_activity(ntk, OUT_ACT);
	}

	if (!error) {
		Io_WriteHie(ntk, blif_file_name, new_blif_file_name);
		printf("Done\n");
	}

	fflush(0);
	return 0;
}
