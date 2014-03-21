/* 
Print VPR netlist as blif so that formal equivalence checking can be done to verify that the input blif netlist == the internal VPR netlist

Author: Jason Luu
Date: Oct 14, 2013
 */

#include <cstdio>
#include <cstring>
#include <assert.h>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "print_netlist_as_blif.h"
#include "read_xml_arch_file.h"


#define LINELENGTH 1024
#define TABLENGTH 1

/*****************************************************************************************
* Internal functions declarations
*****************************************************************************************/
static void print_string(const char *str_ptr, int *column, FILE * fpout);
static void print_net_name(int inet, int *column, FILE * fpout);
static void print_logical_block(FILE *fpout, int ilogical_block, t_block *clb);
static void print_routing_in_clusters(FILE *fpout, t_block *clb, int iclb);
static void print_models(FILE *fpout, t_model *user_models);
static void output_blif(const t_arch *arch, t_block *clb, int num_clusters, const char *out_fname);


/******************************************************************************************
* Public functions
******************************************************************************************/

void print_preplace_netlist(INP const t_arch *Arch, INP const char *filename) {
	output_blif(Arch, block, num_blocks, filename);
}

/******************************************************************************************
* Internal functions
******************************************************************************************/

static void print_string(const char *str_ptr, int *column, FILE * fpout) {

	/* Prints string without making any lines longer than LINELENGTH.  Column  *
	* points to the column in which the next character will go (both used and *
	* updated), and fpout points to the output file.                          */

	int len;

	len = strlen(str_ptr);
	if (len + 3 > LINELENGTH) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
			"in print_string: String %s is too long for desired maximum line length.\n", str_ptr);
	}

	if (*column + len + 2 > LINELENGTH) {
		fprintf(fpout, "\\ \n");
		*column = TABLENGTH;
	}

	fprintf(fpout, "%s ", str_ptr);
	*column += len + 1;
}

static void print_net_name(int inet, int *column, FILE * fpout) {

	/* This routine prints out the g_atoms_nlist.net name (or open) and limits the    *
	* length of a line to LINELENGTH characters by using \ to continue *
	* lines.  net_num is the index of the g_atoms_nlist.net to be printed, while     *
	* column points to the current printing column (column is both     *
	* used and updated by this routine).  fpout is the output file     *
	* pointer.                                                         */

	char *str_ptr;

	if (inet == OPEN) {
		str_ptr = new char[5];
		sprintf(str_ptr, "open");
	}
	else {
		str_ptr = new char[strlen(g_atoms_nlist.net[inet].name) + 6];
		sprintf(str_ptr, "__|e%s", g_atoms_nlist.net[inet].name);
	}

	print_string(str_ptr, column, fpout);
	delete str_ptr;
}

/* Print netlist atom in blif format */
static void print_logical_block(FILE *fpout, int ilogical_block, t_block *clb) {
	t_pb_pin_route_stats * pb_pin_route_stats;
	int clb_index;
	t_pb_graph_node *pb_graph_node;
	t_pb_type *pb_type;

	clb_index = logical_block[ilogical_block].clb_index;
	assert(clb_index != OPEN);
	pb_pin_route_stats = clb[clb_index].pb_pin_route_stats;
	assert(pb_pin_route_stats != NULL);
	pb_graph_node = logical_block[ilogical_block].pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;


	if (logical_block[ilogical_block].type == VPACK_INPAD) {
		int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
		fprintf(fpout, ".names %s clb_%d_rr_node_%d\n", logical_block[ilogical_block].name, clb_index, node_index);
		fprintf(fpout, "1 1\n\n");
	}
	else if (logical_block[ilogical_block].type == VPACK_OUTPAD) {
		int node_index = pb_graph_node->input_pins[0][0].pin_count_in_cluster;
		fprintf(fpout, ".names clb_%d_rr_node_%d %s\n", clb_index, node_index, logical_block[ilogical_block].name + 4);
		fprintf(fpout, "1 1\n\n");
	}
	else if (strcmp(logical_block[ilogical_block].model->name, "names") == 0) {
		/* Print a LUT, a LUT has K input pins and one output pin */
		fprintf(fpout, ".names ");
		for (int i = 0; i < pb_type->num_ports; i++) {
			assert(pb_type->num_ports == 2);
			if (pb_type->ports[i].type == IN_PORT) {
				/* LUTs receive special handling because a LUT has logically equivalent inputs.
				The intra-logic block router may have taken advantage of logical equivalence so we need to unscramble the inputs when we output the LUT logic.
				*/
				assert(pb_type->ports[i].is_clock == FALSE);
				for (int j = 0; j < pb_type->ports[i].num_pins; j++) {
					if (logical_block[ilogical_block].input_nets[0][j] != OPEN) {
						int k;
						for (k = 0; k < pb_type->ports[i].num_pins; k++) {
							int node_index = pb_graph_node->input_pins[0][k].pin_count_in_cluster;
							int a_net_index = pb_pin_route_stats[node_index].atom_net_idx;
							if (a_net_index != OPEN) {
								if (a_net_index == logical_block[ilogical_block].input_nets[0][j]) {
									fprintf(fpout, "clb_%d_rr_node_%d ", clb_index, pb_pin_route_stats[node_index].prev_pb_pin_id);
									break;
								}
							}
						}
						if (k == pb_type->ports[i].num_pins) {
							/* Failed to find LUT input, a netlist error has occurred */
							vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
								"LUT %s missing input %s post packing. This is a VPR internal error, report to vpr@eecg.utoronto.ca\n",
								logical_block[ilogical_block].name, g_atoms_nlist.net[logical_block[ilogical_block].input_nets[0][j]].name);
						}
					}
				}
			}
		}
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].output_nets[0][0] == pb_pin_route_stats[node_index].atom_net_idx);
				fprintf(fpout, "clb_%d_rr_node_%d\n", clb_index, node_index);
			}
		}
		struct s_linked_vptr *truth_table = logical_block[ilogical_block].truth_table;
		while (truth_table) {
			fprintf(fpout, "%s\n", (char *)truth_table->data_vptr);
			truth_table = truth_table->next;
		}
	}
	else if (strcmp(logical_block[ilogical_block].model->name, "latch") == 0) {
		/* Print a flip-flop.  A flip-flop has a D input, a Q output, and a clock input */
		fprintf(fpout, ".latch ");
		assert(pb_type->num_ports == 3);
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == IN_PORT
				&& pb_type->ports[i].is_clock == FALSE) {
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].input_nets[0][0] != OPEN);
				int node_index =
					pb_graph_node->input_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d ", clb_index, pb_pin_route_stats[node_index].prev_pb_pin_id);
			}
			else if (pb_type->ports[i].type == OUT_PORT) {
				assert(pb_type->ports[i].num_pins == 1);
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d re ", clb_index, node_index);
			}
			else if (pb_type->ports[i].type == IN_PORT
				&& pb_type->ports[i].is_clock == TRUE) {
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].clock_net != OPEN);
				int node_index = pb_graph_node->clock_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d 2", clb_index, pb_pin_route_stats[node_index].prev_pb_pin_id);
			}
			else {
				assert(0);
			}
		}
		fprintf(fpout, "\n");
	}
	else {
		/* This is a user defined custom block, print based on model */
	}
}

static void print_routing_in_clusters(FILE *fpout, t_block *clb, int iclb) {
	t_pb_pin_route_stats * pb_pin_route_stats;
	t_pb_graph_node *pb_graph_node;
	t_pb_graph_node *pb_graph_node_of_pin;
	int max_pb_graph_pin;
	t_pb_graph_pin** pb_graph_pin_lookup;

	/* print routing of clusters */
	pb_graph_pin_lookup = alloc_and_load_pb_graph_pin_lookup_from_index(clb[iclb].type);
	pb_graph_node = clb[iclb].pb->pb_graph_node;
	max_pb_graph_pin = pb_graph_node->total_pb_pins;
	pb_pin_route_stats = clb[iclb].pb_pin_route_stats;


	for (int i = 0; i < max_pb_graph_pin; i++) {
		if (pb_pin_route_stats[i].atom_net_idx != OPEN) {
			int column = 0;
			pb_graph_node_of_pin = pb_graph_pin_lookup[i]->parent_node;

			/* Print interconnect */
			if (pb_graph_node_of_pin->pb_type->num_modes != 0 && pb_pin_route_stats[i].prev_pb_pin_id == OPEN) {
				/* Logic block input pin */
				assert(pb_graph_node_of_pin->parent_pb_graph_node == NULL);
				fprintf(fpout, ".names ");
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, " clb_%d_rr_node_%d\n", iclb, i);
				fprintf(fpout, "1 1\n\n");
			}
			else if (pb_graph_node_of_pin->pb_type->num_modes != 0 && pb_graph_node_of_pin->parent_pb_graph_node == NULL) {
				/* Logic block output pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d ", iclb, pb_pin_route_stats[i].prev_pb_pin_id);
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, "\n");
				fprintf(fpout, "1 1\n\n");
			}
			else if (pb_graph_node_of_pin->pb_type->num_modes != 0 || pb_graph_pin_lookup[i]->port->type != OUT_PORT) {
				/* Logic block internal pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d clb_%d_rr_node_%d\n", iclb, pb_pin_route_stats[i].prev_pb_pin_id, iclb, i);
				fprintf(fpout, "1 1\n\n");
			}
		}
	}

	free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
}

static void print_models(FILE *fpout, t_model *user_models) {
}

static void output_blif(const t_arch *arch, t_block *clb, int num_clusters, const char *out_fname) {

	/*
	* This routine dumps out the output netlist in a format suitable for  *
	* input to vpr.  This routine also dumps out the internal structure of   *
	* the cluster, in essentially a graph based format.                           */

	FILE *fpout;
	int bnum, column;
	struct s_linked_vptr *p_io_removed;

	if (clb[0].pb_pin_route_stats == NULL) {
		return;
	}

	printf("\n\n\njedit %s\n\n\n", out_fname);

	fpout = my_fopen(out_fname, "w", 0);

	column = 0;
	fprintf(fpout, ".model %s\n", blif_circuit_name);

	/* Print out all input and output pads. */
	fprintf(fpout, "\n.inputs ");
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_INPAD) {
			print_string(logical_block[bnum].name, &column, fpout);
		}
	}
	p_io_removed = circuit_p_io_removed;
	while (p_io_removed) {
		print_string((char*)p_io_removed->data_vptr, &column, fpout);
		p_io_removed = p_io_removed->next;
	}

	column = 0;
	fprintf(fpout, "\n.outputs ");
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_OUTPAD) {
			/* remove output prefix "out:" */
			print_string(logical_block[bnum].name + 4, &column, fpout);
		}
	}

	column = 0;

	fprintf(fpout, "\n\n");

	/* print out all circuit elements */
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		print_logical_block(fpout, bnum, clb);
	}

	for (int clb_index = 0; clb_index < num_clusters; clb_index++) {
		print_routing_in_clusters(fpout, clb, clb_index);
	}

	fprintf(fpout, "\n.end\n");

	print_models(fpout, arch->models);

	fclose(fpout);
}
