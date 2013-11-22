/*
 Jason Luu 2008
 Print blif representation of circuit
 Assumptions: Assumes first valid rr input to node is the correct rr input
 Assumes clocks are routed globally
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

	const char *str_ptr;

	if (inet == OPEN)
		str_ptr = "open";
	else
		str_ptr = g_atoms_nlist.net[inet].name;

	print_string(str_ptr, column, fpout);
}

#if 0
static int find_fanin_rr_node(t_pb *cur_pb, enum PORTS type, int rr_node_index) {
	/* finds first fanin rr_node */
	t_pb *parent, *sibling, *child;
	int net_num, irr_node;
	int i, j, k, ichild_type, ichild_inst;
	int hack_empty_route_through;
	t_pb_graph_node *hack_empty_pb_graph_node;

	hack_empty_route_through = OPEN;

	net_num = rr_node[rr_node_index].net_num;

	parent = cur_pb->parent_pb;

	if (net_num == OPEN) {
		return OPEN;
	}

	if (type == IN_PORT) {
		/* check parent inputs for valid connection */
		for (i = 0; i < parent->pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < parent->pb_graph_node->num_input_pins[i]; j++) {
				irr_node =
						parent->pb_graph_node->input_pins[i][j].pin_count_in_cluster;
				if (rr_node[irr_node].net_num == net_num) {
					if (cur_pb->pb_graph_node->pb_type->model
							&& strcmp(
									cur_pb->pb_graph_node->pb_type->model->name,
									MODEL_LATCH) == 0) {
						/* HACK: latches are special becuase LUTs can be set to route-through mode for them 
						 I will assume that the input to a LATCH can always come from a parent input pin
						 this only works for hierarchical soft logic structures that follow LUT -> LATCH design
						 must do it better later
						 */
						return irr_node;
					}
					hack_empty_route_through = irr_node;
					for (k = 0; k < rr_node[irr_node].num_edges; k++) {
						if (rr_node[irr_node].edges[k] == rr_node_index) {
							return irr_node;
						}
					}
				}
			}
		}
		/* check parent clocks for valid connection */
		for (i = 0; i < parent->pb_graph_node->num_clock_ports; i++) {
			for (j = 0; j < parent->pb_graph_node->num_clock_pins[i]; j++) {
				irr_node =
						parent->pb_graph_node->clock_pins[i][j].pin_count_in_cluster;
				if (rr_node[irr_node].net_num == net_num) {
					for (k = 0; k < rr_node[irr_node].num_edges; k++) {
						if (rr_node[irr_node].edges[k] == rr_node_index) {
							return irr_node;
						}
					}
				}
			}
		}
		/* check siblings for connection */
		if (parent) {
			for (ichild_type = 0;
					ichild_type
							< parent->pb_graph_node->pb_type->modes[parent->mode].num_pb_type_children;
					ichild_type++) {
				for (ichild_inst = 0;
						ichild_inst
								< parent->pb_graph_node->pb_type->modes[parent->mode].pb_type_children[ichild_type].num_pb;
						ichild_inst++) {
					if (parent->child_pbs[ichild_type]
							&& parent->child_pbs[ichild_type][ichild_inst].name
									!= NULL) {
						sibling = &parent->child_pbs[ichild_type][ichild_inst];
						for (i = 0;
								i < sibling->pb_graph_node->num_output_ports;
								i++) {
							for (j = 0;
									j
											< sibling->pb_graph_node->num_output_pins[i];
									j++) {
								irr_node =
										sibling->pb_graph_node->output_pins[i][j].pin_count_in_cluster;
								if (rr_node[irr_node].net_num == net_num) {
									for (k = 0; k < rr_node[irr_node].num_edges;
											k++) {
										if (rr_node[irr_node].edges[k]
												== rr_node_index) {
											return irr_node;
										}
									}
								}
							}
						}
					} else {
						/* hack just in case routing is down through an empty cluster */
						hack_empty_pb_graph_node =
								&parent->pb_graph_node->child_pb_graph_nodes[ichild_type][0][ichild_inst];
						for (i = 0;
								i < hack_empty_pb_graph_node->num_output_ports;
								i++) {
							for (j = 0;
									j
											< hack_empty_pb_graph_node->num_output_pins[i];
									j++) {
								irr_node =
										hack_empty_pb_graph_node->output_pins[i][j].pin_count_in_cluster;
								if (rr_node[irr_node].net_num == net_num) {
									for (k = 0; k < rr_node[irr_node].num_edges;
											k++) {
										if (rr_node[irr_node].edges[k]
												== rr_node_index) {
											return irr_node;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	} else {
		assert(type == OUT_PORT);

		/* check children for connection */
		for (ichild_type = 0;
				ichild_type
						< cur_pb->pb_graph_node->pb_type->modes[cur_pb->mode].num_pb_type_children;
				ichild_type++) {
			for (ichild_inst = 0;
					ichild_inst
							< cur_pb->pb_graph_node->pb_type->modes[cur_pb->mode].pb_type_children[ichild_type].num_pb;
					ichild_inst++) {
				if (cur_pb->child_pbs[ichild_type]
						&& cur_pb->child_pbs[ichild_type][ichild_inst].name
								!= NULL) {
					child = &cur_pb->child_pbs[ichild_type][ichild_inst];
					for (i = 0; i < child->pb_graph_node->num_output_ports;
							i++) {
						for (j = 0;
								j < child->pb_graph_node->num_output_pins[i];
								j++) {
							irr_node =
									child->pb_graph_node->output_pins[i][j].pin_count_in_cluster;
							if (rr_node[irr_node].net_num == net_num) {
								for (k = 0; k < rr_node[irr_node].num_edges;
										k++) {
									if (rr_node[irr_node].edges[k]
											== rr_node_index) {
										return irr_node;
									}
								}
								hack_empty_route_through = irr_node;
							}
						}
					}
				}
			}
		}

		/* If not in children, check current pb inputs for valid connection */
		for (i = 0; i < cur_pb->pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < cur_pb->pb_graph_node->num_input_pins[i]; j++) {
				irr_node =
						cur_pb->pb_graph_node->input_pins[i][j].pin_count_in_cluster;
				if (rr_node[irr_node].net_num == net_num) {
					hack_empty_route_through = irr_node;
					for (k = 0; k < rr_node[irr_node].num_edges; k++) {
						if (rr_node[irr_node].edges[k] == rr_node_index) {
							return irr_node;
						}
					}
				}
			}
		}
	}

	/* TODO: Once I find a way to output routing in empty blocks then code should never reach here, for now, return OPEN */
	vpr_printf_info("Use hack in blif dumper (do properly later): connecting net %s #%d for pb %s type %s\n",
			g_atoms_nlist.net[net_num].name, net_num, cur_pb->name,
			cur_pb->pb_graph_node->pb_type->name);

	assert(hack_empty_route_through != OPEN);
	return hack_empty_route_through;
}

static void print_primitive(FILE *fpout, int iblk) {
	t_pb *pb;
	int clb_index;
	int i, j, k, node_index;
	int in_port_index, out_port_index, clock_port_index;
	struct s_linked_vptr *truth_table;
	const t_pb_type *pb_type;

	pb = logical_block[iblk].pb;
	pb_type = pb->pb_graph_node->pb_type;
	clb_index = logical_block[iblk].clb_index;

	if (logical_block[iblk].type == VPACK_INPAD
			|| logical_block[iblk].type == VPACK_OUTPAD) {
		/* do nothing */
	} else if (logical_block[iblk].type == VPACK_LATCH) {
		fprintf(fpout, ".latch ");

		in_port_index = 0;
		out_port_index = 0;
		clock_port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == IN_PORT
					&& pb_type->ports[i].is_clock == FALSE) {
				assert(pb_type->ports[i].num_pins == 1);
				assert(logical_block[iblk].input_nets[i][0] != OPEN);
				node_index =
						pb->pb_graph_node->input_pins[in_port_index][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d ", clb_index,
						find_fanin_rr_node(pb, pb_type->ports[i].type,
								node_index));
				in_port_index++;
			}
		}
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == OUT_PORT) {
				assert(pb_type->ports[i].num_pins == 1 && out_port_index == 0);
				node_index =
						pb->pb_graph_node->output_pins[out_port_index][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d re ", clb_index, node_index);
				out_port_index++;
			}
		}
		for (i = 0; i < pb_type->num_ports; i++) {
			if (pb_type->ports[i].type == IN_PORT
					&& pb_type->ports[i].is_clock == TRUE) {
				assert(logical_block[iblk].clock_net != OPEN);
				node_index =
						pb->pb_graph_node->clock_pins[clock_port_index][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d 2", clb_index,
						find_fanin_rr_node(pb, pb_type->ports[i].type,
								node_index));
				clock_port_index++;
			}
		}
		fprintf(fpout, "\n");
	} else if (logical_block[iblk].type == VPACK_COMB) {
		if (strcmp(logical_block[iblk].model->name, "names") == 0) {
			fprintf(fpout, ".names ");
			in_port_index = 0;
			out_port_index = 0;
			for (i = 0; i < pb_type->num_ports; i++) {
				if (pb_type->ports[i].type == IN_PORT
						&& pb_type->ports[i].is_clock == FALSE) {
					/* This is a LUT
					   LUTs receive special handling because a LUT has logically equivalent inputs.
					   The intra-logic block router may have taken advantage of logical equivalence so we need to unscramble the inputs when we output the LUT logic.
					*/
					for (j = 0; j < pb_type->ports[i].num_pins; j++) {
						if (logical_block[iblk].input_nets[in_port_index][j] != OPEN) {
							for (k = 0; k < pb_type->ports[i].num_pins; k++) {								
								node_index = pb->pb_graph_node->input_pins[in_port_index][k].pin_count_in_cluster;
								if(rr_node[node_index].net_num != OPEN) {
									if(rr_node[node_index].net_num == logical_block[iblk].input_nets[in_port_index][j]) {
										fprintf(fpout, "clb_%d_rr_node_%d ", clb_index,
										find_fanin_rr_node(pb,
												pb_type->ports[i].type,
												node_index));
										break;
									}
								}
							}
							if(k == pb_type->ports[i].num_pins) {
								/* Failed to find LUT input, a netlist error has occurred */
								vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
									"LUT %s missing input %s post packing. This is a VPR internal error, report to vpr@eecg.utoronto.ca\n",
									logical_block[iblk].name, g_atoms_nlist.net[logical_block[iblk].input_nets[in_port_index][j]].name);
							}
						}
					}
					in_port_index++;
				}
			}
			for (i = 0; i < pb_type->num_ports; i++) {
				if (pb_type->ports[i].type == OUT_PORT) {
					for (j = 0; j < pb_type->ports[i].num_pins; j++) {
						node_index =
								pb->pb_graph_node->output_pins[out_port_index][j].pin_count_in_cluster;
						fprintf(fpout, "clb_%d_rr_node_%d\n", clb_index,
								node_index);
					}
					out_port_index++;
				}
			}
			truth_table = logical_block[iblk].truth_table;
			while (truth_table) {
				fprintf(fpout, "%s\n", (char *) truth_table->data_vptr);
				truth_table = truth_table->next;
			}
		} else {
			vpr_printf_warning(__FILE__, __LINE__,
					"TODO: Implement blif dumper for subckt %s model %s", 
					logical_block[iblk].name, logical_block[iblk].model->name);
		}
	}
}

static void print_pb(FILE *fpout, t_pb * pb, int clb_index) {

	int column;
	int i, j;
	unsigned int k;
	const t_pb_type *pb_type;
	t_mode *mode;
	int in_port_index, out_port_index, node_index;

	pb_type = pb->pb_graph_node->pb_type;
	mode = &pb_type->modes[pb->mode];
	column = 0;
	if (pb_type->num_modes == 0) {
		print_primitive(fpout, pb->logical_block);
	} else {
		in_port_index = 0;
		out_port_index = 0;
		for (i = 0; i < pb_type->num_ports; i++) {
			if (!pb_type->ports[i].is_clock) {
				for (j = 0; j < pb_type->ports[i].num_pins; j++) {
					/* print .blif buffer to represent routing */
					column = 0;
					if (pb_type->ports[i].type == OUT_PORT) {
						node_index =
								pb->pb_graph_node->output_pins[out_port_index][j].pin_count_in_cluster;
						if (rr_node[node_index].net_num != OPEN) {
							fprintf(fpout, ".names clb_%d_rr_node_%d ",
									clb_index,
									find_fanin_rr_node(pb,
											pb_type->ports[i].type,
											node_index));
							if (pb->parent_pb) {
								fprintf(fpout, "clb_%d_rr_node_%d ", clb_index,
										node_index);
							} else {
								print_net_name(rr_node[node_index].net_num,
										&column, fpout);
							}
							fprintf(fpout, "\n1 1\n");
							if (pb->parent_pb == NULL) {
								for (k = 1;
										k
										< g_atoms_nlist.net[rr_node[node_index].net_num].pins.size();
										k++) {
									/* output pads pre-pended with "out:", must remove */
										if (logical_block[g_atoms_nlist.net[rr_node[node_index].net_num].pins[k].block].type
											== VPACK_OUTPAD
											&& strcmp(
											logical_block[g_atoms_nlist.net[rr_node[node_index].net_num].pins[k].block].name
															+ 4,
													g_atoms_nlist.net[rr_node[node_index].net_num].name)
													!= 0) {
										fprintf(fpout,
												".names clb_%d_rr_node_%d %s",
												clb_index,
												find_fanin_rr_node(pb,
														pb_type->ports[i].type,
														node_index),
													logical_block[g_atoms_nlist.net[rr_node[node_index].net_num].pins[k].block].name
														+ 4);
										fprintf(fpout, "\n1 1\n");
									}
								}
							}
						}

					} else {
						node_index =
								pb->pb_graph_node->input_pins[in_port_index][j].pin_count_in_cluster;
						if (rr_node[node_index].net_num != OPEN) {

							fprintf(fpout, ".names ");
							if (pb->parent_pb) {
								fprintf(fpout, "clb_%d_rr_node_%d ", clb_index,
										find_fanin_rr_node(pb,
												pb_type->ports[i].type,
												node_index));
							} else {
								print_net_name(rr_node[node_index].net_num,
										&column, fpout);
							}
							fprintf(fpout, "clb_%d_rr_node_%d", clb_index,
									node_index);
							fprintf(fpout, "\n1 1\n");
						}
					}
				}
			}
			if (pb_type->ports[i].type == OUT_PORT) {
				out_port_index++;
			} else {
				in_port_index++;
			}
		}
		for (i = 0; i < mode->num_pb_type_children; i++) {
			for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
				/* If child pb is not used but routing is used, I must print things differently */
				if ((pb->child_pbs[i] != NULL)
						&& (pb->child_pbs[i][j].name != NULL)) {
					print_pb(fpout, &pb->child_pbs[i][j], clb_index);
				} else {
					/* do nothing for now, we'll print something later if needed */
				}
			}
		}
	}
}

static void print_clusters(t_block *clb, int num_clusters, FILE * fpout) {

	/* Prints out one cluster (clb).  Both the external pins and the *
	 * internal connections are printed out.                         */

	int icluster;

	for (icluster = 0; icluster < num_clusters; icluster++) {
		rr_node = clb[icluster].pb->rr_graph;
		if(clb[icluster].pb_pin_route_stats == NULL) {
			if (clb[icluster].type != IO_TYPE)
				print_pb(fpout, clb[icluster].pb, icluster);
		} else {
			
		}
	}
}
#endif

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


	if(strcmp(logical_block[ilogical_block].model->name, "names") == 0) {
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
				fprintf(fpout, "clb_%d_rr_node_%d\n", clb_index, node_index);
			}
		}
		struct s_linked_vptr *truth_table = logical_block[ilogical_block].truth_table;
		while (truth_table) {
			fprintf(fpout, "%s\n", (char *) truth_table->data_vptr);
			truth_table = truth_table->next;
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
				int node_index = pb_graph_node->output_pins[0][0].pin_count_in_cluster;
				fprintf(fpout, "clb_%d_rr_node_%d re ", clb_index, node_index);
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
	} else {
		/* This is a user defined custom block, print based on model */
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
				
			if(pb_pin_route_stats[i].prev_pb_pin_id == OPEN) {
				/* Logic block input pin */
				assert(pb_graph_node_of_pin->parent_pb_graph_node == NULL);
				int column = 0;
				fprintf(fpout, ".names ");
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, " clb_%d_rr_node_%d\n", iclb, i);
			} else if (pb_graph_node_of_pin->parent_pb_graph_node == NULL) {
				/* Logic block output pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d ", iclb, pb_pin_route_stats[i].prev_pb_pin_id);
				print_net_name(pb_pin_route_stats[i].atom_net_idx, &column, fpout);
				fprintf(fpout, "\n");
			} else {
				/* Logic block internal pin */
				fprintf(fpout, ".names clb_%d_rr_node_%d clb_%d_rr_node_%d\n", iclb, pb_pin_route_stats[i].prev_pb_pin_id, iclb, i);
			}
			printf("1 1\n\n");
		}
	}

	free_pb_graph_pin_lookup_from_index(pb_graph_pin_lookup);
}
	
void print_models(FILE *fpout, t_model *user_models) {
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
	unsigned int i;

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

	/* print out all non I/O circuit elements */
	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (logical_block[bnum].type == VPACK_INPAD) {
			/* handle special case: input goes straight to output without going through any logic */
			for (i = 1;
					i
					< g_atoms_nlist.net[logical_block[bnum].output_nets[0][0]].pins.size();
					i++) {
					if (logical_block[g_atoms_nlist.net[logical_block[bnum].output_nets[0][0]].pins[i].block].type
						== VPACK_OUTPAD) {
					fprintf(fpout, ".names ");
					print_string(logical_block[bnum].name, &column, fpout);
					print_string(
						logical_block[g_atoms_nlist.net[logical_block[bnum].output_nets[0][0]].pins[i].block].name
									+ 4, &column, fpout);
					fprintf(fpout, "\n1 1\n");
				}
			}
		} else if (logical_block[bnum].type != VPACK_OUTPAD) {
			/* print normal logic */
			print_logical_block(fpout, bnum, clb);
		}
	}

	for(int clb_index = 0; clb_index < num_clusters; clb_index++) {
		print_routing_in_clusters(fpout, clb, clb_index);
	}
	
	fprintf(fpout, "\n.end\n");

	print_models(fpout, arch->models);

	fclose(fpout);
}
