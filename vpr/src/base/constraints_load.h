#ifndef CONSTRAINTS_LOAD_H_
#define CONSTRAINTS_LOAD_H_

#include "region.h"
#include "partition.h"
#include "partition_region.h"
#include "vpr_constraints.h"
#include "vtr_vector.h"

void echo_constraints(char* filename, VprConstraints constraints, int num_parts);

#endif
