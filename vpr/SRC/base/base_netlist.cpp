#include <algorithm>
#include <numeric>

#include "base_netlist.h"
#include "vtr_assert.h"

#include "vpr_error.h"

/*
*
* BaseNetlist class implementation
*
*/
BaseNetlist::BaseNetlist(std::string name, std::string id)
	: netlist_name_(name)
	, netlist_id_(id) {}

/*
*
* Netlist
*
*/
const std::string& BaseNetlist::netlist_name() const {
	return netlist_name_;
}

const std::string& BaseNetlist::netlist_id() const {
	return netlist_id_;
}


/*
*
* Blocks
*
*/
const std::string& BaseNetlist::block_name(const BlockId id) const {
	StringId str_id = block_names_[id];
	return strings_[str_id];
}

BlockType BaseNetlist::block_type(const BlockId id) const {
	const t_model* blk_model = block_model(id);

	BlockType type = BlockType::BLOCK;
	if (blk_model->name == std::string("input")) {
		type = BlockType::INPAD;
	}
	else if (blk_model->name == std::string("output")) {
		type = BlockType::OUTPAD;
	}
	else {
		type = BlockType::BLOCK;
	}
	return type;
}

const t_model* BaseNetlist::block_model(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_models_[id];
}

bool  BaseNetlist::block_is_combinational(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_clock_pins(id).size() == 0;
}

BaseNetlist::pin_range BaseNetlist::block_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return vtr::make_range(block_pins_[id].begin(), block_pins_[id].end());
}

BaseNetlist::pin_range BaseNetlist::block_input_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin();

	auto end = block_pins_[id].begin() + block_num_input_pins_[id];

	return vtr::make_range(begin, end);
}

BaseNetlist::pin_range BaseNetlist::block_output_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin() + block_num_input_pins_[id];

	auto end = begin + block_num_output_pins_[id];

	return vtr::make_range(begin, end);
}

BaseNetlist::pin_range BaseNetlist::block_clock_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin()
		+ block_num_input_pins_[id]
		+ block_num_output_pins_[id];

	auto end = begin + block_num_clock_pins_[id];

	VTR_ASSERT(end == block_pins_[id].end());

	return vtr::make_range(begin, end);
}

BaseNetlist::port_range BaseNetlist::block_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return vtr::make_range(block_ports_[id].begin(), block_ports_[id].end());
}

BaseNetlist::port_range BaseNetlist::block_input_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_ports_[id].begin();

	auto end = block_ports_[id].begin() + block_num_input_ports_[id];

	return vtr::make_range(begin, end);
}

BaseNetlist::port_range BaseNetlist::block_output_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_ports_[id].begin() + block_num_input_ports_[id];

	auto end = begin + block_num_output_ports_[id];

	return vtr::make_range(begin, end);
}

BaseNetlist::port_range BaseNetlist::block_clock_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_ports_[id].begin()
		+ block_num_input_ports_[id]
		+ block_num_output_ports_[id];

	auto end = begin + block_num_clock_ports_[id];

	VTR_ASSERT(end == block_ports_[id].end());

	return vtr::make_range(begin, end);
}
/*
*
* Ports
*
*/
const std::string& BaseNetlist::port_name(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	StringId str_id = port_names_[id];
	return strings_[str_id];
}

BitIndex BaseNetlist::port_width(const PortId id) const {
	return port_model(id)->size;
}

BlockId BaseNetlist::port_block(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return port_blocks_[id];
}

PortType BaseNetlist::port_type(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	const t_model_ports* model_port = port_model(id);

	PortType type = PortType::INPUT;
	if (model_port->dir == IN_PORT) {
		if (model_port->is_clock) {
			type = PortType::CLOCK;
		}
		else {
			type = PortType::INPUT;
		}
	}
	else if (model_port->dir == OUT_PORT) {
		type = PortType::OUTPUT;
	}
	else {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Unrecognized model port type");
	}
	return type;
}

BaseNetlist::pin_range BaseNetlist::port_pins(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return vtr::make_range(port_pins_[id].begin(), port_pins_[id].end());
}

PinId BaseNetlist::port_pin(const PortId port_id, const BitIndex port_bit) const {
	//Convenience look-up bypassing port
	return find_pin(port_id, port_bit);
}

NetId BaseNetlist::port_net(const PortId port_id, const BitIndex port_bit) const {
	//port_pin() will validate that port_bit and port_id are valid so don't 
	//check redundently here

	//Convenience look-up bypassing port and pin
	PinId pin_id = port_pin(port_id, port_bit);
	if (pin_id) {
		return pin_net(pin_id);
	}
	else {
		return NetId::INVALID();
	}
}

const t_model_ports* BaseNetlist::port_model(const PortId port_id) const {
	VTR_ASSERT(valid_port_id(port_id));

	return port_models_[port_id];
}

/*
*
* Pins
*
*/
std::string BaseNetlist::pin_name(const PinId id) const {
	BlockId blk = pin_block(id);
	PortId port = pin_port(id);

	return block_name(blk) + "." + port_name(port) + "[" + std::to_string(pin_port_bit(id)) + "]";
}

NetId BaseNetlist::pin_net (const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_nets_[id];
}

PinType BaseNetlist::pin_type(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	PortId port_id = pin_port(id);
	PinType type;
	switch (port_type(port_id)) {
	case PortType::INPUT: /*fallthrough */;
	case PortType::CLOCK: type = PinType::SINK; break;
	case PortType::OUTPUT: type = PinType::DRIVER; break;
	default: VTR_ASSERT_MSG(false, "Valid atom port type");
	}
	return type;
}

PortId BaseNetlist::pin_port(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_ports_[id];
}

BitIndex BaseNetlist::pin_port_bit(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_port_bits_[id];
}

PortType BaseNetlist::pin_port_type(const PinId id) const {
	PortId port = pin_port(id);

	return port_type(port);
}

BlockId BaseNetlist::pin_block(const PinId id) const {
	//Convenience lookup bypassing the port
	VTR_ASSERT(valid_pin_id(id));

	PortId port_id = pin_port(id);
	return port_block(port_id);
}

bool BaseNetlist::pin_is_constant(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_is_constant_[id];
}

/*
*
* Nets
*
*/
const std::string& BaseNetlist::net_name(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	StringId str_id = net_names_[id];
	return strings_[str_id];
}

BaseNetlist::pin_range BaseNetlist::net_pins(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return vtr::make_range(net_pins_[id].begin(), net_pins_[id].end());
}

PinId BaseNetlist::net_driver(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	if (net_pins_[id].size() > 0) {
		return net_pins_[id][0];
	}
	else {
		return PinId::INVALID();
	}
}

BlockId BaseNetlist::net_driver_block(const NetId id) const {
	auto driver_pin_id = net_driver(id);
	if (driver_pin_id) {
		return pin_block(driver_pin_id);
	}
	return BlockId::INVALID();
}

BaseNetlist::pin_range BaseNetlist::net_sinks(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return vtr::make_range(++net_pins_[id].begin(), net_pins_[id].end());
}

bool BaseNetlist::net_is_constant(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	//Look-up the driver
	auto driver_pin_id = net_driver(id);
	if (driver_pin_id) {
		//Valid driver, see it is constant
		return pin_is_constant(driver_pin_id);
	}

	//No valid driver so can't be const
	return false;
}



/*
*
* Aggregates
*
*/
BaseNetlist::block_range BaseNetlist::blocks() const {
	return vtr::make_range(block_ids_.begin(), block_ids_.end());
}

BaseNetlist::pin_range BaseNetlist::pins() const {
	return vtr::make_range(pin_ids_.begin(), pin_ids_.end());
}

BaseNetlist::net_range BaseNetlist::nets() const {
	return vtr::make_range(net_ids_.begin(), net_ids_.end());
}

/*
*
* Lookups
*
*/
BlockId BaseNetlist::find_block(const std::string& name) const {
	auto str_id = find_string(name);
	if (!str_id) {
		return BlockId::INVALID();
	}
	else {
		return find_block(str_id);
	}
}

PortId BaseNetlist::find_port(const BlockId blk_id, const t_model_ports* model_port) const {
	VTR_ASSERT(valid_block_id(blk_id));
	VTR_ASSERT(model_port);

	//We can tell from the model port the set of ports
	//the port can be found in
	port_range range = (model_port->dir == IN_PORT) ?
		(model_port->is_clock) ? block_clock_ports(blk_id) : block_input_ports(blk_id)
		: (block_output_ports(blk_id));

	for (auto port_id : range) {
		if (port_name(port_id) == model_port->name) {
			return port_id;
		}
	}

	return PortId::INVALID();
}

PortId BaseNetlist::find_port(const BlockId blk_id, const std::string& name) const {
	VTR_ASSERT(valid_block_id(blk_id));

	//Since we only know the port name, we must search all the ports
	for (auto port_id : block_ports(blk_id)) {
		if (port_name(port_id) == name) {
			return port_id;
		}
	}
	return PortId::INVALID();
}

NetId BaseNetlist::find_net(const std::string& name) const {
	auto str_id = find_string(name);
	if (!str_id) {
		return NetId::INVALID();
	}
	else {
		return find_net(str_id);
	}
}

PinId BaseNetlist::find_pin(const PortId port_id, BitIndex port_bit) const {
	VTR_ASSERT(valid_port_id(port_id));
	VTR_ASSERT(valid_port_bit(port_id, port_bit));

	//Pins are stored in ascending order of bit index,
	//so we can binary search for the specific bit
	auto port_bit_cmp = [&](const PinId pin_id, BitIndex bit_index) {
		return pin_port_bit(pin_id) < bit_index;
	};

	auto pin_range = port_pins(port_id);

	//Finds the location where the pin with bit index port_bit should be located (if it exists)
	auto iter = std::lower_bound(pin_range.begin(), pin_range.end(), port_bit, port_bit_cmp);

	if (iter == pin_range.end() || pin_port_bit(*iter) != port_bit) {
		//Either the end of the pins (i.e. not found), or
		//the value does not match (indicating a gap in the indicies, so also not found)
		return PinId::INVALID();
	}
	else {
		//Found it
		VTR_ASSERT(pin_port_bit(*iter) == port_bit);
		return *iter;
	}
}

/*
*
* Validation
*
*/
//Checks that all cross-references are consistent.
//Should take linear time.
bool BaseNetlist::verify_refs() const {
	bool valid = true;
	valid &= validate_block_port_refs();
	valid &= validate_block_pin_refs();
	valid &= validate_port_pin_refs();
	valid &= validate_net_pin_refs();
	valid &= validate_string_refs();
	return valid;
}

bool BaseNetlist::verify_lookups() const {
	//Verify that fast look-ups are consistent

	//Blocks
	for (auto blk_id : blocks()) {
		const auto& name = block_name(blk_id);
		if (find_block(name) != blk_id) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
		}
	}

	//Ports
	for (auto port_id : port_ids_) {
		auto blk_id = port_block(port_id);
		const auto& name = port_name(port_id);
		if (find_port(blk_id, name) != port_id) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
		}
	}

	//Pins
	for (auto pin_id : pin_ids_) {
		auto port_id = pin_port(pin_id);
		auto bit = pin_port_bit(pin_id);
		if (find_pin(port_id, bit) != pin_id) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
		}
	}

	//Nets
	for (auto net_id : nets()) {
		const auto& name = net_name(net_id);
		if (find_net(name) != net_id) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
		}
	}

	//Port common
	for (auto str_id : string_ids_) {
		const auto& name = strings_[str_id];
		if (find_string(name) != str_id) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
		}
	}
	return true;
}


/*
*
* Mutators
*
*/
PortId  BaseNetlist::create_port(const BlockId blk_id, const t_model_ports* model_port) {
	//Check pre-conditions
	VTR_ASSERT_MSG(valid_block_id(blk_id), "Valid block id");

	//See if the port already exists
	StringId name_id = create_string(model_port->name);
	PortId port_id = find_port(blk_id, model_port);
	if (!port_id) {
		//Not found, create it

		//Reserve an id
		port_id = PortId(port_ids_.size());
		port_ids_.push_back(port_id);

		//Initialize the per-port-instance data
		port_blocks_.push_back(blk_id);
		port_names_.push_back(name_id);
		port_models_.push_back(model_port);

		//Allocate the pins, initialize to invalid Ids
		port_pins_.emplace_back();

		associate_port_with_block(port_id, blk_id);
	}

	//Check post-conditions: sizes
	VTR_ASSERT(validate_port_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_port_id(port_id));
	VTR_ASSERT(port_block(port_id) == blk_id);
	VTR_ASSERT(port_name(port_id) == model_port->name);
	VTR_ASSERT(port_width(port_id) == (unsigned)model_port->size);
	VTR_ASSERT(port_model(port_id) == model_port);
	VTR_ASSERT_SAFE(find_port(blk_id, model_port->name) == port_id);
	VTR_ASSERT_SAFE(find_port(blk_id, model_port) == port_id);

	return port_id;
}

PinId BaseNetlist::create_pin(const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType type, bool is_const) {
	//Check pre-conditions (valid ids)
	VTR_ASSERT_MSG(valid_port_id(port_id), "Valid port id");
	VTR_ASSERT_MSG(valid_port_bit(port_id, port_bit), "Valid port bit");
	VTR_ASSERT_MSG(valid_net_id(net_id), "Valid net id");

	//See if the pin already exists
	PinId pin_id = find_pin(port_id, port_bit);
	if (!pin_id) {
		//Not found, create it

		//Reserve an id
		pin_id = PinId(pin_ids_.size());
		pin_ids_.push_back(pin_id);

		//Initialize the pin data
		pin_ports_.push_back(port_id);
		pin_port_bits_.push_back(port_bit);
		pin_nets_.push_back(net_id);
		pin_is_constant_.push_back(is_const);

		//Add the pin to the net
		associate_pin_with_net(pin_id, type, net_id);

		//Add the pin to the port
		associate_pin_with_port(pin_id, port_id);

		//Add the pin to the block
		associate_pin_with_block(pin_id, port_type(port_id), port_block(port_id));
	}

	//Check post-conditions: sizes
	VTR_ASSERT(validate_pin_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_pin_id(pin_id));
	VTR_ASSERT(pin_port(pin_id) == port_id);
	VTR_ASSERT(pin_port_bit(pin_id) == port_bit);
	VTR_ASSERT(pin_net(pin_id) == net_id);
	VTR_ASSERT(pin_type(pin_id) == type);
	VTR_ASSERT(pin_is_constant(pin_id) == is_const);
	VTR_ASSERT_SAFE(find_pin(port_id, port_bit) == pin_id);

	return pin_id;
}


NetId BaseNetlist::create_net(const std::string name) {
	//Creates an empty net (or returns an existing one)
	VTR_ASSERT_MSG(!name.empty(), "Valid net name");

	//Check if the net has already been created
	StringId name_id = create_string(name);
	NetId net_id = find_net(name_id);
	if (net_id == NetId::INVALID()) {
		//Not found, create it

		//Reserve an id
		net_id = NetId(net_ids_.size());
		net_ids_.push_back(net_id);

		//Initialize the data
		net_names_.push_back(name_id);

		//Initialize the look-ups
		net_name_to_net_id_.insert(name_id, net_id);

		//Initialize with no driver
		net_pins_.emplace_back();
		net_pins_[net_id].emplace_back(PinId::INVALID());

		VTR_ASSERT(net_pins_[net_id].size() == 1);
		VTR_ASSERT(net_pins_[net_id][0] == PinId::INVALID());
	}

	//Check post-conditions: size
	VTR_ASSERT(validate_net_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_net_id(net_id));
	VTR_ASSERT(net_name(net_id) == name);
	VTR_ASSERT(find_net(name) == net_id);

	return net_id;
}

NetId BaseNetlist::add_net(const std::string name, PinId driver, std::vector<PinId> sinks) {
	//Creates a net with a full set of pins
	VTR_ASSERT_MSG(!find_net(name), "Net should not exist");

	//Create the empty net
	NetId net_id = create_net(name);

	//Set the driver and sinks of the net
	auto& dest_pins = net_pins_[net_id];
	dest_pins[0] = driver;
	dest_pins.insert(dest_pins.end(),
		std::make_move_iterator(sinks.begin()),
		std::make_move_iterator(sinks.end()));

	//Associate each pin with the net
	pin_nets_[driver] = net_id;
	for (auto sink : sinks) {
		pin_nets_[sink] = net_id;
	}

	return net_id;
}

void BaseNetlist::set_pin_is_constant(const PinId pin_id, const bool value) {
    VTR_ASSERT(valid_pin_id(pin_id));

    pin_is_constant_[pin_id] = value;
}

void BaseNetlist::set_pin_net(const PinId pin, PinType type, const NetId net) {
	VTR_ASSERT(valid_pin_id(pin));
	VTR_ASSERT((type == PinType::DRIVER && pin_port_type(pin) == PortType::OUTPUT)
		|| (type == PinType::SINK && (pin_port_type(pin) == PortType::INPUT || pin_port_type(pin) == PortType::CLOCK)));

	NetId orig_net = pin_net(pin);
	if (orig_net) {
		//Clean up the pin reference on the original net
		remove_net_pin(orig_net, pin);
	}

	//Mark the pin's net
	pin_nets_[pin] = net;

	//Add the pin to the net
	associate_pin_with_net(pin, type, net);
}

void BaseNetlist::remove_block(const BlockId blk_id) {
	VTR_ASSERT(valid_block_id(blk_id));

	//Remove the ports
	for (PortId block_port : block_ports(blk_id)) {
		remove_port(block_port);
	}

	//Invalidate look-up
	StringId name_id = block_names_[blk_id];
	block_name_to_block_id_.insert(name_id, BlockId::INVALID());

	//Mark as invalid
	block_ids_[blk_id] = BlockId::INVALID();

	//Mark netlist dirty
	dirty_ = true;
}

void BaseNetlist::remove_port(const PortId port_id) {
	VTR_ASSERT(valid_port_id(port_id));

	//Remove the pins
	for (auto pin : port_pins(port_id)) {
		if (valid_pin_id(pin)) {
			remove_pin(pin);
		}
	}

	//Mark as invalid
	port_ids_[port_id] = PortId::INVALID();

	//Mark netlist dirty
	dirty_ = true;
}

void BaseNetlist::remove_pin(const PinId pin_id) {
	VTR_ASSERT(valid_pin_id(pin_id));

	//Find the associated net
	NetId net = pin_net(pin_id);

	//Remove the pin from the associated net/port/block
	remove_net_pin(net, pin_id);

	//Mark as invalid
	pin_ids_[pin_id] = PinId::INVALID();

	//Mark netlist dirty
	dirty_ = true;
}

void BaseNetlist::remove_net(const NetId net_id) {
	VTR_ASSERT(valid_net_id(net_id));

	//Disassociate the pins from the net
	for (auto pin_id : net_pins(net_id)) {
		if (pin_id) {
			pin_nets_[pin_id] = NetId::INVALID();
		}
	}
	//Invalidate look-up
	StringId name_id = net_names_[net_id];
	net_name_to_net_id_.insert(name_id, NetId::INVALID());

	//Mark as invalid
	net_ids_[net_id] = NetId::INVALID();

	//Mark netlist dirty
	dirty_ = true;
}

void BaseNetlist::remove_net_pin(const NetId net_id, const PinId pin_id) {
	//Remove a net-pin connection
	//
	//Note that during sweeping either the net or pin could be invalid (i.e. already swept)
	//so we check before trying to use them

	if (valid_net_id(net_id)) {
		//Warning: this is slow!
		auto iter = std::find(net_pins_[net_id].begin(), net_pins_[net_id].end(), pin_id); //Linear search
		VTR_ASSERT(iter != net_pins_[net_id].end());

		if (net_driver(net_id) == pin_id) {
			//Mark no driver
			net_pins_[net_id][0] = PinId::INVALID();
		}
		else {
			//Remove sink
			net_pins_[net_id].erase(iter); //Linear remove
		}

		//Note: since we fully update the net we don't need to mark the netlist dirty_
	}

	//Dissassociate the pin with the net
	if (valid_pin_id(pin_id)) {
		pin_nets_[pin_id] = NetId::INVALID();

		//Mark netlist dirty, since we are leaving an invalid net id
		dirty_ = true;
	}
}

void BaseNetlist::remove_unused() {
	//Mark any nets/pins/ports/blocks which are not in use as invalid
	//so they will be removed

	bool found_unused;
	do {
		found_unused = false;
		//Nets with no connected pins
		for (auto net_id : nets()) {
			if (net_id
				&& !net_driver(net_id)
				&& net_sinks(net_id).size() == 0) {
				remove_net(net_id);
				found_unused = true;
			}
		}

		//Pins with no connected nets
		for (auto pin_id : pin_ids_) {
			if (pin_id && !pin_net(pin_id)) {
				remove_pin(pin_id);
				found_unused = true;
			}
		}

		//Empty ports
		for (auto port_id : port_ids_) {
			if (port_id && port_pins(port_id).size() == 0) {
				remove_port(port_id);
				found_unused = true;
			}
		}

		//Blocks with no pins
		for (auto blk_id : blocks()) {
			if (blk_id && block_pins(blk_id).size() == 0) {
				remove_block(blk_id);
				found_unused = true;
			}
		}
	} while (found_unused);
}

/*
*
* Sanity Checks
*
*/
bool BaseNetlist::valid_block_id(BlockId id) const {
	if (id == BlockId::INVALID()) return false;
	else if (!block_ids_.contains(id)) return false;
	else if (block_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_port_id(PortId id) const {
	if (id == PortId::INVALID()) return false;
	else if (!port_ids_.contains(id)) return false;
	else if (port_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_port_bit(PortId id, BitIndex port_bit) const {
	VTR_ASSERT(valid_port_id(id));
	if (port_bit >= port_width(id)) return false;
	return true;
}

bool BaseNetlist::valid_pin_id(PinId id) const {
	if (id == PinId::INVALID()) return false;
	else if (!pin_ids_.contains(id)) return false;
	else if (pin_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_net_id(NetId id) const {
	if (id == NetId::INVALID()) return false;
	else if (!net_ids_.contains(id)) return false;
	else if (net_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_string_id(StringId id) const {
	if (id == StringId::INVALID()) return false;
	else if (!string_ids_.contains(id)) return false;
	else if (string_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::validate_block_sizes() const {
	if (block_names_.size() != block_ids_.size()
		|| block_models_.size() != block_ids_.size()
		|| block_pins_.size() != block_ids_.size()
		|| block_num_input_pins_.size() != block_ids_.size()
		|| block_num_output_pins_.size() != block_ids_.size()
		|| block_num_clock_pins_.size() != block_ids_.size()
		|| block_ports_.size() != block_ids_.size()
		|| block_num_input_ports_.size() != block_ids_.size()
		|| block_num_output_ports_.size() != block_ids_.size()
		|| block_num_clock_ports_.size() != block_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent block data sizes");
	}
	return true;
}

bool BaseNetlist::validate_port_sizes() const {
	if (port_names_.size() != port_ids_.size()
		|| port_blocks_.size() != port_ids_.size()
		|| port_models_.size() != port_ids_.size()
		|| port_pins_.size() != port_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent port data sizes");
	}
	return true;
}

bool BaseNetlist::validate_pin_sizes() const {
	if (pin_ports_.size() != pin_ids_.size()
		|| pin_port_bits_.size() != pin_ids_.size()
		|| pin_nets_.size() != pin_ids_.size()
		|| pin_is_constant_.size() != pin_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent pin data sizes");
	}
	return true;
}

bool BaseNetlist::validate_net_sizes() const {
	if (net_names_.size() != net_ids_.size()
		|| net_pins_.size() != net_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent net data sizes");
	}
	return true;
}

bool BaseNetlist::validate_string_sizes() const {
	if (strings_.size() != string_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent string data sizes");
	}
	return true;
}

bool BaseNetlist::validate_block_port_refs() const {
	//Verify that all block <-> port references are consistent

	//Track how many times we've seen each port from the blocks 
	vtr::vector_map<PortId, unsigned> seen_port_ids(port_ids_.size());

	for (auto blk_id : blocks()) {
		for (auto in_port_id : block_input_ports(blk_id)) {
			if (blk_id != port_block(in_port_id)) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-input port cross-reference does not match");
			}
			++seen_port_ids[in_port_id];
		}
		for (auto out_port_id : block_output_ports(blk_id)) {
			if (blk_id != port_block(out_port_id)) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-output port cross-reference does not match");
			}
			++seen_port_ids[out_port_id];
		}
		for (auto clock_port_id : block_clock_ports(blk_id)) {
			if (blk_id != port_block(clock_port_id)) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-clock port cross-reference does not match");
			}
			++seen_port_ids[clock_port_id];
		}
	}

	//Check that we have neither orphaned ports (i.e. that aren't referenced by a block) 
	//nor shared ports (i.e. referenced by multiple blocks)
	if (!std::all_of(seen_port_ids.begin(), seen_port_ids.end(),
		[](unsigned val) {
		return val == 1;
	})) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Port not referenced by a single block");
	}

	if (std::accumulate(seen_port_ids.begin(), seen_port_ids.end(), 0u) != port_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned port (not referenced by a block)");
	}

	return true;
}

bool BaseNetlist::validate_block_pin_refs() const {
	//Verify that block pin refs are consistent 
	//(e.g. inputs are inputs, outputs are outputs etc.)
	for (auto blk_id : blocks()) {

		//Check that only input pins are found when iterating through inputs
		for (auto pin_id : block_input_pins(blk_id)) {
			auto port_id = pin_port(pin_id);


			auto type = port_type(port_id);
			if (type != PortType::INPUT) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-input pin in block input pins");
			}
		}

		//Check that only output pins are found when iterating through outputs
		for (auto pin_id : block_output_pins(blk_id)) {
			auto port_id = pin_port(pin_id);

			auto type = port_type(port_id);
			if (type != PortType::OUTPUT) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-output pin in block output pins");
			}
		}

		//Check that only clock pins are found when iterating through clocks
		for (auto pin_id : block_clock_pins(blk_id)) {
			auto port_id = pin_port(pin_id);

			auto type = port_type(port_id);
			if (type != PortType::CLOCK) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-clock pin in block clock pins");
			}
		}
	}
	return true;
}

bool BaseNetlist::validate_port_pin_refs() const {
	//Check that port <-> pin references are consistent

	//Track how many times we've seen each pin from the ports
	vtr::vector_map<PinId, unsigned> seen_pin_ids(pin_ids_.size());

	for (auto port_id : port_ids_) {
		bool first_bit = true;
		BitIndex prev_bit_index = 0;
		for (auto pin_id : port_pins(port_id)) {
			if (pin_port(pin_id) != port_id) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Port-pin cross-reference does not match");
			}
			if (pin_port_bit(pin_id) >= port_width(port_id)) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Out-of-range port bit index");
			}
			++seen_pin_ids[pin_id];

			BitIndex port_bit_index = pin_port_bit(pin_id);

			//Verify that the port bit index is legal
			if (!valid_port_bit(port_id, port_bit_index)) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid pin bit index in port");
			}

			//Verify that the pins are listed in increasing order of port bit index,
			//we rely on this property to perform fast binary searches for pins with specific bit
			//indicies
			if (first_bit) {
				prev_bit_index = port_bit_index;
				first_bit = false;
			}
			else if (prev_bit_index >= port_bit_index) {
				VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Port pin indicies are not in ascending order");
			}
		}
	}

	//Check that we have neither orphaned pins (i.e. that aren't referenced by a port) 
	//nor shared pins (i.e. referenced by multiple ports)
	if (!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(),
		[](unsigned val) {
		return val == 1;
	})) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Pins referenced by zero or multiple ports");
	}

	if (std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned pins (not referenced by a port)");
	}

	return true;
}

bool BaseNetlist::validate_net_pin_refs() const {
	//Check that net <-> pin references are consistent

	//Track how many times we've seen each pin from the ports
	vtr::vector_map<PinId, unsigned> seen_pin_ids(pin_ids_.size());

	for (auto net_id : nets()) {
		auto pin_range = net_pins(net_id);
		for (auto iter = pin_range.begin(); iter != pin_range.end(); ++iter) {
			auto pin_id = *iter;
			if (iter != pin_range.begin()) {
				//The first net pin is the driver, which may be invalid
				//if there is no driver. So we only check for a valid id
				//on the other net pins (which are all sinks and must be valid)
				if (!pin_id) {
					VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid pin found in net sinks");
				}
			}

			if (pin_id) {
				//Verify the cross reference if the pin_id is valid (i.e. a sink or a valid driver)
				if (pin_net(pin_id) != net_id) {
					VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Net-pin cross-reference does not match");
				}

				//We only record valid seen pins since we may see multiple undriven nets with invalid IDs
				++seen_pin_ids[pin_id];
			}
		}
	}

	//Check that we have niether orphaned pins (i.e. that aren't referenced by a port) 
	//nor shared pins (i.e. referenced by multiple ports)
	if (!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(),
		[](unsigned val) {
		return val == 1;
	})) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found pins referenced by zero or multiple nets");
	}

	if (std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned pins (not referenced by a net)");
	}

	return true;
}

bool BaseNetlist::validate_string_refs() const {
	for (const auto& str_id : block_names_) {
		if (!valid_string_id(str_id)) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid block name string reference");
		}
	}
	for (const auto& str_id : port_names_) {
		if (!valid_string_id(str_id)) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid port name string reference");
		}
	}
	for (const auto& str_id : net_names_) {
		if (!valid_string_id(str_id)) {
			VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid net name string reference");
		}
	}
	return true;
}

/*
*
* Internal utilities
*
*/
BaseNetlist::StringId BaseNetlist::find_string(const std::string& str) const {
	auto iter = string_to_string_id_.find(str);
	if (iter != string_to_string_id_.end()) {
		StringId str_id = iter->second;

		VTR_ASSERT(str_id);
		VTR_ASSERT(strings_[str_id] == str);

		return str_id;
	}
	else {
		return StringId::INVALID();
	}
}

BlockId BaseNetlist::find_block(const StringId name_id) const {
	VTR_ASSERT(valid_string_id(name_id));
	auto iter = block_name_to_block_id_.find(name_id);
	if (iter != block_name_to_block_id_.end()) {
		BlockId blk_id = *iter;

		//Check post-conditions
		if (blk_id) {
			VTR_ASSERT(valid_block_id(blk_id));
			VTR_ASSERT(block_names_[blk_id] == name_id);
		}

		return blk_id;
	}
	else {
		return BlockId::INVALID();
	}
}

void BaseNetlist::associate_pin_with_block(const PinId pin_id, const PortType type, const BlockId blk_id) {
	auto port_id = pin_port(pin_id);
	VTR_ASSERT(port_type(port_id) == type);

	//Get an interator pointing to where we want to insert
	pin_iterator iter;
	if (type == PortType::INPUT) {
		iter = block_input_pins(blk_id).end();
		++block_num_input_pins_[blk_id];
	}
	else if (type == PortType::OUTPUT) {
		iter = block_output_pins(blk_id).end();
		++block_num_output_pins_[blk_id];
	}
	else {
		VTR_ASSERT(type == PortType::CLOCK);
		iter = block_clock_pins(blk_id).end();
		++block_num_clock_pins_[blk_id];
	}

	//Insert the element just before iter
	auto new_iter = block_pins_[blk_id].insert(iter, pin_id);

	//Inserted value should be the last valid range element
	if (type == PortType::INPUT) {
		VTR_ASSERT(new_iter + 1 == block_input_pins(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).begin());
	}
	else if (type == PortType::OUTPUT) {
		VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).begin());
	}
	else {
		VTR_ASSERT(type == PortType::CLOCK);
		VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_pins(blk_id).end());
	}
}

void BaseNetlist::associate_pin_with_net(const PinId pin_id, const PinType type, const NetId net_id) {
	//Add the pin to the net
	if (type == PinType::DRIVER) {
		VTR_ASSERT_MSG(net_pins_[net_id].size() > 0, "Must be space for net's pin");
		VTR_ASSERT_MSG(net_pins_[net_id][0] == PinId::INVALID(), "Must be no existing net driver");

		net_pins_[net_id][0] = pin_id; //Set driver
	}
	else {
		VTR_ASSERT(type == PinType::SINK);

		net_pins_[net_id].emplace_back(pin_id); //Add sink
	}
}

void BaseNetlist::associate_pin_with_port(const PinId pin_id, const PortId port_id) {
	port_pins_[port_id].push_back(pin_id);
}

void BaseNetlist::associate_port_with_block(const PortId port_id, const BlockId blk_id) {
	//Associate the port with the blocks inputs/outputs/clocks
	PortType type = port_type(port_id);

	//Get an interator pointing to where we want to insert
	port_iterator iter;
	if (type == PortType::INPUT) {
		iter = block_input_ports(blk_id).end();
		++block_num_input_ports_[blk_id];
	}
	else if (type == PortType::OUTPUT) {
		iter = block_output_ports(blk_id).end();
		++block_num_output_ports_[blk_id];
	}
	else {
		VTR_ASSERT(type == PortType::CLOCK);
		iter = block_clock_ports(blk_id).end();
		++block_num_clock_ports_[blk_id];
	}

	//Insert the element just before iter
	auto new_iter = block_ports_[blk_id].insert(iter, port_id);

	//Inserted value should be the last valid range element
	if (type == PortType::INPUT) {
		VTR_ASSERT(new_iter + 1 == block_input_ports(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).begin());
	}
	else if (type == PortType::OUTPUT) {
		VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).begin());
	}
	else {
		VTR_ASSERT(type == PortType::CLOCK);
		VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).end());
		VTR_ASSERT(new_iter + 1 == block_ports(blk_id).end());
	}
}

BaseNetlist::StringId BaseNetlist::create_string(const std::string& str) {
	StringId str_id = find_string(str);
	if (!str_id) {
		//Not found, create

		//Reserve an id
		str_id = StringId(string_ids_.size());
		string_ids_.push_back(str_id);

		//Store the reverse look-up
		auto key = str;
		string_to_string_id_[key] = str_id;

		//Initialize the data
		strings_.emplace_back(str);
	}

	//Check post-conditions: sizes
	VTR_ASSERT(string_to_string_id_.size() == string_ids_.size());
	VTR_ASSERT(strings_.size() == string_ids_.size());

	//Check post-conditions: values
	VTR_ASSERT(strings_[str_id] == str);
	VTR_ASSERT_SAFE(find_string(str) == str_id);

	return str_id;
}


NetId BaseNetlist::find_net(const StringId name_id) const {
	VTR_ASSERT(valid_string_id(name_id));
	auto iter = net_name_to_net_id_.find(name_id);
	if (iter != net_name_to_net_id_.end()) {
		NetId net_id = *iter;

		if (net_id) {
			//Check post-conditions
			VTR_ASSERT(valid_net_id(net_id));
			VTR_ASSERT(net_names_[net_id] == name_id);
		}

		return net_id;
	}
	else {
		return NetId::INVALID();
	}
}