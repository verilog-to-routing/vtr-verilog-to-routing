#ifndef READ_SDC_H
#define READ_SDC_H
#include "timing_constraints_fwd.hpp"
#include "timing_graph_fwd.hpp"
#include <set>

/*********************** Externally-accessible variables **************************/

extern t_timing_constraints * g_sdc;

/*************************** Function declarations ********************************/

void read_sdc(t_timing_inf timing_inf);
void free_sdc_related_structs(void);
void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints);
const char * get_sdc_file_name(); /* Accessor function for getting SDC file name */

tatum::TimingConstraints create_timing_constraints(const AtomNetlist& netlist, const AtomMap& atom_map, t_timing_inf timing_inf);

#endif
