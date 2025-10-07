
#include "segment_stats.h"

#include "vtr_log.h"
#include "globals.h"
#include "route_common.h"

/*************** Variables and defines local to this module ****************/

static constexpr int LONGLINE = 0;

/******************* Subroutine definitions ********************************/

void get_segment_usage_stats(std::vector<t_segment_inf>& segment_inf) {
    /* Computes statistics on the fractional utilization of segments by type    *
     * (index) and by length.  This routine needs a valid rr_graph, and a       *
     * completed routing.  Note that segments cut off by the end of the array   *
     * are counted as full-length segments (e.g. length 4 even if the last 2    *
     * units of wire were chopped off by the chip edge).                        */

    RRIndexedDataId cost_index;
    float utilization;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    int max_segment_name_length = 0;
    std::map<e_parallel_axis, std::map<int, int>> directed_occ_by_length = {
        {e_parallel_axis::X_AXIS, std::map<int, int>()},
        {e_parallel_axis::Y_AXIS, std::map<int, int>()}};

    std::map<e_parallel_axis, std::map<int, int>> directed_cap_by_length = {
        {e_parallel_axis::X_AXIS, std::map<int, int>()},
        {e_parallel_axis::Y_AXIS, std::map<int, int>()}};

    std::set<int, std::less<>> segment_lengths;
    for (const t_segment_inf& seg_inf : segment_inf) {
        int seg_length = seg_inf.longline ? LONGLINE : seg_inf.length;

        for (e_parallel_axis ax : {e_parallel_axis::X_AXIS, e_parallel_axis::Y_AXIS}) {
            directed_cap_by_length[ax].insert({seg_length, 0});
            directed_occ_by_length[ax].insert({seg_length, 0});
        }

        segment_lengths.insert(seg_length);

        max_segment_name_length = std::max(max_segment_name_length, static_cast<int>(seg_inf.name.size()));
    }

    std::map<e_parallel_axis, std::vector<int>> directed_occ_by_type = {
        {e_parallel_axis::X_AXIS, std::vector<int>(segment_inf.size(), 0)},
        {e_parallel_axis::Y_AXIS, std::vector<int>(segment_inf.size(), 0)}};

    std::map<e_parallel_axis, std::vector<int>> directed_cap_by_type = {
        {e_parallel_axis::X_AXIS, std::vector<int>(segment_inf.size(), 0)},
        {e_parallel_axis::Y_AXIS, std::vector<int>(segment_inf.size(), 0)}};

    for (RRNodeId inode : device_ctx.rr_graph.nodes()) {
        e_rr_type node_type = rr_graph.node_type(inode);
        if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY) {
            cost_index = rr_graph.node_cost_index(inode);
            size_t seg_type = device_ctx.rr_indexed_data[cost_index].seg_index;
            int length = segment_inf[seg_type].longline ? LONGLINE : segment_inf[seg_type].length;

            const short& inode_capacity = rr_graph.node_capacity(inode);
            int occ = route_ctx.rr_node_route_inf[inode].occ();
            e_parallel_axis ax = (node_type == e_rr_type::CHANX) ? e_parallel_axis::X_AXIS : e_parallel_axis::Y_AXIS;

            directed_occ_by_length[ax][length] += occ;
            directed_cap_by_length[ax][length] += inode_capacity;

            directed_occ_by_type[ax][seg_type] += occ;
            directed_cap_by_type[ax][seg_type] += inode_capacity;
        }
    }

    VTR_LOG("\n");
    VTR_LOG("Total Number of Wiring Segments by Direction: direction length number\n");
    VTR_LOG("                                              --------- ------ -------\n");
    for (int length : segment_lengths) {
        for (e_parallel_axis ax : {e_parallel_axis::X_AXIS, e_parallel_axis::Y_AXIS}) {
            std::string ax_name = (ax == e_parallel_axis::X_AXIS) ? "X" : "Y";
            if (directed_cap_by_length[ax][length] != 0) {
                std::string length_str = (length == LONGLINE) ? "longline" : std::to_string(length);
                VTR_LOG("                                              %s%s %s%s %6d\n",
                        std::string(std::max(9 - (int)ax_name.length(), 0), ' ').c_str(),
                        ax_name.c_str(),
                        std::string(std::max(6 - (int)length_str.length(), 0), ' ').c_str(),
                        length_str.c_str(),
                        directed_cap_by_length[ax][length]);
            }
        }
    }

    for (e_parallel_axis ax : {e_parallel_axis::X_AXIS, e_parallel_axis::Y_AXIS}) {
        std::string ax_name = (ax == e_parallel_axis::X_AXIS) ? "X" : "Y";
        VTR_LOG("\n");
        VTR_LOG("%s - Directed Wiring Segment usage by length: length utilization\n", ax_name.c_str());
        VTR_LOG("                                             ------ -----------\n");
        for (int length : segment_lengths) {
            if (directed_cap_by_length[ax][length] != 0) {
                std::string length_str = (length == LONGLINE) ? "longline" : std::to_string(length);
                utilization = (float)directed_occ_by_length[ax][length] / (float)directed_cap_by_length[ax][length];
                VTR_LOG("                                       %s%s %11.3g\n",
                        std::string(std::max(7 - (int)length_str.length(), 0), ' ').c_str(),
                        length_str.c_str(),
                        utilization);
            }
        }
    }

    VTR_LOG("\n");
    VTR_LOG("Segment occupancy by length: Length Occupancy Capacity Utilization\n");
    VTR_LOG("                             ------ --------- -------- -----------\n");
    for (const int seg_length : segment_lengths) {
        if (directed_cap_by_length[e_parallel_axis::X_AXIS][seg_length] != 0 || directed_cap_by_length[e_parallel_axis::Y_AXIS][seg_length] != 0) {
            std::string seg_name = "L" + std::to_string(seg_length);

            int occ = 0;
            int cap = 0;
            for (e_parallel_axis ax : {e_parallel_axis::X_AXIS, e_parallel_axis::Y_AXIS}) {
                occ += directed_occ_by_length[ax][seg_length];
                cap += directed_cap_by_length[ax][seg_length];
            }
            utilization = (float)occ / (float)cap;
            VTR_LOG("                             %-6s %9d %8d %11.3g\n", seg_name.c_str(), occ, cap, utilization);
        }
    }

    VTR_LOG("\n");
    VTR_LOG("Segment occupancy by type: %sname type utilization\n", std::string(std::max(max_segment_name_length - 4, 0), ' ').c_str());
    VTR_LOG("                           %s ---- -----------\n", std::string(std::max(4, max_segment_name_length), '-').c_str());

    for (size_t seg_type = 0; seg_type < segment_inf.size(); seg_type++) {
        if (directed_cap_by_type[e_parallel_axis::X_AXIS][seg_type] != 0 || directed_cap_by_type[e_parallel_axis::Y_AXIS][seg_type] != 0) {
            std::string seg_name = segment_inf[seg_type].name;
            int seg_name_size = static_cast<int>(seg_name.size());
            int occ = 0;
            int cap = 0;
            for (e_parallel_axis ax : {e_parallel_axis::X_AXIS, e_parallel_axis::Y_AXIS}) {
                occ += directed_occ_by_type[ax][seg_type];
                cap += directed_cap_by_type[ax][seg_type];
            }
            utilization = (float)occ / (float)cap;
            VTR_LOG("                           %s%s %4d %11.3g\n", std::string(std::max(4 - seg_name_size, (max_segment_name_length - seg_name_size)), ' ').c_str(), seg_name.c_str(), seg_type, utilization);
        }
    }
}
