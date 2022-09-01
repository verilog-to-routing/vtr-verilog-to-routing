#include "describe_rr_node.h"

#include "rr_node.h"
#include "physical_types_util.h"
#include "vtr_util.h"

/* TODO: This function should adapt RRNodeId */
std::string describe_rr_node(const RRGraphView& rr_graph,
                             const DeviceGrid& grid,
                             const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                             int inode,
                             bool is_flat) {

    std::string msg = vtr::string_fmt("RR node: %d", inode);

    if (rr_graph.node_type(RRNodeId(inode)) == CHANX || rr_graph.node_type(RRNodeId(inode)) == CHANY) {
        auto cost_index = rr_graph.node_cost_index(RRNodeId(inode));

        int seg_index = rr_indexed_data[cost_index].seg_index;
        std::string rr_node_direction_string = rr_graph.node_direction_string(RRNodeId(inode));

        if (seg_index < (int)rr_graph.num_rr_segments()) {
            msg += vtr::string_fmt(" track: %d longline: %d",
                                   rr_graph.node_track_num(RRNodeId(inode)),
                                   rr_graph.rr_segments(RRSegmentId(seg_index)).longline);
        } else {
            msg += vtr::string_fmt(" track: %d seg_type: ILLEGAL_SEG_INDEX %d",
                                   rr_graph.node_track_num(RRNodeId(inode)),
                                   seg_index);
        }
    } else if (rr_graph.node_type(RRNodeId(inode)) == IPIN || rr_graph.node_type(RRNodeId(inode)) == OPIN) {
        auto type = grid[rr_graph.node_xlow(RRNodeId(inode))][rr_graph.node_ylow(RRNodeId(inode))].type;
        std::string pin_name = block_type_pin_index_to_name(type, rr_graph.node_pin_num(RRNodeId(inode)), is_flat);

        msg += vtr::string_fmt(" pin: %d pin_name: %s",
                               rr_graph.node_pin_num(RRNodeId(inode)),
                               pin_name.c_str());
    } else {
        VTR_ASSERT(rr_graph.node_type(RRNodeId(inode)) == SOURCE || rr_graph.node_type(RRNodeId(inode)) == SINK);

        msg += vtr::string_fmt(" class: %d", rr_graph.node_class_num(RRNodeId(inode)));
    }

    msg += vtr::string_fmt(" capacity: %d", rr_graph.node_capacity(RRNodeId(inode)));
    msg += vtr::string_fmt(" fan-in: %d", rr_graph.node_fan_in(RRNodeId(inode)));
    msg += vtr::string_fmt(" fan-out: %d", rr_graph.num_edges(RRNodeId(inode)));

    msg += " " + rr_graph.node_coordinate_to_string(RRNodeId(inode));

    return msg;
}