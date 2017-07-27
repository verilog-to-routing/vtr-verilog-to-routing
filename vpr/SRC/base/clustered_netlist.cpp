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
* Nets
*
*/
NetId ClusteredNetlist::block_net(const BlockId blk_id, const int pin_index) const {
	VTR_ASSERT(valid_block_id(blk_id));
	
	std::vector<PinId> pin_ids = block_pins_[blk_id];
	if (!valid_pin_id(pin_ids[pin_index]))
		return NetId::INVALID();
	else
		return pin_net(pin_ids[pin_index]);
}

bool ClusteredNetlist::net_global(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_global_[id];
}

bool ClusteredNetlist::net_routed(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_routed_[id];
}

bool ClusteredNetlist::net_fixed(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_fixed_[id];
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

PortId ClusteredNetlist::create_port(const BlockId blk_id, const std::string name, BitIndex width, PortType port_type) {
	PortId port_id = find_port(blk_id, name);
	if (!port_id) {
		port_id = BaseNetlist::create_port(blk_id, name, width);
		associate_port_with_block(port_id, port_type, blk_id);
	}

	//Check post-conditions: values
	VTR_ASSERT(port_name(port_id) == name);
	VTR_ASSERT_SAFE(find_port(blk_id, name) == port_id);

	return port_id;
}

NetId ClusteredNetlist::create_net(const std::string name) {
	//Check if the net has already been created
	StringId name_id = create_string(name);
	NetId net_id = find_net(name_id);

	if (net_id == NetId::INVALID()) {
		net_id = BaseNetlist::create_net(name);
		net_global_.push_back(false);
		net_fixed_.push_back(false);
		net_routed_.push_back(false);
	}

	VTR_ASSERT(validate_net_sizes());

	return net_id;
}

void ClusteredNetlist::set_netlist_id(std::string id) {
	//TODO: Add asserts?
	netlist_id_ = id;
}

void ClusteredNetlist::set_global(NetId net_id) {
	VTR_ASSERT(valid_net_id(net_id));

	net_global_[net_id] = true;
}

void ClusteredNetlist::set_routed(NetId net_id) {
	VTR_ASSERT(valid_net_id(net_id));

	net_routed_[net_id] = true;
}

void ClusteredNetlist::set_fixed(NetId net_id) {
	VTR_ASSERT(valid_net_id(net_id));

	net_fixed_[net_id] = true;
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

bool ClusteredNetlist::validate_net_sizes() const {
	if (net_global_.size() != net_ids_.size()
		|| net_fixed_.size() != net_ids_.size()
		|| net_routed_.size() != net_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent net data sizes");
	}
	return BaseNetlist::validate_net_sizes();
}