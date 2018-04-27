#include "ace.h"
#include "depth.h"

#include "base/abc/abc.h"

int calc_depth(Abc_Obj_t * obj);

int calc_depth(Abc_Obj_t * obj) {
	int i, depth;
	Abc_Obj_t * fanin;

	depth = 0;

	switch (Abc_ObjType(obj)) {
	case ABC_OBJ_NODE:
	case ABC_OBJ_PO:
	case ABC_OBJ_BI:
		Abc_ObjForEachFanin(obj, fanin, i)
		{
			Ace_Obj_Info_t * fanin_info = Ace_ObjInfo(fanin);
			depth = MAX(depth, fanin_info->depth);
		}

		return (depth + 1);
	default:
		return 0;
	}

}

int ace_calc_network_depth(Abc_Ntk_t * ntk) {
	int i, depth;
	Abc_Obj_t * obj;
	Vec_Ptr_t * nodes;

	//nodes = Abc_NtkDfsSeq(ntk);
	nodes = Abc_NtkDfs(ntk, TRUE);

	depth = 0;
	Vec_PtrForEachEntry(Abc_Obj_t*, nodes, obj, i)
	{
		Ace_Obj_Info_t * info = Ace_ObjInfo(obj);

		info->depth = calc_depth(obj);
		depth = MAX(depth, info->depth);
	}

    Vec_PtrFree(nodes);
	return depth;
}
