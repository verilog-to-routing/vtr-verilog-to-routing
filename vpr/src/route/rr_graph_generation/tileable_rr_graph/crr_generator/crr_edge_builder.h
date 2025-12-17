#pragma once

/**
 * @file crr_edge_builder.h
 * @brief Builds edges for CRR graphs
 *
 * This file contains the main function to be called from RR Graph Generator to build CRR edges.
 */

#include "vtr_vector.h"
#include "rr_gsb.h"
#include "rr_graph_builder.h"

#include "crr_connection_builder.h"

/**
 * @brief Builds edges for a CRR GSB
 * @param rr_graph_builder The RR graph builder
 * @param rr_node_driver_switches The driver switches for each RR node
 * @param rr_gsb The GSB for which edges are to be built
 * @param connection_builder The connection builder to use to get the connections at each tile
 * @param verbosity The verbosity level of the log
 */
void build_crr_gsb_edges(RRGraphBuilder& rr_graph_builder,
                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                         const RRGSB& rr_gsb,
                         const crrgenerator::CRRConnectionBuilder& connection_builder,
                         const int verbosity);
