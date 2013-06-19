#ifndef READ_SDC_H
#define READ_SDC_H

/*********************** Externally-accessible variables **************************/

extern t_timing_constraints * g_sdc;

/*************************** Function declarations ********************************/

void read_sdc(t_timing_inf timing_inf);
void free_sdc_related_structs(void);
void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints);
const char * get_sdc_file_name(); /* Accessor function for getting SDC file name */

#endif
