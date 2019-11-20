/*
 Jason Luu 2008
 Print complex block information to a file
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "output_clustering.h"
#include "read_xml_arch_file.h"

#define LINELENGTH 1024
#define TAB_LENGTH 4

/****************** Subroutines local to this module ************************/

/**************** Subroutine definitions ************************************/

static void print_tabs(FILE *fpout, int num_tabs) {
	int i;
	for (i = 0; i < num_tabs; i++) {
		fprintf(fpout, "\t");
	}
}

static void print_string(const char *str_ptr, int *column, int num_tabs, FILE * fpout) {

	/* Prints string without making any lines longer than LINELENGTH.  Column  *
	 * points to the column in which the next character will go (both used and *
	 * updated), and fpout points to the output file.                          */

	int len;

	len = strlen(str_ptr);
	if (len + 3 > LINELENGTH) {
		vpr_printf(TIO_MESSAGE_ERROR, "in print_string: String %s is too long for desired maximum line length.\n", str_ptr);
		exit(1);
	}

	if (*column + len + 2 > LINELENGTH) {
		fprintf(fpout, "\n");
		print_tabs(fpout, num_tabs);
		*column = num_tabs * TAB_LENGTH;
	}

	fprintf(fpout, "%s ", str_ptr);
	*column += len + 1;
}

static void print_net_name(int inet, int *column, int num_tabs, FILE * fpout) {

	/* This routine prints out the vpack_net name (or open) and limits the    *
	 * length of a line to LINELENGTH characters by using \ to continue *
	 * lines.  net_num is the index of the vpack_net to be printed, while     *
	 * column points to the current printing column (column is both     *
	 * used and updated by this routine).  fpout is the output file     *
	 * pointer.                                                         */

	const char *str_ptr;

	if (inet == OPEN)
		str_ptr = "open";
	else
		str_ptr = vpack_net[inet].name;

	print_string(str_ptr, column, num_tabs, fpout);
}

static void print_interconnect(int inode, int *column, int num_tabs,
		FILE * fpout) {

	/* This routine prints out the vpack_net name (or open) and limits the    *
	 * length of a line to LINELENGTH characters by using \ to continue *
	 * lines.  net_num is the index of the vpack_net to be printed, while     *
	 * column points to the current printing column (column is both     *
	 * used and updated by this routine).  fpout is the output file     *
	 * pointer.                                                         */

	char *str_ptr, *name;
	int prev_node, prev_edge;
	int len;

	if (rr_node[inode].net_num == OPEN) {
		print_string("open", column, num_tabs, fpout);
	} else {
		str_ptr = NULL;
		prev_node = rr_node[inode].prev_node;
		prev_edge = rr_node[inode].prev_edge;

		if (prev_node == OPEN
				&& rr_node[inode].pb_graph_pin->port->parent_pb_type->num_modes
						== 0
				&& rr_node[inode].pb_graph_pin->port->type == OUT_PORT) { /* This is a primitive output */
			print_net_name(rr_node[inode].net_num, column, num_tabs, fpout);
		} else {
			name =
					rr_node[prev_node].pb_graph_pin->output_edges[prev_edge]->interconnect->name;
			if (rr_node[prev_node].pb_graph_pin->port->parent_pb_type->depth
					>= rr_node[inode].pb_graph_pin->port->parent_pb_type->depth) {
				/* Connections from siblings or children should have an explicit index, connections from parent does not need an explicit index */
				len =
						strlen(
								rr_node[prev_node].pb_graph_pin->parent_node->pb_type->name)
								+ rr_node[prev_node].pb_graph_pin->parent_node->placement_index
										/ 10
								+ strlen(
										rr_node[prev_node].pb_graph_pin->port->name)
								+ rr_node[prev_node].pb_graph_pin->pin_number
										/ 10 + strlen(name) + 11;
				str_ptr = (char*)my_malloc(len * sizeof(char));
				sprintf(str_ptr, "%s[%d].%s[%d]->%s ",
						rr_node[prev_node].pb_graph_pin->parent_node->pb_type->name,
						rr_node[prev_node].pb_graph_pin->parent_node->placement_index,
						rr_node[prev_node].pb_graph_pin->port->name,
						rr_node[prev_node].pb_graph_pin->pin_number, name);
			} else {
				len =
						strlen(
								rr_node[prev_node].pb_graph_pin->parent_node->pb_type->name)
								+ strlen(
										rr_node[prev_node].pb_graph_pin->port->name)
								+ rr_node[prev_node].pb_graph_pin->pin_number
										/ 10 + strlen(name) + 8;
				str_ptr = (char*)my_malloc(len * sizeof(char));
				sprintf(str_ptr, "%s.%s[%d]->%s ",
						rr_node[prev_node].pb_graph_pin->parent_node->pb_type->name,
						rr_node[prev_node].pb_graph_pin->port->name,
						rr_node[prev_node].pb_graph_pin->pin_number, name);
			}
			print_string(str_ptr, column, num_tabs, fpout);
		}
		if (str_ptr)
			free(str_ptr);
	}
}

static void print_open_pb_graph_node(t_pb_graph_node * pb_graph_node,
		int pb_index, boolean is_used, int tab_depth, FILE * fpout) {
	int column = 0;
	int i, j, k, m;
	const t_pb_type * pb_type, *child_pb_type;
	t_mode * mode = NULL;
	int prev_edge, prev_node;
	t_pb_graph_pin *pb_graph_pin;
	int mode_of_edge, port_index, node_index;

	mode_of_edge = UNDEFINED;

	pb_type = pb_graph_node->pb_type;

	print_tabs(fpout, tab_depth);

	if (is_used) {
		/* Determine mode if applicable */
		port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				assert(!pb_type->ports[i].is_clock);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
					if (pb_type->num_modes > 0
							&& rr_node[node_index].net_num != OPEN) {
						prev_edge = rr_node[node_index].prev_edge;
						prev_node = rr_node[node_index].prev_node;
						pb_graph_pin = rr_node[prev_node].pb_graph_pin;
						mode_of_edge =
								pb_graph_pin->output_edges[prev_edge]->interconnect->parent_mode_index;
						assert(
								mode == NULL || &pb_type->modes[mode_of_edge] == mode);
						mode = &pb_type->modes[mode_of_edge];
					}
				}
				port_index++;
			}
		}

		assert(mode != NULL && mode_of_edge != UNDEFINED);
		fprintf(fpout,
				"<block name=\"open\" instance=\"%s[%d]\" mode=\"%s\">\n",
				pb_graph_node->pb_type->name, pb_index, mode->name);

		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t<inputs>\n");
		port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (!pb_type->ports[i].is_clock
					&& pb_type->ports[i].type == IN_PORT) {
				print_tabs(fpout, tab_depth);
				fprintf(fpout, "\t\t<port name=\"%s\">",
						pb_graph_node->pb_type->ports[i].name);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
					print_interconnect(node_index, &column, tab_depth + 2,
							fpout);
				}
				fprintf(fpout, "</port>\n");
				port_index++;
			}
		}
		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t</inputs>\n");

		column = tab_depth * TAB_LENGTH + 8; /* Next column I will write to. */
		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t<outputs>\n");
		port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				print_tabs(fpout, tab_depth);
				fprintf(fpout, "\t\t<port name=\"%s\">",
						pb_graph_node->pb_type->ports[i].name);
				assert(!pb_type->ports[i].is_clock);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
					print_interconnect(node_index, &column, tab_depth + 2,
							fpout);
				}
				fprintf(fpout, "</port>\n");
				port_index++;
			}
		}
		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t</outputs>\n");

		column = tab_depth * TAB_LENGTH + 8; /* Next column I will write to. */
		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t<clocks>\n");
		port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].is_clock
					&& pb_type->ports[i].type == IN_PORT) {
				print_tabs(fpout, tab_depth);
				fprintf(fpout, "\t\t<port name=\"%s\">",
						pb_graph_node->pb_type->ports[i].name);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
					print_interconnect(node_index, &column, tab_depth + 2,
							fpout);
				}
				fprintf(fpout, "</port>\n");
				port_index++;
			}
		}
		print_tabs(fpout, tab_depth);
		fprintf(fpout, "\t</clocks>\n");

		if (pb_type->num_modes > 0) {
			for (i = 0; i < mode->num_pb_type_children; i++) {
				child_pb_type = &mode->pb_type_children[i];
				for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
					port_index = 0;
					is_used = FALSE;
					for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
						if (child_pb_type->ports[k].type == OUT_PORT) {
							for (m = 0; m < child_pb_type->ports[k].num_pins;
									m++) {
								node_index =
										pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j].output_pins[port_index][m].pin_count_in_cluster;
								if (rr_node[node_index].net_num != OPEN) {
									is_used = TRUE;
									break;
								}
							}
							port_index++;
						}
					}
					print_open_pb_graph_node(
							&pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j],
							j, is_used, tab_depth + 1, fpout);
				}
			}
		}

		print_tabs(fpout, tab_depth);
		fprintf(fpout, "</block>\n");
	} else {
		fprintf(fpout, "<block name=\"open\" instance=\"%s[%d]\"/>\n",
				pb_graph_node->pb_type->name, pb_index);
	}
}

static void print_pb(FILE *fpout, t_pb * pb, int pb_index, int tab_depth) {

	int column;
	int i, j, k, m;
	const t_pb_type *pb_type, *child_pb_type;
	t_pb_graph_node *pb_graph_node;
	t_mode *mode;
	int port_index, node_index;
	boolean is_used;

	pb_type = pb->pb_graph_node->pb_type;
	pb_graph_node = pb->pb_graph_node;
	mode = &pb_type->modes[pb->mode];
	column = tab_depth * TAB_LENGTH + 8; /* Next column I will write to. */
	print_tabs(fpout, tab_depth);
	if (pb_type->num_modes == 0) {
		fprintf(fpout, "<block name=\"%s\" instance=\"%s[%d]\">\n", pb->name,
				pb_type->name, pb_index);
	} else {
		fprintf(fpout, "<block name=\"%s\" instance=\"%s[%d]\" mode=\"%s\">\n",
				pb->name, pb_type->name, pb_index, mode->name);
	}

	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t<inputs>\n");
	port_index = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		if (!pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
			print_tabs(fpout, tab_depth);
			fprintf(fpout, "\t\t<port name=\"%s\">",
					pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index =
						pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
				if (pb_type->parent_mode == NULL) {
					print_net_name(rr_node[node_index].net_num, &column,
							tab_depth, fpout);
				} else {
					print_interconnect(node_index, &column, tab_depth + 2,
							fpout);
				}
			}
			fprintf(fpout, "</port>\n");
			port_index++;
		}
	}
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t</inputs>\n");

	column = tab_depth * TAB_LENGTH + 8; /* Next column I will write to. */
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t<outputs>\n");
	port_index = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		if (pb_type->ports[i].type == OUT_PORT) {
			assert(!pb_type->ports[i].is_clock);
			print_tabs(fpout, tab_depth);
			fprintf(fpout, "\t\t<port name=\"%s\">",
					pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index =
						pb->pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
				print_interconnect(node_index, &column, tab_depth + 2, fpout);
			}
			fprintf(fpout, "</port>\n");
			port_index++;
		}
	}
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t</outputs>\n");

	column = tab_depth * TAB_LENGTH + 8; /* Next column I will write to. */
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t<clocks>\n");
	port_index = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		if (pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT) {
			print_tabs(fpout, tab_depth);
			fprintf(fpout, "\t\t<port name=\"%s\">",
					pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index =
						pb->pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
				if (pb_type->parent_mode == NULL) {
					print_net_name(rr_node[node_index].net_num, &column,
							tab_depth, fpout);
				} else {
					print_interconnect(node_index, &column, tab_depth + 2,
							fpout);
				}
			}
			fprintf(fpout, "</port>\n");
			port_index++;
		}
	}
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "\t</clocks>\n");

	if (pb_type->num_modes > 0) {
		for (i = 0; i < mode->num_pb_type_children; i++) {
			for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
				/* If child pb is not used but routing is used, I must print things differently */
				if ((pb->child_pbs[i] != NULL)
						&& (pb->child_pbs[i][j].name != NULL)) {
					print_pb(fpout, &pb->child_pbs[i][j], j, tab_depth + 1);
				} else {
					is_used = FALSE;
					child_pb_type = &mode->pb_type_children[i];
					port_index = 0;

					for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
						if (child_pb_type->ports[k].type == OUT_PORT) {
							for (m = 0; m < child_pb_type->ports[k].num_pins;
									m++) {
								node_index =
										pb_graph_node->child_pb_graph_nodes[pb->mode][i][j].output_pins[port_index][m].pin_count_in_cluster;
								if (rr_node[node_index].net_num != OPEN) {
									is_used = TRUE;
									break;
								}
							}
							port_index++;
						}
					}
					print_open_pb_graph_node(
							&pb_graph_node->child_pb_graph_nodes[pb->mode][i][j],
							j, is_used, tab_depth + 1, fpout);
				}
			}
		}
	}
	print_tabs(fpout, tab_depth);
	fprintf(fpout, "</block>\n");
}

static void print_clusters(t_block *clb, int num_clusters, FILE * fpout) {

	/* Prints out one cluster (clb).  Both the external pins and the *
	 * internal connections are printed out.                         */

	int icluster;

	for (icluster = 0; icluster < num_clusters; icluster++) {
		rr_node = clb[icluster].pb->rr_graph;

		/* TODO: Must do check that total CLB pins match top-level pb pins, perhaps check this earlier? */

		print_pb(fpout, clb[icluster].pb, icluster, 1);
	}
}

static void print_stats(t_block *clb, int num_clusters) {

	/* Prints out one cluster (clb).  Both the external pins and the *
	 * internal connections are printed out.                         */

	int ipin, icluster, itype, inet;/*, iblk;*/
	/*int unabsorbable_ffs;*/
	int total_nets_absorbed;
	boolean * nets_absorbed;

	int *num_clb_types, *num_clb_inputs_used, *num_clb_outputs_used;

	nets_absorbed = NULL;
	num_clb_types = num_clb_inputs_used = num_clb_outputs_used = NULL;

	num_clb_types = (int*) my_calloc(num_types, sizeof(int));
	num_clb_inputs_used = (int*) my_calloc(num_types, sizeof(int));
	num_clb_outputs_used = (int*) my_calloc(num_types, sizeof(int));


	nets_absorbed = (boolean *) my_calloc(num_logical_nets, sizeof(boolean));
	for (inet = 0; inet < num_logical_nets; inet++) {
		nets_absorbed[inet] = TRUE;
	}

#if 0

/*counting number of flipflops which cannot be absorbed to check the optimality of the packer wrt CLB density*/

	unabsorbable_ffs = 0;
	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (strcmp(logical_block[iblk].model->name, "latch") == 0) {
			if (vpack_net[logical_block[iblk].input_nets[0][0]].num_sinks > 1
					|| strcmp(
							logical_block[vpack_net[logical_block[iblk].input_nets[0][0]].node_block[0]].model->name,
							"names") != 0) {
				unabsorbable_ffs++;
			}
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");
	vpr_printf(TIO_MESSAGE_INFO, "%d FFs in input netlist not absorbable (ie. impossible to form BLE).\n", unabsorbable_ffs);
#endif

	/* Counters used only for statistics purposes. */

	for (icluster = 0; icluster < num_clusters; icluster++) {
		for (ipin = 0; ipin < clb[icluster].type->num_pins; ipin++) {
			if (clb[icluster].nets[ipin] != OPEN) {
				nets_absorbed[clb[icluster].nets[ipin]] = FALSE;
				if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== RECEIVER) {
					num_clb_inputs_used[clb[icluster].type->index]++;
				} else if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== DRIVER) {
					num_clb_outputs_used[clb[icluster].type->index]++;
				}
			}
		}
		num_clb_types[clb[icluster].type->index]++;
	}

	for (itype = 0; itype < num_types; itype++) {
		if (num_clb_types[itype] == 0) {
			vpr_printf(TIO_MESSAGE_INFO, "\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
					type_descriptors[itype].name, num_clb_types[itype], 0.0, 0.0);
		} else {
			vpr_printf(TIO_MESSAGE_INFO, "\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
					type_descriptors[itype].name, num_clb_types[itype],
					(float) num_clb_inputs_used[itype] / (float) num_clb_types[itype],
					(float) num_clb_outputs_used[itype] / (float) num_clb_types[itype]);
		}
	}

	total_nets_absorbed = 0;
	for (inet = 0; inet < num_logical_nets; inet++) {
		if (nets_absorbed[inet] == TRUE) {
			total_nets_absorbed++;
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "Absorbed logical nets %d out of %d nets, %d nets not absorbed.\n",
			total_nets_absorbed, num_logical_nets, num_logical_nets - total_nets_absorbed);
	free(nets_absorbed);
	free(num_clb_types);
	free(num_clb_inputs_used);
	free(num_clb_outputs_used);
	/* TODO: print more stats */
}

void output_clustering(t_block *clb, int num_clusters, boolean global_clocks,
		boolean * is_clock, char *out_fname, boolean skip_clustering) {

	/* 
	 * This routine dumps out the output netlist in a format suitable for  *
	 * input to vpr.  This routine also dumps out the internal structure of   *
	 * the cluster, in essentially a graph based format.                           */

	FILE *fpout;
	int bnum, netnum, column;

	fpout = fopen(out_fname, "w");

	fprintf(fpout, "<block name=\"%s\" instance=\"FPGA_packed_netlist[0]\">\n",
			out_fname);
	fprintf(fpout, "\t<inputs>\n\t\t");

	column = 2 * TAB_LENGTH; /* Organize whitespace to ident data inside block */
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_INPAD) {
			print_string(logical_block[bnum].name, &column, 2, fpout);
		}
	}
	fprintf(fpout, "\n\t</inputs>\n");
	fprintf(fpout, "\n\t<outputs>\n\t\t");

	column = 2 * TAB_LENGTH;
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_OUTPAD) {
			print_string(logical_block[bnum].name, &column, 2, fpout);
		}
	}
	fprintf(fpout, "\n\t</outputs>\n");

	column = 2 * TAB_LENGTH;
	if (global_clocks) {
		fprintf(fpout, "\n\t<clocks>\n\t\t");

		for (netnum = 0; netnum < num_logical_nets; netnum++) {
			if (is_clock[netnum]) {
				print_string(vpack_net[netnum].name, &column, 2, fpout);
			}
		}
		fprintf(fpout, "\n\t</clocks>\n\n");
	}

	/* Print out all input and output pads. */

	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		switch (logical_block[bnum].type) {
		case VPACK_INPAD:
		case VPACK_OUTPAD:
		case VPACK_COMB:
		case VPACK_LATCH:
			if (skip_clustering) {
				assert(0);
			}
			break;

		case VPACK_EMPTY:
			vpr_printf(TIO_MESSAGE_ERROR, "in output_netlist: logical_block %d is VPACK_EMPTY.\n",
					bnum);
			exit(1);
			break;

		default:
			vpr_printf(TIO_MESSAGE_ERROR, "in output_netlist: Unexpected type %d for logical_block %d.\n", 
					logical_block[bnum].type, bnum);
		}
	}

	if (skip_clustering == FALSE)
		print_clusters(clb, num_clusters, fpout);

	fprintf(fpout, "</block>\n\n");

	fclose(fpout);

	print_stats(clb, num_clusters);
}
