/*
 Jason Luu 2008
 Print blif representation of circuit
 Assumptions: Assumes first valid rr input to node is the correct rr input
 Assumes clocks are routed globally

 Limitations: [jluu] Due to ABC requiring that all blackbox inputs be named exactly the same in the netlist to be checked as in the input netlist,
 I am forced to skip all internal connectivity checking for inputs going into subckts.  If in the future, ABC no longer treats
 blackboxes as primariy I/Os, then we should revisit this and make it so that we can do formal equivalence on a more representative netlist.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "output_blif.h"
#include "ReadOptions.h"
#include "vpr_utils.h"

#define LINELENGTH 1024
#define TABLENGTH 1

/****************** Subroutines local to this module ************************/

/**************** Subroutine definitions ************************************/

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
		str_ptr = new char [6];
		sprintf(str_ptr, "open");
	}
	else {
		str_ptr = new char[strlen(g_atoms_nlist.net[inet].name) + 7];
		sprintf(str_ptr, "__|e%s", g_atoms_nlist.net[inet].name);
	}

	print_string(str_ptr, column, fpout);
	delete [] str_ptr;
}

/* Print netlist atom in blif format */
void print_logical_block(FILE *fpout, int ilogical_block, t_block *clb) {
	t_pb_pin_route_stats * pb_pin_route_stats;
	int clb_index;
	t_pb_graph_node *pb_graph_node;
	t_pb_type *pb_type;

	clb_index = logical_block[ilogical_block].clb_index;
	assert (clb_index != OPEN);
	pb_pin_route_stats = clb[clb_index].pb_pin_route_stats;
	assert(pb_pin_route_stats != NULL);
	pb_graph_node = logical_block[ilogical_block].pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;


	if(logical_block[ilogical_block].type == VPACK_INPAD) {
		int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
		fprintf(fpout, ".names %s clb_%d_rr_node_%d\n", logical_block[ilogical_block].name, clb_index, node_index);
		fprintf(fpout, "1 1\n\n");
	}
	else if (logical_block[ilogical_block].type == VPACK_OUTPAD) {
		int node_index = pb_graph_node->input_pins[0][0].pin_count_in_cluster;

		if (strcmp(g_atoms_nlist.net[logical_block[ilogical_block].input_nets[0][0]].name,
			logical_block[ilogical_block].name + 4) != 0) {
			fprintf(fpout, ".names clb_%d_rr_node_%d %s\n", clb_index, node_index, logical_block[ilogical_block].name + 4);
			fprintf(fpout, "1 1\n\n");
		}
	}
	else if(strcmp(logical_block[ilogical_block].model->name, "names") == 0) {
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
							if(a_net_index != OPEN) {
								if(a_net_index == logical_block[ilogical_block].input_nets[0][j]) {
									fprintf(fpout, "clb_%d_rr_node_%d ", clb_index, pb_pin_route_stats[node_index].prev_pb_pin_id);
									break;
								}
							}
						}
						if(k == pb_type->ports[i].num_pins) {
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
				fprintf(fpout, "%s\n", logical_block[ilogical_block].name);
			}
		}
		struct s_linked_vptr *truth_table = logical_block[ilogical_block].truth_table;
		while (truth_table) {
			fprintf(fpout, "%s\n", (char *) truth_table->data_vptr);
			truth_table = truth_table->next;
		}


		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].output_nets[0][0] == pb_pin_route_stats[node_index].atom_net_idx);
				fprintf(fpout, "\n.names %s clb_%d_rr_node_%d\n", logical_block[ilogical_block].name, clb_index, node_index);
				fprintf(fpout, "1 1\n");
			}
		}
	} else if (strcmp(logical_block[ilogical_block].model->name, "latch") == 0) {
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
				fprintf(fpout, "clb_%d_rr_node_%d ", clb_index,	pb_pin_route_stats[node_index].prev_pb_pin_id);
			} else if (pb_type->ports[i].type == OUT_PORT) {
				assert(pb_type->ports[i].num_pins == 1);
				fprintf(fpout, "%s re ", g_atoms_nlist.net[logical_block[ilogical_block].output_nets[0][0]].name);
			} else if (pb_type->ports[i].type == IN_PORT
					&& pb_type->ports[i].is_clock == TRUE) {
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].clock_net != OPEN);
				int node_index = pb_graph_node->clock_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d 2", clb_index, pb_pin_route_stats[node_index].prev_pb_pin_id);
			} else {
				assert(0);
			}
		}
		fprintf(fpout, "\n");
		for (int i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[ilogical_block].output_nets[0][0] == pb_pin_route_stats[node_index].atom_net_idx);
				fprintf(fpout, "\n.names %s clb_%d_rr_node_%d\n", g_atoms_nlist.net[logical_block[ilogical_block].output_nets[0][0]].name, clb_index, node_index);
				fprintf(fpout, "1 1\n");
			}
		}
	} else {
		t_model *cur = logical_block[ilogical_block].model;
		fprintf(fpout, ".subckt %s \\\n", cur->name);

		/* Print input ports */
		t_model_ports *port = cur->inputs;
		
		while (port != NULL) {
			for (int i = 0; i < port->size; i++) {
				if (port->is_clock == TRUE) {
					assert(port->index == 0);
					assert(port->size == 1);
					if (logical_block[ilogical_block].clock_pin_name != NULL) {
						fprintf(fpout, "%s[%d]=%s ", port->name, i, logical_block[ilogical_block].clock_pin_name);
					}
					else {
						fprintf(fpout, "%s[%d]=unconn ", port->name, i);
					}
				}
				else{
					if (logical_block[ilogical_block].input_pin_names != NULL && logical_block[ilogical_block].input_pin_names[port->index] != NULL && logical_block[ilogical_block].input_pin_names[port->index][i] != NULL) {
						fprintf(fpout, "%s[%d]=%s ", port->name, i, logical_block[ilogical_block].input_pin_names[port->index][i]);
					}
					else {
						fprintf(fpout, "%s[%d]=unconn ", port->name, i);
					}
				}
				if (i % 4 == 3) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			fprintf(fpout, "\\\n");
		}
		/* Print input ports */
		port = cur->outputs;

		while (port != NULL) {
			for (int i = 0; i < port->size; i++) {
				fprintf(fpout, "%s[%d]=%s ", port->name, i, logical_block[ilogical_block].output_pin_names[port->index][i]);
				
				if (i % 4 == 3) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != NULL) {
				fprintf(fpout, "\\\n");
			}
		}
		fprintf(fpout, "\n");	
		fprintf(fpout, "\n");


		if (logical_block[ilogical_block].output_pin_names != NULL) {
			port = cur->outputs;
			while (port != NULL) {
				if (logical_block[ilogical_block].output_pin_names[port->index] != NULL)	{
					for (int ipin = 0; ipin < port->size; ipin++) {
						int net_index = logical_block[ilogical_block].output_nets[port->index][ipin];
						if (net_index != OPEN && logical_block[ilogical_block].output_pin_names[port->index][ipin] != NULL) {
							t_pb_graph_pin *pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(port, ipin, pb_graph_node);
							fprintf(fpout, ".names %s clb_%d_rr_node_%d\n", logical_block[ilogical_block].output_pin_names[port->index][ipin], clb_index, pb_graph_pin->pin_count_in_cluster);
							fprintf(fpout, "1 1\n\n");
						}
					}
				}
				port = port->next;
			}
		}
	}
}

void print_routing_in_clusters(FILE *fpout, t_block *clb, int iclb) {
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


	for(int i = 0; i < max_pb_graph_pin; i++) {
		if(pb_pin_route_stats[i].atom_net_idx != OPEN) {
			int column = 0;
			pb_graph_node_of_pin = pb_graph_pin_lookup[i]->parent_node;

			/* Print interconnect */	
			if(pb_graph_node_of_pin->pb_type->num_modes != 0 && pb_pin_route_stats[i].prev_pb_pin_id == OPEN) {
				/* Logic block input pin */
				assert(pb_graph_node_of_pin->parent_pb_graph_node == NULL);
				fprintf(fpout, ".names ");
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, " clb_%d_rr_node_%d\n", iclb, i);
				fprintf(fpout, "1 1\n\n");
			} else if (pb_graph_node_of_pin->pb_type->num_modes != 0 && pb_graph_node_of_pin->parent_pb_graph_node == NULL) {
				/* Logic block output pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d ", iclb, pb_pin_route_stats[i].prev_pb_pin_id);
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, "\n");
				fprintf(fpout, "1 1\n\n");
			} else if (pb_graph_node_of_pin->pb_type->num_modes != 0 || pb_graph_pin_lookup[i]->port->type != OUT_PORT) {
				/* Logic block internal pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d clb_%d_rr_node_%d\n", iclb, pb_pin_route_stats[i].prev_pb_pin_id, iclb, i);
				fprintf(fpout, "1 1\n\n");
			}
		}
	}

	free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
}
	
/* Print list of models to file */
void print_models(FILE *fpout, t_model *user_models) {

	t_model *cur;
	cur = user_models;

	while (cur != NULL) {
		fprintf(fpout, "\n");
		fprintf(fpout, ".model %s\n", cur->name);
		
		/* Print input ports */
		t_model_ports *port = cur->inputs;
		if (port == NULL) {
			fprintf(fpout, ".inputs \n");
		}
		else {
			fprintf(fpout, ".inputs \\\n");
		}
		while (port != NULL) {
			for (int i = 0; i < port->size; i++) {
				fprintf(fpout, "%s[%d] ", port->name, i);
				if (i % 8 == 4) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != NULL) {
				fprintf(fpout, "\\\n");
			}
			else {
				fprintf(fpout, "\n");
			}
		}
		/* Print input ports */
		port = cur->outputs;
		if (port == NULL) {
			fprintf(fpout, ".outputs \n");
		}
		else {
			fprintf(fpout, ".outputs \\\n");
		}
		while (port != NULL) {
			for (int i = 0; i < port->size; i++) {
				fprintf(fpout, "%s[%d] ", port->name, i);
				if (i % 8 == 4) {
					if (i + 1 < port->size) {
						fprintf(fpout, "\\\n");
					}
				}
			}
			port = port->next;
			if (port != NULL) {
				fprintf(fpout, "\\\n");
			}
			else {
				fprintf(fpout, "\n");
			}
		}
		fprintf(fpout, ".blackbox\n");
		fprintf(fpout, ".end\n");
		cur = cur->next;
	}
}

void output_blif (const t_arch *arch, t_block *clb, int num_clusters, boolean global_clocks,
		boolean * is_clock, const char *out_fname, boolean skip_clustering) {

	/* 
	 * This routine dumps out the output netlist in a format suitable for  *
	 * input to vpr.  This routine also dumps out the internal structure of   *
	 * the cluster, in essentially a graph based format.                           */

	FILE *fpout;
	int bnum, column;
	struct s_linked_vptr *p_io_removed;
	
	if(clb[0].pb_pin_route_stats == NULL) {
		return;
	}

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
		print_string((char*) p_io_removed->data_vptr, &column, fpout);
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
	fprintf(fpout, ".names unconn\n");
	fprintf(fpout, " 0\n\n");

	/* print out all circuit elements */
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		print_logical_block(fpout, bnum, clb);
	}

	for(int clb_index = 0; clb_index < num_clusters; clb_index++) {
		print_routing_in_clusters(fpout, clb, clb_index);
	}
	
	fprintf(fpout, "\n.end\n");

	print_models(fpout, arch->models);

	fclose(fpout);
}
