#include <algorithm>
#include <unordered_set>
#include <queue>
#include <numeric>

#include "atom_netlist.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"


/*
 *
 *
 * AtomNetlist Class Implementation
 *
 *
 */
AtomNetlist::AtomNetlist(std::string name, std::string id)
    : Netlist<AtomBlockId, AtomPortId, AtomPinId, AtomNetId>(name, id) {}

/*
 *
 * Blocks
 *
 */
AtomBlockType AtomNetlist::block_type (const AtomBlockId id) const {
	const t_model* blk_model = block_model(id);

	AtomBlockType type = AtomBlockType::BLOCK;
	if (blk_model->name == std::string("input")) {
		type = AtomBlockType::INPAD;
	}
	else if (blk_model->name == std::string("output")) {
		type = AtomBlockType::OUTPAD;
	}
	else {
		type = AtomBlockType::BLOCK;
	}
	return type;
}

const t_model* AtomNetlist::block_model(const AtomBlockId id) const {
	VTR_ASSERT(valid_block_id(id));

	return block_models_[id];
}

const AtomNetlist::TruthTable& AtomNetlist::block_truth_table (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return block_truth_tables_[id];
}

/*
 *
 * Ports
 *
 */
const t_model_ports* AtomNetlist::port_model(const AtomPortId id) const {
	VTR_ASSERT(valid_port_id(id));

	return port_models_[id];
}


/*
 *
 * Pins
 *
 */
PinType AtomNetlist::pin_type(const AtomPinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	AtomPortId port_id = pin_port(id);
	PinType type;
	switch (port_type(port_id)) {
		case PortType::INPUT: /*fallthrough */;
		case PortType::CLOCK: type = PinType::SINK; break;
		case PortType::OUTPUT: type = PinType::DRIVER; break;
		default: VTR_ASSERT_MSG(false, "Valid atom port type");
	}
	return type;
}


/*
*
* Lookups
*
*/
AtomPortId AtomNetlist::find_atom_port(const AtomBlockId blk_id, const t_model_ports* model_port) const {
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

	return AtomPortId::INVALID();
}

/*
 *
 * Validation
 *
 */
bool AtomNetlist::verify() const {
    bool valid = true;

    //Verify data structure consistency
    valid &= verify_sizes();
    valid &= verify_refs();
    valid &= verify_lookups();

    //Verify logical consistency
    valid &= verify_block_invariants();

    return valid;
}

//Checks that the sizes of internal data structures
//are consistent. Should take constant time.
bool AtomNetlist::verify_sizes() const {
    bool valid = true;
    valid &= validate_block_sizes();
    valid &= validate_port_sizes();
    valid &= validate_pin_sizes();
    valid &= validate_net_sizes();
    valid &= validate_string_sizes();
    return valid;
}

bool AtomNetlist::verify_block_invariants() const {
    for(auto blk_id : blocks()) {

        {
            //Check block type

            //Find any connected clock
            AtomNetId clk_net_id;
            for(auto pin_id : block_clock_pins(blk_id)) {
                if(pin_id) {
                    clk_net_id = pin_net(pin_id);
                    break;
                }
            }

            if(block_is_combinational(blk_id)) {
                //Non-sequential types must not have a clock
                if(clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is a non-sequential type but has a clock '%s'", 
                              block_name(blk_id).c_str(), net_name(clk_net_id).c_str());
                }

            } else {
                //Sequential types must have a clock
                if(!clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is sequential type but has no clock", 
                              block_name(blk_id).c_str());
                }

            }
        }
        {
            //Check that block pins and ports are consistent (i.e. same number of pins)
            size_t num_block_pins = block_pins(blk_id).size();

            size_t total_block_port_pins = 0;
            for(auto port_id : block_ports(blk_id)) {
                total_block_port_pins += port_pins(port_id).size(); 
            }

            if(num_block_pins != total_block_port_pins) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block pins and port pins do not match on atom block '%s'",
                        block_name(blk_id).c_str());
            }
        }
    }
    return true;
}

/*
 *
 * Mutators
 *
 */
AtomBlockId AtomNetlist::create_block(const std::string name, const t_model* model, const TruthTable truth_table) {
	AtomBlockId blk_id = Netlist::create_block(name);
	
    //Initialize the data
	block_models_.push_back(model);
    block_truth_tables_.push_back(truth_table);

	//Check post-conditions: size
	VTR_ASSERT(validate_block_sizes());

	//Check post-conditions: values
	VTR_ASSERT(block_model(blk_id) == model);
    VTR_ASSERT(block_truth_table(blk_id) == truth_table);

    return blk_id;
}

AtomPortId AtomNetlist::create_port(const AtomBlockId blk_id, const t_model_ports* model_port) {
	AtomPortId port_id = find_port(blk_id, model_port->name);

	PortType type = PortType::INPUT;
	//Determine the PortType
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

	if (!port_id) {
		port_id = Netlist::create_port(blk_id, model_port->name, model_port->size, type);

		port_models_.push_back(model_port);
		associate_port_with_block(port_id, port_type(port_id), blk_id);
	}

	//Check post-conditions: size
	VTR_ASSERT(validate_port_sizes());

	//Check post-conditions: values
	VTR_ASSERT(port_name(port_id) == model_port->name);
	VTR_ASSERT(port_width(port_id) == (unsigned)model_port->size);
	VTR_ASSERT(port_model(port_id) == model_port);
	VTR_ASSERT(port_type(port_id) == type);
	VTR_ASSERT_SAFE(find_port(blk_id, model_port->name) == port_id);
	VTR_ASSERT_SAFE(find_atom_port(blk_id, model_port) == port_id);

	return port_id;
}

AtomPinId AtomNetlist::create_pin(const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const PinType pin_type_, bool is_const) {
	AtomPinId pin_id = Netlist::create_pin(port_id, port_bit, net_id, pin_type_, is_const);
	
	//Check post-conditions: size
	VTR_ASSERT(validate_pin_sizes());

	VTR_ASSERT(pin_type(pin_id) == pin_type_);
	VTR_ASSERT(pin_port(pin_id) == port_id);
	VTR_ASSERT(pin_port_type(pin_id) == port_type(port_id));

	return pin_id;
}

AtomNetId AtomNetlist::create_net(const std::string name) {
	AtomNetId net_id = Netlist::create_net(name);

	//Check post-conditions: size
	VTR_ASSERT(validate_net_sizes());

	return net_id;
}

AtomNetId AtomNetlist::add_net(const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks) {
	return Netlist::add_net(name, driver, sinks);
}

void AtomNetlist::remove_block_impl(const AtomBlockId blk_id) {
	VTR_ASSERT(blk_id);
}

void AtomNetlist::remove_port_impl(const AtomPortId port_id) {
	VTR_ASSERT(port_id);
}

void AtomNetlist::remove_pin_impl(const AtomPinId pin_id) {
	VTR_ASSERT(pin_id);
}

void AtomNetlist::remove_net_impl(const AtomNetId net_id) {
	VTR_ASSERT(net_id);
}

/*
*
* Internal utilities
*
*/

void AtomNetlist::clean_blocks_impl(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map) {
    //Update all the block_models and block
    block_models_ = clean_and_reorder_values(block_models_, block_id_map);
    block_truth_tables_ = clean_and_reorder_values(block_truth_tables_, block_id_map);
}

void AtomNetlist::clean_ports_impl(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map) {
    //Update all port_models_ values
    port_models_ = clean_and_reorder_values(port_models_, port_id_map);
}

void AtomNetlist::clean_pins_impl(const vtr::vector_map<AtomPinId, AtomPinId>& pin_id_map) {
	unsigned unused(pin_id_map.size()); //Remove unused parameter warning
	unused = unused;
}

void AtomNetlist::clean_nets_impl(const vtr::vector_map<AtomNetId, AtomNetId>& net_id_map) {
	unsigned unused(net_id_map.size()); //Remove unused parameter warning
	unused = unused;
}

void AtomNetlist::shrink_to_fit_impl() {
    //Block data
    block_models_.shrink_to_fit();

    //Port data
    port_models_.shrink_to_fit();
}

/*
 *
 * Sanity Checks
 *
 */
bool AtomNetlist::validate_block_sizes_impl() const {
	if (block_truth_tables_.size() != block_ids_.size()
		|| block_models_.size() != block_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent block data sizes");
    }

	return true;
}

bool AtomNetlist::validate_port_sizes_impl() const {
	if (port_models_.size() != port_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent port data sizes");
	}
	return true;
}

bool AtomNetlist::validate_pin_sizes_impl() const {
	return true;
}

bool AtomNetlist::validate_net_sizes_impl() const {
	return true;
}

bool AtomNetlist::validate_block_pin_refs() const {
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

bool AtomNetlist::validate_port_pin_refs() const {
	//Check that port <-> pin references are consistent

	//Track how many times we've seen each pin from the ports
	vtr::vector_map<AtomPinId, unsigned> seen_pin_ids(pin_ids_.size());

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

/*
*
* Validation
*
*/
//Checks that all cross-references are consistent.
//Should take linear time.
bool AtomNetlist::verify_refs() const {
	return Netlist::verify_refs() 
		& validate_port_pin_refs()
		& validate_block_pin_refs();
}
