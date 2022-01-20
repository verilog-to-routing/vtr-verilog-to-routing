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
 * Users can also use the VPR option --floorplan_split to specify which constraints files they would like to generate.
 * The options for floorplan_split include halves, quadrants, sixteenths and one_spot. Two constraints files will be generated
 * in each case. The default setting for floorplan_split is halves.
 *
 * If the user specified halves, two floorplanning partitions would be created which each take up one half the grid.
 * One of the constraints files assigns all cluster blocks to one of these two partitions, while the other constraints
 * file would assign half of all cluster blocks to one of the two partitions (meaning the other half are unconstrained).
 * The same idea applies for the quadrants and sixteenths options, except that the floorplan partitions would split the
 * grid into quadrants and sixteenths respectively, as the names imply.
 *
 * If the one_spot option is specified, all blocks will be locked to their original placement spot in one file, and half
 * of all blocks will be locked to their original placement spot in the other file. The vpr options --place_constraint_subtile
 * and --place_constraint_expand can be used in this case to specify whether the blocks should also be locked down to a particular
 * subtile, or whether the constraint region should expand beyond one spot.
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
void write_vpr_floorplan_constraints(const char* file_name, int expand, bool subtile, enum constraints_split_factor floorplan_split);

void setup_vpr_floorplan_constraints(VprConstraints& constraints, int expand, bool subtile);

void setup_vpr_floorplan_constraints_cutpoints(VprConstraints& constraints, int horizontal_cutpoints, int vertical_cutpoints);

void setup_vpr_floorplan_constraints_cutpoints_half(VprConstraints& constraints, int horizontal_cutpoints, int vertical_cutpoints);

void setup_vpr_floorplan_constraints_half(VprConstraints& constraints, int expand, bool subtile);

void create_partition(Partition& part, std::string part_name, int xmin, int ymin, int xmax, int ymax);

#endif /* VPR_SRC_BASE_VPR_CONSTRAINTS_WRITER_H_ */
