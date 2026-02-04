
#include "build_scatter_gathers.h"

#include <vector>

#include "switchblock_scatter_gather_common_utils.h"
#include "scatter_gather_types.h"
#include "rr_types.h"
#include "globals.h"
#include "place_util.h"
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
 * @param candidates Candidate wires that satisfy the switchpoint and direction constraints. To be populated by this function.
 */
static void find_candidate_wires(const std::vector<t_chan_loc>& channels,
                                 const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const t_wire_type_sizes& wire_type_sizes_x,
                                 const t_wire_type_sizes& wire_type_sizes_y,
                                 bool is_dest,
                                 vtr::RngContainer& rng,
                                 std::vector<t_sg_candidate>& candidates);

/**
 * @brief Identifies wire candidates for a scatter-gather (SG) connection and determines the bottleneck fan-in/fan-out.
 *
 * This functions performs three main steps:
 * 1. Channel Mapping: Translates the sides (TOP, BOTTOM, etc.) of the source and destination
 * switch blocks into physical routing channel locations (CHANX/CHANY + xyz coordinates).
 * 2. Wire Filtering: Searches the routing channels for wire segments that match the
 * required segment type (e.g., L4, L16) and switchpoints specified in the architecture file.
 * 3. Formula Evaluation: Applies the user-defined connectivity formulas (e.g., based on
 * channel width or literal numbers) to determine the fanin and fanout of each SG connection.
 *
 * @param gather_pattern Architectural definition for the source (gather) side of the link.
 * @param scatter_pattern Architectural definition for the destination (scatter) side of the link.
 * @param gather_loc Physical grid location (x, y, layer) of the source switch block.
 * @param scatter_loc Physical grid location (x, y, layer) of the destination switch block.
 * @param chan_details_x Details of wire segment types in horizontal channels.
 * @param chan_details_y Details of wire segment types in vertical channels.
 * @param wire_type_sizes_x Indexing information for wire types in CHANX.
 * @param wire_type_sizes_y Indexing information for wire types in CHANY.
 * @param formula_parser Parser used to evaluate the `num_conns` math expressions.
 * @param formula_data Context data (e.g. W) for the formula evaluation.
 * @param gather_channels [out] Populated with the physical channel locations used by the source.
 * @param scatter_channels [out] Populated with the physical channel locations used by the destination.
 * @param rng Random number generator used to shuffle candidates for uniform distribution.
 * @param gather_wire_candidates [out] The set of physical wire segments matching the gather pattern at the source.
 * @param scatter_wire_candidates [out] The set of physical wire segments matching the scatter pattern at the destination.
 * @param bottleneck_fanin [out] The fanin of each SG link capped by available candidates.
 * @param bottleneck_fanout [out] The fanout of each SG link capped by available candidates.
 */
static void collect_sg_wire_candidates(const t_wireconn_inf& gather_pattern,
                                       const t_wireconn_inf& scatter_pattern,
                                       const t_physical_tile_loc& gather_loc,
                                       const t_physical_tile_loc& scatter_loc,
                                       const t_chan_details& chan_details_x,
                                       const t_chan_details& chan_details_y,
                                       const t_wire_type_sizes& wire_type_sizes_x,
                                       const t_wire_type_sizes& wire_type_sizes_y,
                                       vtr::FormulaParser& formula_parser,
                                       vtr::t_formula_data& formula_data,
                                       std::vector<t_chan_loc>& gather_channels,
                                       std::vector<t_chan_loc>& scatter_channels,
                                       vtr::RngContainer& rng,
                                       std::vector<t_sg_candidate>& gather_wire_candidates,
                                       std::vector<t_sg_candidate>& scatter_wire_candidates,
                                       int& bottleneck_fanin,
                                       int& bottleneck_fanout);

/**
 * @brief Mirrors a scatter-gather (SG) pattern's switch block sides based on the link's displacement.
 *
 * This function is used primarily for bidirectional links to ensure that the "reverse" path
 * looks into the correct routing channels. It performs a spatial flip of the source sides:
 * - If the link moves vertically (y_offset != 0), TOP and BOTTOM sides are swapped.
 * - If the link moves horizontally (x_offset != 0), LEFT and RIGHT sides are swapped.
 * - For 3D links (z-offset), the relative planar sides (LEFT/RIGHT and BOTTOM/TOP) remain unchanged.
 *
 * @param sg_pattern The original pattern (gather or scatter) containing
 * the set of sides to be mirrored.
 * @param sg_link The link definition providing the displacement offsets (x, y, z)
 * used to determine the axis of mirroring.
 * @return t_wireconn_inf A copy of the input pattern with the `sides` set updated to
 * reflect the mirrored spatial orientation.
 */
static t_wireconn_inf mirror_sg_pattern(const t_wireconn_inf& sg_pattern, const t_sg_link& sg_link);

//                             //
// Static Function Definitions //
//                             //

static t_wireconn_inf mirror_sg_pattern(const t_wireconn_inf& sg_pattern, const t_sg_link& sg_link) {
    t_wireconn_inf mirrored_pattern = sg_pattern;

    // Clear the sides to rebuild them mirrored
    mirrored_pattern.sides.clear();

    for (e_side side : sg_pattern.sides) {
        e_side new_side = side;

        // Mirror along the Y-axis (Vertical flip)
        if (sg_link.y_offset != 0) {
            if (side == e_side::TOP) {
                new_side = e_side::BOTTOM;
            } else if (side == e_side::BOTTOM) {
                new_side = e_side::TOP;
            }
        }
        // Mirror along the X-axis (Horizontal flip)
        else if (sg_link.x_offset != 0) {
            if (side == e_side::LEFT) {
                new_side = e_side::RIGHT;
            } else if (side == e_side::RIGHT) {
                new_side = e_side::LEFT;
            }
        }
        // z_offset != 0: do nothing (new_side remains side)

        mirrored_pattern.sides.insert(new_side);
    }

    return mirrored_pattern;
}

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

static void find_candidate_wires(const std::vector<t_chan_loc>& channels,
                                 const std::vector<t_wire_switchpoints>& wire_switchpoints_vec,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const t_wire_type_sizes& wire_type_sizes_x,
                                 const t_wire_type_sizes& wire_type_sizes_y,
                                 bool is_dest,
                                 vtr::RngContainer& rng,
                                 std::vector<t_sg_candidate>& candidates) {
    // `candidates` is passed by reference to avoid memory re-allocation
    // when this function is called many times. We clear the vector's content
    // that was written to it by the previous call.
    candidates.clear();

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
}

static void collect_sg_wire_candidates(const t_wireconn_inf& gather_pattern,
                                       const t_wireconn_inf& scatter_pattern,
                                       const t_physical_tile_loc& gather_loc,
                                       const t_physical_tile_loc& scatter_loc,
                                       const t_chan_details& chan_details_x,
                                       const t_chan_details& chan_details_y,
                                       const t_wire_type_sizes& wire_type_sizes_x,
                                       const t_wire_type_sizes& wire_type_sizes_y,
                                       vtr::FormulaParser& formula_parser,
                                       vtr::t_formula_data& formula_data,
                                       std::vector<t_chan_loc>& gather_channels,
                                       std::vector<t_chan_loc>& scatter_channels,
                                       vtr::RngContainer& rng,
                                       std::vector<t_sg_candidate>& gather_wire_candidates,
                                       std::vector<t_sg_candidate>& scatter_wire_candidates,
                                       int& bottleneck_fanin,
                                       int& bottleneck_fanout) {
    // Determine which routing channels are adjacent to the specified sides of the source and destination switch blocks
    index_to_correct_sg_channels(gather_pattern, gather_loc, chan_details_x, chan_details_y, gather_channels);
    index_to_correct_sg_channels(scatter_pattern, scatter_loc, chan_details_x, chan_details_y, scatter_channels);

    // If either side is missing valid channels (e.g., at the edge of the FPGA grid),
    // no connections can be formed. Early exit to avoid redundant candidate searches.
    if (gather_channels.empty() || scatter_channels.empty()) {
        bottleneck_fanin = 0;
        bottleneck_fanout = 0;
        gather_wire_candidates.clear();
        scatter_wire_candidates.clear();
        return;
    }

    // Filter all wires in the identified channels to find segments that match the given switchpoints
    find_candidate_wires(gather_channels, gather_pattern.from_switchpoint_set,
                         chan_details_x, chan_details_y,
                         wire_type_sizes_x, wire_type_sizes_y,
                         /*is_dest=*/false, rng, gather_wire_candidates);

    find_candidate_wires(scatter_channels, scatter_pattern.to_switchpoint_set,
                         chan_details_x, chan_details_y,
                         wire_type_sizes_x, wire_type_sizes_y,
                         /*is_dest=*/true, rng, scatter_wire_candidates);

    // Evaluate the fanin/fanout of each SG link
    // These formulas determine how many wires drive each SG link,
    // and how many wires are driven by each SG link.
    bottleneck_fanin = evaluate_num_conns_formula(formula_parser, formula_data, gather_pattern.num_conns_formula,
                                                  gather_wire_candidates.size(), scatter_wire_candidates.size());

    bottleneck_fanout = evaluate_num_conns_formula(formula_parser, formula_data, scatter_pattern.num_conns_formula,
                                                   gather_wire_candidates.size(), scatter_wire_candidates.size());

    // Ensure the requested connection count does not exceed the physical number
    // of available wires to prevent out-of-bounds indexing.
    bottleneck_fanin = std::min<int>(bottleneck_fanin, gather_wire_candidates.size());
    bottleneck_fanout = std::min<int>(bottleneck_fanout, scatter_wire_candidates.size());
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

    // Storage for identifying source/destination channels of an SG link.
    // These vectors are re-used for SG links at all locations to avoid memory re-allocation.
    std::vector<t_chan_loc> gather_channels, scatter_channels;

    // Persistent storage for source (gather) and destination (scatter) wire candidates.
    // Re-used across iterations to minimize memory re-allocations.
    // In a "Forward" connection, signal flows from the gather location to the scatter location.
    // For bidirectional (BIDIR) SG links, signal can also flow in the reverse direction:
    // being driven by wires at the scatter location to drive wires at the gather location.
    // These vectors store the physical wire segments identified for each endpoint and direction.
    std::vector<t_sg_candidate> fwd_gather_wire_candidates, fwd_scatter_wire_candidates;
    std::vector<t_sg_candidate> rev_gather_wire_candidates, rev_scatter_wire_candidates;

    vtr::FormulaParser formula_parser;
    vtr::t_formula_data formula_data;

    std::vector<t_bottleneck_link> bottleneck_links;

    // Handle 3D storage: if we have a multi-layer device, we organize inter-layer links
    // spatially (by X,Y) for easier lookup during RR graph construction.
    interdie_3d_links.clear();
    if (grid.get_num_layers() > 1) {
        interdie_3d_links.resize({grid.width(), grid.height()});
    }

    // Pre-calculate the mapping of wire type names to their indices in the channel
    // to avoid re-scanning the entire channel width.
    const auto [wire_type_sizes_x, wire_type_sizes_y] = count_wire_type_sizes(chan_details_x, chan_details_y, nodes_per_chan);

    for (const t_scatter_gather_pattern& sg_pattern : scatter_gather_patterns) {
        for (const t_sg_location& sg_loc_info : sg_pattern.sg_locations) {

            // Sweep every location in the grid to find valid gather (source) points.
            for (const t_physical_tile_loc gather_loc : grid.all_locations()) {
                // Skip locations that don't match the pattern's region.
                if (sb_not_here(grid, inter_cluster_rr, gather_loc, sg_loc_info.type, sg_loc_info.region)) {
                    continue;
                }

                // Find the specific SG link definition to get X/Y/Z offsets and wire segment type.
                auto sg_link_it = std::ranges::find_if(sg_pattern.sg_links,
                                                       [&](const t_sg_link& link) noexcept {
                                                           return link.name == sg_loc_info.sg_link_name;
                                                       });

                VTR_ASSERT(sg_link_it != sg_pattern.sg_links.end());
                const t_sg_link& sg_link = *sg_link_it;

                VTR_ASSERT(vtr::exactly_k_conditions(1, sg_link.x_offset != 0, sg_link.y_offset != 0, sg_link.z_offset != 0));
                // Calculate where the scatter (destination) switch block is physically located.
                t_physical_tile_loc scatter_loc;
                scatter_loc.x = gather_loc.x + sg_link.x_offset;
                scatter_loc.y = gather_loc.y + sg_link.y_offset;
                scatter_loc.layer_num = gather_loc.layer_num + sg_link.z_offset;

                if (!is_loc_on_chip(scatter_loc)) {
                    continue;
                }

                // Determine the routing resources (CHANX/CHANY/CHANZ) involved based on the offset axis.
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

                // Collect wires for the forward path (gather -> scatter).
                int fwd_bottleneck_fanin = 0;
                int fwd_bottleneck_fanout = 0;
                collect_sg_wire_candidates(sg_pattern.gather_pattern, sg_pattern.scatter_pattern,
                                           gather_loc, scatter_loc, chan_details_x, chan_details_y,
                                           wire_type_sizes_x, wire_type_sizes_y, formula_parser, formula_data,
                                           gather_channels, scatter_channels, rng,
                                           fwd_gather_wire_candidates, fwd_scatter_wire_candidates,
                                           fwd_bottleneck_fanin, fwd_bottleneck_fanout);

                // Collect wires for the reverse path if the link is bidirectional.
                int rev_bottleneck_fanin = 0;
                int rev_bottleneck_fanout = 0;
                if (sg_pattern.type == e_scatter_gather_type::BIDIR) {
                    t_wireconn_inf rev_gather_pattern = mirror_sg_pattern(sg_pattern.gather_pattern, sg_link);
                    t_wireconn_inf rev_scatter_pattern = mirror_sg_pattern(sg_pattern.scatter_pattern, sg_link);

                    collect_sg_wire_candidates(rev_gather_pattern, rev_scatter_pattern,
                                               scatter_loc, gather_loc, chan_details_x, chan_details_y,
                                               wire_type_sizes_x, wire_type_sizes_y, formula_parser, formula_data,
                                               gather_channels, scatter_channels, rng,
                                               rev_gather_wire_candidates, rev_scatter_wire_candidates,
                                               rev_bottleneck_fanin, rev_bottleneck_fanout);
                }

                if (fwd_bottleneck_fanin == 0 || fwd_bottleneck_fanout == 0) {

                    VTR_LOGV_WARN(fwd_bottleneck_fanin == 0,
                                  "Scatter-gather pattern '%s' with SG link '%s' at location (layer=%i, x=%i, y=%i) "
                                  "has zero gather fanin connections (candidates=%zu)\n",
                                  sg_pattern.name.c_str(), sg_link.name.c_str(),
                                  gather_loc.layer_num, gather_loc.x, gather_loc.y,
                                  fwd_gather_wire_candidates.size());

                    VTR_LOGV_WARN(fwd_bottleneck_fanout == 0,
                                  "Scatter-gather pattern '%s' with SG link '%s' at location (layer=%i, x=%i, y=%i) "
                                  "has zero scatter fanout connections (candidates=%zu)\n",
                                  sg_pattern.name.c_str(), sg_link.name.c_str(),
                                  scatter_loc.layer_num, scatter_loc.x, scatter_loc.y,
                                  fwd_scatter_wire_candidates.size());

                    // continue;
                }

                const bool is_3d_link = (sg_link.z_offset != 0);
                if (is_3d_link) {
                    interdie_3d_links[gather_loc.x][gather_loc.y].reserve(interdie_3d_links[gather_loc.x][gather_loc.y].size() + sg_loc_info.num);
                } else {
                    bottleneck_links.reserve(bottleneck_links.size() + sg_loc_info.num);
                }

                // Populate the actual bottleneck links. 'sg_loc_info.num' allows the architect
                // to specify multiple parallel links between the same two switch blocks.
                for (int i_bottleneck = 0; i_bottleneck < sg_loc_info.num; i_bottleneck++) {

                    t_bottleneck_link bottleneck_link;
                    bottleneck_link.gather_loc = gather_loc;
                    bottleneck_link.scatter_loc = scatter_loc;
                    bottleneck_link.gather_fanin_connections.reserve(fwd_bottleneck_fanin);
                    bottleneck_link.scatter_fanout_connections.reserve(fwd_bottleneck_fanout);
                    bottleneck_link.bidir = (sg_pattern.type == e_scatter_gather_type::BIDIR);

                    // Combine forward and reverse candidates into the final link object.
                    // This allows the RR builder to know which physical wires can drive this link.
                    for (int i = 0; i < fwd_bottleneck_fanin; i++) {
                        int i_g = i % fwd_bottleneck_fanin;
                        bottleneck_link.gather_fanin_connections.push_back(fwd_gather_wire_candidates[i_g]);
                    }

                    for (int i = 0; i < rev_bottleneck_fanin; i++) {
                        int i_g = i % rev_bottleneck_fanin;
                        bottleneck_link.gather_fanin_connections.push_back(rev_gather_wire_candidates[i_g]);
                    }

                    for (int i = 0; i < fwd_bottleneck_fanout; i++) {
                        int i_s = i % fwd_bottleneck_fanout;
                        bottleneck_link.scatter_fanout_connections.push_back(fwd_scatter_wire_candidates[i_s]);
                    }

                    for (int i = 0; i < rev_bottleneck_fanout; i++) {
                        int i_s = i % rev_bottleneck_fanout;
                        bottleneck_link.scatter_fanout_connections.push_back(rev_scatter_wire_candidates[i_s]);
                    }

                    // Assign electrical and architectural properties from the segment information.
                    bottleneck_link.chan_type = chan_type;
                    bottleneck_link.parallel_segment_index = std::distance(segment_inf.begin(), seg_it);
                    bottleneck_link.R_metal = seg_it->Rmetal;
                    bottleneck_link.C_metal = seg_it->Cmetal;

                    // Choose the correct switch (INC vs DEC) based on the link's physical direction.
                    if (is_3d_link) {
                        if (sg_link.z_offset < 0
                            && wire_segment.arch_wire_switch_dec != ARCH_FPGA_UNDEFINED_VAL
                            && sg_pattern.type != e_scatter_gather_type::BIDIR) {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch_dec;
                        } else {
                            bottleneck_link.arch_wire_switch = wire_segment.arch_wire_switch;
                        }
                        interdie_3d_links[gather_loc.x][gather_loc.y].push_back(std::move(bottleneck_link));
                    } else {
                        if ((sg_link.x_offset < 0 || sg_link.y_offset < 0)
                            && wire_segment.arch_wire_switch_dec != ARCH_FPGA_UNDEFINED_VAL
                            && sg_pattern.type != e_scatter_gather_type::BIDIR) {
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

    // Step 1: Iterate over all layers in the device grid
    for (size_t layer = 0; layer < num_layers; layer++) {

        // Step 2: Process each interposer cut (vertical or horizontal) on this layer
        for (const t_interposer_cut_inf& cut_inf : interposer_inf[layer].interposer_cuts) {
            const int cut_loc = cut_inf.loc;
            e_interposer_cut_type cut_type = cut_inf.dim;

            // Step 3: For each inter-die wire defined at this cut, compute its SG region
            for (const t_interdie_wire_inf& wire_inf : cut_inf.interdie_wires) {
                VTR_ASSERT(wire_inf.offset_definition.repeat_expr.empty());

                // Parse offset expressions and compute absolute start/end positions of the region
                const int start = std::stoi(wire_inf.offset_definition.start_expr) + cut_loc;
                const int end = std::stoi(wire_inf.offset_definition.end_expr) + cut_loc;
                const int incr = std::stoi(wire_inf.offset_definition.incr_expr);

                // Step 4: Find the corresponding SG pattern by name
                auto sg_it = std::ranges::find_if(sg_patterns, [&wire_inf](const t_scatter_gather_pattern& sg) noexcept {
                    return wire_inf.sg_name == sg.name;
                });
                // Must exist; SG pattern names must match between interposer XML and SG patterns
                VTR_ASSERT(sg_it != sg_patterns.end());

                // Step 5: Define the spatial region (X/Y span) for this SG link
                t_specified_loc region;

                // For vertical cuts, X varies along the cut; Y covers full height
                region.reg_x.start = (cut_type == e_interposer_cut_type::VERT) ? start : 0;
                region.reg_x.end = (cut_type == e_interposer_cut_type::VERT) ? end : grid_width - 1;
                region.reg_x.incr = (cut_type == e_interposer_cut_type::VERT) ? incr : 1;
                region.reg_x.repeat = std::numeric_limits<int>::max();

                // For horizontal cuts, Y varies along the cut; X covers full width
                region.reg_y.start = (cut_type == e_interposer_cut_type::HORZ) ? start : 0;
                region.reg_y.end = (cut_type == e_interposer_cut_type::HORZ) ? end : grid_height - 1;
                region.reg_y.incr = (cut_type == e_interposer_cut_type::HORZ) ? incr : 1;
                region.reg_y.repeat = std::numeric_limits<int>::max();

                // Step 6: Build an SG location entry describing where to instantiate links
                t_sg_location sg_location{.type = e_sb_location::E_XY_SPECIFIED,
                                          .region = region,
                                          .num = wire_inf.num,
                                          .sg_link_name = wire_inf.sg_link};

                // Step 7: Append this region definition to the matched SG pattern
                sg_it->sg_locations.push_back(std::move(sg_location));
            }
        }
    }
}

void compute_non_3d_sg_link_geometry(const t_bottleneck_link& link,
                                     e_rr_type& chan_type,
                                     int& xlow,
                                     int& xhigh,
                                     int& ylow,
                                     int& yhigh,
                                     Direction& direction) {
    const t_physical_tile_loc& src_loc = link.gather_loc;
    const t_physical_tile_loc& dst_loc = link.scatter_loc;
    VTR_ASSERT_SAFE(src_loc.layer_num == dst_loc.layer_num);

    if (dst_loc.x > src_loc.x) {
        chan_type = e_rr_type::CHANX;
        ylow = yhigh = dst_loc.y;
        xlow = src_loc.x + 1;
        xhigh = dst_loc.x;
        direction = Direction::INC;
    } else if (dst_loc.x < src_loc.x) {
        chan_type = e_rr_type::CHANX;
        ylow = yhigh = dst_loc.y;
        xlow = dst_loc.x + 1;
        xhigh = src_loc.x;
        direction = Direction::DEC;
    } else if (dst_loc.y > src_loc.y) {
        chan_type = e_rr_type::CHANY;
        xlow = xhigh = dst_loc.x;
        ylow = src_loc.y + 1;
        yhigh = dst_loc.y;
        direction = Direction::INC;
    } else if (dst_loc.y < src_loc.y) {
        chan_type = e_rr_type::CHANY;
        xlow = xhigh = dst_loc.x;
        ylow = dst_loc.y + 1;
        yhigh = src_loc.y;
        direction = Direction::DEC;
    } else {
        VTR_ASSERT_MSG(false, "Source and destination locations cannot be identical.");
    }

    if (link.bidir) {
        direction = Direction::BIDIR;
    }
}
