#include "netlist2.h"

#include "vtr_assert.h"

const std::string& AtomNetlist::netlist_name() const {
    return netlist_name_;
}

const std::string& AtomNetlist::block_name (const AtomBlkId id) const { 
    return block_names_[id];
}

AtomBlockType AtomNetlist::block_type (const AtomBlkId id) const {
    return block_types_[id];
}

const t_model* AtomNetlist::block_model (const AtomBlkId id) const {
    return block_models_[id];
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::block_input_pins (const AtomBlkId id) const {
    return vtr::make_range(block_output_pins_[id].begin(), block_output_pins_[id].end());
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::block_output_pins (const AtomBlkId id) const {
    return vtr::make_range(block_input_pins_[id].begin(), block_input_pins_[id].end());
}


//Pin
AtomBlkId AtomNetlist::pin_block (const AtomPinId id) const { 
    return pin_blocks_[id];
}

AtomNetId AtomNetlist::pin_net (const AtomPinId id) const { 
    return pin_nets_[id];
}

AtomPinType AtomNetlist::pin_type (const AtomPinId id) const { 
    return pin_types_[id];
}


//Net
const std::string& AtomNetlist::net_name (const AtomNetId id) const { 
    return net_names_[id];
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::net_pins (const AtomNetId id) const {
    return vtr::make_range(net_pins_[id].begin(), net_pins_[id].end());
}

AtomPinId AtomNetlist::net_driver (const AtomNetId id) const {
    if(net_pins_[id].size() > 0) {
        return net_pins_[id][0];
    } else {
        return INVALID_ATOM_PIN;
    }
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::net_sinks (const AtomNetId id) const {
    return vtr::make_range(++net_pins_[id].begin(), net_pins_[id].end());
}


//Aggregates
vtr::Range<AtomNetlist::blk_iterator> AtomNetlist::blocks () const {
    return vtr::make_range(block_ids_.begin(), block_ids_.end()); 
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::pins () const {
    return vtr::make_range(pin_ids_.begin(), pin_ids_.end()); 
}

vtr::Range<AtomNetlist::net_iterator> AtomNetlist::nets () const {
    return vtr::make_range(net_ids_.begin(), net_ids_.end()); 
}


//Lookups
AtomNetId AtomNetlist::find_net (const std::string& name) const {
    auto iter = net_name_to_id_.find(name);
    if(iter != net_name_to_id_.end()) {
        AtomNetId net_id = iter->second;

        //Check post-conditions
        VTR_ASSERT(valid_net_id(net_id));
        VTR_ASSERT(net_names_[net_id] == name);

        return iter->second;
    } else {
        return INVALID_ATOM_NET;
    }
}

AtomPinId AtomNetlist::find_pin (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType atom_pin_type) const {
    auto iter = pin_blk_net_type_to_id_.find(std::make_tuple(blk_id,net_id,atom_pin_type));
    if(iter != pin_blk_net_type_to_id_.end()) {
        AtomPinId pin_id = iter->second;

        //Check post-conditions
        VTR_ASSERT(valid_pin_id(pin_id));
        VTR_ASSERT(pin_blocks_[pin_id] == blk_id);
        VTR_ASSERT(pin_nets_[pin_id] == net_id);
        VTR_ASSERT(pin_types_[pin_id] == atom_pin_type);

        return pin_id;
    } else {
        return INVALID_ATOM_PIN;
    }
}

AtomBlkId AtomNetlist::find_block (const std::string& name) const {
    auto iter = block_name_to_id_.find(name);
    if(iter != block_name_to_id_.end()) {
        AtomBlkId blk_id = iter->second;

        //Check post-conditions
        VTR_ASSERT(valid_block_id(blk_id));
        VTR_ASSERT(block_names_[blk_id] == name);

        return blk_id;
    } else {
        return INVALID_ATOM_BLK;
    }
}

//Sanity Checks
bool AtomNetlist::valid_block_id(AtomBlkId id) const {
    if(id < 0) return false;
    else if(static_cast<size_t>(id) >= block_ids_.size()) return false;
    else if(block_ids_[id] != id) return false;
    return true;
}

bool AtomNetlist::valid_net_id(AtomNetId id) const {
    if(id < 0) return false;
    else if(static_cast<size_t>(id) >= net_ids_.size()) return false;
    else if(net_ids_[id] != id) return false;
    return true;
}

bool AtomNetlist::valid_pin_id(AtomPinId id) const {
    if(id < 0) return false;
    else if(static_cast<size_t>(id) >= pin_ids_.size()) return false;
    else if(pin_ids_[id] != id) return false;
    return true;
}


//Mutators
void AtomNetlist::set_netlist_name(const std::string& name) {
    netlist_name_ = name;
}

AtomBlkId AtomNetlist::create_block(const std::string name, const AtomBlockType blk_type, const t_model* model) {
    //First check if the block has already been created
    AtomBlkId blk_id = find_block(name);

    if(blk_id == INVALID_ATOM_BLK) {
        //Not found, create it

        //Reserve an id
        blk_id = block_ids_.size();
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name);
        block_types_.push_back(blk_type);
        block_models_.push_back(model);

        //Initialize the look-ups
        block_name_to_id_.insert(std::make_pair(name, blk_id));
        block_input_pins_.emplace_back();
        block_output_pins_.emplace_back();
    }

    //Check post-conditions: size
    VTR_ASSERT(block_names_.size() == block_ids_.size());
    VTR_ASSERT(block_types_.size() == block_ids_.size());
    VTR_ASSERT(block_models_.size() == block_ids_.size());
    VTR_ASSERT(block_name_to_id_.size() == block_ids_.size());
    VTR_ASSERT(block_input_pins_.size() == block_ids_.size());
    VTR_ASSERT(block_output_pins_.size() == block_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_names_[blk_id] == name);
    VTR_ASSERT(block_types_[blk_id] == blk_type);
    VTR_ASSERT(block_models_[blk_id] == model);
    VTR_ASSERT(find_block(name) == blk_id);

    return blk_id;
}

AtomNetId AtomNetlist::create_net (const std::string name) {
    //First check if the net has already been created
    AtomNetId net_id = find_net(name);
    if(net_id == INVALID_ATOM_NET) {
        //Not found, create it

        //Reserve an id
        net_id = net_ids_.size();
        net_ids_.push_back(net_id);

        //Initialize the data
        net_names_.push_back(name);

        //Initialize the look-ups
        net_name_to_id_.insert(std::make_pair(name, net_id));
    }

    //Check post-conditions: size
    VTR_ASSERT(net_names_.size() == net_ids_.size());
    VTR_ASSERT(net_name_to_id_.size() == net_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_net_id(net_id));
    VTR_ASSERT(net_names_[net_id] == name);
    VTR_ASSERT(find_net(name) == net_id);

    return net_id;

}

AtomPinId AtomNetlist::create_pin (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType atom_pin_type, const std::string name) {
    //Check pre-conditions (valid ids)
    VTR_ASSERT_MSG(block_ids_[blk_id] == blk_id, "Invalid block id");
    VTR_ASSERT_MSG(net_ids_[net_id] == net_id, "Invalid block id");

    //See if the pin already exists
    AtomPinId pin_id = find_pin(blk_id, net_id, atom_pin_type);
    if(pin_id == INVALID_ATOM_PIN) {
        //Not found, create it

        //Reserve an id
        pin_id = pin_ids_.size();
        pin_ids_.push_back(pin_id);

        //Initialize the pin data
        pin_blocks_.push_back(blk_id);
        pin_nets_.push_back(net_id);
        pin_types_.push_back(atom_pin_type);

        //Store the reverse look-up
        auto key = std::make_tuple(blk_id, net_id, atom_pin_type);
        pin_blk_net_type_to_id_.insert(std::make_pair(key, pin_id));

        //Determine if a name for this pin is already stored
        // We expect names to not be unique amoung pins, so we
        // store each pin name once, and each pin reference the name by an ID
        AtomPinNameId pin_name_id = find_pin_name_id(name);
        if(pin_name_id == INVALID_PIN_NAME_ID) {
            //Not found store name
            pin_name_id = pin_names_.size(); //Reserve a name id
            pin_names_.push_back(name); //Store the name

            pin_name_to_name_id_.insert(std::make_pair(name, pin_name_id)); //Store the reverse look-up
        }
        pin_name_ids_.push_back(pin_name_id); //Store the pin to name_id look-up

        //Add the pin to the block pin look-up
        // We first need to figure the pin type and block
        if(atom_pin_type == AtomPinType::DRIVER) {
            block_output_pins_[blk_id].push_back(pin_id);    
        } else {
            VTR_ASSERT(atom_pin_type == AtomPinType::SINK);
            block_input_pins_[blk_id].push_back(pin_id);    
        }
    }

    //Check post-conditions: sizes
    VTR_ASSERT(pin_blocks_.size() == pin_ids_.size());
    VTR_ASSERT(pin_nets_.size() == pin_ids_.size());
    VTR_ASSERT(pin_types_.size() == pin_ids_.size());
    VTR_ASSERT(pin_name_ids_.size() == pin_ids_.size());
    VTR_ASSERT(pin_blk_net_type_to_id_.size() == pin_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_pin_id(pin_id));
    VTR_ASSERT(pin_blocks_[pin_id] == blk_id);
    VTR_ASSERT(pin_nets_[pin_id] == net_id);
    VTR_ASSERT(pin_types_[pin_id] == atom_pin_type);
    VTR_ASSERT(pin_names_[pin_name_ids_[pin_id]] == name);
    VTR_ASSERT(find_pin(blk_id, net_id, atom_pin_type) == pin_id);

    return pin_id;
}

AtomNetlist::AtomPinNameId AtomNetlist::find_pin_name_id(const std::string& name) {
    auto iter = pin_name_to_name_id_.find(name);
    if(iter != pin_name_to_name_id_.end()) {
        return iter->second;
    } else {
        return INVALID_PIN_NAME_ID;
    }
}
