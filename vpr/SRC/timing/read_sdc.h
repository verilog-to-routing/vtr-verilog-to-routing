#ifndef READ_SDC_H
#define READ_SDC_H

/*************************** Function declarations ********************************/

void read_sdc(char * sdc_file);
void free_sdc_related_structs(void);
void free_override_constraint(t_override_constraint *& constraint_array, int num_constraints);

/*************************** Variable declarations ********************************/

extern int g_num_constrained_clocks; /* number of clocks with timing constraints */
extern t_clock * g_constrained_clocks; /* [0..g_num_constrained_clocks - 1] array of clocks with timing constraints */

extern int g_num_constrained_inputs; /* number of inputs with timing constraints */
extern t_io * g_constrained_inputs; /* [0..g_num_constrained_inputs - 1] array of inputs with timing constraints */

extern int g_num_constrained_outputs; /* number of outputs with timing constraints */
extern t_io * g_constrained_outputs; /* [0..g_num_constrained_outputs - 1] array of outputs with timing constraints */

extern float ** g_timing_constraint; /* [0..g_num_constrained_clocks - 1 (source)][0..g_num_constrained_clocks - 1 (sink)] */

extern int g_num_cc_constraints; /* number of special-case clock-to-clock constraints overriding default, calculated, timing constraints */
extern t_override_constraint * g_cc_constraints; /*  [0..g_num_cc_constraints - 1] array of such constraints */

extern int g_num_cf_constraints; /* number of special-case clock-to-flipflop constraints */
extern t_override_constraint * g_cf_constraints; /*  [0..g_num_cf_constraints - 1] array of such constraints */

extern int g_num_fc_constraints; /* number of special-case flipflop-to-clock constraints */
extern t_override_constraint * g_fc_constraints; /*  [0..g_num_fc_constraints - 1] */

extern int g_num_ff_constraints; /* number of special-case flipflop-to-flipflop constraints */
extern t_override_constraint * g_ff_constraints; /*  [0..g_num_ff_constraints - 1] array of such constraints */

#endif
