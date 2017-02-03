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
#include "atom_netlist.h"
#include "pack_types.h"
#include "cluster_router.h"
#include "output_clustering.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"
#include "vpr_utils.h"

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

static void print_net_name(AtomNetId net_id, int *column, int num_tabs, FILE * fpout) {

	/* This routine prints out the g_atom_nl net name (or open).  
     * net_num is the index of the g_atom_nl net to be printed, while     *
	 * column points to the current printing column (column is both     *
	 * used and updated by this routine).  fpout is the output file     *
	 * pointer.                                                         */

	const char *str_ptr;

	if (!net_id)
		str_ptr = "open";
	else
		str_ptr = g_atom_nl.net_name(net_id).c_str();

	print_string(str_ptr, column, num_tabs, fpout);
}

static void print_interconnect(t_type_ptr type, int inode, int *column, int num_tabs, t_pb_route *pb_route, 
		FILE * fpout) {

	char *str_ptr, *name;
	int prev_node, prev_edge;
	int len;


	if (!pb_route[inode].atom_net_id) {
		print_string("open", column, num_tabs, fpout);
	} else {
		str_ptr = NULL;
		prev_node = pb_route[inode].driver_pb_pin_id;

		if (prev_node == OPEN) {
			/* No previous driver implies that this is either a top-level input pin or a primitive output pin */
			t_pb_graph_pin *cur_pin = pb_graph_pin_lookup_from_index_by_type[type->index][inode];
			VTR_ASSERT(cur_pin->parent_node->pb_type->parent_mode == NULL || 
					(cur_pin->parent_node->pb_type->num_modes == 0 && cur_pin->port->type == OUT_PORT)
					);
			print_net_name(pb_route[inode].atom_net_id, column, num_tabs, fpout);
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
						&& pb_route[node_index].atom_net_id) {
						prev_node = pb_route[node_index].driver_pb_pin_id;
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
								if (pb_route[node_index].atom_net_id) {
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
			fprintf(fpout, "\t\t<port name=\"%s\">", pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
				if (pb_type->parent_mode == NULL) {
					print_net_name(pb_route[node_index].atom_net_id, &column, tab_depth, fpout);
				} else {
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route, fpout);
				}
			}
			fprintf(fpout, "</port>\n");

            //The cluster router may have rotated equivalent pins (e.g. LUT inputs), 
            //record the resulting rotation here so it can be unambigously mapped 
            //back to the atom netlist
            if(pb_type->ports[i].equivalent && pb_type->parent_mode != NULL && pb_type->num_modes == 0) {
                //This is a primitive with equivalent inputs

                AtomBlockId atom_blk = g_atom_nl.find_block(pb->name);
                VTR_ASSERT(atom_blk);

                AtomPortId atom_port = g_atom_nl.find_port(atom_blk, pb_type->ports[i].model_port);

                if(atom_port) { //Port exists (some LUTs may have no input and hence no port in the atom netlist)

                    print_tabs(fpout, tab_depth);
                    fprintf(fpout, "\t\t<port_rotation_map name=\"%s\">", pb_graph_node->pb_type->ports[i].name);

                    std::set<AtomPinId> recorded_pins;

                    for (j = 0; j < pb_type->ports[i].num_pins; j++) {
                        node_index = pb->pb_graph_node->input_pins[port_index][j].pin_count_in_cluster;
                        AtomNetId atom_net = pb_route[node_index].atom_net_id;

                        if(atom_net) {
                            //This physical pin is in use, find the original pin in the atom netlist
                            AtomPinId orig_pin;
                            for(AtomPinId atom_pin : g_atom_nl.port_pins(atom_port)) {
                                if(recorded_pins.count(atom_pin)) continue; //Don't add pins twice

                                AtomNetId atom_pin_net = g_atom_nl.pin_net(atom_pin);

                                if(atom_pin_net == atom_net) {
                                    orig_pin = atom_pin;
                                    break;
                                }
                            }

                            VTR_ASSERT(orig_pin);
                            //The physical pin j, maps to a pin in the atom netlist
                            fprintf(fpout, "%d ", g_atom_nl.pin_port_bit(orig_pin));
                        } else {
                            //The physical pin is disconnected
                            fprintf(fpout, "open ");
                        }
                    }
                    fprintf(fpout, "</port_rotation_map>\n");
                }
            }


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
			fprintf(fpout, "\t\t<port name=\"%s\">", pb_graph_node->pb_type->ports[i].name);
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				node_index = pb->pb_graph_node->clock_pins[port_index][j].pin_count_in_cluster;
				if (pb_type->parent_mode == NULL) {
					print_net_name(pb_route[node_index].atom_net_id, &column, tab_depth, fpout);
				} else {
					print_interconnect(type, node_index, &column, tab_depth + 2, pb_route, fpout);
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
				if ((pb->child_pbs[i] != NULL) && (pb->child_pbs[i][j].name != NULL)) {
					print_pb(fpout, type, &pb->child_pbs[i][j], j, pb_route, tab_depth + 1);
				} else {
					is_used = false;
					child_pb_type = &mode->pb_type_children[i];
					port_index = 0;

					for (k = 0; k < child_pb_type->num_ports && !is_used; k++) {
						if (child_pb_type->ports[k].type == OUT_PORT) {
							for (m = 0; m < child_pb_type->ports[k].num_pins; m++) {
								node_index = pb_graph_node->child_pb_graph_nodes[pb->mode][i][j].output_pins[port_index][m].pin_count_in_cluster;
								if (pb_route[node_index].atom_net_id) {
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
	int total_nets_absorbed;
    std::unordered_map<AtomNetId,bool> nets_absorbed;

	int *num_clb_types, *num_clb_inputs_used, *num_clb_outputs_used;

	num_clb_types = num_clb_inputs_used = num_clb_outputs_used = NULL;

	num_clb_types = (int*) vtr::calloc(num_types, sizeof(int));
	num_clb_inputs_used = (int*) vtr::calloc(num_types, sizeof(int));
	num_clb_outputs_used = (int*) vtr::calloc(num_types, sizeof(int));


    for(auto net_id : g_atom_nl.nets()) {
		nets_absorbed[net_id] = true;
	}

	/* Counters used only for statistics purposes. */

	for (icluster = 0; icluster < num_clusters; icluster++) {
		for (ipin = 0; ipin < clb[icluster].type->num_pins; ipin++) {
			if (clb[icluster].pb_route == NULL) {
				if (clb[icluster].nets[ipin] != OPEN) {
                    int clb_net_idx = clb[icluster].nets[ipin];
                    auto net_id = g_atom_map.atom_net(clb_net_idx);
                    VTR_ASSERT(net_id);
					nets_absorbed[net_id] = false;
					if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type == RECEIVER) {
						num_clb_inputs_used[clb[icluster].type->index]++;
					}
					else if (clb[icluster].type->class_inf[clb[icluster].type->pin_class[ipin]].type == DRIVER) {
						num_clb_outputs_used[clb[icluster].type->index]++;
					}
				}
			}
			else {
				int pb_graph_pin_id = get_pb_graph_node_pin_from_block_pin(icluster, ipin)->pin_count_in_cluster;
				auto atom_net_id = clb[icluster].pb_route[pb_graph_pin_id].atom_net_id;
				if (atom_net_id) {
					nets_absorbed[atom_net_id] = false;
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
    for(auto net_id : g_atom_nl.nets()) {
		if (nets_absorbed[net_id] == true) {
			total_nets_absorbed++;
		}
	}
	vtr::printf_info("Absorbed logical nets %d out of %d nets, %d nets not absorbed.\n",
			total_nets_absorbed, (int)g_atom_nl.nets().size(), (int)g_atom_nl.nets().size() - total_nets_absorbed);
	free(num_clb_types);
	free(num_clb_inputs_used);
	free(num_clb_outputs_used);
	/* TODO: print more stats */
}

void output_clustering(t_block *clb, int num_clusters, const vector < vector <t_intra_lb_net> * > &intra_lb_routing, bool global_clocks,
		const std::unordered_set<AtomNetId>& is_clock, const char *out_fname, bool skip_clustering) {

	/* 
	 * This routine dumps out the output netlist in a format suitable for  *
	 * input to vpr.  This routine also dumps out the internal structure of   *
	 * the cluster, in essentially a graph based format.                           */

	FILE *fpout;
	int column;

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
    for(auto blk_id : g_atom_nl.blocks()) {
		if (g_atom_nl.block_type(blk_id) == AtomBlockType::INPAD) {
			print_string(g_atom_nl.block_name(blk_id).c_str(), &column, 2, fpout);
		}
	}
	fprintf(fpout, "\n\t</inputs>\n");
	fprintf(fpout, "\n\t<outputs>\n\t\t");

	column = 2 * TAB_LENGTH;
    for(auto blk_id : g_atom_nl.blocks()) {
		if (g_atom_nl.block_type(blk_id) == AtomBlockType::OUTPAD) {
			print_string(g_atom_nl.block_name(blk_id).c_str(), &column, 2, fpout);
		}
	}
	fprintf(fpout, "\n\t</outputs>\n");

	column = 2 * TAB_LENGTH;
	if (global_clocks) {
		fprintf(fpout, "\n\t<clocks>\n\t\t");

        for(auto net_id : g_atom_nl.nets()) {
            if(is_clock.count(net_id)) {
				print_string(g_atom_nl.net_name(net_id).c_str(), &column, 2, fpout);
			}
		}
		fprintf(fpout, "\n\t</clocks>\n\n");
	}

	/* Print out all input and output pads. */

    for(auto blk_id : g_atom_nl.blocks()) {
        auto type = g_atom_nl.block_type(blk_id);
		switch (type) {
        case AtomBlockType::INPAD:
        case AtomBlockType::OUTPAD:
        case AtomBlockType::COMBINATIONAL:
        case AtomBlockType::SEQUENTIAL:
			if (skip_clustering) {
				VTR_ASSERT(0);
			}
			break;

		default:
			vtr::printf_error(__FILE__, __LINE__, 
					"in output_netlist: Unexpected type %d for atom block %s.\n", 
					type, g_atom_nl.block_name(blk_id).c_str());
		}
	}

	if (skip_clustering == false)
		print_clusters(clb, num_clusters, fpout);

	fprintf(fpout, "</block>\n\n");

	fclose(fpout);

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
