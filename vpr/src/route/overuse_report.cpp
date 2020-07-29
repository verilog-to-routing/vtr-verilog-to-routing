#include "overuse_report.h"

#include <fstream>
#include "globals.h"
#include "vtr_log.h"

static void log_overused_nodes_header();
static void log_single_overused_node_status(int overuse_index, RRNodeId inode);

void log_overused_nodes_status() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();

    //Print overuse info header
    log_overused_nodes_header();

    //Print overuse info body
    int overuse_index = 0;
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        int overuse = route_ctx.rr_node_route_inf[inode].occ() - device_ctx.rr_nodes[inode].capacity();

        if (overuse > 0) {
            log_single_overused_node_status(overuse_index, RRNodeId(inode));
            overuse_index++;
        }
    }
}

void report_overused_nodes() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();

    //Generate overuse infor lookup table
    std::map<RRNodeId, std::set<ClusterNetId>> nodes_to_nets_lookup;
    generate_overused_nodes_to_congested_net_lookup(nodes_to_nets_lookup);

    //Open the report file
    std::ofstream os("report_overused_nodes.rpt");
    os << "Overused nodes information report on the final failed routing attempt" << '\n';

    for (const auto& lookup_pair : nodes_to_nets_lookup) {
        const RRNodeId inode = lookup_pair.first;
        const auto& congested_nets = lookup_pair.second;

        //Report Basic info
        os << '\n';
        os << "Overused RR node index #" << size_t(inode) << '\n';
        os << "Occupancy = " << route_ctx.rr_node_route_inf[size_t(inode)].occ() << '\n';
        os << "Capacity = " << device_ctx.rr_nodes.node_capacity(inode) << '\n';
        os << "Node type = " << device_ctx.rr_nodes.node_type_string(inode) << '\n';
        os << "PTC number = " << device_ctx.rr_nodes.node_ptc_num(inode) << '\n';
        os << "Xlow = " << device_ctx.rr_nodes.node_xlow(inode) << ", Ylow = " << device_ctx.rr_nodes.node_ylow(inode) << '\n';
        os << "Xhigh = " << device_ctx.rr_nodes.node_xhigh(inode) << ", Yhigh = " << device_ctx.rr_nodes.node_yhigh(inode) << '\n';

        //Report Selective info
        auto node_type = device_ctx.rr_nodes.node_type(inode);
        if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
            os << "Direction = " << device_ctx.rr_nodes.node_direction_string(inode) << '\n';
            os << "Resistance = " << device_ctx.rr_nodes.node_R(inode) << '\n';
            os << "Capacitance = " << device_ctx.rr_nodes.node_C(inode) << '\n';
        } else if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
            os << "Side = " << device_ctx.rr_nodes.node_side_string(inode) << '\n';
        }

        //Reported corresponding congested nets
        os << "Number of nets passing through this RR node = " << congested_nets.size() << '\n';
        os << "Net IDs:";
        for (auto net_id : congested_nets) {
            os << ' ' << size_t(net_id);
        }
        os << '\n';
    }

    os.close();
}

void generate_overused_nodes_to_congested_net_lookup(std::map<RRNodeId, std::set<ClusterNetId>>& nodes_to_nets_lookup) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    //Create overused nodes to congested nets look up by
    //traversing through the net trace backs linked lists
    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        for (t_trace* tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
            int inode = tptr->index;

            int overuse = route_ctx.rr_node_route_inf[inode].occ() - device_ctx.rr_nodes[inode].capacity();
            if (overuse > 0) {
                nodes_to_nets_lookup[RRNodeId(inode)].insert(net_id);
            }
        }
    }
}

static void log_overused_nodes_header() {
    VTR_LOG("Routing Failure Diagnostics: Printing Overused Nodes Information\n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- -------\n");
    VTR_LOG("   No.   Inode  Occupancy  Capacity  RR Node    Direction    Side     PTC    Xlow    Ylow   Xhigh   Yhigh\n");
    VTR_LOG("                                        type                          NUM                                \n");
    VTR_LOG("------ ------- ---------- --------- -------- ------------ ------- ------- ------- ------- ------- -------\n");
}

static void log_single_overused_node_status(int overuse_index, RRNodeId inode) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();

    //Determines if direction or side is available for printing
    auto node_type = device_ctx.rr_nodes.node_type(inode);

    //Overuse #
    VTR_LOG("%6d", overuse_index);

    //Inode
    VTR_LOG(" %7d", size_t(inode));

    //Occupancy
    VTR_LOG(" %10d", route_ctx.rr_node_route_inf[size_t(inode)].occ());

    //Capacity
    VTR_LOG(" %9d", device_ctx.rr_nodes.node_capacity(inode));

    //RR node type
    VTR_LOG(" %8s", device_ctx.rr_nodes.node_type_string(inode));

    //Direction
    if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
        VTR_LOG(" %12s", device_ctx.rr_nodes.node_direction_string(inode));
    } else {
        VTR_LOG(" %12s", "N/A");
    }

    //Side
    if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
        VTR_LOG(" %7s", device_ctx.rr_nodes.node_side_string(inode));
    } else {
        VTR_LOG(" %7s", "N/A");
    }

    //PTC number
    VTR_LOG(" %7d", device_ctx.rr_nodes.node_ptc_num(inode));

    //X_low
    VTR_LOG(" %7d", device_ctx.rr_nodes.node_xlow(inode));

    //Y_low
    VTR_LOG(" %7d", device_ctx.rr_nodes.node_ylow(inode));

    //X_high
    VTR_LOG(" %7d", device_ctx.rr_nodes.node_xhigh(inode));

    //Y_high
    VTR_LOG(" %7d", device_ctx.rr_nodes.node_yhigh(inode));

    VTR_LOG("\n");

    fflush(stdout);
}