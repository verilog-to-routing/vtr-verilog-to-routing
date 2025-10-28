#pragma once

#include "rr_graph_fwd.h"
#include "rr_graph_view.h"

#include "crr_common.h"
#include "custom_rr_graph.h"

namespace crrgenerator {

/**
 * @brief Manages efficient node lookup and indexing for RR graphs
 *
 * This class creates spatial indexes for nodes to enable fast lookup
 * by coordinates, type, and other attributes.
 */
class NodeLookupManager {
  public:
    /**
     * @brief Constructor
     * @param rr_graph RR graph to index
     * @param fpga_grid_x Maximum X coordinate
     * @param fpga_grid_y Maximum Y coordinate
     */
    NodeLookupManager(const RRGraphView& rr_graph, size_t fpga_grid_x, size_t fpga_grid_y);

    /**
     * @brief Initialize lookup tables from RR graph
     */
    void initialize();

    /**
     * @brief Get nodes in a specific column
     * @param x Column coordinate
     * @return Map of hash to node for nodes in column
     */
    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& get_column_nodes(size_t x) const;

    /**
     * @brief Get nodes in a specific row
     * @param y Row coordinate
     * @return Map of hash to node for nodes in row
     */
    const std::unordered_map<NodeHash, RRNodeId, NodeHasher>& get_row_nodes(size_t y) const;

    /**
     * @brief Get combined nodes for column and row (for switch block processing)
     * @param x Column coordinate
     * @param y Row coordinate
     * @return Combined map of nodes
     */
    std::unordered_map<NodeHash, RRNodeId, NodeHasher> get_combined_nodes(size_t x, size_t y) const;

    /**
     * @brief Print lookup statistics
     */
    void print_statistics() const;

    /**
     * @brief Clear all lookup tables
     */
    void clear();

  private:
    const RRGraphView& rr_graph_;
    // Spatial indexes - SINK and SOURCE nodes are not stored in these two maps
    std::vector<std::unordered_map<NodeHash, RRNodeId, NodeHasher>> column_lookup_;
    std::vector<std::unordered_map<NodeHash, RRNodeId, NodeHasher>> row_lookup_;

    // Edge spatial indexes. [x][y] -> std::vector<const RREdge*>
    std::vector<std::vector<std::vector<const RREdge*>>> edge_sink_lookup_;

    // Global lookup - Return a pointer to the node corresponding to the hash
    std::unordered_map<NodeHash, const RRNode*, NodeHasher> global_lookup_;

    // Grid dimensions
    size_t fpga_grid_x_;
    size_t fpga_grid_y_;

    // Helper methods
    NodeHash build_node_hash(RRNodeId node_id) const;
    void index_node(RRNodeId node_id);
};

} // namespace crrgenerator
