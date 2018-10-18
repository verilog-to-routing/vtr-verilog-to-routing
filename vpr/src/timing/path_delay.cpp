#include <cstring>
#include <climits>
#include <cmath>
#include <set>
#include <time.h>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "net_delay.h"
#include "vpr_utils.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "read_sdc.h"
#include "stats.h"

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
timing_ctx.tnodes ("timing node").  This is done through alloc_and_load_tnodes (post-
packed) or alloc_and_load_tnodes_from_prepacked_netlist. Then, the timing
graph is topologically sorted ("levelized") in
alloc_and_load_timing_graph_levels, to allow for faster traversals later.

read_sdc reads the SDC file, interprets its contents and stores them in
the data structure timing_ctx.sdc.  (This data structure does not need to remain
global but it is probably easier, since its information is used in both
netlists and only needs to be read in once.)

load_clock_domain_and_clock_and_io_delay then gives each flip-flop and I/O
the index of a constrained clock from the SDC file in timing_ctx.sdc->constrained_
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

We can build timing graphs that match either the primitive (atom_ctx.nlist)
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

/******************** Variables local to this module ************************/

#define NUM_BUCKETS 5 /* Used when printing slack and criticality. */

/* Variables for "chunking" the tedge memory.  If the head pointer in tedge_ch is NULL, *
 * no timing graph exists now.															*/

static vtr::t_chunk tedge_ch;

static vector<size_t> f_num_timing_net_pins;

static t_timing_stats * f_timing_stats = nullptr; /* Critical path delay and worst-case slack per constraint. */

static int * f_net_to_driver_tnode;
/* [0..net.size() - 1]. Gives the index of the tnode that drives each net.
Used for both pre- and post-packed netlists. If you just want the number
of edges on the driver tnode, use:
	num_edges = timing_nets[inet].num_sinks;
instead of the equivalent but more convoluted:
	driver_tnode = f_net_to_driver_tnode[inet];
	num_edges = timing_ctx.tnodes[driver_tnode].num_edges;
Only use this array if you want the actual edges themselves or the index
of the driver tnode. */

/***************** Subroutines local to this module *************************/

static t_slack * alloc_slacks();

static void update_slacks(t_slack * slacks, float criticality_denom,
    bool update_slack, bool is_final_analysis, float smallest_slack_in_design, const t_timing_inf &timing_inf);

static void alloc_and_load_tnodes(const t_timing_inf &timing_inf);

static void alloc_and_load_tnodes_from_prepacked_netlist(float inter_cluster_net_delay,
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode);

static void mark_max_block_delay(const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode);

static void alloc_timing_stats();

static float do_timing_analysis_for_constraint(int source_clock_domain, int sink_clock_domain,
	bool is_prepacked, bool is_final_analysis, long * max_critical_input_paths_ptr,
    long * max_critical_output_paths_ptr, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping, const t_timing_inf &timing_inf);

#ifdef PATH_COUNTING
static void do_path_counting(float criticality_denom);
#endif

static float find_least_slack(bool is_prepacked, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping);

static void load_tnode(t_pb_graph_pin *pb_graph_pin, const ClusterBlockId iblock,
		int *inode);

#ifndef PATH_COUNTING
static void update_normalized_costs(float T_arr_max_this_domain, long max_critical_input_paths,
    long max_critical_output_paths, const t_timing_inf &timing_inf);
#endif

//static void print_primitive_as_blif(FILE *fpout, int iblk, int **lookup_tnode_from_pin_id);

static void load_clock_domain_and_clock_and_io_delay(bool is_prepacked, vtr::vector_map<ClusterBlockId, std::vector<int>> &lookup_tnode_from_pin_id, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping);

static const char * find_tnode_net_name(int inode, bool is_prepacked, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping);

static t_tnode * find_ff_clock_tnode(int inode, bool is_prepacked, vtr::vector_map<ClusterBlockId, std::vector<int>> &lookup_tnode_from_pin_id);

static inline bool has_valid_T_arr(int inode);

static inline bool has_valid_T_req(int inode);

static int find_clock(const char * net_name);

static int find_input(const char * net_name);

static int find_output(const char * net_name);

static int find_cf_constraint(const char * source_clock_name, const char * sink_ff_name);

static void propagate_clock_domain_and_skew(int inode);

static void process_constraints();

static void print_global_criticality_stats(FILE * fp, float ** criticality, const char * singular_name, const char * capitalized_plural_name);

static void print_timing_constraint_info(const char *fname);

static void print_spaces(FILE * fp, int num_spaces);

//The following functions handle counting of net pins for both
//the atom and clb netlist.
//TODO: remove - these are temporarily in use until re-unify the timing graph to be directly driven by the atom netlist only
static std::vector<size_t> init_timing_net_pins();
static std::vector<size_t> init_atom_timing_net_pins();
size_t num_timing_nets();
size_t num_timing_net_pins(int inet);
size_t num_timing_net_sinks(int inet);

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

	t_slack * slacks = nullptr;
	bool do_process_constraints = false;
	vtr::vector_map<ClusterBlockId, std::vector<int>> lookup_tnode_from_pin_id;
	vtr::vector_map<ClusterBlockId, t_pb **> pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();

	if (tedge_ch.chunk_ptr_head != nullptr) {
		vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__,
				"in alloc_and_load_timing_graph: An old timing graph still exists.\n");
	}

    f_num_timing_net_pins = init_timing_net_pins();

	alloc_and_load_tnodes(timing_inf);

    detect_and_fix_timing_graph_combinational_loops();

	alloc_and_load_timing_graph_levels();

	check_timing_graph();

	slacks = alloc_slacks();

    auto& timing_ctx = g_vpr_ctx.timing();

	if (timing_ctx.sdc == nullptr) {
		/* the SDC timing constraints only need to be read in once; *
		 * if they haven't been already, do it now				    */
		read_sdc(timing_inf);
		do_process_constraints = true;
	}

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();

	load_clock_domain_and_clock_and_io_delay(false, lookup_tnode_from_pin_id, pin_id_to_pb_mapping);

	if (do_process_constraints)
		process_constraints();

	if (f_timing_stats == nullptr)
		alloc_timing_stats();

	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);
	free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);

	return slacks;
}

t_slack * alloc_and_load_pre_packing_timing_graph(float inter_cluster_net_delay, t_timing_inf timing_inf,
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode) {

	/* This routine builds the graph used for timing analysis.  Every technology-
	 * mapped netlist pin is a timing node (tnode).  The connectivity between pins is *
	 * represented by timing edges (tedges).  All delay is marked on edges, not *
	 * on nodes.  Returns two arrays that will store slack values:				 *
	 * slack and criticality ([0..net.size()-1][1..num_pins]).           */

	/*  For pads, only the first two pin locations are used (input to pad is first,
	 * output of pad is second).  For CLBs, all OPEN pins on the cb have their
	 * mapping set to OPEN so I won't use it by mistake.                          */

	t_slack * slacks = nullptr;
	bool do_process_constraints = false;
	vtr::vector_map<ClusterBlockId, std::vector<int>> lookup_tnode_from_pin_id;
	vtr::vector_map<ClusterBlockId, t_pb **> pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();

	if (tedge_ch.chunk_ptr_head != nullptr) {
		vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__,
				"in alloc_and_load_timing_graph: An old timing graph still exists.\n");
	}

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();

    f_num_timing_net_pins = init_atom_timing_net_pins();

	alloc_and_load_tnodes_from_prepacked_netlist(inter_cluster_net_delay, expected_lowest_cost_pb_gnode);

    mark_max_block_delay(expected_lowest_cost_pb_gnode);

    detect_and_fix_timing_graph_combinational_loops();

	alloc_and_load_timing_graph_levels();

	slacks = alloc_slacks();

	check_timing_graph();

    auto& timing_ctx = g_vpr_ctx.timing();

	if (timing_ctx.sdc == nullptr) {
		/* the SDC timing constraints only need to be read in once; *
		 * if they haven't been already, do it now				    */
		read_sdc(timing_inf);
		do_process_constraints = true;
	}

	load_clock_domain_and_clock_and_io_delay(true, lookup_tnode_from_pin_id, pin_id_to_pb_mapping);

	if (do_process_constraints)
		process_constraints();

	if (f_timing_stats == nullptr)
		alloc_timing_stats();

	free_pin_id_to_pb_mapping(pin_id_to_pb_mapping);
	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);

	return slacks;
}

static t_slack * alloc_slacks() {

	/* Allocates the slack, criticality and path_criticality structures
	([0..net.size()-1][1..num_pins-1]). Chunk allocated to save space. */

	t_slack * slacks = (t_slack *) vtr::malloc(sizeof(t_slack));

	slacks->slack   = (float **) vtr::malloc(num_timing_nets() * sizeof(float *));
	slacks->timing_criticality = (float **) vtr::malloc(num_timing_nets() * sizeof(float *));
#ifdef PATH_COUNTING
	slacks->path_criticality = (float **) vtr::malloc(num_timing_nets() * sizeof(float *));
#endif
	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		slacks->slack[inet]	  = (float *) vtr::chunk_malloc(num_timing_net_pins(inet) * sizeof(float), &tedge_ch);
		slacks->timing_criticality[inet] = (float *) vtr::chunk_malloc(num_timing_net_pins(inet) * sizeof(float), &tedge_ch);
#ifdef PATH_COUNTING
		slacks->path_criticality[inet] = (float *) vtr::chunk_malloc(num_timing_net_pins(inet) * sizeof(float), &tedge_ch);
#endif
	}

	return slacks;
}

void load_timing_graph_net_delays(vtr::vector_map<ClusterNetId, float *> &net_delay) {

	/* Sets the delays of the inter-CLB nets to the values specified by          *
	 * net_delay[0..net.size()-1][1..num_pins-1].  These net delays should have    *
	 * been allocated and loaded with the net_delay routines.  This routine      *
	 * marks the corresponding edges in the timing graph with the proper delay.  */

    clock_t begin = clock();

    VTR_ASSERT(net_delay.size());

	int inode;
	unsigned ipin;
	t_tedge *tedge;

    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		inode = f_net_to_driver_tnode[inet];
		tedge = timing_ctx.tnodes[inode].out_edges;

		/* Note that the edges of a tnode corresponding to a CLB or INPAD opin must  *
		 * be in the same order as the pins of the net driven by the tnode.          */

		for (ipin = 1; ipin < num_timing_net_pins(inet); ipin++)
			tedge[ipin - 1].Tdel = net_delay[(ClusterNetId)inet][ipin];
	}

    clock_t end = clock();

    timing_ctx.stats.old_delay_annotation_wallclock_time  += double(end - begin) / CLOCKS_PER_SEC;
}

void free_timing_graph(t_slack * slacks) {

	int inode;

    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	if (tedge_ch.chunk_ptr_head == nullptr) {
        VTR_LOG_WARN( "in free_timing_graph: No timing graph to free.\n");
	}

	free_chunk_memory(&tedge_ch);

	if (timing_ctx.tnodes != nullptr && timing_ctx.tnodes[0].prepacked_data) {
		/* If we allocated prepacked_data for the first node,
		it must be allocated for all other nodes too. */
		for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
			free(timing_ctx.tnodes[inode].prepacked_data);
		}
	}
	free(timing_ctx.tnodes);
	free(f_net_to_driver_tnode);
	vtr::free_ivec_vector(timing_ctx.tnodes_at_level, 0, timing_ctx.num_tnode_levels - 1);

	free(slacks->slack);
	free(slacks->timing_criticality);
#ifdef PATH_COUNTING
	free(slacks->path_criticality);
#endif
	free(slacks);

	timing_ctx.tnodes = nullptr;
	timing_ctx.num_tnodes = 0;
	f_net_to_driver_tnode = nullptr;
	//timing_ctx.tnodes_at_level = NULL;
	timing_ctx.num_tnode_levels = 0;
	slacks = nullptr;
}

void free_timing_stats() {
	int i;
    auto& timing_ctx = g_vpr_ctx.timing();

	if(f_timing_stats != nullptr) {
		for (i = 0; i < timing_ctx.sdc->num_constrained_clocks; i++) {
			free(f_timing_stats->cpd[i]);
			free(f_timing_stats->least_slack[i]);
		}
		free(f_timing_stats->cpd);
		free(f_timing_stats->least_slack);
		free(f_timing_stats);
	}
	f_timing_stats = nullptr;
}

void print_slack(float ** slack, bool slack_is_normalized, const char *fname) {

	/* Prints slacks into a file. */

	int ibucket, driver_tnode, num_unused_slacks = 0;
	t_tedge * tedge;
	FILE *fp;
	float max_slack = HUGE_NEGATIVE_FLOAT, min_slack = HUGE_POSITIVE_FLOAT,
		total_slack = 0, total_negative_slack = 0, bucket_size, slk;
	int slacks_in_bucket[NUM_BUCKETS];

    auto& timing_ctx = g_vpr_ctx.timing();

	fp = vtr::fopen(fname, "w");

	if (slack_is_normalized) {
		fprintf(fp, "The following slacks have been normalized to be non-negative by "
					"relaxing the required times to the maximum arrival time.\n\n");
	}

	/* Go through slack once to get the largest and smallest slack, both for reporting and
	   so that we can delimit the buckets. Also calculate the total negative slack in the design. */
	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		size_t num_edges = num_timing_net_sinks(inet);
		for (size_t iedge = 0; iedge < num_edges; iedge++) {
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

		for (size_t inet = 0; inet < num_timing_nets(); inet++) {
			size_t num_edges = num_timing_net_sinks(inet);
			for (size_t iedge = 0; iedge < num_edges; iedge++) {
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

	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		size_t num_edges = timing_ctx.tnodes[driver_tnode].num_edges;
		tedge = timing_ctx.tnodes[driver_tnode].out_edges;
		slk = slack[inet][1];
		if (slk < HUGE_POSITIVE_FLOAT - 1) {
			fprintf(fp, "%5zu\t%5d\t\t%5d\t%g\n", inet, driver_tnode, tedge[0].to_node, slk);
		} else { /* Slack is meaningless, so replace with --. */
			fprintf(fp, "%5zu\t%5d\t\t%5d\t--\n", inet, driver_tnode, tedge[0].to_node);
		}
		for (size_t iedge = 1; iedge < num_edges; iedge++) { /* newline and indent subsequent edges after the first */
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

	int iedge, driver_tnode, num_edges;
	t_tedge * tedge;
	FILE *fp;

    auto& timing_ctx = g_vpr_ctx.timing();

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

	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		num_edges = timing_ctx.tnodes[driver_tnode].num_edges;
		tedge = timing_ctx.tnodes[driver_tnode].out_edges;

		fprintf(fp, "\n%5zu\t%5d\t\t%5d\t\t%.6f", inet, driver_tnode, tedge[0].to_node, slacks->timing_criticality[inet][1]);
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

	int iedge, num_edges, ibucket, criticalities_in_bucket[NUM_BUCKETS];
	float crit, max_criticality = HUGE_NEGATIVE_FLOAT, min_criticality = HUGE_POSITIVE_FLOAT,
		total_criticality = 0, bucket_size;

	/* Go through criticality once to get the largest and smallest timing criticality,
	both for reporting and so that we can delimit the buckets. */
	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		num_edges = num_timing_net_sinks(inet);
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

		for (size_t inet = 0; inet < num_timing_nets(); inet++) {
			num_edges = num_timing_net_sinks(inet);
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

void print_net_delay(vtr::vector_map<ClusterNetId, float *> &net_delay, const char *fname) {

	/* Prints the net delays into a file. */

	int iedge, driver_tnode, num_edges;
	t_tedge * tedge;
	FILE *fp;

    auto& timing_ctx = g_vpr_ctx.timing();

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "Net #\tDriver_tnode\tto_node\tDelay\n\n");

	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		driver_tnode = f_net_to_driver_tnode[inet];
		num_edges = timing_ctx.tnodes[driver_tnode].num_edges;
		tedge = timing_ctx.tnodes[driver_tnode].out_edges;
		fprintf(fp, "%5zu\t%5d\t\t%5d\t%g\n", inet, driver_tnode, tedge[0].to_node, net_delay[(ClusterNetId)inet][1]);
		for (iedge = 1; iedge < num_edges; iedge++) { /* newline and indent subsequent edges after the first */
			fprintf(fp, "\t\t\t%5d\t%g\n", tedge[iedge].to_node, net_delay[(ClusterNetId)inet][iedge+1]);
		}
	}

	fclose(fp);
}

#ifndef PATH_COUNTING
void print_clustering_timing_info(const char *fname) {
	/* Print information from tnodes which is used by the clusterer. */
	int inode;
	FILE *fp;

    auto& timing_ctx = g_vpr_ctx.timing();

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "inode  ");
	if (timing_ctx.sdc->num_constrained_clocks <= 1) {
		/* These values are from the last constraint analysed,
		so they're not meaningful unless there was only one constraint. */
		fprintf(fp, "Critical input paths  Critical output paths  ");
	}
	fprintf(fp, "Normalized slack  Normalized Tarr  Normalized total crit paths\n");
	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		fprintf(fp, "%d\t", inode);
		/* Only print normalized values for tnodes which have valid normalized values. (If normalized_T_arr is valid, the others will be too.) */
		if (has_valid_normalized_T_arr(inode)) {
			if (timing_ctx.sdc->num_constrained_clocks <= 1) {
				fprintf(fp, "%ld\t\t\t%ld\t\t\t", timing_ctx.tnodes[inode].prepacked_data->num_critical_input_paths, timing_ctx.tnodes[inode].prepacked_data->num_critical_output_paths);
			}
			fprintf(fp, "%f\t%f\t%f\n", timing_ctx.tnodes[inode].prepacked_data->normalized_slack, timing_ctx.tnodes[inode].prepacked_data->normalized_T_arr, timing_ctx.tnodes[inode].prepacked_data->normalized_total_critical_paths);
		} else {
			if (timing_ctx.sdc->num_constrained_clocks <= 1) {
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
	int j, k;
	int inode, num_nodes_in_block;
	int count, i_pin_id;
	int dport, dpin, dnode;
	ClusterBlockId iblock, dblock;
	ClusterNetId net_id;
	int normalized_pin, normalization;
	t_pb_graph_pin *ipb_graph_pin;
	t_pb_route *intra_lb_route, *d_intra_lb_route;
	int num_dangling_pins;
	t_pb_graph_pin*** intra_lb_pb_pin_lookup;
	vtr::vector_map<ClusterBlockId, std::vector<int>> lookup_tnode_from_pin_id;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	f_net_to_driver_tnode = (int*)vtr::malloc(num_timing_nets() * sizeof(int));

	intra_lb_pb_pin_lookup = new t_pb_graph_pin**[device_ctx.num_block_types];
	for (int i = 0; i < device_ctx.num_block_types; i++) {
		intra_lb_pb_pin_lookup[i] = alloc_and_load_pb_graph_pin_lookup_from_index(&device_ctx.block_types[i]);
	}

	for (size_t i = 0; i < num_timing_nets(); i++) {
		f_net_to_driver_tnode[i] = OPEN;
	}

	/* allocate space for tnodes */
	timing_ctx.num_tnodes = 0;
	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		num_nodes_in_block = 0;
		int itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;
		for (j = 0; j < cluster_ctx.clb_nlist.block_pb(blk_id)->pb_graph_node->total_pb_pins; j++) {
			if (cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route[j].atom_net_id) {
				if (intra_lb_pb_pin_lookup[itype][j]->type == PB_PIN_INPAD
					|| intra_lb_pb_pin_lookup[itype][j]->type == PB_PIN_OUTPAD
					|| intra_lb_pb_pin_lookup[itype][j]->type == PB_PIN_SEQUENTIAL) {
					num_nodes_in_block += 2;
				} else {
					num_nodes_in_block++;
				}
			}
		}
		timing_ctx.num_tnodes += num_nodes_in_block;
	}
	timing_ctx.tnodes = (t_tnode*)vtr::calloc(timing_ctx.num_tnodes, sizeof(t_tnode));

	/* load tnodes with all info except edge info */
	/* populate tnode lookups for edge info */
	inode = 0;
	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		int itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;
		for (j = 0; j < cluster_ctx.clb_nlist.block_pb(blk_id)->pb_graph_node->total_pb_pins; j++) {
			if (cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route[j].atom_net_id) {
				VTR_ASSERT(timing_ctx.tnodes[inode].pb_graph_pin == nullptr);
				load_tnode(intra_lb_pb_pin_lookup[itype][j], blk_id, &inode);
			}
		}
	}
	VTR_ASSERT(inode == timing_ctx.num_tnodes);
	num_dangling_pins = 0;

	lookup_tnode_from_pin_id = alloc_and_load_tnode_lookup_from_pin_id();


	/* load edge delays and initialize clock domains to OPEN
	and prepacked_data (which is not used post-packing) to NULL. */
    std::set<int> const_gen_tnodes;
	for (int i = 0; i < timing_ctx.num_tnodes; i++) {
		timing_ctx.tnodes[i].clock_domain = OPEN;
		timing_ctx.tnodes[i].prepacked_data = nullptr;

		/* 3 primary scenarios for edge delays
		 1.  Point-to-point delays inside block
		 2.
		 */
		count = 0;
		iblock = timing_ctx.tnodes[i].block;
		int itype = cluster_ctx.clb_nlist.block_type(iblock)->index;
		switch (timing_ctx.tnodes[i].type) {
		case TN_INPAD_OPIN:
		case TN_INTERMEDIATE_NODE:
		case TN_PRIMITIVE_OPIN:
		case TN_FF_OPIN:
		case TN_CLOCK_OPIN:
		case TN_CB_IPIN:
			/* fanout is determined by intra-cluster connections */
			/* Allocate space for edges  */
			i_pin_id = timing_ctx.tnodes[i].pb_graph_pin->pin_count_in_cluster;
			intra_lb_route = cluster_ctx.clb_nlist.block_pb(iblock)->pb_route;
			ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];

			if (ipb_graph_pin->parent_node->pb_type->max_internal_delay
					!= UNDEFINED) {
				if (device_ctx.pb_max_internal_delay == UNDEFINED) {
					device_ctx.pb_max_internal_delay = ipb_graph_pin->parent_node->pb_type->max_internal_delay;
					device_ctx.pbtype_max_internal_delay = ipb_graph_pin->parent_node->pb_type;
				} else if (device_ctx.pb_max_internal_delay < ipb_graph_pin->parent_node->pb_type->max_internal_delay) {
					device_ctx.pb_max_internal_delay = ipb_graph_pin->parent_node->pb_type->max_internal_delay;
					device_ctx.pbtype_max_internal_delay = ipb_graph_pin->parent_node->pb_type;
				}
			}

			for (j = 0; j < ipb_graph_pin->num_output_edges; j++) {
				VTR_ASSERT(ipb_graph_pin->output_edges[j]->num_output_pins == 1);
				dnode = ipb_graph_pin->output_edges[j]->output_pins[0]->pin_count_in_cluster;
				if (intra_lb_route[dnode].driver_pb_pin_id == i_pin_id) {
					count++;
				}
			}
			VTR_ASSERT(count >= 0);
			timing_ctx.tnodes[i].num_edges = count;
			timing_ctx.tnodes[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					count * sizeof(t_tedge), &tedge_ch);

			/* Load edges */
			count = 0;
			for (j = 0; j < ipb_graph_pin->num_output_edges; j++) {
				VTR_ASSERT(ipb_graph_pin->output_edges[j]->num_output_pins == 1);
				dnode = ipb_graph_pin->output_edges[j]->output_pins[0]->pin_count_in_cluster;
				if (intra_lb_route[dnode].driver_pb_pin_id == i_pin_id) {
					VTR_ASSERT(intra_lb_route[dnode].atom_net_id == intra_lb_route[i_pin_id].atom_net_id);

					timing_ctx.tnodes[i].out_edges[count].Tdel = ipb_graph_pin->output_edges[j]->delay_max;
					timing_ctx.tnodes[i].out_edges[count].to_node = lookup_tnode_from_pin_id[iblock][dnode];
					VTR_ASSERT(timing_ctx.tnodes[i].out_edges[count].to_node != OPEN);

                    AtomNetId atom_net_id = intra_lb_route[i_pin_id].atom_net_id;
					if (atom_ctx.nlist.net_is_constant(atom_net_id) && timing_ctx.tnodes[i].type == TN_PRIMITIVE_OPIN) {
						timing_ctx.tnodes[i].out_edges[count].Tdel = HUGE_NEGATIVE_FLOAT;
						timing_ctx.tnodes[i].type = TN_CONSTANT_GEN_SOURCE;
                        const_gen_tnodes.insert(i);
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
				i_pin_id = timing_ctx.tnodes[i].pb_graph_pin->pin_count_in_cluster;
				intra_lb_route = cluster_ctx.clb_nlist.block_pb(iblock)->pb_route;
				ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];
				timing_ctx.tnodes[i].num_edges = ipb_graph_pin->num_pin_timing;
				timing_ctx.tnodes[i].out_edges = (t_tedge *) vtr::chunk_malloc(
						ipb_graph_pin->num_pin_timing * sizeof(t_tedge),
						&tedge_ch);
				k = 0;

				for (j = 0; j < timing_ctx.tnodes[i].num_edges; j++) {
					/* Some outpins aren't used, ignore these.  Only consider output pins that are used */
                    int cluster_pin_idx = ipb_graph_pin->pin_timing[j]->pin_count_in_cluster;
					if (intra_lb_route[cluster_pin_idx].atom_net_id) {
						timing_ctx.tnodes[i].out_edges[k].Tdel = ipb_graph_pin->pin_timing_del_max[j];
						timing_ctx.tnodes[i].out_edges[k].to_node = lookup_tnode_from_pin_id[timing_ctx.tnodes[i].block][ipb_graph_pin->pin_timing[j]->pin_count_in_cluster];
						VTR_ASSERT(timing_ctx.tnodes[i].out_edges[k].to_node != OPEN);
						k++;
					}
				}
				timing_ctx.tnodes[i].num_edges -= (j - k); /* remove unused edges */
				if (timing_ctx.tnodes[i].num_edges == 0) {
					/* Dangling pin */
					num_dangling_pins++;
				}
			}
			break;
		case TN_CB_OPIN:
			/* load up net info */
			i_pin_id = timing_ctx.tnodes[i].pb_graph_pin->pin_count_in_cluster;
			intra_lb_route = cluster_ctx.clb_nlist.block_pb(iblock)->pb_route;
			ipb_graph_pin = intra_lb_pb_pin_lookup[itype][i_pin_id];

			VTR_ASSERT(intra_lb_route[i_pin_id].atom_net_id);
			net_id = atom_ctx.lookup.clb_net(intra_lb_route[i_pin_id].atom_net_id);
			VTR_ASSERT(net_id != ClusterNetId::INVALID());
			f_net_to_driver_tnode[(size_t)net_id] = i;
			timing_ctx.tnodes[i].num_edges = cluster_ctx.clb_nlist.net_sinks(net_id).size();
			timing_ctx.tnodes[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					timing_ctx.tnodes[i].num_edges * sizeof(t_tedge),
					&tedge_ch);
			for (j = 1; j < (int)cluster_ctx.clb_nlist.net_pins(net_id).size(); j++) {
				dblock = cluster_ctx.clb_nlist.net_pin_block(net_id, j);
				normalization = cluster_ctx.clb_nlist.block_type(dblock)->num_pins
						/ cluster_ctx.clb_nlist.block_type(dblock)->capacity;
				normalized_pin = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, j) % normalization;
				d_intra_lb_route = cluster_ctx.clb_nlist.block_pb(dblock)->pb_route;
				dpin = OPEN;
				dport = OPEN;
				count = 0;

				for (k = 0; k < cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_input_ports && dpin == OPEN; k++) {
					if (normalized_pin >= count
                        && (count + cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_input_pins[k] > normalized_pin)) {
						dpin = normalized_pin - count;
						dport = k;
						break;
					}
					count += cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_input_pins[k];
				}
				if (dpin == OPEN) {
					for (k = 0; k < cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_output_ports && dpin == OPEN; k++) {
						count += cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_output_pins[k];
					}
					for (k = 0; k < cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_clock_ports && dpin == OPEN; k++) {
						if (normalized_pin >= count
                            && (count + cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_clock_pins[k] > normalized_pin)) {
							dpin = normalized_pin - count;
							dport = k;
						}
						count += cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node->num_clock_pins[k];
					}
					VTR_ASSERT(dpin != OPEN);

                    t_pb_graph_node* pb_gnode = cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node;
                    int pin_count_in_cluster = pb_gnode->clock_pins[dport][dpin].pin_count_in_cluster;
					ClusterNetId net_id_check = atom_ctx.lookup.clb_net(d_intra_lb_route[pin_count_in_cluster].atom_net_id);
					VTR_ASSERT(net_id == net_id_check);

					timing_ctx.tnodes[i].out_edges[j - 1].to_node = lookup_tnode_from_pin_id[dblock][pin_count_in_cluster];
					VTR_ASSERT(timing_ctx.tnodes[i].out_edges[j - 1].to_node != OPEN);
				} else {
					VTR_ASSERT(dpin != OPEN);

                    t_pb_graph_node* pb_gnode = cluster_ctx.clb_nlist.block_pb(dblock)->pb_graph_node;
                    int pin_count_in_cluster = pb_gnode->input_pins[dport][dpin].pin_count_in_cluster;
					ClusterNetId net_id_check = atom_ctx.lookup.clb_net(d_intra_lb_route[pin_count_in_cluster].atom_net_id);
					VTR_ASSERT(net_id == net_id_check);

					/* delays are assigned post routing */
					timing_ctx.tnodes[i].out_edges[j - 1].to_node = lookup_tnode_from_pin_id[dblock][pin_count_in_cluster];
					VTR_ASSERT(timing_ctx.tnodes[i].out_edges[j - 1].to_node != OPEN);
				}
				timing_ctx.tnodes[i].out_edges[j - 1].Tdel = 0;
				VTR_ASSERT(net_id != ClusterNetId::INVALID());
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
			VTR_LOG_ERROR(
					"Consistency check failed: Unknown tnode type %d.\n", timing_ctx.tnodes[i].type);
			VTR_ASSERT(0);
			break;
		}
	}
	if(num_dangling_pins > 0) {
		VTR_LOG_WARN(
				"Unconnected logic in design, number of dangling tnodes = %d\n", num_dangling_pins);
	}

    //In some instances, we may end-up with constant generators driving constant generators
    //(which is illegal in the timing graph since constant generators shouldn't have any input
    //edges). So we remove those edges now.
    int const_gen_edge_break_count = 0;
    for(inode = 0; inode < timing_ctx.num_tnodes; ++inode) {
        for(int iedge = 0; iedge <timing_ctx.tnodes[inode].num_edges; ++iedge) {
            int& to_node = timing_ctx.tnodes[inode].out_edges[iedge].to_node;
            if(const_gen_tnodes.count(to_node)) {
                //This edge points to a constant generator, mark it invalid
                VTR_ASSERT(timing_ctx.tnodes[to_node].type == TN_CONSTANT_GEN_SOURCE);

                to_node = DO_NOT_ANALYSE;

                ++const_gen_edge_break_count;
            }
        }
    }
    VTR_LOG("Disconnected %d redundant timing edges to constant generators\n", const_gen_edge_break_count);


	for (int i = 0; i < device_ctx.num_block_types; i++) {
		free_pb_graph_pin_lookup_from_index(intra_lb_pb_pin_lookup[i]);
	}
	free_tnode_lookup_from_pin_id(lookup_tnode_from_pin_id);
	delete[] intra_lb_pb_pin_lookup;
}

/* Allocate timing graph for pre packed netlist
 Count number of tnodes first
 Then connect up tnodes with edges
 */
static void alloc_and_load_tnodes_from_prepacked_netlist(float inter_cluster_net_delay,
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode) {
	t_pb_graph_pin *from_pb_graph_pin, *to_pb_graph_pin;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	/* Determine the number of tnode's */
	timing_ctx.num_tnodes = 0;
    for(AtomBlockId blk_id : atom_ctx.nlist.blocks()) {

		const t_model* model = atom_ctx.nlist.block_model(blk_id);
		if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::INPAD) {
            auto pins = atom_ctx.nlist.block_output_pins(blk_id);
            if(pins.size() == 1) {
                timing_ctx.num_tnodes += 2; //SOURCE and OPIN
            } else {
                VTR_ASSERT(pins.size() == 0); //Swept
            }
		} else if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::OUTPAD) {
            auto pins = atom_ctx.nlist.block_input_pins(blk_id);
            if(pins.size() == 1) {
                timing_ctx.num_tnodes += 2; //SINK and OPIN
            } else {
                VTR_ASSERT(pins.size() == 0); //Swept
            }
		} else {
            int incr;
			if (atom_ctx.nlist.block_is_combinational(blk_id)) {
				incr = 1; //Non-sequential so only an IPIN or OPIN node
			} else {
                VTR_ASSERT(atom_ctx.nlist.block_clock_pins(blk_id).size() > 0);
				incr = 2; //Sequential so both and IPIN/OPIN and a SOURCE/SINK nodes
			}
			int j = 0;
			const t_model_ports* model_port = model->inputs;
			while (model_port) {
                AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, model_port);
                if(port_id) {
                    if (model_port->is_clock == false) {
                        //Non-clock port, so add tnodes for each used input pin
                        for (int k = 0; k < model_port->size; k++) {
                            if(atom_ctx.nlist.port_pin(port_id, k)) {
                                timing_ctx.num_tnodes += incr;
                            }
                        }
                        j++;
                    } else {
                        //A clock port so only an FF_CLOCK node
                        VTR_ASSERT(model_port->is_clock);
                        timing_ctx.num_tnodes++; //TODO: consider multi-bit clock ports
                    }
                }
				model_port = model_port->next;
			}

			j = 0;
			model_port = model->outputs;
			while (model_port) {
                AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, model_port);
                if (port_id) {
                    //Add tnodes for each output pin
                    for (int k = 0; k < model_port->size; k++) {
                        if(atom_ctx.nlist.port_pin(port_id, k)) {
                            timing_ctx.num_tnodes += incr;
                        }
                    }
                    j++;
                }
				model_port = model_port->next;
			}
		}
	}

    /* Allocate the tnodes */
	timing_ctx.tnodes = (t_tnode *)vtr::calloc(timing_ctx.num_tnodes, sizeof(t_tnode));

	/* Allocate space for prepacked_data, which is only used pre-packing. */
    //TODO: get rid of prepacked_data
	for (int inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		timing_ctx.tnodes[inode].prepacked_data = (t_prepacked_tnode_data *) vtr::malloc(sizeof(t_prepacked_tnode_data));
	}

	/* load tnodes, alloc edges for tnodes, load all known tnodes */
	int inode = 0;
    for(AtomBlockId blk_id : atom_ctx.nlist.blocks()) {

		const t_model* model = atom_ctx.nlist.block_model(blk_id);
		if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::INPAD) {
            //A primary input

            //Single output pin
            VTR_ASSERT(atom_ctx.nlist.block_input_pins(blk_id).empty());
            VTR_ASSERT(atom_ctx.nlist.block_clock_pins(blk_id).empty());
            auto pins = atom_ctx.nlist.block_output_pins(blk_id);
            if(pins.size() == 1) {
                auto pin_id = *pins.begin();

                //Look-ups
                atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                //The OPIN
                timing_ctx.tnodes[inode].prepacked_data->model_pin = 0;
                timing_ctx.tnodes[inode].prepacked_data->model_port = 0;
                timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model->outputs;
                timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();
                timing_ctx.tnodes[inode].type = TN_INPAD_OPIN;

                auto net_id = atom_ctx.nlist.pin_net(pin_id);
                timing_ctx.tnodes[inode].num_edges = atom_ctx.nlist.net_sinks(net_id).size();
                timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc(
                        timing_ctx.tnodes[inode].num_edges * sizeof(t_tedge),
                        &tedge_ch);

                //The source
                timing_ctx.tnodes[inode + 1].type = TN_INPAD_SOURCE;
                timing_ctx.tnodes[inode + 1].block = ClusterBlockId::INVALID();
                timing_ctx.tnodes[inode + 1].num_edges = 1;
                timing_ctx.tnodes[inode + 1].out_edges = (t_tedge *) vtr::chunk_malloc( 1 * sizeof(t_tedge), &tedge_ch);
                timing_ctx.tnodes[inode + 1].out_edges->Tdel = 0;
                timing_ctx.tnodes[inode + 1].out_edges->to_node = inode;

                inode += 2;
            } else {
                VTR_ASSERT(pins.size() == 0); //Driven net was swept
            }
		} else if (atom_ctx.nlist.block_type(blk_id) == AtomBlockType::OUTPAD) {
            //A primary input

            //Single input pin
            VTR_ASSERT(atom_ctx.nlist.block_output_pins(blk_id).empty());
            VTR_ASSERT(atom_ctx.nlist.block_clock_pins(blk_id).empty());

            //Single pin
            auto pins = atom_ctx.nlist.block_input_pins(blk_id);
            if(pins.size() == 1) {
                auto pin_id = *pins.begin();

                //Look-ups
                atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                //The IPIN
                timing_ctx.tnodes[inode].prepacked_data->model_pin = 0;
                timing_ctx.tnodes[inode].prepacked_data->model_port = 0;
                timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model->inputs;
                timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();
                timing_ctx.tnodes[inode].type = TN_OUTPAD_IPIN;
                timing_ctx.tnodes[inode].num_edges = 1;
                timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc(1 * sizeof(t_tedge), &tedge_ch);
                timing_ctx.tnodes[inode].out_edges->Tdel = 0;
                timing_ctx.tnodes[inode].out_edges->to_node = inode + 1;

                //The sink
                timing_ctx.tnodes[inode + 1].type = TN_OUTPAD_SINK;
                timing_ctx.tnodes[inode + 1].block = ClusterBlockId::INVALID();
                timing_ctx.tnodes[inode + 1].num_edges = 0;
                timing_ctx.tnodes[inode + 1].out_edges = nullptr;

                inode += 2;
            } else {
                VTR_ASSERT(pins.size() == 0); //Input net was swept
            }
		} else {
            //A regular block

            //Process the block's outputs
			int j = 0;
			t_model_ports* model_port = model->outputs;
			while (model_port) {

                AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, model_port);

                if(port_id) {

                    if (model_port->is_clock == false) {
                        //A non-clock output
                        for (int k = 0; k < model_port->size; k++) {
                            auto pin_id = atom_ctx.nlist.port_pin(port_id, k);
                            if (pin_id) {
                                //Look-ups
                                atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                                //The pin's associated net
                                auto net_id = atom_ctx.nlist.pin_net(pin_id);

                                //The first tnode
                                timing_ctx.tnodes[inode].prepacked_data->model_pin = k;
                                timing_ctx.tnodes[inode].prepacked_data->model_port = j;
                                timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model_port;
                                timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();
                                timing_ctx.tnodes[inode].num_edges = atom_ctx.nlist.net_sinks(net_id).size();
                                timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc( timing_ctx.tnodes[inode].num_edges * sizeof(t_tedge), &tedge_ch);

                                if (atom_ctx.nlist.block_is_combinational(blk_id)) {
                                    //Non-sequentail block so only a single OPIN tnode
                                    timing_ctx.tnodes[inode].type = TN_PRIMITIVE_OPIN;

                                    inode++;
                                } else {
                                    VTR_ASSERT(atom_ctx.nlist.block_clock_pins(blk_id).size() > 0);
                                    //A sequential block, so the first tnode was an FF_OPIN
                                    timing_ctx.tnodes[inode].type = TN_FF_OPIN;

                                    //The second tnode is the FF_SOURCE
                                    timing_ctx.tnodes[inode + 1].type = TN_FF_SOURCE;
                                    timing_ctx.tnodes[inode + 1].block = ClusterBlockId::INVALID();

                                    //Initialize the edge between SOURCE and OPIN with the clk-to-q delay
                                    auto iter = expected_lowest_cost_pb_gnode.find(blk_id);
                                    VTR_ASSERT(iter != expected_lowest_cost_pb_gnode.end());

                                    from_pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, k,
                                                            iter->second);
                                    timing_ctx.tnodes[inode + 1].num_edges = 1;
                                    timing_ctx.tnodes[inode + 1].out_edges = (t_tedge *) vtr::chunk_malloc( 1 * sizeof(t_tedge), &tedge_ch);
                                    timing_ctx.tnodes[inode + 1].out_edges->to_node = inode;
                                    timing_ctx.tnodes[inode + 1].out_edges->Tdel = from_pb_graph_pin->tco_max; //Set the clk-to-Q delay

                                    inode += 2;
                                }
                            }
                        }
                    } else {
                        //An output clock port, meaning a clock source (e.g. PLL output)

                        //For every non-empty pin on the clock port create a clock pin and clock source tnode
                        for (int k = 0; k < model_port->size; k++) {
                            auto pin_id = atom_ctx.nlist.port_pin(port_id, k);
                            if (pin_id) {
                                //Look-ups
                                atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                                //Create the OPIN
                                timing_ctx.tnodes[inode].type = TN_CLOCK_OPIN;

                                timing_ctx.tnodes[inode].prepacked_data->model_pin = k;
                                timing_ctx.tnodes[inode].prepacked_data->model_port = j;
                                timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model_port;
                                timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();

                                //Allocate space for the output edges
                                auto net_id = atom_ctx.nlist.pin_net(pin_id);
                                timing_ctx.tnodes[inode].num_edges = atom_ctx.nlist.net_sinks(net_id).size();
                                timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc( timing_ctx.tnodes[inode].num_edges * sizeof(t_tedge), &tedge_ch);


                                //Create the clock SOURCE
                                auto iter = expected_lowest_cost_pb_gnode.find(blk_id);
                                VTR_ASSERT(iter != expected_lowest_cost_pb_gnode.end());

                                timing_ctx.tnodes[inode + 1].type = TN_CLOCK_SOURCE;
                                timing_ctx.tnodes[inode + 1].block = ClusterBlockId::INVALID();
                                timing_ctx.tnodes[inode + 1].num_edges = 1;
                                timing_ctx.tnodes[inode + 1].prepacked_data->model_pin = k;
                                timing_ctx.tnodes[inode + 1].prepacked_data->model_port = j;
                                timing_ctx.tnodes[inode + 1].prepacked_data->model_port_ptr = model_port;

                                //Initialize the edge between them
                                timing_ctx.tnodes[inode + 1].out_edges = (t_tedge *) vtr::chunk_malloc( 1 * sizeof(t_tedge), &tedge_ch);
                                timing_ctx.tnodes[inode + 1].out_edges->to_node = inode;
                                timing_ctx.tnodes[inode + 1].out_edges->Tdel = 0.; //PLL output delay? Not clear what this reallly means... perhaps clock insertion delay from PLL?

                                inode += 2;
                            }
                        }
                    }
                }
                j++;
				model_port = model_port->next;
			}

            //Process the block's inputs
			j = 0;
			model_port = model->inputs;
			while (model_port) {

                AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, model_port);
                if(port_id) {

                    if (model_port->is_clock == false) {
                        //A non-clock (i.e. data) input

                        //For every non-empty pin create the associated tnode(s)
                        for (int k = 0; k < model_port->size; k++) {
                            auto pin_id = atom_ctx.nlist.port_pin(port_id, k);
                            if (pin_id) {

                                //Look-ups
                                atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                                //Initialize the common part of the first tnode
                                timing_ctx.tnodes[inode].prepacked_data->model_pin = k;
                                timing_ctx.tnodes[inode].prepacked_data->model_port = j;
                                timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model_port;
                                timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();

                                auto iter = expected_lowest_cost_pb_gnode.find(blk_id);
                                VTR_ASSERT(iter != expected_lowest_cost_pb_gnode.end());

                                from_pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, k,
                                                        iter->second);

                                if (atom_ctx.nlist.block_is_combinational(blk_id)) {
                                    //A non-sequential/combinational block

                                    timing_ctx.tnodes[inode].type = TN_PRIMITIVE_IPIN;

                                    //Allocate space for all possible the edges
                                    timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc( from_pb_graph_pin->num_pin_timing * sizeof(t_tedge), &tedge_ch);

                                    //Initialize the delay and sink tnodes (i.e. PRIMITIVE_OPINs)
                                    int count = 0;
                                    for(int m = 0; m < from_pb_graph_pin->num_pin_timing; m++) {
                                        to_pb_graph_pin = from_pb_graph_pin->pin_timing[m];

                                        //Find the tnode associated with the sink port & pin
                                        auto sink_blk_id = blk_id; //Within a single atom, so the source and sink blocks are the same
                                        auto sink_port_id = atom_ctx.nlist.find_atom_port(sink_blk_id, to_pb_graph_pin->port->model_port);
                                        if(sink_port_id) {
                                            auto sink_pin_id = atom_ctx.nlist.port_pin(sink_port_id, to_pb_graph_pin->pin_number);
                                            if(sink_pin_id) {
                                                //Pin is in use

                                                int to_node = atom_ctx.lookup.atom_pin_classic_tnode(sink_pin_id);
                                                VTR_ASSERT(to_node != OPEN);

                                                //Skip pins with no connections
                                                if(!sink_pin_id) {
                                                    continue;
                                                }

                                                //Mark the delay
                                                timing_ctx.tnodes[inode].out_edges[count].Tdel = from_pb_graph_pin->pin_timing_del_max[m];

                                                VTR_ASSERT(timing_ctx.tnodes[to_node].type == TN_PRIMITIVE_OPIN);

                                                timing_ctx.tnodes[inode].out_edges[count].to_node = to_node;

                                                count++;
                                            }
                                        }
                                    }
                                    //Mark the number of used edges (note that this may be less than the number allocated, since
                                    //some allocated edges may correspond to pins which were not connected)
                                    timing_ctx.tnodes[inode].num_edges = count;
                                    inode++;
                                } else {
                                    VTR_ASSERT(atom_ctx.nlist.block_clock_pins(blk_id).size() > 0);

                                    //A sequential input
                                    timing_ctx.tnodes[inode].type = TN_FF_IPIN;

                                    //Allocate the edge to the sink, and mark the setup-time on it
                                    timing_ctx.tnodes[inode].num_edges = 1;
                                    timing_ctx.tnodes[inode].out_edges = (t_tedge *) vtr::chunk_malloc( 1 * sizeof(t_tedge), &tedge_ch);
                                    timing_ctx.tnodes[inode].out_edges->to_node = inode + 1;
                                    timing_ctx.tnodes[inode].out_edges->Tdel = from_pb_graph_pin->tsu;

                                    //Initialize the FF_SINK node
                                    timing_ctx.tnodes[inode + 1].type = TN_FF_SINK;
                                    timing_ctx.tnodes[inode + 1].num_edges = 0;
                                    timing_ctx.tnodes[inode + 1].out_edges = nullptr;
                                    timing_ctx.tnodes[inode + 1].block = ClusterBlockId::INVALID();

                                    inode += 2;
                                }
                            }
                        }
                        j++;
                    } else {
                        //A clock input
                        //TODO: consider more than one clock per primitive
                        auto pins = atom_ctx.nlist.port_pins(port_id);
                        VTR_ASSERT(pins.size() == 1);
                        auto pin_id = *pins.begin();

                        if (pin_id) {
                            //Look-up
                            atom_ctx.lookup.set_atom_pin_classic_tnode(pin_id, inode);

                            //Initialize the clock tnode
                            timing_ctx.tnodes[inode].type = TN_FF_CLOCK;
                            timing_ctx.tnodes[inode].block = ClusterBlockId::INVALID();
                            timing_ctx.tnodes[inode].prepacked_data->model_pin = 0;
                            timing_ctx.tnodes[inode].prepacked_data->model_port = 0;
                            timing_ctx.tnodes[inode].prepacked_data->model_port_ptr = model_port;
                            timing_ctx.tnodes[inode].num_edges = 0;
                            timing_ctx.tnodes[inode].out_edges = nullptr;

                            inode++;
                        }
                    }
                }
				model_port = model_port->next;
			}
		}
	}
	VTR_ASSERT(inode == timing_ctx.num_tnodes);

	/* Load net delays and initialize clock domains to OPEN. */
    std::set<int> const_gen_tnodes;
	for (int i = 0; i < timing_ctx.num_tnodes; i++) {
		timing_ctx.tnodes[i].clock_domain = OPEN;

        //Since the setup/clock-to-q/combinational delays have already been set above,
        //we only set the net related delays here
		switch (timing_ctx.tnodes[i].type) {
            case TN_INPAD_OPIN:     //fallthrough
            case TN_PRIMITIVE_OPIN: //fallthrough
            case TN_FF_OPIN:        //fallthrough
            case TN_CLOCK_OPIN: {
                /* An output pin tnode */

                //Find the net driven by the OPIN
                auto pin_id = atom_ctx.lookup.classic_tnode_atom_pin(i);
                auto net_id = atom_ctx.nlist.pin_net(pin_id);
                VTR_ASSERT(net_id);
                VTR_ASSERT_MSG(pin_id == atom_ctx.nlist.net_driver(net_id), "OPIN must be net driver");

                bool is_const_net = atom_ctx.nlist.pin_is_constant(pin_id);
                //TODO: const generators should really be treated by sources launching at t=-inf
                if(is_const_net) {
                    VTR_ASSERT(timing_ctx.tnodes[i].type == TN_PRIMITIVE_OPIN);
                    timing_ctx.tnodes[i].type = TN_CONSTANT_GEN_SOURCE;
                    const_gen_tnodes.insert(i);
                }

                //Look at each sink
                int j = 0;
                for(auto sink_pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                    if (is_const_net) {
                        //The net is a constant generator, we override the driver
                        //tnode type appropriately and set the delay to a large
                        //negative value to ensure the signal is treated as already
                        //'arrived'
                        timing_ctx.tnodes[i].out_edges[j].Tdel = HUGE_NEGATIVE_FLOAT;
                    } else {
                        //This is a real net, so use the default delay
                        timing_ctx.tnodes[i].out_edges[j].Tdel = inter_cluster_net_delay;
                    }

                    //Find the sink tnode
                    int to_node = atom_ctx.lookup.atom_pin_classic_tnode(sink_pin_id);
                    VTR_ASSERT(to_node != OPEN);

                    //Connect the edge to the sink
                    timing_ctx.tnodes[i].out_edges[j].to_node = to_node;
                    ++j;
                }
                VTR_ASSERT(timing_ctx.tnodes[i].num_edges == static_cast<int>(atom_ctx.nlist.net_sinks(net_id).size()));
                break;
            }
            case TN_PRIMITIVE_IPIN: //fallthrough
            case TN_OUTPAD_IPIN:    //fallthrough
            case TN_INPAD_SOURCE:   //fallthrough
            case TN_OUTPAD_SINK:    //fallthrough
            case TN_FF_SINK:        //fallthrough
            case TN_FF_SOURCE:      //fallthrough
            case TN_FF_IPIN:        //fallthrough
            case TN_FF_CLOCK:       //fallthrough
            case TN_CLOCK_SOURCE:   //fallthrough
                /* All other tnode's have had their edge-delays initialized */
                break;
            default:
                VPR_THROW(VPR_ERROR_TIMING, "Unknown tnode type %d.\n", timing_ctx.tnodes[i].type);
                break;
		}
	}

    //In some instances, we may end-up with constant generators driving constant generators
    //(which is illegal in the timing graph since constant generators shouldn't have any input
    //edges). So we remove those edges now.
    int const_gen_edge_break_count = 0;
    for(inode = 0; inode < timing_ctx.num_tnodes; ++inode) {
        for(int iedge = 0; iedge <timing_ctx.tnodes[inode].num_edges; ++iedge) {
            int& to_node = timing_ctx.tnodes[inode].out_edges[iedge].to_node;
            if(const_gen_tnodes.count(to_node)) {
                //This edge points to a constant generator, mark it invalid
                VTR_ASSERT(timing_ctx.tnodes[to_node].type == TN_CONSTANT_GEN_SOURCE);

                to_node = DO_NOT_ANALYSE;

                ++const_gen_edge_break_count;
            }
        }
    }
    VTR_LOG("Disconnected %d redundant timing edges to constant generators\n", const_gen_edge_break_count);


    //Build the net to driver look-up
    //  TODO: convert to use net_id directly when timing graph re-unified
	f_net_to_driver_tnode = (int*)vtr::malloc(num_timing_nets() * sizeof(int));
    auto nets = atom_ctx.nlist.nets();
	for (size_t i = 0; i < num_timing_nets(); i++) {
        auto net_id = *(nets.begin() + i); //XXX: Ugly hack relying on sequentially increasing net id's
        VTR_ASSERT(net_id);

        auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
        VTR_ASSERT(driver_pin_id);

        f_net_to_driver_tnode[i] = atom_ctx.lookup.atom_pin_classic_tnode(driver_pin_id);
    }

    //Sanity check, every net should have a valid driver tnode
	for (size_t i = 0; i < num_timing_nets(); i++) {
		VTR_ASSERT(f_net_to_driver_tnode[i] != OPEN);
	}
}

static void load_tnode(t_pb_graph_pin *pb_graph_pin, const ClusterBlockId iblock,
		int *inode) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	int i = *inode;
	timing_ctx.tnodes[i].pb_graph_pin = pb_graph_pin;
	timing_ctx.tnodes[i].block = iblock;


	if (timing_ctx.tnodes[i].pb_graph_pin->parent_node->pb_type->blif_model == nullptr) {
		VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_NORMAL);
		if (timing_ctx.tnodes[i].pb_graph_pin->parent_node->parent_pb_graph_node == nullptr) {
			if (timing_ctx.tnodes[i].pb_graph_pin->port->type == IN_PORT) {
				timing_ctx.tnodes[i].type = TN_CB_IPIN;
			} else {
				VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->port->type == OUT_PORT);
				timing_ctx.tnodes[i].type = TN_CB_OPIN;
			}
		} else {
			timing_ctx.tnodes[i].type = TN_INTERMEDIATE_NODE;
		}
	} else {
        AtomPinId atom_pin = find_atom_pin(iblock, pb_graph_pin);

		if (timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_INPAD) {
			VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->port->type == OUT_PORT);
			timing_ctx.tnodes[i].type = TN_INPAD_OPIN;
			timing_ctx.tnodes[i + 1].num_edges = 1;
			timing_ctx.tnodes[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			timing_ctx.tnodes[i + 1].out_edges->Tdel = 0;
			timing_ctx.tnodes[i + 1].out_edges->to_node = i;
			timing_ctx.tnodes[i + 1].pb_graph_pin = pb_graph_pin; /* Necessary for propagate_clock_domain_and_skew(). */
			timing_ctx.tnodes[i + 1].type = TN_INPAD_SOURCE;
			timing_ctx.tnodes[i + 1].block = iblock;


            atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i + 1);

			(*inode)++;
		} else if (timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_OUTPAD) {
			VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->port->type == IN_PORT);
			timing_ctx.tnodes[i].type = TN_OUTPAD_IPIN;
			timing_ctx.tnodes[i].num_edges = 1;
			timing_ctx.tnodes[i].out_edges = (t_tedge *) vtr::chunk_malloc(
					1 * sizeof(t_tedge), &tedge_ch);
			timing_ctx.tnodes[i].out_edges->Tdel = 0;
			timing_ctx.tnodes[i].out_edges->to_node = i + 1;
			timing_ctx.tnodes[i + 1].pb_graph_pin = pb_graph_pin; /* Necessary for find_tnode_net_name(). */
			timing_ctx.tnodes[i + 1].type = TN_OUTPAD_SINK;
			timing_ctx.tnodes[i + 1].block = iblock;
			timing_ctx.tnodes[i + 1].num_edges = 0;
			timing_ctx.tnodes[i + 1].out_edges = nullptr;

            atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i + 1);
			(*inode)++;
		} else if (timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_SEQUENTIAL) {
			if (timing_ctx.tnodes[i].pb_graph_pin->port->type == IN_PORT) {
				timing_ctx.tnodes[i].type = TN_FF_IPIN;
				timing_ctx.tnodes[i].num_edges = 1;
				timing_ctx.tnodes[i].out_edges = (t_tedge *) vtr::chunk_malloc(
						1 * sizeof(t_tedge), &tedge_ch);
				timing_ctx.tnodes[i].out_edges->Tdel = pb_graph_pin->tsu;
				timing_ctx.tnodes[i].out_edges->to_node = i + 1;
				timing_ctx.tnodes[i + 1].pb_graph_pin = pb_graph_pin;
				timing_ctx.tnodes[i + 1].type = TN_FF_SINK;
				timing_ctx.tnodes[i + 1].block = iblock;
				timing_ctx.tnodes[i + 1].num_edges = 0;
				timing_ctx.tnodes[i + 1].out_edges = nullptr;

                atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i + 1);
			} else {
				VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->port->type == OUT_PORT);
                //Determine whether we are a standard clocked output pin (TN_FF_OPIN with TN_FF_SOURCE)
                //or a clock source (TN_CLOCK_OPIN with TN_CLOCK_SOURCE)
                t_model_ports* model_port = timing_ctx.tnodes[i].pb_graph_pin->port->model_port;
                if(!model_port->is_clock) {
                    //Standard sequential output
                    timing_ctx.tnodes[i].type = TN_FF_OPIN;
                    timing_ctx.tnodes[i + 1].num_edges = 1;
                    timing_ctx.tnodes[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
                            1 * sizeof(t_tedge), &tedge_ch);
                    timing_ctx.tnodes[i + 1].out_edges->Tdel = pb_graph_pin->tco_max;
                    timing_ctx.tnodes[i + 1].out_edges->to_node = i;
                    timing_ctx.tnodes[i + 1].pb_graph_pin = pb_graph_pin;
                    timing_ctx.tnodes[i + 1].type = TN_FF_SOURCE;
                    timing_ctx.tnodes[i + 1].block = iblock;

                    atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i + 1);
                } else {
                    //Clock source
                    timing_ctx.tnodes[i].type = TN_CLOCK_OPIN;
                    timing_ctx.tnodes[i + 1].num_edges = 1;
                    timing_ctx.tnodes[i + 1].out_edges = (t_tedge *) vtr::chunk_malloc(
                            1 * sizeof(t_tedge), &tedge_ch);
                    timing_ctx.tnodes[i + 1].out_edges->Tdel = 0.; //Not clear what this delay physically represents
                    timing_ctx.tnodes[i + 1].out_edges->to_node = i;
                    timing_ctx.tnodes[i + 1].pb_graph_pin = pb_graph_pin;
                    timing_ctx.tnodes[i + 1].type = TN_CLOCK_SOURCE;
                    timing_ctx.tnodes[i + 1].block = iblock;

                    atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i + 1);
                }
			}
			(*inode)++;
		} else if (timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_CLOCK) {
			timing_ctx.tnodes[i].type = TN_FF_CLOCK;
			timing_ctx.tnodes[i].num_edges = 0;
			timing_ctx.tnodes[i].out_edges = nullptr;

            atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i);
		} else {
			if (timing_ctx.tnodes[i].pb_graph_pin->port->type == IN_PORT) {
				VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_TERMINAL);
				timing_ctx.tnodes[i].type = TN_PRIMITIVE_IPIN;

                atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i);
			} else {
				VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->port->type == OUT_PORT);
				VTR_ASSERT(timing_ctx.tnodes[i].pb_graph_pin->type == PB_PIN_TERMINAL);
				timing_ctx.tnodes[i].type = TN_PRIMITIVE_OPIN;

                atom_ctx.lookup.set_atom_pin_classic_tnode(atom_pin, i);
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

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& timing_ctx = g_vpr_ctx.timing();

	fp = vtr::fopen(fname, "w");

	fprintf(fp, "timing_ctx.num_tnodes: %d\n", timing_ctx.num_tnodes);
	fprintf(fp, "Node #\tType\t\tipin\tiblk\tDomain\tSkew\tI/O Delay\t# edges\t"
			"to_node     Tdel\n\n");

	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		fprintf(fp, "%d\t", inode);

		itype = timing_ctx.tnodes[inode].type;
		fprintf(fp, "%-15.15s\t", tnode_type_names[itype]);

		if (timing_ctx.tnodes[inode].pb_graph_pin != nullptr) {
			fprintf(fp, "%d\t%zu\t",
					timing_ctx.tnodes[inode].pb_graph_pin->pin_count_in_cluster,
					size_t(timing_ctx.tnodes[inode].block));
		} else {
			fprintf(fp, "\t%zu\t", size_t(timing_ctx.tnodes[inode].block));
		}

		if (itype == TN_FF_CLOCK || itype == TN_FF_SOURCE || itype == TN_FF_SINK) {
			fprintf(fp, "%d\t%.3e\t\t", timing_ctx.tnodes[inode].clock_domain, timing_ctx.tnodes[inode].clock_delay);
		} else if (itype == TN_INPAD_SOURCE || itype ==TN_CLOCK_SOURCE) {
			fprintf(fp, "%d\t\t%.3e\t", timing_ctx.tnodes[inode].clock_domain, timing_ctx.tnodes[inode].out_edges ? timing_ctx.tnodes[inode].out_edges[0].Tdel : -1);
		} else if (itype == TN_OUTPAD_SINK) {
			VTR_ASSERT(timing_ctx.tnodes[inode-1].type == TN_OUTPAD_IPIN); /* Outpad ipins should be one prior in the tnode array */
			fprintf(fp, "%d\t\t%.3e\t", timing_ctx.tnodes[inode].clock_domain, timing_ctx.tnodes[inode-1].out_edges[0].Tdel);
		} else {
			fprintf(fp, "\t\t\t\t");
		}

		fprintf(fp, "%d", timing_ctx.tnodes[inode].num_edges);

		/* Print all edges after edge 0 on separate lines */
		tedge = timing_ctx.tnodes[inode].out_edges;
		if (timing_ctx.tnodes[inode].num_edges > 0) {
			fprintf(fp, "\t%4d\t%7.3g", tedge[0].to_node, tedge[0].Tdel);
			for (iedge = 1; iedge < timing_ctx.tnodes[inode].num_edges; iedge++) {
				fprintf(fp, "\n\t\t\t\t\t\t\t\t\t\t%4d\t%7.3g", tedge[iedge].to_node, tedge[iedge].Tdel);
			}
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "\n\ntiming_ctx.num_tnode_levels: %d\n", timing_ctx.num_tnode_levels);

	for (ilevel = 0; ilevel < timing_ctx.num_tnode_levels; ilevel++) {
		fprintf(fp, "\n\nLevel: %d  Num_nodes: %zu\nNodes:", ilevel,
				timing_ctx.tnodes_at_level[ilevel].size());
		for (unsigned j = 0; j < timing_ctx.tnodes_at_level[ilevel].size(); j++)
			fprintf(fp, "\t%d", timing_ctx.tnodes_at_level[ilevel][j]);
	}

	fprintf(fp, "\n");
	fprintf(fp, "\n\nNet #\tNet_to_driver_tnode\n");

	for (i = 0; i < (int) cluster_ctx.clb_nlist.nets().size(); i++)
		fprintf(fp, "%4d\t%6d\n", i, f_net_to_driver_tnode[i]);

	if (timing_ctx.sdc && timing_ctx.sdc->num_constrained_clocks == 1) {

		/* Arrival and required times, and forward and backward weights, will be meaningless for multiclock
		designs, since the values currently on the graph will only correspond to the most recent traversal. */
		fprintf(fp, "\n\nNode #\t\tT_arr\t\tT_req"
#ifdef PATH_COUNTING
			"\tForward weight\tBackward weight"
#endif
			"\n\n");

		for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
			if (timing_ctx.tnodes[inode].T_arr > HUGE_NEGATIVE_FLOAT + 1) {
				fprintf(fp, "%d\t%12g", inode, timing_ctx.tnodes[inode].T_arr);
			} else {
				fprintf(fp, "%d\t\t   -", inode);
			}
			if (timing_ctx.tnodes[inode].T_req < HUGE_POSITIVE_FLOAT - 1) {
				fprintf(fp, "\t%12g", timing_ctx.tnodes[inode].T_req);
			} else {
				fprintf(fp, "\t\t   -");
			}
#ifdef PATH_COUNTING
			fprintf(fp, "\t%12g\t%12g", timing_ctx.tnodes[inode].forward_weight, timing_ctx.tnodes[inode].backward_weight);
#endif
            fprintf(fp, "\n");
		}
	}

	fclose(fp);
}
static void process_constraints() {
	/* Removes all constraints between domains which never intersect. We need to do this
	so that criticality_denom in do_timing_analysis is not affected	by unused constraints.
	BFS through the levelized graph once for each source domain. Whenever we get to a sink,
	mark off that we've used that sink clock domain.  After	each traversal, set all unused
	constraints to DO_NOT_ANALYSE.

	Also, print timing_ctx.sdc->domain_constraints, constrained I/Os and override constraints,
	and convert timing_ctx.sdc->domain_constraints and flip-flop-level override constraints
	to be in seconds rather than nanoseconds. We don't need to normalize timing_ctx.sdc->cc_constraints
	because they're already on the timing_ctx.sdc->domain_constraints matrix, and we don't need
	to normalize constrained_ios because we already did the normalization when
	we put the delays onto the timing graph in load_clock_domain_and_clock_and_io_delay. */

	int source_clock_domain, sink_clock_domain, inode, ilevel, num_at_level, i,
		num_edges, iedge, to_node, icf, ifc, iff;
	t_tedge * tedge;
	float constraint;

    auto& timing_ctx = g_vpr_ctx.timing();

	bool * constraint_used = (bool *) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(bool));

	for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		/* We're going to use arrival time to flag which nodes we've reached,
		even though the values we put in will not correspond to actual arrival times.
		Nodes which are reached on this traversal will get an arrival time of 0.
		Reset arrival times now to an invalid number. */
		for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
			timing_ctx.tnodes[inode].T_arr = HUGE_NEGATIVE_FLOAT;
		}

		/* Reset all constraint_used entries. */
		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			constraint_used[sink_clock_domain] = false;
		}

		/* Set arrival times for each top-level tnode on this clock domain. */
        if(!timing_ctx.tnodes_at_level.empty()) {
            num_at_level = timing_ctx.tnodes_at_level[0].size();
        } else {
            num_at_level = 0;
        }
		for (i = 0; i < num_at_level; i++) {
			inode = timing_ctx.tnodes_at_level[0][i];
			if (timing_ctx.tnodes[inode].clock_domain == source_clock_domain) {
				timing_ctx.tnodes[inode].T_arr = 0.;
			}
		}

		for (ilevel = 0; ilevel < timing_ctx.num_tnode_levels; ilevel++) {	/* Go down one level at a time. */
			num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

			for (i = 0; i < num_at_level; i++) {
				inode = timing_ctx.tnodes_at_level[ilevel][i];	/* Go through each of the tnodes at the level we're on. */
				if (has_valid_T_arr(inode)) {	/* If this tnode has been used */
					num_edges = timing_ctx.tnodes[inode].num_edges;
					if (num_edges == 0) { /* sink */
						/* We've reached the sink domain of this tnode, so set constraint_used
						to true for this tnode's clock domain (if it has a valid one). */
						sink_clock_domain = timing_ctx.tnodes[inode].clock_domain;
						if (sink_clock_domain != -1) {
							constraint_used[sink_clock_domain] = true;
						}
					} else {
						/* Set arrival time to a valid value (0.) for each tnode in this tnode's fanout. */
						tedge = timing_ctx.tnodes[inode].out_edges;
						for (iedge = 0; iedge < num_edges; iedge++) {
							to_node = tedge[iedge].to_node;
							if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

							timing_ctx.tnodes[to_node].T_arr = 0.;
						}
					}
				}
			}
		}

		/* At the end of the source domain traversal, see which sink domains haven't been hit,
		and set the constraint for the pair of source and sink domains to DO_NOT_ANALYSE */

		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			if (!constraint_used[sink_clock_domain]) {
                if(timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] != DO_NOT_ANALYSE) {
                    VTR_LOG_WARN( "Timing constraint from clock %d to %d of value %f will be disabled"
                                                           " since it is not activated by any path in the timing graph.\n",
                                                           source_clock_domain, sink_clock_domain,
                                                           timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain]);
                }
				timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] = DO_NOT_ANALYSE;
			}
		}
	}

	free(constraint_used);

	/* Print constraints */
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_TIMING_CONSTRAINTS)) {
		print_timing_constraint_info(getEchoFileName(E_ECHO_TIMING_CONSTRAINTS));
	}

	/* Convert timing_ctx.sdc->domain_constraint and ff-level override constraints to be in seconds, not nanoseconds. */
	for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			constraint = timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain];
			if (constraint > NEGATIVE_EPSILON) { /* if constraint does not equal DO_NOT_ANALYSE */
				timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] = constraint * 1e-9;
			}
		}
	}
	for (icf = 0; icf < timing_ctx.sdc->num_cf_constraints; icf++) {
		timing_ctx.sdc->cf_constraints[icf].constraint *= 1e-9;
	}
	for (ifc = 0; ifc < timing_ctx.sdc->num_fc_constraints; ifc++) {
		timing_ctx.sdc->fc_constraints[ifc].constraint *= 1e-9;
	}
	for (iff = 0; iff < timing_ctx.sdc->num_ff_constraints; iff++) {
		timing_ctx.sdc->ff_constraints[iff].constraint *= 1e-9;
	}

	/* Finally, free timing_ctx.sdc->cc_constraints since all of its info is contained in timing_ctx.sdc->domain_constraint. */
	free_override_constraint(timing_ctx.sdc->cc_constraints, timing_ctx.sdc->num_cc_constraints);
}

static void alloc_timing_stats() {

	/* Allocate f_timing_stats data structure. */

	int i;

    auto& timing_ctx = g_vpr_ctx.timing();

	f_timing_stats = (t_timing_stats *) vtr::malloc(sizeof(t_timing_stats));
	f_timing_stats->cpd = (float **) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(float *));
	f_timing_stats->least_slack = (float **) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(float *));
	for (i = 0; i < timing_ctx.sdc->num_constrained_clocks; i++) {
		f_timing_stats->cpd[i] = (float *) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(float));
		f_timing_stats->least_slack[i] = (float *) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(float));
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

	The timing graph nodes are stored as an array, tnode [0..timing_ctx.num_tnodes - 1]. Each tnode
	includes an array of all edges, tedge, which fan out from it. Each tedge includes the
	index of the node on its far end (in the tnode array), and the delay to that node.

	The timing graph has sources at each TN_FF_SOURCE (Q output), TN_INPAD_SOURCE (input I/O pad)
	and TN_CONSTANT_GEN_SOURCE (constant 1 or 0 generator) node and sinks at TN_FF_SINK (D input)
	and TN_OUTPAD_SINK (output I/O pad) nodes. Two traversals, one forward (sources to sinks)
	and one backward, are performed for each valid constraint (one which is not DO_NOT_ANALYSE)
	between a source and a sink clock domain in the matrix timing_ctx.sdc->domain_constraint
	[0..timing_ctx.sdc->num_constrained_clocks - 1][0..timing_ctx.sdc->num_constrained_clocks - 1]. This matrix has been
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
	critical path delay per constraint [0..timing_ctx.sdc->num_constrained_clocks - 1][0..timing_ctx.sdc->num_constrained_clocks - 1]
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
	and especially set_multicycle_path). All the info for these constraints is contained in timing_ctx.sdc->fc_constraints and
	timing_ctx.sdc->ff_constraints, but graph traversals are not included yet. Probably, an entire traversal will be needed for
	each constraint. Clock domain to flip-flop constraints are coded but not tested, and are done within
	existing traversals. */

    /* Description of slack_definition:
    slack_definition determines how to normalize negative slacks for the optimizers (not in the final timing analysis
    for output statistics):
    'R' (T_req-relaxed): For each constraint, set the required time at sink nodes to the max of the true required time
    (constraint + timing_ctx.tnodes[inode].clock_skew) and the max arrival time. This means that the required time is "relaxed"
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

	float smallest_slack_in_design = HUGE_POSITIVE_FLOAT;
	/* For slack_definition == 'S', shift all slacks upwards by this number if it is negative. */

    float criticality_denom_global = HUGE_NEGATIVE_FLOAT;
    /* Denominator of criticality for slack_definition == 'S' || slack_definition == 'G' -
    max of all arrival times and all constraints. */

    update_slack = bool(timing_inf.slack_definition == std::string("S") || timing_inf.slack_definition == std::string("G"));
	/* Update slack values for certain slack definitions where they are needed to compute timing criticalities. */

    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	/* Reset all values which need to be reset once per
	timing analysis, rather than once per traversal pair. */

	/* Reset slack and criticality */
	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		for (int ipin = 1; ipin < (int) num_timing_net_pins(inet); ipin++) {
			slacks->slack[inet][ipin]			   = HUGE_POSITIVE_FLOAT;
			slacks->timing_criticality[inet][ipin] = 0.;
#ifdef PATH_COUNTING
			slacks->path_criticality[inet][ipin] = 0.;
#endif
		}
	}

	/* Reset f_timing_stats. */
	for (int i = 0; i < timing_ctx.sdc->num_constrained_clocks; i++) {
		for (int j = 0; j < timing_ctx.sdc->num_constrained_clocks; j++) {
			f_timing_stats->cpd[i][j]		  = HUGE_NEGATIVE_FLOAT;
			f_timing_stats->least_slack[i][j] = HUGE_POSITIVE_FLOAT;
		}
	}

#ifndef PATH_COUNTING
	/* Reset normalized values for clusterer. */
	if (is_prepacked) {
		for (int inode = 0; inode < timing_ctx.num_tnodes; inode++) {
			timing_ctx.tnodes[inode].prepacked_data->normalized_slack = HUGE_POSITIVE_FLOAT;
			timing_ctx.tnodes[inode].prepacked_data->normalized_T_arr = HUGE_NEGATIVE_FLOAT;
			timing_ctx.tnodes[inode].prepacked_data->normalized_total_critical_paths = HUGE_NEGATIVE_FLOAT;
		}
	}
#endif

	vtr::vector_map<ClusterBlockId, t_pb **> pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();

    if (timing_inf.slack_definition == std::string("I")) {
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

	for (int source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (int sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
            if (timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) { /* i.e. != DO_NOT_ANALYSE */

                /* Perform the forward and backward traversal for this constraint. */
                criticality_denom = do_timing_analysis_for_constraint(source_clock_domain, sink_clock_domain,
                    is_prepacked, is_final_analysis, &max_critical_input_paths, &max_critical_output_paths,
                    pin_id_to_pb_mapping, timing_inf);
#ifdef PATH_COUNTING
                /* Weight the importance of each net, used in slack calculation. */
                do_path_counting(criticality_denom);
#endif

                if (timing_inf.slack_definition == std::string("I")) {
                    criticality_denom -= smallest_slack_in_design;
                    /* Remember, smallest_slack_in_design is negative, so we're INCREASING criticality_denom. */
                }

				/* Update the slack and criticality for each edge of each net which was
				analysed on the most recent traversal and has a lower (slack) or
				higher (criticality) value than before. */
				update_slacks(slacks, criticality_denom,
					update_slack, is_final_analysis, smallest_slack_in_design, timing_inf);

#ifndef PATH_COUNTING
				/* Update the normalized costs used by the clusterer. */
				if (is_prepacked) {
					update_normalized_costs(criticality_denom, max_critical_input_paths, max_critical_output_paths, timing_inf);
				}
#endif

                if (timing_inf.slack_definition == std::string("S") || timing_inf.slack_definition == std::string("G")) {
				    /* Set criticality_denom_global to the max of criticality_denom over all traversals. */
				    criticality_denom_global = max(criticality_denom_global, criticality_denom);
                }
			}
		}
	}

#ifdef PATH_COUNTING
	/* Normalize path criticalities by the largest value in the
	circuit. Otherwise, path criticalities would be unbounded. */

	for (inet = 0; inet < num_timing_nets(); inet++) {
		num_edges = num_timing_net_sinks(inet);
		for (iedge = 0; iedge < num_edges; iedge++) {
			max_path_criticality = max(max_path_criticality, slacks->path_criticality[inet][iedge + 1]);
		}
	}

	for (inet = 0; inet < num_timing_nets(); inet++) {
		num_edges = num_timing_net_sinks(inet);
		for (iedge = 0; iedge < num_edges; iedge++) {
			slacks->path_criticality[inet][iedge + 1] /= max_path_criticality;
		}
	}

#endif

    if (timing_inf.slack_definition == std::string("S") || timing_inf.slack_definition == std::string("G")) {
        if (!is_final_analysis) {
            if (timing_inf.slack_definition == std::string("S")) {
                /* Find the smallest slack in the design. */
                for (int i = 0; i < timing_ctx.sdc->num_constrained_clocks; i++) {
                    for (int j = 0; j < timing_ctx.sdc->num_constrained_clocks; j++) {
                        smallest_slack_in_design = min(smallest_slack_in_design, f_timing_stats->least_slack[i][j]);
                    }
                }

                /* Increase all slacks by the value of the smallest slack in the design, if it's negative.
                Or clip all negative slacks to 0, based on how we choose to normalize slacks*/
                if (smallest_slack_in_design < 0) {
                    for (size_t inet = 0; inet < num_timing_nets(); inet++) {
                        int num_edges = num_timing_net_sinks(inet);
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
		    for (size_t inet = 0; inet < num_timing_nets(); inet++) {
			    int num_edges = num_timing_net_sinks(inet);
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
    timing_ctx.stats.old_sta_wallclock_time  += double(end - begin) / CLOCKS_PER_SEC;
    timing_ctx.stats.num_old_sta_full_updates  += 1;
}

static float find_least_slack(bool is_prepacked, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping) {
	/* Perform a simplified version of do_timing_analysis_for_constraint
	to compute only the smallest slack in the design.
    USED ONLY WHEN slack_definition == 'I'! */

	float smallest_slack_in_design = HUGE_POSITIVE_FLOAT;

    auto& timing_ctx = g_vpr_ctx.timing();

	for (int source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (int sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			if (timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) { /* i.e. != DO_NOT_ANALYSE */

				/* Reset all arrival and required times. */
				for (int inode = 0; inode < timing_ctx.num_tnodes; inode++) {
					timing_ctx.tnodes[inode].T_arr = HUGE_NEGATIVE_FLOAT;
					timing_ctx.tnodes[inode].T_req = HUGE_POSITIVE_FLOAT;
				}

				/* Set arrival times for each top-level tnode on this source domain. */
				int num_at_level = timing_ctx.tnodes_at_level[0].size();
				for (int i = 0; i < num_at_level; i++) {
					int inode = timing_ctx.tnodes_at_level[0][i];
					if (timing_ctx.tnodes[inode].clock_domain == source_clock_domain) {
						if (timing_ctx.tnodes[inode].type == TN_FF_SOURCE) {
							timing_ctx.tnodes[inode].T_arr = timing_ctx.tnodes[inode].clock_delay;
						} else if (timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE) {
							timing_ctx.tnodes[inode].T_arr = 0.;
						}
					}
				}

				/* Forward traversal, to compute arrival times. */
				for (int ilevel = 0; ilevel < timing_ctx.num_tnode_levels; ilevel++) {
					num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

					for (int i = 0; i < num_at_level; i++) {
						int inode = timing_ctx.tnodes_at_level[ilevel][i];
						if (timing_ctx.tnodes[inode].T_arr < NEGATIVE_EPSILON) {
							continue;
						}

						int num_edges = timing_ctx.tnodes[inode].num_edges;
						t_tedge *tedge = timing_ctx.tnodes[inode].out_edges;

						for (int iedge = 0; iedge < num_edges; iedge++) {
							int to_node = tedge[iedge].to_node;
							if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

							timing_ctx.tnodes[to_node].T_arr = max(timing_ctx.tnodes[to_node].T_arr, timing_ctx.tnodes[inode].T_arr + tedge[iedge].Tdel);
						}
					}
				}

				/* Backward traversal, to compute required times and slacks.  We can skip nodes which are more than
				1 away from a sink, because the path with the least slack has to use a connection between a sink
				and one node away from a sink. */
				for (int ilevel = timing_ctx.num_tnode_levels - 1; ilevel >= 0; ilevel--) {
					num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

					for (int i = 0; i < num_at_level; i++) {
						int inode = timing_ctx.tnodes_at_level[ilevel][i];
						int num_edges = timing_ctx.tnodes[inode].num_edges;
						if (num_edges == 0) { /* sink */
							if (timing_ctx.tnodes[inode].type == TN_FF_CLOCK || timing_ctx.tnodes[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1
								|| timing_ctx.tnodes[inode].clock_domain != sink_clock_domain) {
								continue;
							}

							float constraint;
							if (timing_ctx.sdc->num_cf_constraints > 0) {
								char *source_domain_name = timing_ctx.sdc->constrained_clocks[source_clock_domain].name;
								const char *sink_register_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
								int icf = find_cf_constraint(source_domain_name, sink_register_name);
								if (icf != DO_NOT_ANALYSE) {
									constraint = timing_ctx.sdc->cf_constraints[icf].constraint;
									if (constraint < NEGATIVE_EPSILON) {
										continue;
									}
								} else {
									constraint = timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain];
								}
							} else {
								constraint = timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain];
							}

							timing_ctx.tnodes[inode].T_req = constraint + timing_ctx.tnodes[inode].clock_delay;
						} else {

							if (timing_ctx.tnodes[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) {
								continue;
							}

							bool found = false;
							t_tedge *tedge = timing_ctx.tnodes[inode].out_edges;
							for (int iedge = 0; iedge < num_edges && !found; iedge++) {
								int to_node = tedge[iedge].to_node;
								if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

								if(timing_ctx.tnodes[to_node].T_req < HUGE_POSITIVE_FLOAT) {
									found = true;
								}
							}
							if (!found) {
								continue;
							}

							for (int iedge = 0; iedge < num_edges; iedge++) {
								int to_node = tedge[iedge].to_node;
								if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

								if (timing_ctx.tnodes[to_node].num_edges == 0 &&
										timing_ctx.tnodes[to_node].clock_domain == sink_clock_domain) { // one away from a register on this sink domain
									float Tdel = tedge[iedge].Tdel;
									float T_req = timing_ctx.tnodes[to_node].T_req;
									smallest_slack_in_design = min(smallest_slack_in_design, T_req - Tdel - timing_ctx.tnodes[inode].T_arr);
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
	long * max_critical_output_paths_ptr, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping, const t_timing_inf &timing_inf) {

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

    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	t_tedge * tedge;
	int num_dangling_nodes;
	bool found;
	long max_critical_input_paths = 0, max_critical_output_paths = 0;

    /* If not passed in, alloc and load pin_id_to_pb_mapping (and make sure to free later). */
    bool must_free_mapping = false;
    if (pin_id_to_pb_mapping.size() == 0) {
        pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();
        must_free_mapping = true;
    }

	/* Reset all values which need to be reset once per
	traversal pair, rather than once per timing analysis. */

	/* Reset all arrival and required times. */
	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		timing_ctx.tnodes[inode].T_arr = HUGE_NEGATIVE_FLOAT;
		timing_ctx.tnodes[inode].T_req = HUGE_POSITIVE_FLOAT;
	}

#ifndef PATH_COUNTING
	/* Reset num_critical_output_paths. */
	if (is_prepacked) {
		for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
			timing_ctx.tnodes[inode].prepacked_data->num_critical_output_paths = 0;
		}
	}
#endif

	/* Set arrival times for each top-level tnode on this source domain. */
	num_at_level = timing_ctx.tnodes_at_level[0].size();
	for (i = 0; i < num_at_level; i++) {
		inode = timing_ctx.tnodes_at_level[0][i];

		if (timing_ctx.tnodes[inode].clock_domain == source_clock_domain) {

			if (timing_ctx.tnodes[inode].type == TN_FF_SOURCE) {
				/* Set the arrival time of this flip-flop tnode to its clock skew. */
				timing_ctx.tnodes[inode].T_arr = timing_ctx.tnodes[inode].clock_delay;

			} else if (timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE) {
				/* There's no such thing as clock skew for external clocks, and
				input delay is already marked on the edge coming out from this node.
				As a result, the signal can be said to arrive at t = 0. */
				timing_ctx.tnodes[inode].T_arr = 0.;
			}

		}
	}

	/* Compute arrival times with a forward topological traversal from sources
	(TN_FF_SOURCE, TN_INPAD_SOURCE, TN_CONSTANT_GEN_SOURCE) to sinks (TN_FF_SINK, TN_OUTPAD_SINK). */

	total = 0;												/* We count up all tnodes to error-check at the end. */
	for (ilevel = 0; ilevel < timing_ctx.num_tnode_levels; ilevel++) {	/* For each level of our levelized timing graph... */
		num_at_level = timing_ctx.tnodes_at_level[ilevel].size();		/* ...there are num_at_level tnodes at that level. */
		total += num_at_level;

		for (i = 0; i < num_at_level; i++) {
			inode = timing_ctx.tnodes_at_level[ilevel][i];		/* Go through each of the tnodes at the level we're on. */
			if (timing_ctx.tnodes[inode].T_arr < NEGATIVE_EPSILON) {	/* If the arrival time is less than 0 (i.e. HUGE_NEGATIVE_FLOAT)... */
				continue;									/* End this iteration of the num_at_level for loop since
															this node is not part of the clock domain we're analyzing.
															(If it were, it would have received an arrival time already.) */
			}

			num_edges = timing_ctx.tnodes[inode].num_edges;				/* Get the number of edges fanning out from the node we're visiting */
			tedge = timing_ctx.tnodes[inode].out_edges;					/* Get the list of edges from the node we're visiting */
#ifndef PATH_COUNTING
			if (is_prepacked && ilevel == 0) {
				timing_ctx.tnodes[inode].prepacked_data->num_critical_input_paths = 1;	/* Top-level tnodes have one locally-critical input path. */
			}

			/* Using a somewhat convoluted procedure inherited from T-VPack,
			count how many locally-critical input paths fan into each tnode,
			and also find the maximum number over all tnodes. */
			if (is_prepacked) {
				for (iedge = 0; iedge < num_edges; iedge++) {
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

					if (fabs(timing_ctx.tnodes[to_node].T_arr - (timing_ctx.tnodes[inode].T_arr + tedge[iedge].Tdel)) < EPSILON) {
						/* If the "local forward slack" (T_arr(to_node) - T_arr(inode) - T_del) for this edge
						is 0 (i.e. the path from inode to to_node is locally as critical as any other path to
						to_node), add to_node's num critical input paths to inode's number. */
						timing_ctx.tnodes[to_node].prepacked_data->num_critical_input_paths += timing_ctx.tnodes[inode].prepacked_data->num_critical_input_paths;
					} else if (timing_ctx.tnodes[to_node].T_arr < (timing_ctx.tnodes[inode].T_arr + tedge[iedge].Tdel)) {
						/* If the "local forward slack" for this edge is negative,
						reset to_node's num critical input paths to inode's number. */
						timing_ctx.tnodes[to_node].prepacked_data->num_critical_input_paths = timing_ctx.tnodes[inode].prepacked_data->num_critical_input_paths;
					}
					/* Set max_critical_input_paths to the maximum number of critical
					input paths for all tnodes analysed on this traversal. */
					if (timing_ctx.tnodes[to_node].prepacked_data->num_critical_input_paths	> max_critical_input_paths) {
						max_critical_input_paths = timing_ctx.tnodes[to_node].prepacked_data->num_critical_input_paths;
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
                timing_ctx.tnodes[to_node].T_arr = max(timing_ctx.tnodes[to_node].T_arr, timing_ctx.tnodes[inode].T_arr + tedge[iedge].Tdel);

                if (timing_inf.slack_definition == std::string("R") || timing_inf.slack_definition == std::string("G")) {
                    /* Since we updated the destination node (to_node), change the max arrival
                    time for the forward traversal if to_node's arrival time is greater than
                    the existing maximum, and it is on the sink clock domain. */
                    if (timing_ctx.tnodes[to_node].num_edges == 0 && timing_ctx.tnodes[to_node].clock_domain == sink_clock_domain) {
                        max_Tarr = max(max_Tarr, timing_ctx.tnodes[to_node].T_arr);
                    }
                }
			}
		}
	}

    auto& atom_ctx = g_vpr_ctx.atom();

	VTR_ASSERT(total == timing_ctx.num_tnodes);
	num_dangling_nodes = 0;
	/* Compute required times with a backward topological traversal from sinks to sources. */
	for (ilevel = timing_ctx.num_tnode_levels - 1; ilevel >= 0; ilevel--) {
		num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

		for (i = 0; i < num_at_level; i++) {
			inode = timing_ctx.tnodes_at_level[ilevel][i];
			num_edges = timing_ctx.tnodes[inode].num_edges;

			if (ilevel == 0) {
				if (!(timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_FF_SOURCE || timing_ctx.tnodes[inode].type == TN_CONSTANT_GEN_SOURCE ||
                      timing_ctx.tnodes[inode].type == TN_CLOCK_SOURCE) && !timing_ctx.tnodes[inode].is_comb_loop_breakpoint) {
					//We suppress node type errors if they have the is_comb_loop_breakpoint flag set.
					//The flag denotes that an input edge to this node was disconnected to break a combinational
					//loop, and hence we don't consider this an error.
                    AtomPinId pin_id = atom_ctx.lookup.classic_tnode_atom_pin(inode);
                    if(pin_id) {
                        AtomPortId port_id = atom_ctx.nlist.pin_port(pin_id);
                        AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);
                        vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__,
                                "Timing graph discovered unexpected edge to node %d  atom block '%s' port '%s' port_bit '%zu'.",
                                inode,
                                atom_ctx.nlist.block_name(blk_id).c_str(),
                                atom_ctx.nlist.port_name(port_id).c_str(),
                                atom_ctx.nlist.pin_port_bit(pin_id));
                    } else if(timing_ctx.tnodes[inode].pb_graph_pin) {
                        vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__,
                                "Timing graph started on unexpected node %d %s.%s[%d].",
                                inode,
                                timing_ctx.tnodes[inode].pb_graph_pin->parent_node->pb_type->name,
                                timing_ctx.tnodes[inode].pb_graph_pin->port->name,
                                timing_ctx.tnodes[inode].pb_graph_pin->pin_number);
                    } else {
                        vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__,
                                "Timing graph started on unexpected node %d.",
                                inode);
                    }
				}
			} else {
				if ((timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_FF_SOURCE || timing_ctx.tnodes[inode].type == TN_CONSTANT_GEN_SOURCE)) {
                    AtomPinId pin_id = atom_ctx.lookup.classic_tnode_atom_pin(inode);
                    if(pin_id) {
                        AtomPortId port_id = atom_ctx.nlist.pin_port(pin_id);
                        AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);
                        vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__,
                                "Timing graph discovered unexpected edge to node %d  atom block '%s' port '%s' port_bit '%zu'.",
                                inode,
                                atom_ctx.nlist.block_name(blk_id).c_str(),
                                atom_ctx.nlist.port_name(port_id).c_str(),
                                atom_ctx.nlist.pin_port_bit(pin_id));
                    } else if(timing_ctx.tnodes[inode].pb_graph_pin) {
                        vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__,
                                "Timing graph discovered unexpected edge to node %d %s.%s[%d].",
                                inode,
                                timing_ctx.tnodes[inode].pb_graph_pin->parent_node->pb_type->name,
                                timing_ctx.tnodes[inode].pb_graph_pin->port->name,
                                timing_ctx.tnodes[inode].pb_graph_pin->pin_number);
                    } else {
                        vpr_throw(VPR_ERROR_TIMING,__FILE__, __LINE__,
                                "Timing graph discovered unexpected edge to node %d.",
                                inode);
                    }
				}
			}

			/* Unlike the forward traversal, the sinks are all on different levels, so we always have to
			check whether a node is a sink. We give every sink on the sink clock domain we're considering
			a valid required time. Every non-sink node in the fanin of one of these sinks and the fanout of
			some source from the forward traversal also gets a valid required time. */

			if (num_edges == 0) { /* sink */

				if (timing_ctx.tnodes[inode].type == TN_FF_CLOCK || timing_ctx.tnodes[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) {
					continue; /* Skip nodes on the clock net itself, and nodes with unset arrival times. */
				}

				if (!(timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK || timing_ctx.tnodes[inode].type == TN_FF_SINK)) {
					if(is_prepacked) {
                        AtomPinId pin_id = atom_ctx.lookup.classic_tnode_atom_pin(inode);
                        VTR_ASSERT(pin_id);
                        AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);
						VTR_LOG_WARN(
								"Pin on block %s.%s[%d] not used\n",
                                atom_ctx.nlist.block_name(blk_id).c_str(),
								timing_ctx.tnodes[inode].prepacked_data->model_port_ptr->name,
								timing_ctx.tnodes[inode].prepacked_data->model_pin);
					}
					num_dangling_nodes++;
					/* Note: Still need to do standard traversals with dangling pins so that algorithm runs properly, but T_arr and T_Req to values such that it dangling nodes do not affect actual timing values */
				}

				/* Skip nodes not on the sink clock domain of the
				constraint we're currently considering */
				if (timing_ctx.tnodes[inode].clock_domain != sink_clock_domain) {
					continue;
				}

				/* See if there's an override constraint between the source clock domain (name is
				timing_ctx.sdc->constrained_clocks[source_clock_domain].name) and the flip-flop or outpad we're at
				now (name is find_tnode_net_name(inode, is_prepacked)). We test if
				timing_ctx.sdc->num_cf_constraints > 0 first so that we can save time looking up names in the vast
				majority of cases where there are no such constraints. */

				if (timing_ctx.sdc->num_cf_constraints > 0) {
					char *source_domain_name = timing_ctx.sdc->constrained_clocks[source_clock_domain].name;
					const char *sink_register_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
					int icf = find_cf_constraint(source_domain_name, sink_register_name);
					if (icf != DO_NOT_ANALYSE) {
						constraint = timing_ctx.sdc->cf_constraints[icf].constraint;
						if (constraint < NEGATIVE_EPSILON) {
							/* Constraint is DO_NOT_ANALYSE (-1) for this particular sink. */
							continue;
						}
					} else {
						/* Use the default constraint from timing_ctx.sdc->domain_constraint. */
						constraint = timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain];
						/* Constraint is guaranteed to be valid since we checked for it at the very beginning. */
					}
				} else {
					constraint = timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain];
				}

				/* Now we know we should analyse this tnode. */

                if (timing_inf.slack_definition == std::string("R") || timing_inf.slack_definition == std::string("G")) {
				    /* Assign the required time T_req for this leaf node, taking into account clock skew. T_req is the
				    time all inputs to a tnode must arrive by before it would degrade this constraint's critical path delay.

				    Relax the required time at the sink node to be non-negative by taking the max of the "real" required
				    time (constraint + timing_ctx.tnodes[inode].clock_delay) and the max arrival time in this domain (max_Tarr), except
				    for the final analysis where we report actual slack. We do this to prevent any slacks from being
				    negative, since negative slacks are not used effectively by the optimizers.

				    E.g. if we have a 10 ns constraint and it takes 14 ns to get here, we'll have a slack of at most -4 ns
				    for any edge along the path that got us here.  If we say the required time is 14 ns (no less than the
				    arrival time), we don't have a negative slack anymore.  However, in the final timing analysis, the real
				    slacks are computed (that's what human designers care about), not the relaxed ones. */

				    if (is_final_analysis) {
					    timing_ctx.tnodes[inode].T_req = constraint + timing_ctx.tnodes[inode].clock_delay;
				    } else {
					    timing_ctx.tnodes[inode].T_req = max(constraint + timing_ctx.tnodes[inode].clock_delay, max_Tarr);
				    }
                } else {
					/* Don't do the relaxation and always set T_req equal to the "real" required time. */
				    timing_ctx.tnodes[inode].T_req = constraint + timing_ctx.tnodes[inode].clock_delay;
                }

				max_Treq = max(max_Treq, timing_ctx.tnodes[inode].T_req);

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
					   (timing_ctx.tnodes[inode].T_arr - timing_ctx.tnodes[inode].clock_delay));

#ifndef PATH_COUNTING
				if (is_prepacked) {
					timing_ctx.tnodes[inode].prepacked_data->num_critical_output_paths = 1; /* Bottom-level tnodes have one locally-critical input path. */
				}
#endif
			} else { /* not a sink */

				VTR_ASSERT(!(timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK || timing_ctx.tnodes[inode].type == TN_FF_SINK || timing_ctx.tnodes[inode].type == TN_FF_CLOCK));

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
				if (timing_ctx.tnodes[inode].T_arr < HUGE_NEGATIVE_FLOAT + 1) {
					continue; /* Skip nodes with unset arrival times. */
				}

				/* Case 2 */
				found = false;
				tedge = timing_ctx.tnodes[inode].out_edges;
				for (iedge = 0; iedge < num_edges && !found; iedge++) {
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

					if (timing_ctx.tnodes[to_node].T_req < HUGE_POSITIVE_FLOAT) {
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
					T_req = timing_ctx.tnodes[to_node].T_req;
					timing_ctx.tnodes[inode].T_req = min(timing_ctx.tnodes[inode].T_req, T_req - Tdel);

					/* Update least slack per constraint. This is NOT the same as the minimum slack we will
					calculate on this traversal for post-packed netlists, which only count inter-cluster
					slacks. We only look at edges adjacent to sink nodes on the sink clock domain since
					all paths go through one of these edges. */
					if (timing_ctx.tnodes[to_node].num_edges == 0 && timing_ctx.tnodes[to_node].clock_domain == sink_clock_domain) {
					    f_timing_stats->least_slack[source_clock_domain][sink_clock_domain] =
					        min(f_timing_stats->least_slack[source_clock_domain][sink_clock_domain],
					           (T_req - Tdel - timing_ctx.tnodes[inode].T_arr));
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
						if (fabs(timing_ctx.tnodes[to_node].T_req - (timing_ctx.tnodes[inode].T_req + tedge[iedge].Tdel)) < EPSILON) {
						    timing_ctx.tnodes[inode].prepacked_data->num_critical_output_paths += timing_ctx.tnodes[to_node].prepacked_data->num_critical_output_paths;
						}
						/* Set max_critical_output_paths to the maximum number of critical
						output paths for all tnodes analysed on this traversal. */
						if (timing_ctx.tnodes[to_node].prepacked_data->num_critical_output_paths > max_critical_output_paths) {
						    max_critical_output_paths = timing_ctx.tnodes[to_node].prepacked_data->num_critical_output_paths;
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
		VTR_LOG_WARN(
				"%d unused pins \n",  num_dangling_nodes);
	}

	/* The criticality denominator is the maximum required time, except for slack_definition == 'N'
	where it is the max of all arrival and required times.

	For definitions except 'R' and 'N', this works out to the maximum of constraint +
	timing_ctx.tnodes[inode].clock_delay over all nodes.  For 'R' and 'N', this works out to the maximum of
	that value and max_Tarr.  The max_Tarr is implicitly incorporated into the denominator through
	its inclusion in the required time, but we have to explicitly include it for 'N'. */
    if (timing_inf.slack_definition == std::string("N")) {
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
	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		timing_ctx.tnodes[inode].forward_weight = 0;
		timing_ctx.tnodes[inode].backward_weight = 0;
	}

	/* Set foward weights for each top-level tnode. */
	num_at_level = timing_ctx.tnodes_at_level[0].size();
	for (i = 0; i < num_at_level; i++) {
		inode = timing_ctx.tnodes_at_level[0][i];
		timing_ctx.tnodes[inode].forward_weight = 1.;
	}

	/* Do a forward topological traversal to populate forward weights. */
	for (ilevel = 0; ilevel < timing_ctx.num_tnode_levels; ilevel++) {
		num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

		for (i = 0; i < num_at_level; i++) {
			inode = timing_ctx.tnodes_at_level[ilevel][i];
			if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
				continue;
			}
			tedge = timing_ctx.tnodes[inode].out_edges;
			num_edges = timing_ctx.tnodes[inode].num_edges;
			for (iedge = 0; iedge < num_edges; iedge++) {
				to_node = tedge[iedge].to_node;
				if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

				if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
				    continue;
				}
				forward_local_slack = timing_ctx.tnodes[to_node].T_arr - timing_ctx.tnodes[inode].T_arr - tedge[iedge].Tdel;
				discount = pow((float) DISCOUNT_FUNCTION_BASE, -1 * forward_local_slack / criticality_denom);
				timing_ctx.tnodes[to_node].forward_weight += discount * timing_ctx.tnodes[inode].forward_weight;
			}
		}
	}

	/* Do a backward topological traversal to populate backward weights.
	Since the sinks are all on different levels, we have to check for them as we go. */
	for (ilevel = timing_ctx.num_tnode_levels - 1; ilevel >= 0; ilevel--) {
		num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

		for (i = 0; i < num_at_level; i++) {
			inode = timing_ctx.tnodes_at_level[ilevel][i];
			if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
				continue;
			}
			num_edges = timing_ctx.tnodes[inode].num_edges;
			if (num_edges == 0) { /* sink */
				timing_ctx.tnodes[inode].backward_weight = 1.;
			} else {
				tedge = timing_ctx.tnodes[inode].out_edges;
				for (iedge = 0; iedge < num_edges; iedge++) {
					to_node = tedge[iedge].to_node;
					if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes
					if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
					    continue;
					}
					backward_local_slack = timing_ctx.tnodes[to_node].T_req - timing_ctx.tnodes[inode].T_req - tedge[iedge].Tdel;
					discount = pow((float) DISCOUNT_FUNCTION_BASE, -1 * backward_local_slack / criticality_denom);
					timing_ctx.tnodes[inode].backward_weight += discount * timing_ctx.tnodes[to_node].backward_weight;
				}
			}
		}
	}
}
#endif

static void update_slacks(t_slack * slacks, float criticality_denom,
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

	int iedge, inode, to_node, num_edges;
	t_tedge *tedge;
	float T_arr, Tdel, T_req, slack;
	float timing_criticality;

    auto& timing_ctx = g_vpr_ctx.timing();

	for (size_t inet = 0; inet < num_timing_nets(); inet++) {
		inode = f_net_to_driver_tnode[inet];
		T_arr = timing_ctx.tnodes[inode].T_arr;

		if (!(has_valid_T_arr(inode) && has_valid_T_req(inode))) {
			continue; /* Only update this net on this traversal if its
					  driver node has been updated on this traversal. */
		}

		num_edges = timing_ctx.tnodes[inode].num_edges;
		tedge = timing_ctx.tnodes[inode].out_edges;

        VTR_ASSERT_MSG(static_cast<size_t>(num_edges) == num_timing_net_sinks(inet), "Number of tnode edges and net sinks do not match");

		for (iedge = 0; iedge < num_edges; iedge++) {
			to_node = tedge[iedge].to_node;
			if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

			if (!(has_valid_T_arr(to_node) && has_valid_T_req(to_node))) {
				continue; /* Only update this edge on this traversal if this
						  particular sink node has been updated on this traversal. */
			}
			Tdel = tedge[iedge].Tdel;
			T_req = timing_ctx.tnodes[to_node].T_req;

			slack = T_req - T_arr - Tdel;

			if (!is_final_analysis) {
                if (timing_inf.slack_definition == std::string("I")) {
                    /* Shift slack UPWARDS by subtracting the smallest slack in the
                    design (which is negative or zero). */
                    slack -= smallest_slack_in_design;
                } else if (timing_inf.slack_definition == std::string("C")) {
				    /* Clip all negative slacks to 0. */
				    if (slack < 0) slack = 0;
                }
			}

			if (update_slack) {
				/* Update the slack for this edge if slk is the new minimum value */
				slacks->slack[inet][iedge + 1] = min(slack, slacks->slack[inet][iedge + 1]);
			}

            if (timing_inf.slack_definition != std::string("S") && timing_inf.slack_definition != std::string("G")) {
                if (!is_final_analysis) { // Criticality is not meaningful using non-normalized slacks.

                    /* Since criticality_denom is not the same on each traversal,
                    we have to update criticality separately. */

                    if (timing_inf.slack_definition == std::string("C")) {
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
                        (timing_ctx.tnodes[inode].forward_weight + timing_ctx.tnodes[to_node].backward_weight) *
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'P' /* Use product of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1],
                        timing_ctx.tnodes[inode].forward_weight * timing_ctx.tnodes[to_node].backward_weight *
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'L' /* Use natural log of product of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1],
                        log(timing_ctx.tnodes[inode].forward_weight * timing_ctx.tnodes[to_node].backward_weight) *
                        pow((float) FINAL_DISCOUNT_FUNCTION_BASE, timing_criticality - 1));
#elif PATH_COUNTING == 'R' /* Use product of natural logs of forward and backward weights. */
                    slacks->path_criticality[inet][iedge + 1] = max(slacks->path_criticality[inet][iedge + 1],
                        log(timing_ctx.tnodes[inode].forward_weight) * log(timing_ctx.tnodes[to_node].backward_weight) *
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
	int nets_on_crit_path, tnodes_on_crit_path, inode;
	e_tnode_type type;
	float total_net_delay, total_logic_delay, Tdel;

    auto& timing_ctx = g_vpr_ctx.timing();

	vtr::vector_map<ClusterBlockId, t_pb **> pin_id_to_pb_mapping = alloc_and_load_pin_id_to_pb_mapping();

	critical_path_head = allocate_and_load_critical_path(timing_inf);
	critical_path_node = critical_path_head;

	fp = vtr::fopen(fname, "w");

	nets_on_crit_path = 0;
	tnodes_on_crit_path = 0;
	total_net_delay = 0.;
	total_logic_delay = 0.;

	while (critical_path_node != nullptr) {
		Tdel = print_critical_path_node(fp, critical_path_node, pin_id_to_pb_mapping);
		inode = critical_path_node->data;
		type = timing_ctx.tnodes[inode].type;
		tnodes_on_crit_path++;

		if (type == TN_CB_OPIN) {
            nets_on_crit_path++;

			total_net_delay += Tdel;
		} else {
			total_logic_delay += Tdel;
		}

		critical_path_node = critical_path_node->next;
	}

	fprintf(fp, "\nTnodes on critical path: %d  Nets on critical path: %d."
			"\n", tnodes_on_crit_path, nets_on_crit_path);
	fprintf(fp, "Total logic delay: %g (s)  Total net delay: %g (s)\n",
			total_logic_delay, total_net_delay);

	VTR_LOG("Nets on critical path: %d normal.\n",
			nets_on_crit_path);

	VTR_LOG("Total logic delay: %g (s), total net delay: %g (s)\n",
			total_logic_delay, total_net_delay);

	/* Make sure total_logic_delay and total_net_delay add
	up to critical path delay,within 5 decimal places. */
	VTR_ASSERT(std::isnan(get_critical_path_delay()) || total_logic_delay + total_net_delay - get_critical_path_delay()/1e9 < 1e-5);

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
	vtr::vector_map<ClusterBlockId, t_pb **> empty_pin_id_to_pb_mapping; //Empty vector_map for do_timing_analysis_for_constraint

    auto& timing_ctx = g_vpr_ctx.timing();

	/* If there's only one clock, we can use the arrival and required times
	currently on the timing graph to find the critical path. If there are multiple
	clocks, however, the values currently on the timing graph correspond to the
	last constraint (pair of clock domains) analysed, which may not be the constraint
	with the critical path.	In this case, we have to find the constraint with the
	least slack and redo the timing analysis for this constraint so we get the right
	values onto the timing graph. */

	if (timing_ctx.sdc->num_constrained_clocks > 1) {
		/* The critical path belongs to the source and sink clock domains
		with the least slack. Find these clock domains now. */

		for (i = 0; i < timing_ctx.sdc->num_constrained_clocks; i++) {
			for (j = 0; j < timing_ctx.sdc->num_constrained_clocks; j++) {
				// Use the true, unrelaxed, least slack (constraint - critical path delay).
				slack = timing_ctx.sdc->domain_constraint[i][j] - f_timing_stats->cpd[i][j];
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
		do_timing_analysis_for_constraint(source_clock_domain, sink_clock_domain, false, false, nullptr, nullptr, empty_pin_id_to_pb_mapping, timing_inf);
	}

	/* Start at the source (level-0) tnode with the least slack (T_req-T_arr).
	This will form the head of our linked list of tnodes on the critical path. */
	min_slack = HUGE_POSITIVE_FLOAT;
	num_at_level_zero = timing_ctx.tnodes_at_level[0].size();
	for (i = 0; i < num_at_level_zero; i++) {
		inode = timing_ctx.tnodes_at_level[0][i];
		if (has_valid_T_arr(inode) && has_valid_T_req(inode)) { /* Valid arrival and required times */
			slack = timing_ctx.tnodes[inode].T_req - timing_ctx.tnodes[inode].T_arr;
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
	num_edges = timing_ctx.tnodes[crit_node].num_edges;

	/* Keep adding the tnode in this tnode's fanout which has the least slack
	to our critical path linked list, then jump to that tnode and repeat, until
	we hit a tnode with no edges, which is the sink of the critical path. */
	while (num_edges != 0) {
		curr_crit_node = (vtr::t_linked_int *) vtr::malloc(sizeof(vtr::t_linked_int));
		prev_crit_node->next = curr_crit_node;
		tedge = timing_ctx.tnodes[crit_node].out_edges;
		min_slack = HUGE_POSITIVE_FLOAT;

		for (iedge = 0; iedge < num_edges; iedge++) {
			to_node = tedge[iedge].to_node;
			if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

			if (has_valid_T_arr(to_node) && has_valid_T_req(to_node)) { /* Valid arrival and required times */
				slack = timing_ctx.tnodes[to_node].T_req - timing_ctx.tnodes[to_node].T_arr;
				if (slack < min_slack) {
					crit_node = to_node;
					min_slack = slack;
				}
			}
		}

		curr_crit_node->data = crit_node;
		prev_crit_node = curr_crit_node;
		num_edges = timing_ctx.tnodes[crit_node].num_edges;
	}

	prev_crit_node->next = nullptr;
	return (critical_path_head);
}

#ifndef PATH_COUNTING
static void update_normalized_costs(float criticality_denom, long max_critical_input_paths,
    long max_critical_output_paths, const t_timing_inf &timing_inf) {
    int inode;

    /* Update the normalized costs for the clusterer.  On each traversal, each cost is
    updated for tnodes analysed on this traversal if it would give this tnode a higher
    criticality when calculating block criticality for the clusterer. */

    if (timing_inf.slack_definition == std::string("R") || timing_inf.slack_definition == std::string("I")) {
        /*VTR_ASSERT(criticality_denom != 0); */
        /* Possible if timing analysis is being run pre-packing
                                        with all delays set to 0. This is not currently done,
                                        but if you're going to do it, you need to decide how
                                        best to normalize these values to suit your purposes. */
    }

    auto& timing_ctx = g_vpr_ctx.mutable_timing();

	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		/* Only calculate for tnodes which have valid arrival and required times. */
		if (has_valid_T_arr(inode) && has_valid_T_req(inode)) {
			timing_ctx.tnodes[inode].prepacked_data->normalized_slack = min(timing_ctx.tnodes[inode].prepacked_data->normalized_slack,
				(timing_ctx.tnodes[inode].T_req - timing_ctx.tnodes[inode].T_arr)/criticality_denom);
			timing_ctx.tnodes[inode].prepacked_data->normalized_T_arr = max(timing_ctx.tnodes[inode].prepacked_data->normalized_T_arr,
				timing_ctx.tnodes[inode].T_arr/criticality_denom);
			timing_ctx.tnodes[inode].prepacked_data->normalized_total_critical_paths = max(timing_ctx.tnodes[inode].prepacked_data->normalized_total_critical_paths,
					((float) timing_ctx.tnodes[inode].prepacked_data->num_critical_input_paths + timing_ctx.tnodes[inode].prepacked_data->num_critical_output_paths) /
							 ((float) max_critical_input_paths + max_critical_output_paths));
		}
	}
}
#endif


static void load_clock_domain_and_clock_and_io_delay(bool is_prepacked, vtr::vector_map<ClusterBlockId, std::vector<int>> &lookup_tnode_from_pin_id, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping) {
/* Loads clock domain and clock delay onto TN_FF_SOURCE and TN_FF_SINK tnodes.
The clock domain of each clock is its index in timing_ctx.sdc->constrained_clocks.
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
	const char * net_name;
	t_tnode * clock_node;

    auto& timing_ctx = g_vpr_ctx.timing();

	/* Wipe fanout of each clock domain in timing_ctx.sdc->constrained_clocks. */
	for (iclock = 0; iclock < timing_ctx.sdc->num_constrained_clocks; iclock++) {
		timing_ctx.sdc->constrained_clocks[iclock].fanout = 0;
	}

	/* First, visit all TN_INPAD_SOURCE and TN_CLOCK_SOURCE tnodes
     * We only need to check the first level of the timing graph, since all SOURCEs should appear there*/
    if(!timing_ctx.tnodes_at_level.empty()) {
        num_at_level = timing_ctx.tnodes_at_level[0].size(); /* There are num_at_level top-level tnodes. */
    } else {
        num_at_level = 0;
    }
    for(i = 0; i < num_at_level; i++) {
        inode = timing_ctx.tnodes_at_level[0][i];	/* Iterate through each tnode. inode is the index of the tnode in the array tnode. */
		if (timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_CLOCK_SOURCE) {	/* See if this node is the start of an I/O pad, or a clock source. */
			net_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
			if ((clock_index = find_clock(net_name)) != -1) { /* We have a clock inpad or clock source. */
				/* Set clock skew to 0 at the source and propagate skew
				recursively to all connected nodes, adding delay as we go.
				Set the clock domain to the index of the clock in the
				timing_ctx.sdc->constrained_clocks array and propagate unchanged. */
				timing_ctx.tnodes[inode].clock_delay = 0.;
				timing_ctx.tnodes[inode].clock_domain = clock_index;
				propagate_clock_domain_and_skew(inode);

				/* Set the clock domain of this clock inpad to -1, so that we do not analyse it.
				If we did not do this, the clock net would be analysed on the same iteration of
				the timing analyzer as the flip-flops it drives! */
				timing_ctx.tnodes[inode].clock_domain = DO_NOT_ANALYSE;

			} else if (timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE && (input_index = find_input(net_name)) != -1) {
				/* We have a constrained non-clock inpad - find the clock it's constrained on. */
				clock_index = find_clock(timing_ctx.sdc->constrained_inputs[input_index].clock_name);
				VTR_ASSERT(clock_index != -1);
				/* The clock domain for this input is that of its virtual clock */
				timing_ctx.tnodes[inode].clock_domain = clock_index;
				/* Increment the fanout of this virtual clock domain. */
				timing_ctx.sdc->constrained_clocks[clock_index].fanout++;
				/* Mark input delay specified in SDC file on the timing graph edge leading out from the TN_INPAD_SOURCE node. */
				timing_ctx.tnodes[inode].out_edges[0].Tdel = timing_ctx.sdc->constrained_inputs[input_index].delay * 1e-9; /* normalize to be in seconds not ns */
			} else { /* We have an unconstrained input - mark with dummy clock domain and do not analyze. */
				timing_ctx.tnodes[inode].clock_domain = DO_NOT_ANALYSE;
			}
		}
	}

    /* Second, visit all TN_OUTPAD_SINK tnodes. Unlike for TN_INPAD_SOURCE tnodes,
	we have to search the entire tnode array since these are all on different levels. */
	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		if (timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK) {
			/*  Since the pb_graph_pin of TN_OUTPAD_SINK tnodes points to NULL, we have to find the net
			from the pb_graph_pin of the corresponding TN_OUTPAD_IPIN node.
			Exploit the fact that the TN_OUTPAD_IPIN node will always be one prior in the tnode array. */
			VTR_ASSERT(timing_ctx.tnodes[inode - 1].type == TN_OUTPAD_IPIN);
			net_name = find_tnode_net_name(inode, is_prepacked, pin_id_to_pb_mapping);
			output_index = find_output(net_name + 4); /* the + 4 removes the prefix "out:" automatically prepended to outputs */
			if (output_index != -1) {
				/* We have a constrained outpad, find the clock it's constrained on. */
				clock_index = find_clock(timing_ctx.sdc->constrained_outputs[output_index].clock_name);
				VTR_ASSERT(clock_index != -1);
				/* The clock doain for this output is that of its virtual clock */
				timing_ctx.tnodes[inode].clock_domain = clock_index;
				/* Increment the fanout of this virtual clock domain. */
				timing_ctx.sdc->constrained_clocks[clock_index].fanout++;
				/* Mark output delay specified in SDC file on the timing graph edge leading into the TN_OUTPAD_SINK node.
				However, this edge is part of the corresponding TN_OUTPAD_IPIN node.
				Exploit the fact that the TN_OUTPAD_IPIN node will always be one prior in the tnode array. */
				timing_ctx.tnodes[inode - 1].out_edges[0].Tdel = timing_ctx.sdc->constrained_outputs[output_index].delay * 1e-9; /* normalize to be in seconds not ns */

			} else { /* We have an unconstrained input - mark with dummy clock domain and do not analyze. */
				timing_ctx.tnodes[inode].clock_domain = -1;
			}
		}
	}

	/* Third, visit all TN_FF_SOURCE and TN_FF_SINK tnodes, and transfer the clock domain and skew from their corresponding TN_FF_CLOCK tnodes*/
	for (inode = 0; inode < timing_ctx.num_tnodes; inode++) {
		if (timing_ctx.tnodes[inode].type == TN_FF_SOURCE || timing_ctx.tnodes[inode].type == TN_FF_SINK) {
			clock_node = find_ff_clock_tnode(inode, is_prepacked, lookup_tnode_from_pin_id);
			timing_ctx.tnodes[inode].clock_domain = clock_node->clock_domain;
			timing_ctx.tnodes[inode].clock_delay   = clock_node->clock_delay;
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

    auto& timing_ctx = g_vpr_ctx.timing();

	tedge = timing_ctx.tnodes[inode].out_edges;	/* Get the list of edges from the node we're visiting. */

	if (!tedge) { /* Leaf/sink node; base case of the recursion. */
		if(timing_ctx.tnodes[inode].type != TN_FF_CLOCK && timing_ctx.tnodes[inode].type != TN_OUTPAD_SINK) {
            VTR_LOG_WARN( "tnode %d appears to take clock as a data input\n", inode);
            return;
        }

		VTR_ASSERT(timing_ctx.tnodes[inode].clock_domain != -1); /* Make sure clock domain is valid. */

		timing_ctx.sdc->constrained_clocks[timing_ctx.tnodes[inode].clock_domain].fanout++;
		return;
	}

	for (iedge = 0; iedge < timing_ctx.tnodes[inode].num_edges; iedge++) {	/* Go through each edge coming out from this tnode */
		to_node = tedge[iedge].to_node;
		if(to_node == DO_NOT_ANALYSE) continue; //Skip marked invalid nodes

		/* Propagate clock skew forward along this clock net, adding the delay of the wires (edges) of the clock network. */
		timing_ctx.tnodes[to_node].clock_delay = timing_ctx.tnodes[inode].clock_delay + tedge[iedge].Tdel;
		/* Propagate clock domain forward unchanged */
		timing_ctx.tnodes[to_node].clock_domain = timing_ctx.tnodes[inode].clock_domain;
		/* Finally, call recursively on the destination tnode. */
		propagate_clock_domain_and_skew(to_node);
	}
}

static const char * find_tnode_net_name(int inode, bool is_prepacked, vtr::vector_map<ClusterBlockId, t_pb **> &pin_id_to_pb_mapping) {
	/* Finds the name of the net which a tnode (inode) is on (different for pre-/post-packed netlists). */

	t_pb_graph_pin * pb_graph_pin;

    auto& timing_ctx = g_vpr_ctx.timing();

	if (is_prepacked) {
        //We only store pin mappings for tnode's which correspond to netlist pins,
        //so we need to convert sources/sinks to their relevant pin tnode's before
        //looking up the netlist pin/net
        auto& atom_ctx = g_vpr_ctx.atom();

        int inode_pin = OPEN;
        if(timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_FF_SOURCE || timing_ctx.tnodes[inode].type == TN_CLOCK_SOURCE) {
            VTR_ASSERT(timing_ctx.tnodes[inode].num_edges == 1);
            inode_pin = timing_ctx.tnodes[inode].out_edges[0].to_node;
        } else if (timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK || timing_ctx.tnodes[inode].type == TN_FF_SINK) {
            inode_pin = inode - 1;
            VTR_ASSERT(timing_ctx.tnodes[inode_pin].out_edges[0].to_node == inode);
        } else {
            inode_pin = inode;
        }
        VTR_ASSERT(inode_pin != OPEN);

        AtomPinId pin_id = atom_ctx.lookup.classic_tnode_atom_pin(inode_pin);
        VTR_ASSERT(pin_id);

        if(timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_INPAD_OPIN ||
           timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK || timing_ctx.tnodes[inode].type == TN_OUTPAD_IPIN) {
            AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);

            //For input/input pads the net name is the same as the block name
            return atom_ctx.nlist.block_name(blk_id).c_str();
        } else {
            AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);

            return atom_ctx.nlist.net_name(net_id).c_str();
        }
	} else {
        auto& cluster_ctx = g_vpr_ctx.clustering();

        ClusterBlockId iblock = timing_ctx.tnodes[inode].block;
        if(timing_ctx.tnodes[inode].type == TN_INPAD_SOURCE || timing_ctx.tnodes[inode].type == TN_INPAD_OPIN ||
           timing_ctx.tnodes[inode].type == TN_OUTPAD_SINK || timing_ctx.tnodes[inode].type == TN_OUTPAD_IPIN) {
            //For input/input pads the net name is the same as the block name
            pb_graph_pin = timing_ctx.tnodes[inode].pb_graph_pin;
			return pin_id_to_pb_mapping[iblock][pb_graph_pin->pin_count_in_cluster]->name;
        } else {
            //We need to find the TN_CB_OPIN/TN_CB_IPIN that this node drives, so that we can look up
            //the net name in the global clb netlist
            int inode_search = inode;
            while(timing_ctx.tnodes[inode_search].type != TN_CB_IPIN && timing_ctx.tnodes[inode_search].type != TN_CB_OPIN ) {
                //Assume we don't have any internal fanout inside the block
                //  Should be true for most source-types
                VTR_ASSERT(timing_ctx.tnodes[inode_search].num_edges == 1);
                inode_search = timing_ctx.tnodes[inode_search].out_edges[0].to_node;
            }
            VTR_ASSERT(timing_ctx.tnodes[inode_search].type == TN_CB_IPIN || timing_ctx.tnodes[inode_search].type == TN_CB_OPIN);

            //Now that we have a complex block pin, we can look-up its connected net in the CLB netlist
            pb_graph_pin = timing_ctx.tnodes[inode_search].pb_graph_pin;
            int ipin = pb_graph_pin->pin_count_in_cluster; //Pin into the CLB

			ClusterNetId net_id = cluster_ctx.clb_nlist.block_net(timing_ctx.tnodes[inode_search].block, ipin); //Net index into the clb netlist
            return cluster_ctx.clb_nlist.net_name(net_id).c_str();
        }
	}
}

static t_tnode * find_ff_clock_tnode(int inode, bool is_prepacked, vtr::vector_map<ClusterBlockId, std::vector<int>> &lookup_tnode_from_pin_id) {
	/* Finds the TN_FF_CLOCK tnode on the same flipflop as an TN_FF_SOURCE or TN_FF_SINK tnode. */

	t_tnode * ff_clock_tnode;
	int ff_tnode;
	t_pb_graph_node * parent_pb_graph_node;
	t_pb_graph_pin * ff_source_or_sink_pb_graph_pin, * clock_pb_graph_pin;

    auto& timing_ctx = g_vpr_ctx.timing();

	if (is_prepacked) {
        //TODO: handle multiple clocks

        int i_pin_tnode = OPEN;
        if(timing_ctx.tnodes[inode].type == TN_FF_SOURCE) {
            VTR_ASSERT(timing_ctx.tnodes[inode].num_edges == 1);

            i_pin_tnode = timing_ctx.tnodes[inode].out_edges[0].to_node;
            VTR_ASSERT(timing_ctx.tnodes[i_pin_tnode].type == TN_FF_OPIN);
        } else if (timing_ctx.tnodes[inode].type == TN_FF_SINK) {

            i_pin_tnode = inode - 1;

            VTR_ASSERT(timing_ctx.tnodes[i_pin_tnode].type == TN_FF_IPIN);
            VTR_ASSERT(timing_ctx.tnodes[i_pin_tnode].num_edges == 1);
            VTR_ASSERT(timing_ctx.tnodes[i_pin_tnode].out_edges[0].to_node == inode);
        } else {
            VTR_ASSERT(false);
        }

        auto& atom_ctx = g_vpr_ctx.atom();

        auto pin_id = atom_ctx.lookup.classic_tnode_atom_pin(i_pin_tnode);
        VTR_ASSERT(pin_id);

        auto blk_id = atom_ctx.nlist.pin_block(pin_id);
        VTR_ASSERT(blk_id);

        //Get the single clock pin
        auto clock_pins = atom_ctx.nlist.block_clock_pins(blk_id);
        VTR_ASSERT(clock_pins.size() == 1);
        auto clock_pin_id = *clock_pins.begin();

        //Get the associated tnode index
        int i_ff_clock = atom_ctx.lookup.atom_pin_classic_tnode(clock_pin_id);
        VTR_ASSERT(i_ff_clock != OPEN);

        ff_clock_tnode = &timing_ctx.tnodes[i_ff_clock];

	} else {
        ClusterBlockId iblock = timing_ctx.tnodes[inode].block;
		ff_source_or_sink_pb_graph_pin = timing_ctx.tnodes[inode].pb_graph_pin;
		parent_pb_graph_node = ff_source_or_sink_pb_graph_pin->parent_node;
		/* Make sure there's only one clock port and only one clock pin in that port */
		VTR_ASSERT(parent_pb_graph_node->num_clock_ports == 1);
		VTR_ASSERT(parent_pb_graph_node->num_clock_pins[0] == 1);
		clock_pb_graph_pin = &parent_pb_graph_node->clock_pins[0][0];
		ff_tnode = lookup_tnode_from_pin_id[iblock][clock_pb_graph_pin->pin_count_in_cluster];
		VTR_ASSERT(ff_tnode != OPEN);
		ff_clock_tnode = &timing_ctx.tnodes[ff_tnode];
	}
	VTR_ASSERT(ff_clock_tnode != nullptr);
	VTR_ASSERT(ff_clock_tnode->type == TN_FF_CLOCK);
	return ff_clock_tnode;
}

static int find_clock(const char * net_name) {
/* Given a string net_name, find whether it's the name of a clock in the array timing_ctx.sdc->constrained_clocks.  *
 * if it is, return the clock's index in timing_ctx.sdc->constrained_clocks; if it's not, return -1. */
    auto& timing_ctx = g_vpr_ctx.timing();

	for (int index = 0; index < timing_ctx.sdc->num_constrained_clocks; index++) {
		if (strcmp(net_name, timing_ctx.sdc->constrained_clocks[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_input(const char * net_name) {
/* Given a string net_name, find whether it's the name of a constrained input in the array timing_ctx.sdc->constrained_inputs.  *
 * if it is, return its index in timing_ctx.sdc->constrained_inputs; if it's not, return -1. */
    auto& timing_ctx = g_vpr_ctx.timing();
	for (int index = 0; index < timing_ctx.sdc->num_constrained_inputs; index++) {
		if (strcmp(net_name, timing_ctx.sdc->constrained_inputs[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_output(const char * net_name) {
/* Given a string net_name, find whether it's the name of a constrained output in the array timing_ctx.sdc->constrained_outputs.  *
 * if it is, return its index in timing_ctx.sdc->constrained_outputs; if it's not, return -1. */
    auto& timing_ctx = g_vpr_ctx.timing();

	for (int index = 0; index < timing_ctx.sdc->num_constrained_outputs; index++) {
		if (strcmp(net_name, timing_ctx.sdc->constrained_outputs[index].name) == 0) {
			return index;
		}
	}
	return -1;
}

static int find_cf_constraint(const char * source_clock_name, const char * sink_ff_name) {
	/* Given a source clock domain and a sink flip-flop, find out if there's an override constraint between them.
	If there is, return the index in timing_ctx.sdc->cf_constraints; if there is not, return -1. */
	int icf, isource, isink;

    auto& timing_ctx = g_vpr_ctx.timing();

	for (icf = 0; icf < timing_ctx.sdc->num_cf_constraints; icf++) {
		for (isource = 0; isource < timing_ctx.sdc->cf_constraints[icf].num_source; isource++) {
			if (strcmp(timing_ctx.sdc->cf_constraints[icf].source_list[isource], source_clock_name) == 0) {
				for (isink = 0; isink < timing_ctx.sdc->cf_constraints[icf].num_sink; isink++) {
					if (strcmp(timing_ctx.sdc->cf_constraints[icf].sink_list[isink], sink_ff_name) == 0) {
						return icf;
					}
				}
			}
		}
	}
	return -1;
}

static inline bool has_valid_T_arr(int inode) {
	/* Has this tnode's arrival time been changed from its original value of HUGE_NEGATIVE_FLOAT? */
    auto& timing_ctx = g_vpr_ctx.timing();
	return (bool) (timing_ctx.tnodes[inode].T_arr > HUGE_NEGATIVE_FLOAT + 1);
}

static inline bool has_valid_T_req(int inode) {
	/* Has this tnode's required time been changed from its original value of HUGE_POSITIVE_FLOAT? */
    auto& timing_ctx = g_vpr_ctx.timing();
	return (bool) (timing_ctx.tnodes[inode].T_req < HUGE_POSITIVE_FLOAT - 1);
}

#ifndef PATH_COUNTING
bool has_valid_normalized_T_arr(int inode) {
	/* Has this tnode's normalized_T_arr been changed from its original value of HUGE_NEGATIVE_FLOAT? */
    auto& timing_ctx = g_vpr_ctx.timing();
	return (bool) (timing_ctx.tnodes[inode].prepacked_data->normalized_T_arr > HUGE_NEGATIVE_FLOAT + 1);
}
#endif

float get_critical_path_delay() {
	/* Finds the critical path delay, which is the minimum clock period required to meet the constraint
	corresponding to the pair of source and sink clock domains with the least slack in the design. */

	int source_clock_domain, sink_clock_domain;
	float least_slack_in_design = HUGE_POSITIVE_FLOAT, critical_path_delay = NAN;

    auto& timing_ctx = g_vpr_ctx.timing();
    auto& device_ctx = g_vpr_ctx.device();

	if (!timing_ctx.sdc) return UNDEFINED; /* If timing analysis is off, for instance. */

	for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			if (least_slack_in_design > f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]) {
				least_slack_in_design = f_timing_stats->least_slack[source_clock_domain][sink_clock_domain];
				critical_path_delay = f_timing_stats->cpd[source_clock_domain][sink_clock_domain];
			}
		}
	}
	if (device_ctx.pb_max_internal_delay != UNDEFINED && device_ctx.pb_max_internal_delay > critical_path_delay) {
		critical_path_delay = device_ctx.pb_max_internal_delay;
    }

	return critical_path_delay * 1e9; /* Convert to nanoseconds */
}

void print_timing_stats() {

	/* Prints critical path delay/fmax, least slack in design, and, for multiple-clock designs,
	minimum required clock period to meet each constraint, least slack per constraint,
	geometric average clock frequency, and fanout-weighted geometric average clock frequency. */

	int source_clock_domain, sink_clock_domain, clock_domain, fanout, total_fanout = 0,
		num_netlist_clocks_with_intra_domain_paths = 0;
	float geomean_period = 1., least_slack_in_design = HUGE_POSITIVE_FLOAT, critical_path_delay = UNDEFINED;
	double fanout_weighted_geomean_period = 0.;
	bool found;

    auto& timing_ctx = g_vpr_ctx.timing();

	for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			if (least_slack_in_design > f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]) {
				least_slack_in_design = f_timing_stats->least_slack[source_clock_domain][sink_clock_domain];
            }
        }
    }

	/* Find critical path delay. If the device_ctx.pb_max_internal_delay is greater than this, it becomes
	 the limiting factor on critical path delay, so print that instead, with a special message. */
    critical_path_delay = get_critical_path_delay();
    VTR_LOG("Final critical path: %g ns", critical_path_delay);

	if (timing_ctx.sdc->num_constrained_clocks <= 1) {
		/* Although critical path delay is always well-defined, it doesn't make sense to talk about fmax for multi-clock circuits */
		VTR_LOG(", f_max: %g MHz", 1e3 / critical_path_delay);
	}
	VTR_LOG("\n");

	/* Also print the least slack in the design */
	VTR_LOG("\n");
	VTR_LOG("Least slack in design: %g ns\n", 1e9 * least_slack_in_design);
	VTR_LOG("\n");

	if (timing_ctx.sdc->num_constrained_clocks > 1) { /* Multiple-clock design */

		/* Print minimum possible clock period to meet each constraint. Convert to nanoseconds. */

		VTR_LOG("Minimum possible clock period to meet each constraint (including skew effects):\n");
		for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
			/* Print the intra-domain constraint if it was analysed. */
			if (timing_ctx.sdc->domain_constraint[source_clock_domain][source_clock_domain] > NEGATIVE_EPSILON) {
				VTR_LOG("%s to %s: %g ns (%g MHz)\n",
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						1e9 * f_timing_stats->cpd[source_clock_domain][source_clock_domain],
						1e-6 / f_timing_stats->cpd[source_clock_domain][source_clock_domain]);
			} else {
				VTR_LOG("%s to %s: --\n",
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name);
			}
			/* Then, print all other constraints on separate lines, indented. We re-print
			the source clock domain's name so there's no ambiguity when parsing. */
			for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain] > NEGATIVE_EPSILON) {
					/* If this domain pair was analysed */
					VTR_LOG("\t%s to %s: %g ns (%g MHz)\n",
							timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
							timing_ctx.sdc->constrained_clocks[sink_clock_domain].name,
							1e9 * f_timing_stats->cpd[source_clock_domain][sink_clock_domain],
							1e-6 / f_timing_stats->cpd[source_clock_domain][sink_clock_domain]);
				} else {
					VTR_LOG("\t%s to %s: --\n",
							timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
							timing_ctx.sdc->constrained_clocks[sink_clock_domain].name);
				}
			}
		}

		/* Print least slack per constraint. */

		VTR_LOG("\n");
		VTR_LOG("Least slack per constraint:\n");
		for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
			/* Print the intra-domain slack if valid. */
			if (f_timing_stats->least_slack[source_clock_domain][source_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
				VTR_LOG("%s to %s: %g ns\n",
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						1e9 * f_timing_stats->least_slack[source_clock_domain][source_clock_domain]);
			} else {
				VTR_LOG("%s to %s: --\n",
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
						timing_ctx.sdc->constrained_clocks[source_clock_domain].name);
			}
			/* Then, print all other slacks on separate lines. */
			for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
				if (source_clock_domain == sink_clock_domain) continue; /* already done that */
				if (f_timing_stats->least_slack[source_clock_domain][sink_clock_domain] < HUGE_POSITIVE_FLOAT - 1) {
					/* If this domain pair was analysed and has a valid slack */
					VTR_LOG("\t%s to %s: %g ns\n",
							timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
							timing_ctx.sdc->constrained_clocks[sink_clock_domain].name,
							1e9 * f_timing_stats->least_slack[source_clock_domain][sink_clock_domain]);
				} else {
					VTR_LOG("\t%s to %s: --\n",
							timing_ctx.sdc->constrained_clocks[source_clock_domain].name,
							timing_ctx.sdc->constrained_clocks[sink_clock_domain].name);
				}
			}
		}

		/* Calculate geometric mean f_max (fanout-weighted and unweighted) from the diagonal (intra-domain) entries of critical_path_delay,
		excluding domains without intra-domain paths (for which the timing constraint is DO_NOT_ANALYSE) and virtual clocks. */
		found = false;
		for (clock_domain = 0; clock_domain < timing_ctx.sdc->num_constrained_clocks; clock_domain++) {
			if (timing_ctx.sdc->domain_constraint[clock_domain][clock_domain] > NEGATIVE_EPSILON && timing_ctx.sdc->constrained_clocks[clock_domain].is_netlist_clock) {
				geomean_period *= f_timing_stats->cpd[clock_domain][clock_domain];
				fanout = timing_ctx.sdc->constrained_clocks[clock_domain].fanout;
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
			VTR_LOG("\n");
			VTR_LOG("Geometric mean intra-domain period: %g ns (%g MHz)\n",
					1e9 * geomean_period, 1e-6 / geomean_period);
			VTR_LOG("Fanout-weighted geomean intra-domain period: %g ns (%g MHz)\n",
					1e9 * fanout_weighted_geomean_period, 1e-6 / fanout_weighted_geomean_period);
		}

		VTR_LOG("\n");
	}
}

static void print_timing_constraint_info(const char *fname) {
	/* Prints the contents of timing_ctx.sdc->domain_constraint, timing_ctx.sdc->constrained_clocks, constrained_ios and timing_ctx.sdc->cc_constraints to a file. */

	FILE * fp;
	int source_clock_domain, sink_clock_domain, i, j;
	int max_clock_name_length = INT_MIN;
	char * clock_name;

    auto& timing_ctx = g_vpr_ctx.timing();

	int * clock_name_length = (int *) vtr::malloc(timing_ctx.sdc->num_constrained_clocks * sizeof(int)); /* Array of clock name lengths */

	fp = vtr::fopen(fname, "w");

	/* Get lengths of each clock name and max length so we can space the columns properly. */
	for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
		clock_name = timing_ctx.sdc->constrained_clocks[sink_clock_domain].name;
		clock_name_length[sink_clock_domain] = strlen(clock_name);
		if (clock_name_length[sink_clock_domain] > max_clock_name_length) {
			max_clock_name_length = clock_name_length[sink_clock_domain];
		}
	}

	/* First, combine info from timing_ctx.sdc->domain_constraint and timing_ctx.sdc->constrained_clocks into a matrix
	(they're indexed the same as each other). */

	fprintf(fp, "Timing constraints in ns (source clock domains down left side, sink along top).\n"
				"A value of -1.00 means the pair of source and sink domains will not be analysed.\n\n");

	print_spaces(fp, max_clock_name_length + 4);

	for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
		fprintf(fp, "%s", timing_ctx.sdc->constrained_clocks[sink_clock_domain].name);
		/* Minimum column width of 8 */
		print_spaces(fp, max(8 - clock_name_length[sink_clock_domain], 4));
	}
	fprintf(fp, "\n");

	for (source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		fprintf(fp, "%s", timing_ctx.sdc->constrained_clocks[source_clock_domain].name);
		print_spaces(fp, max_clock_name_length + 4 - clock_name_length[source_clock_domain]);
		for (sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
			fprintf(fp, "%5.2f", timing_ctx.sdc->domain_constraint[source_clock_domain][sink_clock_domain]);
			/* Minimum column width of 8 */
			print_spaces(fp, max(clock_name_length[sink_clock_domain] - 1, 3));
		}
		fprintf(fp, "\n");
	}

	free(clock_name_length);

	/* Second, print I/O constraints. */
	fprintf(fp, "\nList of constrained inputs (note: constraining a clock net input has no effect):\n");
	for (i = 0; i < timing_ctx.sdc->num_constrained_inputs; i++) {
		fprintf(fp, "Input name %s on clock %s with input delay %.2f ns\n",
			timing_ctx.sdc->constrained_inputs[i].name, timing_ctx.sdc->constrained_inputs[i].clock_name, timing_ctx.sdc->constrained_inputs[i].delay);
	}

	fprintf(fp, "\nList of constrained outputs:\n");
	for (i = 0; i < timing_ctx.sdc->num_constrained_outputs; i++) {
		fprintf(fp, "Output name %s on clock %s with output delay %.2f ns\n",
			timing_ctx.sdc->constrained_outputs[i].name, timing_ctx.sdc->constrained_outputs[i].clock_name, timing_ctx.sdc->constrained_outputs[i].delay);
	}

	/* Third, print override constraints. */
	fprintf(fp, "\nList of override constraints (non-default constraints created by set_false_path, set_clock_groups, \nset_max_delay, and set_multicycle_path):\n");

	for (i = 0; i < timing_ctx.sdc->num_cc_constraints; i++) {
		fprintf(fp, "Clock domain");
		for (j = 0; j < timing_ctx.sdc->cc_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->cc_constraints[i].source_list[j]);
		}
		fprintf(fp, " to clock domain");
		for (j = 0; j < timing_ctx.sdc->cc_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->cc_constraints[i].sink_list[j]);
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (timing_ctx.sdc->cc_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", timing_ctx.sdc->cc_constraints[i].sink_list[j], timing_ctx.sdc->cc_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", timing_ctx.sdc->cc_constraints[i].sink_list[j], timing_ctx.sdc->cc_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < timing_ctx.sdc->num_cf_constraints; i++) {
		fprintf(fp, "Clock domain");
		for (j = 0; j < timing_ctx.sdc->cf_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->cf_constraints[i].source_list[j]);
		}
		fprintf(fp, " to flip-flop");
		for (j = 0; j < timing_ctx.sdc->cf_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->cf_constraints[i].sink_list[j]);
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (timing_ctx.sdc->cf_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", timing_ctx.sdc->cf_constraints[i].sink_list[j], timing_ctx.sdc->cf_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", timing_ctx.sdc->cf_constraints[i].sink_list[j], timing_ctx.sdc->cf_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < timing_ctx.sdc->num_fc_constraints; i++) {
		fprintf(fp, "Flip-flop");
		for (j = 0; j < timing_ctx.sdc->fc_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->fc_constraints[i].source_list[j]);
		}
		fprintf(fp, " to clock domain");
		for (j = 0; j < timing_ctx.sdc->fc_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->fc_constraints[i].sink_list[j]);
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (timing_ctx.sdc->fc_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", timing_ctx.sdc->fc_constraints[i].sink_list[j], timing_ctx.sdc->fc_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", timing_ctx.sdc->fc_constraints[i].sink_list[j], timing_ctx.sdc->fc_constraints[i].num_multicycles);
		}
	}

	for (i = 0; i < timing_ctx.sdc->num_ff_constraints; i++) {
		fprintf(fp, "Flip-flop");
		for (j = 0; j < timing_ctx.sdc->ff_constraints[i].num_source; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->ff_constraints[i].source_list[j]);
		}
		fprintf(fp, " to flip-flop");
		for (j = 0; j < timing_ctx.sdc->ff_constraints[i].num_sink - 1; j++) {
			fprintf(fp, " %s,", timing_ctx.sdc->ff_constraints[i].sink_list[j]);
		} /* We have to print the last one separately because we don't want a comma after it. */
		if (timing_ctx.sdc->ff_constraints[i].num_multicycles == 0) { /* not a multicycle constraint */
			fprintf(fp, " %s: %.2f ns\n", timing_ctx.sdc->ff_constraints[i].sink_list[j], timing_ctx.sdc->ff_constraints[i].constraint);
		} else { /* multicycle constraint */
			fprintf(fp, " %s: %d multicycles\n", timing_ctx.sdc->ff_constraints[i].sink_list[j], timing_ctx.sdc->ff_constraints[i].num_multicycles);
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

/*
 Create a lookup table that returns the tnode index for [iblock][pb_graph_pin_id]
*/
vtr::vector_map<ClusterBlockId, std::vector<int>> alloc_and_load_tnode_lookup_from_pin_id() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& timing_ctx = g_vpr_ctx.timing();

	vtr::vector_map<ClusterBlockId, std::vector<int>> tnode_lookup;
	tnode_lookup.resize(cluster_ctx.clb_nlist.blocks().size());

	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		int num_pins = cluster_ctx.clb_nlist.block_type(blk_id)->pb_graph_head->total_pb_pins;
		tnode_lookup[blk_id].resize(num_pins);
		for (int j = 0; j < num_pins; j++) {
			tnode_lookup[blk_id][j] = OPEN;
		}
	}

	for (int i = 0; i < timing_ctx.num_tnodes; i++) {
		if (timing_ctx.tnodes[i].pb_graph_pin != nullptr) {
			if (timing_ctx.tnodes[i].type != TN_CLOCK_SOURCE &&
				timing_ctx.tnodes[i].type != TN_FF_SOURCE &&
				timing_ctx.tnodes[i].type != TN_INPAD_SOURCE &&
				timing_ctx.tnodes[i].type != TN_FF_SINK &&
				timing_ctx.tnodes[i].type != TN_OUTPAD_SINK
				) {
				int pb_pin_id = timing_ctx.tnodes[i].pb_graph_pin->pin_count_in_cluster;
				ClusterBlockId iblk = timing_ctx.tnodes[i].block;
				VTR_ASSERT(tnode_lookup[iblk][pb_pin_id] == OPEN);
				tnode_lookup[iblk][pb_pin_id] = i;
			}
		}
	}
	return tnode_lookup;
}

void free_tnode_lookup_from_pin_id(vtr::vector_map<ClusterBlockId, std::vector<int>> &tnode_lookup) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		tnode_lookup[blk_id].clear();
	}
	tnode_lookup.clear();
}

std::vector<size_t> init_timing_net_pins() {
    std::vector<size_t> timing_net_pin_counts;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for(auto net_id : cluster_ctx.clb_nlist.nets()) {
        timing_net_pin_counts.push_back(cluster_ctx.clb_nlist.net_pins(net_id).size());
    }

    VTR_ASSERT(timing_net_pin_counts.size() == cluster_ctx.clb_nlist.nets().size());

    return timing_net_pin_counts;
}

std::vector<size_t> init_atom_timing_net_pins() {
    std::vector<size_t> timing_net_pin_counts;
    auto& atom_ctx = g_vpr_ctx.atom();

    for(auto net_id : atom_ctx.nlist.nets()) {
        timing_net_pin_counts.push_back(atom_ctx.nlist.net_pins(net_id).size());
    }

    VTR_ASSERT(timing_net_pin_counts.size() == atom_ctx.nlist.nets().size());

    return timing_net_pin_counts;
}

size_t num_timing_nets() {
    return f_num_timing_net_pins.size();
}

size_t num_timing_net_pins(int inet) {
    return f_num_timing_net_pins[inet];
}

size_t num_timing_net_sinks(int inet) {
    VTR_ASSERT(f_num_timing_net_pins[inet] > 0);
    return f_num_timing_net_pins[inet] - 1;
}

void print_classic_cpds() {
    auto& timing_ctx = g_vpr_ctx.timing();

	for (int source_clock_domain = 0; source_clock_domain < timing_ctx.sdc->num_constrained_clocks; source_clock_domain++) {
		for (int sink_clock_domain = 0; sink_clock_domain < timing_ctx.sdc->num_constrained_clocks; sink_clock_domain++) {
            float least_slack = f_timing_stats->least_slack[source_clock_domain][sink_clock_domain];
            float critical_path_delay = f_timing_stats->cpd[source_clock_domain][sink_clock_domain];

            VTR_LOG("Classic %d -> %d: least_slack=%g cpd=%g\n", source_clock_domain, sink_clock_domain, least_slack, critical_path_delay);
		}
	}
}

static void mark_max_block_delay(const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();

    for(AtomBlockId blk : atom_ctx.nlist.blocks()) {
        auto iter = expected_lowest_cost_pb_gnode.find(blk);
        if(iter != expected_lowest_cost_pb_gnode.end()) {
            const t_pb_graph_node* gnode = iter->second;
            const t_pb_type* pb_type = gnode->pb_type;

			if (pb_type->max_internal_delay != UNDEFINED) {
				if (device_ctx.pb_max_internal_delay == UNDEFINED) {
					device_ctx.pb_max_internal_delay = pb_type->max_internal_delay;
					device_ctx.pbtype_max_internal_delay = pb_type;
				} else if (device_ctx.pb_max_internal_delay < pb_type->max_internal_delay) {
					device_ctx.pb_max_internal_delay = pb_type->max_internal_delay;
					device_ctx.pbtype_max_internal_delay = pb_type;
				}
			}
        }
    }
}
