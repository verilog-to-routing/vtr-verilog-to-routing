/**
 * @file
 * @brief This file contains functions related to writing out a vpr constraints XML file.
 *
 * Overview
 * ========
 * VPR floorplan constraints consist of region constraints specified for primitives that must be respected during packing and placement.
 * The constraints XML file is printed using the XML schema vpr/src/base/vpr_constraints.xsd
 *
 * Routines related to writing out the file are in vpr/src/base/vpr_constraints_serializer.h. For more information on how
 * the writing interface works, refer to vpr/src/route/SCHEMA_GENERATOR.md
 *
 * The option --write_vpr_constraints can be used to generate the constraints files.
 *
 * The routines in this file are currently used to generate floorplan constraints for testing purposes.
 * The constraints files they generate are used to determine whether VPR is correctly adhering to
 * floorplan constraints during its packing and placement stages.
 *
 * The placer options --floorplan_num_horizontal_partitions (int) and --floorplan_num_vertical_partitions (int) can be used
 * to specify how many partitions should be created in the test constraints file.
 * For example, if both options are 2, the constraints file will split the grid into quadrants, dividing the blocks between
 * four partitions - two partitions in the horizontal dimension, and two partitions in the vertical dimension.
 */

#ifndef VPR_SRC_BASE_VPR_CONSTRAINTS_WRITER_H_
#define VPR_SRC_BASE_VPR_CONSTRAINTS_WRITER_H_

/**
 * @brief Write out floorplan constraints to an XML file based on current placement
 *
 *   @param file_name   The name of the file that the constraints will be written to
 *   @param expand      The amount the floorplan region will be expanded around the current
 *   					x, y location of the block. Ex. if location is (1, 1) and expand = 1,
 *   					the floorplan region will be from (0, 0) to (2, 2).
 *   @param subtile     Specifies whether to write out the constraint regions with or without
 *                      subtile values.
 */
void write_vpr_floorplan_constraints(const char* file_name, int expand, bool subtile, int horizontal_partitions, int vertical_partitions);

//Generate constraints which lock all blocks to one location.
void setup_vpr_floorplan_constraints_one_loc(VprConstraints& constraints, int expand, bool subtile);

/* Generate constraints which divide the grid into partition according to the horizontal and vertical partition values passed in
 * and lock down blocks to their appropriate partition.
 */
void setup_vpr_floorplan_constraints_cutpoints(VprConstraints& constraints, int horizontal_cutpoints, int vertical_cutpoints);

void create_partition(Partition& part, std::string part_name, int xmin, int ymin, int xmax, int ymax);

#endif /* VPR_SRC_BASE_VPR_CONSTRAINTS_WRITER_H_ */
