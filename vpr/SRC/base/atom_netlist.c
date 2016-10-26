#include <algorithm>
#include <unordered_set>

#include "atom_netlist.h"

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"
/*
 *
 *
 * Utility templates
 *
 *
 */

template<typename T>
bool are_contiguous(std::vector<T>& values) {
    for(size_t i = 0; i < values.size(); ++i) {
        if (size_t(values[i]) != i) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool all_valid(std::vector<T>& values) {
    for(size_t i = 0; i < values.size(); ++i) {
        if(!values[i]) {
            return false;
        }
    }
    return true;
}

//Builds a mapping from old to new ids (i.e. map[old_id] == new_id)
// Used during netlist compression
template<typename T>
std::pair<std::vector<T>,std::vector<T>> compress_ids(const std::vector<T>& ids) {
    std::vector<T> id_map;
    std::vector<T> new_ids;

    //When we compress the netlist the index of an existing
    //valid value is decremented by the number of invalids that
    //were removed before it (i.e. everything is shifted to the left)
    //So we can build up this mapping by walking through all the ids
    //and only incrementing when we have a valid id

    size_t i = 0;
    for(auto id : ids) {
        if(id) {
            //Valid
            id_map.emplace_back(i); 
            new_ids.emplace_back(i);
            ++i;
        } else {
            //Invalid
            id_map.emplace_back(T::INVALID());
        }
    }

    VTR_ASSERT_SAFE(std::all_of(id_map.begin(), id_map.end(),
        [&](T val) {
            return size_t(val) < i || val == T::INVALID(); 
    }));

    VTR_ASSERT_SAFE(all_valid(new_ids));
    VTR_ASSERT_SAFE(are_contiguous(new_ids));

    return {new_ids, id_map};
}

//Moves the elements in values to a copy which is returned, if the value of corresponding predicate entry evaluates true.
// Note: this changes values!
template<typename T, typename I>
std::vector<T> move_valid(std::vector<T>& values, const std::vector<I>& pred) {
    VTR_ASSERT(values.size() == pred.size());
    std::vector<T> copy;
    for(size_t i = 0; i < values.size(); ++i) {
        if (pred[i]) {
            copy.emplace_back(std::move(values[i]));
        }
    }
    return copy;
}

//Updates values based on id_map, even if the original/new mappings are not valid
template<typename T>
std::vector<T> update_all_refs(const std::vector<T>& values, const std::vector<T>& id_map) {
    std::vector<T> updated;

    for(size_t i = 0; i < values.size(); ++i) {
        //The original item was valid
        auto new_val = id_map[size_t(values[i])]; 
        //The original item exists in the new mapping
        updated.emplace_back(new_val);
    }
    return updated;
}

//Updates values based on id_map, only if the originalx and new mappings are valid
// any invalid values are not included
template<typename T>
std::vector<T> update_valid_refs(const std::vector<T>& values, const std::vector<T>& id_map) {
    std::vector<T> updated;

    for(size_t i = 0; i < values.size(); ++i) {
        if(values[i]) {
            //The original item was valid
            auto new_val = id_map[size_t(values[i])]; 
            if(new_val) {
                //The original item exists in the new mapping
                updated.emplace_back(new_val);
            }
        }
    }
    return updated;
}

template<typename R, typename T>
size_t count_valid_refs(R range, const std::vector<T>& id_map) {
    size_t valid_count = 0;
    for(auto val : range) {
        if(id_map[size_t(val)]) {
            ++valid_count;
        }
    }

    return valid_count;
}

/*
 *
 *
 * AtomNetlit Class Implementation
 *
 *
 */

AtomNetlist::AtomNetlist(std::string name)
    : netlist_name_(name)
    , dirty_(false) {}

/*
 *
 * Netlist
 *
 */
const std::string& AtomNetlist::netlist_name() const {
    return netlist_name_;
}

bool AtomNetlist::dirty() const {
    return dirty_;
}

void AtomNetlist::print_stats() const {
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
const std::string& AtomNetlist::block_name (const AtomBlockId id) const { 
    AtomStringId str_id = block_names_[size_t(id)];
    return strings_[size_t(str_id)];
}

AtomBlockType AtomNetlist::block_type (const AtomBlockId id) const {
    const t_model* blk_model = block_model(id);

    AtomBlockType type = AtomBlockType::COMBINATIONAL;
    if(blk_model->name == std::string("input")) {
        type = AtomBlockType::INPAD;
    } else if(blk_model->name == std::string("output")) {
        type = AtomBlockType::OUTPAD;
    } else {
        //Determine if combinational or sequential.
        // We loop through the inputs looking for clocks
        const t_model_ports* port = blk_model->inputs;
        size_t clk_count = 0;

        while(port) {
            if(port->is_clock) {
                ++clk_count;
            }
            port = port->next;
        }

        if(clk_count == 0) {
            type = AtomBlockType::COMBINATIONAL;
        } else if (clk_count == 1) {
            type = AtomBlockType::SEQUENTIAL;
        } else {
            VTR_ASSERT(clk_count > 1);
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Primitive '%s' has multiple clocks (currently unsupported)",
                      blk_model->name);
        }
    }
    return type;
}

const t_model* AtomNetlist::block_model (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return block_models_[size_t(id)];
}

const AtomNetlist::TruthTable& AtomNetlist::block_truth_table (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return block_truth_tables_[size_t(id)];
}

AtomNetlist::pin_range AtomNetlist::block_pins (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return vtr::make_range(block_pins_[size_t(id)].begin(), block_pins_[size_t(id)].end()); 
}

AtomNetlist::pin_range AtomNetlist::block_input_pins (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_pins_[size_t(id)].begin();

    auto end = block_pins_[size_t(id)].begin() + block_num_input_pins_[size_t(id)];

    return vtr::make_range(begin, end); 
}

AtomNetlist::pin_range AtomNetlist::block_output_pins (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_pins_[size_t(id)].begin() + block_num_input_pins_[size_t(id)];

    auto end = begin + block_num_output_pins_[size_t(id)];

    return vtr::make_range(begin, end); 
}

AtomNetlist::pin_range AtomNetlist::block_clock_pins (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_pins_[size_t(id)].begin()
                 + block_num_input_pins_[size_t(id)]
                 + block_num_output_pins_[size_t(id)];

    auto end = begin + block_num_clock_pins_[size_t(id)];

    VTR_ASSERT(end == block_pins_[size_t(id)].end());

    return vtr::make_range(begin, end); 
}

AtomNetlist::port_range AtomNetlist::block_ports (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return vtr::make_range(block_ports_[size_t(id)].begin(), block_ports_[size_t(id)].end()); 
}

AtomNetlist::port_range AtomNetlist::block_input_ports (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_ports_[size_t(id)].begin();

    auto end = block_ports_[size_t(id)].begin() + block_num_input_ports_[size_t(id)];

    return vtr::make_range(begin, end); 
}

AtomNetlist::port_range AtomNetlist::block_output_ports (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_ports_[size_t(id)].begin() + block_num_input_ports_[size_t(id)];

    auto end = begin + block_num_output_ports_[size_t(id)];

    return vtr::make_range(begin, end); 
}

AtomNetlist::port_range AtomNetlist::block_clock_ports (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    auto begin = block_ports_[size_t(id)].begin()
                 + block_num_input_ports_[size_t(id)]
                 + block_num_output_ports_[size_t(id)];

    auto end = begin + block_num_clock_ports_[size_t(id)];

    VTR_ASSERT(end == block_ports_[size_t(id)].end());

    return vtr::make_range(begin, end); 
}

/*
 *
 * Ports
 *
 */
const std::string& AtomNetlist::port_name (const AtomPortId id) const {
    VTR_ASSERT(valid_port_id(id));

    AtomStringId str_id = port_names_[size_t(id)];
    return strings_[size_t(str_id)];
}

BitIndex AtomNetlist::port_width (const AtomPortId id) const {
    return port_model(id)->size;
}

AtomPortType AtomNetlist::port_type (const AtomPortId id) const {
    VTR_ASSERT(valid_port_id(id));

    const t_model_ports* model_port = port_model(id);

    AtomPortType type;
    if(model_port->dir == IN_PORT) {
        if(model_port->is_clock) {
            type = AtomPortType::CLOCK;
        } else {
            type = AtomPortType::INPUT;
        }
    } else if(model_port->dir == OUT_PORT) {
        type = AtomPortType::OUTPUT;
    } else {
        VTR_ASSERT_MSG(false, "Recognized model port type");
    }
    return type;
}

AtomBlockId AtomNetlist::port_block (const AtomPortId id) const {
    VTR_ASSERT(valid_port_id(id));

    return port_blocks_[size_t(id)];
}

AtomNetlist::pin_range AtomNetlist::port_pins (const AtomPortId id) const {
    VTR_ASSERT(valid_port_id(id));

    return vtr::make_range(port_pins_[size_t(id)].begin(), port_pins_[size_t(id)].end());
}

AtomPinId AtomNetlist::port_pin (const AtomPortId port_id, const BitIndex port_bit) const {
    //Convenience look-up bypassing port
    return find_pin(port_id, port_bit);
}

AtomNetId AtomNetlist::port_net (const AtomPortId port_id, const BitIndex port_bit) const {
    //port_pin() will validate that port_bit and port_id are valid so don't 
    //check redundently here

    //Convenience look-up bypassing port and pin
    AtomPinId pin_id = port_pin(port_id, port_bit);
    if(pin_id) {
        return pin_net(pin_id);
    } else {
        return AtomNetId::INVALID();
    }
}


const t_model_ports* AtomNetlist::port_model (const AtomPortId port_id) const {
    VTR_ASSERT(valid_port_id(port_id));

    return port_models_[size_t(port_id)];
}


/*
 *
 * Pins
 *
 */
AtomNetId AtomNetlist::pin_net (const AtomPinId id) const { 
    VTR_ASSERT(valid_pin_id(id));

    return pin_nets_[size_t(id)];
}

AtomPinType AtomNetlist::pin_type (const AtomPinId id) const { 
    VTR_ASSERT(valid_pin_id(id));

    AtomPortId port_id = pin_port(id);
    AtomPinType type;
    switch(port_type(port_id)) {
        case AtomPortType::INPUT: /*fallthrough */;
        case AtomPortType::CLOCK: type = AtomPinType::SINK; break;
        case AtomPortType::OUTPUT: type = AtomPinType::DRIVER; break;
        default: VTR_ASSERT_MSG(false, "Valid atom port type");
    }
    return type;
}

AtomPortId AtomNetlist::pin_port (const AtomPinId id) const { 
    VTR_ASSERT(valid_pin_id(id));

    return pin_ports_[size_t(id)];
}

BitIndex AtomNetlist::pin_port_bit(const AtomPinId id) const {
    VTR_ASSERT(valid_pin_id(id));

    return pin_port_bits_[size_t(id)];
}

AtomBlockId AtomNetlist::pin_block (const AtomPinId id) const { 
    //Convenience lookup bypassing the port
    VTR_ASSERT(valid_pin_id(id));

    AtomPortId port_id = pin_port(id);
    return port_block(port_id);
}

bool AtomNetlist::pin_is_constant (const AtomPinId id) const {
    VTR_ASSERT(valid_pin_id(id));

    return pin_is_constant_[size_t(id)];
}


/*
 *
 * Nets
 *
 */
const std::string& AtomNetlist::net_name (const AtomNetId id) const { 
    VTR_ASSERT(valid_net_id(id));

    AtomStringId str_id = net_names_[size_t(id)];
    return strings_[size_t(str_id)];
}

AtomNetlist::pin_range AtomNetlist::net_pins (const AtomNetId id) const {
    VTR_ASSERT(valid_net_id(id));

    return vtr::make_range(net_pins_[size_t(id)].begin(), net_pins_[size_t(id)].end());
}

AtomPinId AtomNetlist::net_driver (const AtomNetId id) const {
    VTR_ASSERT(valid_net_id(id));

    if(net_pins_[size_t(id)].size() > 0) {
        return net_pins_[size_t(id)][0];
    } else {
        return AtomPinId::INVALID();
    }
}

AtomBlockId AtomNetlist::net_driver_block (const AtomNetId id) const {
    auto driver_pin_id = net_driver(id);
    if(driver_pin_id) {
        return pin_block(driver_pin_id);
    }
    return AtomBlockId::INVALID();
}

AtomNetlist::pin_range AtomNetlist::net_sinks (const AtomNetId id) const {
    VTR_ASSERT(valid_net_id(id));

    return vtr::make_range(++net_pins_[size_t(id)].begin(), net_pins_[size_t(id)].end());
}

bool AtomNetlist::net_is_constant (const AtomNetId id) const {
    VTR_ASSERT(valid_net_id(id));

    //Look-up the driver
    auto driver_pin_id = net_driver(id);
    if(driver_pin_id) {
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
AtomNetlist::block_range AtomNetlist::blocks () const {
    return vtr::make_range(block_ids_.begin(), block_ids_.end()); 
}

AtomNetlist::net_range AtomNetlist::nets () const {
    return vtr::make_range(net_ids_.begin(), net_ids_.end()); 
}


/*
 *
 * Lookups
 *
 */
AtomBlockId AtomNetlist::find_block (const std::string& name) const {
    auto str_id = find_string(name);
    if(!str_id) {
        return AtomBlockId::INVALID();
    } else {
        return find_block(str_id);
    }
}

AtomPortId AtomNetlist::find_port (const AtomBlockId blk_id, const t_model_ports* model_port) const {
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(model_port);

    //We can tell from the model port the set of ports
    //the port can be found in
    port_range range = (model_port->dir == IN_PORT) ?
                             (model_port->is_clock) ?  block_clock_ports(blk_id) : block_input_ports(blk_id)
                            : (block_output_ports(blk_id));

    for(auto port_id : range) {
        if(port_name(port_id) == model_port->name) {
            return port_id;
        }
    }

    return AtomPortId::INVALID();
}

AtomPortId AtomNetlist::find_port (const AtomBlockId blk_id, const std::string& name) const {
    VTR_ASSERT(valid_block_id(blk_id));

    //Since we only know the port name, we must search all the ports
    for(auto ports : {block_input_ports(blk_id), block_output_ports(blk_id), block_clock_ports(blk_id)}) {
        for(auto port_id : ports) {
            if(port_name(port_id) == name) {
                return port_id;
            }
        }
    }
    return AtomPortId::INVALID();
}

AtomPinId AtomNetlist::find_pin (const AtomPortId port_id, BitIndex port_bit) const {
    VTR_ASSERT(valid_port_id(port_id));
    VTR_ASSERT(valid_port_bit(port_id, port_bit));

    //Pins are stored in ascending order of bit index,
    //so we can binary search for the specific bit
    auto port_bit_cmp = [&](const AtomPinId pin_id, BitIndex bit_index) {
        return pin_port_bit(pin_id) < bit_index; 
    };

    auto pins = port_pins(port_id);

    //Finds the location where the pin with bit index port_bit should be located (if it exists)
    auto iter = std::lower_bound(pins.begin(), pins.end(), port_bit, port_bit_cmp);

    if(iter == pins.end() || pin_port_bit(*iter) != port_bit) {
        //Either the end of the pins (i.e. not found), or
        //the value does not match (indicating a gap in the indicies, so also not found)
        return AtomPinId::INVALID();
    } else {
        //Found it
        VTR_ASSERT(pin_port_bit(*iter) == port_bit);
        return *iter;
    }
}

AtomNetId AtomNetlist::find_net (const std::string& name) const {
    auto str_id = find_string(name);
    if(!str_id) {
        return AtomNetId::INVALID();
    } else {
        return find_net(str_id);
    }
}

/*
 *
 * Validation
 *
 */
bool AtomNetlist::verify() const {
    //Verify data structure consistency
    verify_sizes();
    verify_refs();
    verify_lookups();

    //Verify logical consistency
    verify_block_invariants();

    return true;
}

//Checks that the sizes of internal data structures
//are consistent. Should take constant time.
bool AtomNetlist::verify_sizes() const {
    validate_block_sizes();
    validate_port_sizes();
    validate_pin_sizes();
    validate_net_sizes();
    validate_string_sizes();
    return true;
}

//Checks that all cross-references are consistent.
//Should take linear time.
bool AtomNetlist::verify_refs() const {
    validate_block_port_refs();
    validate_block_pin_refs();
    validate_port_pin_refs();
    validate_net_pin_refs();
    validate_string_refs();
    return true;
}

bool AtomNetlist::verify_lookups() const {
    //Verify that fast look-ups are consistent

    //Blocks
    for(auto blk_id : blocks()) {
        const auto& name = block_name(blk_id);
        if(find_block(name) != blk_id) {
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
        }
    }

    //Ports
    for(auto port_id : port_ids_) {
        auto blk_id = port_block(port_id);
        const auto& name = port_name(port_id);
        if(find_port(blk_id, name) != port_id) {
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
        }
    }

    //Pins
    for(auto pin_id : pin_ids_) {
        auto port_id = pin_port(pin_id);
        auto bit = pin_port_bit(pin_id);
        if(find_pin(port_id, bit) != pin_id) {
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
        }
    }

    //Nets
    for(auto net_id : nets()) {
        const auto& name = net_name(net_id); 
        if(find_net(name) != net_id) {
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
        }
    }

    //Port common
    for(auto str_id : string_ids_) {
        const auto& name = strings_[size_t(str_id)];
        if(find_string(name) != str_id) {
            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block lookup by name mismatch");
        }
    }
    return true;
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

            if(block_type(blk_id) == AtomBlockType::SEQUENTIAL) {
                //Sequential types must have a clock
                if(!clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is sequential type but has no clock", 
                              block_name(blk_id).c_str());
                }

            } else {
                //Non-sequential types must not have a clock
                if(clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is a non-sequential type but has a clock '%s'", 
                              block_name(blk_id).c_str(), net_name(clk_net_id).c_str());
                }
            }
        }
        {
            //Check that block pins and ports are consistent (i.e. same number of pins)
            size_t num_block_pins = block_pins(blk_id).size();

            size_t total_block_port_pins = 0;
            for(auto ports : {block_input_ports(blk_id), block_output_ports(blk_id), block_clock_ports(blk_id)}) {
                for(auto port_id : ports) {
                    total_block_port_pins += port_pins(port_id).size(); 
                }
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
    //Must have a non-mepty name
    VTR_ASSERT_MSG(!name.empty(), "Non-Empty block name");

    //Check if the block has already been created
    AtomStringId name_id = create_string(name);
    AtomBlockId blk_id = find_block(name_id);

    if(blk_id == AtomBlockId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = AtomBlockId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name_id);
        block_models_.push_back(model);
        block_truth_tables_.push_back(truth_table);

        //Initialize the look-ups
        block_name_to_block_id_[name_id] = blk_id;

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
    VTR_ASSERT(block_model(blk_id) == model);
    VTR_ASSERT(block_truth_table(blk_id) == truth_table);
    VTR_ASSERT_SAFE(find_block(name) == blk_id);

    return blk_id;
}

AtomPortId  AtomNetlist::create_port (const AtomBlockId blk_id, const t_model_ports* model_port) {
    //Check pre-conditions
    VTR_ASSERT_MSG(valid_block_id(blk_id), "Valid block id");

    //See if the port already exists
    AtomStringId name_id = create_string(model_port->name);
    AtomPortId port_id = find_port(blk_id, model_port);
    if(!port_id) {
        //Not found, create it

        //Reserve an id
        port_id = AtomPortId(port_ids_.size());
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
    VTR_ASSERT(port_width(port_id) == (unsigned) model_port->size);
    VTR_ASSERT(port_model(port_id) == model_port);
    VTR_ASSERT_SAFE(find_port(blk_id, name) == port_id);
    VTR_ASSERT_SAFE(find_port(blk_id, model_port) == port_id);

    return port_id;
}

AtomPinId AtomNetlist::create_pin (const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const AtomPinType type, bool is_const) {
    //Check pre-conditions (valid ids)
    VTR_ASSERT_MSG(valid_port_id(port_id), "Valid port id");
    VTR_ASSERT_MSG(valid_port_bit(port_id, port_bit), "Valid port bit");
    VTR_ASSERT_MSG(valid_net_id(net_id), "Valid net id");

    //See if the pin already exists
    AtomPinId pin_id = find_pin(port_id, port_bit);
    if(!pin_id) {
        //Not found, create it

        //Reserve an id
        pin_id = AtomPinId(pin_ids_.size());
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

AtomNetId AtomNetlist::create_net (const std::string name) {
    //Creates an empty net (or returns an existing one)
    VTR_ASSERT_MSG(!name.empty(), "Valid net name");

    //Check if the net has already been created
    AtomStringId name_id = create_string(name);
    AtomNetId net_id = find_net(name_id);
    if(net_id == AtomNetId::INVALID()) {
        //Not found, create it

        //Reserve an id
        net_id = AtomNetId(net_ids_.size());
        net_ids_.push_back(net_id);

        //Initialize the data
        net_names_.push_back(name_id);

        //Initialize the look-ups
        net_name_to_net_id_[name_id] = net_id;

        //Initialize with no driver
        net_pins_.emplace_back();
        net_pins_[size_t(net_id)].emplace_back(AtomPinId::INVALID());

        VTR_ASSERT(net_pins_[size_t(net_id)].size() == 1);
        VTR_ASSERT(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID());
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_net_sizes());

    //Check post-conditions: values
    VTR_ASSERT(valid_net_id(net_id));
    VTR_ASSERT(net_name(net_id) == name);
    VTR_ASSERT(find_net(name) == net_id);

    return net_id;

}

AtomNetId AtomNetlist::add_net (const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks) {
    //Creates a net with a full set of pins
    VTR_ASSERT_MSG(!find_net(name), "Net should not exist");

    //Create the empty net
    AtomNetId net_id = create_net(name);

    //Set the driver and sinks of the net
    auto& dest_pins = net_pins_[size_t(net_id)];
    dest_pins[0] = driver;
    dest_pins.insert(dest_pins.end(),
            std::make_move_iterator(sinks.begin()),
            std::make_move_iterator(sinks.end()));

    //Associate each pin with the net
    pin_nets_[size_t(driver)] = net_id;
    for(auto sink : sinks) {
        pin_nets_[size_t(sink)] = net_id;
    }

    return net_id;
}
void AtomNetlist::set_pin_is_constant(const AtomPinId pin_id, const bool value) {
    VTR_ASSERT(valid_pin_id(pin_id));

    pin_is_constant_[size_t(pin_id)] = value;
}

void AtomNetlist::remove_block(const AtomBlockId blk_id) {
    VTR_ASSERT(valid_block_id(blk_id));
    
    //Remove the ports
    for(AtomPortId block_port : block_ports(blk_id)) {
        remove_port(block_port);
    }

    //Invalidate look-up
    AtomStringId name_id = block_names_[size_t(blk_id)];
    block_name_to_block_id_[name_id] = AtomBlockId::INVALID();

    //Mark as invalid
    block_ids_[size_t(blk_id)] = AtomBlockId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

}

void AtomNetlist::remove_net(const AtomNetId net_id) {
    VTR_ASSERT(valid_net_id(net_id));

    //Dissassociate the pins from the net
    for(auto pin_id : net_pins(net_id)) {
        if(pin_id) {
            pin_nets_[size_t(pin_id)] = AtomNetId::INVALID();
        }
    }
    //Invalidate look-up
    AtomStringId name_id = net_names_[size_t(net_id)];
    net_name_to_net_id_[name_id] = AtomNetId::INVALID();

    //Mark as invalid
    net_ids_[size_t(net_id)] = AtomNetId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

}


void AtomNetlist::remove_port(const AtomPortId port_id) {
    VTR_ASSERT(valid_port_id(port_id));

    //Remove the pins
    for(auto pin : port_pins(port_id)) {
        if(valid_pin_id(pin)) {
            remove_pin(pin);
        }
    }

    //Mark as invalid
    port_ids_[size_t(port_id)] = AtomPortId::INVALID();

    //Mark netlist dirty
    dirty_ = true;
}

void AtomNetlist::remove_pin(const AtomPinId pin_id) {
    VTR_ASSERT(valid_pin_id(pin_id));

    //Find the associated net
    AtomNetId net = pin_net(pin_id);

    //Remove the pin from the associated net
    remove_net_pin(net, pin_id);

    //Mark as invalid
    pin_ids_[size_t(pin_id)] = AtomPinId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

}

void AtomNetlist::remove_net_pin(const AtomNetId net_id, const AtomPinId pin_id) {
    //Remove a net-pin connection
    //
    //Note that during sweeping either the net or pin could be invalid (i.e. already swept)
    //so we check before trying to use them

    if(valid_net_id(net_id)) {
        //Warning: this is slow!
        auto iter = std::find(net_pins_[size_t(net_id)].begin(), net_pins_[size_t(net_id)].end(), pin_id); //Linear search
        VTR_ASSERT(iter != net_pins_[size_t(net_id)].end());

        if(net_driver(net_id) == pin_id) {
            //Mark no driver
            net_pins_[size_t(net_id)][0] = AtomPinId::INVALID();
        } else {
            //Remove sink
            net_pins_[size_t(net_id)].erase(iter); //Linear remove
        }

        //Note: since we fully update the net we don't need to mark the netlist dirty_
    }

    //Dissassociate the pin with the net
    if(valid_pin_id(pin_id)) {
        pin_nets_[size_t(pin_id)] = AtomNetId::INVALID();

        //Mark netlist dirty, since we are leaving an invalid net id
        dirty_ = true;
    }
}

void AtomNetlist::remove_unused() {
    //Mark any nets/pins/ports/blocks which are not in use as invalid
    //so they will be removed

    bool found_unused;
    do {
        found_unused = false;
        //Nets with no connected pins
        for(auto net_id : nets()) {
            if(net_id 
                && !net_driver(net_id) 
                && net_sinks(net_id).size() == 0) {
                remove_net(net_id);
                found_unused = true;
            }
        }

        //Pins with no connected nets
        for(auto pin_id : pin_ids_) {
            if(pin_id && !pin_net(pin_id)) {
                remove_pin(pin_id);
                found_unused = true;
            }
        }

        //Empty ports
        for(auto port_id : port_ids_) {
            if(port_id && port_pins(port_id).size() == 0) {
                remove_port(port_id);
                found_unused = true;
            }
        }

        //Blocks with no ports
        for(auto blk_id : blocks()) {
            if(blk_id 
                && block_input_ports(blk_id).size() == 0 
                && block_output_ports(blk_id).size() == 0 
                && block_clock_ports(blk_id).size() == 0) {
                remove_block(blk_id);
                found_unused = true;
            }
        }
    } while(found_unused);
}

void AtomNetlist::compress() {
    //Compress the various netlist components to remove invalid entries
    // Note: this invalidates all Ids

    //Walk the netlist to invalidate any unused items
    remove_unused();

    //The clean_*() functions return a vector which maps from old to new index
    // e.g. block_id_map[old_id] == new_id
    auto net_id_map = clean_nets();
    auto pin_id_map = clean_pins();
    auto port_id_map = clean_ports();
    auto block_id_map = clean_blocks();
    //TODO: clean strings
    //TODO: iterative cleaning?

    //Now we re-build all the cross references
    rebuild_block_refs(pin_id_map, port_id_map);
    rebuild_port_refs(block_id_map, pin_id_map);
    rebuild_pin_refs(port_id_map, net_id_map);
    rebuild_net_refs(pin_id_map);

    //Re-build the lookups
    rebuild_lookups();

    //Resize containers to exact size
    shrink_to_fit();

    //Netlist is now clean
    dirty_ = false;
}

std::vector<AtomBlockId> AtomNetlist::clean_blocks() {
    //Clean the blocks
    std::vector<AtomBlockId> block_id_map;
    std::vector<AtomBlockId> new_ids;
    std::tie(new_ids, block_id_map) = compress_ids(block_ids_);

    //Move all the valid values
    block_names_ = move_valid(block_names_, block_ids_);
    block_models_ = move_valid(block_models_, block_ids_);
    block_truth_tables_ = move_valid(block_truth_tables_, block_ids_);

    block_pins_ = move_valid(block_pins_, block_ids_);
    block_num_input_pins_ = move_valid(block_num_input_pins_, block_ids_);
    block_num_output_pins_ = move_valid(block_num_output_pins_, block_ids_);
    block_num_clock_pins_ = move_valid(block_num_clock_pins_, block_ids_);

    block_ports_ = move_valid(block_ports_, block_ids_);
    block_num_input_ports_ = move_valid(block_num_input_ports_, block_ids_);
    block_num_output_ports_ = move_valid(block_num_output_ports_, block_ids_);
    block_num_clock_ports_ = move_valid(block_num_clock_ports_, block_ids_);

    //Update Ids last since used as predicate
    block_ids_ = new_ids;

    VTR_ASSERT(validate_block_sizes());

    VTR_ASSERT_SAFE_MSG(are_contiguous(block_ids_), "Ids should be contiguous");
    VTR_ASSERT_SAFE_MSG(all_valid(block_ids_), "All Ids should be valid");

    return block_id_map;
}

std::vector<AtomPortId> AtomNetlist::clean_ports() {
    //Clean the ports
    std::vector<AtomPortId> port_id_map;
    std::vector<AtomPortId> new_ids;
    std::tie(new_ids, port_id_map) = compress_ids(port_ids_);

    //Copy all the valid values
    port_names_ = move_valid(port_names_, port_ids_);
    port_blocks_ = move_valid(port_blocks_, port_ids_);
    port_models_ = move_valid(port_models_, port_ids_);
    port_pins_ = move_valid(port_pins_, port_ids_);

    //Update Ids last since used as predicate
    port_ids_ = new_ids;

    VTR_ASSERT(validate_port_sizes());

    VTR_ASSERT_SAFE_MSG(are_contiguous(port_ids_), "Ids should be contiguous");
    VTR_ASSERT_SAFE_MSG(all_valid(port_ids_), "All Ids should be valid");

    return port_id_map;
}

std::vector<AtomPinId> AtomNetlist::clean_pins() {
    //Clean the pins
    std::vector<AtomPinId> pin_id_map;
    std::vector<AtomPinId> new_ids;
    std::tie(new_ids, pin_id_map) = compress_ids(pin_ids_);

    //Copy all the valid values
    pin_ports_ = move_valid(pin_ports_, pin_ids_);
    pin_port_bits_ = move_valid(pin_port_bits_, pin_ids_);
    pin_nets_ = move_valid(pin_nets_, pin_ids_);
    pin_is_constant_ = move_valid(pin_is_constant_, pin_ids_);

    //Update Ids last since used as predicate
    pin_ids_ = new_ids;

    VTR_ASSERT(validate_pin_sizes());

    VTR_ASSERT_SAFE_MSG(are_contiguous(pin_ids_), "Ids should be contiguous");
    VTR_ASSERT_SAFE_MSG(all_valid(pin_ids_), "All Ids should be valid");

    return pin_id_map;
}

std::vector<AtomNetId> AtomNetlist::clean_nets() {
    //Clean the nets
    std::vector<AtomNetId> net_id_map;
    std::vector<AtomNetId> new_ids;
    std::tie(new_ids, net_id_map) = compress_ids(net_ids_);

    //Copy all the valid values
    net_names_ = move_valid(net_names_, net_ids_);
    net_pins_ = move_valid(net_pins_, net_ids_);

    //Update Ids last since used as predicate
    net_ids_ = new_ids;

    VTR_ASSERT(validate_net_sizes());

    VTR_ASSERT_SAFE_MSG(are_contiguous(net_ids_), "Ids should be contiguous");
    VTR_ASSERT_SAFE_MSG(all_valid(net_ids_), "All Ids should be valid");

    return net_id_map;
}

void AtomNetlist::rebuild_block_refs(const std::vector<AtomPinId>& pin_id_map, const std::vector<AtomPortId>& port_id_map) {
    //Update the pin id references held by blocks
    for(auto blk_id : blocks()) {
        //Before update the references, we need to know how many are valid,
        //so we can also update the numbers of input/output/clock pins

        //Note that we take special care to not modify the pin counts until after
        //they have all been counted (since the block_*_pins() functions depend on
        //the counts).
        size_t num_input_pins = count_valid_refs(block_input_pins(blk_id), pin_id_map);
        size_t num_output_pins = count_valid_refs(block_output_pins(blk_id), pin_id_map);
        size_t num_clock_pins = count_valid_refs(block_clock_pins(blk_id), pin_id_map);

        std::vector<AtomPinId>& pins = block_pins_[size_t(blk_id)];

        pins = update_valid_refs(pins, pin_id_map);
        block_num_input_pins_[size_t(blk_id)] = num_input_pins;
        block_num_output_pins_[size_t(blk_id)] = num_output_pins;
        block_num_clock_pins_[size_t(blk_id)] = num_clock_pins;

        VTR_ASSERT_SAFE_MSG(all_valid(pins), "All Ids should be valid");
        VTR_ASSERT(pins.size() == (size_t) block_num_input_pins_[size_t(blk_id)]
                                           + block_num_output_pins_[size_t(blk_id)]
                                           + block_num_clock_pins_[size_t(blk_id)]);

        //Similarily for ports
        size_t num_input_ports = count_valid_refs(block_input_ports(blk_id), port_id_map);
        size_t num_output_ports = count_valid_refs(block_output_ports(blk_id), port_id_map);
        size_t num_clock_ports = count_valid_refs(block_clock_ports(blk_id), port_id_map);

        std::vector<AtomPortId>& ports = block_ports_[size_t(blk_id)];

        ports = update_valid_refs(ports, port_id_map);
        block_num_input_ports_[size_t(blk_id)] = num_input_ports;
        block_num_output_ports_[size_t(blk_id)] = num_output_ports;
        block_num_clock_ports_[size_t(blk_id)] = num_clock_ports;

        VTR_ASSERT_SAFE_MSG(all_valid(ports), "All Ids should be valid");
        VTR_ASSERT(ports.size() == (size_t) block_num_input_ports_[size_t(blk_id)]
                                           + block_num_output_ports_[size_t(blk_id)]
                                           + block_num_clock_ports_[size_t(blk_id)]);
    }

    VTR_ASSERT(validate_block_sizes());
}

void AtomNetlist::rebuild_port_refs(const std::vector<AtomBlockId>& block_id_map, const std::vector<AtomPinId>& pin_id_map) {
    //Update block and pin references held by ports
    port_blocks_ = update_valid_refs(port_blocks_, block_id_map); 
    VTR_ASSERT_SAFE_MSG(all_valid(port_blocks_), "All Ids should be valid");

    for(auto& pins : port_pins_) {
        pins = update_valid_refs(pins, pin_id_map);
        VTR_ASSERT_SAFE_MSG(all_valid(pins), "All Ids should be valid");
    }
    VTR_ASSERT(validate_port_sizes());
}

void AtomNetlist::rebuild_pin_refs(const std::vector<AtomPortId>& port_id_map, const std::vector<AtomNetId>& net_id_map) {
    //Update port and net references held by pins
    pin_ports_ = update_all_refs(pin_ports_, port_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_ports_), "All Ids should be valid");

    pin_nets_ = update_all_refs(pin_nets_, net_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_nets_), "All Ids should be valid");

    VTR_ASSERT(validate_pin_sizes());
}

void AtomNetlist::rebuild_net_refs(const std::vector<AtomPinId>& pin_id_map) {
    //Update pin references held by nets
    for(auto& pins : net_pins_) {
        pins = update_valid_refs(pins, pin_id_map);

        VTR_ASSERT_SAFE_MSG(all_valid(pins), "All sinks should be valid");
    }
    VTR_ASSERT(validate_net_sizes());
}
void AtomNetlist::rebuild_lookups() {
    //We iterate through the reverse-lookups and update the values (i.e. ids)
    //to the new id values

    //Blocks
    block_name_to_block_id_.clear();
    for(auto blk_id : blocks()) {
        const auto& key = block_names_[size_t(blk_id)];
        block_name_to_block_id_[key] = blk_id;
    }

    //Nets
    net_name_to_net_id_.clear();
    for(auto net_id : nets()) {
        const auto& key = net_names_[size_t(net_id)];
        net_name_to_net_id_[key] = net_id;
    }
}

void AtomNetlist::shrink_to_fit() {
    //Block data
    block_ids_.shrink_to_fit();
    block_names_.shrink_to_fit();
    block_models_.shrink_to_fit();
    block_truth_tables_.shrink_to_fit();

    block_pins_.shrink_to_fit();
    for(auto& pins : block_pins_) {
        pins.shrink_to_fit();
    }
    block_num_input_pins_.shrink_to_fit();
    block_num_output_pins_.shrink_to_fit();
    block_num_clock_pins_.shrink_to_fit();

    block_ports_.shrink_to_fit();
    for(auto& ports : block_ports_) {
        ports.shrink_to_fit();
    }
    block_num_input_ports_.shrink_to_fit();
    block_num_output_ports_.shrink_to_fit();
    block_num_clock_ports_.shrink_to_fit();

    VTR_ASSERT(validate_block_sizes());

    //Port data
    port_ids_.shrink_to_fit();
    port_blocks_.shrink_to_fit();
    port_pins_.shrink_to_fit();
    for(auto& pins : port_pins_) {
        pins.shrink_to_fit();
    }
    VTR_ASSERT(validate_port_sizes());

    //Pin data
    pin_ids_.shrink_to_fit();
    pin_ports_.shrink_to_fit();
    pin_port_bits_.shrink_to_fit();
    pin_nets_.shrink_to_fit();
    VTR_ASSERT(validate_pin_sizes());

    //Net data
    net_ids_.shrink_to_fit();
    net_names_.shrink_to_fit();
    net_pins_.shrink_to_fit();
    VTR_ASSERT(validate_net_sizes());

    //String data
    string_ids_.shrink_to_fit();
    VTR_ASSERT(validate_string_sizes());
}

/*
 *
 * Sanity Checks
 *
 */
bool AtomNetlist::valid_block_id(AtomBlockId id) const {
    if(id == AtomBlockId::INVALID()) return false;
    else if(size_t(id) >= block_ids_.size()) return false;
    else if(block_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_port_id(AtomPortId id) const {
    if(id == AtomPortId::INVALID()) return false;
    else if(size_t(id) >= port_ids_.size()) return false;
    else if(port_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_port_bit(AtomPortId id, BitIndex port_bit) const {
    VTR_ASSERT(valid_port_id(id));
    if(port_bit >= port_width(id)) return false;
    return true;
}

bool AtomNetlist::valid_pin_id(AtomPinId id) const {
    if(id == AtomPinId::INVALID()) return false;
    else if(size_t(id) >= pin_ids_.size()) return false;
    else if(pin_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_net_id(AtomNetId id) const {
    if(id == AtomNetId::INVALID()) return false;
    else if(size_t(id) >= net_ids_.size()) return false;
    else if(net_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_string_id(AtomStringId id) const {
    if(id == AtomStringId::INVALID()) return false;
    else if(size_t(id) >= string_ids_.size()) return false;
    else if(string_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::validate_block_sizes() const {
    if(block_names_.size() != block_ids_.size()
        || block_models_.size() != block_ids_.size()
        || block_truth_tables_.size() != block_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent block data sizes");
    }
    return true;
}

bool AtomNetlist::validate_port_sizes() const {
    if(port_names_.size() != port_ids_.size()
        || port_blocks_.size() != port_ids_.size()
        || port_models_.size() != port_models_.size()
        || port_pins_.size() != port_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent port data sizes");
    }
    return true;
}

bool AtomNetlist::validate_pin_sizes() const {
    if(pin_ports_.size() != pin_ids_.size()
        || pin_port_bits_.size() != pin_ids_.size()
        || pin_nets_.size() != pin_ids_.size()
        || pin_is_constant_.size() != pin_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent pin data sizes");
    }
    return true;
}

bool AtomNetlist::validate_net_sizes() const {
    if(net_names_.size() != net_ids_.size()
        || net_pins_.size() != net_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent net data sizes");
    }
    return true;
}

bool AtomNetlist::validate_string_sizes() const {
    if(strings_.size() != string_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent string data sizes");
    }
    return true;
}

bool AtomNetlist::validate_block_port_refs() const {
    //Verify that all block <-> port references are consistent

    //Track how many times we've seen each port from the blocks 
    std::vector<unsigned> seen_port_ids(port_ids_.size());

    for(auto blk_id : blocks()) {
        for(auto in_port_id : block_input_ports(blk_id)) {
            if(blk_id != port_block(in_port_id)) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-input port cross-reference does not match");
            }
            ++seen_port_ids[size_t(in_port_id)];
        }
        for(auto out_port_id : block_output_ports(blk_id)) {
            if(blk_id != port_block(out_port_id)) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-output port cross-reference does not match") ;
            }
            ++seen_port_ids[size_t(out_port_id)];
        }
        for(auto clock_port_id : block_clock_ports(blk_id)) {
            if(blk_id != port_block(clock_port_id)) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block-clock port cross-reference does not match"); 
            }
            ++seen_port_ids[size_t(clock_port_id)];
        }
    }

    //Check that we have neither orphaned ports (i.e. that aren't referenced by a block) 
    //nor shared ports (i.e. referenced by multiple blocks)
    if(!std::all_of(seen_port_ids.begin(), seen_port_ids.end(),
        [](unsigned val) {
            return val == 1;
    })) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Port not referenced by a single block");
    }

    if(std::accumulate(seen_port_ids.begin(), seen_port_ids.end(), 0u) != port_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned port (not referenced by a block)");
    }

    return true;
}

bool AtomNetlist::validate_block_pin_refs() const {
    //Verify that block pin refs are consistent 
    //(e.g. inputs are inputs, outputs are outputs etc.)
    for(auto blk_id : blocks()) {

        //Check that only input pins are found when iterating through inputs
        for(auto pin_id : block_input_pins(blk_id)) {
            auto port_id = pin_port(pin_id);


            auto type = port_type(port_id);
            if(type != AtomPortType::INPUT) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-input pin in block input pins");
            }
        }

        //Check that only output pins are found when iterating through outputs
        for(auto pin_id : block_output_pins(blk_id)) {
            auto port_id = pin_port(pin_id);

            auto type = port_type(port_id);
            if(type != AtomPortType::OUTPUT) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-output pin in block output pins");
            }
        }

        //Check that only clock pins are found when iterating through clocks
        for(auto pin_id : block_clock_pins(blk_id)) {
            auto port_id = pin_port(pin_id);

            auto type = port_type(port_id);
            if(type != AtomPortType::CLOCK) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Non-clock pin in block clock pins");
            }
        }
    }
    return true;
}

bool AtomNetlist::validate_port_pin_refs() const {
    //Check that port <-> pin references are consistent

    //Track how many times we've seen each pin from the ports
    std::vector<unsigned> seen_pin_ids(pin_ids_.size());

    for(auto port_id : port_ids_) {
        for(auto pin_id : port_pins(port_id)) {
            if(pin_port(pin_id) != port_id) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Port-pin cross-reference does not match");
            }
            if(pin_port_bit(pin_id) >= port_width(port_id)) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Out-of-range port bit index");
            }
            ++seen_pin_ids[size_t(pin_id)];
        }
    }

    //Check that we have neither orphaned pins (i.e. that aren't referenced by a port) 
    //nor shared pins (i.e. referenced by multiple ports)
    if(!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(),
        [](unsigned val) {
            return val == 1;
    })) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Pins referenced by zero or multiple ports");
    }

    if(std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned pins (not referenced by a port)");
    }

    return true;
}

bool AtomNetlist::validate_net_pin_refs() const {
    //Check that net <-> pin references are consistent

    //Track how many times we've seen each pin from the ports
    std::vector<unsigned> seen_pin_ids(pin_ids_.size());

    for(auto net_id : nets()) {
        auto pins = net_pins(net_id);
        for(auto iter = pins.begin(); iter != pins.end(); ++iter) {
            auto pin_id = *iter;
            if(iter != pins.begin()) {
                //The first net pin is the driver, which may be invalid
                //if there is no driver. So we only check for a valid id
                //on the other net pins (which are all sinks and must be valid)
                if(!pin_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid pin found in net sinks");
                }
            }

            if(pin_id) {
                //Verify the cross reference if the pin_id is valid (i.e. a sink or a valid driver)
                if(pin_net(pin_id) != net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Net-pin cross-reference does not match");
                }

                //We only record valid seen pins since we may see multiple undriven nets with invalid IDs
                ++seen_pin_ids[size_t(pin_id)];
            }
        }
    }

    //Check that we have niether orphaned pins (i.e. that aren't referenced by a port) 
    //nor shared pins (i.e. referenced by multiple ports)
    if(!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(),
        [](unsigned val) {
            return val == 1;
    })) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found pins referenced by zero or multiple nets");
    }

    if(std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Found orphaned pins (not referenced by a net)");
    }
    
    return true;
}

bool AtomNetlist::validate_string_refs() const {
    for(const auto& string_ids : {block_names_, port_names_, net_names_}) {
        for(const auto& str_id : string_ids) {
            if(!valid_string_id(str_id)) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Invalid string reference");
            }
        }
    }
    return true;
}


/*
 *
 * Internal utilities
 *
 */
AtomStringId AtomNetlist::find_string (const std::string& str) const {
    auto iter = string_to_string_id_.find(str);
    if(iter != string_to_string_id_.end()) {
        AtomStringId str_id = iter->second;

        VTR_ASSERT(str_id);
        VTR_ASSERT(strings_[size_t(str_id)] == str);

        return str_id;
    } else {
        return AtomStringId::INVALID();
    }
}

AtomBlockId AtomNetlist::find_block(const AtomStringId name_id) const {
    VTR_ASSERT(valid_string_id(name_id));
    auto iter = block_name_to_block_id_.find(name_id);
    if(iter != block_name_to_block_id_.end()) {
        AtomBlockId blk_id = iter->second;

        //Check post-conditions
        if(blk_id) {
            VTR_ASSERT(valid_block_id(blk_id));
            VTR_ASSERT(block_names_[size_t(blk_id)] == name_id);
        }

        return blk_id;
    } else {
        return AtomBlockId::INVALID();
    }
}

AtomNetId AtomNetlist::find_net(const AtomStringId name_id) const {
    VTR_ASSERT(valid_string_id(name_id));
    auto iter = net_name_to_net_id_.find(name_id);
    if(iter != net_name_to_net_id_.end()) {
        AtomNetId net_id = iter->second;

        if(net_id) {
            //Check post-conditions
            VTR_ASSERT(valid_net_id(net_id));
            VTR_ASSERT(net_names_[size_t(net_id)] == name_id);
        }

        return iter->second;
    } else {
        return AtomNetId::INVALID();
    }
}

AtomStringId AtomNetlist::create_string (const std::string& str) {
    AtomStringId str_id = find_string(str);
    if(!str_id) {
        //Not found, create

        //Reserve an id
        str_id = AtomStringId(string_ids_.size());
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
    VTR_ASSERT(strings_[size_t(str_id)] == str);
    VTR_ASSERT_SAFE(find_string(str) == str_id);

    return str_id;
}

void AtomNetlist::associate_pin_with_net(const AtomPinId pin_id, const AtomPinType type, const AtomNetId net_id) {
    //Add the pin to the net
    if(type == AtomPinType::DRIVER) {
        VTR_ASSERT_MSG(net_pins_[size_t(net_id)].size() > 0, "Space for net's pin");
        VTR_ASSERT_MSG(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID(), "No existing net driver");
        
        net_pins_[size_t(net_id)][0] = pin_id; //Set driver
    } else {
        VTR_ASSERT(type == AtomPinType::SINK);

        net_pins_[size_t(net_id)].emplace_back(pin_id); //Add sink
    }
}

void AtomNetlist::associate_pin_with_port(const AtomPinId pin_id, const AtomPortId port_id) {
    port_pins_[size_t(port_id)].push_back(pin_id);
}

void AtomNetlist::associate_pin_with_block(const AtomPinId pin_id, const AtomPortType type, const AtomBlockId blk_id) {
    auto port_id = pin_port(pin_id);
    VTR_ASSERT(port_type(port_id) == type);

    //Get an interator pointing to where we want to insert
    pin_iterator iter;
    if(type == AtomPortType::INPUT) {
        iter = block_input_pins(blk_id).end();
        ++block_num_input_pins_[size_t(blk_id)];
    } else if (type == AtomPortType::OUTPUT) {
        iter = block_output_pins(blk_id).end();
        ++block_num_output_pins_[size_t(blk_id)];
    } else {
        VTR_ASSERT(type == AtomPortType::CLOCK);
        iter = block_clock_pins(blk_id).end();
        ++block_num_clock_pins_[size_t(blk_id)];
    }

    //Insert the element just before iter
    auto new_iter = block_pins_[size_t(blk_id)].insert(iter, pin_id);

    //Inserted value should be the last valid range element
    if(type == AtomPortType::INPUT) {
        VTR_ASSERT(new_iter + 1 == block_input_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).begin());
    } else if (type == AtomPortType::OUTPUT) {
        VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).begin());
    } else {
        VTR_ASSERT(type == AtomPortType::CLOCK);
        VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_pins(blk_id).end());
    }
}

void AtomNetlist::associate_port_with_block(const AtomPortId port_id, const AtomBlockId blk_id) {
    //Associate the port with the blocks inputs/outputs/clocks
    AtomPortType type = port_type(port_id);

    //Get an interator pointing to where we want to insert
    port_iterator iter;
    if(type == AtomPortType::INPUT) {
        iter = block_input_ports(blk_id).end();
        ++block_num_input_ports_[size_t(blk_id)];
    } else if (type == AtomPortType::OUTPUT) {
        iter = block_output_ports(blk_id).end();
        ++block_num_output_ports_[size_t(blk_id)];
    } else {
        VTR_ASSERT(type == AtomPortType::CLOCK);
        iter = block_clock_ports(blk_id).end();
        ++block_num_clock_ports_[size_t(blk_id)];
    }

    //Insert the element just before iter
    auto new_iter = block_ports_[size_t(blk_id)].insert(iter, port_id);

    //Inserted value should be the last valid range element
    if(type == AtomPortType::INPUT) {
        VTR_ASSERT(new_iter + 1 == block_input_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).begin());
    } else if (type == AtomPortType::OUTPUT) {
        VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).begin());
    } else {
        VTR_ASSERT(type == AtomPortType::CLOCK);
        VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_ports(blk_id).end());
    }
}
