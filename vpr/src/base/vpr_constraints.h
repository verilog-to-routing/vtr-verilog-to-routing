#ifndef VPR_CONSTRAINTS_H
#define VPR_CONSTRAINTS_H

#include "vtr_vector.h"
#include "vpr_utils.h"
#include "partition.h"
#include "partition_region.h"

/**
 * @file
 * @brief This file defines the VprConstraints class used to store and read out data related to user-specified
 * 		  block and region constraints for the packing and placement stages.
 *
 * Overview
 * ========
 * This class contains functions that read in and store information from a constraints XML file.
 * The XML file provides a partition list, with the names of the partitions, and each atom in the partition.
 * It also specifies which regions the partitions should be placed in. Atoms cannot be placed in more than one partition.
 * If an atom is assigned to more than one partition, the last partition is was assigned to will be the partition it is placed in.
 *
 * Related Classes
 * ===============
 * The following definitions are useful to understanding this class:
 *
 * Partition: a grouping of atoms that are constrained to a portion of an FPGA
 * See vpr/src/base/partition.h for more detail
 *
 * Region: the x and y bounds of a rectangular region, optionally including a subtile value,
 * that atoms in a partition are constrained to
 * See vpr/src/base/region.h for more detail
 *
 * PartitionRegion: the union of regions that a partition can be placed in
 * See vpr/src/base/partition_region.h for more detail
 *
 *
 */

class VprConstraints {
  public:
    /**
     * @brief Store the id of a constrained atom with the id of the partition it belongs to
     *
     *   @param blk_id      The atom being stored
     *   @param part_id     The partition the atom is being constrained to
     */
    void add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id);

    /**
     * @brief Return id of the partition the atom belongs to
     *
     * If an atom is not in a partition (unconstrained), PartitionId::INVALID() is returned.
     *
     *   @param blk_id      The atom for which the partition id is needed
     */
    PartitionId get_atom_partition(AtomBlockId blk_id);

    /**
     * @brief Store a partition
     *
     *   @param part     The partition being stored
     */
    void add_partition(Partition part);

    /**
     * @brief Return a partition
     *
     *   @param part_id    The id of the partition that is wanted
     */
    Partition get_partition(PartitionId part_id);

    /**
     * @brief Return all the atoms that belong to a partition
     *
     *   @param part_id   The id of the partition whose atoms are needed
     */
    std::vector<AtomBlockId> get_part_atoms(PartitionId part_id);

    /**
     * @brief Returns the number of partitions in the object
     */
    int get_num_partitions();

    /**
     * @brief Returns the PartitionRegion belonging to the specified Partition
     *
     *   @param part_id The id of the partition whose PartitionRegion is needed
     */
    PartitionRegion get_partition_pr(PartitionId part_id);

  private:
    /**
     * Store all constrained atoms
     */
    std::unordered_map<AtomBlockId, PartitionId> constrained_atoms;

    /**
     * Store all partitions
     */
    vtr::vector<PartitionId, Partition> partitions;
};

///@brief used to print floorplanning constraints data from a VprConstraints object
void print_constraints(FILE* fp, VprConstraints constraints);

#endif /* VPR_CONSTRAINTS_H */
