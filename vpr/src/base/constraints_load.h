#ifndef CONSTRAINTS_LOAD_H_
#define CONSTRAINTS_LOAD_H_

#include "region.h"
#include "partition.h"
#include "partition_region.h"
#include "vpr_constraints.h"
#include "vtr_vector.h"

///@brief Used to print vpr's floorplanning constraints to an echo file "vpr_constraints.echo"
void echo_constraints(char* filename, VprConstraints constraints);

#endif
