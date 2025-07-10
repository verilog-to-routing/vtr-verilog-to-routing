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

std::vector<RRSwitchId> get_rr_graph_driver_switches(const RRGraphView& rr_graph,
                                                     const RRNodeId node) {
    std::vector<RRSwitchId> driver_switches;

    for (const RREdgeId edge : rr_graph.node_in_edges(node)) {
        if (driver_switches.end() == std::find(driver_switches.begin(), driver_switches.end(), RRSwitchId(rr_graph.edge_switch(edge)))) {
            driver_switches.push_back(RRSwitchId(rr_graph.edge_switch(edge)));
        }
    }

    return driver_switches;
}

std::vector<RRNodeId> get_rr_graph_driver_nodes(const RRGraphView& rr_graph,
                                                const RRNodeId node) {
    std::vector<RRNodeId> driver_nodes;

    for (const RREdgeId edge : rr_graph.node_in_edges(node)) {
        driver_nodes.push_back(rr_graph.edge_src_node(edge));
    }

    return driver_nodes;
}

std::vector<RRNodeId> get_rr_graph_configurable_driver_nodes(const RRGraphView& rr_graph,
                                                             const RRNodeId node) {
    std::vector<RRNodeId> driver_nodes;

    for (const RREdgeId edge : rr_graph.node_in_edges(node)) {
        // Bypass non-configurable edges
        if (false == rr_graph.edge_is_configurable(edge)) {
            continue;
        }
        driver_nodes.push_back(rr_graph.edge_src_node(edge));
    }

    return driver_nodes;
}

std::vector<RRNodeId> get_rr_graph_non_configurable_driver_nodes(const RRGraphView& rr_graph,
                                                                 const RRNodeId node) {
    std::vector<RRNodeId> driver_nodes;

    for (const RREdgeId edge : rr_graph.node_in_edges(node)) {
        // Bypass configurable edges
        if (true == rr_graph.edge_is_configurable(edge)) {
            continue;
        }
        driver_nodes.push_back(rr_graph.edge_src_node(edge));
    }

    return driver_nodes;
}

bool is_opin_direct_connected_ipin(const RRGraphView& rr_graph,
                                   const RRNodeId node) {
    // We only accept OPIN
    VTR_ASSERT(e_rr_type::OPIN == rr_graph.node_type(node));

    if (1 != rr_graph.node_out_edges(node).size()) {
        return false;
    }

    VTR_ASSERT(1 == rr_graph.node_out_edges(node).size());
    for (auto edge : rr_graph.node_out_edges(node)) {
        const RRNodeId& sink_node = rr_graph.edge_sink_node(node, edge);
        if (e_rr_type::IPIN != rr_graph.node_type(sink_node)) {
            return false;
        }
    }

    return true;
}

bool is_ipin_direct_connected_opin(const RRGraphView& rr_graph,
                                   const RRNodeId node) {
    // We only accept IPIN
    VTR_ASSERT(e_rr_type::IPIN == rr_graph.node_type(node));

    if (1 != rr_graph.node_in_edges(node).size()) {
        return false;
    }

    VTR_ASSERT(1 == rr_graph.node_in_edges(node).size());
    for (const RREdgeId edge : rr_graph.node_in_edges(node)) {
        const RRNodeId& src_node = rr_graph.edge_src_node(edge);
        if (e_rr_type::OPIN != rr_graph.node_type(src_node)) {
            return false;
        }
    }

    return true;
}

e_side get_rr_graph_single_node_side(const RRGraphView& rr_graph,
                                     const RRNodeId node) {
    e_side node_side = NUM_2D_SIDES;
    int num_sides = 0;
    for (e_side candidate_side : TOTAL_2D_SIDES) {
        if (rr_graph.is_node_on_specific_side(node, candidate_side)) {
            node_side = candidate_side;
            num_sides++;
        }
    }
    VTR_ASSERT(1 == num_sides && node_side != NUM_2D_SIDES);
    return node_side;
}
