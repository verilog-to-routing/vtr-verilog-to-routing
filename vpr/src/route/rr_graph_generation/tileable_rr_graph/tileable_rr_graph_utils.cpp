#include <algorithm>

#include "vtr_assert.h"
#include "vtr_log.h"

#include "tileable_rr_graph_utils.h"
#include "tileable_rr_graph_types.h"
#include "rr_graph_view.h"
#include "vtr_geometry.h"

vtr::Point<size_t> get_track_rr_node_start_coordinate(const RRGraphView& rr_graph,
                                                      const RRNodeId& track_rr_node) {
    // Make sure we have CHANX or CHANY
    VTR_ASSERT((e_rr_type::CHANX == rr_graph.node_type(track_rr_node))
               || (e_rr_type::CHANY == rr_graph.node_type(track_rr_node)));

    vtr::Point<size_t> start_coordinator(size_t(-1), size_t(-1));

    if (Direction::INC == rr_graph.node_direction(track_rr_node)) {
        start_coordinator.set(rr_graph.node_xlow(track_rr_node), rr_graph.node_ylow(track_rr_node));
    } else {
        VTR_ASSERT(Direction::DEC == rr_graph.node_direction(track_rr_node));
        start_coordinator.set(rr_graph.node_xhigh(track_rr_node), rr_graph.node_yhigh(track_rr_node));
    }

    return start_coordinator;
}

vtr::Point<size_t> get_track_rr_node_end_coordinate(const RRGraphView& rr_graph,
                                                    const RRNodeId& track_rr_node) {
    // Make sure we have CHANX or CHANY
    VTR_ASSERT((e_rr_type::CHANX == rr_graph.node_type(track_rr_node))
               || (e_rr_type::CHANY == rr_graph.node_type(track_rr_node)));

    vtr::Point<size_t> end_coordinator(size_t(-1), size_t(-1));

    if (Direction::INC == rr_graph.node_direction(track_rr_node)) {
        end_coordinator.set(rr_graph.node_xhigh(track_rr_node), rr_graph.node_yhigh(track_rr_node));
    } else {
        VTR_ASSERT(Direction::DEC == rr_graph.node_direction(track_rr_node));
        end_coordinator.set(rr_graph.node_xlow(track_rr_node), rr_graph.node_ylow(track_rr_node));
    }

    return end_coordinator;
}
