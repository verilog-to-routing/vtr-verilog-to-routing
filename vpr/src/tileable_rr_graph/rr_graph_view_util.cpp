/****************************************************************************
 * This file include most-utilized functions that manipulate on the
 * RRGraph object
 ***************************************************************************/
#include "rr_graph_view_util.h"

/****************************************************************************
 * Find the switches interconnecting two nodes
 * Return a vector of switch ids
 ***************************************************************************/
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraphView& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node) {
    std::vector<RRSwitchId> switches;

    std::vector<RREdgeId> edges = rr_graph.find_edges(from_node, to_node);
    if (true == edges.empty()) {
        /* edge is open, we return an empty vector of switches */
        return switches;
    }

    /* Reach here, edge list is not empty, find switch id one by one
     * and update the switch list
     */
    for (auto edge : edges) {
        switches.push_back(rr_graph.edge_switch(edge));
    }

    return switches;
}

/*********************************************************************
 * Like the RRGraph.find_node() but returns all matching nodes,
 * rather than just the first. This is particularly useful for getting all instances
 * of a specific IPIN/OPIN at a specific grid tile (x,y) location.
 **********************************************************************/
std::vector<RRNodeId> find_rr_graph_nodes(const RRGraphView& rr_graph,
                                          const int& x,
                                          const int& y,
                                          const t_rr_type& rr_type,
                                          const int& ptc) {
    std::vector<RRNodeId> indices;

    if (rr_type == IPIN || rr_type == OPIN) {
        //For pins we need to look at all the sides of the current grid tile

        for (e_side side : SIDES) {
            RRNodeId rr_node_index = rr_graph.node_lookup().find_node(x, y, rr_type, ptc, side);

            if (rr_node_index != RRNodeId::INVALID()) {
                indices.push_back(rr_node_index);
            }
        }
    } else {
        //Sides do not effect non-pins so there should only be one per ptc
        RRNodeId rr_node_index = rr_graph.node_lookup().find_node(x, y, rr_type, ptc);

        if (rr_node_index != RRNodeId::INVALID()) {
            indices.push_back(rr_node_index);
        }
    }

    return indices;
}

/*********************************************************************
 * Find a list of rr nodes in a routing channel at (x,y)
 **********************************************************************/
std::vector<RRNodeId> find_rr_graph_chan_nodes(const RRGraphView& rr_graph,
                                               const int& x,
                                               const int& y,
                                               const t_rr_type& rr_type) {
    std::vector<RRNodeId> indices;

    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    for (const RRNodeId& rr_node_index : rr_graph.node_lookup().find_channel_nodes(x, y, rr_type)) {
        if (rr_node_index != RRNodeId::INVALID()) {
            indices.push_back(rr_node_index);
        }
    }

    return indices;
}

/*********************************************************************
 * Find a list of rr_nodes that locate at a side of a grid
 **********************************************************************/
std::vector<RRNodeId> find_rr_graph_grid_nodes(const RRGraphView& rr_graph,
                                               const DeviceGrid& device_grid,
                                               const int& x,
                                               const int& y,
                                               const t_rr_type& rr_type,
                                               const e_side& side,
                                               bool include_clock) {
    std::vector<RRNodeId> indices;

    VTR_ASSERT(rr_type == IPIN || rr_type == OPIN);

    /* Ensure that (x, y) is a valid location in grids */
    VTR_ASSERT(size_t(x) <= device_grid.width() && size_t(y) <= device_grid.height());

    /* Ensure we have a valid side */
    VTR_ASSERT(side != NUM_SIDES);

    /* Find all the pins on the side of the grid */
    int width_offset = device_grid[x][y].width_offset;
    int height_offset = device_grid[x][y].height_offset;
    for (int pin = 0; pin < device_grid[x][y].type->num_pins; ++pin) {
        /* Skip those pins have been ignored during rr_graph build-up */
        if (true == device_grid[x][y].type->is_ignored_pin[pin]) {
            /* If specified, force to include all the clock pins */
            if (!include_clock || std::find(device_grid[x][y].type->get_clock_pins_indices().begin(), device_grid[x][y].type->get_clock_pins_indices().end(), pin) != device_grid[x][y].type->get_clock_pins_indices().end()) {
                continue;
            }
        }
        if (false == device_grid[x][y].type->pinloc[width_offset][height_offset][side][pin]) {
            /* Not the pin on this side, we skip */
            continue;
        }

        /* Try to find the rr node */
        RRNodeId rr_node_index = rr_graph.node_lookup().find_node(x, y, rr_type, pin, side);
        if (rr_node_index != RRNodeId::INVALID()) {
            indices.push_back(rr_node_index);
        }
    }

    return indices;
}
