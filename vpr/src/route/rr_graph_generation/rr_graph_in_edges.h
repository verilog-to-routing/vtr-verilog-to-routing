#pragma once

/**
 * @file
 * @brief Stores and provides access to the incoming edges for each RR graph node.
 *
 * This is an optional data structure built on demand. It is primarily used by
 * the tileable RR graph generation code that needs to traverse edges in reverse
 * (from sink to source).
 *
 * Usage:
 *   RRGraphInEdges in_edges;
 *   in_edges.init(rr_graph);
 *   const auto& edges = in_edges.node_in_edges(node);
 */

#include "rr_graph_view.h"

class RRGraphInEdges {
  public:
    RRGraphInEdges() = default;

    /**
     * @brief Build the incoming edge lists by iterating all edges in the graph.
     * @param rr_graph The RR graph view to read edges from.
     */
    void init(const RRGraphView& rr_graph);

    /**
     * @brief Return the list of incoming edges for a given node.
     * @note init() must be called before this method.
     */
    const std::vector<RREdgeId>& node_in_edges(RRNodeId node) const;

    /** @brief Check if the structure has been initialized. */
    bool is_built() const { return is_built_; }

    /** @brief Clear all stored data. */
    void clear();

  private:
    vtr::vector<RRNodeId, std::vector<RREdgeId>> node_in_edges_;
    bool is_built_ = false;
};
