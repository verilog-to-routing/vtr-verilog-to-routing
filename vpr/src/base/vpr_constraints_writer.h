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
 *
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

void setup_vpr_floorplan_constraints_halves(VprConstraints& constraints);

void setup_vpr_floorplan_constraints_quadrants(VprConstraints& constraints);

void setup_vpr_floorplan_constraints_sixteenths(VprConstraints& constraints);

void create_partition(Partition& part, std::string part_name, int xmin, int ymin, int xmax, int ymax);

#endif /* VPR_SRC_BASE_VPR_CONSTRAINTS_WRITER_H_ */
