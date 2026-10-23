#pragma once

/**
 * @file
 * @brief Conversion between the physical numbers of a cluster's pins and its pb-graph pins, used to build the
 *        intra-cluster RR graph for flat routing.
 *
 * Each pin of a physical tile type, whether on the tile or inside a cluster placed in the tile, has a physical
 * number. The pins on the tile are numbered from 0 to num_pins - 1. The pins inside the clusters come after them:
 * each sub-tile instance reserves a range of numbers for each logical block type it can hold, and a pb-graph pin of
 * a cluster is numbered by the start of that range plus its pin_count_in_cluster. The pins of the cluster's root
 * block are pins on the tile, so they keep their numbers on the tile.
 */

#include <vector>

#include "physical_types.h"

/**
 * @brief Converts between the physical numbers of the pins of one cluster and the cluster's pb-graph pins. The
 *        cluster is a logical block placed in one instance of a sub-tile.
 */
class ClusterPinMap {
  public:
    /**
     * @param physical_type     The tile type the cluster is placed in
     * @param sub_tile          The sub-tile of physical_type the cluster is placed in
     * @param sub_tile_instance Which instance of sub_tile the cluster is placed in, counting from 0
     * @param logical_block     The logical block type of the cluster
     */
    ClusterPinMap(const t_physical_tile_type& physical_type,
                  const t_sub_tile& sub_tile,
                  int sub_tile_instance,
                  t_logical_block_type_ptr logical_block);

    t_physical_tile_type_ptr physical_type() const { return &physical_type_; }

    /// @brief Return the pb-graph pin of a pin of the cluster.
    const t_pb_graph_pin* pb_pin(int pin_physical_num) const;

    /// @brief Return the physical number of a pb-graph pin of the cluster.
    int physical_num(const t_pb_graph_pin* pin) const;

    /// @brief Return whether a pin of the cluster belongs to a primitive. Pins on the tile never do.
    bool is_primitive_pin(int pin_physical_num) const;

    /// @brief Return the delay of the pb-graph edge from one pin of the cluster to another.
    float edge_delay(int src_pin_physical_num, int sink_pin_physical_num) const;

  private:
    const t_physical_tile_type& physical_type_;           ///< Tile type the cluster is placed in
    int first_tile_pin_;                                  ///< Physical number of the sub-tile instance's first pin on the tile
    std::vector<const t_pb_graph_pin*> tile_pin_pb_pins_; ///< pb-graph pins of the sub-tile instance's pins on the tile, in the order of their physical numbers
    std::vector<int> root_pin_physical_nums_;             ///< Physical number of each pin of the cluster's root block, indexed by the pin's pin_count_in_cluster
    int intra_pin_offset_;                                ///< First physical number of the range reserved for the pins inside the cluster
};
