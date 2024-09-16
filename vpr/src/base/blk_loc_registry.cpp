
#include "blk_loc_registry.h"

#include "move_transactions.h"
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

void BlkLocRegistry::apply_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& device_ctx = g_vpr_ctx.device();

    // Swap the blocks, but don't swap the nets or update place_ctx.grid_blocks
    // yet since we don't know whether the swap will be accepted
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& old_loc = moved_block.old_loc;
        const t_pl_loc& new_loc = moved_block.new_loc;

        // move the block to its new location
        block_locs_[blk].loc = new_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x,old_loc.y,old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x,new_loc.y, new_loc.layer});

        // if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk);
        }
    }
}

void BlkLocRegistry::commit_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    // Swap physical location
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& to = moved_block.new_loc;
        const t_pl_loc& from = moved_block.old_loc;

        // Remove from old location only if it hasn't already been updated by a previous block update
        if (grid_blocks_.block_at_location(from) == blk) {
            grid_blocks_.set_block_at_location(from, ClusterBlockId::INVALID());
            grid_blocks_.decrement_usage({from.x, from.y, from.layer});
        }

        // Add to new location
        if (grid_blocks_.block_at_location(to) == ClusterBlockId::INVALID()) {
            //Only need to increase usage if previously unused
            grid_blocks_.increment_usage({to.x, to.y, to.layer});
        }
        grid_blocks_.set_block_at_location(to, blk);

    } // Finish updating clb for all blocks
}

void BlkLocRegistry::revert_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& device_ctx = g_vpr_ctx.device();

    // Swap the blocks back, nets not yet swapped they don't need to be changed
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& old_loc = moved_block.old_loc;
        const t_pl_loc& new_loc = moved_block.new_loc;

        // return the block to where it was before the swap
        block_locs_[blk].loc = old_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x, old_loc.y, old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x, new_loc.y, new_loc.layer});

        // if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk);
        }

        VTR_ASSERT_SAFE_MSG(grid_blocks_.block_at_location(old_loc) == blk,
                            "Grid blocks should only have been updated if swap committed (not reverted)");
    }
}
