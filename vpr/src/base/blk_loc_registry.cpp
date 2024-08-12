
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

void BlkLocRegistry::set_block_location(ClusterBlockId blk_id, const t_pl_loc& location) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const std::string& block_name = cluster_ctx.clb_nlist.block_name(blk_id);

    //Check if block location is out of range of grid dimensions
    if (location.x < 0 || location.x > int(device_ctx.grid.width() - 1)
        || location.y < 0 || location.y > int(device_ctx.grid.height() - 1)) {
        VPR_THROW(VPR_ERROR_PLACE, "Block %s with ID %d is out of range at location (%d, %d). \n",
                  block_name.c_str(), blk_id, location.x, location.y);
    }

    //Set the location of the block
    block_locs_[blk_id].loc = location;

    //Check if block is at an illegal location
    auto physical_tile = device_ctx.grid.get_physical_type({location.x, location.y, location.layer});
    auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

    if (location.sub_tile >= physical_tile->capacity || location.sub_tile < 0) {
        VPR_THROW(VPR_ERROR_PLACE, "Block %s subtile number (%d) is out of range. \n", block_name.c_str(), location.sub_tile);
    }

    if (!is_sub_tile_compatible(physical_tile, logical_block, block_locs_[blk_id].loc.sub_tile)) {
        VPR_THROW(VPR_ERROR_PLACE, "Attempt to place block %s with ID %d at illegal location (%d,%d,%d). \n",
                  block_name.c_str(),
                  blk_id,
                  location.x,
                  location.y,
                  location.layer);
    }

    //Mark the grid location and usage of the block
    grid_blocks_.set_block_at_location(location, blk_id);
    grid_blocks_.increment_usage({location.x, location.y, location.layer});

    place_sync_external_block_connections(blk_id);
}

void BlkLocRegistry::place_sync_external_block_connections(ClusterBlockId iblk) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    t_pl_loc block_loc = block_locs_[iblk].loc;

    auto physical_tile = physical_tile_type(block_loc);
    auto logical_block = clb_nlist.block_type(iblk);

    int sub_tile_index = get_sub_tile_index(iblk, block_locs_);
    auto sub_tile = physical_tile->sub_tiles[sub_tile_index];

    VTR_ASSERT(sub_tile.num_phy_pins % sub_tile.capacity.total() == 0);

    int max_num_block_pins = sub_tile.num_phy_pins / sub_tile.capacity.total();
    /* Logical location and physical location is offset by z * max_num_block_pins */

    int rel_capacity = block_loc.sub_tile - sub_tile.capacity.low;

    for (ClusterPinId pin : clb_nlist.block_pins(iblk)) {
        int logical_pin_index = clb_nlist.pin_logical_index(pin);
        int sub_tile_pin_index = get_sub_tile_physical_pin(sub_tile_index, physical_tile, logical_block, logical_pin_index);

        int new_physical_pin_index = sub_tile.sub_tile_to_tile_pin_indices[sub_tile_pin_index + rel_capacity * max_num_block_pins];

        auto result = physical_pins_.find(pin);
        if (result != physical_pins_.end()) {
            physical_pins_[pin] = new_physical_pin_index;
        } else {
            physical_pins_.insert(pin, new_physical_pin_index);
        }
    }
}
