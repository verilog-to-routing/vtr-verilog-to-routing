/**
 * Jason Luu
 * July 17, 2009
 * pb_graph creates the internal routing edges that join together the different
 * pb_types modes within a pb_type
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "util.h"
#include "token.h"
#include "arch_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "pb_type_graph.h"
#include "pb_type_graph_annotations.h"
#include "cluster_feasibility_filter.h"
#include "power.h"

/* variable global to this section that indexes each pb graph pin within a cluster */
static int pin_count_in_cluster;
static struct s_linked_vptr *edges_head;
static struct s_linked_vptr *num_edges_head;

/* TODO: Software engineering decision needed: Move this file to libarch?

 */

static int check_pb_graph(void);
static void alloc_and_load_pb_graph(INOUTP t_pb_graph_node *pb_graph_node,
		INP t_pb_graph_node *parent_pb_graph_node, INP t_pb_type *pb_type,
		INP int index, boolean load_power_structures);

static void alloc_and_load_mode_interconnect(
		INOUTP t_pb_graph_node *pb_graph_parent_node,
		INOUTP t_pb_graph_node **pb_graph_children_nodes,
		INP const t_mode * mode, boolean load_power_structures);

static boolean realloc_and_load_pb_graph_pin_ptrs_at_var(INP int line_num,
		INP const t_pb_graph_node *pb_graph_parent_node,
		INP t_pb_graph_node **pb_graph_children_nodes,
		INP boolean interconnect_error_check, INP boolean is_input_to_interc,
		INP const t_token *tokens, INOUTP int *token_index,
		INOUTP int *num_pins, OUTP t_pb_graph_pin ***pb_graph_pins);

static t_pb_graph_pin * get_pb_graph_pin_from_name(INP const char * port_name,
		INP const t_pb_graph_node * pb, INP int pin);

static void alloc_and_load_complete_interc_edges(
		INP t_interconnect * interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs);

static void alloc_and_load_direct_interc_edges(
		INP t_interconnect * interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs);

static void alloc_and_load_mux_interc_edges(INP t_interconnect * interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs);

static void alloc_and_load_pin_locations_from_pb_graph(t_type_descriptor *type);

static void echo_pb_rec(INP const t_pb_graph_node *pb, INP int level,
		INP FILE * fp);
static void echo_pb_pins(INP t_pb_graph_pin **pb_graph_pins, INP int num_ports,
		INP int level, INP FILE * fp);
static void free_pb_graph(INOUTP t_pb_graph_node *pb_graph_node);

static void alloc_and_load_interconnect_pins(t_interconnect_pins * interc_pins,
		t_interconnect * interconnect, t_pb_graph_pin *** input_pins,
		int num_input_sets, int * num_input_pins,
		t_pb_graph_pin *** output_pins, int num_output_sets,
		int * num_output_pins);

/**
 * Allocate memory into types and load the pb graph with interconnect edges 
 */
void alloc_and_load_all_pb_graphs(boolean load_power_structures) {
	int i, errors;
	edges_head = NULL;
	num_edges_head = NULL;
	for (i = 0; i < num_types; i++) {
		if (type_descriptors[i].pb_type) {
			pin_count_in_cluster = 0;
			type_descriptors[i].pb_graph_head = (t_pb_graph_node*) my_calloc(1,
					sizeof(t_pb_graph_node));
			alloc_and_load_pb_graph(type_descriptors[i].pb_graph_head, NULL,
					type_descriptors[i].pb_type, 0, load_power_structures);
			type_descriptors[i].pb_graph_head->total_pb_pins =
					pin_count_in_cluster;
			alloc_and_load_pin_locations_from_pb_graph(&type_descriptors[i]);
			load_pin_classes_in_pb_graph_head(
					type_descriptors[i].pb_graph_head);
		} else {
			type_descriptors[i].pb_graph_head = NULL;
			assert(&type_descriptors[i] == EMPTY_TYPE);
		}
	}

	errors = check_pb_graph();
	if (errors > 0) {
		vpr_printf(TIO_MESSAGE_ERROR, "in pb graph");
		exit(1);
	}
	for (i = 0; i < num_types; i++) {
		if (type_descriptors[i].pb_type) {
			load_pb_graph_pin_to_pin_annotations(
					type_descriptors[i].pb_graph_head);
		}
	}
}

/**
 * Free pb graph 
 */
void free_all_pb_graph_nodes(void) {
	int i;
	for (i = 0; i < num_types; i++) {
		if (type_descriptors[i].pb_type) {
			pin_count_in_cluster = 0;
			if (type_descriptors[i].pb_graph_head) {
				free_pb_graph(type_descriptors[i].pb_graph_head);
				free(type_descriptors[i].pb_graph_head);
			}
		}
	}
}

/**
 * Print out the pb_type graph
 */
void echo_pb_graph(char * filename) {
	FILE *fp;
	int i;

	fp = my_fopen(filename, "w", 0);

	fprintf(fp, "Physical Blocks Graph\n");
	fprintf(fp, "--------------------------------------------\n\n");

	for (i = 0; i < num_types; i++) {
		fprintf(fp, "type %s\n", type_descriptors[i].name);
		if (type_descriptors[i].pb_graph_head)
			echo_pb_rec(type_descriptors[i].pb_graph_head, 1, fp);
	}

	fclose(fp);
}

/**
 * check pb_type graph and return the number of errors
 */
static int check_pb_graph(void) {
	int num_errors;
	/* TODO: Error checks to do 
	 1.  All pin and edge connections are bidirectional and match each other
	 2.  All pb_type names are unique in a namespace
	 3.  All ports are unique in a pb_type
	 4.  Number of pb of a pb_type in graph is the same as requested number
	 5.  All pins are connected to edges
	 */
	num_errors = 0;

	return num_errors;
}

static void alloc_and_load_pb_graph(INOUTP t_pb_graph_node *pb_graph_node,
		INP t_pb_graph_node *parent_pb_graph_node, INP t_pb_type *pb_type,
		INP int index, boolean load_power_structures) {
	int i, j, k, i_input, i_output, i_clockport;

	pb_graph_node->placement_index = index;
	pb_graph_node->pb_type = pb_type;
	pb_graph_node->parent_pb_graph_node = parent_pb_graph_node;

	pb_graph_node->num_input_ports = 0;
	pb_graph_node->num_output_ports = 0;
	pb_graph_node->num_clock_ports = 0;

	/* Generate ports for pb graph node */
	for (i = 0; i < pb_type->num_ports; i++) {
		if (pb_type->ports[i].type == IN_PORT && !pb_type->ports[i].is_clock) {
			pb_graph_node->num_input_ports++;
		} else if (pb_type->ports[i].type == OUT_PORT) {
			assert(!pb_type->ports[i].is_clock);
			pb_graph_node->num_output_ports++;
		} else {
			assert(
					pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT);
			pb_graph_node->num_clock_ports++;
		}
	}

	pb_graph_node->num_input_pins = (int*) my_calloc(
			pb_graph_node->num_input_ports, sizeof(int));
	pb_graph_node->num_output_pins = (int*) my_calloc(
			pb_graph_node->num_output_ports, sizeof(int));
	pb_graph_node->num_clock_pins = (int*) my_calloc(
			pb_graph_node->num_clock_ports, sizeof(int));

	pb_graph_node->input_pins = (t_pb_graph_pin**) my_calloc(
			pb_graph_node->num_input_ports, sizeof(t_pb_graph_pin*));
	pb_graph_node->output_pins = (t_pb_graph_pin**) my_calloc(
			pb_graph_node->num_output_ports, sizeof(t_pb_graph_pin*));
	pb_graph_node->clock_pins = (t_pb_graph_pin**) my_calloc(
			pb_graph_node->num_clock_ports, sizeof(t_pb_graph_pin*));

	i_input = i_output = i_clockport = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		if (pb_type->ports[i].model_port) {
			assert(pb_type->num_modes == 0);
		} else {
			assert(pb_type->num_modes != 0 || pb_type->ports[i].is_clock);
		}
		if (pb_type->ports[i].type == IN_PORT && !pb_type->ports[i].is_clock) {
			pb_graph_node->input_pins[i_input] = (t_pb_graph_pin*) my_calloc(
					pb_type->ports[i].num_pins, sizeof(t_pb_graph_pin));
			pb_graph_node->num_input_pins[i_input] = pb_type->ports[i].num_pins;
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				pb_graph_node->input_pins[i_input][j].input_edges = NULL;
				pb_graph_node->input_pins[i_input][j].num_input_edges = 0;
				pb_graph_node->input_pins[i_input][j].output_edges = NULL;
				pb_graph_node->input_pins[i_input][j].num_output_edges = 0;
				pb_graph_node->input_pins[i_input][j].pin_number = j;
				pb_graph_node->input_pins[i_input][j].port = &pb_type->ports[i];
				pb_graph_node->input_pins[i_input][j].parent_node =
						pb_graph_node;
				pb_graph_node->input_pins[i_input][j].pin_count_in_cluster =
						pin_count_in_cluster;
				pb_graph_node->input_pins[i_input][j].type = PB_PIN_NORMAL;
				if (pb_graph_node->pb_type->blif_model != NULL ) {
					if (strcmp(pb_graph_node->pb_type->blif_model, ".output")
							== 0) {
						pb_graph_node->input_pins[i_input][j].type =
								PB_PIN_OUTPAD;
					} else if (pb_graph_node->num_clock_ports != 0) {
						pb_graph_node->input_pins[i_input][j].type =
								PB_PIN_SEQUENTIAL;
					} else {
						pb_graph_node->input_pins[i_input][j].type =
								PB_PIN_TERMINAL;
					}
				}
				pin_count_in_cluster++;
			}
			i_input++;
		} else if (pb_type->ports[i].type == OUT_PORT) {
			pb_graph_node->output_pins[i_output] = (t_pb_graph_pin*) my_calloc(
					pb_type->ports[i].num_pins, sizeof(t_pb_graph_pin));
			pb_graph_node->num_output_pins[i_output] =
					pb_type->ports[i].num_pins;
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				pb_graph_node->output_pins[i_output][j].input_edges = NULL;
				pb_graph_node->output_pins[i_output][j].num_input_edges = 0;
				pb_graph_node->output_pins[i_output][j].output_edges = NULL;
				pb_graph_node->output_pins[i_output][j].num_output_edges = 0;
				pb_graph_node->output_pins[i_output][j].pin_number = j;
				pb_graph_node->output_pins[i_output][j].port =
						&pb_type->ports[i];
				pb_graph_node->output_pins[i_output][j].parent_node =
						pb_graph_node;
				pb_graph_node->output_pins[i_output][j].pin_count_in_cluster =
						pin_count_in_cluster;
				pb_graph_node->output_pins[i_output][j].type = PB_PIN_NORMAL;
				if (pb_graph_node->pb_type->blif_model != NULL ) {
					if (strcmp(pb_graph_node->pb_type->blif_model, ".input")
							== 0) {
						pb_graph_node->output_pins[i_output][j].type =
								PB_PIN_INPAD;
					} else if (pb_graph_node->num_clock_ports != 0) {
						pb_graph_node->output_pins[i_output][j].type =
								PB_PIN_SEQUENTIAL;
					} else {
						pb_graph_node->output_pins[i_output][j].type =
								PB_PIN_TERMINAL;
					}
				}
				pin_count_in_cluster++;
			}
			i_output++;
		} else {
			assert(
					pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT);
			pb_graph_node->clock_pins[i_clockport] =
					(t_pb_graph_pin*) my_calloc(pb_type->ports[i].num_pins,
							sizeof(t_pb_graph_pin));
			pb_graph_node->num_clock_pins[i_clockport] =
					pb_type->ports[i].num_pins;
			for (j = 0; j < pb_type->ports[i].num_pins; j++) {
				pb_graph_node->clock_pins[i_clockport][j].input_edges = NULL;
				pb_graph_node->clock_pins[i_clockport][j].num_input_edges = 0;
				pb_graph_node->clock_pins[i_clockport][j].output_edges = NULL;
				pb_graph_node->clock_pins[i_clockport][j].num_output_edges = 0;
				pb_graph_node->clock_pins[i_clockport][j].pin_number = j;
				pb_graph_node->clock_pins[i_clockport][j].port =
						&pb_type->ports[i];
				pb_graph_node->clock_pins[i_clockport][j].parent_node =
						pb_graph_node;
				pb_graph_node->clock_pins[i_clockport][j].pin_count_in_cluster =
						pin_count_in_cluster;
				pb_graph_node->clock_pins[i_clockport][j].type = PB_PIN_NORMAL;
				if (pb_graph_node->pb_type->blif_model != NULL ) {
					pb_graph_node->clock_pins[i_clockport][j].type =
							PB_PIN_CLOCK;
				}
				pin_count_in_cluster++;
			}
			i_clockport++;
		}
	}

	/* Power */
	if (load_power_structures) {
		pb_graph_node->pb_node_power = (t_pb_graph_node_power*) my_calloc(1,
				sizeof(t_pb_graph_node_power));
		pb_graph_node->pb_node_power->transistor_cnt_buffers = 0.;
		pb_graph_node->pb_node_power->transistor_cnt_interc = 0.;
		pb_graph_node->pb_node_power->transistor_cnt_pb_children = 0.;
	}

	/* Allocate and load child nodes for each mode and create interconnect in each mode */
	pb_graph_node->child_pb_graph_nodes = (t_pb_graph_node***) my_calloc(
			pb_type->num_modes, sizeof(t_pb_graph_node **));
	for (i = 0; i < pb_type->num_modes; i++) {
		pb_graph_node->child_pb_graph_nodes[i] = (t_pb_graph_node**) my_calloc(
				pb_type->modes[i].num_pb_type_children,
				sizeof(t_pb_graph_node *));
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			pb_graph_node->child_pb_graph_nodes[i][j] =
					(t_pb_graph_node*) my_calloc(
							pb_type->modes[i].pb_type_children[j].num_pb,
							sizeof(t_pb_graph_node));
			for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				alloc_and_load_pb_graph(
						&pb_graph_node->child_pb_graph_nodes[i][j][k],
						pb_graph_node, &pb_type->modes[i].pb_type_children[j],
						k, load_power_structures);
			}
		}
	}

	pb_graph_node->interconnect_pins = (t_interconnect_pins**) my_calloc(
			pb_type->num_modes, sizeof(t_interconnect_pins *));
	for (i = 0; i < pb_type->num_modes; i++) {
		/* Create interconnect for mode */
		alloc_and_load_mode_interconnect(pb_graph_node,
				pb_graph_node->child_pb_graph_nodes[i], &pb_type->modes[i],
				load_power_structures);
	}
}

static void free_pb_graph(INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;
	const t_pb_type *pb_type;
	struct s_linked_vptr *cur, *cur_num;
	t_pb_graph_edge *edges;

	pb_type = pb_graph_node->pb_type;

	/*free all lists of connectable input pin pointer of pb_graph_node and it's children*/
	/*free_list_of_connectable_input_pin_ptrs (pb_graph_node);*/

	/* Free ports for pb graph node */
	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			if (pb_graph_node->input_pins[i][j].pin_timing)
				free(pb_graph_node->input_pins[i][j].pin_timing);
			if (pb_graph_node->input_pins[i][j].pin_timing_del_max)
				free(pb_graph_node->input_pins[i][j].pin_timing_del_max);
			if (pb_graph_node->input_pins[i][j].input_edges)
				free(pb_graph_node->input_pins[i][j].input_edges);
			if (pb_graph_node->input_pins[i][j].output_edges)
				free(pb_graph_node->input_pins[i][j].output_edges);
			if (pb_graph_node->input_pins[i][j].parent_pin_class)
				free(pb_graph_node->input_pins[i][j].parent_pin_class);
		}
		free(pb_graph_node->input_pins[i]);
	}
	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			if (pb_graph_node->output_pins[i][j].pin_timing)
				free(pb_graph_node->output_pins[i][j].pin_timing);
			if (pb_graph_node->output_pins[i][j].pin_timing_del_max)
				free(pb_graph_node->output_pins[i][j].pin_timing_del_max);
			if (pb_graph_node->output_pins[i][j].input_edges)
				free(pb_graph_node->output_pins[i][j].input_edges);
			if (pb_graph_node->output_pins[i][j].output_edges)
				free(pb_graph_node->output_pins[i][j].output_edges);
			if (pb_graph_node->output_pins[i][j].parent_pin_class)
				free(pb_graph_node->output_pins[i][j].parent_pin_class);

			if (pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs) {
				for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
					if (pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k]) {
						free(
								pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k]);
					}
				}
				free(
						pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs);
			}

			if (pb_graph_node->output_pins[i][j].num_connectable_primtive_input_pins)
				free(
						pb_graph_node->output_pins[i][j].num_connectable_primtive_input_pins);
		}
		free(pb_graph_node->output_pins[i]);
	}
	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			if (pb_graph_node->clock_pins[i][j].pin_timing)
				free(pb_graph_node->clock_pins[i][j].pin_timing);
			if (pb_graph_node->clock_pins[i][j].pin_timing_del_max)
				free(pb_graph_node->clock_pins[i][j].pin_timing_del_max);
			if (pb_graph_node->clock_pins[i][j].input_edges)
				free(pb_graph_node->clock_pins[i][j].input_edges);
			if (pb_graph_node->clock_pins[i][j].output_edges)
				free(pb_graph_node->clock_pins[i][j].output_edges);
			if (pb_graph_node->clock_pins[i][j].parent_pin_class)
				free(pb_graph_node->clock_pins[i][j].parent_pin_class);
		}
		free(pb_graph_node->clock_pins[i]);
	}


	for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		free(pb_graph_node->interconnect_pins[i]);
	}
	free(pb_graph_node->interconnect_pins);
	
	free(pb_graph_node->input_pins);
	free(pb_graph_node->output_pins);
	free(pb_graph_node->clock_pins);

	free(pb_graph_node->num_input_pins);
	free(pb_graph_node->num_output_pins);
	free(pb_graph_node->num_clock_pins);

	free(pb_graph_node->input_pin_class_size);
	free(pb_graph_node->output_pin_class_size);

	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				free_pb_graph(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
			}
			free(pb_graph_node->child_pb_graph_nodes[i][j]);
		}
		free(pb_graph_node->child_pb_graph_nodes[i]);
	}
	free(pb_graph_node->child_pb_graph_nodes);

	while (edges_head != NULL ) {
		cur = edges_head;
		cur_num = num_edges_head;
		edges = (t_pb_graph_edge*) cur->data_vptr;
		for (i = 0; i < (long) cur_num->data_vptr; i++) {
			free(edges[i].input_pins);
			free(edges[i].output_pins);
			if (edges[i].pack_pattern_indices) {
				free(edges[i].pack_pattern_indices);
			}
			if (edges[i].pack_pattern_names) {
				free(edges[i].pack_pattern_names);
			}
		}
		edges_head = edges_head->next;
		num_edges_head = num_edges_head->next;
		free(edges);
		free(cur_num);
		free(cur);
	}
}

static void alloc_and_load_interconnect_pins(t_interconnect_pins * interc_pins,
		t_interconnect * interconnect, t_pb_graph_pin *** input_pins,
		int num_input_sets, int * num_input_pins,
		t_pb_graph_pin *** output_pins, int num_output_sets,
		int * num_output_pins) {
	int set_idx;
	int pin_idx;
	int port_idx;
	int num_ports;

	interc_pins->interconnect = interconnect;

	switch (interconnect->type) {
	case DIRECT_INTERC:
		assert(num_output_sets == 1);
		/* Fall through here */

	case MUX_INTERC:
		if (!interconnect->interconnect_power->port_info_initialized) {
			for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
				assert(num_input_pins[set_idx] == num_output_pins[0]);
			}
			interconnect->interconnect_power->num_pins_per_port =
					num_input_pins[0];
			interconnect->interconnect_power->num_input_ports = num_input_sets;
			interconnect->interconnect_power->num_output_ports = 1;

			/* No longer used - mux architectures are not configurable, the
			 * default is always assumed
			 if (interconnect->mux_arch) {
			 interconnect->mux_arch->num_inputs =
			 interconnect->num_input_ports;

			 mux_arch_fix_levels(interconnect->mux_arch);

			 interconnect->mux_arch->mux_graph_head =
			 alloc_and_load_mux_graph(interconnect->num_input_ports,
			 interconnect->mux_arch->levels);
			 }
			 */

			interconnect->interconnect_power->port_info_initialized = TRUE;
		}

		interc_pins->input_pins = (t_pb_graph_pin***) my_calloc(num_input_sets,
				sizeof(t_pb_graph_pin**));
		for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
			interc_pins->input_pins[set_idx] = (t_pb_graph_pin**) my_calloc(
					interconnect->interconnect_power->num_pins_per_port,
					sizeof(t_pb_graph_pin*));
		}

		interc_pins->output_pins = (t_pb_graph_pin***) my_calloc(1,
				sizeof(t_pb_graph_pin**));
		interc_pins->output_pins[0] = (t_pb_graph_pin**) my_calloc(
				interconnect->interconnect_power->num_pins_per_port,
				sizeof(t_pb_graph_pin*));

		for (pin_idx = 0;
				pin_idx < interconnect->interconnect_power->num_pins_per_port;
				pin_idx++) {
			for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
				interc_pins->input_pins[set_idx][pin_idx] =
						input_pins[set_idx][pin_idx];
			}
			interc_pins->output_pins[0][pin_idx] = output_pins[0][pin_idx];
		}

		break;
	case COMPLETE_INTERC:

		if (!interconnect->interconnect_power->port_info_initialized) {
			/* The code does not support bus-based crossbars, so all pins from all input sets
			 * connect to all pins from all output sets */
			interconnect->interconnect_power->num_pins_per_port = 1;

			num_ports = 0;
			for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
				num_ports += num_input_pins[set_idx];
			}
			interconnect->interconnect_power->num_input_ports = num_ports;

			num_ports = 0;
			for (set_idx = 0; set_idx < num_output_sets; set_idx++) {
				num_ports += num_output_pins[set_idx];
			}
			interconnect->interconnect_power->num_output_ports = num_ports;

			/*
			 if (interconnect->mux_arch) {
			 interconnect->mux_arch->num_inputs =
			 interconnect->num_input_ports;

			 mux_arch_fix_levels(interconnect->mux_arch);

			 interconnect->mux_arch->mux_graph_head =
			 alloc_and_load_mux_graph(interconnect->num_input_ports,
			 interconnect->mux_arch->levels);
			 }*/

			interconnect->interconnect_power->port_info_initialized = TRUE;
		}

		/* Input Pins */
		interc_pins->input_pins = (t_pb_graph_pin ***) my_calloc(
				interconnect->interconnect_power->num_input_ports,
				sizeof(t_pb_graph_pin**));
		for (port_idx = 0;
				port_idx < interconnect->interconnect_power->num_input_ports;
				port_idx++) {
			interc_pins->input_pins[port_idx] = (t_pb_graph_pin**) my_calloc(
					interconnect->interconnect_power->num_pins_per_port,
					sizeof(t_pb_graph_pin*));
		}
		num_ports = 0;
		for (set_idx = 0; set_idx < num_input_sets; set_idx++) {
			for (pin_idx = 0; pin_idx < num_input_pins[set_idx]; pin_idx++) {
				interc_pins->input_pins[num_ports++][0] =
						input_pins[set_idx][pin_idx];
			}
		}

		/* Output Pins */
		interc_pins->output_pins = (t_pb_graph_pin ***) my_calloc(
				interconnect->interconnect_power->num_output_ports,
				sizeof(t_pb_graph_pin**));
		for (port_idx = 0;
				port_idx < interconnect->interconnect_power->num_output_ports;
				port_idx++) {
			interc_pins->output_pins[port_idx] = (t_pb_graph_pin **) my_calloc(
					interconnect->interconnect_power->num_pins_per_port,
					sizeof(t_pb_graph_pin*));
		}
		num_ports = 0;
		for (set_idx = 0; set_idx < num_output_sets; set_idx++) {
			for (pin_idx = 0; pin_idx < num_output_pins[set_idx]; pin_idx++) {
				interc_pins->output_pins[num_ports++][0] =
						output_pins[set_idx][pin_idx];
			}
		}

		break;
	}

}

/**
 * Generate interconnect associated with a mode of operation
 * pb_graph_parent_node: parent node of pb in mode
 * pb_graph_children_nodes: [0..num_pb_type_in_mode-1][0..num_pb]
 * mode: mode of operation
 */
static void alloc_and_load_mode_interconnect(
		INOUTP t_pb_graph_node *pb_graph_parent_node,
		INOUTP t_pb_graph_node **pb_graph_children_nodes,
		INP const t_mode * mode, boolean load_power_structures) {
	int i, j;
	int *num_input_pb_graph_node_pins, *num_output_pb_graph_node_pins; /* number of pins in a set [0..num_sets-1] */
	int num_input_pb_graph_node_sets, num_output_pb_graph_node_sets;
	t_pb_graph_pin *** input_pb_graph_node_pins, ***output_pb_graph_node_pins;

	if (load_power_structures) {
		assert(pb_graph_parent_node->interconnect_pins[mode->index] == NULL);
		pb_graph_parent_node->interconnect_pins[mode->index] =
				(t_interconnect_pins*) my_calloc(mode->num_interconnect,
						sizeof(t_interconnect_pins));
	}

	for (i = 0; i < mode->num_interconnect; i++) {
		/* determine the interconnect input and output pins */
		input_pb_graph_node_pins = alloc_and_load_port_pin_ptrs_from_string(
				mode->interconnect[i].line_num, pb_graph_parent_node,
				pb_graph_children_nodes, mode->interconnect[i].input_string,
				&num_input_pb_graph_node_pins, &num_input_pb_graph_node_sets,
				TRUE, TRUE);

		output_pb_graph_node_pins = alloc_and_load_port_pin_ptrs_from_string(
				mode->interconnect[i].line_num, pb_graph_parent_node,
				pb_graph_children_nodes, mode->interconnect[i].output_string,
				&num_output_pb_graph_node_pins, &num_output_pb_graph_node_sets,
				FALSE, TRUE);

		if (load_power_structures) {
			alloc_and_load_interconnect_pins(
					&pb_graph_parent_node->interconnect_pins[mode->index][i],
					&mode->interconnect[i], input_pb_graph_node_pins,
					num_input_pb_graph_node_sets, num_input_pb_graph_node_pins,
					output_pb_graph_node_pins, num_output_pb_graph_node_sets,
					num_output_pb_graph_node_pins);
		}

		/* process the interconnect based on its type */
		switch (mode->interconnect[i].type) {

		case COMPLETE_INTERC:
			alloc_and_load_complete_interc_edges(&mode->interconnect[i],
					input_pb_graph_node_pins, num_input_pb_graph_node_sets,
					num_input_pb_graph_node_pins, output_pb_graph_node_pins,
					num_output_pb_graph_node_sets,
					num_output_pb_graph_node_pins);

			break;

		case DIRECT_INTERC:
			alloc_and_load_direct_interc_edges(&mode->interconnect[i],
					input_pb_graph_node_pins, num_input_pb_graph_node_sets,
					num_input_pb_graph_node_pins, output_pb_graph_node_pins,
					num_output_pb_graph_node_sets,
					num_output_pb_graph_node_pins);
			break;

		case MUX_INTERC:
			alloc_and_load_mux_interc_edges(&mode->interconnect[i],
					input_pb_graph_node_pins, num_input_pb_graph_node_sets,
					num_input_pb_graph_node_pins, output_pb_graph_node_pins,
					num_output_pb_graph_node_sets,
					num_output_pb_graph_node_pins);

			break;

		default:
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Unknown interconnect %d for mode %s in pb_type %s, input %s, output %s\n",
					mode->interconnect[i].line_num, mode->interconnect[i].type,
					mode->name, pb_graph_parent_node->pb_type->name,
					mode->interconnect[i].input_string,
					mode->interconnect[i].output_string);
			exit(1);
		}
		for (j = 0; j < num_input_pb_graph_node_sets; j++) {
			free(input_pb_graph_node_pins[j]);
		}
		free(input_pb_graph_node_pins);
		for (j = 0; j < num_output_pb_graph_node_sets; j++) {
			free(output_pb_graph_node_pins[j]);
		}
		free(output_pb_graph_node_pins);
		free(num_input_pb_graph_node_pins);
		free(num_output_pb_graph_node_pins);
	}
}

/**
 * creates an array of pointers to the pb graph node pins in order from the port string
 * returns t_pb_graph_pin ptr indexed by [0..num_sets_in_port - 1][0..num_ptrs - 1]
 */
t_pb_graph_pin *** alloc_and_load_port_pin_ptrs_from_string(INP int line_num,
		INP const t_pb_graph_node *pb_graph_parent_node,
		INP t_pb_graph_node **pb_graph_children_nodes,
		INP const char * port_string, OUTP int ** num_ptrs, OUTP int * num_sets,
		INP boolean is_input_to_interc, INP boolean interconnect_error_check) {
	t_token * tokens;
	int num_tokens, curr_set;
	int i;
	boolean in_squig_bracket, success;

	t_pb_graph_pin ***pb_graph_pins;

	num_tokens = 0;
	tokens = GetTokensFromString(port_string, &num_tokens);
	*num_sets = 0;
	in_squig_bracket = FALSE;

	/* count the number of sets available */
	for (i = 0; i < num_tokens; i++) {
		assert(tokens[i].type != TOKEN_NULL);
		if (tokens[i].type == TOKEN_OPEN_SQUIG_BRACKET) {
			if (in_squig_bracket) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] { inside { in port %s\n", line_num,
						port_string);
				exit(1);
			}
			in_squig_bracket = TRUE;
		} else if (tokens[i].type == TOKEN_CLOSE_SQUIG_BRACKET) {
			if (!in_squig_bracket) {
				(*num_sets)++;
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] No matching '{' for '}' in port %s\n",
						line_num, port_string);
				exit(1);
			}
			in_squig_bracket = FALSE;
		} else if (tokens[i].type == TOKEN_DOT) {
			if (!in_squig_bracket) {
				(*num_sets)++;
			}
		}
	}

	if (in_squig_bracket) {
		(*num_sets)++;
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] No matching '{' for '}' in port %s\n", line_num,
				port_string);
		exit(1);
	}

	pb_graph_pins = (t_pb_graph_pin***) my_calloc(*num_sets,
			sizeof(t_pb_graph_pin**));
	*num_ptrs = (int*) my_calloc(*num_sets, sizeof(int));

	curr_set = 0;
	for (i = 0; i < num_tokens; i++) {
		assert(tokens[i].type != TOKEN_NULL);
		if (tokens[i].type == TOKEN_OPEN_SQUIG_BRACKET) {
			if (in_squig_bracket) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] { inside { in port %s\n", line_num,
						port_string);
				exit(1);
			}
			in_squig_bracket = TRUE;
		} else if (tokens[i].type == TOKEN_CLOSE_SQUIG_BRACKET) {
			if ((*num_ptrs)[curr_set] == 0) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] No data contained in {} in port %s\n",
						line_num, port_string);
				exit(1);
			}
			if (!in_squig_bracket) {
				curr_set++;
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] No matching '{' for '}' in port %s\n",
						line_num, port_string);
				exit(1);
			}
			in_squig_bracket = FALSE;
		} else if (tokens[i].type == TOKEN_STRING) {

			success = realloc_and_load_pb_graph_pin_ptrs_at_var(line_num,
					pb_graph_parent_node, pb_graph_children_nodes,
					interconnect_error_check, is_input_to_interc, tokens, &i,
					&((*num_ptrs)[curr_set]), &pb_graph_pins[curr_set]);

			if (!success) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] syntax error processing port string %s\n",
						line_num, port_string);
				exit(1);
			}

			if (!in_squig_bracket) {
				curr_set++;
			}
		}
	}
	assert(curr_set == *num_sets);
	freeTokens(tokens, num_tokens);
	return pb_graph_pins;
}

/**
 * Creates edges to connect all input pins to output pins 
 */
static void alloc_and_load_complete_interc_edges(
		INP t_interconnect *interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs) {
	int i_inset, i_outset, i_inpin, i_outpin;
	int in_count, out_count;
	t_pb_graph_edge *edges;
	int i_edge;
	struct s_linked_vptr *cur;

	assert(interconnect->infer_annotations == FALSE);

	/* Allocate memory for edges, and reallocate more memory for pins connecting to those edges */
	in_count = out_count = 0;

	for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
		in_count += num_input_ptrs[i_inset];
	}
	for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
		out_count += num_output_ptrs[i_outset];
	}

	edges = (t_pb_graph_edge*) my_calloc(in_count * out_count,
			sizeof(t_pb_graph_edge));
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = edges_head;
	edges_head = cur;
	cur->data_vptr = (void *) edges;
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = num_edges_head;
	num_edges_head = cur;
	cur->data_vptr = (void *) ((long) in_count * out_count);

	for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
		for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
			input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges =
					(t_pb_graph_edge **) my_realloc(
							input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges,
							(input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges
									+ out_count) * sizeof(t_pb_graph_edge *));
		}
	}

	for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
		for (i_outpin = 0; i_outpin < num_output_ptrs[i_outset]; i_outpin++) {
			output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->input_edges =
					(t_pb_graph_edge **) my_realloc(
							output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->input_edges,
							(output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->num_input_edges
									+ in_count) * sizeof(t_pb_graph_edge *));
		}
	}

	i_edge = 0;

	/* Load connections between pins and record these updates in the edges */
	for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
		for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
			for (i_outset = 0; i_outset < num_output_sets; i_outset++) {
				for (i_outpin = 0; i_outpin < num_output_ptrs[i_outset];
						i_outpin++) {

					input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges[input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges] =
							&edges[i_edge];
					input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges++;
					output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->input_edges[output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->num_input_edges] =
							&edges[i_edge];
					output_pb_graph_node_pin_ptrs[i_outset][i_outpin]->num_input_edges++;

					edges[i_edge].num_input_pins = 1;
					edges[i_edge].input_pins = (t_pb_graph_pin **) my_malloc(
							sizeof(t_pb_graph_pin *));
					edges[i_edge].input_pins[0] =
							input_pb_graph_node_pin_ptrs[i_inset][i_inpin];
					edges[i_edge].num_output_pins = 1;
					edges[i_edge].output_pins = (t_pb_graph_pin **) my_malloc(
							sizeof(t_pb_graph_pin *));
					edges[i_edge].output_pins[0] =
							output_pb_graph_node_pin_ptrs[i_outset][i_outpin];

					edges[i_edge].interconnect = interconnect;
					edges[i_edge].driver_set = i_inset;
					edges[i_edge].driver_pin = i_inpin;

					i_edge++;
				}
			}
		}
	}
	assert(i_edge == in_count * out_count);
}

static void alloc_and_load_direct_interc_edges(
		INP t_interconnect *interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs) {

	int i;
	t_pb_graph_edge *edges;
	struct s_linked_vptr *cur;

	/* Allocate memory for edges */
	if (!(num_input_sets == 1 && num_output_sets == 1)) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Direct interconnect allows connections from one set of pins to one other set\n",
				interconnect->line_num);
		exit(1);
	}
	if (!(num_input_ptrs[0] == num_output_ptrs[0])) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Direct interconnect must use an equal number of pins\n",
				interconnect->line_num);
		exit(1);
	}

	edges = (t_pb_graph_edge*) my_calloc(num_input_ptrs[0],
			sizeof(t_pb_graph_edge));
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = edges_head;
	edges_head = cur;
	cur->data_vptr = (void *) edges;
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = num_edges_head;
	num_edges_head = cur;
	cur->data_vptr = (void *) ((long) num_input_ptrs[0]);

	/* Reallocate memory for pins and load connections between pins and record these updates in the edges */
	for (i = 0; i < num_input_ptrs[0]; i++) {
		input_pb_graph_node_pin_ptrs[0][i]->output_edges =
				(t_pb_graph_edge **) my_realloc(
						input_pb_graph_node_pin_ptrs[0][i]->output_edges,
						(input_pb_graph_node_pin_ptrs[0][i]->num_output_edges
								+ 1) * sizeof(t_pb_graph_edge *));
		input_pb_graph_node_pin_ptrs[0][i]->output_edges[input_pb_graph_node_pin_ptrs[0][i]->num_output_edges] =
				&edges[i];
		input_pb_graph_node_pin_ptrs[0][i]->num_output_edges++;

		output_pb_graph_node_pin_ptrs[0][i]->input_edges =
				(t_pb_graph_edge **) my_realloc(
						output_pb_graph_node_pin_ptrs[0][i]->input_edges,
						(output_pb_graph_node_pin_ptrs[0][i]->num_input_edges
								+ 1) * sizeof(t_pb_graph_edge *));
		output_pb_graph_node_pin_ptrs[0][i]->input_edges[output_pb_graph_node_pin_ptrs[0][i]->num_input_edges] =
				&edges[i];
		output_pb_graph_node_pin_ptrs[0][i]->num_input_edges++;

		edges[i].num_input_pins = 1;
		edges[i].input_pins = (t_pb_graph_pin **) my_malloc(
				sizeof(t_pb_graph_pin *));
		edges[i].input_pins[0] = input_pb_graph_node_pin_ptrs[0][i];
		edges[i].num_output_pins = 1;
		edges[i].output_pins = (t_pb_graph_pin **) my_malloc(
				sizeof(t_pb_graph_pin *));
		edges[i].output_pins[0] = output_pb_graph_node_pin_ptrs[0][i];

		edges[i].interconnect = interconnect;
		edges[i].driver_set = 0;
		edges[i].driver_pin = i;
		edges[i].infer_pattern = interconnect->infer_annotations;
	}
}

static void alloc_and_load_mux_interc_edges( INP t_interconnect * interconnect,
		INOUTP t_pb_graph_pin *** input_pb_graph_node_pin_ptrs,
		INP int num_input_sets, INP int *num_input_ptrs,
		INOUTP t_pb_graph_pin *** output_pb_graph_node_pin_ptrs,
		INP int num_output_sets, INP int *num_output_ptrs) {
	int i_inset, i_inpin, i_outpin;
	t_pb_graph_edge *edges;
	struct s_linked_vptr *cur;

	assert(interconnect->infer_annotations == FALSE);

	/* Allocate memory for edges, and reallocate more memory for pins connecting to those edges */
	if (num_output_sets != 1) {
		vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Mux must have one output\n",
				interconnect->line_num);
		exit(1);
	}

	edges = (t_pb_graph_edge*) my_calloc(num_input_sets,
			sizeof(t_pb_graph_edge));
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = edges_head;
	edges_head = cur;
	cur->data_vptr = (void *) edges;
	cur = (struct s_linked_vptr*) my_malloc(sizeof(struct s_linked_vptr));
	cur->next = num_edges_head;
	num_edges_head = cur;
	cur->data_vptr = (void *) ((long) num_input_sets);

	for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
		for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
			input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges =
					(t_pb_graph_edge**) my_realloc(
							input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges,
							(input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges
									+ 1) * sizeof(t_pb_graph_edge *));
		}
	}

	for (i_outpin = 0; i_outpin < num_output_ptrs[0]; i_outpin++) {
		output_pb_graph_node_pin_ptrs[0][i_outpin]->input_edges =
				(t_pb_graph_edge**) my_realloc(
						output_pb_graph_node_pin_ptrs[0][i_outpin]->input_edges,
						(output_pb_graph_node_pin_ptrs[0][i_outpin]->num_input_edges
								+ num_input_sets) * sizeof(t_pb_graph_edge *));
	}

	/* Load connections between pins and record these updates in the edges */
	for (i_inset = 0; i_inset < num_input_sets; i_inset++) {
		if (num_output_ptrs[0] != num_input_ptrs[i_inset]) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] # of pins for a particular data line of a mux must equal number of pins at output of mux\n",
					interconnect->line_num);
			exit(1);
		}
		edges[i_inset].input_pins = (t_pb_graph_pin**) my_calloc(
				num_output_ptrs[0], sizeof(t_pb_graph_pin *));
		edges[i_inset].output_pins = (t_pb_graph_pin**) my_calloc(
				num_output_ptrs[0], sizeof(t_pb_graph_pin *));
		edges[i_inset].num_input_pins = num_output_ptrs[0];
		edges[i_inset].num_output_pins = num_output_ptrs[0];
		for (i_inpin = 0; i_inpin < num_input_ptrs[i_inset]; i_inpin++) {
			input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->output_edges[input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges] =
					&edges[i_inset];
			input_pb_graph_node_pin_ptrs[i_inset][i_inpin]->num_output_edges++;
			output_pb_graph_node_pin_ptrs[0][i_inpin]->input_edges[output_pb_graph_node_pin_ptrs[0][i_inpin]->num_input_edges] =
					&edges[i_inset];
			output_pb_graph_node_pin_ptrs[0][i_inpin]->num_input_edges++;

			edges[i_inset].input_pins[i_inpin] =
					input_pb_graph_node_pin_ptrs[i_inset][i_inpin];
			edges[i_inset].output_pins[i_inpin] =
					output_pb_graph_node_pin_ptrs[0][i_inpin];

			if (i_inpin != 0) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Bus-based mux not yet supported, will consider for future work\n",
						interconnect->line_num);
				exit(1);
			}
			edges[i_inset].interconnect = interconnect;
			edges[i_inset].driver_set = i_inset;
			edges[i_inset].driver_pin = i_inpin;
		}
	}
}

/**
 * populate array of pb graph pins for a single variable of type pb_type[int:int].port[int:int]
 * pb_graph_pins: pointer to array from [0..num_port_pins] of pb_graph_pin pointers
 * tokens: array of tokens to scan
 * num_pins: current number of pins in pb_graph_pin array 
 */
static boolean realloc_and_load_pb_graph_pin_ptrs_at_var(INP int line_num,
		INP const t_pb_graph_node *pb_graph_parent_node,
		INP t_pb_graph_node **pb_graph_children_nodes,
		INP boolean interconnect_error_check, INP boolean is_input_to_interc,
		INP const t_token *tokens, INOUTP int *token_index,
		INOUTP int *num_pins, OUTP t_pb_graph_pin ***pb_graph_pins) {

	int i, j, ipin, ipb;
	int pb_msb, pb_lsb;
	int pin_msb, pin_lsb;
	int max_pb_node_array;
	const t_pb_graph_node *pb_node_array;
	char *port_name;
	t_port *iport;
	int add_or_subtract_pb, add_or_subtract_pin;
	boolean found;
	t_mode *mode = NULL;

	assert(tokens[*token_index].type == TOKEN_STRING);
	pb_node_array = NULL;
	max_pb_node_array = 0;

	if (pb_graph_children_nodes)
		mode = pb_graph_children_nodes[0][0].pb_type->parent_mode;

	pb_msb = pb_lsb = OPEN;
	pin_msb = pin_lsb = OPEN;

	/* parse pb */
	found = FALSE;
	if (0
			== strcmp(pb_graph_parent_node->pb_type->name,
					tokens[*token_index].data)) {
		pb_node_array = pb_graph_parent_node;
		max_pb_node_array = 1;
		pb_msb = pb_lsb = 0;
		found = TRUE;
		(*token_index)++;
		if (tokens[*token_index].type == TOKEN_OPEN_SQUARE_BRACKET) {
			(*token_index)++;
			if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
				return FALSE;
			}
			pb_msb = my_atoi(tokens[*token_index].data);
			(*token_index)++;
			if (!checkTokenType(tokens[*token_index], TOKEN_COLON)) {
				if (!checkTokenType(tokens[*token_index],
						TOKEN_CLOSE_SQUARE_BRACKET)) {
					return FALSE;
				}
				pb_lsb = pb_msb;
				(*token_index)++;
			} else {
				(*token_index)++;
				if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
					return FALSE;
				}
				pb_lsb = my_atoi(tokens[*token_index].data);
				(*token_index)++;
				if (!checkTokenType(tokens[*token_index],
						TOKEN_CLOSE_SQUARE_BRACKET)) {
					return FALSE;
				}
				(*token_index)++;
			}
			/* Check to make sure indices from user match internal data structures for the indices of the parent */
			if ((pb_lsb != pb_msb)
					&& (pb_lsb != pb_graph_parent_node->placement_index)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Incorrect placement index for %s, expected index %d\n",
						line_num, tokens[0].data,
						pb_graph_parent_node->placement_index);
				return FALSE;
			}
			pb_lsb = pb_msb = 0; /* Internal representation of parent is always 0 */
		}
	} else {
		if (mode == NULL ) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] pb_graph_parent_node %s failed\n", line_num,
					pb_graph_parent_node->pb_type->name);
			exit(1);
		}
		for (i = 0; i < mode->num_pb_type_children; i++) {
			assert(
					&mode->pb_type_children[i] == pb_graph_children_nodes[i][0].pb_type);
			if (0
					== strcmp(mode->pb_type_children[i].name,
							tokens[*token_index].data)) {
				pb_node_array = pb_graph_children_nodes[i];
				max_pb_node_array = mode->pb_type_children[i].num_pb;
				found = TRUE;
				(*token_index)++;

				if (tokens[*token_index].type == TOKEN_OPEN_SQUARE_BRACKET) {
					(*token_index)++;
					if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
						return FALSE;
					}
					pb_msb = my_atoi(tokens[*token_index].data);
					(*token_index)++;
					if (!checkTokenType(tokens[*token_index], TOKEN_COLON)) {
						if (!checkTokenType(tokens[*token_index],
								TOKEN_CLOSE_SQUARE_BRACKET)) {
							return FALSE;
						}
						pb_lsb = pb_msb;
						(*token_index)++;
					} else {
						(*token_index)++;
						if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
							return FALSE;
						}
						pb_lsb = my_atoi(tokens[*token_index].data);
						(*token_index)++;
						if (!checkTokenType(tokens[*token_index],
								TOKEN_CLOSE_SQUARE_BRACKET)) {
							return FALSE;
						}
						(*token_index)++;
					}
				} else {
					pb_msb = pb_node_array[0].pb_type->num_pb - 1;
					pb_lsb = 0;
				}
				break;
			}
		}
	}

	if (!found) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Unknown pb_type name %s, not defined in namespace of mode %s\n",
				line_num, tokens[*token_index].data, mode->name);
		return FALSE;
	}

	found = FALSE;

	if (!checkTokenType(tokens[*token_index], TOKEN_DOT)) {
		return FALSE;
	}
	(*token_index)++;
	if (!checkTokenType(tokens[*token_index], TOKEN_STRING)) {
		return FALSE;
	}

	/* parse ports and port pins of pb */
	port_name = tokens[*token_index].data;

	(*token_index)++;
	if (tokens[*token_index].type == TOKEN_OPEN_SQUARE_BRACKET) {
		(*token_index)++;
		if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
			return FALSE;
		}
		pin_msb = my_atoi(tokens[*token_index].data);
		(*token_index)++;
		if (!checkTokenType(tokens[*token_index], TOKEN_COLON)) {
			if (!checkTokenType(tokens[*token_index],
					TOKEN_CLOSE_SQUARE_BRACKET)) {
				return FALSE;
			}
			pin_lsb = pin_msb;
			(*token_index)++;
		} else {
			(*token_index)++;
			if (!checkTokenType(tokens[*token_index], TOKEN_INT)) {
				return FALSE;
			}
			pin_lsb = my_atoi(tokens[*token_index].data);
			(*token_index)++;
			if (!checkTokenType(tokens[*token_index],
					TOKEN_CLOSE_SQUARE_BRACKET)) {
				return FALSE;
			}
			(*token_index)++;
		}
	} else {
		if (pb_lsb < 0 || pb_lsb >= max_pb_node_array) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] pb %d out of range [%d,%d]\n", line_num, pb_lsb,
					max_pb_node_array - 1, 0);
			exit(1);
		}
		if (get_pb_graph_pin_from_name(port_name, &pb_node_array[pb_lsb],
				0) == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] failed to find port name %s\n", line_num,
					port_name);
			exit(1);
		}
		iport =
				get_pb_graph_pin_from_name(port_name, &pb_node_array[pb_lsb], 0)->port;
		pin_msb = iport->num_pins - 1;
		pin_lsb = 0;
	}
	(*token_index)--;

	if (pb_msb < pb_lsb) {
		add_or_subtract_pb = -1;
	} else {
		add_or_subtract_pb = 1;
	}

	if (pin_msb < pin_lsb) {
		add_or_subtract_pin = -1;
	} else {
		add_or_subtract_pin = 1;
	}
	*num_pins += (abs(pb_msb - pb_lsb) + 1) * (abs(pin_msb - pin_lsb) + 1);
	*pb_graph_pins = (t_pb_graph_pin**) my_calloc(*num_pins,
			sizeof(t_pb_graph_pin *));
	i = j = 0;

	ipb = pb_lsb;

	while (ipb != pb_msb + add_or_subtract_pin) {
		ipin = pin_lsb;
		j = 0;
		while (ipin != pin_msb + add_or_subtract_pin) {
			if (ipb < 0 || ipb >= max_pb_node_array) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] pb %d out of range [%d,%d]\n", line_num, ipb,
						max_pb_node_array - 1, 0);
				exit(1);
			}
			(*pb_graph_pins)[i * (abs(pin_msb - pin_lsb) + 1) + j] =
					get_pb_graph_pin_from_name(port_name, &pb_node_array[ipb],
							ipin);
			if ((*pb_graph_pins)[i * (abs(pin_msb - pin_lsb) + 1) + j] == NULL ) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Pin %s.%s[%d] cannot be found\n", line_num,
						pb_node_array[ipb].pb_type->name, port_name, ipin);
				exit(1);
			}
			iport =
					(*pb_graph_pins)[i * (abs(pin_msb - pin_lsb) + 1) + j]->port;
			if (!iport) {
				return FALSE;
			}

			/* Error checking before assignment */
			if (interconnect_error_check) {
				if (pb_node_array == pb_graph_parent_node) {
					if (is_input_to_interc) {
						if (iport->type != IN_PORT) {
							vpr_printf(TIO_MESSAGE_ERROR,
									"[LINE %d] input to interconnect from parent is not an input or clock pin\n",
									line_num);
							return FALSE;
						}
					} else {
						if (iport->type != OUT_PORT) {
							vpr_printf(TIO_MESSAGE_ERROR,
									"[LINE %d] output from interconnect from parent is not an input or clock pin\n",
									line_num);
							return FALSE;
						}
					}
				} else {
					if (is_input_to_interc) {
						if (iport->type != OUT_PORT) {
							vpr_printf(TIO_MESSAGE_ERROR,
									"[LINE %d] output from interconnect from parent is not an input or clock pin\n",
									line_num);
							return FALSE;
						}
					} else {
						if (iport->type != IN_PORT) {
							vpr_printf(TIO_MESSAGE_ERROR,
									"[LINE %d] input to interconnect from parent is not an input or clock pin\n",
									line_num);
							return FALSE;
						}
					}
				}
			}

			/* load pb_graph_pin for pin */

			ipin += add_or_subtract_pin;
			j++;
		}
		i++;
		ipb += add_or_subtract_pb;
	}

	assert((abs(pb_msb - pb_lsb) + 1) * (abs(pin_msb - pin_lsb) + 1) == i * j);

	return TRUE;
}

static t_pb_graph_pin * get_pb_graph_pin_from_name(INP const char * port_name,
		INP const t_pb_graph_node * pb, INP int pin) {
	int i;

	for (i = 0; i < pb->num_input_ports; i++) {
		if (0 == strcmp(port_name, pb->input_pins[i][0].port->name)) {
			if (pin < pb->input_pins[i][0].port->num_pins) {
				return &pb->input_pins[i][pin];
			} else {
				return NULL ;
			}
		}
	}
	for (i = 0; i < pb->num_output_ports; i++) {
		if (0 == strcmp(port_name, pb->output_pins[i][0].port->name)) {
			if (pin < pb->output_pins[i][0].port->num_pins) {
				return &pb->output_pins[i][pin];
			} else {
				return NULL ;
			}
		}
	}
	for (i = 0; i < pb->num_clock_ports; i++) {
		if (0 == strcmp(port_name, pb->clock_pins[i][0].port->name)) {
			if (pin < pb->clock_pins[i][0].port->num_pins) {
				return &pb->clock_pins[i][pin];
			} else {
				return NULL ;
			}
		}
	}
	return NULL ;
}

static void alloc_and_load_pin_locations_from_pb_graph(t_type_descriptor *type) {
	int i, j, k, m, icapacity;
	int num_sides;
	int side_index;
	int pin_num;
	int count;

	int *num_pb_graph_node_pins; /* number of pins in a set [0..num_sets-1] */
	int num_pb_graph_node_sets;
	t_pb_graph_pin*** pb_graph_node_pins;

	num_sides = 2 * (type->height + 1);
	side_index = 0;
	count = 0;

	if (type->pin_location_distribution == E_SPREAD_PIN_DISTR) {
		pin_num = 0;
		/* evenly distribute pins starting at bottom left corner */
		for (j = 0; j < 4; j++) {
			for (i = 0; i < type->height; i++) {
				if (j == TOP && i != type->height - 1) {
					continue;
				}
				if (j == BOTTOM && i != 0) {
					continue;
				}
				for (k = 0; k < (type->num_pins / num_sides) + 1; k++) {
					pin_num = side_index + k * num_sides;
					if (pin_num < type->num_pins) {
						type->pinloc[i][j][pin_num] = 1;
						type->pin_height[pin_num] = i;
						count++;
					}
				}
				side_index++;
			}
		}
		assert(side_index == num_sides);
		assert(count == type->num_pins);
	} else {
		assert(type->pin_location_distribution == E_CUSTOM_PIN_DISTR);
		for (i = 0; i < type->height; i++) {
			for (j = 0; j < 4; j++) {
				if (j == TOP && i != type->height - 1) {
					continue;
				}
				if (j == BOTTOM && i != 0) {
					continue;
				}
				for (k = 0; k < type->num_pin_loc_assignments[i][j]; k++) {
					pb_graph_node_pins =
							alloc_and_load_port_pin_ptrs_from_string(
									type->pb_type->modes[0].interconnect[0].line_num,
									type->pb_graph_head,
									type->pb_graph_head->child_pb_graph_nodes[0],
									type->pin_loc_assignments[i][j][k],
									&num_pb_graph_node_pins,
									&num_pb_graph_node_sets, FALSE, FALSE);
					assert(num_pb_graph_node_sets == 1);

					for (m = 0; m < num_pb_graph_node_pins[0]; m++) {
						pin_num =
								pb_graph_node_pins[0][m]->pin_count_in_cluster;
						assert(pin_num < type->num_pins / type->capacity);
						for (icapacity = 0; icapacity < type->capacity;
								icapacity++) {
							type->pinloc[i][j][pin_num
									+ icapacity * type->num_pins
											/ type->capacity] = 1;
							type->pin_height[pin_num] = i;
							assert(count < type->num_pins);
						}
					}
					free(pb_graph_node_pins[0]);
					free(pb_graph_node_pins);
					free(num_pb_graph_node_pins);
				}
			}
		}
	}
}

static void echo_pb_rec(const INP t_pb_graph_node *pb_graph_node, INP int level,
		INP FILE *fp) {
	int i, j, k;

	print_tabs(fp, level);
	fprintf(fp, "Physical Block: type \"%s\"  index %d  num_children %d\n",
			pb_graph_node->pb_type->name, pb_graph_node->placement_index,
			pb_graph_node->pb_type->num_pb);

	if (pb_graph_node->parent_pb_graph_node) {
		print_tabs(fp, level + 1);
		fprintf(fp, "Parent Block: type \"%s\" index %d \n",
				pb_graph_node->parent_pb_graph_node->pb_type->name,
				pb_graph_node->parent_pb_graph_node->placement_index);
	}

	print_tabs(fp, level);
	fprintf(fp, "Input Ports: total ports %d\n",
			pb_graph_node->num_input_ports);
	echo_pb_pins(pb_graph_node->input_pins, pb_graph_node->num_input_ports,
			level, fp);
	print_tabs(fp, level);
	fprintf(fp, "Output Ports: total ports %d\n",
			pb_graph_node->num_output_ports);
	echo_pb_pins(pb_graph_node->output_pins, pb_graph_node->num_output_ports,
			level, fp);
	print_tabs(fp, level);
	fprintf(fp, "Clock Ports: total ports %d\n",
			pb_graph_node->num_clock_ports);
	echo_pb_pins(pb_graph_node->clock_pins, pb_graph_node->num_clock_ports,
			level, fp);
	print_tabs(fp, level);
	for (i = 0; i < pb_graph_node->num_input_pin_class; i++) {
		fprintf(fp, "Input class %d: %d pins, ", i,
				pb_graph_node->input_pin_class_size[i]);
	}
	fprintf(fp, "\n");
	print_tabs(fp, level);
	for (i = 0; i < pb_graph_node->num_output_pin_class; i++) {
		fprintf(fp, "Output class %d: %d pins, ", i,
				pb_graph_node->output_pin_class_size[i]);
	}
	fprintf(fp, "\n");

	if (pb_graph_node->pb_type->num_modes > 0) {
		print_tabs(fp, level);
		fprintf(fp, "Children:\n");
	}
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
				j++) {
			for (k = 0;
					k
							< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
					k++) {
				echo_pb_rec(&pb_graph_node->child_pb_graph_nodes[i][j][k],
						level + 1, fp);
			}
		}
	}
}

static void echo_pb_pins(INP t_pb_graph_pin **pb_graph_pins, INP int num_ports,
		INP int level, INP FILE * fp) {
	int i, j, k, m;
	t_port *port;

	print_tabs(fp, level);

	for (i = 0; i < num_ports; i++) {
		port = pb_graph_pins[i][0].port;
		print_tabs(fp, level);
		fprintf(fp, "Port \"%s\": num_pins %d type %d parent name \"%s\"\n",
				port->name, port->num_pins, port->type,
				port->parent_pb_type->name);

		for (j = 0; j < port->num_pins; j++) {
			print_tabs(fp, level + 1);
			assert(j == pb_graph_pins[i][j].pin_number);
			assert(pb_graph_pins[i][j].port == port);
			fprintf(fp,
					"Pin %d port name \"%s\" num input edges %d num output edges %d parent type \"%s\" parent index %d\n",
					pb_graph_pins[i][j].pin_number,
					pb_graph_pins[i][j].port->name,
					pb_graph_pins[i][j].num_input_edges,
					pb_graph_pins[i][j].num_output_edges,
					pb_graph_pins[i][j].parent_node->pb_type->name,
					pb_graph_pins[i][j].parent_node->placement_index);
			print_tabs(fp, level + 2);
			if (pb_graph_pins[i][j].parent_node->pb_type->num_modes == 0) {
				fprintf(fp, "pin class (depth, pin class): ");
				for (k = 0; k < pb_graph_pins[i][j].parent_node->pb_type->depth;
						k++) {
					fprintf(fp, "(%d %d), ", k,
							pb_graph_pins[i][j].parent_pin_class[k]);
				}
				fprintf(fp, "\n");
				if (pb_graph_pins[i][j].port->type == OUT_PORT) {
					for (k = 0;
							k < pb_graph_pins[i][j].parent_node->pb_type->depth;
							k++) {
						print_tabs(fp, level + 2);
						fprintf(fp,
								"connectable input pins within depth %d: %d\n",
								k,
								pb_graph_pins[i][j].num_connectable_primtive_input_pins[k]);
						for (m = 0;
								m
										< pb_graph_pins[i][j].num_connectable_primtive_input_pins[k];
								m++) {
							print_tabs(fp, level + 3);
							fprintf(fp, "pb_graph_node %s[%d].%s[%d] \n",
									pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->parent_node->pb_type->name,
									pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->parent_node->placement_index,
									pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->port->name,
									pb_graph_pins[i][j].list_of_connectable_input_pin_ptrs[k][m]->pin_number);
						}
					}
				}
			} else {
				fprintf(fp, "pin class %d \n", pb_graph_pins[i][j].pin_class);
			}
			for (k = 0; k < pb_graph_pins[i][j].num_input_edges; k++) {
				print_tabs(fp, level + 2);
				fprintf(fp, "Input edge %d num inputs %d num outputs %d\n", k,
						pb_graph_pins[i][j].input_edges[k]->num_input_pins,
						pb_graph_pins[i][j].input_edges[k]->num_output_pins);
				print_tabs(fp, level + 3);
				fprintf(fp, "Input edge inputs\n");
				for (m = 0;
						m < pb_graph_pins[i][j].input_edges[k]->num_input_pins;
						m++) {
					print_tabs(fp, level + 3);
					fprintf(fp, "pin number %d port_name \"%s\"\n",
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->pin_number,
							pb_graph_pins[i][j].input_edges[k]->input_pins[m]->port->name);
				}
				print_tabs(fp, level + 3);
				fprintf(fp, "Input edge outputs\n");
				for (m = 0;
						m < pb_graph_pins[i][j].input_edges[k]->num_output_pins;
						m++) {
					print_tabs(fp, level + 3);
					fprintf(fp,
							"pin number %d port_name \"%s\" parent type \"%s\" parent index %d\n",
							pb_graph_pins[i][j].input_edges[k]->output_pins[m]->pin_number,
							pb_graph_pins[i][j].input_edges[k]->output_pins[m]->port->name,
							pb_graph_pins[i][j].input_edges[k]->output_pins[m]->parent_node->pb_type->name,
							pb_graph_pins[i][j].input_edges[k]->output_pins[m]->parent_node->placement_index);
				}
			}
			for (k = 0; k < pb_graph_pins[i][j].num_output_edges; k++) {
				print_tabs(fp, level + 2);
				fprintf(fp, "Output edge %d num inputs %d num outputs %d\n", k,
						pb_graph_pins[i][j].output_edges[k]->num_input_pins,
						pb_graph_pins[i][j].output_edges[k]->num_output_pins);
				print_tabs(fp, level + 3);
				fprintf(fp, "Output edge inputs\n");
				for (m = 0;
						m < pb_graph_pins[i][j].output_edges[k]->num_input_pins;
						m++) {
					print_tabs(fp, level + 3);
					fprintf(fp, "pin number %d port_name \"%s\"\n",
							pb_graph_pins[i][j].output_edges[k]->input_pins[m]->pin_number,
							pb_graph_pins[i][j].output_edges[k]->input_pins[m]->port->name);
				}
				print_tabs(fp, level + 3);
				fprintf(fp, "Output edge outputs\n");
				for (m = 0;
						m < pb_graph_pins[i][j].output_edges[k]->num_output_pins;
						m++) {
					print_tabs(fp, level + 3);
					fprintf(fp,
							"pin number %d port_name \"%s\" parent type \"%s\" parent index %d\n",
							pb_graph_pins[i][j].output_edges[k]->output_pins[m]->pin_number,
							pb_graph_pins[i][j].output_edges[k]->output_pins[m]->port->name,
							pb_graph_pins[i][j].output_edges[k]->output_pins[m]->parent_node->pb_type->name,
							pb_graph_pins[i][j].output_edges[k]->output_pins[m]->parent_node->placement_index);
				}
			}
		}
	}
}

