
#include "blk_loc_registry.h"
#include "globals.h"

const vtr::vector_map<ClusterBlockId, t_block_loc>& BlkLocRegistry::block_locs() const {
    return  block_locs_;
}

vtr::vector_map<ClusterBlockId, t_block_loc>& BlkLocRegistry::mutable_block_locs() {
    return  block_locs_;
}

const GridBlock& BlkLocRegistry::grid_blocks() const {
    return grid_blocks_;
}

GridBlock& BlkLocRegistry::mutable_grid_blocks() {
    return grid_blocks_;
}

const vtr::vector_map<ClusterPinId, int>& BlkLocRegistry::physical_pins() const {
    return physical_pins_;
}

vtr::vector_map<ClusterPinId, int>& BlkLocRegistry::mutable_physical_pins() {
    return physical_pins_;
}

int BlkLocRegistry::tile_pin_index(const ClusterPinId pin) const {
    return physical_pins_[pin];
}

int BlkLocRegistry::net_pin_to_tile_pin_index(const ClusterNetId net_id, int net_pin_index) const {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    // Get the logical pin index of pin within its logical block type
    ClusterPinId pin_id = cluster_ctx.clb_nlist.net_pin(net_id, net_pin_index);

    return this->tile_pin_index(pin_id);
}
