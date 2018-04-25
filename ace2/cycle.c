#include "cycle.h"
#include "ace.h"

bool in_cycle(Abc_Ntk_t * ntk, int obj_id_to_find, Abc_Obj_t * starting_obj_ptr,
		int flag) {
	Ace_Obj_Info_t * info;
	Abc_Obj_t * fanin_ptr;
	int i;

	info = Ace_ObjInfo(starting_obj_ptr);
	if (info->flag == flag) {
		return FALSE;
	}
	info->flag = flag;

	/*
	 if (Abc_ObjType(starting_obj_ptr) == ABC_OBJ_PI)
	 {
	 return FALSE;
	 }
	 */

	/* Don't think this is needed since ABC can traverse through a latch like any other node
	 else if (Abc_ObjType(starting_obj_ptr) == ABC_OBJ_BO)
	 {
	 // Get BI of latch
	 fanin_ptr = Abc_ObjFanin0(Abc_ObjFanin0(starting_obj_ptr));
	 assert(fanin_ptr);

	 return (in_cycle(ntk, obj_id_to_find, fanin_ptr, flag));
	 }
	 */

	Abc_ObjForEachFanin(starting_obj_ptr, fanin_ptr, i)
	{
		if (Abc_ObjId(fanin_ptr) == obj_id_to_find) {
			return TRUE;
		}
		if (in_cycle(ntk, obj_id_to_find, fanin_ptr, flag)) {
			return TRUE;
		}
	}

	return FALSE;
}

Vec_Ptr_t * latches_in_cycles(Abc_Ntk_t * ntk) {
	Vec_Ptr_t * latches_in_cycles_vec;
	Abc_Obj_t * obj_ptr;
	Abc_Obj_t * latch_ptr;
	int i;
	Ace_Obj_Info_t * info;
	int pass_num;

	// Initialize
	latches_in_cycles_vec = Vec_PtrStart(0);

	Abc_NtkForEachObj(ntk, obj_ptr, i)
	{
		info = Ace_ObjInfo(obj_ptr);
		info->flag = -1;
	}

	pass_num = 0;
	Abc_NtkForEachLatch(ntk, latch_ptr, i)
	{
		if (in_cycle(ntk, Abc_ObjId(latch_ptr), latch_ptr, pass_num)) {
			Vec_PtrPush(latches_in_cycles_vec, latch_ptr);
		}
		pass_num++;
	}

	return latches_in_cycles_vec;
}
