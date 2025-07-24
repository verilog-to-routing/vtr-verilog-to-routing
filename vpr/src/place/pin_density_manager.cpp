
#include "pin_density_manager.h"

#include "vpr_utils.h"
#include "move_transactions.h"
#include "globals.h"
#include "read_xml_arch_file_vib.h"

#include <array>

std::pair<float, float> maximum_pin_connections_per_channel() {
    const RRGraphView& rr_graph = g_vpr_ctx.device().rr_graph;
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    int mid_x = grid_width / 2;
    int mid_y = grid_height / 2;

    std::vector<RRNodeId> nodes = rr_graph.node_lookup().find_channel_nodes(0, mid_x, mid_y, e_rr_type::CHANX);

    double estimated_chanx_num_pins = 0.;
    for (const RRNodeId node : nodes) {
        int node_length = rr_graph.node_length(node);

        estimated_chanx_num_pins += (1. / node_length);
    }


    nodes = rr_graph.node_lookup().find_channel_nodes(0, mid_x, mid_y, e_rr_type::CHANY);

    double estimated_chany_num_pins = 0.;
    for (const RRNodeId node : nodes) {
        int node_length = rr_graph.node_length(node);

        estimated_chany_num_pins += (1. / node_length);
    }

    return {estimated_chanx_num_pins, estimated_chany_num_pins};
}

PinDensityManager::PinDensityManager(const PlacerState& placer_state,
                                     const t_placer_opts& placer_opts)
    : placer_state_(placer_state)
    , placer_opts_(placer_opts) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    chanx_input_pin_count_.resize({grid_width, grid_height}, 0);
    chany_input_pin_count_.resize({grid_width, grid_height}, 0);
    chan_output_pin_count_.resize({grid_width, grid_height}, 0);

    ts_chanx_input_pin_count_.resize({grid_width, grid_height}, 0);
    ts_chany_input_pin_count_.resize({grid_width, grid_height}, 0);
    ts_chan_output_pin_count_.resize({grid_width, grid_height}, 0);

    const size_t num_pins = clb_nlist.pins().size();
    pin_info_.resize(num_pins);
    ts_pin_info_.resize(num_pins);

    std::tie (chanx_unique_signals_per_chan_, chany_unique_signals_per_chan_) = maximum_pin_connections_per_channel();

    std::cout << "Unique signals: " << chanx_unique_signals_per_chan_ << " " << chany_unique_signals_per_chan_ << std::endl;
}

double PinDensityManager::compute_cost() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    chanx_input_pin_count_.fill(0);
    chany_input_pin_count_.fill(0);
    chan_output_pin_count_.fill(0);

    for (const ClusterBlockId blk_id : clb_nlist.blocks()) {

        for (const ClusterPinId driver_pin_id : clb_nlist.block_output_pins(blk_id)) {
            t_physical_tile_loc driver_pin_loc = blk_loc_registry.get_coordinate_of_pin(driver_pin_id);
            e_side driver_pin_side = blk_loc_registry.pin_side(driver_pin_id);
            auto [sb_loc0, sb_loc1] = output_pin_sb_locs_(driver_pin_loc, driver_pin_side);
            chan_output_pin_count_[sb_loc0.x][sb_loc0.y]++;
            chan_output_pin_count_[sb_loc1.x][sb_loc1.y]++;

            pin_info_[driver_pin_id] = t_output_pin_sb_locs{sb_loc0, sb_loc1};
        }

        for (const ClusterPinId sink_pin_id : clb_nlist.block_input_pins(blk_id)) {
            t_physical_tile_loc sink_pin_loc = blk_loc_registry.get_coordinate_of_pin(sink_pin_id);
            e_side sink_pin_side = blk_loc_registry.pin_side(sink_pin_id);
            const t_input_pin_adjacent_chan sink_pin_loc_chan = input_pin_loc_chan_type_(sink_pin_loc, sink_pin_side);
            pin_info_[sink_pin_id] = sink_pin_loc_chan;

            if (sink_pin_loc_chan.chan_type == e_rr_type::CHANX) {
                chanx_input_pin_count_[sink_pin_loc_chan.loc.x][sink_pin_loc_chan.loc.y]++;
            } else if (sink_pin_loc_chan.chan_type == e_rr_type::CHANY) {
                chany_input_pin_count_[sink_pin_loc_chan.loc.x][sink_pin_loc_chan.loc.y]++;
            } else {
                VTR_ASSERT(false);
            }
        }
    }

    ts_chanx_input_pin_count_ = chanx_input_pin_count_;
    ts_chany_input_pin_count_ = chany_input_pin_count_;
    ts_chan_output_pin_count_ = chan_output_pin_count_;

    double total_cost = 0.;
    for (e_rr_type chan_type : {e_rr_type::CHANX, e_rr_type::CHANY}) {
        for (int x = 0; x < grid_width; x++) {
            for (int y = 0; y < grid_height; y++) {


                t_physical_tile_loc loc{x, y, 0};

                t_physical_tile_loc sb0_loc = loc;
                t_physical_tile_loc sb1_loc = loc;

                float pin_count = 0.f;
                if (chan_type == e_rr_type::CHANX) {
                    pin_count = chanx_input_pin_count_[loc.x][loc.y];
                    sb1_loc.x = std::max(0, sb1_loc.x - 1);
                } else if (chan_type == e_rr_type::CHANY) {
                    pin_count = chany_input_pin_count_[loc.x][loc.y];
                    sb1_loc.y = std::max(0, sb1_loc.y - 1);
                } else {
                    VTR_ASSERT(false);
                }

                pin_count += static_cast<float>(chan_output_pin_count_[sb0_loc.x][sb0_loc.y] + chan_output_pin_count_[sb1_loc.x][sb1_loc.y])  / 8.0f;

                float cost;
                if (chan_type == e_rr_type::CHANX) {
                    cost = std::max(0.f, (pin_count / chanx_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
                } else if (chan_type == e_rr_type::CHANY) {
                    cost = std::max(0.f, (pin_count / chany_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
                } else {
                    VTR_ASSERT(false);
                }


                // std::cout << rr_node_typename[chan_type] << " " <<  x << ", " << y << ", " << pin_count << " "
                //           << ((chan_type == e_rr_type::CHANX) ? chanx_input_pin_count_[loc.x][loc.y] : chany_input_pin_count_[loc.x][loc.y]) << " "
                //           << ts_chan_output_pin_count_[sb0_loc.x][sb0_loc.y] << " " <<  ts_chan_output_pin_count_[sb1_loc.x][sb1_loc.y] << " "
                //           << cost << std::endl;

                total_cost += cost;
            }
        }
    }

    return total_cost;
}

void PinDensityManager::print() const {
    const size_t grid_width = chanx_input_pin_count_.dim_size(0);
    const size_t grid_height = chanx_input_pin_count_.dim_size(1);

    // Print channel pin counts together
    VTR_LOG("Pin counts at each grid location (x, y): [chanx_input, chany_input, chan_output]\n");

    for (size_t x = 0; x < grid_width; ++x) {
        for (size_t y = 0; y < grid_height; ++y) {
            int chanx = chanx_input_pin_count_[x][y];
            int chany = chany_input_pin_count_[x][y];
            int out   = chan_output_pin_count_[x][y];

            if (chanx > 0 || chany > 0 || out > 0) {
                VTR_LOG("  [%zu][%zu] = [%d, %d, %d]\n", x, y, chanx, chany, out);
            }
        }
    }
}

double PinDensityManager::find_affected_channels_and_update_costs(const t_pl_blocks_to_be_moved& blocks_affected) {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();

    VTR_ASSERT(moved_pins_.empty());

    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk_id = moved_block.block_num;

        for (const ClusterPinId pin_id : clb_nlist.block_output_pins(blk_id)) {

            const t_output_pin_sb_locs old_sb_locs = std::get<t_output_pin_sb_locs>(pin_info_[pin_id]);
            ts_chan_output_pin_count_[old_sb_locs.sb0.x][old_sb_locs.sb0.y]--;
            ts_chan_output_pin_count_[old_sb_locs.sb1.x][old_sb_locs.sb1.y]--;

            const t_physical_tile_loc new_pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
            const e_side new_pin_side = blk_loc_registry.pin_side(pin_id);
            const t_output_pin_sb_locs new_sb_locs = output_pin_sb_locs_(new_pin_loc, new_pin_side);
            ts_chan_output_pin_count_[new_sb_locs.sb0.x][new_sb_locs.sb0.y]++;
            ts_chan_output_pin_count_[new_sb_locs.sb1.x][new_sb_locs.sb1.y]++;

            moved_pins_.push_back({pin_id, new_sb_locs});
            affected_sb_locs_.insert(new_sb_locs.sb0);
            affected_sb_locs_.insert(new_sb_locs.sb1);
            affected_sb_locs_.insert(old_sb_locs.sb0);
            affected_sb_locs_.insert(old_sb_locs.sb1);
        }

        for (const ClusterPinId pin_id : clb_nlist.block_input_pins(blk_id)) {
            const t_physical_tile_loc new_pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);
            const e_side new_pin_side = blk_loc_registry.pin_side(pin_id);
            const t_input_pin_adjacent_chan new_pin_loc_chan = input_pin_loc_chan_type_(new_pin_loc, new_pin_side);

            if (new_pin_loc_chan.chan_type == e_rr_type::CHANX) {
                ts_chanx_input_pin_count_[new_pin_loc_chan.loc.x][new_pin_loc_chan.loc.y]++;
            } else if (new_pin_loc_chan.chan_type == e_rr_type::CHANY) {
                ts_chany_input_pin_count_[new_pin_loc_chan.loc.x][new_pin_loc_chan.loc.y]++;
            } else {
                VTR_ASSERT(false);
            }

            const t_input_pin_adjacent_chan old_pin_loc_chan = std::get<t_input_pin_adjacent_chan>(pin_info_[pin_id]);
            if (old_pin_loc_chan.chan_type == e_rr_type::CHANX) {
                ts_chanx_input_pin_count_[old_pin_loc_chan.loc.x][old_pin_loc_chan.loc.y]--;
            } else if (old_pin_loc_chan.chan_type == e_rr_type::CHANY) {
                ts_chany_input_pin_count_[old_pin_loc_chan.loc.x][old_pin_loc_chan.loc.y]--;
            } else {
                VTR_ASSERT(false);
            }

            moved_pins_.push_back({pin_id, new_pin_loc_chan});
            affected_sb_locs_.insert(new_pin_loc_chan.loc);
            affected_sb_locs_.insert(old_pin_loc_chan.loc);
        }
    }


    affected_chans_.clear();
    translate_affected_sb_locs_to_chans_();

    double cost_diff = 0.;
    for (auto [loc, chan_type] : affected_chans_) {
        float prev_pin_count, curr_pin_count;

        t_physical_tile_loc sb0_loc = loc;
        t_physical_tile_loc sb1_loc = loc;

        if (chan_type == e_rr_type::CHANX) {
            curr_pin_count = ts_chanx_input_pin_count_[loc.x][loc.y];
            prev_pin_count = chanx_input_pin_count_[loc.x][loc.y];
            sb1_loc.x = std::max(0, sb1_loc.x - 1);
        } else if (chan_type == e_rr_type::CHANY) {
            curr_pin_count = ts_chany_input_pin_count_[loc.x][loc.y];
            prev_pin_count = chany_input_pin_count_[loc.x][loc.y];
            sb1_loc.y = std::max(0, sb1_loc.y - 1);
        } else {
            VTR_ASSERT(false);
        }


        curr_pin_count += static_cast<float>(ts_chan_output_pin_count_[sb0_loc.x][sb0_loc.y] + ts_chan_output_pin_count_[sb1_loc.x][sb1_loc.y])  / 8.0f;
        prev_pin_count += static_cast<float>(chan_output_pin_count_[sb0_loc.x][sb0_loc.y] + chan_output_pin_count_[sb1_loc.x][sb1_loc.y])  / 8.0f;

        float curr_cost, prev_cost;

        if (chan_type == e_rr_type::CHANX) {
            curr_cost = std::max(0.f, (curr_pin_count / chanx_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
            prev_cost = std::max(0.f, (prev_pin_count / chanx_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
        } else if (chan_type == e_rr_type::CHANY) {
            curr_cost = std::max(0.f, (curr_pin_count / chany_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
            prev_cost = std::max(0.f, (prev_pin_count / chany_unique_signals_per_chan_) - placer_opts_.pin_util_threshold);
        } else {
            VTR_ASSERT(false);
        }

        cost_diff += curr_cost - prev_cost;
    }

    return cost_diff;
}

void PinDensityManager::update_move_channels() {

    for (const auto& [pin_id, new_pin_info] : moved_pins_) {
        pin_info_[pin_id] = new_pin_info;
    }

    for (const t_physical_tile_loc& affected_chan_loc : affected_sb_locs_) {
        chanx_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = ts_chanx_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
        chany_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = ts_chany_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
        chan_output_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = ts_chan_output_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
    }

    affected_sb_locs_.clear();
    moved_pins_.clear();
}

void PinDensityManager::reset_move_channels() {

    for (const t_physical_tile_loc& affected_chan_loc : affected_sb_locs_) {
        ts_chanx_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = chanx_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
        ts_chany_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = chany_input_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
        ts_chan_output_pin_count_[affected_chan_loc.x][affected_chan_loc.y] = chan_output_pin_count_[affected_chan_loc.x][affected_chan_loc.y];
    }

    affected_sb_locs_.clear();
    moved_pins_.clear();
}

PinDensityManager::t_input_pin_adjacent_chan PinDensityManager::input_pin_loc_chan_type_(const t_physical_tile_loc& loc, e_side side) const {
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

    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    pin_loc.x = std::clamp<int>(pin_loc.x, 0, grid_width - 1);
    pin_loc.y = std::clamp<int>(pin_loc.y, 0, grid_height - 1);

    return {pin_loc, chan_type};
}

PinDensityManager::t_output_pin_sb_locs PinDensityManager::output_pin_sb_locs_(const t_physical_tile_loc& loc, e_side side) const {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    t_physical_tile_loc sb_loc0;
    t_physical_tile_loc sb_loc1;

    switch (side) {
        case TOP:
            sb_loc0 = loc;
            sb_loc1 = {loc.x - 1, loc.y, loc.layer_num};
            break;
        case RIGHT:
            sb_loc0 = loc;
            sb_loc1 = {loc.x, loc.y - 1, loc.layer_num};
            break;
        case BOTTOM:
            sb_loc0 = {loc.x, loc.y - 1, loc.layer_num};;
            sb_loc1 = {loc.x - 1, loc.y - 1, loc.layer_num};
            break;
        case LEFT:
            sb_loc0 = {loc.x - 1, loc.y, loc.layer_num};;
            sb_loc1 = {loc.x - 1, loc.y - 1, loc.layer_num};
            break;
        default:
            VTR_ASSERT(false);
            break;

    }

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();
    const size_t grid_layers = grid.get_num_layers();

    sb_loc0.x = std::clamp<int>(sb_loc0.x, 0, grid_width - 1);
    sb_loc0.y = std::clamp<int>(sb_loc0.y, 0, grid_height - 1);

    sb_loc1.x = std::clamp<int>(sb_loc1.x, 0, grid_width - 1);
    sb_loc1.y = std::clamp<int>(sb_loc1.y, 0, grid_height - 1);


    return {sb_loc0, sb_loc1};
}

void PinDensityManager::translate_affected_sb_locs_to_chans_() {
    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;

    const size_t grid_width = device_grid.width();
    const size_t grid_height = device_grid.height();

    for (const t_physical_tile_loc& affected_sb_loc : affected_sb_locs_) {
        std::array<std::pair<t_physical_tile_loc, e_rr_type>, 4> adjacent_chans;

        adjacent_chans[0] = {affected_sb_loc, e_rr_type::CHANX};
        adjacent_chans[1] = {affected_sb_loc, e_rr_type::CHANY};
        adjacent_chans[2] = {{affected_sb_loc.x, affected_sb_loc.y + 1, affected_sb_loc.layer_num}, e_rr_type::CHANY};
        adjacent_chans[3] = {{affected_sb_loc.x + 1, affected_sb_loc.y, affected_sb_loc.layer_num}, e_rr_type::CHANX};

        for (const auto& [adj_chan_loc, adj_chan_type] : adjacent_chans) {
            if (adj_chan_loc.x >= 0 && adj_chan_loc.x < grid_width && adj_chan_loc.y < grid_height && adj_chan_loc.y >= 0) {
                affected_chans_.insert({adj_chan_loc, adj_chan_type});
            }
        }
    }

}