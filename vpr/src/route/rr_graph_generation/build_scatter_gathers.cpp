
#include "build_scatter_gathers.h"

#include <vector>

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "rr_types.h"
#include "globals.h"
#include "rr_graph_uxsdcxx_interface.h"
#include "vtr_assert.h"
#include "vtr_random.h"

//
// Static Function Declarations
//

/**
 * @brief Finds the routing channels corresponding to a given scatter or gather pattern and location.
 *
 * @param pattern Scatter/gather (SG) pattern to be processed.
 * @param loc Physical switchblock location where the SG pattern is to be applied.
 * @param chan_details_x Channel details for horizontal routing channels.
 * @param chan_details_y Channel details for vertical routing channels.
 * @param correct_channels Output: list of valid channel locations and types.
 */
static void index_to_correct_sg_channels(const t_wireconn_inf& pattern,
                                         const t_physical_tile_loc& loc,
                                         const t_chan_details& chan_details_x,
                                         const t_chan_details& chan_details_y,
                                         std::vector<t_chan_loc>& correct_channels);

/**
 * @brief Collects candidate wires from given channels that match specified switchpoints.
 *
 * @param channels List of channel locations to search for candidates.
 * @param wire_switchpoints_vec Set of wire segments and valid switchpoints for matching.
 * @param chan_details_x Channel details for horizontal routing channels.
 * @param chan_details_y Channel details for vertical routing channels.
 * @param wire_type_sizes_x Stores the number of wires of each wire segment type and their starting index for horizontal routing channels.
 * @param wire_type_sizes_y Stores the number of wires of each wire segment type and their starting index for vertical routing channels.
 * @param is_dest True if searching for destination (scatter) wires, false for source (gather).
 * @param rng Random number generator used to shuffle wire candidates.
 * @return Vector of candidate wires that satisfy the switchpoint and direction constraints.
 */
static std::vector<t_sg_candidate> find_candidate_wires(const std::vector<t_chan_loc>& channels,
                                                        const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                                        const t_chan_details& chan_details_x,
                                                        const t_chan_details& chan_details_y,
                                                        const t_wire_type_sizes& wire_type_sizes_x,
                                                        const t_wire_type_sizes& wire_type_sizes_y,
                                                        bool is_dest,
                                                        vtr::RngContainer& rng);

//
// Static Function Definitions
//

static void index_to_correct_sg_channels(const t_wireconn_inf& pattern,
                                         const t_physical_tile_loc& loc,
                                         const t_chan_details& chan_details_x,
                                         const t_chan_details& chan_details_y,
                                         std::vector<t_chan_loc>& correct_channels) {
    correct_channels.clear();

    for (e_side side : pattern.sides) {
        t_physical_tile_loc chan_loc;
        e_rr_type chan_type;

        index_into_correct_chan(loc, side, chan_details_x, chan_details_y, chan_loc, chan_type);

        if (!chan_coords_out_of_bounds(chan_loc, chan_type)) {
            correct_channels.push_back({chan_loc, chan_type, side});
        }
    }
}

static std::vector<t_sg_candidate> find_candidate_wires(const std::vector<t_chan_loc>& channels,
                                                        const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                                        const t_chan_details& chan_details_x,
                                                        const t_chan_details& chan_details_y,
                                                        const t_wire_type_sizes& wire_type_sizes_x,
                                                        const t_wire_type_sizes& wire_type_sizes_y,
                                                        bool is_dest,
                                                        vtr::RngContainer& rng) {

    // TODO: reuse
    std::vector<t_sg_candidate> candidates;

    for (const auto [chan_loc, chan_type, chan_side] : channels) {
        int seg_coord = (chan_type == e_rr_type::CHANY) ? chan_loc.y : chan_loc.x;

        const t_wire_type_sizes& wire_type_sizes = (chan_type == e_rr_type::CHANX) ? wire_type_sizes_x : wire_type_sizes_y;
        const t_chan_seg_details* chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_x[chan_loc.x][chan_loc.y].data() : chan_details_y[chan_loc.x][chan_loc.y].data();

        for (const t_wire_switchpoints& wire_switchpoints : wire_switchpoints_vec) {
            std::string_view wire_type(wire_switchpoints.segment_name);

            if (wire_type_sizes.count(wire_type) == 0) {
                // wire_type_sizes may not contain wire_type if its seg freq is 0
                continue;
            }

            // Get the number of wires of given type
            int num_type_wires = wire_type_sizes.at(wire_type).num_wires;
            // Get the last wire belonging to this type
            int first_type_wire = wire_type_sizes.at(wire_type).start;
            int last_type_wire = first_type_wire + num_type_wires - 1;

            // Walk through each wire segment of specified type and check whether it matches one
            // of the specified switchpoints.
            // Note that we walk through the points in order, this ensures that returned switchpoints
            // match the order specified in the architecture, which we assume is a priority order specified
            // by the architect.
            for (int valid_switchpoint : wire_switchpoints.switchpoints) {
                for (int iwire = first_type_wire; iwire <= last_type_wire; iwire++) {
                    Direction seg_direction = chan_details[iwire].direction();

                    // unidirectional wires going in the decreasing direction can have an outgoing edge
                    // only from the top or right switch block sides, and an incoming edge only if they are
                    // at the left or bottom sides (analogous for wires going in INC direction) */
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
                    if (wire_switchpoint != valid_switchpoint) {
                        continue;
                    }

                    candidates.push_back({chan_loc, chan_type, chan_side, {iwire, wire_switchpoint}});
                }
            }
        }
    }

    // TODO: Whether we shuffle candidates or not should be determined by switch point order type.
    vtr::shuffle(candidates.begin(), candidates.end(), rng);

    return candidates;
}

//
// Non-static Function Definitions
//

std::vector<t_bottleneck_link> alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                                                         const std::vector<bool>& inter_cluster_rr,
                                                                         const std::vector<t_segment_inf>& segment_inf_x,
                                                                         const std::vector<t_segment_inf>& segment_inf_y,
                                                                         const std::vector<t_segment_inf>& segment_inf_z,
                                                                         const t_chan_details& chan_details_x,
                                                                         const t_chan_details& chan_details_y,
                                                                         const t_chan_width& nodes_per_chan,
                                                                         vtr::RngContainer& rng,
                                                                         vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    std::vector<t_chan_loc> gather_channels;
    std::vector<t_chan_loc> scatter_channels;

    vtr::FormulaParser formula_parser;
    vtr::t_formula_data formula_data;

    std::vector<t_bottleneck_link> bottleneck_links;

    interdie_3d_links.clear();
    if (grid.get_num_layers() > 1) {
        interdie_3d_links.resize({grid.width(), grid.height()});
    }

    const auto [wire_type_sizes_x, wire_type_sizes_y] = count_wire_type_sizes(chan_details_x, chan_details_y, nodes_per_chan);

    for (const t_scatter_gather_pattern& sg_pattern : scatter_gather_patterns) {
        VTR_ASSERT(sg_pattern.type == e_scatter_gather_type::UNIDIR);

        for (const t_sg_location& sg_loc_info : sg_pattern.sg_locations) {

            for (const t_physical_tile_loc gather_loc : grid.all_locations()) {
                if (sb_not_here(grid, inter_cluster_rr, gather_loc, sg_loc_info.type)) {
                    continue;
                }

                auto sg_link_it = std::ranges::find_if(sg_pattern.sg_links,
                                                       [&](const t_sg_link& link) noexcept {
                                                           return link.name == sg_loc_info.sg_link_name;
                                                       });

                VTR_ASSERT(sg_link_it != sg_pattern.sg_links.end());
                const t_sg_link& sg_link = *sg_link_it;

                VTR_ASSERT(vtr::exactly_k_conditions(1, sg_link.x_offset != 0, sg_link.y_offset != 0, sg_link.z_offset != 0));
                t_physical_tile_loc scatter_loc;
                scatter_loc.x = gather_loc.x + sg_link.x_offset;
                scatter_loc.y = gather_loc.y + sg_link.y_offset;
                scatter_loc.layer_num = gather_loc.layer_num + sg_link.z_offset;

                const std::vector<t_segment_inf>& segment_inf = (sg_link.x_offset != 0) ? segment_inf_x : (sg_link.y_offset != 0) ? segment_inf_y
                                                                                                                                  : segment_inf_z;

                const e_rr_type chan_type = (sg_link.x_offset != 0) ? e_rr_type::CHANX : (sg_link.y_offset != 0) ? e_rr_type::CHANY
                                                                                                                 : e_rr_type::CHANZ;

                auto seg_it = std::ranges::find_if(segment_inf,
                                                   [&](const t_segment_inf& seg) noexcept {
                                                       return seg.name == sg_link.seg_type;
                                                   });

                VTR_ASSERT(seg_it != segment_inf.end());
                const t_segment_inf& wire_segment = *seg_it;

                index_to_correct_sg_channels(sg_pattern.gather_pattern, gather_loc, chan_details_x, chan_details_y, gather_channels);
                index_to_correct_sg_channels(sg_pattern.scatter_pattern, scatter_loc, chan_details_x, chan_details_y, scatter_channels);

                if (gather_channels.empty() || scatter_channels.empty()) {
                    continue;
                }

                std::vector<t_sg_candidate> gather_wire_candidates;
                gather_wire_candidates = find_candidate_wires(gather_channels,
                                                              sg_pattern.gather_pattern.from_switchpoint_set,
                                                              chan_details_x, chan_details_y,
                                                              wire_type_sizes_x, wire_type_sizes_y,
                                                              /*is_dest=*/false, rng);

                std::vector<t_sg_candidate> scatter_wire_candidates;
                scatter_wire_candidates = find_candidate_wires(scatter_channels,
                                                               sg_pattern.scatter_pattern.to_switchpoint_set,
                                                               chan_details_x, chan_details_y,
                                                               wire_type_sizes_x, wire_type_sizes_y,
                                                               /*is_dest=*/true, rng);

                int bottleneck_fanin = evaluate_num_conns_formula(formula_parser,
                                                                  formula_data,
                                                                  sg_pattern.gather_pattern.num_conns_formula,
                                                                  gather_wire_candidates.size(),
                                                                  scatter_wire_candidates.size());

                int bottleneck_fanout = evaluate_num_conns_formula(formula_parser,
                                                                   formula_data,
                                                                   sg_pattern.scatter_pattern.num_conns_formula,
                                                                   gather_wire_candidates.size(),
                                                                   scatter_wire_candidates.size());

                bottleneck_fanin = std::min<int>(bottleneck_fanin, gather_wire_candidates.size());
                bottleneck_fanout = std::min<int>(bottleneck_fanout, scatter_wire_candidates.size());

                if (bottleneck_fanin == 0 || bottleneck_fanout == 0) {

                    VTR_LOGV_WARN(bottleneck_fanin == 0,
                                  "Scatter-gather pattern '%s' with SG link '%s' at location (layer=%i, x=%i, y=%i) "
                                  "has zero gather fanin connections (candidates=%zu)\n",
                                  sg_pattern.name.c_str(), sg_link.name.c_str(),
                                  gather_loc.layer_num, gather_loc.x, gather_loc.y,
                                  gather_wire_candidates.size());

                    VTR_LOGV_WARN(bottleneck_fanout == 0,
                                  "Scatter-gather pattern '%s' with SG link '%s' at location (layer=%i, x=%i, y=%i) "
                                  "has zero scatter fanout connections (candidates=%zu)\n",
                                  sg_pattern.name.c_str(), sg_link.name.c_str(),
                                  scatter_loc.layer_num, scatter_loc.x, scatter_loc.y,
                                  scatter_wire_candidates.size());

                    continue;
                }

                const bool is_3d_link = (sg_link.z_offset != 0);
                if (is_3d_link) {
                    interdie_3d_links[gather_loc.x][gather_loc.y].reserve(interdie_3d_links[gather_loc.x][gather_loc.y].size() + sg_loc_info.num);
                } else {
                    bottleneck_links.reserve(bottleneck_links.size() + sg_loc_info.num);
                }

                for (int i_bottleneck = 0, i_s = 0, i_g = 0; i_bottleneck < sg_loc_info.num; i_bottleneck++) {

                    t_bottleneck_link bottleneck_link;
                    bottleneck_link.gather_loc = gather_loc;
                    bottleneck_link.scatter_loc = scatter_loc;
                    bottleneck_link.gather_fanin_connections.reserve(bottleneck_fanin);
                    bottleneck_link.scatter_fanout_connections.reserve(bottleneck_fanout);

                    for (int i = 0; i < bottleneck_fanin; i++) {
                        bottleneck_link.gather_fanin_connections.push_back(gather_wire_candidates[i_g]);
                        i_g = (i_g + 1) % bottleneck_fanin;
                    }

                    for (int i = 0; i < bottleneck_fanout; i++) {
                        bottleneck_link.scatter_fanout_connections.push_back(scatter_wire_candidates[i_s]);
                        i_s = (i_s + 1) % bottleneck_fanout;
                    }

                    bottleneck_link.chan_type = chan_type;
                    bottleneck_link.parallel_segment_index = std::distance(segment_inf.begin(), seg_it);

                    if (is_3d_link) {
                        if (sg_link.z_offset < 0 && wire_segment.arch_wire_switch_dec != ARCH_FPGA_UNDEFINED_VAL) {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch_dec;
                        } else {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch;
                        }
                        interdie_3d_links[gather_loc.x][gather_loc.y].push_back(std::move(bottleneck_link));
                    } else {
                        if ((sg_link.x_offset < 0 || sg_link.y_offset < 0) && wire_segment.arch_wire_switch_dec != ARCH_FPGA_UNDEFINED_VAL) {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch_dec;
                        } else {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch;
                        }

                        bottleneck_links.push_back(std::move(bottleneck_link));
                    }
                }
            }
        }
    }

    return bottleneck_links;
}
