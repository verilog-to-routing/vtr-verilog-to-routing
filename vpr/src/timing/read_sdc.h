#ifndef READ_SDC_H
#define READ_SDC_H
#include <memory>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"

/*********************** Externally-accessible variables **************************/

/*************************** Function declarations ********************************/

void read_sdc(t_timing_inf timing_inf);
void free_sdc_related_structs();
void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints);
const char * get_sdc_file_name(); /* Accessor function for getting SDC file name */

std::unique_ptr<tatum::TimingConstraints> create_timing_constraints(const AtomNetlist& netlist, const AtomLookup& atom_lookup, t_timing_inf timing_inf);

#endif
