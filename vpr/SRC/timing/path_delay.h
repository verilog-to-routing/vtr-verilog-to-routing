#ifndef PATH_DELAY
#define PATH_DELAY

#define DO_NOT_ANALYSE -1

/*********************** Defines for timing options *******************************/

#define SLACK_DEFINITION 'R'
/* Choose how to normalize negative slacks for the optimizers (not in the final timing analysis for output statistics):
   'R' (T_req-relaxed): For each constraint, set the required time at sink nodes to the max of the true required time 
	   (constraint + tnode[inode].clock_skew) and the max arrival time. This means that the required time is "relaxed" 
	   to the max arrival time for tight constraints which would otherwise give negative slack.
	   Criticalities are computed once per constraint, using a criticality denominator unique to that constraint
	   (maximum of the constraint and the max arrival time).
   'S' (Shifted): After all slacks are computed, increase the value of all slacks by the largest negative slack, 
       if it exists. Equivalent to 'R' for single-clock cases. 
	   Criticalities are computed once per timing analysis, using a single criticality denominator for all constraints
	   (maximum of all constraints and all required times).
	   This can give unusual results with multiple, very dissimilar constraints.
*/

#ifdef PATH_COUNTING /* Path counting options: */
	#define DISCOUNT_FUNCTION_BASE 100
	/* The base of the exponential discount function used to calculate 
	forward and backward path weights. Higher values discount paths 
	with higher slacks more greatly. */
	
	#define FINAL_DISCOUNT_FUNCTION_BASE DISCOUNT_FUNCTION_BASE
	/* The base of the exponential disount function used to calculate
	path criticality from forward and backward weights. Higher values 
	discount paths with higher slacks more greatly. By default, this 
	is the same as the original discount function base. */
	
	#define PACK_PATH_WEIGHT 1
	#define TIMING_GAIN_PATH_WEIGHT PACK_PATH_WEIGHT
	#define PLACE_PATH_WEIGHT 0
	#define ROUTE_PATH_WEIGHT 0
	/* The percentage of total criticality taken from path criticality 
	as opposed to timing criticality. A value of 0 uses only timing
	criticality; a value of 1 uses only path criticality. */
#endif

/*************************** Function declarations ********************************/

t_slack * alloc_and_load_timing_graph(t_timing_inf timing_inf);

t_slack * alloc_and_load_pre_packing_timing_graph(float block_delay,
		float inter_cluster_net_delay, t_model *models, t_timing_inf timing_inf);

t_linked_int *allocate_and_load_critical_path(void);

void load_timing_graph_net_delays(float **net_delay);

void do_timing_analysis(t_slack * slacks, boolean is_prepacked, boolean do_lut_input_balancing, boolean is_final_analysis);

void free_timing_graph(t_slack * slack);

void free_timing_stats(void);

void print_timing_graph(const char *fname);

void print_lut_remapping(const char *fname);

void print_slack(float ** slack, boolean slack_is_normalized, const char *fname);

void print_criticality(t_slack * slacks, boolean criticality_is_normalized, const char *fname);

void print_net_delay(float **net_delay, const char *fname);

#ifdef PATH_COUNTING
void print_path_criticality(float ** path_criticality, const char *fname);
#else
void print_clustering_timing_info(const char *fname);

boolean has_valid_normalized_T_arr(int inode);
#endif

void print_timing_stats(void);

float get_critical_path_delay(void);

void print_critical_path(const char *fname);

void get_tnode_block_and_output_net(int inode, int *iblk_ptr, int *inet_ptr);

void do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
		float constant_net_delay_value);

void print_timing_graph_as_blif (const char *fname, t_model *models);

/*************************** Variable declarations ********************************/

extern int num_tnodes; /* Number of nodes (pins) in the timing graph */
extern t_tnode *tnode; /* [0..num_tnodes - 1] nodes in the timing graph */

#endif
