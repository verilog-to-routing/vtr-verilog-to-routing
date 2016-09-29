#include "netlist2.h"
#include <algorithm>
#include <unordered_set>

#include "vtr_assert.h"
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
            VTR_ASSERT(false);
            return false;
        }
    }
    return true;
}

template<typename T>
bool all_valid(std::vector<T>& values) {
    for(size_t i = 0; i < values.size(); ++i) {
        if(!values[i]) {
            VTR_ASSERT(false);
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
    id_map.reserve(ids.size());
    new_ids.reserve(ids.size());

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
    copy.reserve(values.size());
    for(size_t i = 0; i < values.size(); ++i) {
        if (pred[i]) {
            copy.emplace_back(std::move(values[i]));
        }
    }
    return copy;
}

//Updates values based on id_map
template<typename T>
std::vector<T> update_refs(const std::vector<T>& values, const std::vector<T>& id_map) {
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

/*
 *
 * Blocks
 *
 */
const std::string& AtomNetlist::block_name (const AtomBlockId id) const { 
    return block_names_[size_t(id)];
}

AtomBlockType AtomNetlist::block_type (const AtomBlockId id) const {
    return block_types_[size_t(id)];
}

const t_model* AtomNetlist::block_model (const AtomBlockId id) const {
    return block_models_[size_t(id)];
}

const AtomNetlist::TruthTable& AtomNetlist::block_truth_table (const AtomBlockId id) const {
    return block_truth_tables_[size_t(id)];
}

vtr::Range<AtomNetlist::port_iterator> AtomNetlist::block_input_ports (const AtomBlockId id) const {
    return vtr::make_range(block_input_ports_[size_t(id)].begin(), block_input_ports_[size_t(id)].end());
}

vtr::Range<AtomNetlist::port_iterator> AtomNetlist::block_output_ports (const AtomBlockId id) const {
    return vtr::make_range(block_output_ports_[size_t(id)].begin(), block_output_ports_[size_t(id)].end());
}

vtr::Range<AtomNetlist::port_iterator> AtomNetlist::block_clock_ports (const AtomBlockId id) const {
    return vtr::make_range(block_clock_ports_[size_t(id)].begin(), block_clock_ports_[size_t(id)].end());
}

AtomPinId AtomNetlist::block_pin (const AtomPortId port_id, size_t port_bit) const {
    //Convenience look-up bypassing port
    return port_pins_[size_t(port_id)][port_bit];
}

/*
 *
 * Ports
 *
 */
const std::string& AtomNetlist::port_name (const AtomPortId id) const {
    //Same for all ports accross the same model, so use the common Id
    AtomPortCommonId common_id = find_port_common_id(id);
    return port_common_names_[size_t(common_id)];
}

size_t AtomNetlist::port_width (const AtomPortId id) const {
    //We look-up the width via the model
    const t_model_ports* port_model = find_port_model(id, port_name(id));

    return static_cast<size_t>(port_model->size);
}

AtomPortType AtomNetlist::port_type (const AtomPortId id) const {
    //Same for all ports accross the same model, so use the common Id
    AtomPortCommonId common_id = find_port_common_id(id);
    return port_common_types_[size_t(common_id)];
}

AtomBlockId AtomNetlist::port_block (const AtomPortId id) const {
    //Unique accross port instances
    return port_blocks_[size_t(id)];
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::port_pins (const AtomPortId id) const {
    //Unique accross port instances
    return vtr::make_range(port_pins_[size_t(id)].begin(), port_pins_[size_t(id)].end());
}

/*
 *
 * Pins
 *
 */
AtomNetId AtomNetlist::pin_net (const AtomPinId id) const { 
    return pin_nets_[size_t(id)];
}

AtomPinType AtomNetlist::pin_type (const AtomPinId id) const { 
    AtomPortId port_id = pin_port(id);
    AtomPinType type;
    switch(port_type(port_id)) {
        case AtomPortType::INPUT: /*fallthrough */;
        case AtomPortType::CLOCK: type = AtomPinType::SINK; break;
        case AtomPortType::OUTPUT: type = AtomPinType::DRIVER; break;
        default: VTR_ASSERT_MSG(false, "Valid port type");
    }
    return type;
}

AtomPortId AtomNetlist::pin_port (const AtomPinId id) const { 
    return pin_ports_[size_t(id)];
}

AtomBlockId AtomNetlist::pin_block (const AtomPinId id) const { 
    //Convenience lookup bypassing the port
    AtomPortId port_id = pin_port(id);
    return port_blocks_[size_t(port_id)];
}

size_t AtomNetlist::pin_port_bit(const AtomPinId id) const {
    return pin_port_bits_[size_t(id)];
}

/*
 *
 * Nets
 *
 */
const std::string& AtomNetlist::net_name (const AtomNetId id) const { 
    return net_names_[size_t(id)];
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::net_pins (const AtomNetId id) const {
    return vtr::make_range(net_pins_[size_t(id)].begin(), net_pins_[size_t(id)].end());
}

AtomPinId AtomNetlist::net_driver (const AtomNetId id) const {
    if(net_pins_[size_t(id)].size() > 0) {
        return net_pins_[size_t(id)][0];
    } else {
        return AtomPinId::INVALID();
    }
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::net_sinks (const AtomNetId id) const {
    return vtr::make_range(++net_pins_[size_t(id)].begin(), net_pins_[size_t(id)].end());
}

/*
 *
 * Aggregates
 *
 */
vtr::Range<AtomNetlist::block_iterator> AtomNetlist::blocks () const {
    return vtr::make_range(block_ids_.begin(), block_ids_.end()); 
}

vtr::Range<AtomNetlist::net_iterator> AtomNetlist::nets () const {
    return vtr::make_range(net_ids_.begin(), net_ids_.end()); 
}


/*
 *
 * Lookups
 *
 */
AtomBlockId AtomNetlist::find_block (const std::string& name) const {
    auto iter = block_name_to_block_id_.find(name);
    if(iter != block_name_to_block_id_.end()) {
        AtomBlockId blk_id = iter->second;

        //Check post-conditions
        if(blk_id) {
            VTR_ASSERT(valid_block_id(blk_id));
            VTR_ASSERT(block_name(blk_id) == name);
        }

        return blk_id;
    } else {
        return AtomBlockId::INVALID();
    }
}

AtomPortId AtomNetlist::find_port (const AtomBlockId blk_id, const std::string& name) const {
    VTR_ASSERT(valid_block_id(blk_id));
    auto iter = block_id_port_name_to_port_id_.find(std::make_tuple(blk_id, name));
    if(iter != block_id_port_name_to_port_id_.end()) {
        AtomPortId port_id = iter->second;

        //Check post-conditions
        if(port_id) {
            VTR_ASSERT(valid_port_id(port_id));
            VTR_ASSERT(port_name(port_id) == name);
        }
        
        return port_id;
    } else {
        return AtomPortId::INVALID();
    }
}

AtomPinId AtomNetlist::find_pin (const AtomPortId port_id, size_t port_bit) const {
    VTR_ASSERT(valid_port_id(port_id));
    VTR_ASSERT(valid_port_bit(port_id, port_bit));

    auto iter = pin_port_port_bit_to_pin_id_.find(std::make_tuple(port_id, port_bit));
    if(iter != pin_port_port_bit_to_pin_id_.end()) {
        AtomPinId pin_id = iter->second;

        //Check post-conditions
        if(pin_id) {
            VTR_ASSERT(valid_pin_id(pin_id));
            VTR_ASSERT(pin_port_bit(pin_id) == port_bit);
        }

        return pin_id;
    } else {
        return AtomPinId::INVALID();
    }
}

AtomNetId AtomNetlist::find_net (const std::string& name) const {
    auto iter = net_name_to_net_id_.find(name);
    if(iter != net_name_to_net_id_.end()) {
        AtomNetId net_id = iter->second;

        if(net_id) {
            //Check post-conditions
            VTR_ASSERT(valid_net_id(net_id));
            VTR_ASSERT(net_name(net_id) == name);
        }

        return iter->second;
    } else {
        return AtomNetId::INVALID();
    }
}

/*
 *
 * Validation
 *
 */
void AtomNetlist::verify() const {
    verify_sizes();
    verify_refs();
    verify_lookups();
}

//Checks that the sizes of internal data structures
//are consistent. Should take constant time.
void AtomNetlist::verify_sizes() const {
    validate_block_sizes();
    validate_port_sizes();
    validate_pin_sizes();
    validate_net_sizes();


}

//Checks that all cross-references are consistent.
//Should take linear time.
void AtomNetlist::verify_refs() const {
    validate_block_port_refs();
    validate_port_pin_refs();
    validate_net_pin_refs();

}

void AtomNetlist::verify_lookups() const {
    //Verify that fast look-ups are consistent

    //Blocks
    for(auto blk_id : blocks()) {
        const auto& name = block_name(blk_id);
        VTR_ASSERT(find_block(name) == blk_id);
    }

    //Ports
    for(auto port_id : port_ids_) {
        auto blk_id = port_block(port_id);
        const auto& name = port_name(port_id);
        VTR_ASSERT(find_port(blk_id, name) == port_id);
    }

    //Pins
    for(auto pin_id : pin_ids_) {
        auto port_id = pin_port(pin_id);
        auto bit = pin_port_bit(pin_id);
        VTR_ASSERT(find_pin(port_id, bit) == pin_id);
    }

    //Nets
    for(auto net_id : nets()) {
        const auto& name = net_name(net_id); 
        VTR_ASSERT(find_net(name) == net_id);
    }

    //Port common
    for(auto common_id : common_ids_) {
        const auto& name = port_common_names_[size_t(common_id)];
        auto type = port_common_types_[size_t(common_id)];
        VTR_ASSERT(find_port_common_id(name, type) == common_id);
    }
}

/*
 *
 * Mutators
 *
 */
AtomBlockId AtomNetlist::create_block(const std::string name, const AtomBlockType blk_type, const t_model* model, const TruthTable truth_table) {
    //Must have a non-mepty name
    VTR_ASSERT_MSG(!name.empty(), "Non-Empty block name");

    //Check if the block has already been created
    AtomBlockId blk_id = find_block(name);

    if(blk_id == AtomBlockId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = AtomBlockId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name);
        block_types_.push_back(blk_type);
        block_models_.push_back(model);
        block_truth_tables_.push_back(truth_table);

        //Initialize the look-ups
        block_name_to_block_id_[name] = blk_id;
        block_input_ports_.emplace_back();
        block_output_ports_.emplace_back();
        block_clock_ports_.emplace_back();
    }

    //Check post-conditions: size
    VTR_ASSERT(block_names_.size() == block_ids_.size());
    VTR_ASSERT(block_types_.size() == block_ids_.size());
    VTR_ASSERT(block_models_.size() == block_ids_.size());
    VTR_ASSERT(block_truth_tables_.size() == block_ids_.size());
    VTR_ASSERT(block_name_to_block_id_.size() == block_ids_.size());
    VTR_ASSERT(block_input_ports_.size() == block_ids_.size());
    VTR_ASSERT(block_output_ports_.size() == block_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_name(blk_id) == name);
    VTR_ASSERT(block_type(blk_id) == blk_type);
    VTR_ASSERT(block_model(blk_id) == model);
    VTR_ASSERT(block_truth_table(blk_id) == truth_table);
    VTR_ASSERT_SAFE(find_block(name) == blk_id);

    return blk_id;
}

AtomPortId  AtomNetlist::create_port (const AtomBlockId blk_id, const std::string& name) {
    //Check pre-conditions
    VTR_ASSERT_MSG(valid_block_id(blk_id), "Valid block id");

    //See if the port already exists
    AtomPortId port_id = find_port(blk_id, name);
    if(!port_id) {
        //Not found, create it

        //Reserve an id
        port_id = AtomPortId(port_ids_.size());
        port_ids_.push_back(port_id);

        //Save the reverse lookup
        auto key = std::make_tuple(blk_id, name);
        block_id_port_name_to_port_id_[key] = port_id;

        //Initialize the per-port-instance data
        port_blocks_.push_back(blk_id);

        //Find the model port
        const t_model_ports* model_port = find_port_model(port_id, name);

        //Allocate the pins, initialize to invalid Ids
        port_pins_.emplace_back();

        //Determine the port type
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
            VTR_ASSERT_MSG(false, "Recognized port type");
        }

        //Associate the port with the blocks inputs/outputs/clocks
        if (type == AtomPortType::INPUT) {
            block_input_ports_[size_t(blk_id)].push_back(port_id);

        } else if (type == AtomPortType::OUTPUT) {
            block_output_ports_[size_t(blk_id)].push_back(port_id);

        } else if (type == AtomPortType::CLOCK) {
            block_clock_ports_[size_t(blk_id)].push_back(port_id);

        } else {
            VTR_ASSERT_MSG(false, "Recognized port type");
        }

        //Initialize the shared/common per port/block type data
        AtomPortCommonId port_common_id = create_port_common(name, type);
        port_common_ids_.push_back(port_common_id);
    }

    //Check post-conditions: sizes
    VTR_ASSERT(port_blocks_.size() == port_ids_.size());
    VTR_ASSERT(port_pins_.size() == port_ids_.size());
    VTR_ASSERT(port_common_ids_.size() == port_ids_.size());
    
    //Check post-conditions: values
    VTR_ASSERT(valid_port_id(port_id));
    VTR_ASSERT(port_block(port_id) == blk_id);
    VTR_ASSERT(port_name(port_id) == name);
    VTR_ASSERT_SAFE(find_port(blk_id, name) == port_id);

    return port_id;
}

AtomNetlist::AtomPortCommonId AtomNetlist::create_port_common(const std::string& name, const AtomPortType type) {
    AtomPortCommonId common_id = find_port_common_id(name, type);
    if(!common_id) {
        //Not found, create

        //Reserve an id
        common_id = AtomPortCommonId(common_ids_.size());
        common_ids_.push_back(common_id);

        //Store the reverse look-up
        auto key = std::make_tuple(name, type);
        port_name_type_to_common_id_[key] = common_id;

        //Initialize the data
        port_common_names_.emplace_back(name);
        port_common_types_.emplace_back(type);
    }

    //Check post-conditions: sizes
    VTR_ASSERT(port_name_type_to_common_id_.size() == common_ids_.size());
    VTR_ASSERT(port_common_names_.size() == common_ids_.size());
    VTR_ASSERT(port_common_types_.size() == common_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(port_common_names_[size_t(common_id)] == name);
    VTR_ASSERT(port_common_types_[size_t(common_id)] == type);
    VTR_ASSERT_SAFE(find_port_common_id(name, type) == common_id);

    return common_id;
}

AtomPinId AtomNetlist::create_pin (const AtomPortId port_id, size_t port_bit, const AtomNetId net_id, const AtomPinType type) {
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

        //Store the reverse look-up
        auto key = std::make_tuple(port_id, port_bit);
        pin_port_port_bit_to_pin_id_[key] = pin_id;

        //Add the pin to the net
        if(type == AtomPinType::DRIVER) {
            VTR_ASSERT_MSG(net_pins_[size_t(net_id)].size() > 0, "Space for net's pin");
            VTR_ASSERT_MSG(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID(), "No existing net driver");
            
            net_pins_[size_t(net_id)][0] = pin_id; //Set driver
        } else {
            net_pins_[size_t(net_id)].emplace_back(pin_id); //Add sink
        }

        //Add the pin to the port
        port_pins_[size_t(port_id)].push_back(pin_id);
    }

    //Check post-conditions: sizes
    VTR_ASSERT(pin_ports_.size() == pin_ids_.size());
    VTR_ASSERT(pin_port_bits_.size() == pin_ids_.size());
    VTR_ASSERT(pin_nets_.size() == pin_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_pin_id(pin_id));
    VTR_ASSERT(pin_port(pin_id) == port_id);
    VTR_ASSERT(pin_port_bit(pin_id) == port_bit);
    VTR_ASSERT(pin_net(pin_id) == net_id);
    VTR_ASSERT(pin_type(pin_id) == type);
    VTR_ASSERT_SAFE(find_pin(port_id, port_bit) == pin_id);
    VTR_ASSERT_SAFE_MSG(std::count(net_pins_[size_t(net_id)].begin(), net_pins_[size_t(net_id)].end(), pin_id) == 1, "No missing or duplicate pins");

    return pin_id;
}

AtomNetId AtomNetlist::create_net (const std::string name) {
    //Creates an empty net (or returns an existing one)
    VTR_ASSERT_MSG(!name.empty(), "Valid net name");

    //Check if the net has already been created
    AtomNetId net_id = find_net(name);
    if(net_id == AtomNetId::INVALID()) {
        //Not found, create it

        //Reserve an id
        net_id = AtomNetId(net_ids_.size());
        net_ids_.push_back(net_id);

        //Initialize the data
        net_names_.push_back(name);

        //Initialize the look-ups
        net_name_to_net_id_[name] = net_id;

        //Initialize with no driver
        net_pins_.emplace_back();
        net_pins_[size_t(net_id)].emplace_back(AtomPinId::INVALID());

        VTR_ASSERT(net_pins_[size_t(net_id)].size() == 1);
        VTR_ASSERT(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID());
    }

    //Check post-conditions: size
    VTR_ASSERT(net_names_.size() == net_ids_.size());
    VTR_ASSERT(net_pins_.size() == net_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_net_id(net_id));
    VTR_ASSERT(net_names_[size_t(net_id)] == name);
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

void AtomNetlist::remove_block(const AtomBlockId blk_id) {
    VTR_ASSERT(valid_block_id(blk_id));
    
    //Remove the ports
    for(auto block_ports : {block_input_ports(blk_id), block_output_ports(blk_id), block_clock_ports(blk_id)}) {
        for(AtomPortId block_port : block_ports) {
            remove_port(block_port);
        }
    }

    //Mark as invalid
    block_ids_[size_t(blk_id)] = AtomBlockId::INVALID();
    block_name_to_block_id_[block_name(blk_id)] = AtomBlockId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

    //Blow the block away
    /*
     *block_name_to_block_id_.erase(block_name(blk_id));
     *block_ids_.erase(block_ids_.begin() + size_t(blk_id));
     *block_names_.erase(block_names_.begin() + size_t(blk_id));
     *block_types_.erase(block_types_.begin() + size_t(blk_id));
     *block_models_.erase(block_models_.begin() + size_t(blk_id));
     *block_truth_tables_.erase(block_truth_tables_.begin() + size_t(blk_id));
     *block_input_ports_.erase(block_input_ports_.begin() + size_t(blk_id));
     *block_output_ports_.erase(block_output_ports_.begin() + size_t(blk_id));
     *block_clock_ports_.erase(block_clock_ports_.begin() + size_t(blk_id));
     */
}

void AtomNetlist::remove_net(const AtomNetId net_id) {
    VTR_ASSERT(valid_net_id(net_id));

    //Dissassociate the pins from the net
    for(auto pin_id : net_pins(net_id)) {
        pin_nets_[size_t(pin_id)] = AtomNetId::INVALID();
    }

    //Mark as invalid
    net_ids_[size_t(net_id)] = AtomNetId::INVALID();
    net_name_to_net_id_[net_name(net_id)] = AtomNetId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

    //Blow away the net
    /*
     *net_name_to_net_id_.erase(net_name(net_id));
     *net_ids_.erase(net_ids_.begin() + size_t(net_id));
     *net_pins_.erase(net_pins_.begin() + size_t(net_id));
     *net_names_.erase(net_names_.begin() + size_t(net_id));
     */
}


void AtomNetlist::remove_port(const AtomPortId port_id) {
    VTR_ASSERT(valid_port_id(port_id));

    //Remove the pins
    for(auto pin : port_pins(port_id)) {
        if(pin) {
            remove_pin(pin);
        }
    }

    //Mark as invalid
    port_ids_[size_t(port_id)] = AtomPortId::INVALID();
    block_id_port_name_to_port_id_[std::make_tuple(port_block(port_id), port_name(port_id))] = AtomPortId::INVALID();

    //Mark netlist dirty
    dirty_ = true;
    //Blow the port away
    /*
     *block_id_port_name_to_port_id_.erase(std::make_tuple(port_block(port_id), port_name(port_id)));
     *port_ids_.erase(port_ids_.begin() + size_t(port_id));
     *port_blocks_.erase(port_blocks_.begin() + size_t(port_id));
     *port_pins_.erase(port_pins_.begin() + size_t(port_id));
     */

    //Note that we currently don't bother cleaning up the port_common* items
    //(they will change rarely and aren't directly iterable by users)
}

void AtomNetlist::remove_pin(const AtomPinId pin_id) {
    VTR_ASSERT(valid_pin_id(pin_id));

    //Find the associated net
    AtomNetId net = pin_net(pin_id);

    //Remove the pin from the associated net
    remove_net_pin(net, pin_id);

    //Mark as invalid
    pin_ids_[size_t(pin_id)] = AtomPinId::INVALID();
    pin_port_port_bit_to_pin_id_[std::make_tuple(pin_port(pin_id), pin_port_bit(pin_id))] = AtomPinId::INVALID();

    //Mark netlist dirty
    dirty_ = true;

    //Blow away the pin
    /*
     *pin_port_port_bit_to_pin_id_.erase(std::make_tuple(pin_port(pin_id), pin_port_bit(pin_id)));
     *pin_ids_.erase(pin_ids_.begin() + size_t(pin_id));
     *pin_ports_.erase(pin_ports_.begin() + size_t(pin_id));
     *pin_port_bits_.erase(pin_port_bits_.begin() + size_t(pin_id));
     *pin_nets_.erase(pin_nets_.begin() + size_t(pin_id));
     */
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

void AtomNetlist::compress() {
    //Compress the various netlist components to remove invalid entries
    // Note: this invalidates all Ids

    //The clean_*() functions return a vector which maps from old to new index
    // e.g. block_id_map[old_id] == new_id
    auto block_id_map = clean_blocks();
    auto port_id_map = clean_ports();
    auto pin_id_map = clean_pins();
    auto net_id_map = clean_nets();

    //Now we re-build all the cross references
    rebuild_block_refs(port_id_map);
    rebuild_port_refs(block_id_map, pin_id_map);
    rebuild_pin_refs(port_id_map, net_id_map);
    rebuild_net_refs(pin_id_map);

    //Netlist is now clean
    dirty_ = false;
}

std::vector<AtomBlockId> AtomNetlist::clean_blocks() {
    std::vector<AtomBlockId> block_id_map;
    std::vector<AtomBlockId> new_ids;
    std::tie(new_ids, block_id_map) = compress_ids(block_ids_);

    //Move all the valid values
    block_names_ = move_valid(block_names_, block_ids_);
    block_types_ = move_valid(block_types_, block_ids_);
    block_models_ = move_valid(block_models_, block_ids_);
    block_truth_tables_ = move_valid(block_truth_tables_, block_ids_);
    block_input_ports_ = move_valid(block_input_ports_, block_ids_);
    block_output_ports_ = move_valid(block_output_ports_, block_ids_);
    block_clock_ports_ = move_valid(block_clock_ports_, block_ids_);

    //Update Ids last since used as predicate
    block_ids_ = new_ids;

    VTR_ASSERT_SAFE_MSG(are_contiguous(block_ids_), "IDs should be contiguous");

    VTR_ASSERT_SAFE(all_valid(block_ids_));

    return block_id_map;
}

std::vector<AtomPortId> AtomNetlist::clean_ports() {
    std::vector<AtomPortId> port_id_map;
    std::vector<AtomPortId> new_ids;
    std::tie(new_ids, port_id_map) = compress_ids(port_ids_);

    //Copy all the valid values
    port_blocks_ = move_valid(port_blocks_, port_ids_);
    port_pins_ = move_valid(port_pins_, port_ids_);
    port_common_ids_ = move_valid(port_common_ids_, port_ids_);

    //Update Ids last since used as predicate
    port_ids_ = new_ids;

    VTR_ASSERT_SAFE_MSG(are_contiguous(port_ids_), "IDs should be contiguous");

    VTR_ASSERT_SAFE(all_valid(port_ids_));

    return port_id_map;
}

std::vector<AtomPinId> AtomNetlist::clean_pins() {
    //When we remove a net we may leave the pins dangling,
    //so mark such pins invalid
    for(size_t i = 0; i < pin_nets_.size(); ++i) {
        if(!pin_nets_[i]) {
            //Dangling pin (no associated net)

            //Mark as invalid so it will be removed
            pin_ids_[i] = AtomPinId::INVALID();
        }
    }


    std::vector<AtomPinId> pin_id_map;
    std::vector<AtomPinId> new_ids;
    std::tie(new_ids, pin_id_map) = compress_ids(pin_ids_);

    //Copy all the valid values
    pin_ports_ = move_valid(pin_ports_, pin_ids_);
    pin_port_bits_ = move_valid(pin_port_bits_, pin_ids_);
    pin_nets_ = move_valid(pin_nets_, pin_ids_);

    //Update Ids last since used as predicate
    pin_ids_ = new_ids;

    VTR_ASSERT_SAFE_MSG(are_contiguous(pin_ids_), "IDs should be contiguous");

    VTR_ASSERT_SAFE(all_valid(pin_ids_));

    return pin_id_map;
}

std::vector<AtomNetId> AtomNetlist::clean_nets() {
    std::vector<AtomNetId> net_id_map;
    std::vector<AtomNetId> new_ids;
    std::tie(new_ids, net_id_map) = compress_ids(net_ids_);

    //Copy all the valid values
    net_names_ = move_valid(net_names_, net_ids_);
    net_pins_ = move_valid(net_pins_, net_ids_);

    //Update Ids last since used as predicate
    net_ids_ = new_ids;

    VTR_ASSERT_SAFE_MSG(are_contiguous(net_ids_), "IDs should be contiguous");

    VTR_ASSERT_SAFE(all_valid(net_ids_));

    return net_id_map;
}

void AtomNetlist::rebuild_block_refs(const std::vector<AtomPortId>& port_id_map) {
    //Update the port id references held by blocks
    for(std::vector<AtomPortId>& port : block_input_ports_) {
        port = update_refs(port, port_id_map);
        VTR_ASSERT_SAFE(all_valid(port));
    }
    for(std::vector<AtomPortId>& port : block_output_ports_) {
        port = update_refs(port, port_id_map);
        VTR_ASSERT_SAFE(all_valid(port));
    }
    for(std::vector<AtomPortId>& port : block_clock_ports_) {
        port = update_refs(port, port_id_map);
        VTR_ASSERT_SAFE(all_valid(port));
    }
}

void AtomNetlist::rebuild_port_refs(const std::vector<AtomBlockId>& block_id_map, const std::vector<AtomPinId>& pin_id_map) {
    //Update block and pin references held by ports
    port_blocks_ = update_refs(port_blocks_, block_id_map); 
    VTR_ASSERT_SAFE(all_valid(port_blocks_));

    for(auto& pins : port_pins_) {
        pins = update_refs(pins, pin_id_map);
        VTR_ASSERT_SAFE(all_valid(pins));
    }
}

void AtomNetlist::rebuild_pin_refs(const std::vector<AtomPortId>& port_id_map, const std::vector<AtomNetId>& net_id_map) {
    //Update port and net references held by pins
    pin_ports_ = update_refs(pin_ports_, port_id_map);
    VTR_ASSERT_SAFE(all_valid(pin_ports_));

    pin_nets_ = update_refs(pin_nets_, net_id_map);
    VTR_ASSERT_SAFE(all_valid(pin_nets_));
}

void AtomNetlist::rebuild_net_refs(const std::vector<AtomPinId>& pin_id_map) {
    //Update pin references held by nets
    for(auto& pins : net_pins_) {
        pins = update_refs(pins, pin_id_map);

        VTR_ASSERT_SAFE_MSG(all_valid(pins), "Only valid sinks");
    }
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

bool AtomNetlist::valid_port_bit(AtomPortId id, size_t port_bit) const {
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

void AtomNetlist::validate_block_sizes() const {
    VTR_ASSERT(block_names_.size() == block_ids_.size());
    VTR_ASSERT(block_types_.size() == block_ids_.size());
    VTR_ASSERT(block_models_.size() == block_ids_.size());
    VTR_ASSERT(block_truth_tables_.size() == block_ids_.size());
}

void AtomNetlist::validate_port_sizes() const {
    VTR_ASSERT(port_blocks_.size() == port_ids_.size());
    VTR_ASSERT(port_pins_.size() == port_ids_.size());
    VTR_ASSERT(port_common_ids_.size() == port_ids_.size());

    VTR_ASSERT(port_common_names_.size() == common_ids_.size());
    VTR_ASSERT(port_common_names_.size() == common_ids_.size());
}

void AtomNetlist::validate_pin_sizes() const {
    VTR_ASSERT(pin_ports_.size() == pin_ids_.size());
    VTR_ASSERT(pin_port_bits_.size() == pin_ids_.size());
    VTR_ASSERT(pin_nets_.size() == pin_ids_.size());
}

void AtomNetlist::validate_net_sizes() const {
    VTR_ASSERT(net_names_.size() == net_ids_.size());
    VTR_ASSERT(net_pins_.size() == net_ids_.size());
}

void AtomNetlist::validate_block_port_refs() const {
    //Verify that all block <-> port references are consistent

    //Track how many times we've seen each port from the blocks 
    std::unordered_multiset<AtomPortId> seen_port_ids;

    for(auto blk_id : blocks()) {
        for(auto in_port_id : block_input_ports(blk_id)) {
            VTR_ASSERT(blk_id == port_block(in_port_id));
            seen_port_ids.insert(in_port_id);
        }
        for(auto out_port_id : block_output_ports(blk_id)) {
            VTR_ASSERT(blk_id == port_block(out_port_id));
            seen_port_ids.insert(out_port_id);
        }
        for(auto clock_port_id : block_clock_ports(blk_id)) {
            VTR_ASSERT(blk_id == port_block(clock_port_id));
            seen_port_ids.insert(clock_port_id);
        }
    }

    //Check that we have no orphaned ports (i.e. that aren't referenced by blocks) 
    //or shared ports (i.e. referenced by multiple blocks)
    for(auto port_id : port_ids_) {
        VTR_ASSERT_MSG(seen_port_ids.count(port_id) == 1, "Port referenced by a single block");
    }
    VTR_ASSERT_MSG(seen_port_ids.size() == port_ids_.size(), "All ports checked");
}

void AtomNetlist::validate_port_pin_refs() const {
    //Check that port <-> pin references are consistent

    //Track how many times we've seen each pin from the ports
    std::unordered_multiset<AtomPinId> seen_pin_ids;

    for(auto port_id : port_ids_) {
        for(auto pin_id : port_pins(port_id)) {
            VTR_ASSERT(pin_port(pin_id) == port_id);
            VTR_ASSERT(pin_port_bit(pin_id) < port_width(port_id));
            seen_pin_ids.insert(pin_id);
        }
    }

    //Check that we have no orphaned pins (i.e. that aren't referenced by a port) 
    //or shared pins (i.e. referenced by multiple ports) 
    for(auto pin_id : pin_ids_) {
        VTR_ASSERT_MSG(seen_pin_ids.count(pin_id) == 1, "Pin referenced by a single port");
    }
    VTR_ASSERT_MSG(seen_pin_ids.size() == pin_ids_.size(), "All port pins checked");
}

void AtomNetlist::validate_net_pin_refs() const {
    //Check that net <-> pin references are consistent

    //Track how often we see a pin from the nets
    std::unordered_multiset<AtomPinId> seen_pin_ids;

    for(auto net_id : nets()) {
        auto pins = net_pins(net_id);
        for(auto iter = pins.begin(); iter != pins.end(); ++iter) {
            auto pin_id = *iter;
            if(iter != pins.begin()) {
                //The first net pin is the driver, which may be invalid
                //if there is no driver. So we only check for a valid id
                //on the other net pins (which are all sinks and must be valid)
                VTR_ASSERT(pin_id);
            }

            if(pin_id) {
                //Verify the cross reference if the pin_id is valid (i.e. a sink or a valid driver)
                VTR_ASSERT(pin_net(pin_id) == net_id);

                //We only record valid seen pins since we may see multiple undriven nets with invalid IDs
                seen_pin_ids.insert(pin_id);
            }
        }
    }

    //Check that we have no orphaned pins (i.e. that aren't referenced by a net) 
    //or shared pins (i.e. referenced by multiple nets) 
    for(auto pin_id : pin_ids_) {
        VTR_ASSERT_MSG(seen_pin_ids.count(pin_id) == 1, "Pin referenced by a single net");
    }
    VTR_ASSERT_MSG(seen_pin_ids.size() == pin_ids_.size(), "All net pins checked");
}


/*
 *
 * Internal utilities
 *
 */
AtomNetlist::AtomPortCommonId AtomNetlist::find_port_common_id (const std::string& name, const AtomPortType type) const {
    auto iter = port_name_type_to_common_id_.find(std::make_tuple(name, type));
    if(iter != port_name_type_to_common_id_.end()) {
        return iter->second;
    } else {
        return AtomPortCommonId::INVALID();
    }
}
AtomNetlist::AtomPortCommonId AtomNetlist::find_port_common_id (const AtomPortId id) const {
    return port_common_ids_[size_t(id)];
}

const t_model_ports* AtomNetlist::find_port_model(const AtomPortId id, const std::string& name) const {
    AtomBlockId blk_id = port_block(id);
    const t_model* blk_model = block_model(blk_id);

    const t_model_ports* model_port = nullptr;
    for(const t_model_ports* blk_ports : {blk_model->inputs, blk_model->outputs}) {
        model_port = blk_ports;
        while(model_port) {
            if(name == model_port->name) {
                //Found
                break;
            }
            model_port = model_port->next;
        }
        if(model_port) {
            //Found
            break;
        }
    }
    VTR_ASSERT_MSG(model_port, "Found model port");
    VTR_ASSERT_MSG(model_port->size >= 0, "Positive port width");
    return model_port;
}


/*
 *
 * Non-member functions
 *
 */
