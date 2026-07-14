#pragma once

#include "physical_types.h"

struct t_pb_graph_edge_comparator {
    int input_pin_id_in_cluster;
    int output_pin_id_in_cluster;
    t_pb_graph_pin* input_pin;
    t_pb_graph_pin* output_pin;
    t_pb_graph_edge* parent_edge;
};

// Find the edge between driver_pin and pin.
//
// Returns nullptr on invalid input or error.
const t_pb_graph_edge* get_edge_between_pins(
    const t_pb_graph_pin* driver_pin,
    const t_pb_graph_pin* pin);

/**
 * @brief BFS through pb_graph output_edges to find the accumulated delay_max
 *        from src to sink.
 *
 * @param src   Source pb_graph pin to search from.
 * @param sink  Sink pb_graph pin to search for.
 *
 * @return The accumulated delay_max along the path from src to sink, or
 *         -1.0f if no path exists between the two pins.
 */
float calc_pb_graph_path_delay(const t_pb_graph_pin* src, const t_pb_graph_pin* sink);

/**
 * @brief BFS through pb_graph output_edges from src to find the accumulated
 *        delay_max to the nearest reachable root-block (cluster-level) pin.
 *
 * Used to estimate the delay from a primitive pin to the boundary of its
 * cluster, e.g. for connections that leave the cluster via dedicated chain
 * wiring (such as carry chains).
 *
 * @param src   Source pb_graph pin to search from.
 *
 * @return The accumulated delay_max along the path from src to the nearest
 *         root-block pin, or -1.0f if no root-block pin is reachable.
 */
float calc_pb_graph_delay_to_root_pin(const t_pb_graph_pin* src);

/**
 * @brief Backward BFS through pb_graph input_edges from sink to find the
 *        accumulated delay_max from the nearest reachable root-block
 *        (cluster-level) pin.
 *
 * Used to estimate the delay from the boundary of a cluster to a primitive
 * pin, e.g. for connections that enter the cluster via dedicated chain
 * wiring (such as carry chains).
 *
 * @param sink  Sink pb_graph pin to search from (traversed backwards).
 *
 * @return The accumulated delay_max along the path from the nearest
 *         root-block pin to sink, or -1.0f if no root-block pin is reachable.
 */
float calc_pb_graph_delay_from_root_pin(const t_pb_graph_pin* sink);

void alloc_and_load_all_pb_graphs(bool load_power_structures, bool is_flat);
void echo_pb_graph(char* filename);
void free_pb_graph_edges();
t_pb_graph_pin*** alloc_and_load_port_pin_ptrs_from_string(const int line_num,
                                                           const t_pb_graph_node* pb_graph_parent_node,
                                                           t_pb_graph_node** pb_graph_children_nodes,
                                                           std::string_view port_string,
                                                           int** num_ptrs,
                                                           int* num_sets,
                                                           const bool is_input_to_interc,
                                                           const bool interconnect_error_check);
