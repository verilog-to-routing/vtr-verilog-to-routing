#pragma once

/**
 * @file
 * @brief Conversion between the pin physical numbers of a cluster and its pb-graph pins, used to build the
 *        intra-cluster RR graph for flat routing.
 */

#include <vector>

#include "physical_types.h"

/**
 * @brief Converts between the pin physical numbers of one cluster (a logical block placed in one sub-tile instance)
 *        and its pb-graph pins, and walks the pb-graph edges of a pin without building lists of pin numbers.
 *
 * The pins connected to a pin through one pb-graph edge are visited edge by edge, in the order the pb graph stores
 * them; a pin reached through several edges is visited once per edge.
 */
class ClusterPinMap {
  public:
    ClusterPinMap(t_physical_tile_type_ptr physical_type,
                  const t_sub_tile* sub_tile,
                  int rel_cap,
                  t_logical_block_type_ptr logical_block);

    t_physical_tile_type_ptr physical_type() const { return physical_type_; }
    const t_sub_tile* sub_tile() const { return sub_tile_; }
    int rel_cap() const { return rel_cap_; }
    t_logical_block_type_ptr logical_block() const { return logical_block_; }

    /// @brief Return the pb-graph pin of a pin of the cluster.
    const t_pb_graph_pin* pb_pin(int pin_physical_num) const;

    /// @brief Return the physical number of a pb-graph pin of the cluster.
    int physical_num(const t_pb_graph_pin* pin) const;

    /// @brief Return whether a pin of the cluster belongs to a primitive (pins on the tile never do).
    bool is_primitive_pin(int pin_physical_num) const;

    /// @brief Return the delay of the pb-graph edge from one pin of the cluster to another (see edge()).
    float edge_delay(int src_pin_physical_num, int sink_pin_physical_num) const;

    /// @brief Return the pb-graph edge from one pin to another; if several edges connect them, the first one, as get_edge_sw_arch_idx() does.
    static const t_pb_graph_edge* edge(const t_pb_graph_pin* from, const t_pb_graph_pin* to);

    /// @brief Call fn(pin physical number, pb-graph pin) for each pin that pin drives through one pb-graph edge.
    template<typename Fn>
    void for_each_sink_pin(const t_pb_graph_pin* pin, Fn&& fn) const;

    /// @brief Call fn(pin physical number, pb-graph pin) for each pin that drives pin through one pb-graph edge.
    template<typename Fn>
    void for_each_src_pin(const t_pb_graph_pin* pin, Fn&& fn) const;

  private:
    t_physical_tile_type_ptr physical_type_;              ///< Tile type the cluster is placed in
    const t_sub_tile* sub_tile_;                          ///< Sub-tile the cluster is placed in
    int rel_cap_;                                         ///< Instance of sub_tile_ the cluster is placed in
    t_logical_block_type_ptr logical_block_;              ///< Logical block type of the cluster
    int first_tile_pin_;                                  ///< Physical number of the instance's first pin on the tile
    std::vector<const t_pb_graph_pin*> tile_pin_pb_pins_; ///< [tile pin - first_tile_pin_] -> pb-graph pin
    std::vector<int> root_pin_physical_nums_;             ///< [pin_count_in_cluster of a root-block pin] -> tile pin
    int intra_pin_offset_;                                ///< Physical number of the intra-cluster pin numbered 0 in the cluster
};

template<typename Fn>
void ClusterPinMap::for_each_sink_pin(const t_pb_graph_pin* pin, Fn&& fn) const {
    for (int edge_idx = 0; edge_idx < pin->num_output_edges; edge_idx++) {
        const t_pb_graph_edge* pb_edge = pin->output_edges[edge_idx];
        for (int pin_idx = 0; pin_idx < pb_edge->num_output_pins; pin_idx++) {
            const t_pb_graph_pin* sink_pin = pb_edge->output_pins[pin_idx];
            fn(physical_num(sink_pin), sink_pin);
        }
    }
}

template<typename Fn>
void ClusterPinMap::for_each_src_pin(const t_pb_graph_pin* pin, Fn&& fn) const {
    for (int edge_idx = 0; edge_idx < pin->num_input_edges; edge_idx++) {
        const t_pb_graph_edge* pb_edge = pin->input_edges[edge_idx];
        for (int pin_idx = 0; pin_idx < pb_edge->num_input_pins; pin_idx++) {
            const t_pb_graph_pin* src_pin = pb_edge->input_pins[pin_idx];
            fn(physical_num(src_pin), src_pin);
        }
    }
}
