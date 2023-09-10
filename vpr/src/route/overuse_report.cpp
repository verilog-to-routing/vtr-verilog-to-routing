#include "overuse_report.h"

#include <fstream>
#include "vtr_log.h"

/**
 * @brief Definitions of global and helper routines related to printing RR node overuse info.
 *
 * All the global routines defined here are declared in the header file overuse_report.h
 * The helper routines that are called by the global routine should stay local to this file.
 * They provide subroutine hierarchy to allow easier customization of the logfile/report format.
 */
static void generate_node_to_net_lookup(const Netlist<>& net_list,
                                        std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map);
static void report_overused_ipin_opin(std::ostream& os,
                                      RRNodeId node_id,
                                      const std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map);
static void report_overused_chanx_chany(std::ostream& os, RRNodeId node_id);
static void report_overused_source_sink(std::ostream& os, RRNodeId node_id);
static void report_congested_nets(const Netlist<>& net_list,
                                  const AtomLookup& atom_lookup,
                                  std::ostream& os,
                                  const std::set<ParentNetId>& congested_nets,
                                  bool is_flat,
                                  int layer_num,
                                  int x,
                                  int y,
                                  bool report_sinks);

static void log_overused_nodes_header();
static void log_single_overused_node_status(int overuse_index, RRNodeId inode);
void print_block_pins_nets(std::ostream& os,
                           t_physical_tile_type_ptr physical_type,
                           int layer,
                           int root_x,
                           int root_y,
                           int pin_physical_num,
                           const std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map);
/**
 * @brief Print out RR node overuse info in the VPR logfile.
 *
 * Print out limited amount of overused node info in the vpr.out logfile.
 * The limit is specified by the VPR option --max_logged_overused_rr_nodes
 */
void log_overused_nodes_status(int max_logged_overused_rr_nodes) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();

    //Print overuse info header
    log_overused_nodes_header();

    //Print overuse info body
    int overuse_index = 0;
    for (RRNodeId inode : rr_graph.nodes()) {
        int overuse = route_ctx.rr_node_route_inf[inode].occ() - rr_graph.node_capacity(inode);

        if (overuse > 0) {
            log_single_overused_node_status(overuse_index, inode);
            ++overuse_index;

            //Reached the logging limit
            if (overuse_index >= max_logged_overused_rr_nodes) {
                return;
            }
        }
    }
}

/**
 * @brief Print out RR node overuse info in a post-VPR report file.
 *
 * Print all the overused RR nodes' info in the report file report_overused_nodes.rpt.
 * The report generation is turned on by the VPR option: --generate_rr_node_overuse_report on.
 * This report will be generated only if the last routing attempt fails, which
 * causes the whole VPR flow to fail.
 */
void report_overused_nodes(const Netlist<>& net_list,
                           const RRGraphView& rr_graph,
                           bool is_flat) {
    const auto& route_ctx = g_vpr_ctx.routing();

    /* Generate overuse info lookup table */
    std::map<RRNodeId, std::set<ParentNetId>> over_used_nodes_to_nets_lookup;
    std::map<RRNodeId, std::set<ParentNetId>> rr_node_to_net_map;
    generate_overused_nodes_to_congested_net_lookup(net_list,
                                                    over_used_nodes_to_nets_lookup);
    generate_node_to_net_lookup(net_list, rr_node_to_net_map);

    /* Open the report file and print header info */
    std::ofstream os("report_overused_nodes.rpt");
    os << "Overused nodes information report on the final failed routing attempt" << '\n';
    os << "Total number of overused nodes = " << over_used_nodes_to_nets_lookup.size() << '\n';

    /* Go through each rr node and the nets that pass through it */
    size_t inode = 0;
    for (const auto& lookup_pair : over_used_nodes_to_nets_lookup) {
        const RRNodeId node_id = lookup_pair.first;
        const auto& congested_nets = lookup_pair.second;

        os << "************************************************\n\n"; //Separation line

        /* Report basic rr node info */
        os << "Overused RR node #" << inode << '\n';
        os << "Node id = " << size_t(node_id) << '\n';
        os << "Occupancy = " << route_ctx.rr_node_route_inf[node_id].occ() << '\n';
        os << "Capacity = " << rr_graph.node_capacity(node_id) << "\n\n";

        /* Report selective info based on the rr node type */
        auto node_type = rr_graph.node_type(node_id);
        os << "Node type = " << rr_graph.node_type_string(node_id) << '\n';
        bool report_sinks = false;
        int x = rr_graph.node_xlow(node_id);
        int y = rr_graph.node_ylow(node_id);
        int layer_num = rr_graph.node_layer(node_id);
        switch (node_type) {
            case IPIN:
            case OPIN:
                report_overused_ipin_opin(os,
                                          node_id,
                                          rr_node_to_net_map);
                report_sinks = true;
                x -= g_vpr_ctx.device().grid.get_physical_type({x, y, layer_num})->width;
                y -= g_vpr_ctx.device().grid.get_physical_type({x, y, layer_num})->width;
                break;
            case CHANX:
            case CHANY:
                report_overused_chanx_chany(os, node_id);
                break;
            case SOURCE:
            case SINK:
                report_overused_source_sink(os, node_id);
                report_sinks = true;
                break;

            default:
                break;
        }

        /* Finished printing the node info. Now print out the  *
         * info on the nets passing through this overused node */
        os << "-----------------------------\n"; //Separation line
        report_congested_nets(net_list,
                              g_vpr_ctx.atom().lookup,
                              os,
                              congested_nets,
                              is_flat,
                              layer_num,
                              x,
                              y,
                              report_sinks);

        ++inode;
    }

    os.close();
}

/**
 * @brief Generate a overused RR nodes to congested nets lookup table.
 *
 * Uses map data structure to store a lookup table that matches RR nodes
 * to the nets that pass through them. Only overused nodes and congested
 * nets will be recorded.
 *
 * This routine goes through the trace back linked list of each net.
 */
void generate_overused_nodes_to_congested_net_lookup(const Netlist<>& net_list,
                                                     std::map<RRNodeId, std::set<ParentNetId>>& nodes_to_nets_lookup) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();

    //Create overused nodes to congested nets look up by
    //traversing through the net trace backs linked lists
    for (ParentNetId net_id : net_list.nets()) {
        if (!route_ctx.route_trees[net_id])
            continue;

        for (auto& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            int overuse = route_ctx.rr_node_route_inf[inode].occ() - rr_graph.node_capacity(inode);
            if (overuse > 0) {
                nodes_to_nets_lookup[inode].insert(net_id);
            }
        }
    }
}

static void generate_node_to_net_lookup(const Netlist<>& net_list,
                                        std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map) {
    const auto& route_ctx = g_vpr_ctx.routing();

    //Create overused nodes to congested nets look up by
    //traversing through the net trace backs linked lists
    for (ParentNetId net_id : net_list.nets()) {
        if (!route_ctx.route_trees[net_id])
            continue;

        for (const RouteTreeNode& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            rr_node_to_net_map[rt_node.inode].insert(net_id);
        }
    }
}

///@brief Print out information specific to IPIN/OPIN type rr nodes
static void report_overused_ipin_opin(std::ostream& os,
                                      RRNodeId node_id,
                                      const std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& place_ctx = g_vpr_ctx.placement();

    auto grid_x = rr_graph.node_xlow(node_id);
    auto grid_y = rr_graph.node_ylow(node_id);
    auto grid_layer = rr_graph.node_layer(node_id);

    VTR_ASSERT_MSG(
        grid_x == rr_graph.node_xhigh(node_id) && grid_y == rr_graph.node_yhigh(node_id),
        "Non-track RR node should not span across multiple grid blocks.");

    t_physical_tile_type_ptr physical_tile = device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer});

    os << "Pin physical number = " << rr_graph.node_pin_num(node_id) << '\n';
    if (is_inter_cluster_node(physical_tile, rr_graph.node_type(node_id), rr_graph.node_ptc_num(node_id))) {
        os << "On Tile Pin"
           << "\n";
    } else {
        auto pb_type_name = get_pb_graph_node_from_pin_physical_num(device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer}),
                                                                    rr_graph.node_ptc_num(node_id))
                                ->pb_type->name;
        auto pb_pin = get_pb_pin_from_pin_physical_num(device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer}),
                                                       rr_graph.node_ptc_num(node_id));
        os << "Intra-Tile Pin - Port : " << pb_pin->port->name << " - PB Type : " << std::string(pb_type_name) << "\n";
    }
    print_block_pins_nets(os,
                          device_ctx.grid.get_physical_type({grid_x, grid_y, grid_layer}),
                          grid_layer,
                          grid_x - device_ctx.grid.get_width_offset({grid_x, grid_y, grid_layer}),
                          grid_y - device_ctx.grid.get_height_offset({grid_x, grid_y, grid_layer}),
                          rr_graph.node_ptc_num(node_id),
                          rr_node_to_net_map);
    os << "Side = " << rr_graph.node_side_string(node_id) << "\n\n";

    //Add block type for IPINs/OPINs in overused rr-node report
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const auto& grid_info = place_ctx.grid_blocks;

    os << "Grid location: X = " << grid_x << ", Y = " << grid_y << '\n';
    os << "Number of blocks currently occupying this grid location = " << grid_info.get_usage({grid_x, grid_y, grid_layer}) << '\n';

    size_t iblock = 0;
    for (int isubtile = 0; isubtile < (int)grid_info.num_blocks_at_location({grid_x, grid_y, grid_layer}); ++isubtile) {
        //Check if there is a valid block at this subtile location
        if (grid_info.is_sub_tile_empty({grid_x, grid_y, grid_layer}, isubtile)) {
            continue;
        }

        //Print out the block index, name and type
        // TODO: Needs to be updated when RR Graph Nodes know their layer_num
        ClusterBlockId block_id = grid_info.block_at_location({grid_x, grid_y, isubtile, 0});
        os << "Block #" << iblock << ": ";
        os << "Block name = " << clb_nlist.block_pb(block_id)->name << ", ";
        os << "Block type = " << clb_nlist.block_type(block_id)->name << '\n';
        ++iblock;
    }
}

///@brief Print out information specific to CHANX/CHANY type rr nodes
static void report_overused_chanx_chany(std::ostream& os, RRNodeId node_id) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    os << "Track number = " << rr_graph.node_track_num(node_id) << '\n';
    //Print out associated RC characteristics as they will be non-zero
    os << "Resistance = " << rr_graph.node_R(node_id) << '\n';
    os << "Capacitance = " << rr_graph.node_C(node_id) << '\n';

    // print out type, side, x_low, x_high, y_low, y_high, length, direction
    os << rr_graph.node_coordinate_to_string(node_id) << '\n';
}

///@brief Print out information specific to SOURCE/SINK type rr nodes
static void report_overused_source_sink(std::ostream& os, RRNodeId node_id) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    auto grid_x = rr_graph.node_xlow(node_id);
    auto grid_y = rr_graph.node_ylow(node_id);
    VTR_ASSERT_MSG(
        grid_x == rr_graph.node_xhigh(node_id) && grid_y == rr_graph.node_yhigh(node_id),
        "Non-track RR node should not span across multiple grid blocks.");

    os << "Class number = " << rr_graph.node_class_num(node_id) << '\n';
    os << "Grid location: X = " << grid_x << ", Y = " << grid_y << '\n';
}

/**
 * @brief Print out info on congested nets in the router.
 *
 * Report information on the congested nets that pass through a specific rr node. *
 * These nets are congested because the number of nets currently passing through  *
 * this rr node exceed the node's routing net capacity.
 */
static void report_congested_nets(const Netlist<>& net_list,
                                  const AtomLookup& atom_lookup,
                                  std::ostream& os,
                                  const std::set<ParentNetId>& congested_nets,
                                  bool is_flat,
                                  int layer_num,
                                  int x,
                                  int y,
                                  bool report_sinks) {
    os << "Number of nets passing through this RR node = " << congested_nets.size() << '\n';

    size_t inet = 0;
    for (ParentNetId net_id : congested_nets) {
        ParentBlockId block_id = net_list.net_driver_block(net_id);
        os << "Net #" << inet << ": ";
        os << "Net ID = " << size_t(net_id) << ", ";
        os << "Net name = " << net_list.net_name(net_id) << ", ";
        if (is_flat) {
            AtomBlockId atom_blk_id = convert_to_atom_block_id(block_id);
            os << "Driving block name = " << atom_lookup.atom_pb(atom_blk_id)->name << ", ";
            os << "Driving block type = " << g_vpr_ctx.clustering().clb_nlist.block_type(atom_lookup.atom_clb(atom_blk_id))->name << '\n';
        } else {
            ClusterBlockId clb_blk_id = convert_to_cluster_block_id(block_id);
            os << "Driving block name = " << g_vpr_ctx.clustering().clb_nlist.block_pb(clb_blk_id)->name << ", ";
            os << "Driving block type = " << g_vpr_ctx.clustering().clb_nlist.block_type(clb_blk_id)->name << '\n';
        }

        if (report_sinks) {
            for (auto sink_id : net_list.net_sinks(net_id)) {
                ClusterBlockId cluster_block_id;
                if (is_flat) {
                    AtomBlockId atom_blk_id = convert_to_atom_block_id(net_list.pin_block(sink_id));
                    cluster_block_id = atom_lookup.atom_clb(convert_to_atom_block_id(atom_blk_id));
                } else {
                    cluster_block_id = convert_to_cluster_block_id(net_list.pin_block(sink_id));
                }
                auto cluster_loc = g_vpr_ctx.placement().block_locs[cluster_block_id];
                auto physical_type = g_vpr_ctx.device().grid.get_physical_type({x, y, layer_num});
                int cluster_layer_num = cluster_loc.loc.layer;
                int cluster_x = cluster_loc.loc.x - g_vpr_ctx.device().grid.get_physical_type({cluster_loc.loc.x, cluster_loc.loc.y, cluster_layer_num})->width;
                int cluster_y = cluster_loc.loc.y - g_vpr_ctx.device().grid.get_physical_type({cluster_loc.loc.x, cluster_loc.loc.y, cluster_layer_num})->height;
                if (cluster_x == x && cluster_y == y) {
                    VTR_ASSERT(physical_type == g_vpr_ctx.device().grid.get_physical_type({cluster_x, cluster_y, cluster_layer_num}));
                    os << "Sink in the same location = "
                       << "\n";
                    if (is_flat) {
                        auto pb_pin = atom_lookup.atom_pin_pb_graph_pin(convert_to_atom_pin_id(sink_id));
                        auto pb_net_list = atom_lookup.atom_pb(convert_to_atom_block_id(net_list.pin_block(sink_id)));
                        os << "  "
                           << "Pin Logical Num: " << pb_pin->pin_count_in_cluster << "  PB Type: " << pb_pin->parent_node->pb_type->name << " Netlist PB: " << pb_net_list->name << " Parent PB Type: " << pb_net_list->parent_pb->pb_graph_node->pb_type->name << "Parent Netlist PB : " << pb_net_list->parent_pb->name << "\n";
                        os << "  "
                           << "Hierarchical Type Name : " << pb_pin->parent_node->hierarchical_type_name() << "\n";
                    } else {
                        os << "  " << g_vpr_ctx.placement().physical_pins[convert_to_cluster_pin_id(sink_id)] << "\n";
                    }
                }
            }
        }
        ++inet;
    }
    os << '\n';
}

///@brief Print out a single-line info that corresponds to a single overused rr node in the logfile
static void log_overused_nodes_header() {
    VTR_LOG("Routing Failure Diagnostics: Printing Overused Nodes Information\n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- ------- -------\n");
    VTR_LOG("   No.  NodeId  Occupancy  Capacity  RR Node    Direction    Side     PTC   Block    Xlow    Ylow   Xhigh   Yhigh\n");
    VTR_LOG("                                        type                          NUM    Name                                 \n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- ------- -------\n");
}

///@brief Print out a single-line info that corresponds to a single overused rr node in the logfile
static void log_single_overused_node_status(int overuse_index, RRNodeId node_id) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();
    int x = rr_graph.node_xlow(node_id);
    int y = rr_graph.node_ylow(node_id);
    int layer_num = rr_graph.node_layer(node_id);
    auto physical_blk = device_ctx.grid.get_physical_type({x, y, layer_num});

    //Determines if direction or side is available for printing
    auto node_type = rr_graph.node_type(node_id);

    //Overuse #
    VTR_LOG("%6d", overuse_index);

    //Inode
    VTR_LOG(" %7d", size_t(node_id));

    //Occupancy
    VTR_LOG(" %10d", route_ctx.rr_node_route_inf[node_id].occ());

    //Capacity
    VTR_LOG(" %9d", rr_graph.node_capacity(node_id));

    //RR node type
    VTR_LOG(" %8s", rr_graph.node_type_string(node_id));

    //Direction
    if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
        VTR_LOG(" %12s", rr_graph.node_direction_string(node_id).c_str());
    } else {
        VTR_LOG(" %12s", "N/A");
    }

    //Side
    if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
        VTR_LOG(" %7s", rr_graph.node_side_string(node_id));
    } else {
        VTR_LOG(" %7s", "N/A");
    }

    //PTC number
    VTR_LOG(" %7d", rr_graph.node_ptc_num(node_id));

    // Block Name
    VTR_LOG(" %7s", physical_blk->name);

    //X_low
    VTR_LOG(" %7d", x);

    //Y_low
    VTR_LOG(" %7d", y);

    //X_high
    VTR_LOG(" %7d", rr_graph.node_xhigh(node_id));

    //Y_high
    VTR_LOG(" %7d", rr_graph.node_yhigh(node_id));

    VTR_LOG("\n");

    fflush(stdout);
}

void print_block_pins_nets(std::ostream& os,
                           t_physical_tile_type_ptr physical_type,
                           int layer,
                           int root_x,
                           int root_y,
                           int pin_physical_num,
                           const std::map<RRNodeId, std::set<ParentNetId>>& rr_node_to_net_map) {
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;

    t_pin_range pin_num_range;
    if (is_pin_on_tile(physical_type, pin_physical_num)) {
        pin_num_range.low = 0;
        pin_num_range.high = physical_type->num_pins - 1;
    } else {
        const t_sub_tile* sub_tile = nullptr;
        int rel_cap = -1;
        std::tie(sub_tile, rel_cap) = get_sub_tile_from_pin_physical_num(physical_type, pin_physical_num);
        VTR_ASSERT(rel_cap != -1);
        auto logical_block = get_logical_block_from_pin_physical_num(physical_type, pin_physical_num);
        VTR_ASSERT(logical_block != nullptr);
        auto pb_graph_node = get_pb_pin_from_pin_physical_num(physical_type, pin_physical_num);
        VTR_ASSERT(pb_graph_node != nullptr);
        pin_num_range = get_pb_graph_node_pins(physical_type,
                                               sub_tile,
                                               logical_block,
                                               rel_cap,
                                               pb_graph_node->parent_node);
    }

    for (int pin = pin_num_range.low; pin <= pin_num_range.high; pin++) {
        t_rr_type rr_type = (get_pin_type_from_pin_physical_num(physical_type, pin) == DRIVER) ? t_rr_type::OPIN : t_rr_type::IPIN;
        RRNodeId node_id = get_pin_rr_node_id(rr_graph.node_lookup(), physical_type, layer, root_x, root_y, pin);
        VTR_ASSERT(node_id != RRNodeId::INVALID());
        auto search_result = rr_node_to_net_map.find(node_id);
        if (rr_type == t_rr_type::OPIN) {
            os << "  OPIN - ";
        } else {
            VTR_ASSERT(rr_type == t_rr_type::IPIN);
            os << "  IPIN - ";
        }
        os << "RRNodeId: " << size_t(node_id) << " - Physical Num: " << pin << "\n";
        os << "  ";
        if (search_result != rr_node_to_net_map.end()) {
            auto nets = search_result->second;
            for (auto net : nets) {
                os << "  " << size_t(net);
            }
        } else {
            os << "  -1";
        }
        os << "\n";
    }
}
