#include "overuse_report.h"

#include <fstream>
#include "globals.h"
#include "vtr_log.h"

/**
 * @brief Definitions of global and helper routines related to printing RR node overuse info.
 *
 * All the global routines defined here are declared in the header file overuse_report.h
 * The helper routines that are called by the global routine should stay local to this file.
 * They provide subroutine hierarchy to allow easier customization of the logfile/report format.
 */

static void report_overused_ipin_opin(std::ostream& os, RRNodeId node_id);
static void report_overused_chanx_chany(std::ostream& os, RRNodeId node_id);
static void report_overused_source_sink(std::ostream& os, RRNodeId node_id);
static void report_congested_nets(std::ostream& os, const std::set<ClusterNetId>& congested_nets);

static void log_overused_nodes_header();
static void log_single_overused_node_status(int overuse_index, RRNodeId inode);

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
    for (const RRNodeId& rr_id : rr_graph.nodes()) {
        int overuse = route_ctx.rr_node_route_inf[(size_t)rr_id].occ() - rr_graph.node_capacity(rr_id);

        if (overuse > 0) {
            log_single_overused_node_status(overuse_index, rr_id);
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
void report_overused_nodes(const RRGraphView& rr_graph) {
    const auto& route_ctx = g_vpr_ctx.routing();

    /* Generate overuse info lookup table */
    std::map<RRNodeId, std::set<ClusterNetId>> nodes_to_nets_lookup;
    generate_overused_nodes_to_congested_net_lookup(nodes_to_nets_lookup);

    /* Open the report file and print header info */
    std::ofstream os("report_overused_nodes.rpt");
    os << "Overused nodes information report on the final failed routing attempt" << '\n';
    os << "Total number of overused nodes = " << nodes_to_nets_lookup.size() << '\n';

    /* Go through each rr node and the nets that pass through it */
    size_t inode = 0;
    for (const auto& lookup_pair : nodes_to_nets_lookup) {
        const RRNodeId node_id = lookup_pair.first;
        const auto& congested_nets = lookup_pair.second;

        os << "************************************************\n\n"; //Separation line

        /* Report basic rr node info */
        os << "Overused RR node #" << inode << '\n';
        os << "Node id = " << size_t(node_id) << '\n';
        os << "Occupancy = " << route_ctx.rr_node_route_inf[size_t(node_id)].occ() << '\n';
        os << "Capacity = " << rr_graph.node_capacity(node_id) << "\n\n";

        /* Report selective info based on the rr node type */
        auto node_type = rr_graph.node_type(node_id);
        os << "Node type = " << rr_graph.node_type_string(node_id) << '\n';

        switch (node_type) {
            case IPIN:
            case OPIN:
                report_overused_ipin_opin(os, node_id);
                break;
            case CHANX:
            case CHANY:
                report_overused_chanx_chany(os, node_id);
                break;
            case SOURCE:
            case SINK:
                report_overused_source_sink(os, node_id);
                break;

            default:
                break;
        }

        /* Finished printing the node info. Now print out the  *
         * info on the nets passing through this overused node */
        os << "-----------------------------\n"; //Separation line
        report_congested_nets(os, congested_nets);

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
void generate_overused_nodes_to_congested_net_lookup(std::map<RRNodeId, std::set<ClusterNetId>>& nodes_to_nets_lookup) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    //Create overused nodes to congested nets look up by
    //traversing through the net trace backs linked lists
    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        for (t_trace* tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
            int inode = tptr->index;

            int overuse = route_ctx.rr_node_route_inf[inode].occ() - rr_graph.node_capacity(RRNodeId(inode));
            if (overuse > 0) {
                nodes_to_nets_lookup[RRNodeId(inode)].insert(net_id);
            }
        }
    }
}

///@brief Print out information specific to IPIN/OPIN type rr nodes
static void report_overused_ipin_opin(std::ostream& os, RRNodeId node_id) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& place_ctx = g_vpr_ctx.placement();

    auto grid_x = rr_graph.node_xlow(node_id);
    auto grid_y = rr_graph.node_ylow(node_id);
    VTR_ASSERT_MSG(
        grid_x == rr_graph.node_xhigh(node_id) && grid_y == rr_graph.node_yhigh(node_id),
        "Non-track RR node should not span across multiple grid blocks.");

    os << "Pin number = " << rr_graph.node_pin_num(node_id) << '\n';
    os << "Side = " << rr_graph.node_side_string(node_id) << "\n\n";

    //Add block type for IPINs/OPINs in overused rr-node report
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    auto& grid_info = place_ctx.grid_blocks[grid_x][grid_y];

    os << "Grid location: X = " << grid_x << ", Y = " << grid_y << '\n';
    os << "Number of blocks currently occupying this grid location = " << grid_info.usage << '\n';

    size_t iblock = 0;
    for (size_t isubtile = 0; isubtile < grid_info.blocks.size(); ++isubtile) {
        //Check if there is a valid block at this subtile location
        if (grid_info.subtile_empty(isubtile)) {
            continue;
        }

        //Print out the block index, name and type
        ClusterBlockId block_id = grid_info.blocks[isubtile];
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
static void report_congested_nets(std::ostream& os, const std::set<ClusterNetId>& congested_nets) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    os << "Number of nets passing through this RR node = " << congested_nets.size() << '\n';

    size_t inet = 0;
    for (ClusterNetId net_id : congested_nets) {
        ClusterBlockId block_id = clb_nlist.net_driver_block(net_id);
        os << "Net #" << inet << ": ";
        os << "Net ID = " << size_t(net_id) << ", ";
        os << "Net name = " << clb_nlist.net_name(net_id) << ", ";
        os << "Driving block name = " << clb_nlist.block_pb(block_id)->name << ", ";
        os << "Driving block type = " << clb_nlist.block_type(block_id)->name << '\n';
        ++inet;
    }
    os << '\n';
}

///@brief Print out the header of the overused rr node info in the logfile
static void log_overused_nodes_header() {
    VTR_LOG("Routing Failure Diagnostics: Printing Overused Nodes Information\n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- -------\n");
    VTR_LOG("   No.  NodeId  Occupancy  Capacity  RR Node    Direction    Side     PTC    Xlow    Ylow   Xhigh   Yhigh\n");
    VTR_LOG("                                        type                          NUM                                \n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- -------\n");
}

///@brief Print out a single-line info that corresponds to a single overused rr node in the logfile
static void log_single_overused_node_status(int overuse_index, RRNodeId node_id) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();

    //Determines if direction or side is available for printing
    auto node_type = rr_graph.node_type(node_id);

    //Overuse #
    VTR_LOG("%6d", overuse_index);

    //Inode
    VTR_LOG(" %7d", size_t(node_id));

    //Occupancy
    VTR_LOG(" %10d", route_ctx.rr_node_route_inf[size_t(node_id)].occ());

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

    //X_low
    VTR_LOG(" %7d", rr_graph.node_xlow(node_id));

    //Y_low
    VTR_LOG(" %7d", rr_graph.node_ylow(node_id));

    //X_high
    VTR_LOG(" %7d", rr_graph.node_xhigh(node_id));

    //Y_high
    VTR_LOG(" %7d", rr_graph.node_yhigh(node_id));

    VTR_LOG("\n");

    fflush(stdout);
}
