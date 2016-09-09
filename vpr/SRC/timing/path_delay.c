#include <cstring>
#include <climits>
#include <cmath>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "net_delay.h"
#include "vpr_utils.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"
#include "read_sdc.h"
#include "stats.h"
#include <time.h>

/**************************** Top-level summary ******************************

Timing analysis by Vaughn Betz, Jason Luu, and Michael Wainberg.

Timing analysis is a three-step process:
1. Interpret the constraints specified by the user in an SDC (Synopsys Design
Constraints) file or, if none is specified, use default constraints.
2. Convert the pre- or post-packed netlist (depending on the stage of the 
flow) into a timing graph, where nodes represent pins and edges represent 
dependencies and delays between pins (see "Timing graph structure", below).
3. Traverse the timing graph to obtain information about which connections 
to optimize, as well as statistics for the user.

Steps 1 and 2 are performed through one of two helper functions:
alloc_and_load_timing_graph and alloc_and_load_pre_packing_timing_graph.
The first step is to create the timing graph, which is stored in the array 
tnode ("timing node").  This is done through alloc_and_load_tnodes (post-
packed) or alloc_and_load_tnodes_from_prepacked_netlist. Then, the timing 
graph is topologically sorted ("levelized") in 
alloc_and_load_timing_graph_levels, to allow for faster traversals later.

read_sdc reads the SDC file, interprets its contents and stores them in 
the data structure g_sdc.  (This data structure does not need to remain 
global but it is probably easier, since its information is used in both
netlists and only needs to be read in once.)

load_clock_domain_and_clock_and_io_delay then gives each flip-flop and I/O
the index of a constrained clock from the SDC file in g_sdc->constrained_
clocks, or -1 if an I/O is unconstrained.

process_constraints does a pre-traversal through the timing graph and prunes 
all constraints between domains that never intersect so they are not analysed.

Step 3 is performed through do_timing_analysis.  For each constraint between 
a pair of clock domains we do a forward traversal from the "source" domain
to the "sink" domain to compute each tnode's arrival time, the time when the 
latest signal would arrive at the node.  We also do a backward traversal to 
compute required time, the time when the earliest signal has to leave the 
node to meet the constraint.  The "slack" of each sink pin on each net is 
basically the difference between the two times. 

If path counting is on, we calculate a forward and backward path weight in 
do_path_counting.  These represent the importance of paths fanning, 
respectively, into and out of this pin, in such a way that paths with a 
higher slack are discounted exponentially in importance.

If path counting is off and we are using the pre-packed netlist, we also 
calculate normalized costs for the clusterer (normalized arrival time, slack 
and number of critical paths) in normalized_costs. The clusterer uses these 
to calculate a criticality for each block.

Finally, in update_slacks, we calculate the slack for each sink pin on each net
for printing, as well as a derived metric, timing criticality, which the 
optimizers actually use. If path counting is on, we calculate a path criticality
from the forward and backward weights on each tnode. */

/************************* Timing graph structure ****************************

Author:  V. Betz

We can build timing graphs that match either the primitive (logical_block) 
netlist (of basic elements before clustering, like FFs and LUTs) or that 
match the clustered netlist (block).  You pass in the is_pre_packed flag to 
say which kind of netlist and timing graph you are working with.
                                                                                                                                           
Every used (not OPEN) block pin becomes a timing node, both on primitive
blocks and (if you are building the timing graph that matches a clustered 
netlist) on clustered blocks.  For the clustered (not pre_packed) timing
graph, every used pin within a clustered (pb_type) block also becomes a timing
node.  So a CLB that contains ALMs that contains LUTs will have nodes created 
for the CLB pins, ALM pins and LUT pins, with edges that connect them as 
specified in the clustered netlist.  Unused (OPEN) pins don't create any 
timing nodes. 

The primitive blocks have edges from the tnodes that represent input pins and
output pins that represent the timing dependencies within these lowest level 
blocks (e.g. LUTs and FFs and IOs).  The exact edges created come from reading
the architecture file.  A LUT for example will have delays specified from all
its inputs to its output, and hence we will create a tedge from each tnode 
corresponding to an input pin of the LUT to the tnode that represents the LUT
output pin.  The delay marked on each edge is read from the architecture file.

A FF has nodes representing its pins, but also two extra nodes, representing 
the TN_FF_SINK and TN_FF_SOURCE.  Those two nodes represent the internal storage 
node of the FF.  The TN_FF_SINK has edges going to it from the regular FF inputs
(but not the clock input), with appropriate delays.  It has no outgoing edges,
since it terminates timing paths. The TN_FF_SOURCE has an edge going from it to
the FF output pin, with the appropriate delay.  TN_FF_SOURCES have no edges; they
start timing paths.  FF clock pins have no outgoing edges, but the delay to 
them can be looked up, and is used in the timing traversals to compute clock 
delay for slack computations. 

In the timing graph I create, input pads and constant generators have no   
inputs (incoming edges), just like TN_FF_SOURCES.  Every input pad and output 
pad is represented by two tnodes -- an input pin/source and an output pin/
sink.  For an input pad the input source comes from off chip and has no fanin, 
while the output pin drives signals within the chip.  For output pads, the 
input pin is driven by signal (net) within the chip, and the output sink node 
goes off chip and has no fanout (out-edges).  I need two nodes to respresent 
things like pads because I mark all delay on tedges, not on tnodes.                         
                                                                          
One other subblock that needs special attention is a constant generator.   
This has no used inputs, but its output is used.  I create an extra tnode, 
a dummy input, in addition to the output pin tnode.  The dummy tnode has   
no fanin.  Since constant generators really generate their outputs at T =  
-infinity, I set the delay from the input tnode to the output to a large-  
magnitude negative number.  This guarantees every block that needs the     
output of a constant generator sees it available very early.               
                                                                           
The main structure of the timing graph is given by the nodes and the edges 
that connect them.  We also store some extra information on tnodes that
(1) lets us figure out how to map from a tnode back to the netlist pin (or 
other item) it represents and (2) figure out what clock domain it is on, if 
it is a timing path start point (no incoming edges) or end point (no outgoing
edges) and (3) lets us figure out the delay of the clock to that node, if it 
is a timing path start or end point.

To map efficiently from tedges back to the netlist pins, we create the tedge 
array driven by a tnode the represents a netlist output pin *in the same order 
as the netlist net pins*.  That means the edge index for the tedge array from 
such a tnode guarantees iedge = net_pin_index  1.  The code to map slacks 
from the timing graph back to the netlist relies on this. */

/*************************** Global variables *******************************/ 

t_tnode *tnode = NULL; /* [0..num_tnodes - 1] */
int num_tnodes = 0; /* Number of nodes (pins) in the timing graph */
clock_t timing_analysis_runtime = 0;

/******************** Variables local to this module ************************/

#define NUM_BUCKETS 5 /* Used when printing slack and criticality. */

/* Variables for "chunking" the tedge memory.  If the head pointer in tedge_ch is NULL, *
 * no timing graph exists now.															*/

static vtr::t_chunk tedge_ch = {NULL, 0, NULL};

static vector<t_vnet> *timing_nets = NULL;

static int num_timing_nets = 0;

static t_timing_stats * f_timing_stats = NULL; /* Critical path delay and worst-case slack per constraint. */

static int * f_net_to_driver_tnode; 
/* [0..net.size() - 1]. Gives the index of the tnode that drives each net. 
Used for both pre- and post-packed netlists. If you just want the number
of edges on the driver tnode, use:
	num_edges = timing_nets[inet].num_sinks;
instead of the equivalent but more convoluted:
	driver_tnode = f_net_to_driver_tnode[inet];
	num_edges = tnode[driver_tnode].num_edges;
Only use this array if you want the actual edges themselves or the index 
of the driver tnode. */

/***************** Subroutines local to this module *************************/

static t_slack * alloc_slacks(void);

static void update_slacks(t_slack * slacks, int source_clock_domain, int sink_clock_domain, float criticality_denom,
    bool update_slack, bool is_final_analysis, float smallest_slack_in_design, const t_timing_inf &timing_inf);

static void alloc_and_load_tnodes(const t_timing_inf &timing_inf);

static void alloc_and_load_tnodes_from_prepacked_netlist(float block_delay,
		float inter_cluster_net_delay);

static void alloc_timing_stats(void);

static float do_timing_analysis_for_constraint(int source_clock_domain, int sink_clock_domain, 
	bool is_prepacked, bool is_final_analysis, long * max_critical_input_paths_ptr, 
    long * max_critical_output_paths_ptr, t_pb ***pin_id_to_pb_mapping, const t_timing_inf &timing_inf);

#ifdef PATH_COUNTING
static void do_path_counting(float criticality_denom);
#endif

static float find_least_slack(bool is_prepacked, t_pb ***pin_id_to_pb_mapping);

static void load_tnode(t_pb_graph_pin *pb_graph_pin, const int iblock,
		int *inode, const t_timing_inf timing_inf);

#ifndef PATH_COUNTING
static void update_normalized_costs(float T_arr_max_this_domain, long max_critical_input_paths,
    long max_critical_output_paths, const t_timing_inf &timing_inf);
#endif

//static void print_primitive_as_blif(FILE *fpout, int iblk, int **lookup_tnode_from_pin_id);

static void load_clock_domain_and_clock_and_io_delay(bool is_prepacked, int **lookup_tnode_from_pin_id, t_pb*** pin_id_to_pb_mapping);

static char * find_tnode_net_name(int inode, bool is_prepacked, t_pb*** pin_id_to_pb_mapping);

static t_tnode * find_ff_clock_tnode(int inode, bool is_prepacked, int **lookup_tnode_from_pin_id);

static inline int get_tnode_index(t_tnode * node);

static inline bool has_valid_T_arr(int inode);

static inline bool has_valid_T_req(int inode);

static int find_clock(char * net_name);

static int find_input(char * net_name);

static int find_output(char * net_name);

static int find_cf_constraint(char * source_clock_name, char * sink_ff_name);

static void propagate_clock_domain_and_skew(int inode);

static void process_constraints(void);

static void print_global_criticality_stats(FILE * fp, float ** criticality, const char * singular_name, const char * capitalized_plural_name);

static void print_timing_constraint_info(const char *fname);

static void print_spaces(FILE * fp, int num_spaces);

/********************* Subroutine definitions *******************************/

t_slack * alloc_and_load_timing_graph(t_timing_inf timing_inf) {

	/* This routine builds the graph used for timing analysis.  Every cb pin is a 
	 * timing node (tnode).  The connectivity between pins is					*
	 * represented by timing edges (tedges).  All delay is marked on edges, not *
	 * on nodes.  Returns two arrays that will store slack values:				*
	 * slack and criticality ([0..net.size()-1][1..num_pins]).           */

	/*  For pads, only the first two pin locations are used (input to pad is first,
	 * output of pad is second).  For CLBs, all OPEN pins on the cb have their 
	 * mapping set to OPEN so I won't use it by mistake.                          */

	int num_sinks;
	t_slack * slacks = NULL;
	bool do_process_constraints = false;
	int ** lookup_tnode_from_pin_id;
	t_pb*** pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();
	
	if (tedge_ch.chunk_ptr_head != NULL) {
		vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, 
				"in alloc_and_load_timing_graph: An old timing graph still exists.\n");
	}

	num_timing_nets = (int) g_clbs_nlist.net.size();
	timing_nets = &g_clbs_nlist.net;

	alloc_and_load_tnodes(timing_inf);

    detect_and_fix_timing_graph_combinational_loops();

	num_sinks = alloc_and_load_timing_graph_levels();

	check_timing_graph(num_sinks);

	slacks = alloc_slacks();

	if (g_sdc == NULL) {
		/* the SDC timing constraints only need to be read in once; *
		 * if they haven't been already, do it now				    */
		read_sdc(timing_inf);
		do_process_constraints = true;
	}

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();
		
	load_clock_domain_and_clock_and_io_delay(false, lookup_tnode_from_pin_id, pin_id_to_pb_mapping);

	if (do_process_constraints) 
		process_constraints();
	
	if (f_timing_stats == NULL)
		alloc_timing_stats();

	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);
	free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);

	return slacks;
}

t_slack * alloc_and_load_pre_packing_timing_graph(float block_delay,
		float inter_cluster_net_delay, t_model *models, t_timing_inf timing_inf) {

	/* This routine builds the graph used for timing analysis.  Every technology-
	 * mapped netlist pin is a timing node (tnode).  The connectivity between pins is *
	 * represented by timing edges (tedges).  All delay is marked on edges, not *
	 * on nodes.  Returns two arrays that will store slack values:				 *
	 * slack and criticality ([0..net.size()-1][1..num_pins]).           */

	/*  For pads, only the first two pin locations are used (input to pad is first,
	 * output of pad is second).  For CLBs, all OPEN pins on the cb have their 
	 * mapping set to OPEN so I won't use it by mistake.                          */
	
	int num_sinks;
	t_slack * slacks = NULL;
	bool do_process_constraints = false;
	int **lookup_tnode_from_pin_id;
	t_pb***pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();
	
	if (tedge_ch.chunk_ptr_head != NULL) {
		vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__, 
				"in alloc_and_load_timing_graph: An old timing graph still exists.\n");
	}

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();

	num_timing_nets = (int) g_atoms_nlist.net.size();
	timing_nets = &g_atoms_nlist.net;

	alloc_and_load_tnodes_from_prepacked_netlist(block_delay,
			inter_cluster_net_delay);

    detect_and_fix_timing_graph_combinational_loops();

	num_sinks = alloc_and_load_timing_graph_levels();

	slacks = alloc_slacks();

	check_timing_graph(num_sinks);

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH_AS_BLIF)) {
		print_timing_graph_as_blif(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH_AS_BLIF), models);
	}
	
	if (g_sdc == NULL) {
		/* the SDC timing constraints only need to be read in once; *
		 * if they haven't been already, do it now				    */
		read_sdc(timing_inf);
		do_process_constraints = true;
	}
	
	load_clock_domain_and_clock_and_io_delay(true, lookup_tnode_from_pin_id, pin_id_to_pb_mapping);

	if (do_process_constraints) 
		process_constraints();
	
	if (f_timing_stats == NULL)
		alloc_timing_stats();

	free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);
	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);

	return slacks;
}

static t_slack * alloc_slacks(void) {

	/* Allocates the slack, criticality and path_criticality structures 
	([0..net.size()-1][1..num_pins-1]). Chunk allocated to save space. */

	int inet;
	vector<t_vnet> & tnets = *timing_nets; 
	t_slack * slacks = (t_slack *) vtr::malloc(sizeof(t_slack));
	
	slacks->slack   = (float **) vtr::malloc(num_timing_nets * sizeof(float *));
	slacks->timing_criticality = (float **) vtr::malloc(num_timing_nets * sizeof(float *));
#ifdef PATH_COUNTING
	slacks->path_criticality = (float **) vtr::malloc(num_timing_nets * sizeof(float *));
#endif
	for (inet = 0; inet < num_timing_nets; inet++) {
		slacks->slack[inet]	  = (float *) vtr::chunk_malloc(tnets[inet].pins.size() * sizeof(float), &tedge_ch);
		slacks->timing_criticality[inet] = (float *) vtr::chunk_malloc(tnets[inet].pins.size() * sizeof(float), &tedge_ch);
#ifdef PATH_COUNTING
		slacks->path_criticality[inet] = (float *) vtr::chunk_malloc(tnets[inet].pins.size() * sizeof(float), &tedge_ch);
#endif
	}

	return slacks;
}

void load_timing_graph_net_delays(float **net_delay) {

	/* Sets the delays of the inter-CLB nets to the values specified by          *
	 * net_delay[0..net.size()-1][1..num_pins-1].  These net delays should have    *
	 * been allocated and loaded with the net_delay routines.  This routine      *
	 * marks the corresponding edges in the timing graph with the proper delay.  */

	int inet, inode;
	unsigned ipin;
	vector<t_vnet> & tnets = *timing_nets; 
	t_tedge *tedge;

	for (inet = 0; inet < num_timing_nets; inet++) {
		inode = f_net_to_driver_tnode[inet];
		tedge = tnode[inode].out_edges;

		/* Note that the edges of a tnode corresponding to a CLB or INPAD opin must  *
		 * be in the same order as the pins of the net driven by the tnode.          */

		for (ipin = 1; ipin < tnets[inet].pins.size(); ipin++)
			tedge[ipin - 1].Tdel = net_delay[inet][ipin];
	}
}

void free_timing_graph(t_slack * slacks) {

	int inode;

	if (tedge_ch.chunk_ptr_head == NULL) {
		vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__, 
				"in free_timing_graph: No timing graph to free.\n");
	}

	free_chunk_memory(&tedge_ch);

	if (tnode[0].prepacked_data) {
		/* If we allocated prepacked_data for the first node, 
		it must be allocated for all other nodes too. */
		for (inode = 0; inode < num_tnodes; inode++) {
			free(tnode[inode].prepacked_data);
		}
	}
	free(tnode);
	free(f_net_to_driver_tnode);
	free_ivec_vector(tnodes_at_level, 0, num_tnode_levels - 1);

	free(slacks->slack);
	free(slacks->timing_criticality);
#ifdef PATH_COUNTING
	free(slacks->path_criticality);
#endif
	free(slacks);
	
	tnode = NULL;
	num_tnodes = 0;
	f_net_to_driver_tnode = NULL;
	tnodes_at_level = NULL;
	num_tnode_levels = 0;
	slacks = NULL;
}

void free_timing_stats(void) {
	int i;
	if(f_timing_stats != NULL) {
		for (i = 0; i < g_sdc->num_constrained_clocks; i++) {
			free(f_timing_stats->cpd[i]);
			free(f_timing_stats->least_slack[i]);
		}
		free(f_timing_stats->cpd);
		free(f_timing_stats->least_slack);
		free(f_timing_stats);
	}
	f_timing_stats = NULL;
}

void print_slack(float ** slack, bool slack_is_normalized, const char *fname) {

	/* Prints slacks into a file. */

	int inet, iedge, ibucket, driver_tnode, num_edges, num_unused_slacks = 0;
	t_tedge * tedge;
	FILE *fp;
	float max_slack = HUGE_NEGATIVE_FLOAT, min_slack = HUGE_POSITIVE_FLOAT, 
		total_slack = 0, total_negative_slack = 0, bucket_size, slk;
	int slacks_in_bucket[NUM_BUCKETS]; 

	vector<t_vnet> & tnets = *timing_nets; 

	fp = vtr::fopen(fname, "w");

	if (slack_is_normalized) {
		fprintf(fp, "The following slacks have been normalized to be non-negative by "
					"relaxing the required times to the maximum arrival time.\n\n");
	}

	/* Go through slack once to get the largest and smallest slack, both for reporting and 
	   so that we can delimit the buckets. Also calculate the total negative slack in the design. */
	for (inet = 0; inet < num_timing_nets; inet++) {
		num_edges = tnets[inet].num_sinks();
		for (iedge = 0; iedge < num_edges; iedge++) { 
			slk = slack[inet][iedge + 1];
			if (slk < HUGE_POSITIVE_FLOAT - 1) { /* if slack was analysed */
				max_slack = max(max_slack, slk);
				min_slack = min(min_slack, slk);
				total_slack += slk;
				if (slk < NEGATIVE_EPSILON) { 
					total_negative_slack -= slk; /* By convention, we'll have total_negative_slack be a positive number. */
				}
			} else { /* slack was never analysed */
				num_unused_slacks++;
			}
		}
	}

	if (max_slack > HUGE_NEGATIVE_FLOAT + 1) {
		fprintf(fp, "Largest slack in design: %g\n", max_slack);
	} else {
		fprintf(fp, "Largest slack in design: --\n");
	}
	
	if (min_slack < HUGE_POSITIVE_FLOAT - 1) {
		fprintf(fp, "Smallest slack in design: %g\n", min_slack);
	} else {
		fprintf(fp, "Smallest slack in design: --\n");
	}

	fprintf(fp, "Total slack in design: %g\n", total_slack);
	fprintf(fp, "Total negative slack: %g\n", total_negative_slack);

	if (max_slack - min_slack > EPSILON) { /* Only sort the slacks into buckets if not all slacks are the same (if they are identical, no need to sort). */
		/* Initialize slacks_in_bucket, an array counting how many slacks are within certain linearly-spaced ranges (buckets). */
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			slacks_in_bucket[ibucket] = 0;
		}

		/* The size of each bucket is the range of slacks, divided by the number of buckets. */
		bucket_size = (max_slack - min_slack)/NUM_BUCKETS;

		/* Now, pass through again, incrementing the number of slacks in the nth bucket 
			for each slack between (min_slack + n*bucket_size) and (min_slack + (n+1)*bucket_size). */

		for (inet = 0; inet < num_timing_nets; inet++) {
			num_edges = tnets[inet].num_sinks();
			for (iedge = 0; iedge < num_edges; iedge++) { 
				slk = slack[inet][iedge + 1];
				if (slk < HUGE_POSITIVE_FLOAT - 1) {
					/* We have to watch out for the special case where slack = max_slack, in which case ibucket = NUM_BUCKETS and we go out of bounds of the array. */
					ibucket = min(NUM_BUCKETS - 1, (int) ((slk - min_slack)/bucket_size));
					VTR_ASSERT(ibucket >= 0 && ibucket < NUM_BUCKETS);
					slacks_in_bucket[ibucket]++;
				}
			}
		}

		/* Now print how many slacks are in each bucket. */
		fprintf(fp, "\n\nRange\t\t");
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			fprintf(fp, "%.1e to ", min_slack);
			min_slack += bucket_size;
			fprintf(fp, "%.1e\t", min_slack);
		}
		fprintf(fp, "Not analysed");
		fprintf(fp, "\nSlacks in range\t\t");
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			fprintf(fp, "%d\t\t\t", slacks_in_bucket[ibucket]);
		}
		fprintf(fp, "%d", num_unused_slacks);
	}

	/* Finally, print all the slacks, organized by net. */
	fprintf(fp, "\n\nNet #\tDriver_tnode\tto_node\tSlack\n\n");

	for (inet = 0; inet < num_timing_nets; inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		num_edges = tnode[driver_tnode].num_edges;
		tedge = tnode[driver_tnode].out_edges;
		slk = slack[inet][1];
		if (slk < HUGE_POSITIVE_FLOAT - 1) {
			fprintf(fp, "%5d\t%5d\t\t%5d\t%g\n", inet, driver_tnode, tedge[0].to_node, slk);
		} else { /* Slack is meaningless, so replace with --. */
			fprintf(fp, "%5d\t%5d\t\t%5d\t--\n", inet, driver_tnode, tedge[0].to_node);
		}
		for (iedge = 1; iedge < num_edges; iedge++) { /* newline and indent subsequent edges after the first */
			slk = slack[inet][iedge+1];
			if (slk < HUGE_POSITIVE_FLOAT - 1) {
				fprintf(fp, "\t\t\t%5d\t%g\n", tedge[iedge].to_node, slk);
			} else { /* Slack is meaningless, so replace with --. */
				fprintf(fp, "\t\t\t%5d\t--\n", tedge[iedge].to_node);
			}
		}
	}

	fclose(fp);
}

void print_criticality(t_slack * slacks, const char *fname) {

	/* Prints timing criticalities (and path criticalities if enabled) into a file. */

	int inet, iedge, driver_tnode, num_edges;
	t_tedge * tedge;
	FILE *fp;

	fp = vtr::fopen(fname, "w");

	print_global_criticality_stats(fp, slacks->timing_criticality, "timing criticality", "Timing criticalities");
#ifdef PATH_COUNTING
	print_global_criticality_stats(fp, slacks->path_criticality, "path criticality", "Path criticalities");
#endif

	/* Finally, print all the criticalities, organized by net. */
	fprintf(fp, "\n\nNet #\tDriver_tnode\t to_node\tTiming criticality"
#ifdef PATH_COUNTING
		"\tPath criticality"
#endif
		"\n");

	for (inet = 0; inet < num_timing_nets; inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		num_edges = tnode[driver_tnode].num_edges;
		tedge = tnode[driver_tnode].out_edges;

		fprintf(fp, "\n%5d\t%5d\t\t%5d\t\t%.6f", inet, driver_tnode, tedge[0].to_node, slacks->timing_criticality[inet][1]);
#ifdef PATH_COUNTING
		fprintf(fp, "\t\t%g", slacks->path_criticality[inet][1]);
#endif
		for (iedge = 1; iedge < num_edges; iedge++) { /* newline and indent subsequent edges after the first */
			fprintf(fp, "\n\t\t\t%5d\t\t%.6f", tedge[iedge].to_node, slacks->timing_criticality[inet][iedge+1]);
#ifdef PATH_COUNTING
			fprintf(fp, "\t\t%g", slacks->path_criticality[inet][iedge+1]);
#endif
		}
	}

	fclose(fp);
}

static void print_global_criticality_stats(FILE * fp, float ** criticality, const char * singular_name, const char * capitalized_plural_name) {
	
	/* Prints global stats for timing or path criticality to the file pointed to by	fp,
	including maximum criticality, minimum criticality, total criticality in the design,
	and the number of criticalities within various ranges, or buckets. */

	int inet, iedge, num_edges, ibucket, criticalities_in_bucket[NUM_BUCKETS];
	float crit, max_criticality = HUGE_NEGATIVE_FLOAT, min_criticality = HUGE_POSITIVE_FLOAT, 
		total_criticality = 0, bucket_size;
	vector<t_vnet> & tnets = *timing_nets; 

	/* Go through criticality once to get the largest and smallest timing criticality, 
	both for reporting and so that we can delimit the buckets. */
	for (inet = 0; inet < num_timing_nets; inet++) {
		num_edges = tnets[inet].num_sinks();
		for (iedge = 0; iedge < num_edges; iedge++) { 
			crit = criticality[inet][iedge + 1];
			max_criticality = max(max_criticality, crit);
			min_criticality = min(min_criticality, crit);
			total_criticality += crit;
		}
	}

	fprintf(fp, "Largest %s in design: %g\n", singular_name, max_criticality);
	fprintf(fp, "Smallest %s in design: %g\n", singular_name, min_criticality);
	fprintf(fp, "Total %s in design: %g\n", singular_name, total_criticality);

	if (max_criticality - min_criticality > EPSILON) { /* Only sort the criticalities into buckets if not all criticalities are the same (if they are identical, no need to sort). */
		/* Initialize criticalities_in_bucket, an array counting how many criticalities are within certain linearly-spaced ranges (buckets). */
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			criticalities_in_bucket[ibucket] = 0;
		}

		/* The size of each bucket is the range of criticalities, divided by the number of buckets. */
		bucket_size = (max_criticality - min_criticality)/NUM_BUCKETS;

		/* Now, pass through again, incrementing the number of criticalities in the nth bucket 
			for each criticality between (min_criticality + n*bucket_size) and (min_criticality + (n+1)*bucket_size). */

		for (inet = 0; inet < num_timing_nets; inet++) {
			num_edges = tnets[inet].num_sinks();
			for (iedge = 0; iedge < num_edges; iedge++) { 
				crit = criticality[inet][iedge + 1];
				/* We have to watch out for the special case where criticality = max_criticality, in which case ibucket = NUM_BUCKETS and we go out of bounds of the array. */
				ibucket = min(NUM_BUCKETS - 1, (int) ((crit - min_criticality)/bucket_size));
				VTR_ASSERT(ibucket >= 0 && ibucket < NUM_BUCKETS);
				criticalities_in_bucket[ibucket]++;
			}
		}

		/* Now print how many criticalities are in each bucket. */
		fprintf(fp, "\nRange\t\t");
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			fprintf(fp, "%.1e to ", min_criticality);
			min_criticality += bucket_size;
			fprintf(fp, "%.1e\t", min_criticality);
		}
		fprintf(fp, "\n%s in range\t\t", capitalized_plural_name);
		for (ibucket = 0; ibucket < NUM_BUCKETS; ibucket++) {
			fprintf(fp, "%d\t\t\t", criticalities_in_bucket[ibucket]);
		}
	}
	fprintf(fp, "\n\n");
}

void print_net_delay(float **net_delay, const char *fname) {

	/* Prints the net delays into a file. */

	int inet, iedge, driver_tnode, num_edges;
	t_tedge * tedge;
	FILE *fp;

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "Net #\tDriver_tnode\tto_node\tDelay\n\n");

	for (inet = 0; inet < num_timing_nets; inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		num_edges = tnode[driver_tnode].num_edges;
		tedge = tnode[driver_tnode].out_edges;
		fprintf(fp, "%5d\t%5d\t\t%5d\t%g\n", inet, driver_tnode, tedge[0].to_node, net_delay[inet][1]);
		for (iedge = 1; iedge < num_edges; iedge++) { /* newline and indent subsequent edges after the first */
			fprintf(fp, "\t\t\t%5d\t%g\n", tedge[iedge].to_node, net_delay[inet][iedge+1]);
		}
	}

	fclose(fp);
}

#ifndef PATH_COUNTING
void print_clustering_timing_info(const char *fname) {
	/* Print information from tnodes which is used by the clusterer. */
	int inode;
	FILE *fp;

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "inode  ");
	if (g_sdc->num_constrained_clocks <= 1) {
		/* These values are from the last constraint analysed, 
		so they're not meaningful unless there was only one constraint. */
		fprintf(fp, "Critical input paths  Critical output paths  ");
	}
	fprintf(fp, "Normalized slack  Normalized Tarr  Normalized total crit paths\n");
	for (inode = 0; inode < num_tnodes; inode++) {
		fprintf(fp, "%d\t", inode);
		/* Only print normalized values for tnodes which have valid normalized values. (If normalized_T_arr is valid, the others will be too.) */
		if (has_valid_normalized_T_arr(inode)) {
			if (g_sdc->num_constrained_clocks <= 1) {
				fprintf(fp, "%ld\t\t\t%ld\t\t\t", tnode[inode].prepacked_data->num_critical_input_paths, tnode[inode].prepacked_data->num_critical_output_paths); 
			}
			fprintf(fp, "%f\t%f\t%f\n", tnode[inode].prepacked_data->normalized_slack, tnode[inode].prepacked_data->normalized_T_arr, tnode[inode].prepacked_data->normalized_total_critical_paths);
		} else {
			if (g_sdc->num_constrained_clocks <= 1) {
				fprintf(fp, "--\t\t\t--\t\t\t"); 
			}
			fprintf(fp, "--\t\t--\t\t--\n");
		}
	}

	fclose(fp);
}
#endif
/* Count # of tnodes, allocates space, and loads the tnodes and its associated edges */
static void alloc_and_load_tnodes(const t_timing_inf &timing_inf) {
	int i, j, k;
	int inode;
	int num_nodes_in_block;
	int count;
	int iblock, i_pin_id;
	int inet, dport, dpin, dblock, dnode;
	int normalized_pin, normalization;
	t_pb_graph_pin *ipb_graph_pin;
	t_pb_route *intra_lb_route, *d_intra_lb_route;
	int num_dangling_pins;
	t_pb_graph_pin*** intra_lb_pb_pin_lookup; 
	int **lookup_tnode_from_pin_id;

	f_net_to_driver_tnode = (int*)vtr::malloc(num_timing_nets * sizeof(int));

	intra_lb_pb_pin_lookup = new t_pb_graph_pin**[num_types];
	for (i = 0; i < num_types; i++) {
		intra_lb_pb_pin_lookup[i] = alloc_and_load_pb_graph_pin_lookup_from_index(&type_descriptors[i]);
	}

	for (i = 0; i < num_timing_nets; i++) {
		f_net_to_driver_tnode[i] = OPEN;
	}

	/* allocate space for tnodes */
	num_tnodes = 0;
	for (i = 0; i < num_blocks; i++) {
		num_nodes_in_block = 0;
		int itype = block[i].type->index;
		for (j = 0; j < block[i].pb->pb_graph_node->total_pb_pins; j++) {
			if (block[i].pb_route[j].atom_net_idx != OPEN) {
				if (intra_lb_pb_pin_lookup[itype][j]->type == PB_PIN_INPAD
					|| intra_lb_pb_pin_lookup[itype][j]->type
								== PB_PIN_OUTPAD
					|| intra_lb_pb_pin_lookup[itype][j]->type
								== PB_PIN_SEQUENTIAL) {
					num_nodes_in_block += 2;
				} else {
					num_nodes_in_block++;
				}
			}
		}
		num_tnodes += num_nodes_in_block;
	}
	tnode = (t_tnode*)vtr::calloc(num_tnodes, sizeof(t_tnode));

	/* load tnodes with all info except edge info */
	/* populate tnode lookups for edge info */
	inode = 0;
	for (i = 0; i < num_blocks; i++) {
		int itype = block[i].type->index;
		for (j = 0; j < block[i].pb->pb_graph_node->total_pb_pins; j++) {
			if (block[i].pb_route[j].atom_net_idx != OPEN) {
				VTR_ASSERT(tnode[inode].pb_graph_pin == NULL);
				load_tnode(intra_lb_pb_pin_lookup[itype][j], i, &inode,
						timing_inf);
			}
		}
	}
	VTR_ASSERT(inode == num_tnodes);
	num_dangling_pins = 0;

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();


	/* load edge delays and initialize clock domains to OPEN 
	and prepacked_data (which is not used post-packing) to NULL. */
	for (i = 0; i < num_tnodes; i++) {
		tnode[i].clock_domain = OPEN;
		tnode[i].prepacked_data = NULL;

		/* 3 primary scenarios for edge delays
		 1.  Point-to-point delays inside block
		 2.  
		 */
		count = 0;
		iblock = tnode[i].block;
		int itype = block[iblock].type->index;
		switch (tnode[i].type) {
		case TN_INPAD_OPIN:
		case TN_INTERMEDIATE_NODE:
		case TN_PRIMITIVE_OPIN:
		case TN_FF_OPIN:
		case TN_CLOCK_OPIN:
		case TN_CB_IPIN:
			/* fanout is determined by intra-cluster connections */
			/* Allocate space for edges  */
			i_pin_id = tnode[i].pb_graph_pin->pin_count_in_cluster;
			intra_lb_route = block[iblock].pb_route;
			ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];

			if (ipb_graph_pin->parent_node->pb_type->max_internal_delay
					!= UNDEFINED) {
				if (pb_max_internal_delay == UNDEFINED) {
					pb_max_internal_delay =
							ipb_graph_pin->parent_node->pb_type->max_internal_delay;
					pbtype_max_internal_delay =
							ipb_graph_pin->parent_node->pb_type;
				} else if (pb_max_internal_delay
						< ipb_graph_pin->parent_node->pb_type->max_internal_delay) {
					pb_max_internal_delay =
							ipb_graph_pin->parent_node->pb_type->max_internal_delay;
					pbtype_max_internal_delay =
							ipb_graph_pin->parent_node->pb_type;
				}
			}

			for (j = 0; j < ipb_graph_pin->num_output_edges; j++) {
				VTR_ASSERT(ipb_graph_pin->output_edges[j]->num_output_pins == 1);
				dnode = ipb_graph_pin->output_edges[j]->output_pins[0]->pin_count_in_cluster;
				if (intra_lb_route[dnode].prev_pb_pin_id == i_pin_id) {
					count++;
				}
			}
			VTR_ASSERT(count >= 0);
			tnode[i].num_edges = count;
			tnode[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					count * sizeof(t_tedge), &tedge_ch);

			/* Load edges */
			count = 0;
			for (j = 0; j < ipb_graph_pin->num_output_edges; j++) {
				VTR_ASSERT(ipb_graph_pin->output_edges[j]->num_output_pins == 1);
				dnode = ipb_graph_pin->output_edges[j]->output_pins[0]->pin_count_in_cluster;
				if (intra_lb_route[dnode].prev_pb_pin_id == i_pin_id) {
					VTR_ASSERT(intra_lb_route[dnode].atom_net_idx == intra_lb_route[i_pin_id].atom_net_idx);

					tnode[i].out_edges[count].Tdel =
							ipb_graph_pin->output_edges[j]->delay_max;
					tnode[i].out_edges[count].to_node = lookup_tnode_from_pin_id[iblock][dnode];
					VTR_ASSERT(tnode[i].out_edges[count].to_node != OPEN);

					if (g_atoms_nlist.net[intra_lb_route[i_pin_id].atom_net_idx].is_const_gen
							== true && tnode[i].type == TN_PRIMITIVE_OPIN) {
						tnode[i].out_edges[count].Tdel = HUGE_NEGATIVE_FLOAT;
						tnode[i].type = TN_CONSTANT_GEN_SOURCE;
					}

					count++;
				}
			}
			VTR_ASSERT(count >= 0);

			break;
		case TN_PRIMITIVE_IPIN:
			/* Pin info comes from pb_graph block delays
			 */
			/*there would be no slack information if timing analysis is off*/
			if (timing_inf.timing_analysis_enabled)
			{
				i_pin_id = tnode[i].pb_graph_pin->pin_count_in_cluster;
				intra_lb_route = block[iblock].pb_route;
				ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];
				tnode[i].num_edges = ipb_graph_pin->num_pin_timing;
				tnode[i].out_edges = (t_tedge *) vtr::chunk_malloc(
						ipb_graph_pin->num_pin_timing * sizeof(t_tedge),
						&tedge_ch);
				k = 0;

				for (j = 0; j < tnode[i].num_edges; j++) {
					/* Some outpins aren't used, ignore these.  Only consider output pins that are used */
					if (intra_lb_route[ipb_graph_pin->pin_timing[j]->pin_count_in_cluster].atom_net_idx
							!= OPEN) {
						tnode[i].out_edges[k].Tdel =
								ipb_graph_pin->pin_timing_del_max[j];
						tnode[i].out_edges[k].to_node = lookup_tnode_from_pin_id[tnode[i].block][ipb_graph_pin->pin_timing[j]->pin_count_in_cluster];
						VTR_ASSERT(tnode[i].out_edges[k].to_node != OPEN);
						k++;
					}
				}
				tnode[i].num_edges -= (j - k); /* remove unused edges */
				if (tnode[i].num_edges == 0) {
					/* Dangling pin */
					num_dangling_pins++;
				}
			}
			break;
		case TN_CB_OPIN:
			/* load up net info */
			i_pin_id = tnode[i].pb_graph_pin->pin_count_in_cluster;
			intra_lb_route = block[iblock].pb_route;
			ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];

			VTR_ASSERT(intra_lb_route[i_pin_id].atom_net_idx != OPEN);
			inet = vpack_to_clb_net_mapping[intra_lb_route[i_pin_id].atom_net_idx];
			VTR_ASSERT(inet != OPEN);
			f_net_to_driver_tnode[inet] = i;
			tnode[i].num_edges = g_clbs_nlist.net[inet].num_sinks();
			tnode[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					g_clbs_nlist.net[inet].num_sinks()* sizeof(t_tedge),
					&tedge_ch);
			for (j = 1; j < (int) g_clbs_nlist.net[inet].pins.size(); j++) {
				dblock = g_clbs_nlist.net[inet].pins[j].block;
				normalization = block[dblock].type->num_pins
						/ block[dblock].type->capacity;
				normalized_pin = g_clbs_nlist.net[inet].pins[j].block_pin
						% normalization;
				d_intra_lb_route = block[dblock].pb_route;
				dpin = OPEN;
				dport = OPEN;
				count = 0;

				for (k = 0;
						k < block[dblock].pb->pb_graph_node->num_input_ports
								&& dpin == OPEN; k++) {
					if (normalized_pin >= count
							&& (count
									+ block[dblock].pb->pb_graph_node->num_input_pins[k]
									> normalized_pin)) {
						dpin = normalized_pin - count;
						dport = k;
						break;
					}
					count += block[dblock].pb->pb_graph_node->num_input_pins[k];
				}
				if (dpin == OPEN) {
					for (k = 0;
							k
									< block[dblock].pb->pb_graph_node->num_output_ports
									&& dpin == OPEN; k++) {
						count +=
								block[dblock].pb->pb_graph_node->num_output_pins[k];
					}
					for (k = 0;
							k < block[dblock].pb->pb_graph_node->num_clock_ports
									&& dpin == OPEN; k++) {
						if (normalized_pin >= count
								&& (count
										+ block[dblock].pb->pb_graph_node->num_clock_pins[k]
										> normalized_pin)) {
							dpin = normalized_pin - count;
							dport = k;
						}
						count +=
								block[dblock].pb->pb_graph_node->num_clock_pins[k];
					}
					VTR_ASSERT(dpin != OPEN);
					VTR_ASSERT(
						inet == vpack_to_clb_net_mapping[d_intra_lb_route[block[dblock].pb->pb_graph_node->clock_pins[dport][dpin].pin_count_in_cluster].atom_net_idx]);
					tnode[i].out_edges[j - 1].to_node =
						lookup_tnode_from_pin_id[dblock][block[dblock].pb->pb_graph_node->clock_pins[dport][dpin].pin_count_in_cluster];
					VTR_ASSERT(tnode[i].out_edges[j - 1].to_node != OPEN);
				} else {
					VTR_ASSERT(dpin != OPEN);
					VTR_ASSERT(
						inet == vpack_to_clb_net_mapping[d_intra_lb_route[block[dblock].pb->pb_graph_node->input_pins[dport][dpin].pin_count_in_cluster].atom_net_idx]);
					/* delays are assigned post routing */
					tnode[i].out_edges[j - 1].to_node =
						lookup_tnode_from_pin_id[dblock][block[dblock].pb->pb_graph_node->input_pins[dport][dpin].pin_count_in_cluster];
					VTR_ASSERT(tnode[i].out_edges[j - 1].to_node != OPEN);
				}
				tnode[i].out_edges[j - 1].Tdel = 0;
				VTR_ASSERT(inet != OPEN);
			}
			break;
		case TN_OUTPAD_IPIN:
		case TN_INPAD_SOURCE:
		case TN_OUTPAD_SINK:
		case TN_FF_SINK:
		case TN_FF_SOURCE:
		case TN_FF_IPIN:
		case TN_FF_CLOCK:
		case TN_CLOCK_SOURCE:
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, 
					"Consistency check failed: Unknown tnode type %d.\n", tnode[i].type);
			VTR_ASSERT(0);
			break;
		}
	}
	if(num_dangling_pins > 0) {
		vtr::printf_warning(__FILE__, __LINE__, 
				"Unconnected logic in design, number of dangling tnodes = %d\n", num_dangling_pins);
	}

	
	for (i = 0; i < num_types; i++) {
		free_pb_graph_pin_lookup_from_index(intra_lb_pb_pin_lookup[i]);
	}
	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);
	delete[] intra_lb_pb_pin_lookup;
}

/* Allocate timing graph for pre packed netlist
 Count number of tnodes first
 Then connect up tnodes with edges
 */
static void alloc_and_load_tnodes_from_prepacked_netlist(float block_delay,
		float inter_cluster_net_delay) {
	int i, j, k;
	const t_model *model;
	t_model_ports *model_port;
	t_pb_graph_pin *from_pb_graph_pin, *to_pb_graph_pin;
	int inode, inet;
	int incr;
	int count;

	f_net_to_driver_tnode = (int*)vtr::malloc(g_atoms_nlist.net.size() * sizeof(int));

	for (i = 0; i < (int) g_atoms_nlist.net.size(); i++) {
		f_net_to_driver_tnode[i] = OPEN;
	}
	
	/* allocate space for tnodes */
	num_tnodes = 0;
	for (i = 0; i < num_logical_blocks; i++) {
		model = logical_block[i].model;
		logical_block[i].clock_net_tnode = NULL;
		if (logical_block[i].type == VPACK_INPAD) {
			logical_block[i].output_net_tnodes = (t_tnode***)vtr::calloc(1,
					sizeof(t_tnode**));
			num_tnodes += 2;
		} else if (logical_block[i].type == VPACK_OUTPAD) {
			logical_block[i].input_net_tnodes = (t_tnode***)vtr::calloc(1, sizeof(t_tnode**));
			num_tnodes += 2;
		} else {
			if (logical_block[i].clock_net == OPEN) {
				incr = 1;
			} else {
				incr = 2;
			}
			j = 0;
			model_port = model->inputs;
			while (model_port) {
				if (model_port->is_clock == false) {
					for (k = 0; k < model_port->size; k++) {
						if (logical_block[i].input_nets[j][k] != OPEN) {
							num_tnodes += incr;
						}
					}
					j++;
				} else {
					num_tnodes++;
				}
				model_port = model_port->next;
			}
			logical_block[i].input_net_tnodes = (t_tnode ***)vtr::calloc(j, sizeof(t_tnode**));

			j = 0;
			model_port = model->outputs;
			while (model_port) {
				for (k = 0; k < model_port->size; k++) {
					if (logical_block[i].output_nets[j][k] != OPEN) {
						num_tnodes += incr;
					}
				}
				j++;
				model_port = model_port->next;
			}
			logical_block[i].output_net_tnodes = (t_tnode ***)vtr::calloc(j, sizeof(t_tnode**));
		}
	}
	tnode = (t_tnode *)vtr::calloc(num_tnodes, sizeof(t_tnode));
	
	/* Allocate space for prepacked_data, which is only used pre-packing. */
	for (inode = 0; inode < num_tnodes; inode++) {
		tnode[inode].prepacked_data = (t_prepacked_tnode_data *) vtr::malloc(sizeof(t_prepacked_tnode_data));
	}

	/* load tnodes, alloc edges for tnodes, load all known tnodes */
	inode = 0;
	for (i = 0; i < num_logical_blocks; i++) {
		model = logical_block[i].model;
		if (logical_block[i].type == VPACK_INPAD) {
			logical_block[i].output_net_tnodes[0] = (t_tnode **)vtr::calloc(1,
					sizeof(t_tnode*));
			logical_block[i].output_net_tnodes[0][0] = &tnode[inode];
			f_net_to_driver_tnode[logical_block[i].output_nets[0][0]] = inode;
			tnode[inode].prepacked_data->model_pin = 0;
			tnode[inode].prepacked_data->model_port = 0;
			tnode[inode].prepacked_data->model_port_ptr = model->outputs;
			tnode[inode].block = i;
			tnode[inode].type = TN_INPAD_OPIN;

			tnode[inode].num_edges =
					g_atoms_nlist.net[logical_block[i].output_nets[0][0]].num_sinks();
			tnode[inode].out_edges = (t_tedge *) vtr::chunk_malloc(
					tnode[inode].num_edges * sizeof(t_tedge),
					&tedge_ch);
			tnode[inode + 1].num_edges = 1;
			tnode[inode + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			tnode[inode + 1].out_edges->Tdel = 0;
			tnode[inode + 1].out_edges->to_node = inode;
			tnode[inode + 1].type = TN_INPAD_SOURCE;
			tnode[inode + 1].block = i;
			inode += 2;
		} else if (logical_block[i].type == VPACK_OUTPAD) {
			logical_block[i].input_net_tnodes[0] = (t_tnode **)vtr::calloc(1,
					sizeof(t_tnode*));
			logical_block[i].input_net_tnodes[0][0] = &tnode[inode];
			tnode[inode].prepacked_data->model_pin = 0;
			tnode[inode].prepacked_data->model_port = 0;
			tnode[inode].prepacked_data->model_port_ptr = model->inputs;
			tnode[inode].block = i;
			tnode[inode].type = TN_OUTPAD_IPIN;
			tnode[inode].num_edges = 1;
			tnode[inode].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			tnode[inode].out_edges->Tdel = 0;
			tnode[inode].out_edges->to_node = inode + 1;
			tnode[inode + 1].type = TN_OUTPAD_SINK;
			tnode[inode + 1].block = i;
			tnode[inode + 1].num_edges = 0;
			tnode[inode + 1].out_edges = NULL;
			inode += 2;
		} else {
			j = 0;
			model_port = model->outputs;
			while (model_port) {
                logical_block[i].output_net_tnodes[j] = (t_tnode **)vtr::calloc( model_port->size, sizeof(t_tnode*));
				if (model_port->is_clock == false) {
                    for (k = 0; k < model_port->size; k++) {
                        if (logical_block[i].output_nets[j][k] != OPEN) {
                            tnode[inode].prepacked_data->model_pin = k;
                            tnode[inode].prepacked_data->model_port = j;
                            tnode[inode].prepacked_data->model_port_ptr = model_port;
                            tnode[inode].block = i;
                            f_net_to_driver_tnode[logical_block[i].output_nets[j][k]] =
                                    inode;
                            logical_block[i].output_net_tnodes[j][k] =
                                    &tnode[inode];

                            tnode[inode].num_edges =
                                    g_atoms_nlist.net[logical_block[i].output_nets[j][k]].num_sinks();
                            tnode[inode].out_edges = (t_tedge *) vtr::chunk_malloc(
                                    tnode[inode].num_edges * sizeof(t_tedge),
                                    &tedge_ch);

                            if (logical_block[i].clock_net == OPEN) {
                                tnode[inode].type = TN_PRIMITIVE_OPIN;
                                inode++;
                            } else {
                                /* load delays from predicted clock-to-Q time */
                                from_pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, k, logical_block[i].expected_lowest_cost_primitive);
                                tnode[inode].type = TN_FF_OPIN;
                                tnode[inode + 1].num_edges = 1;
                                tnode[inode + 1].out_edges =
                                        (t_tedge *) vtr::chunk_malloc(
                                                1 * sizeof(t_tedge),
                                                &tedge_ch);
                                tnode[inode + 1].out_edges->to_node = inode;
                                tnode[inode + 1].out_edges->Tdel = from_pb_graph_pin->tsu_tco;
                                tnode[inode + 1].type = TN_FF_SOURCE;
                                tnode[inode + 1].block = i;
                                inode += 2;
                            }
                        }
                    }
                } else {
                    //An output clock port, that is a clock source (e.g. PLL output)

                    //For every non-empty pin on the clock port create a clock pin and clock source tnode
                    for (k = 0; k < model_port->size; k++) {
                        if (logical_block[i].output_nets[j][k] != OPEN) {

                            //Clock output pin
                            tnode[inode].type = TN_CLOCK_OPIN;

                            tnode[inode].prepacked_data->model_pin = k;
                            tnode[inode].prepacked_data->model_port = j;
                            tnode[inode].prepacked_data->model_port_ptr = model_port;
                            tnode[inode].block = i;

                            //Save this as the driver tnode
                            f_net_to_driver_tnode[logical_block[i].output_nets[j][k]] = inode;
                            logical_block[i].output_net_tnodes[j][k] = &tnode[inode];

                            //Allocate space for the output edges
                            tnode[inode].num_edges = g_atoms_nlist.net[logical_block[i].output_nets[j][k]].num_sinks();
                            tnode[inode].out_edges = (t_tedge *) vtr::chunk_malloc( tnode[inode].num_edges * sizeof(t_tedge), &tedge_ch);

                            //Create the clock source
                            from_pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, k, logical_block[i].expected_lowest_cost_primitive);
                            tnode[inode + 1].type = TN_CLOCK_SOURCE;
                            tnode[inode + 1].num_edges = 1;
                            tnode[inode + 1].out_edges = (t_tedge *) vtr::chunk_malloc( 1 * sizeof(t_tedge), &tedge_ch);
                            tnode[inode + 1].out_edges->to_node = inode;
                            tnode[inode + 1].out_edges->Tdel = from_pb_graph_pin->tsu_tco;
                            tnode[inode + 1].prepacked_data->model_pin = k;
                            tnode[inode + 1].prepacked_data->model_port = j;
                            tnode[inode + 1].prepacked_data->model_port_ptr = model_port;
                            tnode[inode + 1].block = i;
                            inode += 2;
                        }
                    }
                }
				j++;
				model_port = model_port->next;
			}

			j = 0;
			model_port = model->inputs;
			while (model_port) {
				if (model_port->is_clock == false) {
					logical_block[i].input_net_tnodes[j] = (t_tnode **)vtr::calloc(
							model_port->size, sizeof(t_tnode*));
					for (k = 0; k < model_port->size; k++) {
						if (logical_block[i].input_nets[j][k] != OPEN) {
							tnode[inode].prepacked_data->model_pin = k;
							tnode[inode].prepacked_data->model_port = j;
							tnode[inode].prepacked_data->model_port_ptr = model_port;
							tnode[inode].block = i;
							logical_block[i].input_net_tnodes[j][k] =
									&tnode[inode];
							from_pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, k, logical_block[i].expected_lowest_cost_primitive);
							if (logical_block[i].clock_net == OPEN) {
								/* load predicted combinational delays to predicted edges */
								tnode[inode].type = TN_PRIMITIVE_IPIN;
								tnode[inode].out_edges =
										(t_tedge *) vtr::chunk_malloc(
										from_pb_graph_pin->num_pin_timing * sizeof(t_tedge),
												&tedge_ch);
								count = 0;
								for(int m = 0; m < from_pb_graph_pin->num_pin_timing; m++) {
									to_pb_graph_pin = from_pb_graph_pin->pin_timing[m];
									if(logical_block[i].output_nets[to_pb_graph_pin->port->model_port->index][to_pb_graph_pin->pin_number] == OPEN) {
										continue;
									}
									tnode[inode].out_edges[count].Tdel = from_pb_graph_pin->pin_timing_del_max[m];
									tnode[inode].out_edges[count].to_node = 
										get_tnode_index(logical_block[i].output_net_tnodes[to_pb_graph_pin->port->model_port->index][to_pb_graph_pin->pin_number]);
									count++;
								}
								tnode[inode].num_edges = count;
								inode++;
							} else {
								/* load predicted setup time */
								tnode[inode].type = TN_FF_IPIN;
								tnode[inode].num_edges = 1;
								tnode[inode].out_edges =
										(t_tedge *) vtr::chunk_malloc(
												1 * sizeof(t_tedge),
												&tedge_ch);
								tnode[inode].out_edges->to_node = inode + 1;
								tnode[inode].out_edges->Tdel = from_pb_graph_pin->tsu_tco;
								tnode[inode + 1].type = TN_FF_SINK;
								tnode[inode + 1].num_edges = 0;
								tnode[inode + 1].out_edges = NULL;
								tnode[inode + 1].block = i;
								inode += 2;
							}
						}
					}
					j++;
				} else {
					if (logical_block[i].clock_net != OPEN) {
						VTR_ASSERT(logical_block[i].clock_net_tnode == NULL);
						logical_block[i].clock_net_tnode = &tnode[inode];
						tnode[inode].block = i;
						tnode[inode].prepacked_data->model_pin = 0;
						tnode[inode].prepacked_data->model_port = 0;
						tnode[inode].prepacked_data->model_port_ptr = model_port;
						tnode[inode].num_edges = 0;
						tnode[inode].out_edges = NULL;
						tnode[inode].type = TN_FF_CLOCK;
						inode++;
					}
				}
				model_port = model_port->next;
			}
		}
	}
	VTR_ASSERT(inode == num_tnodes);

	/* load edge delays and initialize clock domains to OPEN. */
	for (i = 0; i < num_tnodes; i++) {
		tnode[i].clock_domain = OPEN;

		/* 3 primary scenarios for edge delays
		 1.  Point-to-point delays inside block
		 2.  
		 */
		count = 0;
		switch (tnode[i].type) {
		case TN_INPAD_OPIN:
		case TN_PRIMITIVE_OPIN:
		case TN_FF_OPIN:
		case TN_CLOCK_OPIN:
			/* fanout is determined by intra-cluster connections */
			/* Allocate space for edges  */
			inet =
					logical_block[tnode[i].block].output_nets[tnode[i].prepacked_data->model_port][tnode[i].prepacked_data->model_pin];
			VTR_ASSERT(inet != OPEN);

			for (j = 1; j < (int) g_atoms_nlist.net[inet].pins.size(); j++) {
				if (g_atoms_nlist.net[inet].is_const_gen) {
					tnode[i].out_edges[j - 1].Tdel = HUGE_NEGATIVE_FLOAT;
					tnode[i].type = TN_CONSTANT_GEN_SOURCE;
				} else {
					tnode[i].out_edges[j - 1].Tdel = inter_cluster_net_delay;
				}
				if (g_atoms_nlist.net[inet].is_global) {
					VTR_ASSERT(
						logical_block[g_atoms_nlist.net[inet].pins[j].block].clock_net == inet);
					tnode[i].out_edges[j - 1].to_node =
						get_tnode_index(logical_block[g_atoms_nlist.net[inet].pins[j].block].clock_net_tnode);
				} else {
					VTR_ASSERT(
						logical_block[g_atoms_nlist.net[inet].pins[j].block].input_net_tnodes[g_atoms_nlist.net[inet].pins[j].block_port][g_atoms_nlist.net[inet].pins[j].block_pin] != NULL);
					tnode[i].out_edges[j - 1].to_node =
						get_tnode_index(logical_block[g_atoms_nlist.net[inet].pins[j].block].input_net_tnodes[g_atoms_nlist.net[inet].pins[j].block_port][g_atoms_nlist.net[inet].pins[j].block_pin]);
				}
			}
			VTR_ASSERT(tnode[i].num_edges == g_atoms_nlist.net[inet].num_sinks());
			break;
		case TN_PRIMITIVE_IPIN:
		case TN_OUTPAD_IPIN:
		case TN_INPAD_SOURCE:
		case TN_OUTPAD_SINK:
		case TN_FF_SINK:
		case TN_FF_SOURCE:
		case TN_FF_IPIN:
		case TN_FF_CLOCK:
		case TN_CLOCK_SOURCE:
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, 
					"Consistency check failed: Unknown tnode type %d.\n", tnode[i].type);
			VTR_ASSERT(0);
			break;
		}
	}

	for (i = 0; i < (int) g_atoms_nlist.net.size(); i++) {
		VTR_ASSERT(f_net_to_driver_tnode[i] != OPEN);
	}
}

static void load_tnode(t_pb_graph_pin *pb_graph_pin, const int iblock,
		int *inode, const t_timing_inf timing_inf) {
	int i;
	i = *inode;
	tnode[i].pb_graph_pin = pb_graph_pin;
	tnode[i].block = iblock;

	if (tnode[i].pb_graph_pin->parent_node->pb_type->blif_model == NULL) {
		VTR_ASSERT(tnode[i].pb_graph_pin->type == PB_PIN_NORMAL);
		if (tnode[i].pb_graph_pin->parent_node->parent_pb_graph_node == NULL) {
			if (tnode[i].pb_graph_pin->port->type == IN_PORT) {
				tnode[i].type = TN_CB_IPIN;
			} else {
				VTR_ASSERT(tnode[i].pb_graph_pin->port->type == OUT_PORT);
				tnode[i].type = TN_CB_OPIN;
			}
		} else {
			tnode[i].type = TN_INTERMEDIATE_NODE;
		}
	} else {
		if (tnode[i].pb_graph_pin->type == PB_PIN_INPAD) {
			VTR_ASSERT(tnode[i].pb_graph_pin->port->type == OUT_PORT);
			tnode[i].type = TN_INPAD_OPIN;
			tnode[i + 1].num_edges = 1;
			tnode[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			tnode[i + 1].out_edges->Tdel = 0;
			tnode[i + 1].out_edges->to_node = i;
			tnode[i + 1].pb_graph_pin = pb_graph_pin; /* Necessary for propagate_clock_domain_and_skew(). */
			tnode[i + 1].type = TN_INPAD_SOURCE;
			tnode[i + 1].block = iblock;
			(*inode)++;
		} else if (tnode[i].pb_graph_pin->type == PB_PIN_OUTPAD) {
			VTR_ASSERT(tnode[i].pb_graph_pin->port->type == IN_PORT);
			tnode[i].type = TN_OUTPAD_IPIN;
			tnode[i].num_edges = 1;
			tnode[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			tnode[i].out_edges->Tdel = 0;
			tnode[i].out_edges->to_node = i + 1;
			tnode[i + 1].pb_graph_pin = pb_graph_pin; /* Necessary for find_tnode_net_name(). */
			tnode[i + 1].type = TN_OUTPAD_SINK;
			tnode[i + 1].block = iblock;
			tnode[i + 1].num_edges = 0;
			tnode[i + 1].out_edges = NULL;
			(*inode)++;
		} else if (tnode[i].pb_graph_pin->type == PB_PIN_SEQUENTIAL) {
			if (tnode[i].pb_graph_pin->port->type == IN_PORT) {
				tnode[i].type = TN_FF_IPIN;
				tnode[i].num_edges = 1;
				tnode[i].out_edges = (t_tedge *) vtr::chunk_malloc(
						1 * sizeof(t_tedge), &tedge_ch);
				tnode[i].out_edges->Tdel = pb_graph_pin->tsu_tco;
				tnode[i].out_edges->to_node = i + 1;
				tnode[i + 1].pb_graph_pin = pb_graph_pin;
				tnode[i + 1].type = TN_FF_SINK;
				tnode[i + 1].block = iblock;
				tnode[i + 1].num_edges = 0;
				tnode[i + 1].out_edges = NULL;
			} else {
				VTR_ASSERT(tnode[i].pb_graph_pin->port->type == OUT_PORT);
                //Determine whether we are a standard clocked output pin (TN_FF_OPIN with TN_FF_SOURCE)
                //or a clock source (TN_CLOCK_OPIN with TN_CLOCK_SOURCE)
                t_model_ports* model_port = tnode[i].pb_graph_pin->port->model_port;
                if(!model_port->is_clock) {
                    //Standard sequential output
                    tnode[i].type = TN_FF_OPIN;
                    tnode[i + 1].num_edges = 1;
                    tnode[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
                            1 * sizeof(t_tedge), &tedge_ch);
                    tnode[i + 1].out_edges->Tdel = pb_graph_pin->tsu_tco;
                    tnode[i + 1].out_edges->to_node = i;
                    tnode[i + 1].pb_graph_pin = pb_graph_pin;
                    tnode[i + 1].type = TN_FF_SOURCE;
                    tnode[i + 1].block = iblock;
                } else {
                    //Clock source
                    tnode[i].type = TN_CLOCK_OPIN;
                    tnode[i + 1].num_edges = 1;
                    tnode[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
                            1 * sizeof(t_tedge), &tedge_ch);
                    tnode[i + 1].out_edges->Tdel = pb_graph_pin->tsu_tco;
                    tnode[i + 1].out_edges->to_node = i;
                    tnode[i + 1].pb_graph_pin = pb_graph_pin;
                    tnode[i + 1].type = TN_CLOCK_SOURCE;
                    tnode[i + 1].block = iblock;
                }
			}
			(*inode)++;
		} else if (tnode[i].pb_graph_pin->type == PB_PIN_CLOCK) {
			tnode[i].type = TN_FF_CLOCK;
			tnode[i].num_edges = 0;
			tnode[i].out_edges = NULL;
		} else {
			if (tnode[i].pb_graph_pin->port->type == IN_PORT) {
				VTR_ASSERT(tnode[i].pb_graph_pin->type == PB_PIN_TERMINAL);
				tnode[i].type = TN_PRIMITIVE_IPIN;
			} else {
				VTR_ASSERT(tnode[i].pb_graph_pin->port->type == OUT_PORT);
				VTR_ASSERT(tnode[i].pb_graph_pin->type == PB_PIN_TERMINAL);
				tnode[i].type = TN_PRIMITIVE_OPIN;
			}
		}
	}
	(*inode)++;
}

void print_timing_graph(const char *fname) {

	/* Prints the timing graph into a file. */

	FILE *fp;
	int inode, iedge, ilevel, i;
	t_tedge *tedge;
	e_tnode_type itype;
	const char *tnode_type_names[] = {  "TN_INPAD_SOURCE", "TN_INPAD_OPIN", "TN_OUTPAD_IPIN",
			"TN_OUTPAD_SINK", "TN_CB_IPIN", "TN_CB_OPIN", "TN_INTERMEDIATE_NODE",
			"TN_PRIMITIVE_IPIN", "TN_PRIMITIVE_OPIN", "TN_FF_IPIN", "TN_FF_OPIN", "TN_FF_SINK",
			"TN_FF_SOURCE", "TN_FF_CLOCK", "TN_CLOCK_SOURCE", "TN_CLOCK_OPIN", "TN_CONSTANT_GEN_SOURCE" };

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "num_tnodes: %d\n", num_tnodes);
	fprintf(fp, "Node #\tType\t\tipin\tiblk\tDomain\tSkew\tI/O Delay\t# edges\t"
			"to_node     Tdel\n\n");

	for (inode = 0; inode < num_tnodes; inode++) {
		fprintf(fp, "%d\t", inode);

		itype = tnode[inode].type;
		fprintf(fp, "%-15.15s\t", tnode_type_names[itype]);

		if (tnode[inode].pb_graph_pin != NULL) {
			fprintf(fp, "%d\t%d\t",
					tnode[inode].pb_graph_pin->pin_count_in_cluster,
					tnode[inode].block);
		} else {
			fprintf(fp, "\t%d\t", tnode[inode].block);
		}

		if (itype == TN_FF_CLOCK || itype == TN_FF_SOURCE || itype == TN_FF_SINK) {
			fprintf(fp, "%d\t%.3e\t\t", tnode[inode].clock_domain, tnode[inode].clock_delay);
		} else if (itype == TN_INPAD_SOURCE || itype ==TN_CLOCK_SOURCE) {
			fprintf(fp, "%d\t\t%.3e\t", tnode[inode].clock_domain, tnode[inode].out_edges ? tnode[inode].out_edges[0].Tdel : -1);
		} else if (itype == TN_OUTPAD_SINK) {
			VTR_ASSERT(tnode[inode-1].type == TN_OUTPAD_IPIN); /* Outpad ipins should be one prior in the tnode array */
			fprintf(fp, "%d\t\t%.3e\t", tnode[inode].clock_domain, tnode[inode-1].out_edges[0].Tdel);
		} else {
			fprintf(fp, "\t\t\t\t");
		}

		fprintf(fp, "%d", tnode[inode].num_edges);

		/* Print all edges after edge 0 on separate lines */
		tedge = tnode[inode].out_edges;
		if (tnode[inode].num_edges > 0) {
			fprintf(fp, "\t%4d\t%7.3g", tedge[0].to_node, tedge[0].Tdel);
			for (iedge = 1; iedge < tnode[inode].num_edges; iedge++) {
				fprintf(fp, "\n\t\t\t\t\t\t\t\t\t\t%4d\t%7.3g", tedge[iedge].to_node, tedge[iedge].Tdel);
			}
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "\n\nnum_tnode_levels: %d\n", num_tnode_levels);

	for (ilevel = 0; ilevel < num_tnode_levels; ilevel++) {
		fprintf(fp, "\n\nLevel: %d  Num_nodes: %d\nNodes:", ilevel,
				tnodes_at_level[ilevel].nelem);
		for (i = 0; i < tnodes_at_level[ilevel].nelem; i++)
			fprintf(fp, "\t%d", tnodes_at_level[ilevel].list[i]);
	}

	fprintf(fp, "\n");
	fprintf(fp, "\n\nNet #\tNet_to_driver_tnode\n");

	for (i = 0; i < (int)g_clbs_nlist.net.size(); i++)
		fprintf(fp, "%4d\t%6d\n", i, f_net_to_driver_tnode[i]);

	if (g_sdc && g_sdc->num_constrained_clocks == 1) {

		/* Arrival and required times, and forward and backward weights, will be meaningless for multiclock
		designs, since the values currently on the graph will only correspond to the most recent traversal. */
		fprintf(fp, "\n\nNode #\t\tT_arr\t\tT_req"
#ifdef PATH_COUNTING
			"\tForward weight\tBackward weight"
#endif
			"\n\n");

		for (inode = 0; inode < num_tnodes; inode++) {
			if (tnode[inode].T_arr > HUGE_NEGATIVE_FLOAT + 1) {
				fprintf(fp, "%d\t%12g", inode, tnode[inode].T_arr);
			} else {
				fprintf(fp, "%d\t\t   -", inode);
			}
			if (tnode[inode].T_req < HUGE_POSITIVE_FLOAT - 1) {
				fprintf(fp, "\t%12g", tnode[inode].T_req);
			} else {
				fprintf(fp, "\t\t   -");
			}
#ifdef PATH_COUNTING
			fprintf(fp, "\t%12g\t%12g", tnode[inode].forward_weight, tnode[inode].backward_weight);
#endif
            fprintf(fp, "\n");
		}
	}

	fclose(fp);
}
static void process_constraints(void) {
	/* Removes all constraints between domains which never intersect. We need to do this 
	so that criticality_denom in do_timing_analysis is not affected	by unused constraints. 
	BFS through the levelized graph once for each source domain. Whenever we get to a sink, 
	mark off that we've used that sink clock domain.  After	each traversal, set all unused
	constraints to DO_NOT_ANALYSE. 
	
	Also, print g_sdc->domain_constraints, constrained I/Os and override constraints, 
	and convert g_sdc->domain_constraints and flip-flop-level override constraints
	to be in seconds rather than nanoseconds. We don't need to normalize g_sdc->cc_constraints
	because they're already on the g_sdc->domain_constraints matrix, and we don't need
	to normalize constrained_ios because we already did the normalization when
	we put the delays onto the timing graph in load_clock_domain_and_clock_and_io_delay. */

	int source_clock_domain, sink_clock_domain, inode, ilevel, num_at_level, i,
		num_edges, iedge, to_node, icf, ifc, iff;
	t_tedge * tedge;
	float constraint;
	bool * constraint_used = (bool *) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(bool));

	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		/* We're going to use arrival time to flag which nodes we've reached, 
		even though the values we put in will not correspond to actual arrival times. 
		Nodes which are reached on this traversal will get an arrival time of 0. 
		Reset arrival times now to an invalid number. */
		for (inode = 0; inode < num_tnodes; inode++) {
			tnode[inode].T_arr = HUGE_NEGATIVE_FLOAT;
		}

		/* Reset all constraint_used entries. */
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			constraint_used[sink_clock_domain] = false;
		}

		/* Set arrival times for each top-level tnode on this clock domain. */
		num_at_level = tnodes_at_level[0].nelem;	
		for (i = 0; i < num_at_level; i++) {
			inode = tnodes_at_level[0].list[i];
			if (tnode[inode].clock_domain == source_clock_domain) {
				tnode[inode].T_arr = 0.;
			}
		}

		for (ilevel = 0; ilevel < num_tnode_levels; ilevel++) {	/* Go down one level at a time. */
			num_at_level = tnodes_at_level[ilevel].nelem;			
						
			for (i = 0; i < num_at_level; i++) {					
				inode = tnodes_at_level[ilevel].list[i];	/* Go through each of the tnodes at the level we're on. */
				if (has_valid_T_arr(inode)) {	/* If this tnode has been used */
					num_edges = tnode[inode].num_edges;
					if (num_edges == 0) { /* sink */
						/* We've reached the sink domain of this tnode, so set constraint_used  
						to true for this tnode's clock domain (if it has a valid one). */
						sink_clock_domain = tnode[inode].clock_domain;
						if (sink_clock_domain != -1) {
							constraint_used[sink_clock_domain] = true;
						}
					} else {
						/* Set arrival time to a valid value (0.) for each tnode in this tnode's fanout. */
						tedge = tnode[inode].out_edges;	
						for (iedge = 0; iedge < num_edges; iedge++) {
							to_node = tedge[iedge].to_node;
							if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

							tnode[to_node].T_arr = 0.;
						}
					}
				}
			}
		}

		/* At the end of the source domain traversal, see which sink domains haven't been hit, 
		and set the constraint for the pair of source and sink domains to DO_NOT_ANALYSE */

		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			if (!constraint_used[sink_clock_domain]) {
                if(g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] != DO_NOT_ANALYSE) {
                    vtr::printf_warning(__FILE__, __LINE__, "Timing constraint from clock %d to %d of value %f will be disabled"
                                                           " since it is not activated by any path in the timing graph.\n",
                                                           source_clock_domain, sink_clock_domain,
                                                           g_sdc->domain_constraint[source_clock_domain][sink_clock_domain]);
                }
				g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = DO_NOT_ANALYSE;
			}
		}
	}

	free(constraint_used);

	/* Print constraints */	
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_TIMING_CONSTRAINTS)) {
		print_timing_constraint_info(getEchoFileName(E_ECHO_TIMING_CONSTRAINTS));
	}

	/* Convert g_sdc->domain_constraint and ff-level override constraints to be in seconds, not nanoseconds. */
	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			constraint = g_sdc->domain_constraint[source_clock_domain][sink_clock_domain];
			if (constraint > NEGATIVE_EPSILON) { /* if constraint does not equal DO_NOT_ANALYSE */
				g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] = constraint * 1e-9;
			}
		}
	}
	for (icf = 0; icf < g_sdc->num_cf_constraints; icf++) {
		g_sdc->cf_constraints[icf].constraint *= 1e-9;
	}
	for (ifc = 0; ifc < g_sdc->num_fc_constraints; ifc++) {
		g_sdc->fc_constraints[ifc].constraint *= 1e-9;
	}
	for (iff = 0; iff < g_sdc->num_ff_constraints; iff++) {
		g_sdc->ff_constraints[iff].constraint *= 1e-9;
	}

	/* Finally, free g_sdc->cc_constraints since all of its info is contained in g_sdc->domain_constraint. */
	free_override_constraint(g_sdc->cc_constraints, g_sdc->num_cc_constraints);
}

static void alloc_timing_stats(void) {

	/* Allocate f_timing_stats data structure. */

	int i;

	f_timing_stats = (t_timing_stats *) vtr::malloc(sizeof(t_timing_stats));
	f_timing_stats->cpd = (float **) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(float *));
	f_timing_stats->least_slack = (float **) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(float *));
	for (i = 0; i < g_sdc->num_constrained_clocks; i++) {
		f_timing_stats->cpd[i] = (float *) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(float));
		f_timing_stats->least_slack[i] = (float *) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(float));
	}
}

void do_timing_analysis(t_slack * slacks, const t_timing_inf &timing_inf, bool is_prepacked, bool is_final_analysis) {

/*  Performs timing analysis on the circuit.  Before this routine is called, t_slack * slacks 
	must have been allocated, and the circuit must have been converted into a timing graph.
	The nodes of the timing graph represent pins and the edges between them represent delays 
	and m dependencies from one pin to another.  Most elements are modeled as a pair of nodes so
	that the delay through the element can be marked on the edge between them (e.g.
	TN_INPAD_SOURCE->TN_INPAD_OPIN, TN_OUTPAD_IPIN->TN_OUTPAD_SINK, TN_PRIMITIVE_OPIN->
	TN_PRIMITIVE_OPIN, etc.).
	
	The timing graph nodes are stored as an array, tnode [0..num_tnodes - 1]. Each tnode 
	includes an array of all edges, tedge, which fan out from it. Each tedge includes the
	index of the node on its far end (in the tnode array), and the delay to that node.

	The timing graph has sources at each TN_FF_SOURCE (Q output), TN_INPAD_SOURCE (input I/O pad)
	and TN_CONSTANT_GEN_SOURCE (constant 1 or 0 generator) node and sinks at TN_FF_SINK (D input) 
	and TN_OUTPAD_SINK (output I/O pad) nodes. Two traversals, one forward (sources to sinks)
	and one backward, are performed for each valid constraint (one which is not DO_NOT_ANALYSE) 
	between a source and a sink clock domain in the matrix g_sdc->domain_constraint 
	[0..g_sdc->num_constrained_clocks - 1][0..g_sdc->num_constrained_clocks - 1]. This matrix has been 
	pruned so that all domain pairs with no paths between them have been set to DO_NOT_ANALYSE.
	
	During the traversal pair for each constraint, all nodes in the fanout of sources on the
	source clock domain are assigned a T_arr, the arrival time of the last input signal to the node.
	All nodes in the fanin of sinks on the sink clock domain are assigned a T_req, the required 
	arrival time of the last input signal to the node if the critical path for this constraint is 
	not to be lengthened.  Nodes which receive both a valid T_arr and T_req are flagged with 
	used_on_this_traversal, and nodes which are used on at least one traversal pair	are flagged 
	with has_valid_slack so that later functions know the slack is valid.

	After each traversal pair, a slack is calculated for each sink pin on each net (or equivalently, 
	each connection or tedge fanning out from that net's driver tnode). Slack is calculated as:
		T_req (dest node) - T_arr (source node) - Tdel (edge)
	and represents the amount of delay which could be added to this connection before the critical 
	path delay for this constraint would worsen.  Edges on the critical path have a slack of 0.  
	Slacks which are never used are set to HUGE_POSITIVE_FLOAT.
	
	The optimizers actually use a metric called timing_criticality.  Timing criticality is defined 
	as 1 - slack / criticality_denom, where the normalization factor criticality_denom is the max
	of all arrival times in the constraint and the constraint itself (T_req-relaxed slacks) or
	all arrival times and constraints in the design (shifted slacks).  See below for a further 
	discussion of these two regimes.  Timing criticality is always between 0 (not critical) and 1
	(very critical).  Unlike slack, which is set to HUGE_POSITIVE_FLOAT for unanalysed connections,
	timing criticality is 0 for these, meaning no special check has to be made for which connections
	have been analysed.

	If path counting is on (PATH_COUNTING is defined in vpr_types.h), the optimizers use a weighted
	sum of timing_criticality and path_criticality, the latter of which is an estimate of the 
	importance of the number of paths using a particular connection.  As a path's timing_criticality
	decreases, it will become exponentially less important to the path_criticality of any connection
	which this path uses.  Path criticality also goes from 0 (not critical or unanalysed) to 1.

	Slack and criticalities are only calculated if both the driver of the net and the sink pin were
	used_on_this_traversal, and are only overwritten if lower than previously-obtained values. 
	The optimizers actually use criticality rather than slack, but slack is useful for human 
	designers and so we calculate it only if we need to print it.
	
	This routine outputs slack and criticality to t_slack * slacks. It also stores least slack and
	critical path delay per constraint [0..g_sdc->num_constrained_clocks - 1][0..g_sdc->num_constrained_clocks - 1] 
	in the file-scope variable f_timing_stats. 

	Is_prepacked flags whether to calculate normalized costs for the clusterer (normalized_slack, 
	normalized_Tarr, normalized_total_critical_paths). Setting this to false saves time in post-
	packed timing analyses.

	Is_final_analysis flags whether this is the final, analysis pass.  If it is, the analyser will 
	compute actual slacks instead of relaxed ones. We "relax" slacks by setting the required time to 
	the maximum arrival time for tight constraints so that no slacks are negative (which confuses 
	the optimizers). This is called "T_req-relaxed" slack.  However, human designers want to see 
	actual slack values, so we report those in the final analysis.  The alternative ways of making 
	slacks positive are shifting them upwards by the value of the largest negative slack, after 
	all traversals are complete ("shifted slacks"), which can be enabled by changing slack_definition 
	from 'R' to 'I'; or using either 'R' or 'I' with a criticality denominator which 
	is the same for every traversal (respectively called 'G' and 'S").

	To do: flip-flop to flip-flop and flip-flop to clock domain constraints (set_false_path, set_max_delay,
	and especially set_multicycle_path). All the info for these constraints is contained in g_sdc->fc_constraints and 
	g_sdc->ff_constraints, but graph traversals are not included yet. Probably, an entire traversal will be needed for
	each constraint. Clock domain to flip-flop constraints are coded but not tested, and are done within 
	existing traversals. */

    /* Description of slack_definition:
    slack_definition determines how to normalize negative slacks for the optimizers (not in the final timing analysis
    for output statistics):
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

    clock_t begin = clock();

#ifdef PATH_COUNTING
	float max_path_criticality = HUGE_NEGATIVE_FLOAT /* used to normalize path_criticalities */;
#endif

	bool update_slack = (is_final_analysis || getEchoEnabled());
								 /* Only update slack values if we need to print it, i.e.
								 for the final output file (is_final_analysis) or echo files. */

	float criticality_denom; /* (slack_definition == 'R' and 'I' only) For a particular constraint, the maximum of 
							 the constraint and all arrival times for the constraint's traversal. Used to 
							 normalize the clusterer's normalized_slack and, more importantly, criticality. */

	long max_critical_output_paths, max_critical_input_paths;

	vector<t_vnet> & tnets = *timing_nets; 

	float smallest_slack_in_design = HUGE_POSITIVE_FLOAT;
	/* For slack_definition == 'S', shift all slacks upwards by this number if it is negative. */

    float criticality_denom_global = HUGE_NEGATIVE_FLOAT;
    /* Denominator of criticality for slack_definition == 'S' || slack_definition == 'G' -
    max of all arrival times and all constraints. */

    update_slack = bool(timing_inf.slack_definition == 'S' || timing_inf.slack_definition == 'G');
	/* Update slack values for certain slack definitions where they are needed to compute timing criticalities. */

	/* Reset all values which need to be reset once per 
	timing analysis, rather than once per traversal pair. */

	/* Reset slack and criticality */
	for (int inet = 0; inet < num_timing_nets; inet++) {
		for (int ipin = 1; ipin < (int) tnets[inet].pins.size(); ipin++) {
			slacks->slack[inet][ipin]			   = HUGE_POSITIVE_FLOAT; 
			slacks->timing_criticality[inet][ipin] = 0.; 
#ifdef PATH_COUNTING
			slacks->path_criticality[inet][ipin] = 0.;
#endif
		}
	}

	/* Reset f_timing_stats. */
	for (int i = 0; i < g_sdc->num_constrained_clocks; i++) {	
		for (int j = 0; j < g_sdc->num_constrained_clocks; j++) {
			f_timing_stats->cpd[i][j]		  = HUGE_NEGATIVE_FLOAT;
			f_timing_stats->least_slack[i][j] = HUGE_POSITIVE_FLOAT;
		}
	}

#ifndef PATH_COUNTING
	/* Reset normalized values for clusterer. */
	if (is_prepacked) { 
		for (int inode = 0; inode < num_tnodes; inode++) {			
			tnode[inode].prepacked_data->normalized_slack = HUGE_POSITIVE_FLOAT;
			tnode[inode].prepacked_data->normalized_T_arr = HUGE_NEGATIVE_FLOAT;
			tnode[inode].prepacked_data->normalized_total_critical_paths = HUGE_NEGATIVE_FLOAT;
		}
	}
#endif

    t_pb*** pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();
    
    if (timing_inf.slack_definition == 'I') {
        /* Find the smallest slack in the design, if negative. */
        smallest_slack_in_design = find_least_slack(is_prepacked, pin_id_to_pb_mapping);
        if (smallest_slack_in_design > 0) smallest_slack_in_design = 0;
    }

	/* For each valid constraint (pair of source and sink clock domains), we do one 
	forward and one backward topological traversal to find arrival and required times,
	in do_timing_analysis_for_constraint. If path counting is on, we then do another, 
	simpler traversal to find forward and backward weights, relying on the largest 
	required time we found from the first traversal.  After each constraint's traversals,
	we update the slacks, timing criticalities and (if necessary) path criticalities or
	normalized costs used by the clusterer. */

	for (int source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (int sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
            if (g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) { /* i.e. != DO_NOT_ANALYSE */

                /* Perform the forward and backward traversal for this constraint. */
                criticality_denom = do_timing_analysis_for_constraint(source_clock_domain, sink_clock_domain,
                    is_prepacked, is_final_analysis, &max_critical_input_paths, &max_critical_output_paths, 
                    pin_id_to_pb_mapping, timing_inf);
#ifdef PATH_COUNTING
                /* Weight the importance of each net, used in slack calculation. */
                do_path_counting(criticality_denom);
#endif

                if (timing_inf.slack_definition == 'I') {
                    criticality_denom -= smallest_slack_in_design;
                    /* Remember, smallest_slack_in_design is negative, so we're INCREASING criticality_denom. */
                }

				/* Update the slack and criticality for each edge of each net which was  
				analysed on the most recent traversal and has a lower (slack) or 
				higher (criticality) value than before. */
				update_slacks(slacks, source_clock_domain, sink_clock_domain, criticality_denom, 
					update_slack, is_final_analysis, smallest_slack_in_design, timing_inf);

#ifndef PATH_COUNTING
				/* Update the normalized costs used by the clusterer. */
				if (is_prepacked) {
					update_normalized_costs(criticality_denom, max_critical_input_paths, max_critical_output_paths, timing_inf);
				}
#endif

                if (timing_inf.slack_definition == 'S' || timing_inf.slack_definition == 'G') {
				    /* Set criticality_denom_global to the max of criticality_denom over all traversals. */
				    criticality_denom_global = max(criticality_denom_global, criticality_denom);
                }
			}
		}
	} 

#ifdef PATH_COUNTING
	/* Normalize path criticalities by the largest value in the 
	circuit. Otherwise, path criticalities would be unbounded. */

	for (inet = 0; inet < num_timing_nets; inet++) {
		num_edges = tnets[inet].num_sinks();
		for (iedge = 0; iedge < num_edges; iedge++) {
			max_path_criticality = max(max_path_criticality, slacks->path_criticality[inet][iedge + 1]);
		}
	}

	for (inet = 0; inet < num_timing_nets; inet++) {
		num_edges = tnets[inet].num_sinks();
		for (iedge = 0; iedge < num_edges; iedge++) {
			slacks->path_criticality[inet][iedge + 1] /= max_path_criticality;
		}
	}

#endif

    if (timing_inf.slack_definition == 'S' || timing_inf.slack_definition == 'G') {
        if (!is_final_analysis) {
            if (timing_inf.slack_definition == 'S') {
                /* Find the smallest slack in the design. */
                for (int i = 0; i < g_sdc->num_constrained_clocks; i++) {
                    for (int j = 0; j < g_sdc->num_constrained_clocks; j++) {
                        smallest_slack_in_design = min(smallest_slack_in_design, f_timing_stats->least_slack[i][j]);
                    }
                }

                /* Increase all slacks by the value of the smallest slack in the design, if it's negative.
                Or clip all negative slacks to 0, based on how we choose to normalize slacks*/
                if (smallest_slack_in_design < 0) {
                    for (int inet = 0; inet < num_timing_nets; inet++) {
                        int num_edges = tnets[inet].num_sinks();
                        for (int iedge = 0; iedge < num_edges; iedge++) {
                            slacks->slack[inet][iedge + 1] -= smallest_slack_in_design; 
                            /* Remember, smallest_slack_in_design is negative, so we're INCREASING all the slacks. */
                            /* Note that if slack was equal to HUGE_POSITIVE_FLOAT, it will still be equal to more than this, 
                            so it will still be ignored when we calculate criticalities. */
                        }
                    }
                    criticality_denom_global -= smallest_slack_in_design;
                    /* Also need to increase the criticality denominator. In some cases, slack > criticality_denom_global, causing 
                    timing_criticality < 0 when calculated later. Do this shift to keep timing_criticality non-negative.
                    */
                }
            }

		    /* We can now calculate criticalities (for 'S', this must be done after we normalize slacks). */
		    for (int inet = 0; inet < num_timing_nets; inet++) {
			    int num_edges = tnets[inet].num_sinks();
			    for (int iedge = 0; iedge < num_edges; iedge++) {
				    if (slacks->slack[inet][iedge + 1] < HUGE_POSITIVE_FLOAT - 1) { /* if the slack is valid */
					    slacks->timing_criticality[inet][iedge + 1] = 1 - slacks->slack[inet][iedge + 1]/criticality_denom_global; 
					    VTR_ASSERT(slacks->timing_criticality[inet][iedge + 1] > -0.01);
					    VTR_ASSERT(slacks->timing_criticality[inet][iedge + 1] <  1.01);
				    }
				    /* otherwise, criticality remains 0, as it was initialized */
			    }
		    }
	    }		
    }
    free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);

    clock_t end = clock();
    timing_analysis_runtime += end - begin;
}

static float find_least_slack(bool is_prepacked, t_pb ***pin_id_to_pb_mapping) {
	/* Perform a simplified version of do_timing_analysis_for_constraint 
	to compute only the smallest slack in the design. 
    USED ONLY WHEN slack_definition == 'I'! */

	float smallest_slack_in_design = HUGE_POSITIVE_FLOAT;

	for (int source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (int sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			if (g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) { /* i.e. != DO_NOT_ANALYSE */

				/* Reset all arrival and required times. */
				for (int inode = 0; inode < num_tnodes; inode++) {
					tnode[inode].T_arr = HUGE_NEGATIVE_FLOAT; 
					tnode[inode].T_req = HUGE_POSITIVE_FLOAT;
				}

				/* Set arrival times for each top-level tnode on this source domain. */
				int num_at_level = tnodes_at_level[0].nelem;	
				for (int i = 0; i < num_at_level; i++) {
					int inode = tnodes_at_level[0].list[i];	
					if (tnode[inode].clock_domain == source_clock_domain) {
						if (tnode[inode].type == TN_FF_SOURCE) { 
							tnode[inode].T_arr = tnode[inode].clock_delay;
						} else if (tnode[inode].type == TN_INPAD_SOURCE) { 
							tnode[inode].T_arr = 0.;
						}
					}
				}

				/* Forward traversal, to compute arrival times. */
				for (int ilevel = 0; ilevel < num_tnode_levels; ilevel++) {	
					num_at_level = tnodes_at_level[ilevel].nelem;		

					for (int i = 0; i < num_at_level; i++) {				
						int inode = tnodes_at_level[ilevel].list[i];		
						if (tnode[inode].T_arr < NEGATIVE_EPSILON) {	
							continue;																
						}

						int num_edges = tnode[inode].num_edges;				
						t_tedge *tedge = tnode[inode].out_edges;					

						for (int iedge = 0; iedge < num_edges; iedge++) {
							int to_node = tedge[iedge].to_node;	
							if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

							tnode[to_node].T_arr = max(tnode[to_node].T_arr, tnode[inode].T_arr + tedge[iedge].Tdel);
						}
					}
				}

				/* Backward traversal, to compute required times and slacks.  We can skip nodes which are more than 
				1 away from a sink, because the path with the least slack has to use a connection between a sink 
				and one node away from a sink. */
				for (int ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--) {
					num_at_level = tnodes_at_level[ilevel].nelem;
	
					for (int i = 0; i < num_at_level; i++) {
						int inode = tnodes_at_level[ilevel].list[i];
						int num_edges = tnode[inode].num_edges;
						if (num_edges == 0) { /* sink */
							if (tnode[inode].type == TN_FF_CLOCK || tnode[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1
								|| tnode[inode].clock_domain != sink_clock_domain) {
								continue; 
							}

							float constraint;
							if (g_sdc->num_cf_constraints > 0) {
								char *source_domain_name = g_sdc->constrained_clocks[source_clock_domain].name;
								char *sink_register_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
								int icf = find_cf_constraint(source_domain_name, sink_register_name);
								if (icf != DO_NOT_ANALYSE) {
									constraint = g_sdc->cf_constraints[icf].constraint;
									if (constraint < NEGATIVE_EPSILON) { 
										continue;
									}
								} else {
									constraint = g_sdc->domain_constraint[source_clock_domain][sink_clock_domain];
								}
							} else { 
								constraint = g_sdc->domain_constraint[source_clock_domain][sink_clock_domain];
							}

							tnode[inode].T_req = constraint + tnode[inode].clock_delay;
						} else {

							if (tnode[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) { 
								continue; 
							}
				
							bool found = false;
							t_tedge *tedge = tnode[inode].out_edges;
							for (int iedge = 0; iedge < num_edges && !found; iedge++) { 
								int to_node = tedge[iedge].to_node;
								if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

								if(tnode[to_node].T_req < HUGE_POSITIVE_FLOAT) {
									found = true;
								}
							}
							if (!found) {
								continue;
							}

							for (int iedge = 0; iedge < num_edges; iedge++) {
								int to_node = tedge[iedge].to_node;
								if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

								if (tnode[to_node].num_edges == 0 && 
										tnode[to_node].clock_domain == sink_clock_domain) { // one away from a register on this sink domain
									float Tdel = tedge[iedge].Tdel;
									float T_req = tnode[to_node].T_req;
									smallest_slack_in_design = min(smallest_slack_in_design, T_req - Tdel - tnode[inode].T_arr);
								}
							}
						}
					}
				} 
			}
		}
	}

	return smallest_slack_in_design;
}

static float do_timing_analysis_for_constraint(int source_clock_domain, int sink_clock_domain, 
	bool is_prepacked, bool is_final_analysis, long * max_critical_input_paths_ptr, 
	long * max_critical_output_paths_ptr, t_pb ***pin_id_to_pb_mapping, const t_timing_inf &timing_inf) {
	
	/* Performs a single forward and backward traversal for the domain pair 
	source_clock_domain and sink_clock_domain. Returns the denominator that
	will be later used to normalize criticality - the maximum of all arrival 
	times from this traversal and the constraint for this pair of domains. 
	Also returns the maximum number of critical input and output paths of any 
	node analysed for this constraint, passed by reference from do_timing_analysis. */

	int inode, num_at_level, i, total, ilevel, num_edges, iedge, to_node;
	float constraint, Tdel, T_req; 
	
	float max_Tarr = HUGE_NEGATIVE_FLOAT; /* Max of all arrival times for this constraint - 
											 used to relax required times for slack_definition 'R' and 'G'
                                             and to normalize slacks for slack_definition 'N'. */

	float max_Treq = HUGE_NEGATIVE_FLOAT; /* Max of all required time for this constraint - 
											 used in criticality denominator. */

	t_tedge * tedge;
	int num_dangling_nodes;
	bool found;
	long max_critical_input_paths = 0, max_critical_output_paths = 0;

    /* If not passed in, alloc and load pin_id_to_pb_mapping (and make sure to free later). */
    bool must_free_mapping = false;
    if (!pin_id_to_pb_mapping) {
        pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();
        must_free_mapping = true;
    }
    
	/* Reset all values which need to be reset once per 
	traversal pair, rather than once per timing analysis. */

	/* Reset all arrival and required times. */
	for (inode = 0; inode < num_tnodes; inode++) {
		tnode[inode].T_arr = HUGE_NEGATIVE_FLOAT; 
		tnode[inode].T_req = HUGE_POSITIVE_FLOAT;
	}

#ifndef PATH_COUNTING
	/* Reset num_critical_output_paths. */
	if (is_prepacked) {
		for (inode = 0; inode < num_tnodes; inode++) {
			tnode[inode].prepacked_data->num_critical_output_paths = 0;
		}
	}
#endif

	/* Set arrival times for each top-level tnode on this source domain. */
	num_at_level = tnodes_at_level[0].nelem;	
	for (i = 0; i < num_at_level; i++) {
		inode = tnodes_at_level[0].list[i];	

		if (tnode[inode].clock_domain == source_clock_domain) {

			if (tnode[inode].type == TN_FF_SOURCE) { 
				/* Set the arrival time of this flip-flop tnode to its clock skew. */
				tnode[inode].T_arr = tnode[inode].clock_delay;

			} else if (tnode[inode].type == TN_INPAD_SOURCE) { 
				/* There's no such thing as clock skew for external clocks, and
				input delay is already marked on the edge coming out from this node. 
				As a result, the signal can be said to arrive at t = 0. */
				tnode[inode].T_arr = 0.;
			}

		}
	}

	/* Compute arrival times with a forward topological traversal from sources
	(TN_FF_SOURCE, TN_INPAD_SOURCE, TN_CONSTANT_GEN_SOURCE) to sinks (TN_FF_SINK, TN_OUTPAD_SINK). */

	total = 0;												/* We count up all tnodes to error-check at the end. */
	for (ilevel = 0; ilevel < num_tnode_levels; ilevel++) {	/* For each level of our levelized timing graph... */
		num_at_level = tnodes_at_level[ilevel].nelem;		/* ...there are num_at_level tnodes at that level. */
		total += num_at_level;
		
		for (i = 0; i < num_at_level; i++) {					
			inode = tnodes_at_level[ilevel].list[i];		/* Go through each of the tnodes at the level we're on. */
			if (tnode[inode].T_arr < NEGATIVE_EPSILON) {	/* If the arrival time is less than 0 (i.e. HUGE_NEGATIVE_FLOAT)... */
				continue;									/* End this iteration of the num_at_level for loop since 
															this node is not part of the clock domain we're analyzing. 
															(If it were, it would have received an arrival time already.) */
			}

			num_edges = tnode[inode].num_edges;				/* Get the number of edges fanning out from the node we're visiting */
			tedge = tnode[inode].out_edges;					/* Get the list of edges from the node we're visiting */
#ifndef PATH_COUNTING		
			if (is_prepacked && ilevel == 0) {
				tnode[inode].prepacked_data->num_critical_input_paths = 1;	/* Top-level tnodes have one locally-critical input path. */
			}

			/* Using a somewhat convoluted procedure inherited from T-VPack, 
			count how many locally-critical input paths fan into each tnode,
			and also find the maximum number over all tnodes. */
			if (is_prepacked) {
				for (iedge = 0; iedge < num_edges; iedge++) {		
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

					if (fabs(tnode[to_node].T_arr - (tnode[inode].T_arr + tedge[iedge].Tdel)) < EPSILON) {
						/* If the "local forward slack" (T_arr(to_node) - T_arr(inode) - T_del) for this edge 
						is 0 (i.e. the path from inode to to_node is locally as critical as any other path to
						to_node), add to_node's num critical input paths to inode's number. */
						tnode[to_node].prepacked_data->num_critical_input_paths += tnode[inode].prepacked_data->num_critical_input_paths;
					} else if (tnode[to_node].T_arr < (tnode[inode].T_arr + tedge[iedge].Tdel)) {
						/* If the "local forward slack" for this edge is negative,
						reset to_node's num critical input paths to inode's number. */
						tnode[to_node].prepacked_data->num_critical_input_paths = tnode[inode].prepacked_data->num_critical_input_paths;
					}
					/* Set max_critical_input_paths to the maximum number of critical 
					input paths for all tnodes analysed on this traversal. */
					if (tnode[to_node].prepacked_data->num_critical_input_paths	> max_critical_input_paths) {
						max_critical_input_paths = tnode[to_node].prepacked_data->num_critical_input_paths;
					}
				}
			}
#endif			
            for (iedge = 0; iedge < num_edges; iedge++) {	/* Now go through each edge coming out from this tnode */
                to_node = tedge[iedge].to_node;				/* Get the index of the destination tnode of this edge. */
                if (to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

                /* The arrival time T_arr at the destination node is set to the maximum of all
                the possible arrival times from all edges fanning in to the node. The arrival
                time represents the latest time that all inputs must arrive at a node. */
                tnode[to_node].T_arr = max(tnode[to_node].T_arr, tnode[inode].T_arr + tedge[iedge].Tdel);

                if (timing_inf.slack_definition == 'R' || timing_inf.slack_definition == 'G') {
                    /* Since we updated the destination node (to_node), change the max arrival  
                    time for the forward traversal if to_node's arrival time is greater than 
                    the existing maximum, and it is on the sink clock domain. */
                    if (tnode[to_node].num_edges == 0 && tnode[to_node].clock_domain == sink_clock_domain) {
			max_Tarr = max(max_Tarr, tnode[to_node].T_arr);
                    }
                }
			}
		}
	}
	
	VTR_ASSERT(total == num_tnodes);
	num_dangling_nodes = 0;
	/* Compute required times with a backward topological traversal from sinks to sources. */
	
	for (ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--) {
		num_at_level = tnodes_at_level[ilevel].nelem;
	
		for (i = 0; i < num_at_level; i++) {
			inode = tnodes_at_level[ilevel].list[i];
			num_edges = tnode[inode].num_edges;
	
			if (ilevel == 0) {
				if (!(tnode[inode].type == TN_INPAD_SOURCE || tnode[inode].type == TN_FF_SOURCE || tnode[inode].type == TN_CONSTANT_GEN_SOURCE ||
                      tnode[inode].type == TN_CLOCK_SOURCE) && !tnode[inode].is_comb_loop_breakpoint) {
					//We suppress node type errors if they have the is_comb_loop_breakpoint flag set.
					//The flag denotes that an input edge to this node was disconnected to break a combinational
					//loop, and hence we don't consider this an error.
					vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__, 
							"Timing graph started on unexpected node %d %s.%s[%d]. "
							"This is a VPR internal error, contact VPR development team.\n",
							tnode[inode].pb_graph_pin->parent_node->pb_type->name, 
							tnode[inode].pb_graph_pin->port->name, 
							tnode[inode].pb_graph_pin->pin_number);
				}
			} else {
				if ((tnode[inode].type == TN_INPAD_SOURCE || tnode[inode].type == TN_FF_SOURCE || tnode[inode].type == TN_CONSTANT_GEN_SOURCE)) {
					vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__, 
							"Timing graph discovered unexpected edge to node %s.%s[%d].\n"
							"This is a VPR internal error, contact VPR development team.\n",
							tnode[inode].pb_graph_pin->parent_node->pb_type->name, 
							tnode[inode].pb_graph_pin->port->name, 
							tnode[inode].pb_graph_pin->pin_number);
				}
			}
	
			/* Unlike the forward traversal, the sinks are all on different levels, so we always have to
			check whether a node is a sink. We give every sink on the sink clock domain we're considering
			a valid required time. Every non-sink node in the fanin of one of these sinks and the fanout of
			some source from the forward traversal also gets a valid required time. */
			
			if (num_edges == 0) { /* sink */

				if (tnode[inode].type == TN_FF_CLOCK || tnode[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) {
					continue; /* Skip nodes on the clock net itself, and nodes with unset arrival times. */
				}
	
				if (!(tnode[inode].type == TN_OUTPAD_SINK || tnode[inode].type == TN_FF_SINK)) {
					if(is_prepacked) {
						vtr::printf_warning(__FILE__, __LINE__, 
								"Pin on block %s.%s[%d] not used\n", 
								logical_block[tnode[inode].block].name, 
								tnode[inode].prepacked_data->model_port_ptr->name, 
								tnode[inode].prepacked_data->model_pin);
					}
					num_dangling_nodes++;
					/* Note: Still need to do standard traversals with dangling pins so that algorithm runs properly, but T_arr and T_Req to values such that it dangling nodes do not affect actual timing values */
				}
		
				/* Skip nodes not on the sink clock domain of the 
				constraint we're currently considering */
				if (tnode[inode].clock_domain != sink_clock_domain) { 
					continue;
				}
	
				/* See if there's an override constraint between the source clock domain (name is 
				g_sdc->constrained_clocks[source_clock_domain].name) and the flip-flop or outpad we're at 
				now (name is find_tnode_net_name(inode, is_prepacked)). We test if 
				g_sdc->num_cf_constraints > 0 first so that we can save time looking up names in the vast 
				majority of cases where there are no such constraints. */

				if (g_sdc->num_cf_constraints > 0) {
					char *source_domain_name = g_sdc->constrained_clocks[source_clock_domain].name;
					char *sink_register_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
					int icf = find_cf_constraint(source_domain_name, sink_register_name);
					if (icf != DO_NOT_ANALYSE) {
						constraint = g_sdc->cf_constraints[icf].constraint;
						if (constraint < NEGATIVE_EPSILON) { 
							/* Constraint is DO_NOT_ANALYSE (-1) for this particular sink. */
							continue;
						}
					} else {
						/* Use the default constraint from g_sdc->domain_constraint. */
						constraint = g_sdc->domain_constraint[source_clock_domain][sink_clock_domain];
						/* Constraint is guaranteed to be valid since we checked for it at the very beginning. */
					}
				} else { 
					constraint = g_sdc->domain_constraint[source_clock_domain][sink_clock_domain];
				}
	
				/* Now we know we should analyse this tnode. */
	
                if (timing_inf.slack_definition == 'R' || timing_inf.slack_definition == 'G') {
				    /* Assign the required time T_req for this leaf node, taking into account clock skew. T_req is the 
				    time all inputs to a tnode must arrive by before it would degrade this constraint's critical path delay. 

				    Relax the required time at the sink node to be non-negative by taking the max of the "real" required 
				    time (constraint + tnode[inode].clock_delay) and the max arrival time in this domain (max_Tarr), except
				    for the final analysis where we report actual slack. We do this to prevent any slacks from being 
				    negative, since negative slacks are not used effectively by the optimizers.

				    E.g. if we have a 10 ns constraint and it takes 14 ns to get here, we'll have a slack of at most -4 ns 
				    for any edge along the path that got us here.  If we say the required time is 14 ns (no less than the 
				    arrival time), we don't have a negative slack anymore.  However, in the final timing analysis, the real 
				    slacks are computed (that's what human designers care about), not the relaxed ones. */	
	
				    if (is_final_analysis) {
					    tnode[inode].T_req = constraint + tnode[inode].clock_delay;
				    } else {
					    tnode[inode].T_req = max(constraint + tnode[inode].clock_delay, max_Tarr);
				    }
                } else {
					/* Don't do the relaxation and always set T_req equal to the "real" required time. */
				    tnode[inode].T_req = constraint + tnode[inode].clock_delay;
                }

				max_Treq = max(max_Treq, tnode[inode].T_req);
							
				/* Store the largest critical path delay for this constraint (source domain AND sink domain) 
				in the matrix critical_path_delay. C.P.D. = T_arr at destination - clock skew at destination 
				= (datapath delay + clock delay to source) - clock delay to destination.

				Critical path delay is really telling us how fast we can run the source clock before we can no
				longer meet this constraint.  e.g. If the datapath delay is 10 ns, the clock delay at source is
				2 ns and the clock delay at destination is 5 ns, then C.P.D. is 7 ns by the above formula. We 
				can run the source clock at 7 ns because the clock skew gives us 3 ns extra to meet the 10 ns 
				datapath delay. */

				f_timing_stats->cpd[source_clock_domain][sink_clock_domain] = 
					max(f_timing_stats->cpd[source_clock_domain][sink_clock_domain], 
					   (tnode[inode].T_arr - tnode[inode].clock_delay)); 

#ifndef PATH_COUNTING
				if (is_prepacked) {
					tnode[inode].prepacked_data->num_critical_output_paths = 1; /* Bottom-level tnodes have one locally-critical input path. */
				}
#endif
			} else { /* not a sink */

				VTR_ASSERT(!(tnode[inode].type == TN_OUTPAD_SINK || tnode[inode].type == TN_FF_SINK || tnode[inode].type == TN_FF_CLOCK));

				/* We need to skip this node unless it is on a path from source_clock_domain to 
				sink_clock_domain.  We need to skip all nodes which:
				   1. Fan out to the sink domain but do not fan in from the source domain. 
				   2. Fan in from the source domain but do not fan out to the sink domain.
				   3. Do not fan in or out to either domain. 
				If a node does not fan in from the source domain, it will not have a valid arrival time.  
				So cases 1 and 3 can be skipped by continuing if T_arr = HUGE_NEGATIVE_FLOAT. We cannot 
				treat case 2 as simply since the required time for this node has not yet been assigned, 
				so we have to look at the required time for every node in its immediate fanout instead. */

				/* Cases 1 and 3 */
				if (tnode[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) { 
					continue; /* Skip nodes with unset arrival times. */
				}
				
				/* Case 2 */
				found = false;
				tedge = tnode[inode].out_edges;
				for (iedge = 0; iedge < num_edges && !found; iedge++) { 
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
					
					if (tnode[to_node].T_req < HUGE_POSITIVE_FLOAT) {
					    found = true;
					}
				}
				if (!found) {
					continue;
				}

				/* Now we know this node is on a path from source_clock_domain to 
				sink_clock_domain, and needs to be analyzed. */

				/* Opposite to T_arr, set T_req to the MINIMUM of the 
				required times of all edges fanning OUT from this node. */
				for (iedge = 0; iedge < num_edges; iedge++) {
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
					
					Tdel = tedge[iedge].Tdel;
					T_req = tnode[to_node].T_req;
					tnode[inode].T_req = min(tnode[inode].T_req, T_req - Tdel);

					/* Update least slack per constraint. This is NOT the same as the minimum slack we will
					calculate on this traversal for post-packed netlists, which only count inter-cluster 
					slacks. We only look at edges adjacent to sink nodes on the sink clock domain since
					all paths go through one of these edges. */
					if (tnode[to_node].num_edges == 0 && tnode[to_node].clock_domain == sink_clock_domain) {
					    f_timing_stats->least_slack[source_clock_domain][sink_clock_domain] = 
					        min(f_timing_stats->least_slack[source_clock_domain][sink_clock_domain],
					           (T_req - Tdel - tnode[inode].T_arr)); 
					}
				}
#ifndef PATH_COUNTING

				/* Similar to before, we count how many locally-critical output paths fan out from each tnode, 
				and also find the maximum number over all tnodes. Unlike for input paths, where we haven't set
				the arrival time at to_node before analysing it, here the required time is set at both nodes,
				so the "local backward slack" (T_req(to_node) - T_req(inode) - T_del) will never be negative.  
				Hence, we only have to test if the "local backward slack" is 0. */
				if (is_prepacked) {
					for (iedge = 0; iedge < num_edges; iedge++) { 
						to_node = tedge[iedge].to_node;
						if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
						
						/* If the "local backward slack" (T_arr(to_node) - T_arr(inode) - T_del) for this edge 
						is 0 (i.e. the path from inode to to_node is locally as critical as any other path to
						to_node), add to_node's num critical output paths to inode's number. */
						if (fabs(tnode[to_node].T_req - (tnode[inode].T_req + tedge[iedge].Tdel)) < EPSILON) {
						    tnode[inode].prepacked_data->num_critical_output_paths += tnode[to_node].prepacked_data->num_critical_output_paths;
						}
						/* Set max_critical_output_paths to the maximum number of critical 
						output paths for all tnodes analysed on this traversal. */
						if (tnode[to_node].prepacked_data->num_critical_output_paths > max_critical_output_paths) {
						    max_critical_output_paths = tnode[to_node].prepacked_data->num_critical_output_paths;
						}
					}
				}
#endif
			}
		}
	}

    if (must_free_mapping) {
        free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);
    }

	/* Return max critical input/output paths for this constraint through 
	the pointers we passed in. */
	if (max_critical_input_paths_ptr && max_critical_output_paths_ptr) {
		*max_critical_input_paths_ptr = max_critical_input_paths;
		*max_critical_output_paths_ptr = max_critical_output_paths;
	}

	if(num_dangling_nodes > 0 && (is_final_analysis || is_prepacked)) {
		vtr::printf_warning(__FILE__, __LINE__, 
				"%d unused pins \n",  num_dangling_nodes);
	}

	/* The criticality denominator is the maximum required time, except for slack_definition == 'N'
	where it is the max of all arrival and required times.

	For definitions except 'R' and 'N', this works out to the maximum of constraint + 
	tnode[inode].clock_delay over all nodes.  For 'R' and 'N', this works out to the maximum of
	that value and max_Tarr.  The max_Tarr is implicitly incorporated into the denominator through
	its inclusion in the required time, but we have to explicitly include it for 'N'. */
    if (timing_inf.slack_definition == 'N') {
        return max_Treq + max_Tarr;
    } else {
        return max_Treq;
    }
}
#ifdef PATH_COUNTING
static void do_path_counting(float criticality_denom) {
	/* Count the importance of the number of paths going through each net 
	by giving each net a forward and backward path weight. This is the first step of
	"A Novel Net Weighting Algorithm for Timing-Driven Placement" (Kong, 2002). 
	We only visit nodes with set arrival and required times, so this function 
	must be called after do_timing_analysis_for_constraints, which sets T_arr and T_req. */
	
	int inode, num_at_level, i, ilevel, num_edges, iedge, to_node;
	t_tedge * tedge;
	float forward_local_slack, backward_local_slack, discount;

	/* Reset forward and backward weights for all tnodes. */
	for (inode = 0; inode < num_tnodes; inode++) {
		tnode[inode].forward_weight = 0;
		tnode[inode].backward_weight = 0;
	}

	/* Set foward weights for each top-level tnode. */
	num_at_level = tnodes_at_level[0].nelem;	
	for (i = 0; i < num_at_level; i++) {
		inode = tnodes_at_level[0].list[i];	
		tnode[inode].forward_weight = 1.;
	}

	/* Do a forward topological traversal to populate forward weights. */
	for (ilevel = 0; ilevel < num_tnode_levels; ilevel++) {	
		num_at_level = tnodes_at_level[ilevel].nelem;			
		
		for (i = 0; i < num_at_level; i++) {					
			inode = tnodes_at_level[ilevel].list[i];			
			if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
				continue;	
			}
			tedge = tnode[inode].out_edges;
			num_edges = tnode[inode].num_edges;
			for (iedge = 0; iedge < num_edges; iedge++) {
				to_node = tedge[iedge].to_node;
				if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
				
				if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
				    continue;	
				}
				forward_local_slack = tnode[to_node].T_arr - tnode[inode].T_arr - tedge[iedge].Tdel;
				discount = pow((float) DISCOUNT_FUNCTION_BASE, -1 * forward_local_slack / criticality_denom);
				tnode[to_node].forward_weight += discount * tnode[inode].forward_weight;
			}
		}
	}

	/* Do a backward topological traversal to populate backward weights. 
	Since the sinks are all on different levels, we have to check for them as we go. */
	for (ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--) {
		num_at_level = tnodes_at_level[ilevel].nelem;
	
		for (i = 0; i < num_at_level; i++) {
			inode = tnodes_at_level[ilevel].list[i];
			if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
				continue;	
			}
			num_edges = tnode[inode].num_edges;
			if (num_edges == 0) { /* sink */
				tnode[inode].backward_weight = 1.;
			} else {
				tedge = tnode[inode].out_edges;
				for (iedge = 0; iedge < num_edges; iedge++) {
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
					if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
					    continue;	
					}
					backward_local_slack = tnode[to_node].T_req - tnode[inode].T_req - tedge[iedge].Tdel;
					discount = pow((float) DISCOUNT_FUNCTION_BASE, -1 * backward_local_slack / criticality_denom);
					tnode[inode].backward_weight += discount * tnode[to_node].backward_weight;
				}
			}
		}
	}
}
#endif

static void update_slacks(t_slack * slacks, int source_clock_domain, int sink_clock_domain, float criticality_denom,
    bool update_slack, bool is_final_analysis, float smallest_slack_in_design, const t_timing_inf &timing_inf) {

	/* Updates the slack and criticality of each sink pin, or equivalently 
	each edge, of each net. Go through the list of nets. If the net's 
	driver tnode has been used,	go through each tedge of that tnode and 
	take the minimum of the slack/criticality for this traversal and the 
	existing values. Since the criticality_denom can vary greatly between 
	traversals, we have to update slack and criticality separately. 
	Only update slack if we need to print it later (update_slack == true). 
	
	Note: there is a correspondence in indexing between out_edges and the 
	net data structure: out_edges[iedge] = net[inet].node_block[iedge + 1]
	There is an offset of 1 because net[inet].node_block includes the driver
	node at index 0, while out_edges is part of the driver node and does
	not bother to refer to itself. */

	int inet, iedge, inode, to_node, num_edges;
	t_tedge *tedge;
	float T_arr, Tdel, T_req, slack;
	float timing_criticality;

	for (inet = 0; inet < num_timing_nets; inet++) {
		inode = f_net_to_driver_tnode[inet];
		T_arr = tnode[inode].T_arr;

		if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
			continue; /* Only update this net on this traversal if its 
					  driver node has been updated on this traversal. */
		}

		num_edges = tnode[inode].num_edges;
		tedge = tnode[inode].out_edges;

		for (iedge = 0; iedge < num_edges; iedge++) {
			to_node = tedge[iedge].to_node;
			if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

			if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
				continue; /* Only update this edge on this traversal if this 
						  particular sink node has been updated on this traversal. */
			}
			Tdel = tedge[iedge].Tdel;
			T_req = tnode[to_node].T_req;

			slack = T_req - T_arr - Tdel;

			if (!is_final_analysis) {
                if (timing_inf.slack_definition == 'I') {
                    /* Shift slack UPWARDS by subtracting the smallest slack in the
                    design (which is negative or zero). */
                    slack -= smallest_slack_in_design;
                } else if (timing_inf.slack_definition == 'C') {
				    /* Clip all negative slacks to 0. */
				    if (slack < 0) slack = 0;
                }
			}
			
			if (update_slack) {
				/* Update the slack for this edge if slk is the new minimum value */		
				slacks->slack[inet][iedge + 1] = min(slack, slacks->slack[inet][iedge + 1]);
			}

            if (timing_inf.slack_definition != 'S' && timing_inf.slack_definition != 'G') {
                if (!is_final_analysis) { // Criticality is not meaningful using non-normalized slacks.

                    /* Since criticality_denom is not the same on each traversal,
                    we have to update criticality separately. */

                    if (timing_inf.slack_definition == 'C') {
                        /* For clipped, criticality_denom is the raw maximum required time, and this can
                        be 0 if the constraint was 0, leading to division by 0.  In this case, all of the
                        slacks will be clipped to zero anyways, so we can just set the criticality to 1. */
                        timing_criticality = criticality_denom ? 1 - slack / criticality_denom : 1;
                    } else {
                        // VTR_ASSERT(criticality_denom);
                        timing_criticality = 1 - slack / criticality_denom;
                    }

                    // Criticality must be normalized to between 0 and 1 
                    // Note: floating-point error may be ~1e-5 since we are dividing by a small
                    // criticality_denom (~1e-9).
                    VTR_ASSERT(timing_criticality > -0.01);
                    VTR_ASSERT(timing_criticality <  1.01);

                    slacks->timing_criticality[inet][iedge + 1] =
                        max(timing_criticality, slacks->timing_criticality[inet][iedge + 1]);

#ifdef PATH_COUNTING
                    /* Also update path criticality separately.  Kong uses slack / T_crit for the exponent, 
                    which is equivalent to criticality - 1.  Depending on how PATH_COUNTING is defined,
                    different functional forms are used. */
#if PATH_COUNTING == 'S' /* Use sum of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1],
                        (tnode[inode].forward_weight + tnode[to_node].backward_weight) * 
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'P' /* Use product of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1],
                        tnode[inode].forward_weight * tnode[to_node].backward_weight * 
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'L' /* Use natural log of product of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1], 
                        log(tnode[inode].forward_weight * tnode[to_node].backward_weight) * 
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'R' /* Use product of natural logs of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1], 
                        log(tnode[inode].forward_weight) * log(tnode[to_node].backward_weight) * 
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#endif				
#endif
                }
            }
		}
	}
}


void print_critical_path(const char *fname, const t_timing_inf &timing_inf) {

	/* Prints the critical path to a file. */

	vtr::t_linked_int *critical_path_head, *critical_path_node;
	FILE *fp;
	int non_global_nets_on_crit_path, global_nets_on_crit_path;
	int tnodes_on_crit_path, inode, iblk, inet;
	e_tnode_type type;
	float total_net_delay, total_logic_delay, Tdel;

	vector<t_vnet> & tnets = *timing_nets; 
	t_pb*** pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();

	critical_path_head = allocate_and_load_critical_path(timing_inf);
	critical_path_node = critical_path_head;

	fp = vtr::fopen(fname, "w");

	non_global_nets_on_crit_path = 0;
	global_nets_on_crit_path = 0;
	tnodes_on_crit_path = 0;
	total_net_delay = 0.;
	total_logic_delay = 0.;

	while (critical_path_node != NULL) {
		Tdel = print_critical_path_node(fp, critical_path_node, pin_id_to_pb_mapping);
		inode = critical_path_node->data;
		type = tnode[inode].type;
		tnodes_on_crit_path++;

		if (type == TN_CB_OPIN) {
			get_tnode_block_and_output_net(inode, &iblk, &inet);

			if (!tnets[inet].is_global)
				non_global_nets_on_crit_path++;
			else
				global_nets_on_crit_path++;

			total_net_delay += Tdel;
		} else {
			total_logic_delay += Tdel;
		}

		critical_path_node = critical_path_node->next;
	}

	fprintf(fp, "\nTnodes on critical path: %d  Non-global nets on critical path: %d."
			"\n", tnodes_on_crit_path, non_global_nets_on_crit_path);
	fprintf(fp, "Global nets on critical path: %d.\n", global_nets_on_crit_path);
	fprintf(fp, "Total logic delay: %g (s)  Total net delay: %g (s)\n",
			total_logic_delay, total_net_delay);

	vtr::printf_info("Nets on critical path: %d normal, %d global.\n",
			non_global_nets_on_crit_path, global_nets_on_crit_path);

	vtr::printf_info("Total logic delay: %g (s), total net delay: %g (s)\n",
			total_logic_delay, total_net_delay);

	/* Make sure total_logic_delay and total_net_delay add 
	up to critical path delay,within 5 decimal places. */
	VTR_ASSERT(total_logic_delay + total_net_delay - get_critical_path_delay()/1e9 < 1e-5);

	fclose(fp);
	free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);
	free_int_list(&critical_path_head);
}

vtr::t_linked_int * allocate_and_load_critical_path(const t_timing_inf &timing_inf) {

	/* Finds the critical path and returns a list of the tnodes on the critical *
	 * path in a linked list, from the path SOURCE to the path SINK.			*/

	vtr::t_linked_int *critical_path_head, *curr_crit_node, *prev_crit_node;
	int inode, iedge, to_node, num_at_level_zero, i, j, crit_node = OPEN, num_edges;
	int source_clock_domain = UNDEFINED, sink_clock_domain = UNDEFINED;
	float min_slack = HUGE_POSITIVE_FLOAT, slack;
	t_tedge *tedge;

	/* If there's only one clock, we can use the arrival and required times 
	currently on the timing graph to find the critical path. If there are multiple 
	clocks, however, the values currently on the timing graph correspond to the 
	last constraint (pair of clock domains) analysed, which may not be the constraint 
	with the critical path.	In this case, we have to find the constraint with the 
	least slack and redo the timing analysis for this constraint so we get the right
	values onto the timing graph. */

	if (g_sdc->num_constrained_clocks > 1) {
		/* The critical path belongs to the source and sink clock domains 
		with the least slack. Find these clock domains now. */

		for (i = 0; i < g_sdc->num_constrained_clocks; i++) {
			for (j = 0; j < g_sdc->num_constrained_clocks; j++) {
				// Use the true, unrelaxed, least slack (constraint - critical path delay).
				slack = g_sdc->domain_constraint[i][j] - f_timing_stats->cpd[i][j];
				if (slack < min_slack) {
					min_slack = slack;
					source_clock_domain = i;
					sink_clock_domain = j;
				}
			}
		}

		/* Do a timing analysis for this clock domain pair only. 
		Set is_prepacked to false since we don't care about the clusterer's normalized values. 
		Set is_final_analysis to false to get actual, rather than relaxed, slacks.
		Set max critical input/output paths to NULL since they aren't used unless is_prepacked is true. */
		do_timing_analysis_for_constraint(source_clock_domain, sink_clock_domain, false, false, NULL, NULL, NULL, timing_inf);
	}

	/* Start at the source (level-0) tnode with the least slack (T_req-T_arr). 
	This will form the head of our linked list of tnodes on the critical path. */
	min_slack = HUGE_POSITIVE_FLOAT;
	num_at_level_zero = tnodes_at_level[0].nelem;
	for (i = 0; i < num_at_level_zero; i++) { 
		inode = tnodes_at_level[0].list[i];
		if (has_valid_T_arr(inode) && has_valid_T_req(inode)) { /* Valid arrival and required times */
			slack = tnode[inode].T_req - tnode[inode].T_arr;
			if (slack < min_slack) {
				crit_node = inode;
				min_slack = slack;
			}
		}
	}
	critical_path_head = (vtr::t_linked_int *) vtr::malloc(sizeof(vtr::t_linked_int));
	critical_path_head->data = crit_node;
	VTR_ASSERT(crit_node != OPEN);
	prev_crit_node = critical_path_head;
	num_edges = tnode[crit_node].num_edges;

	/* Keep adding the tnode in this tnode's fanout which has the least slack
	to our critical path linked list, then jump to that tnode and repeat, until
	we hit a tnode with no edges, which is the sink of the critical path. */
	while (num_edges != 0) { 
		curr_crit_node = (vtr::t_linked_int *) vtr::malloc(sizeof(vtr::t_linked_int));
		prev_crit_node->next = curr_crit_node;
		tedge = tnode[crit_node].out_edges;
		min_slack = HUGE_POSITIVE_FLOAT;

		for (iedge = 0; iedge < num_edges; iedge++) {
			to_node = tedge[iedge].to_node;
			if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

			if (has_valid_T_arr(to_node) && has_valid_T_req(to_node)) { /* Valid arrival and required times */
				slack = tnode[to_node].T_req - tnode[to_node].T_arr;
				if (slack < min_slack) {
					crit_node = to_node;
					min_slack = slack;
				}
			}
		}

		curr_crit_node->data = crit_node;
		prev_crit_node = curr_crit_node;
		num_edges = tnode[crit_node].num_edges;
	}

	prev_crit_node->next = NULL;
	return (critical_path_head);
}

void get_tnode_block_and_output_net(int inode, int *iblk_ptr, int *inet_ptr) {

	/* Returns the index of the block that this tnode is part of.  If the tnode *
	 * is a TN_CB_OPIN or TN_INPAD_OPIN (i.e. if it drives a net), the net index is  *
	 * returned via inet_ptr.  Otherwise inet_ptr points at OPEN.               */

	int inet, ipin, iblk;
	e_tnode_type tnode_type;

	iblk = tnode[inode].block;
	tnode_type = tnode[inode].type;

	if (tnode_type == TN_CB_OPIN) {
		ipin = tnode[inode].pb_graph_pin->pin_count_in_cluster;
		inet = block[iblk].pb_route[ipin].atom_net_idx;
		VTR_ASSERT(inet != OPEN);
		inet = vpack_to_clb_net_mapping[inet];
	} else {
		inet = OPEN;
	}

	*iblk_ptr = iblk;
	*inet_ptr = inet;
}

void do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
		float constant_net_delay_value) {

	/* Does a timing analysis (simple) where it assumes that each net has a      *
	 * constant delay value.  Used only when operation == TIMING_ANALYSIS_ONLY.  */

	vtr::t_chunk net_delay_ch = {NULL, 0, NULL};
	t_slack * slacks = NULL;
	float **net_delay = NULL;
	
	slacks = alloc_and_load_timing_graph(timing_inf);
	net_delay = alloc_net_delay(&net_delay_ch, *timing_nets,
		(*timing_nets).size());

	load_constant_net_delay(net_delay, constant_net_delay_value, *timing_nets,
		(*timing_nets).size());
	load_timing_graph_net_delays(net_delay);
	
	do_timing_analysis(slacks, timing_inf, false, true);

	if (getEchoEnabled()) {
		if(isEchoFileEnabled(E_ECHO_CRITICAL_PATH))
			print_critical_path("critical_path.echo", timing_inf);
		if(isEchoFileEnabled(E_ECHO_TIMING_GRAPH))
			print_timing_graph(getEchoFileName(E_ECHO_TIMING_GRAPH));
		if(isEchoFileEnabled(E_ECHO_SLACK))
			print_slack(slacks->slack, true, getEchoFileName(E_ECHO_SLACK));
		if(isEchoFileEnabled(E_ECHO_NET_DELAY))
			print_net_delay(net_delay, getEchoFileName(E_ECHO_NET_DELAY));
	}

	print_timing_stats();

	free_timing_graph(slacks);
	free_net_delay(net_delay, &net_delay_ch);
}
#ifndef PATH_COUNTING
static void update_normalized_costs(float criticality_denom, long max_critical_input_paths,
    long max_critical_output_paths, const t_timing_inf &timing_inf) {
    int inode;

    /* Update the normalized costs for the clusterer.  On each traversal, each cost is
    updated for tnodes analysed on this traversal if it would give this tnode a higher
    criticality when calculating block criticality for the clusterer. */

    if (timing_inf.slack_definition == 'R' || timing_inf.slack_definition == 'I') {
        VTR_ASSERT(criticality_denom != 0); /* Possible if timing analysis is being run pre-packing
                                        with all delays set to 0. This is not currently done, 
                                        but if you're going to do it, you need to decide how 
                                        best to normalize these values to suit your purposes. */
    }

	for (inode = 0; inode < num_tnodes; inode++) {
		/* Only calculate for tnodes which have valid arrival and required times. */
		if (has_valid_T_arr(inode) && has_valid_T_req(inode)) {
			tnode[inode].prepacked_data->normalized_slack = min(tnode[inode].prepacked_data->normalized_slack, 
				(tnode[inode].T_req - tnode[inode].T_arr)/criticality_denom);
			tnode[inode].prepacked_data->normalized_T_arr = max(tnode[inode].prepacked_data->normalized_T_arr, 
				tnode[inode].T_arr/criticality_denom);
			tnode[inode].prepacked_data->normalized_total_critical_paths = max(tnode[inode].prepacked_data->normalized_total_critical_paths, 
					((float) tnode[inode].prepacked_data->num_critical_input_paths + tnode[inode].prepacked_data->num_critical_output_paths) /
							 ((float) max_critical_input_paths + max_critical_output_paths));
		}
	}
}
#endif


static void load_clock_domain_and_clock_and_io_delay(bool is_prepacked, int **lookup_tnode_from_pin_id, t_pb*** pin_id_to_pb_mapping) {
/* Loads clock domain and clock delay onto TN_FF_SOURCE and TN_FF_SINK tnodes.
The clock domain of each clock is its index in g_sdc->constrained_clocks.
We do this by matching each clock input pad (TN_INPAD_SOURCE), or internal clock 
source (TN_CLOCK_SOURCE) to a constrained clock name, then propagating forward its 
domain index to all flip-flops fed by it (TN_FF_CLOCK tnodes), then later looking 
up the TN_FF_CLOCK tnode corresponding to every TN_FF_SOURCE and TN_FF_SINK tnode. 
We also add up the delays along the clock net to each TN_FF_CLOCK tnode to give it 
(and the SOURCE/SINK nodes) a clock delay.

Also loads input delay/output delay (from set_input_delay or set_output_delay SDC 
constraints) onto the tedges between TN_INPAD_SOURCE/OPIN and TN_OUTPAD_IPIN/SINK 
tnodes.  Finds fanout of each clock domain, including virtual (external) clocks.  
Marks unconstrained I/Os with a dummy clock domain (-1). */

	int i, iclock, inode, num_at_level, clock_index, input_index, output_index;
	char * net_name;
	t_tnode * clock_node;

	/* Wipe fanout of each clock domain in g_sdc->constrained_clocks. */
	for (iclock = 0; iclock < g_sdc->num_constrained_clocks; iclock++) {
		g_sdc->constrained_clocks[iclock].fanout = 0;
	}

	/* First, visit all TN_INPAD_SOURCE and TN_CLOCK_SOURCE tnodes 
     * We only need to check the first level of the timing graph, since all SOURCEs should appear there*/
    num_at_level = tnodes_at_level[0].nelem; /* There are num_at_level top-level tnodes. */
    for(i = 0; i < num_at_level; i++) {
        inode = tnodes_at_level[0].list[i];	/* Iterate through each tnode. inode is the index of the tnode in the array tnode. */
		if (tnode[inode].type == TN_INPAD_SOURCE || tnode[inode].type == TN_CLOCK_SOURCE) {	/* See if this node is the start of an I/O pad, or a clock source. */			
			net_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
			if ((clock_index = find_clock(net_name)) != -1) { /* We have a clock inpad or clock source. */
				/* Set clock skew to 0 at the source and propagate skew 
				recursively to all connected nodes, adding delay as we go. 
				Set the clock domain to the index of the clock in the
				g_sdc->constrained_clocks array and propagate unchanged. */
				tnode[inode].clock_delay = 0.;
				tnode[inode].clock_domain = clock_index;
				propagate_clock_domain_and_skew(inode);

				/* Set the clock domain of this clock inpad to -1, so that we do not analyse it.  
				If we did not do this, the clock net would be analysed on the same iteration of 
				the timing analyzer as the flip-flops it drives! */
				tnode[inode].clock_domain = DO_NOT_ANALYSE;

			} else if (tnode[inode].type == TN_INPAD_SOURCE && (input_index = find_input(net_name)) != -1) {
				/* We have a constrained non-clock inpad - find the clock it's constrained on. */
				clock_index = find_clock(g_sdc->constrained_inputs[input_index].clock_name);
				VTR_ASSERT(clock_index != -1);
				/* The clock domain for this input is that of its virtual clock */
				tnode[inode].clock_domain = clock_index;
				/* Increment the fanout of this virtual clock domain. */
				g_sdc->constrained_clocks[clock_index].fanout++;
				/* Mark input delay specified in SDC file on the timing graph edge leading out from the TN_INPAD_SOURCE node. */
				tnode[inode].out_edges[0].Tdel = g_sdc->constrained_inputs[input_index].delay * 1e-9; /* normalize to be in seconds not ns */
			} else { /* We have an unconstrained input - mark with dummy clock domain and do not analyze. */
				tnode[inode].clock_domain = DO_NOT_ANALYSE;
			}
		}
	}	

    /* Second, visit all TN_OUTPAD_SINK tnodes. Unlike for TN_INPAD_SOURCE tnodes,
	we have to search the entire tnode array since these are all on different levels. */
	for (inode = 0; inode < num_tnodes; inode++) {
		if (tnode[inode].type == TN_OUTPAD_SINK) {
			/*  Since the pb_graph_pin of TN_OUTPAD_SINK tnodes points to NULL, we have to find the net 
			from the pb_graph_pin of the corresponding TN_OUTPAD_IPIN node. 
			Exploit the fact that the TN_OUTPAD_IPIN node will always be one prior in the tnode array. */
			VTR_ASSERT(tnode[inode - 1].type == TN_OUTPAD_IPIN);
			net_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
			output_index = find_output(net_name + 4); /* the + 4 removes the prefix "out:" automatically prepended to outputs */
			if (output_index != -1) {
				/* We have a constrained outpad, find the clock it's constrained on. */
				clock_index = find_clock(g_sdc->constrained_outputs[output_index].clock_name);
				VTR_ASSERT(clock_index != -1);
				/* The clock doain for this output is that of its virtual clock */
				tnode[inode].clock_domain = clock_index;
				/* Increment the fanout of this virtual clock domain. */
				g_sdc->constrained_clocks[clock_index].fanout++;
				/* Mark output delay specified in SDC file on the timing graph edge leading into the TN_OUTPAD_SINK node. 
				However, this edge is part of the corresponding TN_OUTPAD_IPIN node. 
				Exploit the fact that the TN_OUTPAD_IPIN node will always be one prior in the tnode array. */
				tnode[inode - 1].out_edges[0].Tdel = g_sdc->constrained_outputs[output_index].delay * 1e-9; /* normalize to be in seconds not ns */

			} else { /* We have an unconstrained input - mark with dummy clock domain and do not analyze. */
				tnode[inode].clock_domain = -1;
			}
		}
	}

	/* Third, visit all TN_FF_SOURCE and TN_FF_SINK tnodes, and transfer the clock domain and skew from their corresponding TN_FF_CLOCK tnodes*/
	for (inode = 0; inode < num_tnodes; inode++) {
		if (tnode[inode].type == TN_FF_SOURCE || tnode[inode].type == TN_FF_SINK) {
			clock_node = find_ff_clock_tnode(inode, is_prepacked, lookup_tnode_from_pin_id);
			tnode[inode].clock_domain = clock_node->clock_domain;
			tnode[inode].clock_delay   = clock_node->clock_delay;
		}
	}
}

static void propagate_clock_domain_and_skew(int inode) {
/* Given a tnode indexed by inode (which is part of a clock net), 
 * propagate forward the clock domain (unchanged) and skew (adding the delay of edges) to all nodes in its fanout. 
 * We then call recursively on all children in a depth-first search.  If num_edges is 0, we should be at an TN_FF_CLOCK tnode;
 * We implicitly rely on a tnode not being part of two separate clock nets, since undefined behaviour would result if one DFS 
 * overwrote the results of another.  This may be problematic in cases of multiplexed or locally-generated clocks. */

	int iedge, to_node;
	t_tedge * tedge;

	tedge = tnode[inode].out_edges;	/* Get the list of edges from the node we're visiting. */

	if (!tedge) { /* Leaf/sink node; base case of the recursion. */
		VTR_ASSERT(tnode[inode].type == TN_FF_CLOCK);
		VTR_ASSERT(tnode[inode].clock_domain != -1); /* Make sure clock domain is valid. */
		g_sdc->constrained_clocks[tnode[inode].clock_domain].fanout++;
		return;
	}

	for (iedge = 0; iedge < tnode[inode].num_edges; iedge++) {	/* Go through each edge coming out from this tnode */
		to_node = tedge[iedge].to_node;
		if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

		/* Propagate clock skew forward along this clock net, adding the delay of the wires (edges) of the clock network. */ 
		tnode[to_node].clock_delay = tnode[inode].clock_delay + tedge[iedge].Tdel; 
		/* Propagate clock domain forward unchanged */
		tnode[to_node].clock_domain = tnode[inode].clock_domain;
		/* Finally, call recursively on the destination tnode. */
		propagate_clock_domain_and_skew(to_node);
	}
}

static char * find_tnode_net_name(int inode, bool is_prepacked, t_pb*** pin_id_to_pb_mapping) {
	/* Finds the name of the net which a tnode (inode) is on (different for pre-/post-packed netlists). */
	
	int logic_block; /* Name chosen not to conflict with the array logical_block */
	t_pb_graph_pin * pb_graph_pin;

	logic_block = tnode[inode].block;
	if (is_prepacked) {
        if(tnode[inode].type == TN_INPAD_SOURCE || tnode[inode].type == TN_INPAD_OPIN ||
           tnode[inode].type == TN_OUTPAD_SINK || tnode[inode].type == TN_OUTPAD_IPIN) {
            //For input/input pads the net name is the same as the block name
            return logical_block[logic_block].name;
        } else {
            //Otherwise we need to look it up via the extra prepacked data
            VTR_ASSERT(tnode[inode].prepacked_data);

            int iport = tnode[inode].prepacked_data->model_port;
            int ipin = tnode[inode].prepacked_data->model_pin;
            int inet;

            if(tnode[inode].prepacked_data->model_port_ptr->dir == IN_PORT) {
                inet = logical_block[logic_block].input_nets[iport][ipin];
            } else {
                VTR_ASSERT(tnode[inode].prepacked_data->model_port_ptr->dir == OUT_PORT);
                inet = logical_block[logic_block].output_nets[iport][ipin];
            }

            return g_atoms_nlist.net[inet].name;
        }
	} else {
        if(tnode[inode].type == TN_INPAD_SOURCE || tnode[inode].type == TN_INPAD_OPIN ||
           tnode[inode].type == TN_OUTPAD_SINK || tnode[inode].type == TN_OUTPAD_IPIN) {
            //For input/input pads the net name is the same as the block name
            pb_graph_pin = tnode[inode].pb_graph_pin;
			return pin_id_to_pb_mapping[logic_block][pb_graph_pin->pin_count_in_cluster]->name;
        } else {
            //We need to find the TN_CB_OPIN/TN_CB_IPIN that this node drives, so that we can look up
            //the net name in the global clb netlist
            int inode_search = inode;
            while(tnode[inode_search].type != TN_CB_IPIN && tnode[inode_search].type != TN_CB_OPIN ) { 
                //Assume we don't have any internal fanout inside the block
                //  Should be true for most source-types
                VTR_ASSERT(tnode[inode_search].num_edges == 1);
                inode_search = tnode[inode_search].out_edges[0].to_node;
            }
            VTR_ASSERT(tnode[inode_search].type == TN_CB_IPIN || tnode[inode_search].type == TN_CB_OPIN);

            //Now that we have a complex block pin, we can look-up its connected net in the CLB netlist
            pb_graph_pin = tnode[inode_search].pb_graph_pin;
            int ipin = pb_graph_pin->pin_count_in_cluster; //Pin into the CLB
            int inet = block[tnode[inode_search].block].nets[ipin]; //Net index into the clb netlist
            VTR_ASSERT(inet != OPEN);

            return g_clbs_nlist.net[inet].name;
        }
	}
}

static t_tnode * find_ff_clock_tnode(int inode, bool is_prepacked, int **lookup_tnode_from_pin_id) {
	/* Finds the TN_FF_CLOCK tnode on the same flipflop as an TN_FF_SOURCE or TN_FF_SINK tnode. */
	
	int logic_block; /* Name chosen not to conflict with the array logical_block */
	t_tnode * ff_clock_tnode;
	int ff_tnode;
	t_pb_graph_node * parent_pb_graph_node;
	t_pb_graph_pin * ff_source_or_sink_pb_graph_pin, * clock_pb_graph_pin;

	logic_block = tnode[inode].block;
	if (is_prepacked) {
		ff_clock_tnode = logical_block[logic_block].clock_net_tnode;
	} else {
		ff_source_or_sink_pb_graph_pin = tnode[inode].pb_graph_pin;
		parent_pb_graph_node = ff_source_or_sink_pb_graph_pin->parent_node;
		/* Make sure there's only one clock port and only one clock pin in that port */
		VTR_ASSERT(parent_pb_graph_node->num_clock_ports == 1);
		VTR_ASSERT(parent_pb_graph_node->num_clock_pins[0] == 1);
		clock_pb_graph_pin = &parent_pb_graph_node->clock_pins[0][0];
		ff_tnode = lookup_tnode_from_pin_id[logic_block][clock_pb_graph_pin->pin_count_in_cluster];
		VTR_ASSERT(ff_tnode != OPEN);
		ff_clock_tnode = &tnode[ff_tnode];
	}
	VTR_ASSERT(ff_clock_tnode != NULL);
	VTR_ASSERT(ff_clock_tnode->type == TN_FF_CLOCK);
	return ff_clock_tnode;
}

static int find_clock(char * net_name) {
/* Given a string net_name, find whether it's the name of a clock in the array g_sdc->constrained_clocks.  *
 * if it is, return the clock's index in g_sdc->constrained_clocks; if it's not, return -1. */
	int index;
	for (index = 0; index < g_sdc->num_constrained_clocks; index++) {
		if (strcmp(net_name, g_sdc->constrained_clocks[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_input(char * net_name) {
/* Given a string net_name, find whether it's the name of a constrained input in the array g_sdc->constrained_inputs.  *
 * if it is, return its index in g_sdc->constrained_inputs; if it's not, return -1. */
	int index;
	for (index = 0; index < g_sdc->num_constrained_inputs; index++) {
		if (strcmp(net_name, g_sdc->constrained_inputs[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_output(char * net_name) {
/* Given a string net_name, find whether it's the name of a constrained output in the array g_sdc->constrained_outputs.  *
 * if it is, return its index in g_sdc->constrained_outputs; if it's not, return -1. */
	int index;
	for (index = 0; index < g_sdc->num_constrained_outputs; index++) {
		if (strcmp(net_name, g_sdc->constrained_outputs[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_cf_constraint(char * source_clock_name, char * sink_ff_name) {
	/* Given a source clock domain and a sink flip-flop, find out if there's an override constraint between them.
	If there is, return the index in g_sdc->cf_constraints; if there is not, return -1. */
	int icf, isource, isink;

	for (icf = 0; icf < g_sdc->num_cf_constraints; icf++) {
		for (isource = 0; isource < g_sdc->cf_constraints[icf].num_source; isource++) {
			if (strcmp(g_sdc->cf_constraints[icf].source_list[isource], source_clock_name) == 0) {
				for (isink = 0; isink < g_sdc->cf_constraints[icf].num_sink; isink++) {
					if (strcmp(g_sdc->cf_constraints[icf].sink_list[isink], sink_ff_name) == 0) {
						return icf;
					}
				}
			}
		}
	}
	return -1;
}

static inline int get_tnode_index(t_tnode * node) {
	/* Returns the index of pointer_to_tnode in the array tnode [0..num_tnodes - 1]
	using pointer arithmetic. */
	return node - tnode;
}

static inline bool has_valid_T_arr(int inode) {
	/* Has this tnode's arrival time been changed from its original value of HUGE_NEGATIVE_FLOAT? */
	return (bool) (tnode[inode].T_arr > HUGE_NEGATIVE_FLOAT + 1);
}

static inline bool has_valid_T_req(int inode) {
	/* Has this tnode's required time been changed from its original value of HUGE_POSITIVE_FLOAT? */
	return (bool) (tnode[inode].T_req < HUGE_POSITIVE_FLOAT - 1);
}

#ifndef PATH_COUNTING
bool has_valid_normalized_T_arr(int inode) {
	/* Has this tnode's normalized_T_arr been changed from its original value of HUGE_NEGATIVE_FLOAT? */
	return (bool) (tnode[inode].prepacked_data->normalized_T_arr > HUGE_NEGATIVE_FLOAT + 1);
}
#endif

float get_critical_path_delay(void) {
	/* Finds the critical path delay, which is the minimum clock period required to meet the constraint
	corresponding to the pair of source and sink clock domains with the least slack in the design. */
	
	int source_clock_domain, sink_clock_domain;
	float least_slack_in_design = HUGE_POSITIVE_FLOAT, critical_path_delay = UNDEFINED;

	if (!g_sdc) return UNDEFINED; /* If timing analysis is off, for instance. */

	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			if (least_slack_in_design > f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]) {
				least_slack_in_design = f_timing_stats->least_slack[source_clock_domain][sink_clock_domain];
				critical_path_delay = f_timing_stats->cpd[source_clock_domain][sink_clock_domain];
			}
		}
	}

	return critical_path_delay * 1e9; /* Convert to nanoseconds */
}

void print_timing_stats(void) {

	/* Prints critical path delay/fmax, least slack in design, and, for multiple-clock designs, 
	minimum required clock period to meet each constraint, least slack per constraint,
	geometric average clock frequency, and fanout-weighted geometric average clock frequency. */

	int source_clock_domain, sink_clock_domain, clock_domain, fanout, total_fanout = 0, 
		num_netlist_clocks_with_intra_domain_paths = 0;
	float geomean_period = 1., least_slack_in_design = HUGE_POSITIVE_FLOAT, critical_path_delay = UNDEFINED;
	double fanout_weighted_geomean_period = 0.;
	bool found;

		// REMOVE AFTER
		critical_path_delay = get_critical_path_delay();
		vtr::printf_info("Critical path in print timing: %g ns\n", critical_path_delay);
		critical_path_delay = UNDEFINED;

	/* Find critical path delay. If the pb_max_internal_delay is greater than this, it becomes
	 the limiting factor on critical path delay, so print that instead, with a special message. */
	
	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			if (least_slack_in_design > f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]) {
				least_slack_in_design = f_timing_stats->least_slack[source_clock_domain][sink_clock_domain];
				critical_path_delay = f_timing_stats->cpd[source_clock_domain][sink_clock_domain];
			}
		}
	}

	if (pb_max_internal_delay != UNDEFINED && pb_max_internal_delay > critical_path_delay) {
		critical_path_delay = pb_max_internal_delay;
		vtr::printf_info("Final critical path: %g ns", 1e9 * critical_path_delay);
		vtr::printf_direct(" (capped by fmax of block type %s)", pbtype_max_internal_delay->name);
		
	} else {
		vtr::printf_info("Final critical path: %g ns", 1e9 * critical_path_delay);
	}

	if (g_sdc->num_constrained_clocks <= 1) {
		/* Although critical path delay is always well-defined, it doesn't make sense to talk about fmax for multi-clock circuits */
		vtr::printf_direct(", f_max: %g MHz", 1e-6 / critical_path_delay);
	}
	vtr::printf_direct("\n");
	
	/* Also print the least slack in the design */
	vtr::printf_info("\n");
	vtr::printf_info("Least slack in design: %g ns\n", 1e9 * least_slack_in_design);
	vtr::printf_info("\n");

	if (g_sdc->num_constrained_clocks > 1) { /* Multiple-clock design */

		/* Print minimum possible clock period to meet each constraint. Convert to nanoseconds. */

		vtr::printf_info("Minimum possible clock period to meet each constraint (including skew effects):\n");
		for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
			/* Print the intra-domain constraint if it was analysed. */
			if (g_sdc->domain_constraint[source_clock_domain][source_clock_domain] > NEGATIVE_EPSILON) { 
				vtr::printf_info("%s to %s: %g ns (%g MHz)\n", 
						g_sdc->constrained_clocks[source_clock_domain].name,
						g_sdc->constrained_clocks[source_clock_domain].name, 
						1e9 * f_timing_stats->cpd[source_clock_domain][source_clock_domain],
						1e-6 / f_timing_stats->cpd[source_clock_domain][source_clock_domain]);
			} else {
				vtr::printf_info("%s to %s: --\n", 
						g_sdc->constrained_clocks[source_clock_domain].name,
						g_sdc->constrained_clocks[source_clock_domain].name);
			}
			/* Then, print all other constraints on separate lines, indented. We re-print 
			the source clock domain's name so there's no ambiguity when parsing. */
			for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (g_sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) { 
					/* If this domain pair was analysed */
					vtr::printf_info("\t%s to %s: %g ns (%g MHz)\n", 
							g_sdc->constrained_clocks[source_clock_domain].name,
							g_sdc->constrained_clocks[sink_clock_domain].name, 
							1e9 * f_timing_stats->cpd[source_clock_domain][sink_clock_domain],
							1e-6 / f_timing_stats->cpd[source_clock_domain][sink_clock_domain]);
				} else {
					vtr::printf_info("\t%s to %s: --\n", 
							g_sdc->constrained_clocks[source_clock_domain].name,
							g_sdc->constrained_clocks[sink_clock_domain].name);
				}
			}
		}

		/* Print least slack per constraint. */

		vtr::printf_info("\n");
		vtr::printf_info("Least slack per constraint:\n");
		for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
			/* Print the intra-domain slack if valid. */
			if (f_timing_stats->least_slack[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
				vtr::printf_info("%s to %s: %g ns\n", 
						g_sdc->constrained_clocks[source_clock_domain].name, 
						g_sdc->constrained_clocks[source_clock_domain].name, 
						1e9 * f_timing_stats->least_slack[source_clock_domain][source_clock_domain]);
			} else {
				vtr::printf_info("%s to %s: --\n", 
						g_sdc->constrained_clocks[source_clock_domain].name,
						g_sdc->constrained_clocks[source_clock_domain].name);
			}
			/* Then, print all other slacks on separate lines. */
			for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (f_timing_stats->least_slack[source_clock_domain][sink_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
					/* If this domain pair was analysed and has a valid slack */
					vtr::printf_info("\t%s to %s: %g ns\n", 
							g_sdc->constrained_clocks[source_clock_domain].name,
							g_sdc->constrained_clocks[sink_clock_domain].name, 
							1e9 * f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]);
				} else {
					vtr::printf_info("\t%s to %s: --\n", 
							g_sdc->constrained_clocks[source_clock_domain].name,
							g_sdc->constrained_clocks[sink_clock_domain].name);
				}
			}
		}
	
		/* Calculate geometric mean f_max (fanout-weighted and unweighted) from the diagonal (intra-domain) entries of critical_path_delay, 
		excluding domains without intra-domain paths (for which the timing constraint is DO_NOT_ANALYSE) and virtual clocks. */
		found = false;
		for (clock_domain = 0; clock_domain < g_sdc->num_constrained_clocks; clock_domain++) {
			if (g_sdc->domain_constraint[clock_domain][clock_domain] > NEGATIVE_EPSILON && g_sdc->constrained_clocks[clock_domain].is_netlist_clock) {
				geomean_period *= f_timing_stats->cpd[clock_domain][clock_domain];
				fanout = g_sdc->constrained_clocks[clock_domain].fanout;
				fanout_weighted_geomean_period += log((double) f_timing_stats->cpd[clock_domain][clock_domain])*(double)fanout;
				total_fanout += fanout;
				num_netlist_clocks_with_intra_domain_paths++;
				found = true;
			}
		}
		if (found) { /* Only print these if we found at least one clock domain with intra-domain paths. */
			geomean_period = pow(geomean_period, (float) 1/num_netlist_clocks_with_intra_domain_paths);
			fanout_weighted_geomean_period = exp(fanout_weighted_geomean_period/total_fanout);
			/* Convert to MHz */
			vtr::printf_info("\n");
			vtr::printf_info("Geometric mean intra-domain period: %g ns (%g MHz)\n", 
					1e9 * geomean_period, 1e-6 / geomean_period);
			vtr::printf_info("Fanout-weighted geomean intra-domain period: %g ns (%g MHz)\n", 
					1e9 * fanout_weighted_geomean_period, 1e-6 / fanout_weighted_geomean_period);
		}

		vtr::printf_info("\n");
	}
}

static void print_timing_constraint_info(const char *fname) {
	/* Prints the contents of g_sdc->domain_constraint, g_sdc->constrained_clocks, constrained_ios and g_sdc->cc_constraints to a file. */
	
	FILE * fp;
	int source_clock_domain, sink_clock_domain, i, j;
	int * clock_name_length = (int *) vtr::malloc(g_sdc->num_constrained_clocks * sizeof(int)); /* Array of clock name lengths */
	int max_clock_name_length = INT_MIN;
	char * clock_name;

	fp = vtr::fopen(fname, "w");

	/* Get lengths of each clock name and max length so we can space the columns properly. */
	for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
		clock_name = g_sdc->constrained_clocks[sink_clock_domain].name;
		clock_name_length[sink_clock_domain] = strlen(clock_name);
		if (clock_name_length[sink_clock_domain] > max_clock_name_length) {
			max_clock_name_length = clock_name_length[sink_clock_domain];	
		}
	}

	/* First, combine info from g_sdc->domain_constraint and g_sdc->constrained_clocks into a matrix 
	(they're indexed the same as each other). */

	fprintf(fp, "Timing constraints in ns (source clock domains down left side, sink along top).\n"
				"A value of -1.00 means the pair of source and sink domains will not be analysed.\n\n");

	print_spaces(fp, max_clock_name_length + 4); 

	for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
		fprintf(fp, "%s", g_sdc->constrained_clocks[sink_clock_domain].name);
		/* Minimum column width of 8 */
		print_spaces(fp, max(8 - clock_name_length[sink_clock_domain], 4));
	}
	fprintf(fp, "\n");

	for (source_clock_domain = 0; source_clock_domain < g_sdc->num_constrained_clocks; source_clock_domain++) {
		fprintf(fp, "%s", g_sdc->constrained_clocks[source_clock_domain].name);
		print_spaces(fp, max_clock_name_length + 4 - clock_name_length[source_clock_domain]);
		for (sink_clock_domain = 0; sink_clock_domain < g_sdc->num_constrained_clocks; sink_clock_domain++) {
			fprintf(fp, "%5.2f", g_sdc->domain_constraint[source_clock_domain][sink_clock_domain]);
			/* Minimum column width of 8 */
			print_spaces(fp, max(clock_name_length[sink_clock_domain] - 1, 3));
		}
		fprintf(fp, "\n");
	}

	free(clock_name_length);

	/* Second, print I/O constraints. */
	fprintf(fp, "\nList of constrained inputs (note: constraining a clock net input has no effect):\n");
	for (i = 0; i < g_sdc->num_constrained_inputs; i++) {
		fprintf(fp, "Input name %s on clock %s with input delay %.2f ns\n", 
			g_sdc->constrained_inputs[i].name, g_sdc->constrained_inputs[i].clock_name, g_sdc->constrained_inputs[i].delay);
	}

	fprintf(fp, "\nList of constrained outputs:\n");
	for (i = 0; i < g_sdc->num_constrained_outputs; i++) {
		fprintf(fp, "Output name %s on clock %s with output delay %.2f ns\n", 
			g_sdc->constrained_outputs[i].name, g_sdc->constrained_outputs[i].clock_name, g_sdc->constrained_outputs[i].delay);
	}

	/* Third, print override constraints. */
	fprintf(fp, "\nList of override constraints (non-default constraints created by set_false_path, set_clock_groups, \nset_max_delay, and set_multicycle_path):\n");
	
	for (i = 0; i < g_sdc->num_cc_constraints; i++) {
		fprintf(fp, "Clock domain");
		for (j = 0; j < g_sdc->cc_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", g_sdc->cc_constraints[i].source_list[j]); 
		}
		fprintf(fp, " to clock domain");
		for (j = 0; j < g_sdc->cc_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", g_sdc->cc_constraints[i].sink_list[j]); 
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (g_sdc->cc_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", g_sdc->cc_constraints[i].sink_list[j], g_sdc->cc_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", g_sdc->cc_constraints[i].sink_list[j], g_sdc->cc_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < g_sdc->num_cf_constraints; i++) {
		fprintf(fp, "Clock domain");
		for (j = 0; j < g_sdc->cf_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", g_sdc->cf_constraints[i].source_list[j]); 
		}
		fprintf(fp, " to flip-flop");
		for (j = 0; j < g_sdc->cf_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", g_sdc->cf_constraints[i].sink_list[j]); 
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (g_sdc->cf_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", g_sdc->cf_constraints[i].sink_list[j], g_sdc->cf_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", g_sdc->cf_constraints[i].sink_list[j], g_sdc->cf_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < g_sdc->num_fc_constraints; i++) {
		fprintf(fp, "Flip-flop");
		for (j = 0; j < g_sdc->fc_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", g_sdc->fc_constraints[i].source_list[j]); 
		}
		fprintf(fp, " to clock domain");
		for (j = 0; j < g_sdc->fc_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", g_sdc->fc_constraints[i].sink_list[j]); 
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (g_sdc->fc_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", g_sdc->fc_constraints[i].sink_list[j], g_sdc->fc_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", g_sdc->fc_constraints[i].sink_list[j], g_sdc->fc_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < g_sdc->num_ff_constraints; i++) {
		fprintf(fp, "Flip-flop");
		for (j = 0; j < g_sdc->ff_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", g_sdc->ff_constraints[i].source_list[j]); 
		}
		fprintf(fp, " to flip-flop");
		for (j = 0; j < g_sdc->ff_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", g_sdc->ff_constraints[i].sink_list[j]); 
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (g_sdc->ff_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", g_sdc->ff_constraints[i].sink_list[j], g_sdc->ff_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", g_sdc->ff_constraints[i].sink_list[j], g_sdc->ff_constraints[i].num_multicycles);
		}
	}

	fclose(fp);
}

static void print_spaces(FILE * fp, int num_spaces) {
	/* Prints num_spaces spaces to file pointed to by fp. */
	for ( ; num_spaces > 0; num_spaces--) {
		fprintf(fp, " ");
	}
}

void print_timing_graph_as_blif (const char *fname, t_model *models) {
	struct s_model_ports *port;
	vtr::t_linked_vptr *p_io_removed;
	/* Prints out the critical path to a file.  */

	FILE *fp;
	int i, j;
	
	//int **lookup_tnode_from_pin_id = NULL;

	fp = vtr::fopen(fname, "w");

	fprintf(fp, ".model %s\n", blif_circuit_name);

	fprintf(fp, ".inputs ");
	for (i = 0; i < num_logical_blocks; i++) {
		if (logical_block[i].type == VPACK_INPAD) {
			fprintf(fp, "\\\n%s ", logical_block[i].name);
		}
	}
	p_io_removed = circuit_p_io_removed;
	while (p_io_removed) {
		fprintf(fp, "\\\n%s ", (char *) p_io_removed->data_vptr);
		p_io_removed = p_io_removed->next;
	}

	fprintf(fp, "\n");

	fprintf(fp, ".outputs ");
	for (i = 0; i < num_logical_blocks; i++) {
		if (logical_block[i].type == VPACK_OUTPAD) {
			/* Outputs have a "out:" prepended to them, must remove */
			fprintf(fp, "\\\n%s ", &logical_block[i].name[4]);
		}
	}
	fprintf(fp, "\n");
	fprintf(fp, ".names unconn\n");
	fprintf(fp, " 0\n\n");
	
	
	/* Print out primitives */
	//FIXME: Causing a segfault	
	//for (i = 0; i < num_logical_blocks; i++) {
	//	print_primitive_as_blif(fp, i, lookup_tnode_from_pin_id);
	//}
	
	/* Print out tnode connections */
	for (i = 0; i < num_tnodes; i++) {
		if (tnode[i].type != TN_PRIMITIVE_IPIN && tnode[i].type != TN_FF_SOURCE
				&& tnode[i].type != TN_INPAD_SOURCE
				&& tnode[i].type != TN_OUTPAD_IPIN) {
			for (j = 0; j < tnode[i].num_edges; j++) {
				fprintf(fp, ".names tnode_%d tnode_%d\n", i,
					tnode[i].out_edges[j].to_node);
				fprintf(fp, "1 1\n\n");
			}
		}
	}

	fprintf(fp, ".end\n\n");

	/* Print out .subckt models */
	while (models) {
		fprintf(fp, ".model %s\n", models->name);
		fprintf(fp, ".inputs ");
		port = models->inputs;
		while (port) {
			if (port->size > 1) {
				for (j = 0; j < port->size; j++) {
					fprintf(fp, "%s[%d] ", port->name, j);
				}
			} else {
				fprintf(fp, "%s ", port->name);
			}
			port = port->next;
		}
		fprintf(fp, "\n");
		fprintf(fp, ".outputs ");
		port = models->outputs;
		while (port) {
			if (port->size > 1) {
				for (j = 0; j < port->size; j++) {
					fprintf(fp, "%s[%d] ", port->name, j);
				}
			} else {
				fprintf(fp, "%s ", port->name);
			}
			port = port->next;
		}
		fprintf(fp, "\n.blackbox\n.end\n\n");
		fprintf(fp, "\n\n");
		models = models->next;
	}
	fclose(fp);
}

/*
 Create a lookup table that returns the tnode index for [iblock][pb_graph_pin_id]
*/
int **alloc_and_load_tnode_lookup_from_pin_id() {
	int **tnode_lookup;

	tnode_lookup = new int* [num_blocks];

	for (int i = 0; i < num_blocks; i++) {
		int num_pins = block[i].type->pb_graph_head->total_pb_pins;
		tnode_lookup[i] = new int[num_pins];
		for (int j = 0; j < num_pins; j++) {
			tnode_lookup[i][j] = OPEN;
		}
	}

	for (int i = 0; i < num_tnodes; i++) {
		if (tnode[i].pb_graph_pin != NULL) {
			if (tnode[i].type != TN_CLOCK_SOURCE &&
				tnode[i].type != TN_FF_SOURCE &&
				tnode[i].type != TN_INPAD_SOURCE &&
				tnode[i].type != TN_FF_SINK &&
				tnode[i].type != TN_OUTPAD_SINK
				) {
				int pb_pin_id = tnode[i].pb_graph_pin->pin_count_in_cluster;
				int iblk = tnode[i].block;
				VTR_ASSERT(tnode_lookup[iblk][pb_pin_id] == OPEN);				
				tnode_lookup[iblk][pb_pin_id] = i;
			}
		}
	}
	return tnode_lookup;
}

void free_tnode_lookup_from_pin_id(int **tnode_lookup) {
	for (int i = 0; i < num_blocks; i++) {
		delete[] tnode_lookup[i];
	}
	delete[] tnode_lookup;
}
