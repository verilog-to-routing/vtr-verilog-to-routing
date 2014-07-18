#ifndef PATH_DELAY
#define PATH_DELAY

#define DO_NOT_ANALYSE -1

/*********************** Defines for timing options *******************************/

#define SLACK_DEFINITION 'R'
/* Choose how to normalize negative slacks for the optimizers (not in the final timing analysis for output statistics):
   'R' (T_req-relaxed): For each constraint, set the required time at sink nodes to the max of the true required time 
	   (constraint + tnode[inode].clock_skew) and the max arrival time. This means that the required time is "relaxed" 
	   to the max arrival time for tight constraints which would otherwise give negative slack.
   'I' (Improved Shifted): After all slacks are computed, increase the value of all slacks by the magnitude of the 
	   largest negative slack, if it exists. More computationally demanding. Equivalent to 'R' for a single clock. 
   'S' (Shifted): Same as improved shifted, but criticalities are only computed after all traversals.  Equivalent to 'R'
	   for a single clock.
   'G' (Global T_req-relaxed): Same as T_req-relaxed, but criticalities are only computed after all traversals.  
	   Equivalent to 'R' for a single clock.  Note: G is a global version of R, just like S is a global version of I.
   'C' (Clipped): All negative slacks are clipped to 0. 
   'N' (None): Negative slacks are not normalized. 

   This definition also affects how criticality is computed.  For all methods except 'S', the criticality denominator 
   is the maximum required time for each constraint, and criticality is updated after each constraint.  'S' only updates
   criticalities once after all traversals, and the criticality denominator is the maximum required time over all 
   traversals.  

   Because the criticality denominator must be >= the largest slack it is used on, and slack normalization affects the
   size of the largest slack, the denominator must also be normalized.  We choose the following normalization methods:

   'R': Denominator is already taken care of because the maximum required time now depends on the constraint. No further
		normalization is necessary.
   'I': Denominator is increased by the magnitude of the largest negative slack.
   'S': Denominator is the maximum of the 'I' denominator over all constraints.
   'G': Denominator is the maximum of the 'R' denominator over all constraints.
   'C': Denominator is unchanged.  However, if Treq_max is 0, there is division by 0.  To avoid this, note that in this
		case, all of the slacks will be clipped to zero anyways, so we can just set the criticality to 1. 
   'N': Denominator is set to max(max_Treq, max_Tarr), so that the magnitude of criticality will at least be bounded to 
		2.  This is the same denominator as 'R', though calculated differently.
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

void do_timing_analysis(t_slack * slacks, boolean is_prepacked, boolean is_final_analysis);

void free_timing_graph(t_slack * slack);

void free_timing_stats(void);

void print_timing_graph(const char *fname);

void print_lut_remapping(const char *fname);

void print_slack(float ** slack, boolean slack_is_normalized, const char *fname);

void print_criticality(t_slack * slacks, const char *fname);

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
