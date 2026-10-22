#pragma once

/**
 * @file
 * @brief Conversion between the pin physical numbers of a cluster and its pb-graph pins, used to build the
 *        intra-cluster RR graph for flat routing.
 */

#include <ranges>
#include <vector>

#include "physical_types.h"
#include "vtr_assert.h"

/**
 * @brief Converts between the pin physical numbers of one cluster (a logical block placed in one sub-tile instance)
 *        and its pb-graph pins, and walks the pb-graph edges of a pin without building lists of pin numbers.
 *
 * sink_pins() and src_pins() list the pins connected to a pin through one pb-graph edge, edge by edge in the order
 * the pb graph stores them; a pin reached through several edges appears once per edge.
 */
class ClusterPinMap {
  public:
    /// @brief A pin connected to another pin through one pb-graph edge.
    struct t_connected_pin {
        int physical_num;             ///< Physical number of the connected pin
        const t_pb_graph_pin* pb_pin; ///< The connected pb-graph pin
    };

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

    /// @brief Return a range of the pins that pin drives, one t_connected_pin per output edge.
    auto sink_pins(const t_pb_graph_pin* pin) const {
        return std::views::counted(pin->output_edges.data(), pin->num_output_edges)
               | std::views::transform([this](const t_pb_graph_edge* pb_edge) {
                     // Every pb-graph edge connects one pin to one pin
                     VTR_ASSERT(pb_edge->num_output_pins == 1);
                     const t_pb_graph_pin* sink_pin = pb_edge->output_pins[0];
                     return t_connected_pin{physical_num(sink_pin), sink_pin};
                 });
    }

    /// @brief Return a range of the pins that drive pin, one t_connected_pin per input edge.
    auto src_pins(const t_pb_graph_pin* pin) const {
        return std::views::counted(pin->input_edges.data(), pin->num_input_edges)
               | std::views::transform([this](const t_pb_graph_edge* pb_edge) {
                     // Every pb-graph edge connects one pin to one pin
                     VTR_ASSERT(pb_edge->num_input_pins == 1);
                     const t_pb_graph_pin* src_pin = pb_edge->input_pins[0];
                     return t_connected_pin{physical_num(src_pin), src_pin};
                 });
    }

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
