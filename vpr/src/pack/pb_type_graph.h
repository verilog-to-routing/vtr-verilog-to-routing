#pragma once

#include <vector>

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
 *
 * NOTE: We use BFS here since we are only trying to estimate what the delay
 *       would eventually be after intra-lb routing. Since the current intra-lb
 *       router is not timing-driven, we want the path that minimizes resource
 *       use. We could switch to a minimum delay (Djikstra) search, but it would
 *       probably make little difference in timing estimation for these short
 *       routes.
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
 * NOTE: See the explanation above for why we use BFS here.
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
 * NOTE: See the explanation above for why we use BFS here.
 *
 * @param sink  Sink pb_graph pin to search from (traversed backwards).
 *
 * @return The accumulated delay_max along the path from the nearest
 *         root-block pin to sink, or -1.0f if no root-block pin is reachable.
 */
float calc_pb_graph_delay_from_root_pin(const t_pb_graph_pin* sink);

/**
 * @brief Find every pin in the same cluster which is equivalent to the given
 *        primitive pin.
 *
 * Equivalent pins are the pins with the same model port and pin number on
 * every primitive in the same cluster (root pb_graph node), in any mode, which
 * implements the same model as the given pin's primitive. These are the pins
 * that a primitive pin could be implemented by when the specific primitive
 * that an atom is packed into is not known.
 *
 * @param pin   A pin of a primitive pb_graph node.
 *
 * @return The equivalent pins (including the given pin).
 */
std::vector<const t_pb_graph_pin*> find_equivalent_pb_graph_pins(const t_pb_graph_pin* pin);

/**
 * @brief Find the minimum delay of a path from any pin equivalent to src to
 *        any pin equivalent to sink (see find_equivalent_pb_graph_pins).
 *
 * This is useful to estimate the intra-cluster delay between two primitive
 * pins when the specific primitives that will be used are not known. For
 * example, a LUT driving an adder may be packed into the LUT that directly
 * drives the adder, rather than a LUT which must use the cluster's local
 * interconnect.
 *
 * NOTE: See the explanation above for why we use BFS here.
 *
 * @param src   Source pb_graph pin whose equivalent pins are searched from.
 * @param sink  Sink pb_graph pin whose equivalent pins are searched for.
 *
 * @return The minimum accumulated delay_max along a path from an equivalent
 *         source pin to an equivalent sink pin, or -1.0f if no path exists.
 */
float calc_min_equivalent_pin_path_delay(const t_pb_graph_pin* src, const t_pb_graph_pin* sink);

/**
 * @brief Find the minimum delay from any pin equivalent to src to the nearest
 *        reachable root-block (cluster-level) pin.
 *
 * See find_equivalent_pb_graph_pins for the definition of equivalent
 * pins. This is useful when the
 * specific instance of the primitive is not known, but the connection must
 * leave the cluster through a specific instance. For example, a carry chain
 * may only leave a cluster through the cout pin of the last adder in the
 * cluster.
 *
 * NOTE: See the explanation above for why we use BFS here.
 *
 * @param src   Source pb_graph pin whose equivalent pins are searched from.
 *
 * @return The minimum accumulated delay_max from an equivalent pin to a
 *         root-block pin, or -1.0f if no root-block pin is reachable.
 */
float calc_min_equivalent_pin_delay_to_root_pin(const t_pb_graph_pin* src);

/**
 * @brief Find the minimum delay from the nearest reachable root-block
 *        (cluster-level) pin to any pin equivalent to sink.
 *
 * See find_equivalent_pb_graph_pins for the definition of equivalent
 * pins. This is useful when the
 * specific instance of the primitive is not known, but the connection must
 * enter the cluster through a specific instance. For example, a carry chain
 * may only enter a cluster through the cin pin of the first adder in the
 * cluster.
 *
 * NOTE: See the explanation above for why we use BFS here.
 *
 * @param sink  Sink pb_graph pin whose equivalent pins are searched to.
 *
 * @return The minimum accumulated delay_max from a root-block pin to an
 *         equivalent pin, or -1.0f if no equivalent pin is reachable.
 */
float calc_min_equivalent_pin_delay_from_root_pin(const t_pb_graph_pin* sink);

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
