#ifndef VPR_SRC_BASE_VPR_CONSTRAINTS_H_
#define VPR_SRC_BASE_VPR_CONSTRAINTS_H_

#include "vtr_vector.h"
#include "vpr_utils.h"

/**
 * @file
 * @brief This file defines the VprConstraints class used to store and read out data related to user-specified
 * 		  block and region constraints for the packing and placement stages.
 *
 * Overview
 * ========
 * This class contains functions that read in and store information from a constraints XML file.
 * The XML file provides a partition list, with the names/ids of the partitions, and each atom in the partition.
 * It also provides placement constraints, where it is specified which regions the partitions should go in.
 * In the constraints file, a placement constraint can also be created for a specific primitive. A partition would be automatically
 * created for the primitive when a placement constraint was specified for it.
 *
 */

class VprConstraints {

	public: //Constructors

	public: //Public members

		/**
		 * Store constraints information of each  constrained primitive.
		 * Vector indexed by atom block ids, for each constrained primitive it stores the xmin, xmax, ymin, ymax, zmin, zmax of the region
		 * it is in using a vector.
		 */
		vtr::vector_map<AtomBlockId, std::vector<int>> constrained_primitives;

		/**
		 * Store the name/id of each partition, and the ids of the primitives that are in it
		 * used std::vector instead of vtr::vector for atom ids because vtr::vector requires two arguments - ok to use?
		 */
		vtr::vector<int, std::vector<AtomBlockId>> partition_primitives;

		/**
		 * Vector indexed by partition ids, for each partition it stores the name/id of placement regions that it can be placed in.
		 * This is for the case where the placement constraint would be a union of multiple rectangles.
		 */
		vtr::vector<int, std::vector<int>> partition_regions;

	public: //Public Methods

		/**
		 * Method to create a new partition (assign a new id) when a new partition is declared in XML file
		 */
		void create_partition();

		/**
		 * Method to create a new placement region (assign a new id) when a new placement region is declared in XML file
		 */
		void create_placement_region();

		/**
		 * Next two methods are for loading the above data structures
		 */
		void load_constrained_primitives(const char* constraints_file);

		void load_partitions(const char* constraints_file);

		/**
		 * Higher level function to call above two functions if constraints file is being read
		 */
		void read_constraints_file(const char* constraints_file);

		/**
		 * Method to see if regions of primitives intersect for packing
		 * Should take in any number of primitives and see if any have intersecting regions
		 */
		bool do_prim_regions_intersect(const AtomBlockId id, const AtomBlockId id);

		/**
		 * Method to see whether a given primitive has been constrained by user or not
		 * Can check constrained_primitives vector map for this - if vector length is 0 for a primitive, it is not constrained.
		 */
		bool is_atom_constrained(const AtomBlockId);

		/**
		 * Method to check if an atom is found in a given partition
		 */
		bool is_atom_in_partition(const AtomBlockId, int);

	private: //Private Members

	private: //Private Methods
};

#endif /* VPR_SRC_BASE_VPR_CONSTRAINTS_H_ */
