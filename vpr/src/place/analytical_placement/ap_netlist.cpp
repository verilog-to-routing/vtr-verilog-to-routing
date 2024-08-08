
#include "ap_netlist.h"
#include <string>
#include "netlist_fwd.h"
#include "netlist_utils.h"
#include "vpr_types.h"
#include "vtr_assert.h"

/*
 * Blocks
 */
const t_pack_molecule* APNetlist::block_molecule(const APBlockId id) const {
    VTR_ASSERT_SAFE(valid_block_id(id));

    return block_molecules_[id];
}

APBlockType APNetlist::block_type(const APBlockId id) const {
    VTR_ASSERT_SAFE(valid_block_id(id));

    return block_types_[id];
}

const APFixedBlockLoc& APNetlist::block_loc(const APBlockId id) const {
    VTR_ASSERT_SAFE(valid_block_id(id));
    VTR_ASSERT(block_type(id) == APBlockType::FIXED);

    return block_locs_[id];
}

/*
 * Mutators
 */
APBlockId APNetlist::create_block(const std::string& name, const t_pack_molecule* mol) {
    APBlockId blk_id = Netlist::create_block(name);

    // Initialize the data
    block_molecules_.insert(blk_id, mol);
    block_types_.insert(blk_id, APBlockType::MOVEABLE);
    block_locs_.insert(blk_id, APFixedBlockLoc());

    // Check post-conditions: size
    VTR_ASSERT(validate_block_sizes());

    // Check post-conditions: values
    VTR_ASSERT(block_molecule(blk_id) == mol);
    VTR_ASSERT(block_type(blk_id) == APBlockType::MOVEABLE);

    return blk_id;
}

void APNetlist::set_block_loc(const APBlockId id, const APFixedBlockLoc& loc) {
    VTR_ASSERT_SAFE(valid_block_id(id));

    // Check that the location is fixed, if all values are -1 then it is not fixed.
    if (loc.x == -1 && loc.y == -1 && loc.sub_tile == -1 && loc.layer_num == -1)
        return;

    block_locs_[id] = loc;
    block_types_[id] = APBlockType::FIXED;
}

APPortId APNetlist::create_port(const APBlockId blk_id, const std::string& name, BitIndex width, PortType type) {
    APPortId port_id = find_port(blk_id, name);
    if (!port_id) {
        port_id = Netlist::create_port(blk_id, name, width, type);
        associate_port_with_block(port_id, type, blk_id);
    }

    // Check post-conditions: size
    VTR_ASSERT(validate_port_sizes());

    // Check post-conditions: values
    VTR_ASSERT(port_name(port_id) == name);
    VTR_ASSERT(find_port(blk_id, name) == port_id);

    return port_id;
}

APPinId APNetlist::create_pin(const APPortId port_id, BitIndex port_bit, const APNetId net_id, const PinType pin_type_, bool is_const) {
    APPinId pin_id = Netlist::create_pin(port_id, port_bit, net_id, pin_type_, is_const);

    // Check post-conditions: size
    VTR_ASSERT(validate_pin_sizes());

    // Check post-conditions: values
    VTR_ASSERT(pin_type(pin_id) == pin_type_);
    VTR_ASSERT(pin_port(pin_id) == port_id);
    VTR_ASSERT(pin_port_type(pin_id) == port_type(port_id));

    return pin_id;
}

APNetId APNetlist::create_net(const std::string& name) {
    APNetId net_id = Netlist::create_net(name);

    // Check post-conditions: size
    VTR_ASSERT(validate_net_sizes());

    return net_id;
}

/*
 * Internal utilities
 */
void APNetlist::clean_blocks_impl(const vtr::vector_map<APBlockId, APBlockId>& block_id_map) {
    // Update all the block molecules
    block_molecules_ = clean_and_reorder_values(block_molecules_, block_id_map);
    // Update all the block types
    block_types_ = clean_and_reorder_values(block_types_, block_id_map);
    // Update the fixed block locations
    block_locs_ = clean_and_reorder_values(block_locs_, block_id_map);
}

void APNetlist::clean_ports_impl(const vtr::vector_map<APPortId, APPortId>& /*port_id_map*/) {
    // Unused
}

void APNetlist::clean_pins_impl(const vtr::vector_map<APPinId, APPinId>& /*pin_id_map*/) {
    // Unused
}

void APNetlist::clean_nets_impl(const vtr::vector_map<APNetId, APNetId>& /*net_id_map*/) {
    // Unused
}

void APNetlist::rebuild_block_refs_impl(const vtr::vector_map<APPinId, APPinId>& /*pin_id_map*/,
                                        const vtr::vector_map<APPortId, APPortId>& /*port_id_map*/) {
    // Unused
}

void APNetlist::rebuild_port_refs_impl(const vtr::vector_map<APBlockId, APBlockId>& /*block_id_map*/, const vtr::vector_map<APPinId, APPinId>& /*pin_id_map*/) {
    // Unused
}

void APNetlist::rebuild_pin_refs_impl(const vtr::vector_map<APPortId, APPortId>& /*port_id_map*/, const vtr::vector_map<APNetId, APNetId>& /*net_id_map*/) {
    // Unused
}

void APNetlist::rebuild_net_refs_impl(const vtr::vector_map<APPinId, APPinId>& /*pin_id_map*/) {
    // Unused
}

void APNetlist::shrink_to_fit_impl() {
    // Block data
    block_molecules_.shrink_to_fit();
    block_types_.shrink_to_fit();
    block_locs_.shrink_to_fit();
}

void APNetlist::remove_block_impl(const APBlockId /*blk_id*/) {
    // Unused
}

void APNetlist::remove_port_impl(const APPortId /*port_id*/) {
    // Unused
}

void APNetlist::remove_pin_impl(const APPinId /*pin_id*/) {
    // Unused
}

void APNetlist::remove_net_impl(const APNetId /*net_id*/) {
    // Unused
}

/*
 * Sanity Checks
 */
bool APNetlist::validate_block_sizes_impl(size_t num_blocks) const {
    if (block_molecules_.size() != num_blocks)
        return false;
    if (block_types_.size() != num_blocks)
        return false;
    if (block_locs_.size() != num_blocks)
        return false;
    return true;
}

bool APNetlist::validate_port_sizes_impl(size_t /*num_ports*/) const {
    // No AP-specific port data to check
    return true;
}

bool APNetlist::validate_pin_sizes_impl(size_t /*num_pins*/) const {
    // No AP-specific pin data to check
    return true;
}

bool APNetlist::validate_net_sizes_impl(size_t /*num_nets*/) const {
    // No AP-specific net data to checl
    return true;
}

