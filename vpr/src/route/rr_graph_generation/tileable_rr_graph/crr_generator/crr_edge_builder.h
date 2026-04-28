#pragma once

/**
 * @file crr_edge_builder.h
 * @brief Builds edges for CRR graphs
 *
 * This file contains the main function to be called from RR Graph Generator to build CRR edges.
 */

#include <unordered_map>

#include "vtr_vector.h"
#include "rr_gsb.h"
#include "rr_graph_builder.h"

#include "crr_connection_builder.h"

/**
 * @brief Pre-create one CRR architecture switch per integer delay (ps) in the
 *        inclusive range [min_delay_ps, max_delay_ps] and return a map from
 *        delay to RRSwitchId.
 *
 * Switches are inserted into the device's all_sw_inf and indexed only by
 * their delay - template ids are intentionally not used.
 *
 * @param min_delay_ps Minimum delay observed across all switch template files
 * @param max_delay_ps Maximum delay observed across all switch template files
 * @param verbosity Logging verbosity
 */
std::unordered_map<int, RRSwitchId> pre_create_crr_switches(const int min_delay_ps,
                                                            const int max_delay_ps,
                                                            const int verbosity);

/**
 * @brief Builds edges for a CRR GSB
 * @param rr_graph_builder The RR graph builder
 * @param num_edges_to_create The number of edges created
 * @param rr_node_driver_switches The driver switches for each RR node
 * @param rr_gsb The GSB for which edges are to be built
 * @param connection_builder The connection builder to use to get the connections at each tile
 * @param delay_to_switch_id Map from delay (ps) to a pre-created RRSwitchId. Any
 *                           connection delay other than -1 must be present in
 *                           this map; otherwise an assertion is raised.
 * @param verbosity The verbosity level of the log
 */
void build_crr_gsb_edges(RRGraphBuilder& rr_graph_builder,
                         size_t& num_edges_to_create,
                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                         const RRGSB& rr_gsb,
                         const crrgenerator::CRRConnectionBuilder& connection_builder,
                         const std::unordered_map<int, RRSwitchId>& delay_to_switch_id,
                         const int verbosity);
