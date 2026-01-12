
#include "rr_graph_opin_chan_edges.h"

#include "rr_graph2.h"
#include "rr_graph_sg.h"
#include "rr_graph_builder.h"
#include "rr_graph_view.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "clb2clb_directs.h"
#include "get_parallel_segs.h"
#include "build_scatter_gathers.h"

#include "globals.h"
#include "physical_types_util.h"

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
                                 int layer,
                                 int i,
                                 int j,
                                 e_side side,
                                 const t_pin_to_track_lookup& opin_to_track_map,
                                 const std::vector<vtr::Matrix<int>>& Fc_out,
                                 t_rr_edge_info_set& created_rr_edges,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const DeviceGrid& grid,
                                 const std::vector<t_direct_inf>& directs,
                                 const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                 int num_seg_types);

static void build_unidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                  const RRGraphView& rr_graph,
                                  int layer,
                                  int i,
                                  int j,
                                  e_side side,
                                  const DeviceGrid& grid,
                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                  const t_chan_width& nodes_per_chan,
                                  const t_chan_details& chan_details_x,
                                  const t_chan_details& chan_details_y,
                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                  t_rr_edge_info_set& created_rr_edges,
                                  bool* Fc_clipped,
                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                  const std::vector<t_direct_inf>& directs,
                                  const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                  int num_seg_types,
                                  int& edge_count);
/**
 * @brief Builds bidirectional OPIN-->CHAN edges for a given CLB output pin.
 *
 * Finds all routing tracks that the OPIN at (i, j, layer, ipin) connects to
 * based on the pin-to-track mapping and channel details, and appends the
 * corresponding RR edges to @p rr_edges_to_create.
 */
static int get_bidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                                      int layer,
                                      int i,
                                      int j,
                                      int ipin,
                                      RRNodeId from_rr_node,
                                      t_rr_edge_info_set& rr_edges_to_create,
                                      const t_pin_to_track_lookup& opin_to_track_map,
                                      const t_chan_details& chan_details_x,
                                      const t_chan_details& chan_details_y);

/**
 * @brief Creates unidirectional OPIN-->CHAN edges for a given segment type.
 *
 * Connects an OPIN node to nearby routing tracks (INC/DEC muxes) based on Fc.
 * Uses @p Fc_ofs to stagger connections and may clip Fc if not enough muxes exist
 */
static int get_unidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                                       int layer,
                                       int chan,
                                       int seg,
                                       int Fc,
                                       int seg_type_index,
                                       e_rr_type chan_type,
                                       const t_chan_seg_details* seg_details,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create,
                                       vtr::NdMatrix<int, 3>& Fc_ofs,
                                       int max_len,
                                       const t_chan_width& nodes_per_chan,
                                       bool* Fc_clipped);

/**
 * @brief Adds direct CLB-to-CLB OPIN-->IPIN connections for a given OPIN.
 *
 * Finds and creates all direct pin-to-pin edges (bypassing routing channels)
 * from the OPIN at (layer, x, y, side) to corresponding IPINs defined by
 * architecture-specified direct connections.
 */
static int get_opin_direct_connections(RRGraphBuilder& rr_graph_builder,
                                       const RRGraphView& rr_graph,
                                       int layer,
                                       int x,
                                       int y,
                                       e_side side,
                                       int opin,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create,
                                       const std::vector<t_direct_inf>& directs,
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs);

static RRNodeId pick_best_direct_connect_target_rr_node(const RRGraphView& rr_graph,
                                                        RRNodeId from_rr,
                                                        const std::vector<RRNodeId>& candidate_rr_nodes);

static void build_bidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                 const RRGraphView& rr_graph,
                                 int layer,
                                 int i,
                                 int j,
                                 e_side side,
                                 const t_pin_to_track_lookup& opin_to_track_map,
                                 const std::vector<vtr::Matrix<int>>& Fc_out,
                                 t_rr_edge_info_set& rr_edges_to_create,
                                 const t_chan_details& chan_details_x,
                                 const t_chan_details& chan_details_y,
                                 const DeviceGrid& grid,
                                 const std::vector<t_direct_inf>& directs,
                                 const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                 int num_seg_types) {
    // Don't connect pins which are not adjacent to channels around the perimeter
    if ((i == 0 && side != RIGHT)
        || (i == int(grid.width() - 1) && side != LEFT)
        || (j == 0 && side != TOP)
        || (j == int(grid.height() - 1) && side != BOTTOM)) {
        return;
    }

    t_physical_tile_type_ptr type = grid.get_physical_type({i, j, layer});
    int width_offset = grid.get_width_offset({i, j, layer});
    int height_offset = grid.get_height_offset({i, j, layer});

    const vtr::Matrix<int>& Fc = Fc_out[type->index];

    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        // We only are working with opins so skip non-drivers
        if (get_pin_type_from_pin_physical_num(type, pin_index) != e_pin_type::DRIVER) {
            continue;
        }

        // Can't do anything if pin isn't at this location
        if (0 == type->pinloc[width_offset][height_offset][side][pin_index]) {
            continue;
        }

        // get number of tracks that this pin connects to
        int total_pin_Fc = 0;
        for (int iseg = 0; iseg < num_seg_types; iseg++) {
            total_pin_Fc += Fc[pin_index][iseg];
        }

        RRNodeId node_index = rr_graph_builder.node_lookup().find_node(layer, i, j, e_rr_type::OPIN, pin_index, side);
        VTR_ASSERT(node_index);

        if (total_pin_Fc > 0) {
            get_bidir_opin_connections(rr_graph_builder, layer, i, j, pin_index,
                                       node_index, rr_edges_to_create, opin_to_track_map,
                                       chan_details_x,
                                       chan_details_y);
        }

        // Add in direct connections
        get_opin_direct_connections(rr_graph_builder, rr_graph, layer, i, j, side, pin_index,
                                    node_index, rr_edges_to_create,
                                    directs, clb_to_clb_directs);
    }
}

static void build_unidir_rr_opins(RRGraphBuilder& rr_graph_builder,
                                  const RRGraphView& rr_graph,
                                  int layer,
                                  int i,
                                  int j,
                                  e_side side,
                                  const DeviceGrid& grid,
                                  const std::vector<vtr::Matrix<int>>& Fc_out,
                                  const t_chan_width& nodes_per_chan,
                                  const t_chan_details& chan_details_x,
                                  const t_chan_details& chan_details_y,
                                  vtr::NdMatrix<int, 3>& Fc_xofs,
                                  vtr::NdMatrix<int, 3>& Fc_yofs,
                                  t_rr_edge_info_set& rr_edges_to_create,
                                  bool* Fc_clipped,
                                  const t_unified_to_parallel_seg_index& seg_index_map,
                                  const std::vector<t_direct_inf>& directs,
                                  const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                                  int num_seg_types,
                                  int& rr_edge_count) {
    // This routine adds the edges from opins to channels at the specified grid location (i,j) and grid tile side
    *Fc_clipped = false;

    t_physical_tile_type_ptr type = grid.get_physical_type({i, j, layer});
    int width_offset = grid.get_width_offset({i, j, layer});
    int height_offset = grid.get_height_offset({i, j, layer});

    // Go through each pin and find its fanout.
    for (int pin_index = 0; pin_index < type->num_pins; ++pin_index) {
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(type, pin_index);

        // Skip global pins and pins that are not of DRIVER type
        if (pin_type != e_pin_type::DRIVER || type->is_ignored_pin[pin_index]) {
            continue;
        }

        RRNodeId opin_node_index = rr_graph_builder.node_lookup().find_node(layer, i, j, e_rr_type::OPIN, pin_index, side);
        if (!opin_node_index) {
            continue; // No valid from node
        }

        for (int iseg = 0; iseg < num_seg_types; iseg++) {
            // Fc for this segment type
            int seg_type_Fc = Fc_out[type->index][pin_index][iseg];
            VTR_ASSERT(seg_type_Fc >= 0);

            // Skip if the pin is not at this location or is not connected
            if (seg_type_Fc == 0 || !type->pinloc[width_offset][height_offset][side][pin_index]) {
                continue;
            }

            // Figure out the chan seg at that side.
            // side is the side of the logic or io block.
            bool vert = (side == TOP) || (side == BOTTOM);
            bool pos_dir = (side == TOP) || (side == RIGHT);
            e_rr_type chan_type = (vert ? e_rr_type::CHANX : e_rr_type::CHANY);
            int chan = (vert ? (j) : (i));
            int seg = (vert ? (i) : (j));
            int max_len = (vert ? grid.width() : grid.height());
            e_parallel_axis wanted_axis = chan_type == e_rr_type::CHANX ? e_parallel_axis::X_AXIS : e_parallel_axis::Y_AXIS;
            int seg_index = get_parallel_seg_index(iseg, seg_index_map, wanted_axis);

            // The segment at index iseg doesn't have the proper adjacency so skip building Fc_out connections for it.
            if (seg_index < 0) {
                continue;
            }

            vtr::NdMatrix<int, 3>& Fc_ofs = (vert ? Fc_xofs : Fc_yofs);
            if (false == pos_dir) {
                --chan;
            }

            // Skip the location if there is no channel.
            if (chan < 0) {
                continue;
            }
            if (seg < 1) {
                continue;
            }
            if (seg > int(vert ? grid.width() : grid.height()) - 2) { //-2 since no channels around perim
                continue;
            }
            if (chan > int(vert ? grid.height() : grid.width()) - 2) { //-2 since no channels around perim
                continue;
            }

            const t_chan_seg_details* seg_details = (chan_type == e_rr_type::CHANX ? chan_details_x[seg][chan] : chan_details_y[chan][seg]).data();

            if (seg_details[0].length() == 0) {
                continue;
            }

            // Get the list of opin to mux connections for that chan seg.
            bool clipped;

            // Check the pin physical layer and connect it to the same layer if necessary
            rr_edge_count += get_unidir_opin_connections(rr_graph_builder, layer, chan, seg,
                                                         seg_type_Fc, seg_index, chan_type, seg_details,
                                                         opin_node_index,
                                                         rr_edges_to_create,
                                                         Fc_ofs, max_len, nodes_per_chan,
                                                         &clipped);

            if (clipped) {
                *Fc_clipped = true;
            }
        }

        // Add in direct connections
        get_opin_direct_connections(rr_graph_builder, rr_graph, layer, i, j, side, pin_index, opin_node_index, rr_edges_to_create,
                                    directs, clb_to_clb_directs);
    }
}

int get_bidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                               int layer,
                               int i,
                               int j,
                               int ipin,
                               RRNodeId from_rr_node,
                               t_rr_edge_info_set& rr_edges_to_create,
                               const t_pin_to_track_lookup& opin_to_track_map,
                               const t_chan_details& chan_details_x,
                               const t_chan_details& chan_details_y) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type({i, j, layer});
    int width_offset = device_ctx.grid.get_width_offset({i, j, layer});
    int height_offset = device_ctx.grid.get_height_offset({i, j, layer});

    int num_conn = 0;

    // [0..device_ctx.num_block_types-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1]
    for (e_side side : TOTAL_2D_SIDES) {
        // Figure out coords of channel segment based on side
        int tr_i = (side == LEFT) ? i - 1 : i;
        int tr_j = (side == BOTTOM) ? j - 1 : j;

        e_rr_type to_type = ((side == LEFT) || (side == RIGHT)) ? e_rr_type::CHANY : e_rr_type::CHANX;

        int chan = (to_type == e_rr_type::CHANX) ? tr_j : tr_i;
        int seg = (to_type == e_rr_type::CHANX) ? tr_i : tr_j;

        bool vert = !((side == TOP) || (side == BOTTOM));

        // Don't connect where no tracks on fringes
        if (tr_i < 0 || tr_i > int(device_ctx.grid.width() - 2)) { //-2 for no perimeter channels
            continue;
        }
        if (tr_j < 0 || tr_j > int(device_ctx.grid.height() - 2)) { //-2 for no perimeter channels
            continue;
        }
        if (e_rr_type::CHANX == to_type && tr_i < 1) {
            continue;
        }
        if (e_rr_type::CHANY == to_type && tr_j < 1) {
            continue;
        }
        if (opin_to_track_map[type->index].empty()) {
            continue;
        }

        bool is_connected_track = false;

        const t_chan_seg_details* seg_details = (vert ? chan_details_y[chan][seg] : chan_details_x[seg][chan]).data();

        // Iterate of the opin to track connections
        for (int to_track : opin_to_track_map[type->index][ipin][width_offset][height_offset][side]) {
            // Skip unconnected connections
            if (UNDEFINED == to_track || is_connected_track) {
                is_connected_track = true;
                VTR_ASSERT(UNDEFINED == opin_to_track_map[type->index][ipin][width_offset][height_offset][side][0]);
                continue;
            }

            // Only connect to wire if there is a CB
            if (is_cblock(chan, seg, to_track, seg_details)) {
                RRNodeId to_node = rr_graph_builder.node_lookup().find_node(layer, tr_i, tr_j, to_type, to_track);

                if (!to_node) {
                    continue;
                }

                int to_switch = seg_details[to_track].arch_wire_switch();
                rr_edges_to_create.emplace_back(from_rr_node, to_node, to_switch, false);

                ++num_conn;
            }
        }
    }

    return num_conn;
}

int get_unidir_opin_connections(RRGraphBuilder& rr_graph_builder,
                                int layer,
                                int chan,
                                int seg,
                                int Fc,
                                int seg_type_index,
                                e_rr_type chan_type,
                                const t_chan_seg_details* seg_details,
                                RRNodeId from_rr_node,
                                t_rr_edge_info_set& rr_edges_to_create,
                                vtr::NdMatrix<int, 3>& Fc_ofs,
                                int max_len,
                                const t_chan_width& nodes_per_chan,
                                bool* Fc_clipped) {
    *Fc_clipped = false;

    // Fc is assigned in pairs so check it is even.
    VTR_ASSERT(Fc % 2 == 0);

    // get_rr_node_indices needs x and y coords.
    int x = (e_rr_type::CHANX == chan_type) ? seg : chan;
    int y = (e_rr_type::CHANX == chan_type) ? chan : seg;

    // Get the lists of possible muxes.
    int dummy;
    std::vector<int> inc_muxes;
    std::vector<int> dec_muxes;
    int num_inc_muxes, num_dec_muxes;
    // Determine the channel width instead of using max channels to not create hanging nodes
    int max_chan_width = (e_rr_type::CHANX == chan_type) ? nodes_per_chan.x_list[y] : nodes_per_chan.y_list[x];

    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     Direction::INC, max_chan_width, true, inc_muxes, &num_inc_muxes, &dummy);
    label_wire_muxes(chan, seg, seg_details, seg_type_index, max_len,
                     Direction::DEC, max_chan_width, true, dec_muxes, &num_dec_muxes, &dummy);

    // Clip Fc to the number of muxes.
    if ((Fc / 2 > num_inc_muxes) || (Fc / 2 > num_dec_muxes)) {
        *Fc_clipped = true;
        Fc = 2 * std::min(num_inc_muxes, num_dec_muxes);
    }

    // Assign tracks to meet Fc demand
    int num_edges = 0;
    for (int iconn = 0; iconn < (Fc / 2); ++iconn) {
        // Figure of the next mux to use for the 'inc' and 'dec' connections
        int inc_mux = Fc_ofs[chan][seg][seg_type_index] % num_inc_muxes;
        int dec_mux = Fc_ofs[chan][seg][seg_type_index] % num_dec_muxes;
        ++Fc_ofs[chan][seg][seg_type_index];

        // Figure out the track it corresponds to.
        int inc_track = inc_muxes[inc_mux];
        int dec_track = dec_muxes[dec_mux];

        // Figure the inodes of those muxes
        RRNodeId inc_inode_index = rr_graph_builder.node_lookup().find_node(layer, x, y, chan_type, inc_track);
        RRNodeId dec_inode_index = rr_graph_builder.node_lookup().find_node(layer, x, y, chan_type, dec_track);

        if (!inc_inode_index || !dec_inode_index) {
            continue;
        }

        // Add to the list.
        short to_switch = seg_details[inc_track].arch_opin_switch();
        rr_edges_to_create.emplace_back(from_rr_node, inc_inode_index, to_switch, false);
        ++num_edges;

        to_switch = seg_details[dec_track].arch_opin_switch();
        rr_edges_to_create.emplace_back(from_rr_node, dec_inode_index, to_switch, false);
        ++num_edges;
    }

    return num_edges;
}

static int get_opin_direct_connections(RRGraphBuilder& rr_graph_builder,
                                       const RRGraphView& rr_graph,
                                       int layer,
                                       int x,
                                       int y,
                                       e_side side,
                                       int opin,
                                       RRNodeId from_rr_node,
                                       t_rr_edge_info_set& rr_edges_to_create,
                                       const std::vector<t_direct_inf>& directs,
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr curr_type = device_ctx.grid.get_physical_type({x, y, layer});

    int num_pins = 0;

    int width_offset = device_ctx.grid.get_width_offset({x, y, layer});
    int height_offset = device_ctx.grid.get_height_offset({x, y, layer});
    if (!curr_type->pinloc[width_offset][height_offset][side][opin]) {
        return num_pins; //No source pin on this side
    }

    // Capacity location determined by pin number relative to pins per capacity instance
    auto [z, relative_opin] = get_capacity_location_from_physical_pin(curr_type, opin);
    VTR_ASSERT(z >= 0 && z < curr_type->capacity);
    const int num_directs = directs.size();

    // Iterate through all direct connections
    for (int i = 0; i < num_directs; i++) {
        // Find matching direct clb-to-clb connections with the same type as current grid location
        if (clb_to_clb_directs[i].from_clb_type == curr_type) { //We are at a valid starting point
            if (directs[i].from_side != NUM_2D_SIDES && directs[i].from_side != side) continue;

            // Offset must be in range
            if (x + directs[i].x_offset < int(device_ctx.grid.width() - 1)
                && x + directs[i].x_offset > 0
                && y + directs[i].y_offset < int(device_ctx.grid.height() - 1)
                && y + directs[i].y_offset > 0) {
                // Only add connections if the target clb type matches the type in the direct specification
                t_physical_tile_type_ptr target_type = device_ctx.grid.get_physical_type({x + directs[i].x_offset,
                                                                                          y + directs[i].y_offset,
                                                                                          layer});

                if (clb_to_clb_directs[i].to_clb_type == target_type
                    && z + directs[i].sub_tile_offset < int(target_type->capacity)
                    && z + directs[i].sub_tile_offset >= 0) {
                    // Compute index of opin with regards to given pins
                    int max_index, min_index;
                    bool swap = false;
                    if (clb_to_clb_directs[i].from_clb_pin_start_index > clb_to_clb_directs[i].from_clb_pin_end_index) {
                        swap = true;
                        max_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        min_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    } else {
                        swap = false;
                        min_index = clb_to_clb_directs[i].from_clb_pin_start_index;
                        max_index = clb_to_clb_directs[i].from_clb_pin_end_index;
                    }

                    if (max_index >= relative_opin && min_index <= relative_opin) {
                        int offset = relative_opin - min_index;
                        // This opin is specified to connect directly to an ipin, now compute which ipin to connect to
                        int relative_ipin;
                        if (clb_to_clb_directs[i].to_clb_pin_start_index > clb_to_clb_directs[i].to_clb_pin_end_index) {
                            if (swap) {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index + offset;
                            } else {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index - offset;
                            }
                        } else {
                            if (swap) {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_end_index - offset;
                            } else {
                                relative_ipin = clb_to_clb_directs[i].to_clb_pin_start_index + offset;
                            }
                        }

                        // directs[i].sub_tile_offset is added to from_capacity(z) to get the
                        // target_capacity
                        int target_cap = z + directs[i].sub_tile_offset;

                        // Iterate over all sub_tiles to get the sub_tile which the target_cap belongs to.
                        const t_sub_tile* target_sub_tile = nullptr;
                        for (const t_sub_tile& sub_tile : target_type->sub_tiles) {
                            if (sub_tile.capacity.is_in_range(target_cap)) {
                                target_sub_tile = &sub_tile;
                                break;
                            }
                        }
                        VTR_ASSERT(target_sub_tile != nullptr);
                        if (relative_ipin >= target_sub_tile->num_phy_pins) continue;

                        // If this block has capacity > 1 then the pins of z position > 0 are offset
                        // by the number of pins per capacity instance
                        int ipin = get_physical_pin_from_capacity_location(target_type, relative_ipin, target_cap);

                        // Add new ipin edge to list of edges
                        std::vector<RRNodeId> inodes;

                        int target_width_offset = device_ctx.grid.get_width_offset({x + directs[i].x_offset, y + directs[i].y_offset, layer});
                        int target_height_offset = device_ctx.grid.get_height_offset({x + directs[i].x_offset, y + directs[i].y_offset, layer});
                        int final_ipin_x = x + directs[i].x_offset - target_width_offset + target_type->pin_width_offset[ipin];
                        int final_ipin_y = y + directs[i].y_offset - target_height_offset + target_type->pin_height_offset[ipin];

                        if (directs[i].to_side != NUM_2D_SIDES) {
                            // Explicit side specified, only create if pin exists on that side
                            RRNodeId inode = rr_graph_builder.node_lookup().find_node(layer, final_ipin_x, final_ipin_y,
                                                                                      e_rr_type::IPIN, ipin, directs[i].to_side);
                            if (inode) {
                                inodes.push_back(inode);
                            }
                        } else {
                            // No side specified, get all candidates
                            inodes = rr_graph_builder.node_lookup().find_nodes_at_all_sides(layer, final_ipin_x, final_ipin_y, e_rr_type::IPIN, ipin);
                        }

                        if (!inodes.empty()) {
                            // There may be multiple physical pins corresponding to the logical
                            // target ipin. We only need to connect to one of them (since the physical pins
                            // are logically equivalent). This also ensures the graphics look reasonable and map
                            // back fairly directly to the architecture file in the case of pin equivalence
                            RRNodeId inode = pick_best_direct_connect_target_rr_node(rr_graph, from_rr_node, inodes);

                            rr_edges_to_create.emplace_back(from_rr_node, inode, clb_to_clb_directs[i].switch_index, false);
                            ++num_pins;
                        }
                    }
                }
            }
        }
    }
    return num_pins;
}

static RRNodeId pick_best_direct_connect_target_rr_node(const RRGraphView& rr_graph,
                                                        RRNodeId from_rr,
                                                        const std::vector<RRNodeId>& candidate_rr_nodes) {
    // With physically equivalent pins there may be multiple candidate rr nodes (which are equivalent)
    // to connect the direct edge to.
    // As a result it does not matter (from a correctness standpoint) which is picked.
    //
    // However intuitively we would expect (e.g. when visualizing the drawn RR graph) that the 'closest'
    // candidate would be picked (i.e. to minimize the drawn edge length).
    //
    // This function attempts to pick the 'best/closest' of the candidates.
    VTR_ASSERT(rr_graph.node_type(from_rr) == e_rr_type::OPIN);

    float best_dist = std::numeric_limits<float>::infinity();
    RRNodeId best_rr = RRNodeId::INVALID();

    for (const e_side& from_side : TOTAL_2D_SIDES) {
        /* Bypass those side where the node does not appear */
        if (!rr_graph.is_node_on_specific_side(from_rr, from_side)) {
            continue;
        }

        for (RRNodeId to_rr : candidate_rr_nodes) {
            VTR_ASSERT(rr_graph.node_type(to_rr) == e_rr_type::IPIN);
            float to_dist = std::abs(rr_graph.node_xlow(from_rr) - rr_graph.node_xlow(to_rr))
                            + std::abs(rr_graph.node_ylow(from_rr) - rr_graph.node_ylow(to_rr));

            for (const e_side& to_side : TOTAL_2D_SIDES) {
                // Bypass those side where the node does not appear
                if (!rr_graph.is_node_on_specific_side(to_rr, to_side)) {
                    continue;
                }

                // Include a partial unit of distance based on side alignment to ensure
                // we prefer facing sides
                if ((from_side == RIGHT && to_side == LEFT)
                    || (from_side == LEFT && to_side == RIGHT)
                    || (from_side == TOP && to_side == BOTTOM)
                    || (from_side == BOTTOM && to_side == TOP)) {
                    // Facing sides
                    to_dist += 0.25;
                } else if (((from_side == RIGHT || from_side == LEFT) && (to_side == TOP || to_side == BOTTOM))
                           || ((from_side == TOP || from_side == BOTTOM) && (to_side == RIGHT || to_side == LEFT))) {
                    // Perpendicular sides
                    to_dist += 0.5;

                } else {
                    // Opposite sides
                    to_dist += 0.75;
                }

                if (to_dist < best_dist) {
                    best_dist = to_dist;
                    best_rr = to_rr;
                }
            }
        }
    }

    VTR_ASSERT(best_rr);

    return best_rr;
}

void add_opin_chan_edges(RRGraphBuilder& rr_graph_builder,
                         const RRGraphView& rr_graph,
                         size_t num_seg_types,
                         size_t num_seg_types_x,
                         size_t num_seg_types_y,
                         const t_chan_width& chan_width,
                         const t_chan_details& chan_details_x,
                         const t_chan_details& chan_details_y,
                         const t_unified_to_parallel_seg_index& seg_index_map,
                         const t_pin_to_track_lookup& opin_to_track_map,
                         const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                         const std::vector<vtr::Matrix<int>>& Fc_out,
                         const std::vector<t_direct_inf>& directs,
                         const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs,
                         e_directionality directionality,
                         int& num_edges,
                         int& rr_edges_before_directs,
                         bool* Fc_clipped) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    e_3d_opin_connectivity_type opin_chanz_connectivity = g_vpr_ctx.device().arch->opin_chanz_connectivity_type;

    t_rr_edge_info_set rr_edges_to_create;

    // These are data structures used by the unidir opin mapping. They are used
    // to spread connections evenly for each segment type among the available
    // wire start points
    // [0..grid.height()-2][0..grid.width()-2][0..num_seg_types_x/y-1]
    vtr::NdMatrix<int, 3> Fc_xofs({grid.height() - 1, grid.width() - 1, num_seg_types_x}, 0);
    vtr::NdMatrix<int, 3> Fc_yofs({grid.width() - 1, grid.height() - 1, num_seg_types_y}, 0);

    vtr::NdMatrix<int, 3> Fc_zofs({grid.width(), grid.height(), num_seg_types_y}, 0);

    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        for (size_t i = 0; i < grid.width(); ++i) {
            for (size_t j = 0; j < grid.height(); ++j) {
                for (e_side side : TOTAL_2D_SIDES) {
                    if (BI_DIRECTIONAL == directionality) {
                        build_bidir_rr_opins(rr_graph_builder, rr_graph, layer, i, j, side,
                                             opin_to_track_map, Fc_out, rr_edges_to_create, chan_details_x,
                                             chan_details_y,
                                             grid,
                                             directs, clb_to_clb_directs, num_seg_types);
                    } else {
                        VTR_ASSERT(UNI_DIRECTIONAL == directionality);
                        bool clipped;
                        build_unidir_rr_opins(rr_graph_builder, rr_graph, layer, i, j, side, grid, Fc_out, chan_width,
                                              chan_details_x, chan_details_y, Fc_xofs, Fc_yofs,
                                              rr_edges_to_create, &clipped, seg_index_map,
                                              directs, clb_to_clb_directs, num_seg_types,
                                              rr_edges_before_directs);
                        if (clipped) {
                            *Fc_clipped = true;
                        }
                    }

                    if (opin_chanz_connectivity == e_3d_opin_connectivity_type::PER_SIDE) {
                        add_edges_opin_chanz_per_side(rr_graph,
                                                      layer, i, j,
                                                      side,
                                                      Fc_out,
                                                      seg_index_map,
                                                      num_seg_types,
                                                      Fc_zofs,
                                                      rr_edges_to_create,
                                                      interdie_3d_links);
                    }
                }

                if (opin_chanz_connectivity == e_3d_opin_connectivity_type::PER_BLOCK) {
                    add_edges_opin_chanz_per_block(rr_graph,
                                                   layer, i, j,
                                                   Fc_out,
                                                   seg_index_map,
                                                   num_seg_types,
                                                   rr_edges_to_create,
                                                   interdie_3d_links[i][j]);
                }

                // Create the actual OPIN->CHANX/CHANY edges
                uniquify_edges(rr_edges_to_create);
                rr_graph_builder.alloc_and_load_edges(&rr_edges_to_create);
                num_edges += rr_edges_to_create.size();
                rr_edges_to_create.clear();
            }
        }

        Fc_zofs.fill(0);
    }
}
