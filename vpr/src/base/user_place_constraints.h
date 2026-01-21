#pragma once
/**
 * 
 * @brief This file defines the UserPlaceConstraints class used to store and read out data related to user-specified
 * 		  block and region constraints for the packing and placement stages.
 *
 * Overview
 * ========
 * This class contains functions that read in and store information related to floorplan constraints from a constraints XML file.
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

#include <unordered_map>
#include <unordered_set>

#include "vtr_vector.h"
#include "partition.h"
#include "partition_region.h"
#include "pack.h"

class UserPlaceConstraints {
  public:
    /**
     * @brief Store the id of a constrained atom with the id of the partition it belongs to
     *
     *   @param blk_id      The atom being stored
     *   @param part_id     The partition the atom is being constrained to
     */
    void add_constrained_atom(AtomBlockId blk_id, PartitionId part_id);

    /**
     * @brief Return id of the partition the atom belongs to
     *
     * If an atom is not in a partition (unconstrained), PartitionId::INVALID() is returned.
     *
     *   @param blk_id      The atom for which the partition id is needed
     */
    PartitionId get_atom_partition(AtomBlockId blk_id) const;
    /**
     * @brief Store a partition
     *
     *   @param part     The partition being stored
     */
    void add_partition(const Partition& part);

    /**
     * @brief Return a partition
     *
     *   @param part_id    The id of the partition that is wanted
     */
    const Partition& get_partition(PartitionId part_id) const;

    /**
     * @brief Returns a mutable partition
     *
     *   @param part_id    The id of the partition that is wanted
     */
    Partition& get_mutable_partition(PartitionId part_id);

    /**
     * @brief Return all the atoms that belong to a partition
     *
     *   @param part_id   The id of the partition whose atoms are needed
     */
    std::vector<AtomBlockId> get_part_atoms(PartitionId part_id) const;

    /**
     * @brief Returns the number of partitions in the object
     */
    int get_num_partitions() const;

    /**
     * @brief Returns the PartitionRegion belonging to the specified Partition
     *
     *   @param part_id The id of the partition whose PartitionRegion is needed
     */
    const PartitionRegion& get_partition_pr(PartitionId part_id) const;

    /**
     * @brief Returns the mutable PartitionRegion belonging to the specified Partition
     *
     *   @param part_id The id of the partition whose PartitionRegion is needed
     */
    PartitionRegion& get_mutable_partition_pr(PartitionId part_id);

    /**
     * @brief Constrain the atoms in the given partition to be within blocks of a specific type
     *
     *   @param part_id   The id of the partition being constrained
     *   @param lb_type   The logical block type the partition is constrained to
     */
    void constrain_part_lb_type(PartitionId part_id, t_logical_block_type_ptr lb_type);

    /**
     * @brief Check if a partition has logical block type constraints
     *
     *   @param part_id The id of the partition to check
     *
     *   @return True if the partition has logical block type constraints, false otherwise
     */
    bool is_part_constrained_to_lb_types(PartitionId part_id) const;

    /**
     * @brief Return the set of logical block types a partition is constrained to
     *
     *   @param part_id The id of the partition whose type constraints are needed
     */
    const std::unordered_set<t_logical_block_type_ptr>& get_part_lb_type_constraints(PartitionId part_id) const;

  private:
    /**
     * Store logical block type constraints for each partition
     */
    std::unordered_map<PartitionId, std::unordered_set<t_logical_block_type_ptr>> constrained_part_lb_types_;

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
void print_placement_constraints(FILE* fp, const UserPlaceConstraints& constraints);
