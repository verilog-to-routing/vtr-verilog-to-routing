
#include "build_scatter_gathers.h"

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "rr_types.h"
#include "globals.h"
#include "vtr_assert.h"

#include <vector>

//
// Static Function Declrations
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
static void index_to_correct_channels(const t_wireconn_inf& pattern,
                                      const t_physical_tile_loc& loc,
                                      const t_chan_details& chan_details_x,
                                      const t_chan_details& chan_details_y,
                                      std::vector<t_chan_loc>& correct_channels);

/**
 * @brief Collects candidate wires from given channels that match specified switchpoints.
 *
 * @param channels List of channel locations to search for candiates.
 * @param wire_switchpoints_vec Set of wire segments and valid switchpoints for matching.
 * @param chan_details_x Channel details for horizontal routing channels.
 * @param chan_details_y Channel details for vertical routing channels.
 * @param is_dest True if searching for destination (scatter) wires, false for source (gather).
 * @param wire_type_sizes_x Stores the number of wires of each wire segment type and their starting index for horizontal routing channels.
 * @param wire_type_sizes_y Stores the number of wires of each wire segment type and their starting index for vertical routing channels.
 * @return Vector of candidate wires that satisfy the switchpoint and direction constraints.
 */
static std::vector<t_sg_candidate> find_candidate_wires(const std::vector<t_chan_loc>& channels,
                                                        const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                                        const t_chan_details& chan_details_x,
                                                        const t_chan_details& chan_details_y,
                                                        const t_wire_type_sizes& wire_type_sizes_x,
                                                        const t_wire_type_sizes& wire_type_sizes_y,
                                                        bool is_dest);

//
// Static Function Definitions
//

static void index_to_correct_channels(const t_wireconn_inf& pattern,
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
                                                        bool is_dest) {

    // TODO: reuse
    std::vector<t_sg_candidate> candidates;

    for (const auto [chan_loc, chan_type, chan_side] : channels) {
        int seg_coord = (chan_type == e_rr_type::CHANY) ? chan_loc.y : chan_loc.x;

        const t_wire_type_sizes& wire_type_sizes = (chan_type == e_rr_type::CHANX) ? wire_type_sizes_x : wire_type_sizes_y;
        const t_chan_seg_details* chan_details = (chan_type == e_rr_type::CHANX) ? chan_details_x[chan_loc.x][chan_loc.y].data() : chan_details_y[chan_loc.x][chan_loc.y].data();

        for (const t_wire_switchpoints& wire_switchpoints : wire_switchpoints_vec) {
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

                    candidates.push_back({chan_loc, chan_type, chan_side, {iwire, wire_switchpoint}});
                }
            }
        }
    }

    return candidates;
}

//
// Non-satatic Function Definitions
//

std::vector<t_bottleneck_link> alloc_and_load_scatter_gather_connections(const std::vector<t_scatter_gather_pattern>& scatter_gather_patterns,
                                                                         const std::vector<bool>& inter_cluster_rr,
                                                                         const std::vector<t_segment_inf>& segment_inf,
                                                                         const t_chan_details& chan_details_x,
                                                                         const t_chan_details& chan_details_y,
                                                                         const t_chan_width& nodes_per_chan,
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

                auto seg_it = std::ranges::find_if(segment_inf,
                                                   [&](const t_segment_inf& seg) noexcept {
                                                       return seg.name == sg_link.seg_type;
                                                   });

                VTR_ASSERT(seg_it != segment_inf.end());
                const t_segment_inf& wire_segment = *seg_it;

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
                                                                    /*is_dest=*/true);

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

void convert_interposer_cuts_to_sg_patterns(const std::vector<t_layer_def>& interposer_inf,
                                            std::vector<t_scatter_gather_pattern>& sg_patterns) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    const size_t num_layers = grid.get_num_layers();
    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    VTR_ASSERT(interposer_inf.size() == num_layers);

    for (size_t layer = 0; layer < num_layers; layer++) {

        for (const t_interposer_cut_inf& cut_inf : interposer_inf[layer].interposer_cuts) {
            const int cut_loc = cut_inf.loc;
            e_interposer_cut_dim cut_dim = cut_inf.dim;

            for (const t_interdie_wire_inf& wire_inf : cut_inf) {
                VTR_ASSERT(wire_inf.offset_definition.repeat_expr.empty());

                const int start = std::stoi(wire_inf.offset_definition.start_expr) + cut_loc;
                const int end = std::stoi(wire_inf.offset_definition.end_expr) + cut_loc;
                const int incr = std::stoi(wire_inf.offset_definition.incr_expr);

                auto sg_it = std::ranges::find_if(sg_patterns, [&wire_inf](const t_scatter_gather_pattern& sg) {
                    return wire_inf.sg_name == sg.name;
                });

                VTR_ASSERT(sg_it != sg_patterns.end());

                t_specified_loc region;

                region.reg_x.start = (cut_dim == e_interposer_cut_dim::X) ? cut_loc + start : 0;
                region.reg_x.end = (cut_dim == e_interposer_cut_dim::X) ? cut_loc + end : grid_width - 1;
                region.reg_x.incr = (cut_dim == e_interposer_cut_dim::X) ? incr : 1;
                region.reg_y.start = (cut_dim == e_interposer_cut_dim::Y) ? cut_loc + start : 0;
                region.reg_y.end = (cut_dim == e_interposer_cut_dim::Y) ? cut_loc + end : grid_height - 1;
                region.reg_y.incr = (cut_dim == e_interposer_cut_dim::Y) ? incr : 1;

                t_sg_location sg_location{.type = e_sb_location::E_XY_SPECIFIED,
                                          .region = region,
                                          .num = wire_inf.num,
                                          .sg_link_name = wire_inf.sg_link};

                sg_it->sg_locations.push_back(std::move(sg_location));

            }
        }
    }
}