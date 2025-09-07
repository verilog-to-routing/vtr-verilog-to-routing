
#include "build_scatter_gathers.h"

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "rr_types.h"
#include "globals.h"
#include "vtr_assert.h"

#include <vector>

static void index_to_correct_channels(const t_wireconn_inf& pattern,
                                      const t_physical_tile_loc& loc,
                                      const t_chan_details& chan_details_x,
                                      const t_chan_details& chan_details_y,
                                      std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side>>& correct_channels) {
    correct_channels.clear();

    for (e_side side : pattern.sides) {
        t_physical_tile_loc chan_loc;
        e_rr_type chan_type;

        index_into_correct_chan(loc, side, chan_details_x, chan_details_y,chan_loc, chan_type);

        if (!chan_coords_out_of_bounds(chan_loc, chan_type)) {
            correct_channels.push_back({chan_loc, chan_type, side});
        }
    }
}

static
std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side, t_wire_switchpoint>>
            find_candidate_wires(const std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side>>& channels,
                                 const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const t_wire_type_sizes& wire_type_sizes_x,
                                 const t_wire_type_sizes& wire_type_sizes_y,
                                 bool is_dest) {

    // TODO: reuse
    std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side, t_wire_switchpoint>> collected_wire_switchpoints;

    for (const auto [chan_loc, chan_type, chan_side] : channels) {
        int seg_coord = (chan_type == e_rr_type::CHANY) ? chan_loc.y : chan_loc.x;

        const t_wire_type_sizes& wire_type_sizes = (chan_type == e_rr_type::CHANX) ? wire_type_sizes_x : wire_type_sizes_y;
        const t_chan_seg_details* chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_x[chan_loc.x][chan_loc.y].data() : chan_details_y[chan_loc.x][chan_loc.y].data();

        for (const t_wire_switchpoints& wire_switchpoints : wire_switchpoints_vec) {
            collected_wire_switchpoints.clear();
            auto wire_type = vtr::string_view(wire_switchpoints.segment_name);

            if (wire_type_sizes.find(wire_type) == wire_type_sizes.end()) {
                // wire_type_sizes may not contain wire_type if its seg freq is 0
                continue;
            }

            // Get the number of wires of given type
            int num_type_wires = wire_type_sizes.at(wire_type).num_wires;
            // Get the last wire belonging to this type
            int first_type_wire = wire_type_sizes.at(wire_type).start;
            int last_type_wire = first_type_wire + num_type_wires - 1;

            for (int valid_switchpoint : wire_switchpoints.switchpoints) {
                for (int iwire = first_type_wire; iwire <= last_type_wire; iwire++) {
                    Direction seg_direction = chan_details[iwire].direction();

                    /* unidirectional wires going in the decreasing direction can have an outgoing edge
                     * only from the top or right switch block sides, and an incoming edge only if they are
                     * at the left or bottom sides (analogous for wires going in INC direction) */
                    if (chan_side == TOP || chan_side == RIGHT) {
                        if (seg_direction == Direction::DEC && is_dest) {
                            continue;
                        }
                        if (seg_direction == Direction::INC && !is_dest) {
                            continue;
                        }
                    } else {
                        VTR_ASSERT(chan_side == LEFT || chan_side == BOTTOM);
                        if (seg_direction == Direction::DEC && !is_dest) {
                            continue;
                        }
                        if (seg_direction == Direction::INC && is_dest) {
                            continue;
                        }
                    }

                    int wire_switchpoint = get_switchpoint_of_wire(chan_type, chan_details[iwire], seg_coord, chan_side);

                    // Check if this wire belongs to one of the specified switchpoints; add it to our 'wires' vector if so
                    if (wire_switchpoint != valid_switchpoint) continue;

                    collected_wire_switchpoints.push_back({chan_loc, chan_type, chan_side, {iwire, wire_switchpoint}});
                }
            }

        }
    }

    return collected_wire_switchpoints;
}

void alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                               const std::vector<bool>& inter_cluster_rr,
                                               const t_chan_details& chan_details_x,
                                               const t_chan_details& chan_details_y,
                                               const t_chan_width& nodes_per_chan) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side>> gather_channels;
    std::vector<std::tuple<t_physical_tile_loc, e_rr_type, e_side>> scatter_channels;

    const auto [wire_type_sizes_x, wire_type_sizes_y] = count_wire_type_sizes(chan_details_x, chan_details_y, nodes_per_chan);

    for (const t_scatter_gather_pattern& sg_pattern : scatter_gather_patterns) {
        VTR_ASSERT(sg_pattern.type == e_scatter_gather_type::UNIDIR);

        for (const t_sg_location& sg_loc_info : sg_pattern.sg_locations) {

            for (const t_physical_tile_loc gather_loc : grid.all_locations()) {
                if (sb_not_here(grid, inter_cluster_rr, gather_loc, sg_loc_info.type)) {
                    continue;
                }

                auto it = std::find_if(sg_pattern.sg_links.begin(), sg_pattern.sg_links.end(),
                       [&](const t_sg_link& link) {
                           return link.name == sg_loc_info.sg_link_name;
                       });

                VTR_ASSERT_SAFE(it != sg_pattern.sg_links.end());
                const t_sg_link& sg_link = *it;

                t_physical_tile_loc scatter_loc;
                scatter_loc.x = gather_loc.x + sg_link.x_offset;
                scatter_loc.y = gather_loc.y + sg_link.y_offset;
                scatter_loc.layer_num = gather_loc.layer_num + sg_link.z_offset;


                index_to_correct_channels(sg_pattern.gather_pattern, gather_loc, chan_details_x, chan_details_y, gather_channels);
                index_to_correct_channels(sg_pattern.scatter_pattern, scatter_loc, chan_details_x, chan_details_y, scatter_channels);

                if (gather_channels.empty() || scatter_channels.empty()) {
                    continue;
                }

                auto gather_wire_candidates = find_candidate_wires(gather_channels,
                                                                            sg_pattern.gather_pattern.from_switchpoint_set,
                                                                            chan_details_x, chan_details_y,
                                                                            wire_type_sizes_x, wire_type_sizes_y,
                                                                            /*is_dest=*/false);

                auto scatter_wire_candidates = find_candidate_wires(scatter_channels,
                                                                             sg_pattern.scatter_pattern.to_switchpoint_set,
                                                                             chan_details_x, chan_details_y,
                                                                             wire_type_sizes_x, wire_type_sizes_y,
                                                                             /*is_dest=*/false);
            }
        }


    }
}
