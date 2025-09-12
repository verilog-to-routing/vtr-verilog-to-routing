
#include "rr_node_indices.h"

#include "build_scatter_gathers.h"
#include "describe_rr_node.h"
#include "globals.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "rr_graph2.h"
#include "vpr_utils.h"

/**
 * @brief Assigns and loads rr_node indices for block-level routing resources (SOURCE, SINK, IPIN, OPIN).
 *
 * This function walks through the device grid and assigns unique rr_node indices to the routing resources
 * associated with each block (tiles).
 *
 * For SINKs and SOURCEs, it uses side 0 by convention (since they have no geometric side). For IPINs and OPINs,
 * it determines the correct sides based on the tile's position in the grid, following special rules for
 * edge and corner tiles.
 *
 * The index counter is passed and updated as rr_nodes are added.
 */
static void load_block_rr_indices(RRGraphBuilder& rr_graph_builder,
                                  const DeviceGrid& grid,
                                  int* index);

/**
 * @brief Populates the lookup indices for channel (CHANX or CHANY) RR nodes.
 *
 * This function builds part of the RR spatial lookup structure, specifically
 * the RR nodes associated with routing channels (CHANX or CHANY).
 *
 * @param max_chan_width   Maximum channel width (number of tracks).
 * @param grid             Device grid layout.
 * @param chan_len         Length of the channel being processed.
 * @param num_chans        Total number of channels in the direction being processed.
 * @param type             RR node type: should be CHANX or CHANY.
 * @param chan_details     Channel details used to determine segment and track information.
 * @param node_lookup      Spatial RR node lookup to be filled by this function.
 * @param index            The next available RR node index.
 */
static void load_chan_rr_indices(const int max_chan_width,
                                 const DeviceGrid& grid,
                                 const int chan_len,
                                 const int num_chans,
                                 const e_rr_type type,
                                 const t_chan_details& chan_details,
                                 RRSpatialLookup& node_lookup,
                                 int* index);

static void add_classes_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                       t_physical_tile_type_ptr physical_type_ptr,
                                       const std::vector<int>& class_num_vec,
                                       const t_physical_tile_loc& root_loc,
                                       int block_width,
                                       int block_height,
                                       int* index);

static void add_pins_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                    t_physical_tile_type_ptr physical_type_ptr,
                                    const std::vector<int>& pin_num_vec,
                                    const t_physical_tile_loc& root_loc,
                                    int* index,
                                    const std::vector<e_side>& wanted_sides);

/**
 * @brief Check consistency between RR node count and expected coverage from RR nodes
 *
 * This helper validates that each RR node appears the correct number of times
 * in the node lookup structure based on its span or area.
 */
static void check_rr_node_counts(const std::unordered_map<RRNodeId, int>& rr_node_counts,
                                 const RRGraphView& rr_graph,
                                 const t_rr_graph_storage& rr_nodes,
                                 const DeviceGrid& grid,
                                 const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                 bool is_flat);

/* As the rr_indices builders modify a local copy of indices, use the local copy in the builder
 * TODO: these building functions should only talk to a RRGraphBuilder object
 */
static void load_block_rr_indices(RRGraphBuilder& rr_graph_builder,
                                  const DeviceGrid& grid,
                                  int* index) {
    // Walk through the grid assigning indices to SOURCE/SINK IPIN/OPIN
    for (const t_physical_tile_loc grid_loc : grid.all_locations()) {
        //Process each block from its root location
        if (grid.is_root_location(grid_loc)) {
            t_physical_tile_type_ptr physical_type = grid.get_physical_type(grid_loc);

            // Assign indices for SINKs and SOURCEs
            // Note that SINKS/SOURCES have no side, so we always use side 0
            std::vector<int> class_num_vec = get_tile_root_classes(physical_type);
            std::vector<int> pin_num_vec = get_tile_root_pins(physical_type);

            add_classes_spatial_lookup(rr_graph_builder,
                                       physical_type,
                                       class_num_vec,
                                       grid_loc,
                                       physical_type->width,
                                       physical_type->height,
                                       index);

            /* Limited sides for grids
             *   The wanted side depends on the location of the grid.
             *   In particular for perimeter grid,
             *   -------------------------------------------------------
             *   Grid location |  IPIN side
             *   -------------------------------------------------------
             *   TOP           |  BOTTOM
             *   -------------------------------------------------------
             *   RIGHT         |  LEFT
             *   -------------------------------------------------------
             *   BOTTOM        |  TOP
             *   -------------------------------------------------------
             *   LEFT          |  RIGHT
             *   -------------------------------------------------------
             *   TOP-LEFT      |  BOTTOM & RIGHT
             *   -------------------------------------------------------
             *   TOP-RIGHT     |  BOTTOM & LEFT
             *   -------------------------------------------------------
             *   BOTTOM-LEFT   |  TOP & RIGHT
             *   -------------------------------------------------------
             *   BOTTOM-RIGHT  |  TOP & LEFT
             *   -------------------------------------------------------
             *   Other         |  First come first fit
             *   -------------------------------------------------------
             *
             * Special for IPINs:
             *   If there are multiple wanted sides, first come first fit is applied
             *   This guarantee that there is only a unique rr_node
             *   for the same input pin on multiple sides, and thus avoid multiple driver problems
             */
            std::vector<e_side> wanted_sides;
            if ((int)grid.height() - 1 == grid_loc.y) { // TOP side
                wanted_sides.push_back(BOTTOM);
            }
            if ((int)grid.width() - 1 == grid_loc.x) { // RIGHT side
                wanted_sides.push_back(LEFT);
            }
            if (0 == grid_loc.y) { // BOTTOM side
                wanted_sides.push_back(TOP);
            }
            if (0 == grid_loc.x) { // LEFT side
                wanted_sides.push_back(RIGHT);
            }

            // If wanted sides is empty still, this block does not have specific wanted sides, Deposit all the sides
            if (wanted_sides.empty()) {
                for (e_side side : TOTAL_2D_SIDES) {
                    wanted_sides.push_back(side);
                }
            }

            add_pins_spatial_lookup(rr_graph_builder,
                                    physical_type,
                                    pin_num_vec,
                                    grid_loc,
                                    index,
                                    wanted_sides);
        }
    }
}

static void load_chan_rr_indices(const int max_chan_width,
                                 const DeviceGrid& grid,
                                 const int chan_len,
                                 const int num_chans,
                                 const e_rr_type type,
                                 const t_chan_details& chan_details,
                                 RRSpatialLookup& node_lookup,
                                 int* index) {
    const auto& device_ctx = g_vpr_ctx.device();

    for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
        // Skip the current die if architecture file specifies that it doesn't require global resource routing
        if (!device_ctx.inter_cluster_prog_routing_resources.at(layer)) {
            continue;
        }

        for (int chan = 0; chan < num_chans - 1; ++chan) {
            for (int seg = 1; seg < chan_len - 1; ++seg) {
                // Assign an inode to the starts of tracks
                const int x = (type == e_rr_type::CHANX) ? seg : chan;
                const int y = (type == e_rr_type::CHANX) ? chan : seg;
                const t_chan_seg_details* seg_details = chan_details[x][y].data();

                // Reserve nodes in lookup to save memory
                node_lookup.reserve_nodes(layer, x, y, type, max_chan_width);

                for (int track = 0; track < max_chan_width; ++track) {
                    /* TODO: May let the length() == 0 case go through, to model muxes */
                    if (seg_details[track].length() <= 0)
                        continue;

                    int start = get_seg_start(seg_details, track, chan, seg);
                    int node_start_x = (type == e_rr_type::CHANX) ? start : chan;
                    int node_start_y = (type == e_rr_type::CHANX) ? chan : start;

                    // If the start of the wire doesn't have an RRNodeId, assign one to it.
                    RRNodeId inode = node_lookup.find_node(layer, node_start_x, node_start_y, type, track);
                    if (!inode) {
                        inode = RRNodeId(*index);
                        ++(*index);
                        node_lookup.add_node(inode, layer, node_start_x, node_start_y, type, track);
                    }

                    // Assign RRNodeId of start of wire to current position
                    node_lookup.add_node(inode, layer, x, y, type, track);
                }
            }
        }
    }
}

static void add_classes_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                       t_physical_tile_type_ptr physical_type_ptr,
                                       const std::vector<int>& class_num_vec,
                                       const t_physical_tile_loc& root_loc,
                                       int block_width,
                                       int block_height,
                                       int* index) {
    for (int x_tile = root_loc.x; x_tile < (root_loc.x + block_width); x_tile++) {
        for (int y_tile = root_loc.y; y_tile < (root_loc.y + block_height); y_tile++) {
            rr_graph_builder.node_lookup().reserve_nodes(root_loc.layer_num, x_tile, y_tile, e_rr_type::SOURCE, class_num_vec.size(), TOTAL_2D_SIDES[0]);
            rr_graph_builder.node_lookup().reserve_nodes(root_loc.layer_num, x_tile, y_tile, e_rr_type::SINK, class_num_vec.size(), TOTAL_2D_SIDES[0]);
        }
    }

    for (const int class_num : class_num_vec) {
        e_pin_type class_type = get_class_type_from_class_physical_num(physical_type_ptr, class_num);
        e_rr_type node_type = e_rr_type::SINK;
        if (class_type == e_pin_type::DRIVER) {
            node_type = e_rr_type::SOURCE;
        } else {
            VTR_ASSERT(class_type == e_pin_type::RECEIVER);
        }

        for (int x_offset = 0; x_offset < block_width; x_offset++) {
            for (int y_offset = 0; y_offset < block_height; y_offset++) {
                int curr_x = root_loc.x + x_offset;
                int curr_y = root_loc.y + y_offset;

                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), root_loc.layer_num, curr_x, curr_y, node_type, class_num);
            }
        }

        ++(*index);
    }
}

static void add_pins_spatial_lookup(RRGraphBuilder& rr_graph_builder,
                                    t_physical_tile_type_ptr physical_type_ptr,
                                    const std::vector<int>& pin_num_vec,
                                    const t_physical_tile_loc& root_loc,
                                    int* index,
                                    const std::vector<e_side>& wanted_sides) {
    for (e_side side : wanted_sides) {
        for (int width_offset = 0; width_offset < physical_type_ptr->width; ++width_offset) {
            int x_tile = root_loc.x + width_offset;
            for (int height_offset = 0; height_offset < physical_type_ptr->height; ++height_offset) {
                int y_tile = root_loc.y + height_offset;
                // only nodes on the tile may be located in a location other than the root-location
                rr_graph_builder.node_lookup().reserve_nodes(root_loc.layer_num, x_tile, y_tile, e_rr_type::OPIN, physical_type_ptr->num_pins, side);
                rr_graph_builder.node_lookup().reserve_nodes(root_loc.layer_num, x_tile, y_tile, e_rr_type::IPIN, physical_type_ptr->num_pins, side);
            }
        }
    }

    for (const int pin_num : pin_num_vec) {
        bool assigned_to_rr_node = false;
        const auto [x_offset, y_offset, pin_sides] = get_pin_coordinates(physical_type_ptr, pin_num, wanted_sides);
        e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_type_ptr, pin_num);
        for (int pin_coord_idx = 0; pin_coord_idx < (int)pin_sides.size(); pin_coord_idx++) {
            int x_tile = root_loc.x + x_offset[pin_coord_idx];
            int y_tile = root_loc.y + y_offset[pin_coord_idx];
            e_side side = pin_sides[pin_coord_idx];
            if (pin_type == e_pin_type::DRIVER) {
                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), root_loc.layer_num, x_tile, y_tile, e_rr_type::OPIN, pin_num, side);
                assigned_to_rr_node = true;
            } else {
                VTR_ASSERT(pin_type == e_pin_type::RECEIVER);
                rr_graph_builder.node_lookup().add_node(RRNodeId(*index), root_loc.layer_num, x_tile, y_tile, e_rr_type::IPIN, pin_num, side);
                assigned_to_rr_node = true;
            }
        }
        /* A pin may locate on multiple sides of a tile.
         * Instead of allocating multiple rr_nodes for the pin,
         * we just create a rr_node and make it indexable on these sides
         * As such, we can avoid redundant rr_node to be allocated
         * and multiple nets to be mapped to the pin
         *
         * Considering that some pin could be just dangling, we do not need
         * to create a void rr_node for it.
         * As such, we only allocate a rr node when the pin is indeed located
         * on at least one side
         */
        if (assigned_to_rr_node) {
            ++(*index);
        }
    }
}

void alloc_and_load_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                    const t_chan_width& nodes_per_chan,
                                    const DeviceGrid& grid,
                                    int* index,
                                    const t_chan_details& chan_details_x,
                                    const t_chan_details& chan_details_y) {
    // Alloc the lookup table
    for (e_rr_type rr_type : RR_TYPES) {
        rr_graph_builder.node_lookup().resize_nodes(grid.get_num_layers(), grid.width(), grid.height(), rr_type, NUM_2D_SIDES);
    }

    // Assign indices for block nodes
    load_block_rr_indices(rr_graph_builder, grid, index);

    // Load the data for x and y channels
    load_chan_rr_indices(nodes_per_chan.x_max, grid, grid.width(), grid.height(),
                         e_rr_type::CHANX, chan_details_x, rr_graph_builder.node_lookup(), index);
    load_chan_rr_indices(nodes_per_chan.y_max, grid, grid.height(), grid.width(),
                         e_rr_type::CHANY, chan_details_y, rr_graph_builder.node_lookup(), index);
}

void alloc_and_load_inter_die_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                              const vtr::NdMatrix<std::vector<t_bottleneck_link>, 2>& interdie_3d_links,
                                              int* index) {
    // In case of multi-die FPGAs, we add extra nodes of type CHANZ to
    // support inter-die communication coming from switch blocks (connection between two tracks in different layers)
    // The extra nodes have the following attribute:
    // 1) type = CHANZ
    // 2) xhigh == xlow, yhigh == ylow
    // 3) ptc = [0:number_of_connection-1]
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    for (size_t x = 0; x < grid.width(); x++) {
        for (size_t y = 0; y < grid.height(); y++) {
            const int num_chanz_nodes = interdie_3d_links[x][y].size();

            // reserve extra nodes for inter-die track-to-track connection
            for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
                rr_graph_builder.node_lookup().reserve_nodes(layer, x, y, e_rr_type::CHANZ, num_chanz_nodes);
            }

            for (int track_num = 0; track_num < num_chanz_nodes; track_num++) {
                bool incremnet_index = false;
                for (size_t layer = 0; layer < grid.get_num_layers(); layer++) {
                    RRNodeId inode = rr_graph_builder.node_lookup().find_node(layer, x, y, e_rr_type::CHANZ, track_num);
                    if (!inode) {
                        inode = RRNodeId(*index);
                        rr_graph_builder.node_lookup().add_node(inode, layer, x, y, e_rr_type::CHANZ, track_num);
                        incremnet_index = true;
                    }
                }

                if (incremnet_index) {
                    ++(*index);
                }
            }
        }
    }
}

void alloc_and_load_tile_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                         t_physical_tile_type_ptr physical_tile,
                                         const t_physical_tile_loc& root_loc,
                                         int* num_rr_nodes) {
    std::vector<e_side> wanted_sides{TOP, BOTTOM, LEFT, RIGHT};
    auto class_num_range = get_flat_tile_primitive_classes(physical_tile);
    auto pin_num_vec = get_flat_tile_pins(physical_tile);

    std::vector<int> class_num_vec(class_num_range.total_num());
    std::iota(class_num_vec.begin(), class_num_vec.end(), class_num_range.low);

    add_classes_spatial_lookup(rr_graph_builder,
                               physical_tile,
                               class_num_vec,
                               root_loc,
                               physical_tile->width,
                               physical_tile->height,
                               num_rr_nodes);

    add_pins_spatial_lookup(rr_graph_builder,
                            physical_tile,
                            pin_num_vec,
                            root_loc,
                            num_rr_nodes,
                            wanted_sides);
}

void alloc_and_load_intra_cluster_rr_node_indices(RRGraphBuilder& rr_graph_builder,
                                                  const DeviceGrid& grid,
                                                  const vtr::vector<ClusterBlockId, t_cluster_pin_chain>& pin_chains,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<int>>& pin_chains_num,
                                                  int* index) {

    for (const t_physical_tile_loc grid_loc : grid.all_locations()) {

        // Process each block from its root location
        if (grid.is_root_location(grid_loc)) {
            t_physical_tile_type_ptr physical_type = grid.get_physical_type(grid_loc);
            // Assign indices for SINKs and SOURCEs
            // Note that SINKS/SOURCES have no side, so we always use side 0
            std::vector<int> class_num_vec;
            std::vector<int> pin_num_vec;
            class_num_vec = get_cluster_netlist_intra_tile_classes_at_loc(grid_loc, physical_type);
            pin_num_vec = get_cluster_netlist_intra_tile_pins_at_loc(grid_loc,
                                                                     pin_chains,
                                                                     pin_chains_num,
                                                                     physical_type);
            add_classes_spatial_lookup(rr_graph_builder,
                                       physical_type,
                                       class_num_vec,
                                       grid_loc,
                                       physical_type->width,
                                       physical_type->height,
                                       index);

            std::vector<e_side> wanted_sides;
            wanted_sides.push_back(e_side::TOP);
            add_pins_spatial_lookup(rr_graph_builder,
                                    physical_type,
                                    pin_num_vec,
                                    grid_loc,
                                    index,
                                    wanted_sides);
        }
    }
}

bool verify_rr_node_indices(const DeviceGrid& grid,
                            const RRGraphView& rr_graph,
                            const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const t_rr_graph_storage& rr_nodes,
                            bool is_flat) {
    std::unordered_map<RRNodeId, int> rr_node_counts;

    int width = grid.width();
    int height = grid.height();
    int layer = grid.get_num_layers();

    for (int l = 0; l < layer; ++l) {
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                for (e_rr_type rr_type : RR_TYPES) {
                    // Get the list of nodes at a specific location (x, y)
                    std::vector<RRNodeId> nodes_from_lookup;
                    if (rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY || rr_type == e_rr_type::CHANZ) {
                        nodes_from_lookup = rr_graph.node_lookup().find_channel_nodes(l, x, y, rr_type);
                    } else {
                        nodes_from_lookup = rr_graph.node_lookup().find_grid_nodes_at_all_sides(l, x, y, rr_type);
                    }

                    for (RRNodeId inode : nodes_from_lookup) {
                        rr_node_counts[inode]++;

                        if (rr_graph.node_type(inode) != rr_type) {
                            VPR_ERROR(VPR_ERROR_ROUTE, "RR node type does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                                      rr_node_typename[rr_graph.node_type(inode)],
                                      rr_node_typename[rr_type],
                                      describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                        }

                        if (l < rr_graph.node_layer_low(inode) && l > rr_graph.node_layer_high(inode)) {
                            VPR_ERROR(VPR_ERROR_ROUTE, "RR node layer does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                                      rr_node_typename[rr_graph.node_type(inode)],
                                      rr_node_typename[rr_type],
                                      describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                        }

                        if (rr_graph.node_type(inode) == e_rr_type::CHANX) {
                            VTR_ASSERT_MSG(rr_graph.node_ylow(inode) == rr_graph.node_yhigh(inode), "CHANX should be horizontal");
                            if (y != rr_graph.node_ylow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.x_in_node_range(x, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else if (rr_graph.node_type(inode) == e_rr_type::CHANY) {
                            VTR_ASSERT_MSG(rr_graph.node_xlow(inode) == rr_graph.node_xhigh(inode), "CHANY should be vertical");

                            if (x != rr_graph.node_xlow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.y_in_node_range(y, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else if (rr_graph.node_type(inode) == e_rr_type::CHANZ) {
                            VTR_ASSERT_MSG(rr_graph.node_xlow(inode) == rr_graph.node_xhigh(inode), "CHANZ should move only along layers");
                            VTR_ASSERT_MSG(rr_graph.node_ylow(inode) == rr_graph.node_yhigh(inode), "CHANZ should move only along layers");

                            if (x != rr_graph.node_xlow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (y != rr_graph.node_ylow(inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y position does not agree between rr_nodes (%d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                        } else if (rr_graph.node_type(inode) == e_rr_type::SOURCE || rr_graph.node_type(inode) == e_rr_type::SINK || rr_graph.node_type(inode) == e_rr_type::MUX) {
                            // Sources have co-ordinates covering the entire block they are in, but not sinks
                            if (!rr_graph.x_in_node_range(x, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node x positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_xlow(inode),
                                          rr_graph.node_xlow(inode),
                                          x,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }

                            if (!rr_graph.y_in_node_range(y, inode)) {
                                VPR_ERROR(VPR_ERROR_ROUTE, "RR node y positions do not agree between rr_nodes (%d <-> %d) and rr_node_indices (%d): %s",
                                          rr_graph.node_ylow(inode),
                                          rr_graph.node_ylow(inode),
                                          y,
                                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
                            }
                        } else {
                            VTR_ASSERT(rr_graph.node_type(inode) == e_rr_type::IPIN || rr_graph.node_type(inode) == e_rr_type::OPIN);
                            /* As we allow a pin to be indexable on multiple sides,
                             * This check code should be invalid
                             * if (rr_node.xlow() != x) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node xlow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.xlow(),
                             *               x,
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * }
                             *
                             * if (rr_node.ylow() != y) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node ylow does not match between rr_nodes and rr_node_indices (%d/%d): %s",
                             *               rr_node.ylow(),
                             *               y,
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * }
                             */
                        }

                        if (rr_type == e_rr_type::IPIN || rr_type == e_rr_type::OPIN) {
                            /* As we allow a pin to be indexable on multiple sides,
                             * This check code should be invalid
                             * if (rr_node.side() != side) {
                             *     VPR_ERROR(VPR_ERROR_ROUTE, "RR node xlow does not match between rr_nodes and rr_node_indices (%s/%s): %s",
                             *               TOTAL_2D_SIDE_STRINGS[rr_node.side()],
                             *               TOTAL_2D_SIDE_STRINGS[side],
                             *               describe_rr_node(rr_graph, grid, rr_indexed_data, inode).c_str());
                             * } else {
                             *     VTR_ASSERT(rr_node.side() == side);
                             * }
                             */
                        }
                    }
                }
            }
        }
    }

    check_rr_node_counts(rr_node_counts, rr_graph, rr_nodes, grid, rr_indexed_data, is_flat);

    return true;
}

static void check_rr_node_counts(const std::unordered_map<RRNodeId, int>& rr_node_counts,
                                 const RRGraphView& rr_graph,
                                 const t_rr_graph_storage& rr_nodes,
                                 const DeviceGrid& grid,
                                 const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                 bool is_flat) {
    if (rr_node_counts.size() != rr_nodes.size()) {
        VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch in number of unique RR nodes in rr_nodes (%zu) and rr_node_indices (%zu)",
                  rr_nodes.size(),
                  rr_node_counts.size());
    }

    for (const auto& [inode, count] : rr_node_counts) {
        const t_rr_node& rr_node = rr_nodes[size_t(inode)];
        e_rr_type node_type = rr_graph.node_type(inode);

        if (node_type == e_rr_type::SOURCE || node_type == e_rr_type::SINK) {
            int rr_width = rr_graph.node_xhigh(inode) - rr_graph.node_xlow(inode) + 1;
            int rr_height = rr_graph.node_yhigh(inode) - rr_graph.node_ylow(inode) + 1;
            int rr_area = rr_width * rr_height;

            if (count != rr_area) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node area (%d) and count within rr_node_indices (%d): %s",
                          rr_area,
                          count,
                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
            }

        } else if (node_type != e_rr_type::OPIN && node_type != e_rr_type::IPIN) {
            if (count != rr_node.length() + 1) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Mismatch between RR node length (%d) and count within rr_node_indices (%d, should be length + 1): %s",
                          rr_node.length(),
                          count,
                          describe_rr_node(rr_graph, grid, rr_indexed_data, inode, is_flat).c_str());
            }
        }
    }
}
