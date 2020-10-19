#ifndef PARTITION_H
#define PARTITION_H

#include "vtr_strong_id.h"
#include "region.h"
#include "partition_regions.h"
#include "atom_netlist_fwd.h"
/**
 * @file
 * @brief This file defines the data for a partition: a grouping of atoms that are constrained to a portion of an FPGA. A partition defines
 * the atoms that are assigned to it, and the locations in which they can be placed. The locations are a union of rectangles in x, y,
 * sub_tile space. Common cases include a single rectangle or a single x, y, sub_tile (specific location). More complex partitions
 * with L, T or other shapes can be created with a union of multiple rectangles.
 */

//Type tag for PartitionId
struct partition_id_tag;

//A unique identifier for a partition
typedef vtr::StrongId<partition_id_tag> PartitionId;

class Partition {
  public:
    PartitionId get_partition_id();
    void set_partition_id(PartitionId _part_id);

    std::string get_name();
    void set_name(std::string _part_name);

    //add an id to the atom_blocks vector
    void add_to_atoms(AtomBlockId atom_id);

    //check if a given atom is in the partition
    bool contains_atom(AtomBlockId atom_id);

    //get the atom_blocks vector
    std::vector<AtomBlockId> get_atoms();

    //set the union of regions for this partition;
    void set_part_regions(PartitionRegions pr);

    //get the union of regions of this partition
    PartitionRegions get_part_regions();

  private:
    PartitionId id;                       //unique id for this partition
    std::string name;                     //name of the partition, name will be unique across partitions
    std::vector<AtomBlockId> atom_blocks; //atoms that belong to this partition, each atom should only belong to one partition
    PartitionRegions part_regions;        //the union of regions that the partition can be placed in
};

#endif /* PARTITION_H */
