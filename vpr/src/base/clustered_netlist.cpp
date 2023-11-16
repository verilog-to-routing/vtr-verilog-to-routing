#include "clustered_netlist.h"

#include "vtr_assert.h"
#include "vpr_error.h"

#include <utility>

/**
 * @file
 * @brief ClusteredNetlist Class Implementation
 */

ClusteredNetlist::ClusteredNetlist(std::string name, std::string id)
    : Netlist<ClusterBlockId, ClusterPortId, ClusterPinId, ClusterNetId>(std::move(name), std::move(id)) {}

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

const std::vector<ClusterBlockId>& ClusteredNetlist::blocks_per_type(const t_logical_block_type& blk_type) const {
    // empty vector is declared static to avoid re-allocation every time the function is called
    static std::vector<ClusterBlockId> empty_vector;
    if (blocks_per_type_.count(blk_type.index) == 0) {
        return empty_vector;
    }

    // the vector is returned as const reference to avoid unnecessary copies,
    // especially that returned vectors may be very large as they contain
    // all clustered blocks with a specific block type
    return blocks_per_type_.at(blk_type.index);
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

        blocks_per_type_[type->index].push_back(blk_id);

        //Allocate and initialize every potential pin of the block
        block_logical_pins_.insert(blk_id, std::vector<ClusterPinId>(get_max_num_pins(type), ClusterPinId::INVALID()));
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_block_sizes());

    //Check post-conditions: values
    VTR_ASSERT(block_pb(blk_id) == pb);
    VTR_ASSERT(block_type(blk_id) == type);

    return blk_id;
}

ClusterPortId ClusteredNetlist::create_port(const ClusterBlockId blk_id, const std::string& name, BitIndex width, PortType type) {
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

ClusterNetId ClusteredNetlist::create_net(const std::string& name) {
    //Check if the net has already been created
    StringId name_id = create_string(name);
    ClusterNetId net_id = find_net(name_id);

    if (net_id == ClusterNetId::INVALID()) {
        net_id = Netlist::create_net(name);
    }

    VTR_ASSERT(validate_net_sizes());

    return net_id;
}

void ClusteredNetlist::remove_block_impl(const ClusterBlockId blk_id) {
    //find the block type, so we can remove it from blocks_per_type_ data structure
    auto blk_type = block_type(blk_id);
    //Remove & invalidate pointers
    free_pb(block_pbs_[blk_id]);
    delete block_pbs_[blk_id];
    block_pbs_.insert(blk_id, NULL);
    block_types_.insert(blk_id, NULL);
    std::remove(blocks_per_type_[blk_type->index].begin(), blocks_per_type_[blk_type->index].end(), blk_id);
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

void ClusteredNetlist::clean_nets_impl(const vtr::vector_map<ClusterNetId, ClusterNetId>& /* net_id_map */) {
    //Update all the net values
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

bool ClusteredNetlist::validate_net_sizes_impl(size_t /* num_nets */) const {
    return true;
}

ClusterBlockId ClusteredNetlist::find_block_by_name_fragment(const std::string& name_pattern, const std::vector<ClusterBlockId>& cluster_block_candidates) const {
    ClusterBlockId blk_id = ClusterBlockId::INVALID();
    std::regex name_to_match(name_pattern);

    for (auto cluster_block_candidate : cluster_block_candidates) {
        if (std::regex_match(Netlist::block_name(cluster_block_candidate), name_to_match)) {
            blk_id = cluster_block_candidate;
            break;
        }
    }

    return blk_id;
}
