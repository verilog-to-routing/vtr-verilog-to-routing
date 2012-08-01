#ifndef PATH_DELAY
#define PATH_DELAY

#define DO_NOT_ANALYSE -1

/*********************** Defines for timing options *******************************/

#define SLACK_DEFINITION 5
/* Choose whether, and how, to normalize negative slacks for optimization 
  (not for final analysis, since real slacks are always given here).
  Possible values:
   1: Slacks are not normalized at all.  Negative slacks are possible.
   2: All negative slacks are "clipped" to 0.  
   3: If negative slacks exist, increase the value of all slacks by the largest negative slack.  Only the critical path will have 0 slack.
   4: Set the required time to the max of the "real" required time (constraint + tnode[inode].clock_skew) and the arrival time.  Only the critical path will have 0 slack.
   5: As 4, except give normalized slacks in the final analysis as well.
   */

#define SLACK_RATIO_DEFINITION 1
/* Which definition of slack ratio (related to criticality) should VPR use?  In general, slack ratio is slack/maximum delay.  Possible values:
   1: slack ratio = minimum over all traversals of (slack of edge for this traversal)/(maximum required time T_req_max_this_domain for this traversal)
   2: slack ratio = (slack of this edge)/(maximum required time T_req_max_global in design)
   Note that if SLACK_DEFINITION = 4 or 5 above, T_req_max will be taken from normalized required times, not real required times.
*/

/* WARNING: SLACK_DEFINITION 3 and SLACK_RATIO_DEFINITION 1 are not compatible. */

/*************************** Function declarations ********************************/

t_slack * alloc_and_load_timing_graph(t_timing_inf timing_inf);

t_slack * alloc_and_load_pre_packing_timing_graph(float block_delay,
		float inter_cluster_net_delay, t_model *models, t_timing_inf timing_inf);

t_linked_int *allocate_and_load_critical_path(void);

void load_timing_graph_net_delays(float **net_delay);

t_timing_stats * do_timing_analysis(t_slack * slacks, boolean is_prepacked, boolean do_lut_input_balancing, boolean is_final_analysis);

void free_timing_graph(t_slack * slack);

void free_timing_stats(t_timing_stats * timing_stats);

void print_timing_graph(const char *fname);

void print_lut_remapping(const char *fname);

void print_net_slack(float ** net_slack, const char *fname);

void print_net_slack_ratio(float ** net_slack, const char *fname);

void print_net_delay(float **net_delay, const char *fname);

void print_timing_place_crit(float ** timing_place_crit, const char *fname);

#if CLUSTERER_CRITICALITY != 'S'
void print_clustering_timing_info(const char *fname);
#endif

void print_critical_path(const char *fname);

void get_tnode_block_and_output_net(int inode, int *iblk_ptr, int *inet_ptr);

void do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
		float constant_net_delay_value);

void print_timing_graph_as_blif (const char *fname, t_model *models);

/*************************** Variable declarations ********************************/

extern int num_constrained_clocks; /* number of clocks with timing constraints */
extern t_clock * constrained_clocks; /* [0..num_constrained_clocks - 1] array of clocks with timing constraints */

extern int num_constrained_ios; /* number of I/Os with timing constraints */
extern t_io * constrained_ios; /* [0..num_constrained_ios - 1] array of I/Os with timing constraints */

extern float ** timing_constraint; /* [0..num_constrained_clocks - 1 (source)][0..num_constrained_clocks - 1 (destination)] */

extern int num_cf_constraints; /* number of special-case clock-to-flipflop constraints overriding default, calculated, timing constraints */
extern t_cf_constraint * clock_to_ff_constraints; /*  [0..num_cf_constraints - 1] array of such constraints */

extern int num_fc_constraints; /* number of special-case flipflop-to-clock constraints */
extern t_fc_constraint * ff_to_clock_constraints; /*  [0..num_fc_constraints - 1] */

extern int num_ff_constraints; /* number of special-case flipflop-to-flipflop constraints */
extern t_ff_constraint * ff_to_ff_constraints; /*  [0..num_ff_constraints - 1] array of such constraints */

#endif
