#include <algorithm>
#include <numeric>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"

/*
*
* Netlist class implementation
*
*/

template<typename BlockId, typename PortId, typename PinId, typename NetId>
Netlist<BlockId, PortId, PinId, NetId>::Netlist(std::string name, std::string id)
	: netlist_name_(name)
	, netlist_id_(id) 
	, dirty_(false) {}

/*
*
* Netlist
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::netlist_name() const {
	return netlist_name_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::netlist_id() const {
	return netlist_id_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::is_dirty() const {
	return dirty_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::is_compressed() const {
	return !is_dirty();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::print_stats() const {
	vtr::printf_info("Blocks  %zu capacity/size: %.2f\n", block_ids_.size(), float(block_ids_.capacity()) / block_ids_.size());
	vtr::printf_info("Ports   %zu capacity/size: %.2f\n", port_ids_.size(), float(port_ids_.capacity()) / port_ids_.size());
	vtr::printf_info("Pins    %zu capacity/size: %.2f\n", pin_ids_.size(), float(pin_ids_.capacity()) / pin_ids_.size());
	vtr::printf_info("Nets    %zu capacity/size: %.2f\n", net_ids_.size(), float(net_ids_.capacity()) / net_ids_.size());
	vtr::printf_info("Strings %zu capacity/size: %.2f\n", string_ids_.size(), float(string_ids_.capacity()) / string_ids_.size());
}

/*
*
* Blocks
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::block_name(const BlockId id) const {
	StringId str_id = block_names_[id];
	return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::block_is_combinational(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_clock_pins(id).size() == 0;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return vtr::make_range(block_pins_[id].begin(), block_pins_[id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_input_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin();

	auto end = block_pins_[id].begin() + block_num_input_pins_[id];

	return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_output_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin() + block_num_input_pins_[id];

	auto end = begin + block_num_output_pins_[id];

	return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_clock_pins(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_pins_[id].begin()
		+ block_num_input_pins_[id]
		+ block_num_output_pins_[id];

	auto end = begin + block_num_clock_pins_[id];

	VTR_ASSERT(end == block_pins_[id].end());

	return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return vtr::make_range(block_ports_[id].begin(), block_ports_[id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_input_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_ports_[id].begin();

	auto end = block_ports_[id].begin() + block_num_input_ports_[id];

	return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_output_ports(const BlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	auto begin = block_ports_[id].begin() + block_num_input_ports_[id];

	auto end = begin + block_num_output_ports_[id];

	return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_clock_ports(const BlockId id) const {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::port_name(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	StringId str_id = port_names_[id];
	return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::port_block(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return port_blocks_[id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::port_pins(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return vtr::make_range(port_pins_[id].begin(), port_pins_[id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::port_pin(const PortId port_id, const BitIndex port_bit) const {
	//Convenience look-up bypassing port
	return find_pin(port_id, port_bit);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::port_net(const PortId port_id, const BitIndex port_bit) const {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BitIndex Netlist<BlockId, PortId, PinId, NetId>::port_width(const PortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return port_widths_[id];
}

/*
*
* Pins
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
std::string Netlist<BlockId, PortId, PinId, NetId>::pin_name(const PinId id) const {
	BlockId blk = pin_block(id);
	PortId port = pin_port(id);

	return block_name(blk) + "." + port_name(port) + "[" + std::to_string(pin_port_bit(id)) + "]";
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::pin_net (const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_nets_[id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::pin_port(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_ports_[id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BitIndex Netlist<BlockId, PortId, PinId, NetId>::pin_port_bit(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_port_bits_[id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::pin_block(const PinId id) const {
	//Convenience lookup bypassing the port
	VTR_ASSERT(valid_pin_id(id));

	PortId port_id = pin_port(id);
	return port_block(port_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::pin_is_constant(const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_is_constant_[id];
}

/*
*
* Nets
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::net_name(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	StringId str_id = net_names_[id];
	return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::net_pins(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return vtr::make_range(net_pins_[id].begin(), net_pins_[id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::net_driver(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	if (net_pins_[id].size() > 0) {
		return net_pins_[id][0];
	}
	else {
		return PinId::INVALID();
	}
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::net_driver_block(const NetId id) const {
	auto driver_pin_id = net_driver(id);
	if (driver_pin_id) {
		return pin_block(driver_pin_id);
	}
	return BlockId::INVALID();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::net_sinks(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return vtr::make_range(++net_pins_[id].begin(), net_pins_[id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::net_is_constant(const NetId id) const {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::block_range Netlist<BlockId, PortId, PinId, NetId>::blocks() const {
	return vtr::make_range(block_ids_.begin(), block_ids_.end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::pins() const {
	return vtr::make_range(pin_ids_.begin(), pin_ids_.end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::net_range Netlist<BlockId, PortId, PinId, NetId>::nets() const {
	return vtr::make_range(net_ids_.begin(), net_ids_.end());
}

/*
*
* Lookups
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::find_block(const std::string& name) const {
	auto str_id = find_string(name);
	if (!str_id) {
		return BlockId::INVALID();
	}
	else {
		return find_block(str_id);
	}
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::find_port(const BlockId blk_id, const std::string& name) const {
	VTR_ASSERT(valid_block_id(blk_id));

	//Since we only know the port name, we must search all the ports
	for (auto port_id : block_ports(blk_id)) {
		if (port_name(port_id) == name) {
			return port_id;
		}
	}
	return PortId::INVALID();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::find_net(const std::string& name) const {
	auto str_id = find_string(name);
	if (!str_id) {
		return NetId::INVALID();
	}
	else {
		return find_net(str_id);
	}
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::find_pin(const PortId port_id, BitIndex port_bit) const {
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
/*bool Netlist::verify() const {
	bool valid = true;

	//Verify data structure consistency
	valid &= verify_sizes();
	valid &= verify_refs();
	valid &= verify_lookups();

	//Verify logical consistency
	valid &= verify_block_invariants();

	return valid;
}*/

//Checks that all cross-references are consistent.
//Should take linear time.
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_refs() const {
	bool valid = true;
	valid &= validate_block_port_refs();
	valid &= validate_net_pin_refs();
	valid &= validate_string_refs();
	return valid;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_lookups() const {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::create_block(const std::string name) {
    //Must have a non-empty name
    VTR_ASSERT_MSG(!name.empty(), "Non-Empty block name");

    //Check if the block has already been created
    StringId name_id = create_string(name);
    BlockId blk_id = find_block(name_id);

    if(blk_id == BlockId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = BlockId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name_id);

        //Initialize the look-ups
        block_name_to_block_id_.insert(name_id, blk_id);

        block_pins_.emplace_back();
        block_num_input_pins_.push_back(0);
        block_num_output_pins_.push_back(0);
        block_num_clock_pins_.push_back(0);

        block_ports_.emplace_back();
        block_num_input_ports_.push_back(0);
        block_num_output_ports_.push_back(0);
        block_num_clock_ports_.push_back(0);
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_block_sizes());

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_name(blk_id) == name);
    VTR_ASSERT_SAFE(find_block(name) == blk_id);

    return blk_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::create_port(const BlockId blk_id, const std::string name, BitIndex width) {
	//Check pre-conditions
	VTR_ASSERT_MSG(valid_block_id(blk_id), "Valid block id");

	//See if the port already exists
	StringId name_id = create_string(name);
	PortId port_id = find_port(blk_id, name);
	if (!port_id) { //Not found, create it
					//Reserve an id
		port_id = PortId(port_ids_.size());
		port_ids_.push_back(port_id);

		//Initialize the per-port-instance data
		port_blocks_.push_back(blk_id);
		port_names_.push_back(name_id);
		port_widths_.push_back(width);

		//Allocate the pins, initialize to invalid Ids
		port_pins_.emplace_back();
	}

	//Check post-conditions: sizes
	VTR_ASSERT(validate_port_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_port_id(port_id));
	VTR_ASSERT(port_block(port_id) == blk_id);
	VTR_ASSERT(port_width(port_id) == width);

	return port_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::create_pin(const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType pin_type, const PortType port_type, bool is_const) {
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
		associate_pin_with_net(pin_id, pin_type, net_id);

		//Add the pin to the port
		associate_pin_with_port(pin_id, port_id);

		//Add the pin to the block
		associate_pin_with_block(pin_id, port_type, port_block(port_id));
	}

	//Check post-conditions: sizes
	VTR_ASSERT(validate_pin_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_pin_id(pin_id));
	VTR_ASSERT(pin_port(pin_id) == port_id);
	VTR_ASSERT(pin_port_bit(pin_id) == port_bit);
	VTR_ASSERT(pin_net(pin_id) == net_id);
	VTR_ASSERT(pin_is_constant(pin_id) == is_const);
	VTR_ASSERT_SAFE(find_pin(port_id, port_bit) == pin_id);

	return pin_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::create_net(const std::string name) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::add_net(const std::string name, PinId driver, std::vector<PinId> sinks) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_pin_is_constant(const PinId pin_id, const bool value) {
    VTR_ASSERT(valid_pin_id(pin_id));

    pin_is_constant_[pin_id] = value;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_pin_net(const PinId pin, PinType type, const NetId net) {
	VTR_ASSERT(valid_pin_id(pin));

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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_block(const BlockId blk_id) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_port(const PortId port_id) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_pin(const PinId pin_id) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_net(const NetId net_id) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_net_pin(const NetId net_id, const PinId pin_id) {
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

	//Disassociate the pin with the net
	if (valid_pin_id(pin_id)) {
		pin_nets_[pin_id] = NetId::INVALID();

		//Mark netlist dirty, since we are leaving an invalid net id
		dirty_ = true;
	}
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_unused() {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_block_refs(const vtr::vector_map<PinId, PinId>& pin_id_map,
	const vtr::vector_map<PortId, PortId>& port_id_map) {
	//Update the pin id references held by blocks
	for (auto blk_id : blocks()) {
		//Before update the references, we need to know how many are valid,
		//so we can also update the numbers of input/output/clock pins

		//Note that we take special care to not modify the pin counts until after
		//they have all been counted (since the block_*_pins() functions depend on
		//the counts).
		size_t num_input_pins = count_valid_refs(block_input_pins(blk_id), pin_id_map);
		size_t num_output_pins = count_valid_refs(block_output_pins(blk_id), pin_id_map);
		size_t num_clock_pins = count_valid_refs(block_clock_pins(blk_id), pin_id_map);

		std::vector<PinId>& pin_collection = block_pins_[blk_id];

		pin_collection = update_valid_refs(pin_collection, pin_id_map);
		block_num_input_pins_[blk_id] = num_input_pins;
		block_num_output_pins_[blk_id] = num_output_pins;
		block_num_clock_pins_[blk_id] = num_clock_pins;

		VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
		VTR_ASSERT(pin_collection.size() == (size_t)block_num_input_pins_[blk_id]
			+ block_num_output_pins_[blk_id]
			+ block_num_clock_pins_[blk_id]);

		//Similarily for ports
		size_t num_input_ports = count_valid_refs(block_input_ports(blk_id), port_id_map);
		size_t num_output_ports = count_valid_refs(block_output_ports(blk_id), port_id_map);
		size_t num_clock_ports = count_valid_refs(block_clock_ports(blk_id), port_id_map);

		std::vector<PortId>& ports = block_ports_[blk_id];

		ports = update_valid_refs(ports, port_id_map);
		block_num_input_ports_[blk_id] = num_input_ports;
		block_num_output_ports_[blk_id] = num_output_ports;
		block_num_clock_ports_[blk_id] = num_clock_ports;

		VTR_ASSERT_SAFE_MSG(all_valid(ports), "All Ids should be valid");
		VTR_ASSERT(ports.size() == (size_t)block_num_input_ports_[blk_id]
			+ block_num_output_ports_[blk_id]
			+ block_num_clock_ports_[blk_id]);
	}

	VTR_ASSERT(validate_block_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_port_refs(const vtr::vector_map<BlockId, BlockId>& block_id_map,
	const vtr::vector_map<PinId, PinId>& pin_id_map) {
	//Update block and pin references held by ports
	port_blocks_ = update_valid_refs(port_blocks_, block_id_map);
	VTR_ASSERT_SAFE_MSG(all_valid(port_blocks_), "All Ids should be valid");

	VTR_ASSERT(port_blocks_.size() == port_ids_.size());

	for (auto& pin_collection : port_pins_) {
		pin_collection = update_valid_refs(pin_collection, pin_id_map);
		VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
	}
	VTR_ASSERT(validate_port_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_pin_refs(const vtr::vector_map<PortId, PortId>& port_id_map,
	const vtr::vector_map<NetId, NetId>& net_id_map) {
	//Update port and net references held by pins
	pin_ports_ = update_all_refs(pin_ports_, port_id_map);
	VTR_ASSERT_SAFE_MSG(all_valid(pin_ports_), "All Ids should be valid");

	pin_nets_ = update_all_refs(pin_nets_, net_id_map);
	VTR_ASSERT_SAFE_MSG(all_valid(pin_nets_), "All Ids should be valid");

	VTR_ASSERT(validate_pin_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_net_refs(const vtr::vector_map<PinId, PinId>& pin_id_map) {
	//Update pin references held by nets
	for (auto& pin_collection : net_pins_) {
		pin_collection = update_valid_refs(pin_collection, pin_id_map);

		VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All sinks should be valid");
	}
	VTR_ASSERT(validate_net_sizes());
}




/*
*
* Sanity Checks
*
*/
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_block_id(BlockId id) const {
	if (id == BlockId::INVALID()) return false;
	else if (!block_ids_.contains(id)) return false;
	else if (block_ids_[id] != id) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_port_id(PortId id) const {
	if (id == PortId::INVALID()) return false;
	else if (!port_ids_.contains(id)) return false;
	else if (port_ids_[id] != id) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_port_bit(PortId id, BitIndex port_bit) const {
	VTR_ASSERT(valid_port_id(id));
	if (port_bit >= port_width(id)) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_pin_id(PinId id) const {
	if (id == PinId::INVALID()) return false;
	else if (!pin_ids_.contains(id)) return false;
	else if (pin_ids_[id] != id) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_net_id(NetId id) const {
	if (id == NetId::INVALID()) return false;
	else if (!net_ids_.contains(id)) return false;
	else if (net_ids_[id] != id) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_string_id(Netlist<BlockId, PortId, PinId, NetId>::StringId id) const {
	if (id == StringId::INVALID()) return false;
	else if (!string_ids_.contains(id)) return false;
	else if (string_ids_[id] != id) return false;
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_block_sizes() const {
	if (block_names_.size() != block_ids_.size()
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_port_sizes() const {
	if (port_names_.size() != port_ids_.size()
		|| port_blocks_.size() != port_ids_.size()
		|| port_pins_.size() != port_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent port data sizes");
	}
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_pin_sizes() const {
	if (pin_ports_.size() != pin_ids_.size()
		|| pin_port_bits_.size() != pin_ids_.size()
		|| pin_nets_.size() != pin_ids_.size()
		|| pin_is_constant_.size() != pin_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent pin data sizes");
	}
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_net_sizes() const {
	if (net_names_.size() != net_ids_.size()
		|| net_pins_.size() != net_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent net data sizes");
	}
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_string_sizes() const {
	if (strings_.size() != string_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent string data sizes");
	}
	return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_block_port_refs() const {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_net_pin_refs() const {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_string_refs() const {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::StringId Netlist<BlockId, PortId, PinId, NetId>::find_string(const std::string& str) const {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::find_block(const Netlist<BlockId, PortId, PinId, NetId>::StringId name_id) const {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_block(const PinId pin_id, const PortType type, const BlockId blk_id) {

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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_net(const PinId pin_id, const PinType type, const NetId net_id) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_port(const PinId pin_id, const PortId port_id) {
	port_pins_[port_id].push_back(pin_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_port_with_block(const PortId port_id, const PortType type, const BlockId blk_id) {
	//Associate the port with the blocks inputs/outputs/clocks

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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::StringId Netlist<BlockId, PortId, PinId, NetId>::create_string(const std::string& str) {
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

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::find_net(const Netlist<BlockId, PortId, PinId, NetId>::StringId name_id) const {
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