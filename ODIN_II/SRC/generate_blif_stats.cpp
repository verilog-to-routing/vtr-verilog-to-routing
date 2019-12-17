/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"
#include "output_blif.h"
#include "generate_blif_stats.h"

#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_util.h"
#include "vtr_memory.h"

void depth_first_traversal_to_output_stats(short marker_value, netlist_t *netlist, FILE *out);
void depth_traverse_output_blif_stats(nnode_t *node, int traverse_mark_number);
void gather_vcc_gnd_stats(short marker_value, netlist_t *netlist);
void output_node_stats(nnode_t *node, short traverse_number);
void depth_traverse_gnd_vcc(nnode_t *node, int traverse_mark_number);
void depth_traverse_clear_stat(nnode_t *node, int traverse_mark_number , bool hardClear);
int get_max_levels_rec(nnode_t *node, int traverse_mark_number, int cycleId, netlist_t *netlist);
int get_max_levels(short marker_value, netlist_t *netlist);
void single_node_lookahead(nnode_t *node, short travId, short marker);
//int reverse_lvl(short marker_value, netlist_t *netlist);
//int reverse_lvl_rec(nnode_t *node, int traverse_mark_number, int cycleId, netlist_t *netlist);
void node_profile(nnode_t *node, bool verbose, FILE *out);
int get_node_fanout(nnode_t *node);
void set_path_lengths(nnode_t *node, short marker);
int pathlen_heuristic(nnode_t *node, short marker);

static unsigned int num_latches = 0;
static unsigned int vcc_fanout_live = 0;
static unsigned int gnd_fanout_live = 0;
static unsigned int pad_fanout_live = 0;
static unsigned int vcc_fanout_all = 0;
static unsigned int gnd_fanout_all = 0;
static unsigned int pad_fanout_all = 0;
static unsigned int max_input_pathrange = 0; // this is like reverse net_range bc it is dist to output not depth, abs(min-max pathlength)
static unsigned int min_input_pathrange = 0-1;
static float avg_pathrange = 0;
static unsigned int num_nodes = 0;
static unsigned int num_carry_func = 0;
static unsigned int num_names = 0; // consider renaming this to num_gates bc that's what it is
static unsigned int num_nets = 0; // same as output pin total it seems
static unsigned int num_edges_fo = 0; // fan out total for all nets (includes vcc and gnd, unclear if it should, abc does not)
static unsigned int num_edges_fi = 0; // fan in total, will equal num_total_input_pins and thus is redundant
static unsigned int num_logic_nodes = 0;
static unsigned int num_total_input_pins = 0; // reflects number of edges in graph that are live, should equal num_edges_fo but may not if there are fo pins w/out node 
static unsigned int output_pin_total = 0; // this (should) always be the same as num_nets, maybe redundant
static unsigned int num_subckts = 0;
static unsigned int num_muxes = 0;
static unsigned int num_generic_nodes = 0;
static unsigned int num_input_nodes = 0;
static unsigned int top_input_fanout = 0;
static unsigned int num_unconn = 0; // Unreliable, maybe count pad traversals
static unsigned int num_clock_nodes = 0;
static unsigned int num_adders = 0;

// This is all metadata on the number of traversals it took to calculate these figures, not passed to parser
static unsigned int num_traversals_stat = 0; // how many depth first traversals have we done? Internal stat metric to measure cost of collection
static unsigned int num_traversals_input = 0; // only counts traversals from top inputs (derived value not counter)
static unsigned int gnd_vcc_pad_traverals = 0; // metadata on measuring gnd and vcc traversals, should be minimal if non-verbose and after all other traversals
static unsigned int num_clearing_traversals = 0; // node traversals from clearing stat structs

//static unsigned int num_unique_paths = 0; // Experimental, number of unique paths, this would be hard to calculate
static unsigned int fanin_outputs_total = 0; // this is possibly not a useful stat
//static unsigned int longest_chain = 1; // we're looking at latches but one might want adders or carries etc.
//static unsigned int longest_consecutive_chain = 1;
static unsigned int fanin_node_to_output = 0; // not very interesting
static unsigned int fanout_node_to_output = 0; // experimental
static unsigned int num_nodes_leading_to_output = 0; // unless we have fanout in net this will equal fanin_outputs_total, isn't v interesting but needed to derive
static unsigned int num_deadends = 0; // experimental, every time we hit a deadEnd in path not including already visited nodes
static unsigned int num_already_visited = 0; // experimental, every time we hit a previously visited node, prob not interesting but maybe
static unsigned int num_deadend_inputs = 0; // number of inputs that go nowhere, dist values of 0, not including vcc and gnd for now
static unsigned int max_fanout_node = 0; // highest # of fanout pins of a node including all of its associated nets
static unsigned int max_outpins_node = 0; // highest num_output_pins of a node, tends to be low
static unsigned int max_fanout_net = 0; // num_output_pins of net with highest value
static unsigned int max_fanin_node = 0;
static int output_node_count = 0; // count outputs so we can figure out if any outputs are unconn
// our averages do not include zero length paths
static unsigned int max_dist_out = 0;
static unsigned int min_dist_out = 0-1;
static unsigned int gnd_min_path = 0;
static unsigned int vcc_min_path = 0;
static unsigned int gnd_max_path = 0;
static unsigned int vcc_max_path = 0;
static unsigned int shortest_maxpath = 0-1; 
static unsigned int longest_minpath = 0;
static float top_input_avg_dist_max = 0; // avg longest path of top level inputs
static float top_input_avg_dist_min = 0;
static float top_lvl_avg_max = 0; // this name is confusing, sum of node->stat->avg_max_dist_to_out of top input nodes, experimental, possibly not useful (average of averages...)
static unsigned int num_memories = 0;
static unsigned int max_level = 0; // max number of levels in a CIs logic cone
static bool sequentialCycles = false; // not for parser

static bool verbose_mode = false; // only set once, hoping to avoid parameter bloat

void output_blif_stats(std::string file_name, netlist_t *netlist, bool verbose)
{
	if(verbose)
		verbose_mode = true; // file scope var
	FILE *out = fopen(file_name.c_str(), "w");
	
	if (out == NULL) // if output not specified print to stdout
	{
		// This will only work with "" as the output file, can't be nothing
		printf("No output file specified, printing to stdout\n");
		out = stdout;
		// the parser can read the output but for now we don't care about it so it's just a printf
		printf("output_file: stdout\n"); // it's more important to get input_file to the parser but we may as well capture this too
	}
	else{
		printf("output_file: %s\n", file_name.c_str());
		printf("input_file: %s\n", configuration.list_of_file_names[0].c_str()); // print this to stdout and to fout later
	}
	fprintf(out, "input_file: %s\n", configuration.list_of_file_names[0].c_str());

	for (int i = 0; i < netlist->num_top_output_nodes; i++)
	{
		if (netlist->top_output_nodes[i]->input_pins[0]->net->driver_pin == NULL)
		{
			// remove maybe
			warning_message(NETLIST_ERROR, netlist->top_output_nodes[i]->related_ast_node->line_number, netlist->top_output_nodes[i]->related_ast_node->file_number, "This output is undriven (%s) and will be removed\n", netlist->top_output_nodes[i]->name);
		}
	}
	/* add gnd, unconn, and vcc */
	num_names+=3; // consider removing, not in spirit of tracking .names (but yields same results as other node counter from simulator)

	/* traverse the internals of the flat net-list */
		max_level = get_max_levels(SEQUENTIAL_LEVELIZE, netlist); // get levels first (full traversal required)
		depth_first_traversal_to_output_stats(STATS, netlist, out);

/* ==== This is by far the most costly approach to gathering (average) path length statistics, less prone feedback loops*/
		/*int sum_all_dist = 0;
		int k = 0;
		while(k < netlist->num_top_input_nodes){
			for(int i = 0; i<100; i++){
				depth_traverse_clear_stat(netlist->top_input_nodes[k], RESET, true);
				sum_all_dist += pathlen_heuristic(netlist->top_input_nodes[k], marker);
				if(sum_all_dist == 0)
					continue;
			}
			if(sum_all_dist != 0)
				printf("average path length heuristic %f\t", (float)sum_all_dist / (float)1000);
			node_profile(netlist->top_input_nodes[k], false, out);
			k++;
			sum_all_dist = 0;
		}
		*/

	/* connect all the outputs up to the last gate */
	for (int i = 0; i < netlist->num_top_output_nodes; i++)
	{
		nnode_t *node = netlist->top_output_nodes[i];
		if (node->input_pins[0]->net->driver_pin != NULL)
		{
			if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED)
				num_names++;
			else
			{
				/*  Use the name of the driver pin as the name of the driver as long as that name is set, and is not equal to the name of the output pin.
				 *  Otherwise, use the name of the driver node.*/
				char *driver = node->input_pins[0]->net->driver_pin->name;
				char *output = node->name;
				if (!driver || !strcmp(driver,output))
					driver = node->input_pins[0]->net->driver_pin->node->name; // I still don't know what this is
				/* Skip if the driver and output have the same name (i.e. the output of a flip-flop) */
				if (strcmp(driver,output) != 0)
					num_names++;
			}
		}
	}
	float avg_output_pins_per_node = (float)output_pin_total / (float)num_nodes; // POs don't have output pins so probably < 1, not very interesting number
	fprintf(out, "==============COUNTERS=============\n");
	fprintf(out, "num_nodes: %u\n", num_nodes); // verified correct
	fprintf(out, "num_latches: %u\n", num_latches); // verified correct
	fprintf(out, "num_total_input_pins: %u\n", num_total_input_pins); // a measure of fanin to nodes
	fprintf(out, "num_nets: %u\n", num_nets);
	fprintf(out, "num_names: %u\n", num_names); // verified correct
	fprintf(out, "num_edges_fo: %u\n", num_edges_fo); // All fanout from all nets
	//fprintf(out, "num_edges_fi: %u\n", num_edges_fi); // probably pointless? should always be == num_total_input_pins
	fprintf(out, "output_pin_total: %u\n", output_pin_total); // pointless? If including output nodes the number can be < 1
	fprintf(out, "num_subckts: %u\n", num_subckts);
	fprintf(out, "num_muxes: %u\n", num_muxes);
	//fprintf(out, "num_ff_nodes: %d\n", netlist->num_ff_nodes); // # latches: This value is already available and seems correct always but we play it safe and count the latches
	fprintf(out, "num_top_output_nodes: %u\n", netlist->num_top_output_nodes);	
	fprintf(out, "num_input_nodes: %u\n", num_input_nodes); // this tends to be off by one compared to struct member, represents true input nodes, not clocks
	if(verbose_mode)
	{
		fprintf(out, "(W/ clock %d top inputs)\n", netlist->num_top_input_nodes); // includes clock, not in parser
		fprintf(out, "num_deadend_inputs: %u\n", num_deadend_inputs); // number of inputs that go nowhere, dist values of 0
		fprintf(out, "num_clock_nodes: %u\n", num_clock_nodes); // this doesn't include global sim base clock which is confusingly of type input
		fprintf(out, "num_logic_nodes: %u\n", num_logic_nodes);
		fprintf(out, "num_carry_func: %u\n", num_carry_func);
		fprintf(out, "num_generic_nodes: %u\n", num_generic_nodes);
		fprintf(out, "num_unconn: %u\n", num_unconn); // unreliable, how would we find an unconn node?
		fprintf(out, "num_memories: %u\n", num_memories);
		fprintf(out, "num_adders: %u\n", num_adders);
		fprintf(out, "num_deadends: %u\n", num_deadends); // unclear if fully reliable
		fprintf(out, "num_already_visited: %u\n", num_already_visited); // How many times do we encounter a node we've already visited
		if(output_node_count < netlist->num_top_output_nodes) // not super important
		{
			fprintf(out, "WARNING UNCONNECTED OUTPUTS, TRUE COUNT: %d\n", output_node_count); // difference is we counted these, only show if there is a disparity
			fprintf(out, "num_dead_outputs: %d\n", abs(output_node_count - netlist->num_top_output_nodes)); // should we print this always? Doesn't do parser much good
		}
	}

	fprintf(out, "==============FANIN FANOUT=============\n");
	fprintf(out, "avg_fanin_per_node: %f\n", ((float)num_edges_fi / (float)num_nodes));
	fprintf(out, "avg_fanout_per_net: %f\n", ((float)num_edges_fo/(float)num_nets));
	fprintf(out, "top_input_fanout: %u\n", top_input_fanout); // this does not include clock fanout, this refers to top input's net fanout, confusing
	float avg_top_lvl_fanout = (num_input_nodes - num_deadend_inputs > 0) ? ((float)top_input_fanout/(float)(num_input_nodes - num_deadend_inputs)) : 0.0; // no div by 0
	fprintf(out, "avg_top_lvl_fanout: %f\n", avg_top_lvl_fanout); // this does not include clock fanout (or deadend inputs for now)
	fprintf(out, "max_outpins_node: %u\n", max_outpins_node); // node with most output pins
	fprintf(out, "max_fanout_node: %u\n", max_fanout_node); // fanout including all connected output pins and associated nets
	fprintf(out, "max_fanout_net: %u\n", max_fanout_net);
	fprintf(out, "max_fanin_node: %u\n", max_fanin_node);
	fprintf(out, "pad_fanout_all: %u\n", pad_fanout_all);
	fprintf(out, "vcc_fanout_all: %u\n", vcc_fanout_all);
	fprintf(out, "gnd_fanout_all: %u\n", gnd_fanout_all);
	if(verbose_mode)
	{
		fprintf(out, "pad_fanout_live: %u\n", pad_fanout_live);
		fprintf(out, "vcc_fanout_live: %u\n", vcc_fanout_live);
		fprintf(out, "gnd_fanout_live: %u\n", gnd_fanout_live);
		fprintf(out, "avg_output_pins_per_node: %f\n", avg_output_pins_per_node);
		fprintf(out, "fanin_outputs_total: %u\n", fanin_outputs_total); // num inputs for all top level outputs, seems to often match num outputs
		fprintf(out, "fanin_outputs_avg: %f\n", (float)(fanin_outputs_total / netlist->num_top_output_nodes)); // maybe only use this since the above can be derived
		/* Even in verbose mode these seem unimportant */
		fprintf(out, "fanin_node_to_output: %u\n", fanin_node_to_output); // node that leads to output
		fprintf(out, "fanout_node_to_output: %u\n", fanout_node_to_output); // node that leads to output
		fprintf(out, "avg_fanin_node_leading_to_output: %f\n", (float)fanin_node_to_output / (float)num_nodes_leading_to_output); // var too long, not important or in parser
		fprintf(out, "avg_fanout_node_leading_to_output: %f\n", (float)fanout_node_to_output / (float)num_nodes_leading_to_output); // var too long, not important or in parser
	}

	fprintf(out, "==============DEPTH=============\n");
	fprintf(out,"max_level: %u\n", max_level); // largest number of levels in COs logic cone, is actually verifiably accurate unlike other possibly flawed path metrics
	/* Unclear how reliable these numbers are for circuits with sequential feedback loops */
	if(verbose_mode)
	{
		fprintf(out, "min_dist_out: %u\n", min_dist_out); // min dist input to output
		fprintf(out, "max_dist_out: %u\n", max_dist_out);
		fprintf(out, "top_input_avg_dist_min: %f\n", top_input_avg_dist_min); // not including vcc,gnd,clock
		fprintf(out, "top_input_avg_dist_max: %f\n", top_input_avg_dist_max);
		fprintf(out, "gnd_min_path: %u\n", gnd_min_path);
		fprintf(out, "gnd_max_path: %u\n", gnd_max_path);
		fprintf(out, "vcc_min_path: %u\n", vcc_min_path);
		fprintf(out, "vcc_max_path: %u\n", vcc_max_path);
		fprintf(out, "shortest_maxpath: %u\n", shortest_maxpath);
		fprintf(out, "longest_minpath: %u\n", longest_minpath);
		fprintf(out, "min_input_pathrange: %u\n", min_input_pathrange);
		fprintf(out, "max_input_pathrange: %u\n", max_input_pathrange);
		fprintf(out, "avg_pathrange: %f\n", avg_pathrange);

		//fprintf(out, "----------------Approximation-------------\n");
		// TODO: Some kind of mechanism that counts the longest chain of latches or something, maybe with the get_max_levels algorithm
		//fprintf(out, "longest_latch_chain: %u\n", longest_chain); // number of nodes of same type in chain in one path, in this case latches
		//fprintf(out, "longest_consecutive_latch_chain: %u\n", longest_consecutive_chain); // number of node type connected to eachother in one path, in this case latches
		fprintf(out, "==============EXPERIMENTAL/METADATA=============\n");
		fprintf(out, "num_traversals_stat: %u\n", num_traversals_stat);
		//fprintf(out, "num_unique_paths: %u\n", num_unique_paths); // this would be hard to calculate
		fprintf(out, "num_traversals_input: %u\n", num_traversals_input);
		fprintf(out, "gnd_vcc_pad_traverals: %u\n", gnd_vcc_pad_traverals);
		fprintf(out, "num_clearing_traversals: %u\n", num_clearing_traversals);
		fprintf(out, "top_lvl_avg_max: %f\n", top_lvl_avg_max); // confusing name, avg of avg max dist of top input nodes
		std::string sequential_cycle_out = sequentialCycles ? "Yes" : "No";
		fprintf(out, "has_sequential_cycles: %s\n", sequential_cycle_out.c_str()); // not for parser 
	}
}

/*---------------------------------------------------------------------------
 * (function: depth_first_traversal_to_output_stats()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_output_stats(short marker_value, netlist_t *netlist, FILE *out)
{
	/* start with the primary input list */
	int min_dist_temp = 0;
	int max_dist_temp = 0;
	int total_dist_out_max = 0;
	int total_dist_out_min = 0;
	int pathrange = 0; // temp var
	int total_pathrange = 0;
	float sum_avgs = 0;
	for (int i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			/* Without altering mark number the first few paths traversed will always yield higher values so top input pathlen is inherently flawed value */
			depth_traverse_output_blif_stats(netlist->top_input_nodes[i], marker_value);
		}
	}
	if(verbose_mode)
	{
		for (int i = 0; i < netlist->num_top_input_nodes; i++)
		{
			if (netlist->top_input_nodes[i] == NULL) //this outcome is fairly unlikely and perhaps should just throw and error and quit
			{
				num_deadend_inputs++;
				num_deadends++;
				continue;
			}
			/* The idea here is to separate path calcs from node profiling so that we can clear the netlist stat structs and markers on each top lvl traversal.
			 * Very costly but otherwise top level results wrong.*/
			depth_traverse_clear_stat(netlist->top_input_nodes[i], RESET, true);
			set_path_lengths(netlist->top_input_nodes[i], marker_value);
			//node_profile(netlist->top_input_nodes[i], true, out); // For netlists with larger numbers of inputs this may be far too verbose but is sometimes helpful
			/* Without altering/resetting mark number the first few paths traversed will always yield higher values so top input pathlen will be a flawed value */
			min_dist_temp = netlist->top_input_nodes[i]->stat->min_dist_to_out;
			max_dist_temp = netlist->top_input_nodes[i]->stat->max_dist_to_out;
			float avg_max_temp = netlist->top_input_nodes[i]->stat->avg_max_dist;
			if(max_dist_temp == 0)
				num_deadend_inputs++;
			sum_avgs += avg_max_temp;
			// exclude deadend inputs (e.g. sim clock)
			if(min_dist_temp != 0-1 && max_dist_temp != 0)
			{
				total_dist_out_min += min_dist_temp;
				total_dist_out_max += max_dist_temp;
				if(max_dist_temp > max_dist_out)
					max_dist_out = max_dist_temp;
				if(min_dist_temp < min_dist_out && min_dist_temp != 0) // don't count zero length chains
					min_dist_out = min_dist_temp;
				if(max_dist_temp < shortest_maxpath && max_dist_temp > 0)
					shortest_maxpath = max_dist_temp;
				if(min_dist_temp > longest_minpath)
					longest_minpath = min_dist_temp;
				pathrange = abs(min_dist_temp - max_dist_temp);
				total_pathrange += pathrange;
				if(pathrange > max_input_pathrange)
					max_input_pathrange = pathrange;
				if(pathrange < max_input_pathrange)
					min_input_pathrange = pathrange;
			}
		}
		top_input_avg_dist_min = (float)total_dist_out_min / (float)(netlist->num_top_input_nodes - num_deadend_inputs); // now not excluding clock
		top_input_avg_dist_max = (float)total_dist_out_max / (float)(netlist->num_top_input_nodes - num_deadend_inputs);
		avg_pathrange = (float)total_pathrange / (float)(netlist->num_top_input_nodes - num_deadend_inputs);
		top_lvl_avg_max = (float)sum_avgs / (float)(netlist->num_top_input_nodes - num_deadend_inputs);
	}
	/* traverse gnd, vcc, pad nodes */
	num_nodes += 3;
	gather_vcc_gnd_stats(STATS, netlist); // collect vcc and gnd stats after normal stats collected
	num_traversals_input = num_traversals_stat - gnd_vcc_pad_traverals;
}

/* Begins Traversal of gnd, vcc and pad nodes, we run this after traversing inputs because otherwise they will affect input node stats */
void gather_vcc_gnd_stats(short marker_value, netlist_t *netlist)
{
	netlist->gnd_node->name = vtr::strdup("gnd");
	netlist->vcc_node->name = vtr::strdup("vcc");
	netlist->pad_node->name = vtr::strdup("unconn");
	/* now traverse the ground, vcc, and unconn pins */
	// we just quintupled our traversals to collect gnd stats, reconsider
	vcc_fanout_all = get_node_fanout(netlist->vcc_node);
	gnd_fanout_all = get_node_fanout(netlist->gnd_node);
	pad_fanout_all = get_node_fanout(netlist->pad_node);
	num_edges_fo += (vcc_fanout_all + gnd_fanout_all + pad_fanout_all); // ABC does not count these as edges, should we?
	// these are very aggressive clearing measures
	if(verbose_mode)
	{
		depth_traverse_clear_stat(netlist->gnd_node, RESET, true);
		depth_traverse_gnd_vcc(netlist->gnd_node, marker_value);
		gnd_max_path = netlist->gnd_node->stat->max_dist_to_out;
		gnd_min_path = (netlist->gnd_node->stat->min_dist_to_out == 0-1) ? gnd_max_path: netlist->gnd_node->stat->min_dist_to_out;
		depth_traverse_clear_stat(netlist->gnd_node, RESET, true);
		depth_traverse_clear_stat(netlist->vcc_node, RESET, true);
		depth_traverse_gnd_vcc(netlist->vcc_node, marker_value);
		vcc_max_path = netlist->vcc_node->stat->max_dist_to_out;
		vcc_min_path = (netlist->vcc_node->stat->min_dist_to_out == 0-1) ? vcc_max_path: netlist->vcc_node->stat->min_dist_to_out;
		depth_traverse_clear_stat(netlist->vcc_node, RESET, true);
		depth_traverse_gnd_vcc(netlist->pad_node, marker_value);
		depth_traverse_clear_stat(netlist->pad_node, RESET, true);
	}
	num_traversals_stat += gnd_vcc_pad_traverals;
}

/*--------------------------------------------------------------------------
 * (function: depth_traverse_output_blif_stats)
 *------------------------------------------------------------------------*/
void depth_traverse_output_blif_stats(nnode_t *node, int traverse_mark_number)
{
	num_traversals_stat++;
	int i, j;
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	if (node->traverse_visited == traverse_mark_number)
	{
		num_already_visited++;
		return;
	}
	else
	{
		unsigned int total_node_fanout = 0; // experimental
		/* ELSE - this is a new node so depth visit it */
		output_node_stats(node, traverse_mark_number);
		output_pin_total += node->num_output_pins;
		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;
		num_edges_fi += node->num_input_pins;

		if(node->num_output_pins > max_outpins_node)
			max_outpins_node = node->num_output_pins;

		if(node->num_input_pins > max_fanin_node)
			max_fanin_node = node->num_input_pins;

		if(node->type == OUTPUT_NODE)
			fanin_outputs_total += node->num_input_pins;

		if(node->num_output_pins == 0 && node->type != OUTPUT_NODE)
			num_deadends++;

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
			{
				num_deadends++;
				continue;
			}

			next_net = node->output_pins[i]->net;
			num_nets++;
			//next_net->traversal_id = 0;
			if(node->type == INPUT_NODE)
			{
				top_input_fanout += next_net->num_fanout_pins;
			}
			bool check_fanout = true; // exclude vcc and gnd from max fanout of a net check (this may be obsolete)
			/* If we want to exclude a node type's fanout in calculations check for it here and set check_fanout to false
			*/
			if(check_fanout && next_net->num_fanout_pins > max_fanout_net)
				max_fanout_net = next_net->num_fanout_pins;
			if(next_net->num_fanout_pins == 0 && node->type != OUTPUT_NODE)
				num_deadends++;
			total_node_fanout += next_net->num_fanout_pins;

			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				if(next_net->fanout_pins[j] == NULL)
				{
					num_deadends++;
					continue;
				}
				num_edges_fo++;
				next_node = next_net->fanout_pins[j]->node;
				if (next_node == NULL)
				{
					num_deadends++;
					continue;
				}
				if(next_node->type == PAD_NODE)
					num_unconn++; // unclear if this will work, do unconn nodes lead to or from PAD? (probably from)

				if(next_node->type == OUTPUT_NODE)
				{
					num_nodes_leading_to_output++;
					fanin_node_to_output += node->num_input_pins;
					fanout_node_to_output += node->num_output_pins;
				}
				/* recursive call point */
				depth_traverse_output_blif_stats(next_node, traverse_mark_number);
			}
		}
		if(total_node_fanout > max_fanout_node)
			max_fanout_node = total_node_fanout;
		return;
	}
}
/*-------------------------------------------------------------------
 * (function: output_node_stats)
 * 	Depending on node type, collects node stats
 *------------------------------------------------------------------*/
void output_node_stats(nnode_t *node, short /*traverse_number*/)
{
	num_nodes++;
	num_total_input_pins += node->num_input_pins; // living edges
	if( (node->type >= 17 && node->type <=24) || node->type == LOGICAL_EQUAL)
	{
		num_logic_nodes++;
		num_names++;
		return;
	}
	switch (node->type)
	{
		case INPUT_NODE:
			num_input_nodes++;
			break;
		case OUTPUT_NODE:
			output_node_count++;
			//node->stat->min_dist_to_out = node->stat->max_dist_to_out = 0; // obsolete probably
			break;
		case VCC_NODE:
			break;
		case CLOCK_NODE:
			num_clock_nodes++;
			break;
		case GND_NODE: 
			break;
		case PAD_NODE:
			num_unconn++; // not very useful
			break;
		case CARRY_FUNC:
			num_names++;
			num_carry_func++;
			break;
		case ADDER_FUNC:
			num_adders++; /* FALLTHROUGH */
		case LT:
		case GT:
		case BITWISE_NOT:
			num_names++;
 			break;
		case MUX_2:
			num_names++;
			num_muxes++;
			break;
		case MULTIPLY:
		case HARD_IP:
		case ADD:
			num_subckts++;
			break;
		case FF_NODE: 
			num_latches++;
			break;
		case GENERIC:
			num_generic_nodes++;
			num_names++;
			break;
		case MEMORY:
			num_memories++;
			break;
		default:
			break;
	}
}

/*-------------------------------------------------------------------
 * (function: depth_traverse_gnd_vcc)
 * 	Traversal of gnd, vcc and pad nodes, which we treat as separate from the top input node traversal
 *------------------------------------------------------------------*/
void depth_traverse_gnd_vcc(nnode_t *node, int traverse_mark_number)
{
	gnd_vcc_pad_traverals++;
	int i, j;
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	if (node->traverse_visited == traverse_mark_number)
	{
		// The idea here is to fill in gaps when a node has been left unset but visited because of a cycle, it looks ahead one to see if its neighbours have been since set
		if(node->stat->min_dist_to_out+1 == 0)
		{
			single_node_lookahead(node, node->stat->traversal_id+1, traverse_mark_number);
		}
		return;
	}
	else
	{
		if(!node->stat)
			node->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
		node->stat->max_dist_to_out = 0;
		node->stat->min_dist_to_out = 0-1;
		node->stat->avg_max_dist = 0; // experimental value sum max dist of all nets
		// mark node as visited
		node->traverse_visited = traverse_mark_number;
		if(node->type == OUTPUT_NODE)
		{
			node->stat->min_dist_to_out = node->stat->max_dist_to_out = 0;
			return;
		}
		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
				continue;

			next_net = node->output_pins[i]->net;
			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				if(next_net->fanout_pins[j] == NULL)
					continue;

				next_node = next_net->fanout_pins[j]->node;
				if (next_node == NULL)
				{
					continue;
				}
				// this is a pretty uninteresting metric, maybe remove
				if(node->type == VCC_NODE)
					vcc_fanout_live++;
				if(node->type == GND_NODE)
					gnd_fanout_live++;
				if(node->type == PAD_NODE)
					pad_fanout_live++;
				unsigned int test_max = 0;
				unsigned int test_min = 0-1;
				depth_traverse_gnd_vcc(next_node, traverse_mark_number);
				if(verbose_mode)
				{
					test_min = next_node->stat->min_dist_to_out+1;
					test_max = next_node->stat->max_dist_to_out+1;
					if(test_min == 0)
						continue;
					if(node->stat->min_dist_to_out>test_min)
						node->stat->min_dist_to_out = test_min;
					if(node->stat->max_dist_to_out < test_max)
						node->stat->max_dist_to_out = test_max;
				}
			}
		}
		return;
	}
}

/*-------------------------------------------------------------------
 * (function: depth_traverse_clear_stat)
 * 	Traversal of netlist, unmarking all nodes and optionally freeing all node->stat structs.
 *------------------------------------------------------------------*/
void depth_traverse_clear_stat(nnode_t *node, int traverse_mark_number, bool hardClear)
{
	num_clearing_traversals++;
	int i, j;
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	if(node->traverse_visited == traverse_mark_number) // preferably RESET
	{
		return;
	}
	else // Clear anything that isn't traverse_mark_number, removed check for specific mark
	{
		if(node->stat && hardClear)
		{
			free(node->stat);
			node->stat = NULL;
		}
		node->traverse_visited = traverse_mark_number;
		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
			{
				continue;
			}
			next_net = node->output_pins[i]->net;
			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				if(next_net->fanout_pins[j] == NULL)
				{
					continue;
				}
				next_node = next_net->fanout_pins[j]->node;
				if(next_node == NULL)
				{
					continue;
				}
				depth_traverse_clear_stat(next_node, traverse_mark_number, hardClear);
			}
		}
		return;
	}
}

/*-------------------------------------------------------------------
 * (function: get_max_levels)
 * Recursively computes the number of levels in logic cone of each PO and Latch Output and the max level, results and method consistent with that seen in ABC. Considered using this system to get more detailed
 * path information but unable to verify if results were accurate/useful.
 * 	Efforts towards breaking cycles or at least preventing feedback loops from causing huge path length values have not yielded different results, it may be that the path lengths are correct
	and/or not actually being changed by feedback loops or not all cycles are found, more cycles are found traversing inputs upwards than outputs downwards. cycleId is the ID of the associated latch.
	This presently has little - no function as identifying the cycles has breaking them has unclear if any effect. It may be more worthwhile to search for combinational loops.
*/
int get_max_levels(short marker_value, netlist_t *netlist)
{
	int levelMax = 0;
	for (int i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			if(!netlist->top_input_nodes[i]->stat)
			{
				netlist->top_input_nodes[i]->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
				netlist->top_input_nodes[i]->stat->traversal_id = 0;
			}
			netlist->top_input_nodes[i]->stat->depth = 0;
		}
	}
	for (int i = 0; i < netlist->num_ff_nodes; i++)
	{
		if (netlist->ff_nodes[i] != NULL)
		{
			if(!netlist->ff_nodes[i]->stat)
			{
				netlist->ff_nodes[i]->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
				netlist->ff_nodes[i]->stat->traversal_id = 0;
			}
			netlist->ff_nodes[i]->stat->depth = 0;
		}
	}
	for (int i = 0; i < netlist->num_ff_nodes; i++)
	{
		for(int j=0; j < netlist->ff_nodes[i]->num_input_pins; j++)
		{
			nnode_t *node = netlist->ff_nodes[i]->input_pins[j]->net->driver_pin->node;
			get_max_levels_rec(node, marker_value, netlist->ff_nodes[i]->unique_id, netlist);
			// this yields very different results by transmitting level datapoint to the latch itself 
			/*if(netlist->ff_nodes[i]->stat->depth < node->stat->depth+1){
				netlist->ff_nodes[i]->stat->depth = node->stat->depth+1;
			 }*/
            if(levelMax < (int)node->stat->depth)
               	levelMax = (int)node->stat->depth;
		}
	}
	for (int i = 0; i < (netlist->num_top_output_nodes); i++)
	{
		if (netlist->top_output_nodes[i] != NULL)
		{
			// possibly off by one at PO level if not starting at input node of PO (need 2 extra loops for this)
			nnode_t *node = netlist->top_output_nodes[i];
			get_max_levels_rec(node, marker_value, netlist->top_output_nodes[i]->unique_id, netlist);
			if(levelMax < (int)node->stat->depth)
			{
				levelMax = (int)node->stat->depth;
			}
			// only makes sense if latch lvl set and node is the PO's input node
			//if(netlist->top_output_nodes[i]->stat->depth < node->stat->depth+1)
				//netlist->top_output_nodes[i]->stat->depth = (node->stat->depth+1);
        }
	}
	return levelMax;
}

int get_max_levels_rec(nnode_t *node, int traverse_mark_number, int cycleId, netlist_t *netlist)
{
	nnode_t *next;
    int level = 0;
	if(!node->stat)
	{
		node->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
	}
	if(node->unique_id == cycleId && node->type != OUTPUT_NODE) // indicates cycle, has little effect and choice of which edge is cycle somewhat arbitrary
	{
		sequentialCycles = true;
		//	return -1;
	}
	if(node->type == CLOCK_NODE)
		return -1;

	if(node->type == INPUT_NODE || node->type == FF_NODE || /* maybe remove ->*/ node->type == VCC_NODE || node->type == GND_NODE || node->type == PAD_NODE)
	{
		return node->stat->depth;
	}
	if (node->traverse_visited == traverse_mark_number)
	{
		return node->stat->depth;
	}
	node->traverse_visited = traverse_mark_number;
	node->stat->depth = 0;
	for (int i = 0; i < node->num_input_pins; i++)
	{
		next = node->input_pins[i]->net->driver_pin->node;
		level = get_max_levels_rec(next, traverse_mark_number, cycleId, netlist);
		//If cycle found, mark the pin, did not work as intended and unclear whether cycles caused bad path values
		/*if(level == -1)
			node->input_pins[i]->cycle = true;
		else node->input_pins[i]->cycle = false; */
        if(node->stat->depth < level)
            node->stat->depth = level;
    }
	node->stat->depth++;
    return node->stat->depth;
}

/*-------------------------------------------------------------------
 * (function: node_profile)
 * 	Debugging Tool, print basic or verbose stats about a node
 *------------------------------------------------------------------*/
void node_profile(nnode_t *node, bool verbose, FILE *out)
{
	if(!node) return;
	int total_fanout = 0;
	for(int i=0; i<node->num_output_pins; i++)
	{
		if(!node->output_pins[i]->net)
			continue;
		total_fanout += node->output_pins[i]->net->num_fanout_pins;
	}
	fprintf(out, "node_name: %s ID: %ld type: %d nOut_pins: %ld nFI: %ld nFO: %d ", node->name, node->unique_id, node->type, node->num_output_pins, node->num_input_pins, total_fanout);
	if(verbose && node->stat)
		fprintf(out, "shortest_path_out: %u longest_path_out: %u avg_max_path: %f", node->stat->min_dist_to_out, node->stat->max_dist_to_out, node->stat->avg_max_dist);
	fprintf(out,"\n");
}

/*-------------------------------------------------------------------
 * (function: get_node_fanout)
 * 	for collecting vcc, gnd node fanout without interference
 *------------------------------------------------------------------*/
int get_node_fanout(nnode_t *node)
{
	if(!node) 
		return 0;
	int total_fanout = 0;
	bool incr_counts = false;
	if(node->type == VCC_NODE || node->type == GND_NODE || node->type == PAD_NODE)
	{
		incr_counts = true;
		output_pin_total += node->num_output_pins; // count output pins which are almost certainly == num_nets if pin points to nothing (unlikely)
	}
	for(int i=0; i<node->num_output_pins; i++)
	{
		if(!node->output_pins[i]->net)
			continue;
		if(incr_counts)
			num_nets++; // count live nets just in case output pins are not reflective of net count (possibly off by one bc of pad)
		total_fanout += node->output_pins[i]->net->num_fanout_pins;
	}
	return total_fanout;
}

/* This is similar to the level finding function except in reverse, starting at PIs / LOs (CIs) and traversing downwards recursively
	Additionally there are some experimental changes that maybe should be removed if they can't be verified as accurate or useful
	Specifically, each PI traversal increments the mark number, thus traversing as if no nodes have been visited. The hope was that this might give good path length data.
	Unable to verify if this is the case. Incrementing traversal ID for FFs as well gives very different results with larger level counts for PIs.
	Additionally reverse_lvl_rec detects cycles in latches and breaks traversal if one is detected, disregarding invalid purely cyclical paths. Also FF nodes get updated lvl values.
	Another approach would be to have 3 different travIDs, one for current, one for prev and one for never visited, this can detect combinational loops.
*/
/*int reverse_lvl(short marker_value, netlist_t *netlist){
	int levelMax = 0;
	short traversalInc = 50; // Experimental: traversal mark increments for each PI
	for (int i = 0; i < netlist->num_top_output_nodes; i++)
	{
		if (netlist->top_output_nodes[i] != NULL)
		{
			if(!netlist->top_output_nodes[i]->stat)
				netlist->top_output_nodes[i]->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
			netlist->top_output_nodes[i]->stat->depth = 0;
		}
	}
	 for (int i = 0; i < netlist->num_ff_nodes; i++)
	{
		if (netlist->ff_nodes[i] != NULL)
		{
			if(!netlist->ff_nodes[i]->stat)
				netlist->ff_nodes[i]->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
			netlist->ff_nodes[i]->stat->depth = 0;
		}
	}
	for (int i = 0; i < netlist->num_ff_nodes; i++)
	{
		for(int j=0; j < netlist->ff_nodes[i]->num_output_pins; j++)
		{
			for(int k=0; k < netlist->ff_nodes[i]->output_pins[j]->net->num_fanout_pins; k++){
				nnode_t *node = netlist->ff_nodes[i]->output_pins[j]->net->fanout_pins[k]->node;
				reverse_lvl_rec(node, marker_value, netlist->ff_nodes[i]->unique_id, netlist);
				//reverse_lvl_rec(node, traversalInc, netlist->ff_nodes[i]->unique_id, netlist); // EXPERIMENTAL
				if(netlist->ff_nodes[i]->stat->depth < node->stat->depth+1){
					netlist->ff_nodes[i]->stat->depth = node->stat->depth+1;
				}
            	if(levelMax < node->stat->depth ){
                	levelMax = node->stat->depth;
				}
				traversalInc++; // experimental
			}
		}
	}
	for (int i = 0; i < (netlist->num_top_input_nodes); i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			nnode_t *node = netlist->top_input_nodes[i];
			reverse_lvl_rec(node, marker_value, netlist->top_input_nodes[i]->unique_id, netlist);
			//reverse_lvl_rec(node, traversalInc, netlist->top_input_nodes[i]->unique_id, netlist); // experimental
			if(levelMax < node->stat->depth){
				levelMax = node->stat->depth;
			}
        }
		traversalInc++; // experimental
	}
	return levelMax;
}

int reverse_lvl_rec(nnode_t *node, int traverse_mark_number, int cycleId, netlist_t *netlist){
	nnode_t *next;
    int level = 0;
	if(!node->stat){
		node->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
	}
	if(node->unique_id == cycleId && node->type != INPUT_NODE){ // cycle detected
		return -1;
	}
	if(node->type == OUTPUT_NODE || node->type == FF_NODE || node->type == CLOCK_NODE || node->type == VCC_NODE || node->type == GND_NODE || node->type == PAD_NODE){
		return node->stat->depth;
	}
	if (node->traverse_visited == traverse_mark_number)
	{
		return node->stat->depth;
	}
	node->traverse_visited = traverse_mark_number;
	node->stat->depth = 0;
	bool validPath = false;
	for (int i = 0; i < node->num_output_pins; i++){
		for(int j = 0; j < node->output_pins[i]->net->num_fanout_pins; j++){
			next = node->output_pins[i]->net->fanout_pins[j]->node;
			level = reverse_lvl_rec(next, traverse_mark_number, cycleId, netlist);
			if(level >= 0)
				validPath = true;
			else
				continue;
        	if(node->stat->depth < level)
            	node->stat->depth = level;
		}
    }
	if(!validPath){
		node->stat->depth = -1; // mark invalid path, this doesn't do as much as hoped as it only spots complete deadend cycle paths
	}
    else
		node->stat->depth++;
    return node->stat->depth;
}
*/

/*-------------------------------------------------------------------
 * (function: set_path_lengths)
 * This basically does what the path calculations do but in isolation, ideally the other node properties should be separated from path length stuff because you need to
 * unmark the nodes after each top input traversal. In combination with dfs_clear this produces consistent path length results that are independent of which
 * input is evaluated first.
 *------------------------------------------------------------------*/
void set_path_lengths(nnode_t *node, short marker)
{
	num_traversals_stat++;
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	
	if(node->traverse_visited == marker)
	{
		// The idea here is to fill in gaps when a node has been left unset but visited because of a cycle, it looks ahead one to see if its neighbours have been since set
		if(node->stat->min_dist_to_out+1 == 0)
		{
			single_node_lookahead(node, node->stat->traversal_id+1, marker);
		}
		return;
	}
	if(!node->stat)
	{
		node->stat = (nnode_stats_t *) vtr::calloc(1, sizeof(nnode_stats_t));
		node->stat->traversal_id = 0;
	}
	node->stat->max_dist_to_out = 0;
	node->stat->min_dist_to_out = 0-1;
	unsigned int sum_dist = 0;
	node->stat->avg_max_dist = 0;
	unsigned int total_node_fanout = 0;
	node->traverse_visited = marker;
	if(node->type == OUTPUT_NODE)
	{
		node->stat->min_dist_to_out = node->stat->max_dist_to_out = 0;
		return;
	}
	node->stat->traversal_id++;
	for (int i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_pins[i]->net == NULL)
			continue;
		next_net = node->output_pins[i]->net;
		next_net->traversal_id++;
		total_node_fanout += next_net->num_fanout_pins;
		for (int j = 0; j < next_net->num_fanout_pins; j++)
		{
			if(next_net->fanout_pins[j] == NULL) // this happens with .v input sometimes
				continue;
			next_node = next_net->fanout_pins[j]->node;
			if(next_node == NULL)
				continue;
			unsigned int test_max;
			unsigned int test_min;
			// if on path, skip
			if(next_node->traverse_visited == marker && next_node->stat->traversal_id == next_net->traversal_id)
			{
				continue;
			}
			set_path_lengths(next_node, marker);
			test_min = next_node->stat->min_dist_to_out;
			if(test_min+1 == 0)
				continue; // don't bother setting anything if next_node is itself unset
			test_max = next_node->stat->max_dist_to_out+1;
			sum_dist += test_max;
			if(node->stat->min_dist_to_out > test_min)
				node->stat->min_dist_to_out = test_min+1;
			if(node->stat->max_dist_to_out < test_max)
				node->stat->max_dist_to_out = test_max;
		}
		next_net->traversal_id = 0;
	}
	node->stat->traversal_id = 0;
	node->stat->avg_max_dist = (sum_dist == 0) ? 0.0 : ((float)sum_dist / (float)total_node_fanout);
	return;
}

/*-------------------------------------------------------------------
 * (function: pathlen_heuristic)
 * The idea of this is to do a dfs starting at random fanout node and run many times to see if this gives us any kind of indication of what mean path lengths should be.
 * Ideally would probably iterate over the precreding fanout nodes after as well. Over enough iterations the results are consistent.
 * Very very costly to just repeat this many times.	
 *------------------------------------------------------------------*/
int pathlen_heuristic(nnode_t *node, short marker)
{
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	unsigned int max = 0;
	if(node->traverse_visited == marker)
		return 0;
	node->traverse_visited = marker;
	if(node->type == OUTPUT_NODE)
	{
		return 1;
	}
	for (int i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_pins[i]->net == NULL)
			continue;
		next_net = node->output_pins[i]->net;
		int startAt = 0;
		if(next_net->num_fanout_pins > 0)
			startAt = rand() % next_net->num_fanout_pins;
		for (int j = startAt; j < next_net->num_fanout_pins; j++)
		{
			if(next_net->fanout_pins[j] == NULL) // this happens with .v input sometimes
				continue;
			next_node = next_net->fanout_pins[j]->node;
			if(next_node == NULL)
				continue;
			unsigned int test_max = 0;
			test_max = pathlen_heuristic(next_node, marker);
			if(max<test_max)
				max = test_max+1;
		}
	}
	return max;
}

/*-------------------------------------------------------------------
 * (function: single_node_lookahead)
 * If a node is visited but unset then it is possible that the latch that caused the problem has since been set properly.
 *  This won't address if the latch hasn't been set yet.
 *------------------------------------------------------------------*/
void single_node_lookahead(nnode_t *node, short travId, short marker)
{
	nnode_t *next_node = NULL;
	nnet_t *next_net = NULL;
	int sum_max = 0;
	int total_fanout = 0;
	if(node->stat->traversal_id == travId) // no effect?
		return;
	node->stat->traversal_id++;
	for (int i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_pins[i]->net == NULL)
			continue;
		next_net = node->output_pins[i]->net;
		if(next_net->traversal_id == travId)
			continue;
		next_net->traversal_id++;
		for (int j = 0; j < next_net->num_fanout_pins; j++)
		{
			total_fanout++;
			if(next_net->fanout_pins[j] == NULL) // this happens with .v input sometimes
				continue;
			next_node = next_net->fanout_pins[j]->node;
			if(next_node == NULL)
				continue;
			unsigned int test_max;
			unsigned int test_min;
			if(!next_node->stat)
				continue;
			if(next_node->type == OUTPUT_NODE)
			{
				node->stat->min_dist_to_out = 1;
				node->stat->max_dist_to_out = (node->stat->max_dist_to_out < 1) ? 1 : node->stat->max_dist_to_out;
			}
			if(next_node->stat->traversal_id == travId || next_node->traverse_visited != marker) // don't do anything with unvisited nodes
				continue;
			// do this because a chain of MUX_2 nodes can have bad values and may need to look ahead further. (possibly flawed)
			// The end result of this is not very dramatic, fills in the gaps of some unmarked nodes, generally leads to slightly longer path lens when many muxes & FFs in netlist
			// This can get caught in (combinational?) loops, and this marking scheme is generally meant to detect that but we're using it for something else so take only 1 fo pin
			if(/*next_net->num_fanout_pins == 1 && */j == next_net->num_fanout_pins-1 && next_node->stat->min_dist_to_out+1 == 0 && next_node->stat->traversal_id != travId && node->stat->min_dist_to_out+1 == 0)
			{
				single_node_lookahead(next_node, travId, marker);
			}
			test_max = next_node->stat->max_dist_to_out+1;
			test_min = next_node->stat->min_dist_to_out;
			if(test_min+1 == 0)
				continue;
			sum_max += test_max;
			if(node->stat->min_dist_to_out > test_min)
				node->stat->min_dist_to_out = test_min+1;
			if(node->stat->max_dist_to_out < test_max)
				node->stat->max_dist_to_out = test_max;
		}
		next_net->traversal_id = 0; // unclear if this works as intended
	}
	node->stat->traversal_id = 0; // resetting this makes loop possible if second lookahead is not limited to 1 fo pin
	node->stat->avg_max_dist = (float)sum_max / (float)total_fanout;
}