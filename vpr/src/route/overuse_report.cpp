#include "overuse_report.h"

#include <fstream>
#include "globals.h"
#include "vtr_log.h"

static void log_overused_nodes_header();
static void log_single_overused_node_status(int overuse_index, int inode);

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
            log_single_overused_node_status(overuse_index, inode);
            overuse_index++;
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

static void log_single_overused_node_status(int overuse_index, int inode) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();

    //Determines if direction or side is available for printing
    auto node_type = device_ctx.rr_nodes[inode].type();

    //Overuse #
    VTR_LOG("%6d", overuse_index);

    //Inode
    VTR_LOG(" %7d", inode);

    //Occupancy
    VTR_LOG(" %10d", route_ctx.rr_node_route_inf[inode].occ());

    //Capacity
    VTR_LOG(" %9d", device_ctx.rr_nodes[inode].capacity());

    //RR node type
    VTR_LOG(" %8s", device_ctx.rr_nodes[inode].type_string());

    //Direction
    if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
        VTR_LOG(" %12s", device_ctx.rr_nodes[inode].direction_string());
    } else {
        VTR_LOG(" %12s", "N/A");
    }

    //Side
    if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
        VTR_LOG(" %7s", device_ctx.rr_nodes[inode].side_string());
    } else {
        VTR_LOG(" %7s", "N/A");
    }

    //PTC number
    VTR_LOG(" %7d", device_ctx.rr_nodes[inode].ptc_num());

    //X_low
    VTR_LOG(" %7d", device_ctx.rr_nodes[inode].xlow());

    //Y_low
    VTR_LOG(" %7d", device_ctx.rr_nodes[inode].ylow());

    //X_high
    VTR_LOG(" %7d", device_ctx.rr_nodes[inode].xhigh());

    //Y_high
    VTR_LOG(" %7d", device_ctx.rr_nodes[inode].yhigh());

    VTR_LOG("\n");

    fflush(stdout);
}

void report_overused_nodes() {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    //Create overused nodes to congested nets loop up
    //by traversing through the net trackbacks
    std::unordered_map<size_t, std::unordered_set<ClusterNetId>> overused_nodes_to_net_lookup;
    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        for (t_trace* tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
            size_t inode = size_t(tptr->index);

            int overuse = route_ctx.rr_node_route_inf[inode].occ() - device_ctx.rr_nodes[inode].capacity();
            if (overuse > 0) {
                overused_nodes_to_net_lookup[inode].insert(net_id);
            }
        }
    }

    //Append to the current report file
    std::ofstream os("report_overused_nodes.rpt");
    os << "Overused nodes information report on the final failed routing attempt" << '\n';

    for (auto lookup_pair : overused_nodes_to_net_lookup) {
        size_t inode = lookup_pair.first;
        const auto& congested_nets = lookup_pair.second;

        //Report Basic info
        os << '\n';
        os << "Overused RR node index #" << inode << '\n';
        os << "Occupancy = " << route_ctx.rr_node_route_inf[inode].occ() << '\n';
        os << "Capacity = " << device_ctx.rr_nodes[inode].capacity() << '\n';
        os << "Node type = " << device_ctx.rr_nodes[inode].type_string() << '\n';
        os << "PTC number = " << device_ctx.rr_nodes[inode].ptc_num() << '\n';
        os << "Xlow = " << device_ctx.rr_nodes[inode].xlow() << ", Ylow = " << device_ctx.rr_nodes[inode].ylow() << '\n';
        os << "Xhigh = " << device_ctx.rr_nodes[inode].xhigh() << ", Yhigh = " << device_ctx.rr_nodes[inode].yhigh() << '\n';

        //Report Selective info
        auto node_type = device_ctx.rr_nodes[inode].type();
        if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
            os << "Direction = " << device_ctx.rr_nodes[inode].direction_string() << '\n';
            os << "Resistance = " << device_ctx.rr_nodes[inode].R() << '\n';
            os << "Capacitance = " << device_ctx.rr_nodes[inode].C() << '\n';
        } else if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
            os << "Side = " << device_ctx.rr_nodes[inode].side_string() << '\n';
        }

        //Reported corresponding congested nets
        os << "Number of nets passing through this RR node = " << congested_nets.size() << '\n';
        os << "Net IDs:" << '\n';
        for (auto net_id : congested_nets) {
            os << size_t(net_id) << '\n';
        }
    }

    os.close();
}