#include "netlist2.h"
#include <map>

#include "vtr_assert.h"

AtomNetlist::AtomNetlist(std::string name)
    : netlist_name_(name) {}

const std::string& AtomNetlist::netlist_name() const {
    return netlist_name_;
}

bool AtomNetlist::is_blackbox() const {
    return is_blackbox_;
}

const std::string& AtomNetlist::block_name (const AtomBlkId id) const { 
    return block_names_[size_t(id)];
}

AtomBlkType AtomNetlist::block_type (const AtomBlkId id) const {
    return block_types_[size_t(id)];
}

const t_model* AtomNetlist::block_model (const AtomBlkId id) const {
    return block_models_[size_t(id)];
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::block_input_pins (const AtomBlkId id) const {
    return vtr::make_range(block_input_pins_[size_t(id)].begin(), block_input_pins_[size_t(id)].end());
}

vtr::Range<AtomNetlist::pin_iterator> AtomNetlist::block_output_pins (const AtomBlkId id) const {
    return vtr::make_range(block_output_pins_[size_t(id)].begin(), block_output_pins_[size_t(id)].end());
}


//Pin
AtomBlkId AtomNetlist::pin_block (const AtomPinId id) const { 
    return pin_blocks_[size_t(id)];
}

AtomNetId AtomNetlist::pin_net (const AtomPinId id) const { 
    return pin_nets_[size_t(id)];
}

AtomPinType AtomNetlist::pin_type (const AtomPinId id) const { 
    return pin_types_[size_t(id)];
}
const std::string& AtomNetlist::pin_name (const AtomPinId id) const { 
    return pin_names_[pin_name_ids_[size_t(id)]];
}


//Net
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
        VTR_ASSERT(net_names_[size_t(net_id)] == name);

        return iter->second;
    } else {
        return AtomNetId::INVALID();
    }
}

AtomPinId AtomNetlist::find_pin (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType atom_pin_type, const std::string& name) const {
    auto iter = pin_blk_net_type_name_to_id_.find(std::make_tuple(blk_id,net_id,atom_pin_type, name));
    if(iter != pin_blk_net_type_name_to_id_.end()) {
        AtomPinId pin_id = iter->second;

        //Check post-conditions
        VTR_ASSERT(valid_pin_id(pin_id));
        VTR_ASSERT(pin_blocks_[size_t(pin_id)] == blk_id);
        VTR_ASSERT(pin_nets_[size_t(pin_id)] == net_id);
        VTR_ASSERT(pin_types_[size_t(pin_id)] == atom_pin_type);
        VTR_ASSERT(pin_names_[pin_name_ids_[size_t(pin_id)]] == name);

        return pin_id;
    } else {
        return AtomPinId::INVALID();
    }
}

AtomBlkId AtomNetlist::find_block (const std::string& name) const {
    auto iter = block_name_to_id_.find(name);
    if(iter != block_name_to_id_.end()) {
        AtomBlkId blk_id = iter->second;

        //Check post-conditions
        VTR_ASSERT(valid_block_id(blk_id));
        VTR_ASSERT(block_names_[size_t(blk_id)] == name);

        return blk_id;
    } else {
        return AtomBlkId::INVALID();
    }
}

//Sanity Checks
bool AtomNetlist::valid_block_id(AtomBlkId id) const {
    if(id == AtomBlkId::INVALID()) return false;
    else if(size_t(id) >= block_ids_.size()) return false;
    else if(block_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_net_id(AtomNetId id) const {
    if(id == AtomNetId::INVALID()) return false;
    else if(size_t(id) >= net_ids_.size()) return false;
    else if(net_ids_[size_t(id)] != id) return false;
    return true;
}

bool AtomNetlist::valid_pin_id(AtomPinId id) const {
    if(id == AtomPinId::INVALID()) return false;
    else if(size_t(id) >= pin_ids_.size()) return false;
    else if(pin_ids_[size_t(id)] != id) return false;
    return true;
}


//Mutators
AtomBlkId AtomNetlist::create_block(const std::string name, const AtomBlkType blk_type, const t_model* model, const TruthTable truth_table) {
    //Must have a non-mepty name
    VTR_ASSERT_MSG(!name.empty(), "Empty block name");

    //Check if the block has already been created
    AtomBlkId blk_id = find_block(name);

    if(blk_id == AtomBlkId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = AtomBlkId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name);
        block_types_.push_back(blk_type);
        block_models_.push_back(model);
        block_truth_tables_.push_back(truth_table);

        //Initialize the look-ups
        block_name_to_id_.insert(std::make_pair(name, blk_id));
        block_input_pins_.emplace_back();
        block_output_pins_.emplace_back();
    }

    //Check post-conditions: size
    VTR_ASSERT(block_names_.size() == block_ids_.size());
    VTR_ASSERT(block_types_.size() == block_ids_.size());
    VTR_ASSERT(block_models_.size() == block_ids_.size());
    VTR_ASSERT(block_truth_tables_.size() == block_ids_.size());
    VTR_ASSERT(block_name_to_id_.size() == block_ids_.size());
    VTR_ASSERT(block_input_pins_.size() == block_ids_.size());
    VTR_ASSERT(block_output_pins_.size() == block_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_names_[size_t(blk_id)] == name);
    VTR_ASSERT(block_types_[size_t(blk_id)] == blk_type);
    VTR_ASSERT(block_models_[size_t(blk_id)] == model);
    VTR_ASSERT(block_truth_tables_[size_t(blk_id)] == truth_table);
    VTR_ASSERT(find_block(name) == blk_id);

    return blk_id;
}

AtomNetId AtomNetlist::create_net (const std::string name) {
    VTR_ASSERT_MSG(!name.empty(), "Empty net name");

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
        net_name_to_id_.insert(std::make_pair(name, net_id));

        //Initialize with no driver
        net_pins_.emplace_back();
        net_pins_[size_t(net_id)].emplace_back(AtomPinId::INVALID());

        VTR_ASSERT(net_pins_[size_t(net_id)].size() == 1);
        VTR_ASSERT(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID());
    }

    //Check post-conditions: size
    VTR_ASSERT(net_names_.size() == net_ids_.size());
    VTR_ASSERT(net_name_to_id_.size() == net_ids_.size());
    VTR_ASSERT(net_pins_.size() == net_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_net_id(net_id));
    VTR_ASSERT(net_names_[size_t(net_id)] == name);
    VTR_ASSERT(find_net(name) == net_id);

    return net_id;

}

AtomPinId AtomNetlist::create_pin (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType atom_pin_type, const std::string name) {
    //Check pre-conditions (valid ids)
    VTR_ASSERT_MSG(valid_block_id(blk_id), "Invalid block id");
    VTR_ASSERT_MSG(valid_net_id(net_id), "Invalid block id");

    //See if the pin already exists
    AtomPinId pin_id = find_pin(blk_id, net_id, atom_pin_type, name);
    if(pin_id == AtomPinId::INVALID()) {
        //Not found, create it

        //Reserve an id
        pin_id = AtomPinId(pin_ids_.size());
        pin_ids_.push_back(pin_id);

        //Initialize the pin data
        pin_blocks_.push_back(blk_id);
        pin_nets_.push_back(net_id);
        pin_types_.push_back(atom_pin_type);

        //Store the reverse look-up
        auto key = std::make_tuple(blk_id, net_id, atom_pin_type, name);
        pin_blk_net_type_name_to_id_.insert(std::make_pair(key, pin_id));

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

        //Add the pin to the net
        if(atom_pin_type == AtomPinType::DRIVER) {
            VTR_ASSERT(net_pins_[size_t(net_id)].size() > 0); //Space reserved
            VTR_ASSERT(net_pins_[size_t(net_id)][0] == AtomPinId::INVALID()); //No existing driver
            
            net_pins_[size_t(net_id)][0] = pin_id;
        } else {
            net_pins_[size_t(net_id)].emplace_back(pin_id);
        }

        //Add the pin to the block pin look-up
        // We first need to figure the pin type and block
        if(atom_pin_type == AtomPinType::DRIVER) {
            block_output_pins_[size_t(blk_id)].push_back(pin_id);    
        } else {
            VTR_ASSERT(atom_pin_type == AtomPinType::SINK);
            block_input_pins_[size_t(blk_id)].push_back(pin_id);    
        }
    }

    //Check post-conditions: sizes
    VTR_ASSERT(pin_blocks_.size() == pin_ids_.size());
    VTR_ASSERT(pin_nets_.size() == pin_ids_.size());
    VTR_ASSERT(pin_types_.size() == pin_ids_.size());
    VTR_ASSERT(pin_name_ids_.size() == pin_ids_.size());
    VTR_ASSERT(pin_blk_net_type_name_to_id_.size() == pin_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(valid_pin_id(pin_id));
    VTR_ASSERT(pin_blocks_[size_t(pin_id)] == blk_id);
    VTR_ASSERT(pin_nets_[size_t(pin_id)] == net_id);
    VTR_ASSERT(pin_types_[size_t(pin_id)] == atom_pin_type);
    VTR_ASSERT(pin_names_[pin_name_ids_[size_t(pin_id)]] == name);
    VTR_ASSERT(find_pin(blk_id, net_id, atom_pin_type, name) == pin_id);
    VTR_ASSERT_SAFE_MSG(std::count(net_pins_[size_t(net_id)].begin(), net_pins_[size_t(net_id)].end(), pin_id) == 1, "Invalid net (missing or duplicate pin)");

    return pin_id;
}

void AtomNetlist::set_blackbox(bool val) {
    is_blackbox_ = val;
}

AtomNetlist::AtomPinNameId AtomNetlist::find_pin_name_id(const std::string& name) {
    auto iter = pin_name_to_name_id_.find(name);
    if(iter != pin_name_to_name_id_.end()) {
        return iter->second;
    } else {
        return INVALID_PIN_NAME_ID;
    }
}

void print_netlist(FILE* f, const AtomNetlist& netlist) {

    //Build a map of the blocks by type
    std::multimap<AtomBlkType,AtomBlkId> blocks_by_type;
    for(AtomBlkId blk_id : netlist.blocks()) {
        blocks_by_type.insert({netlist.block_type(blk_id), blk_id});
    }

    //Iterating through the map ensures blocks of the same type are printed together
    for(auto kv : blocks_by_type) {
        AtomBlkType type = kv.first;
        AtomBlkId blk_id = kv.second;
        const t_model* model = netlist.block_model(blk_id);
        fprintf(f, "Block %s", model->name);
        fprintf(f, " (");
        switch(type) {
            case AtomBlkType::INPAD : fprintf(f, "INPAD"); break;
            case AtomBlkType::OUTPAD: fprintf(f, "OUTPAD"); break;
            case AtomBlkType::COMBINATIONAL: fprintf(f, "COMBINATIONAL"); break;
            case AtomBlkType::SEQUENTIAL: fprintf(f, "SEQUENTIAL"); break;
            default: VTR_ASSERT_MSG(false, "Unrecognzied AtomBlkType");
        }
        fprintf(f, "):");
        fprintf(f, " %s\n", netlist.block_name(blk_id).c_str());

        for(auto pin_id : netlist.block_input_pins(blk_id)) {
            fprintf(f, "\tInput %s <- %s\n", netlist.pin_name(pin_id).c_str(), netlist.net_name(netlist.pin_net(pin_id)).c_str());
        }
        for(auto pin_id : netlist.block_output_pins(blk_id)) {
            fprintf(f, "\tOutput %s -> %s\n", netlist.pin_name(pin_id).c_str(), netlist.net_name(netlist.pin_net(pin_id)).c_str());
        }
    }

    //Print out per-net information
    for(auto net_id : netlist.nets()) {
        auto sinks = netlist.net_sinks(net_id);
        fprintf(f, "Net %s (fanout %zu)\n", netlist.net_name(net_id).c_str(), sinks.size());

        AtomPinId driver_pin_id = netlist.net_driver(net_id);
        if(driver_pin_id != AtomPinId::INVALID()) {
            printf("\tDriver: %s %s\n", netlist.block_name(netlist.pin_block(driver_pin_id)).c_str(), netlist.pin_name(driver_pin_id).c_str());
        } else {
            printf("\tDriver: \n");
        }

        for(auto sink_pin_id : sinks) {
            printf("\tSink  : %s %s\n", netlist.block_name(netlist.pin_block(sink_pin_id)).c_str(), netlist.pin_name(sink_pin_id).c_str());
        }
    }
}
