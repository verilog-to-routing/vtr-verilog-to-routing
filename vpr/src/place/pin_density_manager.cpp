
#include "pin_density_manager.h"

#include "vpr_utils.h"
#include "move_transactions.h"
#include "globals.h"

PinDensityManager::PinDensityManager(const PlacerState& placer_state)
    : placer_state_(placer_state) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    chanx_input_pin_count_.resize({grid_width, grid_height}, 0);
    chany_input_pin_count_.resize({grid_width, grid_height}, 0);
    chan_output_pin_count_.resize({grid_width, grid_height}, 0);

    const size_t num_pins = clb_nlist.pins().size();
    pin_locs_.resize(num_pins, {OPEN, OPEN, OPEN});
    pin_chan_type_.resize(num_pins, e_rr_type::NUM_RR_TYPES);
}

void PinDensityManager::find_affected_channels_and_update_costs(const t_pl_blocks_to_be_moved& blocks_affected) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();

    VTR_ASSERT(moved_pins_.empty());

    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk_id = moved_block.block_num;

        for (const ClusterPinId pin_id : clb_nlist.block_output_pins(blk_id)) {
            const t_physical_tile_loc new_pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
            const t_physical_tile_loc& old_pin_loc = pin_locs_[pin_id];

            chan_output_pin_count_[new_pin_loc.x][new_pin_loc.y]++;
            chan_output_pin_count_[old_pin_loc.x][old_pin_loc.y]--;

            moved_pins_.push_back({pin_id, new_pin_loc, e_rr_type::NUM_RR_TYPES});
        }

        for (const ClusterPinId pin_id : clb_nlist.block_input_pins(blk_id)) {
            const t_physical_tile_loc new_pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
            const e_side new_pin_side = blk_loc_registry.pin_side(pin_id);
            const auto [new_adjusted_pin_loc, new_chan_type] = input_pin_loc_chan_type_(new_pin_loc, new_pin_side);

            if (new_chan_type == e_rr_type::CHANX) {
                chanx_input_pin_count_[new_adjusted_pin_loc.x][new_adjusted_pin_loc.y]++;
            } else if (new_chan_type == e_rr_type::CHANY) {
                chany_input_pin_count_[new_adjusted_pin_loc.x][new_adjusted_pin_loc.y]++;
            } else {
                VTR_ASSERT(false);
            }

            const t_physical_tile_loc& old_pin_loc = pin_locs_[pin_id];
            const e_rr_type old_chan_type = pin_chan_type_[pin_id];
            if (old_chan_type == e_rr_type::CHANX) {
                chanx_input_pin_count_[old_pin_loc.x][old_pin_loc.y]--;
            } else if (old_chan_type == e_rr_type::CHANY) {
                chany_input_pin_count_[old_pin_loc.x][old_pin_loc.y]--;
            } else {
                VTR_ASSERT(false);
            }

            moved_pins_.push_back({pin_id, new_adjusted_pin_loc, new_chan_type});
        }
    }


}

double PinDensityManager::compute_cost() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();

    chanx_input_pin_count_.fill(0);
    chany_input_pin_count_.fill(0);
    chan_output_pin_count_.fill(0);

    for (const ClusterNetId net_id : clb_nlist.nets()) {
        // Ignored nets don't affect pin density
        if (clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        const ClusterPinId driver_pin_id = clb_nlist.net_driver(net_id);
        t_physical_tile_loc driver_pin_loc = blk_loc_registry.get_coordinate_of_pin(driver_pin_id);
        chan_output_pin_count_[driver_pin_loc.x][driver_pin_loc.y]++;
        pin_locs_[driver_pin_id] = driver_pin_loc;
        pin_chan_type_[driver_pin_id] = e_rr_type::NUM_RR_TYPES;

        for (const ClusterPinId sink_pin_id : clb_nlist.net_sinks(net_id)) {
            t_physical_tile_loc sink_pin_loc = blk_loc_registry.get_coordinate_of_pin(sink_pin_id);
            e_side sink_pin_side = blk_loc_registry.pin_side(sink_pin_id);

            t_physical_tile_loc adjusted_sink_pin_loc;
            e_rr_type sink_pin_chan_type;
            std::tie(adjusted_sink_pin_loc, sink_pin_chan_type) = input_pin_loc_chan_type_(sink_pin_loc, sink_pin_side);
            pin_locs_[sink_pin_id] = adjusted_sink_pin_loc;
            pin_chan_type_[sink_pin_id] = sink_pin_chan_type;

            if (sink_pin_chan_type == e_rr_type::CHANX) {
                chanx_input_pin_count_[adjusted_sink_pin_loc.x][adjusted_sink_pin_loc.y]++;
            } else if (sink_pin_chan_type == e_rr_type::CHANY) {
                chany_input_pin_count_[adjusted_sink_pin_loc.x][adjusted_sink_pin_loc.y]++;
            } else {
                VTR_ASSERT(false);
            }
        }
    }

    return 0;
}

void PinDensityManager::update_move_channels() {

    for (const auto [pin_id, new_loc, new_chan_type] : moved_pins_) {
        pin_locs_[pin_id] = new_loc;
        pin_chan_type_[pin_id] = new_chan_type;
    }

    moved_pins_.clear();
}

void PinDensityManager::reset_move_channels() {
}

std::pair<t_physical_tile_loc, e_rr_type> PinDensityManager::input_pin_loc_chan_type_(const t_physical_tile_loc& loc, e_side side) const {
    e_rr_type chan_type;
    t_physical_tile_loc pin_loc;

    if (side == TOP) {
        pin_loc = loc;
        chan_type = e_rr_type::CHANX;
    } else if (side == RIGHT) {
        pin_loc = loc;
        chan_type = e_rr_type::CHANY;
    } else if (side == LEFT) {
        pin_loc = {loc.x - 1, loc.y, loc.layer_num};
        chan_type = e_rr_type::CHANY;
    } else if (side == BOTTOM) {
        pin_loc = {loc.x, loc.y - 1, loc.layer_num};
        chan_type = e_rr_type::CHANX;
    } else {
        VTR_ASSERT(false);
    }

    return {pin_loc, chan_type};
}
