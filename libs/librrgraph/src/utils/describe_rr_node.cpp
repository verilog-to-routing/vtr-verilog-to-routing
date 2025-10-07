#include "describe_rr_node.h"

#include "rr_node.h"
#include "physical_types_util.h"
#include "vtr_util.h"

std::string describe_rr_node(const RRGraphView& rr_graph,
                             const DeviceGrid& grid,
                             const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                             RRNodeId inode,
                             bool is_flat) {

    const e_rr_type node_type = rr_graph.node_type(inode);
    std::string msg = vtr::string_fmt("RR node: %d", inode);
    if (node_type == e_rr_type::CHANX || node_type == e_rr_type::CHANY || node_type == e_rr_type::CHANZ) {
        RRIndexedDataId cost_index = rr_graph.node_cost_index(inode);

        int seg_index = rr_indexed_data[cost_index].seg_index;
        std::string rr_node_direction_string = rr_graph.node_direction_string(inode);

        if (seg_index < (int)rr_graph.num_rr_segments()) {
            msg += vtr::string_fmt(" track: %d longline: %d",
                                   rr_graph.node_track_num(inode),
                                   rr_graph.rr_segments(RRSegmentId(seg_index)).longline);
        } else {
            msg += vtr::string_fmt(" track: %d seg_type: ILLEGAL_SEG_INDEX %d",
                                   rr_graph.node_track_num(inode),
                                   seg_index);
        }
    } else if (node_type == e_rr_type::IPIN || node_type == e_rr_type::OPIN) {
        t_physical_tile_type_ptr type = grid.get_physical_type({rr_graph.node_xlow(inode),
                                                                rr_graph.node_ylow(inode),
                                                                rr_graph.node_layer_low(inode)});

        std::string pin_name = block_type_pin_index_to_name(type, rr_graph.node_pin_num(inode), is_flat);

        msg += vtr::string_fmt(" pin: %d pin_name: %s",
                               rr_graph.node_pin_num(inode),
                               pin_name.c_str());
    } else if (rr_graph.node_type(inode) == e_rr_type::MUX) {
        int index = rr_graph.node_ptc_num(inode);
        
        msg += vtr::string_fmt(" mux index: %d",
                               index);
    } else {
        VTR_ASSERT(node_type == e_rr_type::SOURCE || node_type == e_rr_type::SINK);

        msg += vtr::string_fmt(" class: %d", rr_graph.node_class_num(inode));
    }

    msg += vtr::string_fmt(" capacity: %d", rr_graph.node_capacity(inode));
    msg += vtr::string_fmt(" fan-in: %d", rr_graph.node_fan_in(inode));
    msg += vtr::string_fmt(" fan-out: %d", rr_graph.num_edges(inode));

    msg += " " + rr_graph.node_coordinate_to_string(inode);

    return msg;
}
