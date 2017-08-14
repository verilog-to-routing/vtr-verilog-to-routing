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
	: Netlist<ClusterBlockId, ClusterPortId, ClusterPinId, ClusterNetId>(name, id) {}

/*
*
* Blocks
*
*/
t_pb* ClusteredNetlist::block_pb(const ClusterBlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_pbs_[id];
}

t_type_ptr ClusteredNetlist::block_type(const ClusterBlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_types_[id];
}

ClusterNetId ClusteredNetlist::block_net(const ClusterBlockId blk_id, const int _pin_index) const {
	VTR_ASSERT(valid_block_id(blk_id));

	return block_nets_[blk_id][_pin_index];
}

int ClusteredNetlist::block_net_count(const ClusterBlockId blk_id, const int _pin_index) const {
	VTR_ASSERT(valid_block_id(blk_id));

	return block_net_count_[blk_id][_pin_index];
}

/*
*
* Pins
*
*/
int ClusteredNetlist::pin_index(const ClusterPinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_index_[id];
}

int ClusteredNetlist::pin_index(ClusterNetId net_id, int count) const {

	for (auto pin_id : net_pins(net_id)) {
		if (count == 0) {
			return pin_index(pin_id);
		}
		count--;
	}

	VTR_ASSERT(count);

	return -1;
}

/*
*
* Nets
*
*/
ClusterBlockId ClusteredNetlist::net_pin_block(const ClusterNetId net_id, int _pin_index) const {
	VTR_ASSERT(valid_net_id(net_id));

	for (auto pin_id : net_pins(net_id)) {
		if (_pin_index == 0) {
			return pin_block(pin_id);
		}
		_pin_index--;
	}
	
	VTR_ASSERT(_pin_index);
	return ClusterBlockId::INVALID();
}

int ClusteredNetlist::net_pin_index(ClusterNetId net_id, ClusterPinId pin_id) const {
	VTR_ASSERT(valid_net_id(net_id));
	VTR_ASSERT(valid_pin_id(pin_id));

	int count = 0;
	
	for (ClusterPinId pin : net_pins(net_id)) {
		if (pin == pin_id)
			break;
		count++;
	}

	//Ensure that a pin is found
	VTR_ASSERT(count < (int)net_pins(net_id).size());

	return count;
}

bool ClusteredNetlist::net_global(const ClusterNetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_global_[id];
}

bool ClusteredNetlist::net_routed(const ClusterNetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_routed_[id];
}

bool ClusteredNetlist::net_fixed(const ClusterNetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return net_fixed_[id];
}


/*
*
* Mutators
*
*/
ClusterBlockId ClusteredNetlist::create_block(const char *name, t_pb* pb, t_type_ptr type) {
	ClusterBlockId blk_id = find_block(name);
	if (blk_id == ClusterBlockId::INVALID()) {
		blk_id = Netlist::create_block(name);

		block_pbs_.insert(blk_id, pb);
		block_pbs_[blk_id]->name = vtr::strdup(name);
		block_types_.insert(blk_id, type);

		//Allocate and initialize every potential net of the block
		block_nets_.insert(blk_id, std::vector<ClusterNetId>());
		block_net_count_.insert(blk_id, std::vector<int>());

		for (int i = 0; i < type->num_pins; i++) {
			block_nets_[blk_id].push_back(ClusterNetId::INVALID());
			block_net_count_[blk_id].push_back(-1);
		}
	}

	//Check post-conditions: size
	VTR_ASSERT(validate_block_sizes());

	//Check post-conditions: values
	VTR_ASSERT(block_pb(blk_id) == pb);
	VTR_ASSERT(block_type(blk_id) == type);

	return blk_id;
}

void ClusteredNetlist::set_block_net(const ClusterBlockId blk_id, const int _pin_index, const ClusterNetId net_id) {
	VTR_ASSERT(valid_block_id(blk_id));
	VTR_ASSERT(valid_net_id(net_id));

	block_nets_[blk_id][_pin_index] = net_id;
}

void ClusteredNetlist::set_block_net_count(const ClusterBlockId blk_id, const int _pin_index, const int count) {
	VTR_ASSERT(valid_block_id(blk_id));

	block_net_count_[blk_id][_pin_index] = count;
}

ClusterPortId ClusteredNetlist::create_port(const ClusterBlockId blk_id, const std::string name, BitIndex width, PortType port_type) {
	ClusterPortId port_id = find_port(blk_id, name);
	if (!port_id) {
		port_id = Netlist::create_port(blk_id, name, width);
		associate_port_with_block(port_id, port_type, blk_id);
	}

	//Check post-conditions: values
	VTR_ASSERT(port_name(port_id) == name);
	VTR_ASSERT_SAFE(find_port(blk_id, name) == port_id);

	return port_id;
}

ClusterPinId ClusteredNetlist::create_pin(const ClusterPortId port_id, BitIndex port_bit, const ClusterNetId net_id, const PinType pin_type_, const PortType port_type_, int _pin_index, bool is_const) {
	ClusterPinId pin_id = Netlist::create_pin(port_id, port_bit, net_id, pin_type_, port_type_, is_const);

	pin_index_.push_back(_pin_index);

	ClusterBlockId block_id = port_block(port_id);
	
	VTR_ASSERT(pin_index < block_type(block_id)->num_pins);
	block_nets_[block_id][_pin_index] = net_id;

	VTR_ASSERT(validate_pin_sizes());

	return pin_id;
}

void ClusteredNetlist::set_pin_index(const ClusterPinId pin_id, const int index) {
	VTR_ASSERT(valid_pin_id(pin_id));

	pin_index_[pin_id] = index;
}


ClusterNetId ClusteredNetlist::create_net(const std::string name) {
	//Check if the net has already been created
	StringId name_id = create_string(name);
	ClusterNetId net_id = find_net(name_id);

	if (net_id == ClusterNetId::INVALID()) {
		net_id = Netlist::create_net(name);
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

void ClusteredNetlist::set_global(ClusterNetId net_id, bool state) {
	VTR_ASSERT(valid_net_id(net_id));

	net_global_[net_id] = state;
}

void ClusteredNetlist::set_routed(ClusterNetId net_id, bool state) {
	VTR_ASSERT(valid_net_id(net_id));

	net_routed_[net_id] = state;
}

void ClusteredNetlist::set_fixed(ClusterNetId net_id, bool state) {
	VTR_ASSERT(valid_net_id(net_id));

	net_fixed_[net_id] = state;
}

void ClusteredNetlist::remove_block(const ClusterBlockId blk_id) {
	Netlist::remove_block(blk_id);

	//Remove & invalidate pointers
	free_pb(block_pbs_[blk_id]);
	delete block_pbs_[blk_id];
	block_pbs_.insert(blk_id, NULL);
	block_types_.insert(blk_id, NULL);
	block_nets_.insert(blk_id, std::vector<ClusterNetId>());
	block_net_count_.insert(blk_id, std::vector<int>());

	//Mark netlist dirty
	dirty_ = true;
}

void ClusteredNetlist::compress() {
	//Compress the various netlist components to remove invalid entries
	// Note: this invalidates all Ids
	vtr::vector_map<ClusterBlockId, ClusterBlockId> block_id_map(block_ids_.size());
	vtr::vector_map<ClusterPortId, ClusterPortId> port_id_map(port_ids_.size());
	vtr::vector_map<ClusterPinId, ClusterPinId> pin_id_map(pin_ids_.size());
	vtr::vector_map<ClusterNetId, ClusterNetId> net_id_map(net_ids_.size());

	build_id_maps(block_id_map, port_id_map, pin_id_map, net_id_map);

	Netlist::clean_nets(net_id_map);
	Netlist::clean_pins(pin_id_map);
	Netlist::clean_ports(port_id_map);
	Netlist::clean_blocks(block_id_map);

	//Now we re-build all the cross references
	rebuild_block_refs(pin_id_map, port_id_map);
	rebuild_port_refs(block_id_map, pin_id_map);
	rebuild_pin_refs(port_id_map, net_id_map);
	rebuild_net_refs(pin_id_map);

	//Re-build the lookups
	rebuild_lookups();

	clean_nets(net_id_map);
	clean_pins(pin_id_map);
	clean_blocks(block_id_map);

	//Resize containers to exact size
	shrink_to_fit();
}

void ClusteredNetlist::clean_blocks(const vtr::vector_map<ClusterBlockId, ClusterBlockId>& block_id_map) {
	//Update all the block values
	block_pbs_ = clean_and_reorder_values(block_pbs_, block_id_map);
	block_types_ = clean_and_reorder_values(block_types_, block_id_map);
	block_nets_ = clean_and_reorder_values(block_nets_, block_id_map);
	block_net_count_ = clean_and_reorder_values(block_net_count_, block_id_map);

	VTR_ASSERT(validate_block_sizes());
}

void ClusteredNetlist::clean_pins(const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map) {
	//Update all the pin values
	pin_index_ = clean_and_reorder_values(pin_index_, pin_id_map);

	VTR_ASSERT(validate_pin_sizes());
}

void ClusteredNetlist::clean_nets(const vtr::vector_map<ClusterNetId, ClusterNetId>& net_id_map) {
	//Update all the net values
	net_global_ = clean_and_reorder_values(net_global_, net_id_map);
	net_routed_ = clean_and_reorder_values(net_routed_, net_id_map);
	net_fixed_ = clean_and_reorder_values(net_fixed_, net_id_map);

	VTR_ASSERT(validate_net_sizes());
}

void ClusteredNetlist::shrink_to_fit() {
	Netlist::shrink_to_fit();
	
	//Block data
	block_pbs_.shrink_to_fit();
	block_types_.shrink_to_fit();
	block_nets_.shrink_to_fit();
	block_net_count_.shrink_to_fit();
	VTR_ASSERT(validate_block_sizes());

	//Pin data
	pin_index_.shrink_to_fit();
	VTR_ASSERT(validate_pin_sizes());

	//Net data
	net_global_.shrink_to_fit();
	net_routed_.shrink_to_fit();
	net_fixed_.shrink_to_fit();
	VTR_ASSERT(validate_net_sizes());
}


/*
*
* Sanity Checks
*
*/
bool ClusteredNetlist::validate_block_sizes() const {
	if (block_pbs_.size() != block_ids_.size()
		|| block_types_.size() != block_ids_.size()
		|| block_nets_.size() != block_ids_.size()
		|| block_net_count_.size() != block_ids_.size()) {
		VPR_THROW(VPR_ERROR_CLB_NETLIST, "Inconsistent block data sizes");
	}
	return Netlist::validate_block_sizes();
}

bool ClusteredNetlist::validate_pin_sizes() const {
	if (pin_index_.size() != pin_ids_.size()) {
		VPR_THROW(VPR_ERROR_CLB_NETLIST, "Inconsistent pin data sizes");
	}
	return Netlist::validate_pin_sizes();
}

bool ClusteredNetlist::validate_net_sizes() const {
	if (net_global_.size() != net_ids_.size()
		|| net_fixed_.size() != net_ids_.size()
		|| net_routed_.size() != net_ids_.size()) {
		VPR_THROW(VPR_ERROR_CLB_NETLIST, "Inconsistent net data sizes");
	}
	return Netlist::validate_net_sizes();
}
