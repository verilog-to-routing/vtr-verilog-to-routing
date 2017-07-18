#include "clustered_netlist.h"

#include "vtr_assert.h"
#include "vpr_error.h"

/*
*
*
* ClusteredNetlist Class Implementation
*
*
*/
ClusteredNetlist::ClusteredNetlist(std::string name, std::string id)
	: BaseNetlist(name, id) {}


/*
*
* Blocks
*
*/
t_pb* ClusteredNetlist::block_pb(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_pbs_[id];
}

t_type_ptr ClusteredNetlist::block_type(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_types_[id];
}

/*
*
* Mutators
*
*/
BlockId ClusteredNetlist::create_block(const char *name, t_pb* pb, t_type_ptr type) {
	BlockId blk_id = BaseNetlist::create_block(name);

	block_pbs_.insert(blk_id, pb);
	block_pbs_[blk_id]->name = vtr::strdup(name);
	block_types_.insert(blk_id, type);

	//Check post-conditions: size
	VTR_ASSERT(validate_block_sizes());

	//Check post-conditions: values
	VTR_ASSERT(block_pb(blk_id) == pb);
	VTR_ASSERT(block_type(blk_id) == type);

	return blk_id;
}

void ClusteredNetlist::set_netlist_id(std::string id) {
	//TODO: Add asserts?
	netlist_id_ = id;
}

/*
*
* Sanity Checks
*
*/
bool ClusteredNetlist::validate_block_sizes() const {
	if (block_pbs_.size() != block_ids_.size()
		|| block_types_.size() != block_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent block data sizes");
	}
	return BaseNetlist::validate_block_sizes();
}
