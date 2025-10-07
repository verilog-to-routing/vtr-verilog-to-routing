
#pragma once

/**
 * @file
 * @brief Functions for creating RR graph nodes for tile classes and pins.
 *
 * This header declares utilities used during RR graph construction:
 * - Adds SOURCE and SINK nodes for within a tile.
 * - Adds OPIN and IPIN nodes for physical pins of a tile.
 * - Provides connections between source/sinks and pins using delayless switches.
 */

#include <vector>
#include "physical_types.h"
#include "rr_edge.h"

class RRGraphBuilder;

/// @brief Create SOURCE and SINK nodes for each class in a tile and set their properties.
void add_classes_rr_graph(RRGraphBuilder& rr_graph_builder,
                          const std::vector<int>& class_num_vec,
                          const t_physical_tile_loc& root_loc,
                          t_physical_tile_type_ptr physical_type);

/// @brief Create OPIN and IPIN nodes for each pin in a tile and set their properties.
void add_pins_rr_graph(RRGraphBuilder& rr_graph_builder,
                       const std::vector<int>& pin_num_vec,
                       const t_physical_tile_loc& root_loc,
                       t_physical_tile_type_ptr physical_type);

/**
 * @brief Add the edges between IPIN to SINK and SOURCE to OPIN to rr_edges_to_create
 * @param rr_graph_builder RR Graph Builder object which contain the RR Graph storage
 * @param class_num_vec Class physical numbers to add the edges connected to them
 * @param tile_loc The root location of the block to add the SINK/SRC connections of it.
 * @param rr_edges_to_create An object which store all the edges created in this function.
 * @param delayless_switch Switch ID of the delayless switch.
 * @param physical_type_ptr A pointer to the physical type of the block for which the edges are created.
 * @param switches_remapped A flag to indicate whether edge switch IDs are remapped
 */
void connect_src_sink_to_pins(RRGraphBuilder& rr_graph_builder,
                              const std::vector<int>& class_num_vec,
                              const t_physical_tile_loc& tile_loc,
                              t_rr_edge_info_set& rr_edges_to_create,
                              const int delayless_switch,
                              t_physical_tile_type_ptr physical_type_ptr,
                              bool switches_remapped);
