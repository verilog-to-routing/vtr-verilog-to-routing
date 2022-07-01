#include "clustered_netlist.h"

#include "vtr_assert.h"
#include "vpr_error.h"

/**
 * @file
 * @brief ClusteredNetlist Class Implementation
 */

ClusteredNetlist::ClusteredNetlist(std::string name, std::string id)
    : Netlist<ClusterBlockId, ClusterPortId, ClusterPinId, ClusterNetId>(name, id) {}

/*
 *
 * Blocks
 *
 */
t_pb* ClusteredNetlist::block_pb(const ClusterBlockId id) const {
    VTR_ASSERT_SAFE(valid_block_id(id));

    return block_pbs_[id];
}

t_logical_block_type_ptr ClusteredNetlist::block_type(const ClusterBlockId id) const {
    VTR_ASSERT_SAFE(valid_block_id(id));

    return block_types_[id];
}

ClusterNetId ClusteredNetlist::block_net(const ClusterBlockId blk_id, const int logical_pin_index) const {
    auto pin_id = block_pin(blk_id, logical_pin_index);

    if (pin_id) {
        return pin_net(pin_id);
    }

    return ClusterNetId::INVALID();
}

int ClusteredNetlist::block_pin_net_index(const ClusterBlockId blk_id, const int pin_index) const {
    auto pin_id = block_pin(blk_id, pin_index);

    if (pin_id) {
        return pin_net_index(pin_id);
    }

    return OPEN;
}

ClusterPinId ClusteredNetlist::block_pin(const ClusterBlockId blk, const int logical_pin_index) const {
    VTR_ASSERT_SAFE(valid_block_id(blk));
    VTR_ASSERT_SAFE_MSG(logical_pin_index >= 0 && logical_pin_index < static_cast<ssize_t>(block_logical_pins_[blk].size()), "Logical pin index must be in range");

    return block_logical_pins_[blk][logical_pin_index];
}

bool ClusteredNetlist::block_contains_primary_input(const ClusterBlockId blk) const {
    const t_pb* pb = block_pb(blk);
    const t_pb* primary_input_pb = pb->find_pb_for_model(".input");
    return primary_input_pb != nullptr;
}

///@brief Returns true if the specified block contains a primary output (e.g. BLIF .output primitive)
bool ClusteredNetlist::block_contains_primary_output(const ClusterBlockId blk) const {
    const t_pb* pb = block_pb(blk);
    const t_pb* primary_output_pb = pb->find_pb_for_model(".output");
    return primary_output_pb != nullptr;
}

/*
 *
 * Pins
 *
 */
int ClusteredNetlist::pin_logical_index(const ClusterPinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_logical_index_[pin_id];
}

int ClusteredNetlist::net_pin_logical_index(const ClusterNetId net_id, int net_pin_index) const {
    auto pin_id = net_pin(net_id, net_pin_index);

    if (pin_id) {
        return pin_logical_index(pin_id);
    }

    return OPEN; //No valid pin found
}

/*
 *
 * Nets
 *
 */
bool ClusteredNetlist::net_is_ignored(const ClusterNetId id) const {
    VTR_ASSERT_SAFE(valid_net_id(id));

    return net_is_ignored_[id];
}

bool ClusteredNetlist::net_is_global(const ClusterNetId id) const {
    VTR_ASSERT_SAFE(valid_net_id(id));

    return net_is_global_[id];
}

/*
 *
 * Mutators
 *
 */
ClusterBlockId ClusteredNetlist::create_block(const char* name, t_pb* pb, t_logical_block_type_ptr type) {
    ClusterBlockId blk_id = find_block(name);
    if (blk_id == ClusterBlockId::INVALID()) {
        blk_id = Netlist::create_block(name);

        block_pbs_.insert(blk_id, pb);
        block_types_.insert(blk_id, type);

        //Allocate and initialize every potential pin of the block
        block_logical_pins_.insert(blk_id, std::vector<ClusterPinId>(get_max_num_pins(type), ClusterPinId::INVALID()));

        add_block_to_logical_type(blk_id, type);
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_block_sizes());

    //Check post-conditions: values
    VTR_ASSERT(block_pb(blk_id) == pb);
    VTR_ASSERT(block_type(blk_id) == type);

    return blk_id;
}

ClusterPortId ClusteredNetlist::create_port(const ClusterBlockId blk_id, const std::string name, BitIndex width, PortType type) {
    ClusterPortId port_id = find_port(blk_id, name);
    if (!port_id) {
        port_id = Netlist::create_port(blk_id, name, width, type);
        associate_port_with_block(port_id, type, blk_id);
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_port_sizes());

    //Check post-conditions: values
    VTR_ASSERT(port_name(port_id) == name);
    VTR_ASSERT_SAFE(find_port(blk_id, name) == port_id);

    return port_id;
}

ClusterPinId ClusteredNetlist::create_pin(const ClusterPortId port_id, BitIndex port_bit, const ClusterNetId net_id, const PinType pin_type_, int pin_index, bool is_const) {
    ClusterPinId pin_id = Netlist::create_pin(port_id, port_bit, net_id, pin_type_, is_const);

    pin_logical_index_.push_back(pin_index);

    ClusterBlockId block_id = port_block(port_id);
    block_logical_pins_[block_id][pin_index] = pin_id;

    //Check post-conditions: size
    VTR_ASSERT(validate_pin_sizes());

    return pin_id;
}

ClusterNetId ClusteredNetlist::create_net(const std::string name) {
    //Check if the net has already been created
    StringId name_id = create_string(name);
    ClusterNetId net_id = find_net(name_id);

    if (net_id == ClusterNetId::INVALID()) {
        net_id = Netlist::create_net(name);
        net_is_ignored_.push_back(false);
        net_is_global_.push_back(false);
    }

    VTR_ASSERT(validate_net_sizes());

    return net_id;
}

void ClusteredNetlist::set_net_is_ignored(ClusterNetId net_id, bool state) {
    VTR_ASSERT(valid_net_id(net_id));

    net_is_ignored_[net_id] = state;
}

void ClusteredNetlist::set_net_is_global(ClusterNetId net_id, bool state) {
    VTR_ASSERT(valid_net_id(net_id));

    net_is_global_[net_id] = state;
}

void ClusteredNetlist::remove_block_impl(const ClusterBlockId blk_id) {
    //Remove & invalidate pointers
    free_pb(block_pbs_[blk_id]);
    delete block_pbs_[blk_id];
    block_pbs_.insert(blk_id, NULL);
    block_types_.insert(blk_id, NULL);
    block_logical_pins_.insert(blk_id, std::vector<ClusterPinId>());
}

void ClusteredNetlist::remove_port_impl(const ClusterPortId port_id) {
    VTR_ASSERT(port_id);
}

void ClusteredNetlist::remove_pin_impl(const ClusterPinId pin_id) {
    VTR_ASSERT(pin_id);
}

void ClusteredNetlist::remove_net_impl(const ClusterNetId net_id) {
    VTR_ASSERT(net_id);
}

void ClusteredNetlist::clean_blocks_impl(const vtr::vector_map<ClusterBlockId, ClusterBlockId>& block_id_map) {
    //Update all the block values
    block_pbs_ = clean_and_reorder_values(block_pbs_, block_id_map);
    block_types_ = clean_and_reorder_values(block_types_, block_id_map);
    block_logical_pins_ = clean_and_reorder_values(block_logical_pins_, block_id_map);
}

void ClusteredNetlist::clean_ports_impl(const vtr::vector_map<ClusterPortId, ClusterPortId>& /*port_id_map*/) {
    //Unused
}

void ClusteredNetlist::clean_pins_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map) {
    //Update all the pin values
    pin_logical_index_ = clean_and_reorder_values(pin_logical_index_, pin_id_map);
}

void ClusteredNetlist::clean_nets_impl(const vtr::vector_map<ClusterNetId, ClusterNetId>& net_id_map) {
    //Update all the net values
    net_is_ignored_ = clean_and_reorder_values(net_is_ignored_, net_id_map);
    net_is_global_ = clean_and_reorder_values(net_is_global_, net_id_map);
}

void ClusteredNetlist::rebuild_block_refs_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& /*pin_id_map*/,
                                               const vtr::vector_map<ClusterPortId, ClusterPortId>& /*port_id_map*/) {
    for (auto blk : blocks()) {
        block_logical_pins_[blk] = std::vector<ClusterPinId>(get_max_num_pins(block_type(blk)), ClusterPinId::INVALID()); //Reset
        for (auto pin : block_pins(blk)) {
            int logical_pin_index = pin_logical_index(pin);
            block_logical_pins_[blk][logical_pin_index] = pin;
        }
    }
}

void ClusteredNetlist::rebuild_port_refs_impl(const vtr::vector_map<ClusterBlockId, ClusterBlockId>& /*block_id_map*/,
                                              const vtr::vector_map<ClusterPinId, ClusterPinId>& /*pin_id_map*/) {
    //Unused
}

void ClusteredNetlist::rebuild_pin_refs_impl(const vtr::vector_map<ClusterPortId, ClusterPortId>& /*port_id_map*/,
                                             const vtr::vector_map<ClusterNetId, ClusterNetId>& /*net_id_map*/) {
    //Unused
}

void ClusteredNetlist::rebuild_net_refs_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& /*pin_id_map*/) {
    //Unused
}

void ClusteredNetlist::shrink_to_fit_impl() {
    //Block data
    block_pbs_.shrink_to_fit();
    block_types_.shrink_to_fit();
    block_logical_pins_.shrink_to_fit();

    //Pin data
    pin_logical_index_.shrink_to_fit();

    //Net data
    net_is_ignored_.shrink_to_fit();
    net_is_global_.shrink_to_fit();
}

/**
 * @brief Given a newly created block, find its logical type and store the 
 *        block in a list where all the other blocks in the list are of the
 *        blocks logical type.
 */
void ClusteredNetlist::add_block_to_logical_type(ClusterBlockId blk_id, t_logical_block_type_ptr type) {
    std::string logical_block_type_name = type->name;

    // check if a group of blocks exist for the current logical block type
    // basically checking if this is the first time we are seeing this logical block type
    auto logical_type_blocks = block_type_to_id.find(logical_block_type_name);

    if (logical_type_blocks == block_type_to_id.end()) {
        // if the current logical block doesnt exist then create a new group of blocks for it and add it
        block_type_to_id.emplace(logical_block_type_name, std::vector<ClusterBlockId>({blk_id}));
    } else {
        // current logical block exists, so add the current block to the group other blocks of this type
        logical_type_blocks->second.push_back(blk_id);
    }
    return;
}

/*
 *
 * Sanity Checks
 *
 */
bool ClusteredNetlist::validate_block_sizes_impl(size_t num_blocks) const {
    if (block_pbs_.size() != num_blocks
        || block_types_.size() != num_blocks
        || block_logical_pins_.size() != num_blocks) {
        return false;
    }
    return true;
}

bool ClusteredNetlist::validate_port_sizes_impl(size_t /*num_ports*/) const {
    //No cluster specific port data to check
    return true;
}

bool ClusteredNetlist::validate_pin_sizes_impl(size_t num_pins) const {
    if (pin_logical_index_.size() != num_pins) {
        return false;
    }
    return true;
}

bool ClusteredNetlist::validate_net_sizes_impl(size_t num_nets) const {
    if (net_is_ignored_.size() != num_nets && net_is_global_.size() != num_nets) {
        return false;
    }
    return true;
}

/**
 * @brief Finds a block where the blocks name contains within it the
 *        provided input name. The intented use is to find the block id of a 
 *        hard block when provided with the its module name in the HDL design.
 *        
 *        For example, suppose a RAM block was instantiated in the design and it
 *        was named "test_ram". The generated netlist would not have the block
 *        named as "test_ram", instead it would be something different but the
 *        name should contain "test_ram" inside it since it represents that  
 *        block. If "test_ram" is provided to find_block() above, then an 
 *        invalid block ID would be returned. The find_block_with_matching_name
 *        () can instead be used and it should find the ram block that has 
 *        "test_ram" within its name.
 * 
 *        There is a similiar function in the Netlist Class. This function 
 *        additionally requires the logical type of the block as well. Since
 *        the inteded use is to find hard blocks, it is quite inefficient to 
 *        to go through all the blocks to find a matching one. Instead, an
 *        additional datastructure is created that groups clusters by their
 *        logical type. This function filters the clusters and only searches
 *        for the matching block within a list of blocks that are the same 
 *        logical type. The idea here is that the filtered list should be
 *        considereably smaller that a list of every block in the netlist
 *        and this should help improve run time.
 * 
 */
ClusterBlockId ClusteredNetlist::find_block_with_matching_name(const std::string& name, t_logical_block_type_ptr blk_type) const {
    ClusterBlockId blk_id = ClusterBlockId::INVALID();
    auto blks_of_logical_type = block_type_to_id.find(blk_type->name);
    std::regex name_to_match(name);

    if (blks_of_logical_type != block_type_to_id.end()) {
        // get the list of blocks that are of the specified logical type
        const std::vector<ClusterBlockId>* blk_list = &blks_of_logical_type->second;

        // go through the list of blocks to find if any block name matches the provided name (contains the input string in its name)
        for (auto blk = blk_list->begin(); blk != blk_list->end(); blk++) {
            // another thing you can do is go through blocks and instead string.find(), you can use a regular expression version (so match a regular expression)

            // check for the string match
            if (std::regex_match(Netlist::block_name(*blk), name_to_match)) {
                blk_id = *blk;
                break;
            }
        }
    }
    return blk_id;
}
