#pragma once

/**
 * @file router_lookahead_interposer.h
 * @brief This file contains the class that calculates the lookahead matrix for interposer-based architectures.
 * 
 */

#include "rr_graph_view.h"

/**
 * @brief Provides a fast, matrix-based lookahead for interposer-crossing delays.
 * In 2.5D FPGA architectures, die-crossing delays can be calculated in constant time
 * by calling get_interposer_lookahead_delay.
 *
 * See compute_interposer_delay_matrix for information on how the internal delay matrix
 * works.
 */
class InterposerDelayLookahead {
  public:
    /**
     * @brief Constructs the lookahead and populates the internal delay matrix.
     * @param rr_graph The Routing Resource Graph.
     * @param grid The Device Grid, used to determine die boundaries and interposer positions.
     */
    InterposerDelayLookahead(const RRGraphView& rr_graph, const DeviceGrid& grid);

    /**
     * @brief Estimates the delay between two nodes based on their die locations.
     * @param from_node The source routing resource node.
     * @param to_node The destination routing resource node.
     * @return float The estimated delay
     */
    float get_interposer_lookahead_delay(RRNodeId from_node, RRNodeId to_node) const;

  private:
    /// @brief 2D Matrix storing pre-calculated delays between Die[i] and Die[j].
    vtr::NdMatrix<float, 2> die_to_die_delay_matrix_;
    
    const RRGraphView& rr_graph_;
    const DeviceGrid& grid_;
};