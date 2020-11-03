#ifndef PARTITION_H
#define PARTITION_H

#include "vtr_strong_id.h"
#include "region.h"
#include "atom_netlist_fwd.h"
#include "partition_region.h"
/**
 * @file
 * @brief This file defines the data for a partition: a grouping of atoms that are constrained to a portion of an FPGA.
 *
 * A partition defines the atoms that are assigned to it, and the locations in which they can be placed.
 * The locations are a union of rectangles in x, y, sub_tile space.
 * Common cases include a single rectangle or a single x, y, sub_tile (specific location). More complex partitions
 * with L, T or other shapes can be created with a union of multiple rectangles.
 */

/// @brief Type tag for PartitionId
struct partition_id_tag;

/// @brief A unique identifier for a partition
typedef vtr::StrongId<partition_id_tag> PartitionId;

class Partition {
  public:
    /**
     *  @brief Get the unique name of the partition
     */
    const std::string get_name();

    /**
     * @brief Set the name of the partition
     *   @param _part_name      The name being assigned to the partition.
     */
    void set_name(std::string _part_name);

    /**
     * @brief Set the PartitionRegion (union of rectangular regions) for this partition
     *
     *   @param pr     The PartitionRegion belonging to the partition
     */
    void set_part_region(PartitionRegion pr);

    /**
     * @brief Get the PartitionRegion (union of rectangular regions) for this partition
     */
    const PartitionRegion get_part_region();

  private:
    std::string name;            ///< name of the partition, name will be unique across partitions
    PartitionRegion part_region; ///< the union of regions that the partition can be placed in
};

#endif /* PARTITION_H */
