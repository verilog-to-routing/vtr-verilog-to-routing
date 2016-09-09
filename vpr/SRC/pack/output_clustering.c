/*
 Jason Luu 2008
 Print complex block information to a file
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "pack_types.h"
#include "cluster_router.h"
#include "output_clustering.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"
#include "vpr_utils.h"
#include "output_blif.h"

#define LINELENGTH 1024
#define TAB_LENGTH 4

//#define OUTPUT_BLIF /* Dump blif representation of logic block for debugging using formal verification */

/****************** Static variables local to this module ************************/

static t_pb_graph_pin ***pb_graph_pin_lookup_from_index_by_type = NULL; /* [0..num_types-1][0..num_pb_graph_pins-1] lookup pointer to pb_graph_pin from pb_graph_pin index */


/**************** Subroutine definitions ************************************/

static void print_string(const char *str_ptr, int *column, int num_tabs, FILE * fpout) {

	/* 
     * Column points to the column in which the next character will go (both 
     * used and updated), and fpout points to the output file.
     */

	int len;

	len = strlen(str_ptr);

	if (*column + len + 2 > LINELENGTH) {
		fprintf(fpout, "\n");
		print_tabs(fpout, num_tabs);
		*column = num_tabs * TAB_LENGTH;
	}

	fprintf(fpout, "%s ", str_ptr);
	*column += len + 1;
}

static void print_net_name(int inet, int *column, int num_tabs, FILE * fpout) {

	/* This routine prints out the g_atoms_nlist.net name (or open).  
     * net_num is the index of the g_atoms_nlist.net to be printed, while     *
	 * column points to the current printing column (column is both     *
	 * used and updated by this routine).  fpout is the output file     *
	 * pointer.                                                         */

	const char *str_ptr;

	if (inet == OPEN)
		str_ptr = "open";
	else
		str_ptr = g_atoms_nlist.net[inet].name;

	print_string(str_ptr, column, num_tabs, fpout);
}

static void print_interconnect(t_type_ptr type, int inode, int *column, int num_tabs, t_pb_route *pb_route, 
		FILE * fpout) {

	char *str_ptr, *name;
	int prev_node, prev_edge;
	int len;


	if (pb_route[inode].atom_net_idx == OPEN) {
		print_string("open", column, num_tabs, fpout);
	} else {
		str_ptr = NULL;
		prev_node = pb_route[inode].prev_pb_pin_id;

		if (prev_node == OPEN) {
			/* No previous driver implies that this is either a top-level input pin or a primitive output pin */
			t_pb_graph_pin *cur_pin = pb_graph_pin_lookup_from_index_by_type[type->index][inode];
			VTR_ASSERT(cur_pin->parent_node->pb_type->parent_mode == NULL || 
					(cur_pin->parent_node->pb_type->num_modes == 0 && cur_pin->port->type == OUT_PORT)
					);
			print_net_name(pb_route[inode].atom_net_idx, column, num_tabs, fpout);
		} else {
			t_pb_graph_pin *cur_pin = pb_graph_pin_lookup_from_index_by_type[type->index][inode];
			t_pb_graph_pin *prev_pin = pb_graph_pin_lookup_from_index_by_type[type->index][prev_node];
			
			for(prev_edge = 0; prev_edge < prev_pin->num_output_edges; prev_edge++) {
				VTR_ASSERT(prev_pin->output_edges[prev_edge]->num_output_pins == 1);
				if(prev_pin->output_edges[prev_edge]->output_pins[0]->pin_count_in_cluster == inode) {
					break;
				}
			}
			VTR_ASSERT(prev_edge < prev_pin->num_output_edges);

			name =	prev_pin->output_edges[prev_edge]->interconnect->name;
			if (prev_pin->port->parent_pb_type->depth
					>= cur_pin->port->parent_pb_type->depth) {
				/* Connections from siblings or children should have an explicit index, connections from parent does not need an explicit index */
				len =
						strlen(
								prev_pin->parent_node->pb_type->name)
								+ prev_pin->parent_node->placement_index
										/ 10
								+ strlen(
										prev_pin->port->name)
								+ prev_pin->pin_number
										/ 10 + strlen(name) + 11;
				str_ptr = (char*)vtr::malloc(len * sizeof(char));
				sprintf(str_ptr, "%s[%d].%s[%d]->%s ",
						prev_pin->parent_node->pb_type->name,
						prev_pin->parent_node->placement_index,
						prev_pin->port->name,
						prev_pin->pin_number, name);
			} else {
				len =
						strlen(
								prev_pin->parent_node->pb_type->name)
								+ strlen(
										prev_pin->port->name)
								+ prev_pin->pin_number
										/ 10 + strlen(name) + 8;
				str_ptr = (char*)vtr::malloc(len * sizeof(char));
				sprintf(str_ptr, "%s.%s[%d]->%s ",
						prev_pin->parent_node->pb_type->name,
						prev_pin->port->name,
						prev_pin->pin_number, name);
			}
			print_string(str_ptr, column, num_tabs, fpout);
		}
		if (str_ptr)
			free(str_ptr);
	}
}

static void print_open_pb_graph_node(t_type_ptr type, t_pb_graph_node * pb_graph_node,
		int pb_index, bool is_used, t_pb_route *pb_route, int tab_depth, FILE * fpout) {
	int column = 0;
	int i, j, k, m;
	const t_pb_type * pb_type, *child_pb_type;
	t_mode * mode = NULL;
	int prev_edge, prev_node;
	int mode_of_edge, port_index, node_index;

	mode_of_edge = UNDEFINED;

	pb_type = pb_graph_node->pb_type;

	print_tabs(fpout, tab_depth);

	if (is_used) {
		/* Determine mode if applicable */
		port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				VTR_ASSERT(!pb_type->ports[i].is_clock);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
					if (pb_type->num_modes > 0
						&& pb_route[node_index].atom_net_idx != OPEN) {
						prev_node = pb_route[node_index].prev_pb_pin_id;
						t_pb_graph_pin *prev_pin = pb_graph_pin_lookup_from_index_by_type[type->index][prev_node];
						for(prev_edge = 0; prev_edge < prev_pin->num_output_edges; prev_edge++) {
							VTR_ASSERT(prev_pin->output_edges[prev_edge]->num_output_pins == 1);
							if(prev_pin->output_edges[prev_edge]->output_pins[0]->pin_count_in_cluster == node_index) {
								break;
							}
						}
						VTR_ASSERT(prev_edge < prev_pin->num_output_edges);
						mode_of_edge =
								prev_pin->output_edges[prev_edge]->interconnect->parent_mode_index;
						VTR_ASSERT(
								mode == NULL || &pb_type->modes[mode_of_edge] == mode);
						VTR_ASSERT(mode_of_edge == 0); /* for now, unused blocks must always default to use mode 0 */
						mode = &pb_type->modes[mode_of_edge];
					}
				}
				port_index++;
			}
		}

		VTR_ASSERT(mode != NULL && mode_of_edge != UNDEFINED);
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
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route,
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
				VTR_ASSERT(!pb_type->ports[i].is_clock);
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					node_index =
							pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route,
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
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route,
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
					is_used = false;
					for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
						if (child_pb_type->ports[k].type == OUT_PORT) {
							for (m = 0; m < child_pb_type->ports[k].num_pins;
									m++) {
								node_index =
										pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j].output_pins[port_index][m].pin_count_in_cluster;
								if (pb_route[node_index].atom_net_idx != OPEN) {
									is_used = true;
									break;
								}
							}
							port_index++;
						}
					}
					print_open_pb_graph_node(type, 
							&pb_graph_node->child_pb_graph_nodes[mode_of_edge][i][j],
							j, is_used, pb_route, tab_depth + 1, fpout);
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

static void print_pb(FILE *fpout, t_type_ptr type, t_pb * pb, int pb_index, t_pb_route *pb_route, int tab_depth) {

	int column;
	int i, j, k, m;
	const t_pb_type *pb_type, *child_pb_type;
	t_pb_graph_node *pb_graph_node;
	t_mode *mode;
	int port_index, node_index;
	bool is_used;

	
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
					print_net_name(pb_route[node_index].atom_net_idx, &column,
							tab_depth, fpout);
				} else {
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route,
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
			VTR_ASSERT(!pb_type->ports[i].is_clock);
			print_tabs(fpout, tab_depth);
			fprintf(fpout, "\t\t<port name=\"%s\">",
					pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index =
						pb->pb_graph_node->output_pins[port_index][j].pin_count_in_cluster;
				print_interconnect(type, node_index, &column, tab_depth + 2, pb_route, fpout);
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
					print_net_name(pb_route[node_index].atom_net_idx, &column,
							tab_depth, fpout);
				} else {
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route,
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
					print_pb(fpout, type, &pb->child_pbs[i][j], j, pb_route, tab_depth + 1);
				} else {
					is_used = false;
					child_pb_type = &mode->pb_type_children[i];
					port_index = 0;

					for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
						if (child_pb_type->ports[k].type == OUT_PORT) {
							for (m = 0; m < child_pb_type->ports[k].num_pins;
									m++) {
								node_index =
										pb_graph_node->child_pb_graph_nodes[pb->mode][i][j].output_pins[port_index][m].pin_count_in_cluster;
								if (pb_route[node_index].atom_net_idx != OPEN) {
									is_used = true;
									break;
								}
							}
							port_index++;
						}
					}
					print_open_pb_graph_node(type, 
							&pb_graph_node->child_pb_graph_nodes[pb->mode][i][j],
							j, is_used, pb_route, tab_depth + 1, fpout);
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
		/* TODO: Must do check that total CLB pins match top-level pb pins, perhaps check this earlier? */
		if(clb[icluster].pb_route != NULL) {
			print_pb(fpout, clb[icluster].type, clb[icluster].pb, icluster, clb[icluster].pb_route, 1);
		} else {
			print_pb(fpout, clb[icluster].type, clb[icluster].pb, icluster, NULL, 1);
		}
	}
}

static void print_stats(t_block *clb, int num_clusters) {

	/* Prints out one cluster (clb).  Both the external pins and the *
	 * internal connections are printed out.                         */

	int ipin, icluster, itype;/*, iblk;*/
	unsigned int inet;
	/*int unabsorbable_ffs;*/
	int total_nets_absorbed;
	bool * nets_absorbed;

	int *num_clb_types, *num_clb_inputs_used, *num_clb_outputs_used;

	nets_absorbed = NULL;
	num_clb_types = num_clb_inputs_used = num_clb_outputs_used = NULL;

	num_clb_types = (int*) vtr::calloc(num_types, sizeof(int));
	num_clb_inputs_used = (int*) vtr::calloc(num_types, sizeof(int));
	num_clb_outputs_used = (int*) vtr::calloc(num_types, sizeof(int));


	nets_absorbed = (bool *) vtr::calloc(g_atoms_nlist.net.size(), sizeof(bool));
	for (inet = 0; inet < g_atoms_nlist.net.size(); inet++) {
		nets_absorbed[inet] = true;
	}

#if 0

/*counting number of flipflops which cannot be absorbed to check the optimality of the packer wrt CLB density*/

	unabsorbable_ffs = 0;
	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (strcmp(logical_block[iblk].model->name, "latch") == 0) {
			if (g_atoms_nlist.net[logical_block[iblk].input_nets[0][0]].num_sinks() > 1
					|| strcmp(
							logical_block[g_atoms_nlist.net[logical_block[iblk].input_nets[0][0]].pins[0].block].model->name,
							"names") != 0) {
				unabsorbable_ffs++;
			}
		}
	}
	vtr::printf_info("\n");
	vtr::printf_info("%d FFs in input netlist not absorbable (ie. impossible to form BLE).\n", unabsorbable_ffs);
#endif

	/* Counters used only for statistics purposes. */

	for (icluster = 0; icluster < num_clusters; icluster++) {
		for (ipin = 0; ipin < clb[icluster].type->num_pins; ipin++) {
			if (clb[icluster].pb_route == NULL) {
				if (clb[icluster].nets[ipin] != OPEN) {
					nets_absorbed[clb[icluster].nets[ipin]] = false;
					if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== RECEIVER) {
						num_clb_inputs_used[clb[icluster].type->index]++;
					}
					else if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== DRIVER) {
						num_clb_outputs_used[clb[icluster].type->index]++;
					}
				}
			}
			else {
				int pb_graph_pin_id = get_pb_graph_node_pin_from_block_pin(icluster, ipin)->pin_count_in_cluster;
				int atom_net_idx = clb[icluster].pb_route[pb_graph_pin_id].atom_net_idx;
				if (atom_net_idx != OPEN) {
					nets_absorbed[atom_net_idx] = false;
					if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== RECEIVER) {
						num_clb_inputs_used[clb[icluster].type->index]++;
					}
					else if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type
						== DRIVER) {
						num_clb_outputs_used[clb[icluster].type->index]++;
					}
				}
			}
		}
		num_clb_types[clb[icluster].type->index]++;
	}

	for (itype = 0; itype < num_types; itype++) {
		if (num_clb_types[itype] == 0) {
			vtr::printf_info("\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
					type_descriptors[itype].name, num_clb_types[itype], 0.0, 0.0);
		} else {
			vtr::printf_info("\t%s: # blocks: %d, average # input + clock pins used: %g, average # output pins used: %g\n",
					type_descriptors[itype].name, num_clb_types[itype],
					(float) num_clb_inputs_used[itype] / (float) num_clb_types[itype],
					(float) num_clb_outputs_used[itype] / (float) num_clb_types[itype]);
		}
	}

	total_nets_absorbed = 0;
	for (inet = 0; inet < g_atoms_nlist.net.size(); inet++) {
		if (nets_absorbed[inet] == true) {
			total_nets_absorbed++;
		}
	}
	vtr::printf_info("Absorbed logical nets %d out of %d nets, %d nets not absorbed.\n",
			total_nets_absorbed, (int)g_atoms_nlist.net.size(), (int)g_atoms_nlist.net.size() - total_nets_absorbed);
	free(nets_absorbed);
	free(num_clb_types);
	free(num_clb_inputs_used);
	free(num_clb_outputs_used);
	/* TODO: print more stats */
}

void output_clustering(const t_arch *arch, t_block *clb, int num_clusters, const vector < vector <t_intra_lb_net> * > &intra_lb_routing, bool global_clocks,
		bool * is_clock, const char *out_fname, bool skip_clustering) {

	/* 
	 * This routine dumps out the output netlist in a format suitable for  *
	 * input to vpr.  This routine also dumps out the internal structure of   *
	 * the cluster, in essentially a graph based format.                           */

	FILE *fpout;
	int bnum, column;
	unsigned netnum;

	if(!intra_lb_routing.empty()) {
		VTR_ASSERT((int)intra_lb_routing.size() == num_clusters);
		for(int icluster = 0; icluster < num_clusters; icluster++) {
			clb[icluster].pb_route = alloc_and_load_pb_route(intra_lb_routing[icluster], clb[icluster].pb->pb_graph_node);
		}
	}

	pb_graph_pin_lookup_from_index_by_type = new t_pb_graph_pin **[num_types];
	for(int itype = 0; itype < num_types; itype++) {
		pb_graph_pin_lookup_from_index_by_type[itype] = alloc_and_load_pb_graph_pin_lookup_from_index(&type_descriptors[itype]);
	}

	
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

		for (netnum = 0; netnum < g_atoms_nlist.net.size(); netnum++) {
			if (is_clock[netnum]) {
				print_string(g_atoms_nlist.net[netnum].name, &column, 2, fpout);
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
				VTR_ASSERT(0);
			}
			break;

		case VPACK_EMPTY:
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, 
					"in output_netlist: logical_block %d is VPACK_EMPTY.\n",
					bnum);
			break;

		default:
			vtr::printf_error(__FILE__, __LINE__, 
					"in output_netlist: Unexpected type %d for logical_block %d.\n", 
					logical_block[bnum].type, bnum);
		}
	}

	if (skip_clustering == false)
		print_clusters(clb, num_clusters, fpout);

	fprintf(fpout, "</block>\n\n");

	fclose(fpout);

#ifdef OUTPUT_BLIF
		output_blif (arch, clb, num_clusters, getEchoFileName(E_ECHO_POST_PACK_NETLIST));
#endif

	print_stats(clb, num_clusters);
	if(!intra_lb_routing.empty()) {
		for(int icluster = 0; icluster < num_clusters; icluster++) {
			free_pb_route(clb[icluster].pb_route);
			clb[icluster].pb_route = NULL;
		}
	}

	for(int itype = 0; itype < num_types; itype++) {
		free_pb_graph_pin_lookup_from_index (pb_graph_pin_lookup_from_index_by_type[itype]);
	}
	delete[] pb_graph_pin_lookup_from_index_by_type;
}
