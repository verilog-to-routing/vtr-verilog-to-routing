#ifndef READ_SDC_H
#define READ_SDC_H

void read_sdc(char * sdc_file);
void free_sdc_related_structs(void);
void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints);

#endif
