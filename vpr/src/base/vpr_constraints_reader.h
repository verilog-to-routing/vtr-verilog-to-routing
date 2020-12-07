/* Defines the function used to load a vpr constraints file written in XML format into vpr
 * The functions loads up the VprConstraints, Partition, Region, and PartitionRegion data structures
 * according to the data provided in the XML file*/

#ifndef VPR_CONSTRAINTS_READER_H_
#define VPR_CONSTRAINTS_READER_H_

void load_vpr_constraints_file(const char* read_vpr_constraints_name);

#endif /* VPR_CONSTRAINTS_READER_H_ */
