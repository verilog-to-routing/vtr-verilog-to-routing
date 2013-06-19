/* 
 Feasibility filter used during packing that determines if various necessary conditions for legality are met

 Important for 2 reasons:
 1) Quickly reject cases that are bad so that we don't waste time exploring useless cases in packing
 2) Robustness issue.  During packing, we have a limited size queue to store candidates to try to pack.  A good filter helps keep that queue filled with candidates likely to pass.

 1st major filter: Pin counting based on pin classes
 Rationale: If the number of a particular gruop of pins supplied by the pb_graph_node in the architecture is insufficient to meet a candidate packing solution's demand for that group of pins, then that
 candidate solution is for sure invalid without any further legalization checks.  For example, if a candidate solution requires 2 clock pins but the architecture only has one clock, then that solution
 can't be legal.

 Implementation details:
 a) Definition of a pin class - If there exists a path (ignoring directionality of connections) from pin A to pin B and pin A and pin B are of the same type (input, output, or clock), then pin A and pin B are in the same pin class.  Otherwise, pin A and pin B are in different pin classes.
 b) Code Identifies pin classes.  Given a candidate solution  

 TODO: May 30, 2012 Jason Luu - Must take into consideration modes when doing pin counting.  For fracturable LUTs FI = 5, the soft logic block sees 6 pins instead of 5 pins for the dual LUT mode messing up the pin counter.  The packer still produces correct results but runs slower than its best (experiment on a modified architecture file that forces correct pin counting shows 40x speedup vs VPR 6.0 as opposed to 3x speedup at the time)

 Author: Jason Luu
 Date: May 16, 2012
 */

#include <assert.h>

#include "read_xml_arch_file.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "cluster_feasibility_filter.h"
#include "vpr_utils.h"
#include "ReadOptions.h"

/* header functions that identify pin classes */
static void alloc_pin_classes_in_pb_graph_node(
		INOUTP t_pb_graph_node *pb_graph_node);
static int get_max_depth_of_pb_graph_node(INP t_pb_graph_node *pb_graph_node);
static void load_pin_class_by_depth(INOUTP t_pb_graph_node *pb_graph_node,
		INP int depth, OUTP int *input_count, OUTP int *output_count);
static void load_list_of_connectable_input_pin_ptrs(
		INOUTP t_pb_graph_node *pb_graph_node);
static void expand_pb_graph_node_and_load_output_to_input_connections(
		INOUTP t_pb_graph_pin *current_pb_graph_pin,
		INOUTP t_pb_graph_pin *reference_pin, INP int depth);
static void unmark_fanout_intermediate_nodes(
		INOUTP t_pb_graph_pin *current_pb_graph_pin);
static void reset_pin_class_scratch_pad_rec(
		INOUTP t_pb_graph_node *pb_graph_node);
static void expand_pb_graph_node_and_load_pin_class_by_depth(
		INOUTP t_pb_graph_pin *current_pb_graph_pin,
		INP t_pb_graph_pin *reference_pb_graph_pin, INP int depth,
		OUTP int *input_count, OUTP int *output_count);
static void sum_pin_class(INOUTP t_pb_graph_node *pb_graph_node);

static void discover_all_forced_connections(INOUTP t_pb_graph_node *pb_graph_node);
static boolean is_forced_connection(INP t_pb_graph_pin *pb_graph_pin);


/* Identify all pin class information for complex block
 */
void load_pin_classes_in_pb_graph_head(INOUTP t_pb_graph_node *pb_graph_node) {
	int i, depth, input_count, output_count;

	/* Allocate memory for primitives */
	alloc_pin_classes_in_pb_graph_node(pb_graph_node);

	/* Load pin classes */
	depth = get_max_depth_of_pb_graph_node(pb_graph_node);
	for (i = 0; i < depth; i++) {
		input_count = output_count = 0;
		reset_pin_class_scratch_pad_rec(pb_graph_node);
		load_pin_class_by_depth(pb_graph_node, i, &input_count, &output_count);
	}

	/* Load internal output-to-input connections within each cluster */
	reset_pin_class_scratch_pad_rec(pb_graph_node);
	load_list_of_connectable_input_pin_ptrs(pb_graph_node);
	discover_all_forced_connections(pb_graph_node);
}

/**
 * Recursive function to allocate memory space for pin classes in primitives
 */
static void alloc_pin_classes_in_pb_graph_node(
		INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;

	/* If primitive, allocate space, else go to primitive */
	if (pb_graph_node->pb_type->num_modes == 0) {
		/* allocate space */
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
				pb_graph_node->input_pins[i][j].parent_pin_class =
						(int *) my_calloc(pb_graph_node->pb_type->depth,
								sizeof(int*));
				for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
					pb_graph_node->input_pins[i][j].parent_pin_class[k] = OPEN;
				}
			}
		}
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				pb_graph_node->output_pins[i][j].parent_pin_class =
						(int *) my_calloc(pb_graph_node->pb_type->depth,
								sizeof(int*));
				pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs =
						(t_pb_graph_pin ***) my_calloc(
								pb_graph_node->pb_type->depth,
								sizeof(t_pb_graph_pin**));
				pb_graph_node->output_pins[i][j].num_connectable_primtive_input_pins =
						(int*) my_calloc(pb_graph_node->pb_type->depth,
								sizeof(int));
				for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
					pb_graph_node->output_pins[i][j].parent_pin_class[k] = OPEN;
				}
			}
		}
		for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
			for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
				pb_graph_node->clock_pins[i][j].parent_pin_class =
						(int *) my_calloc(pb_graph_node->pb_type->depth,
								sizeof(int*));
				for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
					pb_graph_node->clock_pins[i][j].parent_pin_class[k] = OPEN;
				}
			}
		}
	} else {
		for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k
								< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					alloc_pin_classes_in_pb_graph_node(
							&pb_graph_node->child_pb_graph_nodes[i][j][k]);
				}
			}
		}
	}
}

/* determine maximum depth of pb_graph_node */
static int get_max_depth_of_pb_graph_node(INP t_pb_graph_node *pb_graph_node) {
	int i, j, k;
	int max_depth, depth;

	max_depth = 0;

	/* If primitive, allocate space, else go to primitive */
	if (pb_graph_node->pb_type->num_modes == 0) {
		return pb_graph_node->pb_type->depth;
	} else {
		for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k
								< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					depth = get_max_depth_of_pb_graph_node(
							&pb_graph_node->child_pb_graph_nodes[i][j][k]);
					if (depth > max_depth) {
						max_depth = depth;
					}
				}
			}
		}
	}

	return max_depth;
}

static void reset_pin_class_scratch_pad_rec(
		INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;

	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			pb_graph_node->input_pins[i][j].scratch_pad = OPEN;
		}
	}
	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			pb_graph_node->output_pins[i][j].scratch_pad = OPEN;
		}
	}
	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			pb_graph_node->clock_pins[i][j].scratch_pad = OPEN;
		}
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
				j++) {
			for (k = 0;
					k
							< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
					k++) {
				reset_pin_class_scratch_pad_rec(
						&pb_graph_node->child_pb_graph_nodes[i][j][k]);
			}
		}
	}
}

/* load pin class based on limited depth */
static void load_pin_class_by_depth(INOUTP t_pb_graph_node *pb_graph_node,
		INP int depth, OUTP int *input_count, OUTP int *output_count) {
	int i, j, k;

	if (pb_graph_node->pb_type->num_modes == 0) {
		if (pb_graph_node->pb_type->depth > depth) {
			/* At primitive, determine which pin class each of its pins belong to */
			for (i = 0; i < pb_graph_node->num_input_ports; i++) {
				for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
					if (pb_graph_node->input_pins[i][j].parent_pin_class[depth]
							== OPEN) {
						expand_pb_graph_node_and_load_pin_class_by_depth(
								&pb_graph_node->input_pins[i][j],
								&pb_graph_node->input_pins[i][j], depth,
								input_count, output_count);
						(*input_count)++;
					}
				}
			}
			for (i = 0; i < pb_graph_node->num_output_ports; i++) {
				for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
					if (pb_graph_node->output_pins[i][j].parent_pin_class[depth]
							== OPEN) {
						expand_pb_graph_node_and_load_pin_class_by_depth(
								&pb_graph_node->output_pins[i][j],
								&pb_graph_node->output_pins[i][j], depth,
								input_count, output_count);
						(*output_count)++;
					}
				}
			}
			for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
				for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
					if (pb_graph_node->clock_pins[i][j].parent_pin_class[depth]
							== OPEN) {
						expand_pb_graph_node_and_load_pin_class_by_depth(
								&pb_graph_node->clock_pins[i][j],
								&pb_graph_node->clock_pins[i][j], depth,
								input_count, output_count);
						(*input_count)++;
					}
				}
			}
		}
	}

	if (pb_graph_node->pb_type->depth == depth) {
		/* Load pin classes for all pb_graph_nodes of this depth, therefore, at a particular pb_graph_node of this depth, set # of pin classes to be 0 */
		*input_count = 0;
		*output_count = 0;
		for (i = 0; i < pb_graph_node->num_input_ports; i++) {
			for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
				pb_graph_node->input_pins[i][j].pin_class = OPEN;
			}
		}
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				pb_graph_node->output_pins[i][j].pin_class = OPEN;
			}
		}
		for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
			for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
				pb_graph_node->clock_pins[i][j].pin_class = OPEN;
			}
		}
	}

	/* Expand down to primitives */
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
				j++) {
			for (k = 0;
					k
							< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
					k++) {
				load_pin_class_by_depth(
						&pb_graph_node->child_pb_graph_nodes[i][j][k], depth,
						input_count, output_count);
			}
		}
	}

	if (pb_graph_node->pb_type->depth == depth
			&& pb_graph_node->pb_type->num_modes != 0) {
		/* Record pin class information for cluster */
		pb_graph_node->num_input_pin_class = *input_count + 1; /* number of input pin classes discovered + 1 for primitive inputs not reachable from cluster input pins */
		pb_graph_node->input_pin_class_size = (int*) my_calloc(*input_count + 1,
				sizeof(int));
		pb_graph_node->num_output_pin_class = *output_count + 1; /* number of output pin classes discovered + 1 for primitive inputs not reachable from cluster input pins */
		pb_graph_node->output_pin_class_size = (int*) my_calloc(*output_count + 1,
				sizeof(int));
		sum_pin_class(pb_graph_node);
	}

}

/**
 * Load internal output-to-input connections within each cluster 
 */
static void load_list_of_connectable_input_pin_ptrs(
		INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;

	if (pb_graph_node->pb_type->num_modes == 0) {
		/* If this is a primitive, discover what input pins the output pins can connect to */
		for (i = 0; i < pb_graph_node->num_output_ports; i++) {
			for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
					expand_pb_graph_node_and_load_output_to_input_connections(
							&pb_graph_node->output_pins[i][j],
							&pb_graph_node->output_pins[i][j], k);
					unmark_fanout_intermediate_nodes(
							&pb_graph_node->output_pins[i][j]);
				}
			}
		}
	}
	/* Expand down to primitives */
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
				j++) {
			for (k = 0;k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
					k++) {
				load_list_of_connectable_input_pin_ptrs(
						&pb_graph_node->child_pb_graph_nodes[i][j][k]);
			}
		}
	}
}

/*	Traverse outputs of output pin or primitive to see what input pins it reaches 
 Record list of input pins based on depth
 */
static void expand_pb_graph_node_and_load_output_to_input_connections(
		INOUTP t_pb_graph_pin *current_pb_graph_pin,
		INOUTP t_pb_graph_pin *reference_pin, INP int depth) {
	int i;

	if (current_pb_graph_pin->scratch_pad == OPEN
			&& current_pb_graph_pin->parent_node->pb_type->depth > depth) {
		current_pb_graph_pin->scratch_pad = 1;
		for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
			assert(current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
			expand_pb_graph_node_and_load_output_to_input_connections(
					current_pb_graph_pin->output_edges[i]->output_pins[0],
					reference_pin, depth);
		}
		if (current_pb_graph_pin->parent_node->pb_type->num_modes == 0
				&& current_pb_graph_pin->port->type == IN_PORT) {
			reference_pin->num_connectable_primtive_input_pins[depth]++;
			reference_pin->list_of_connectable_input_pin_ptrs[depth] =
					(t_pb_graph_pin**) my_realloc(
							reference_pin->list_of_connectable_input_pin_ptrs[depth],
							reference_pin->num_connectable_primtive_input_pins[depth]
									* sizeof(t_pb_graph_pin*));
			reference_pin->list_of_connectable_input_pin_ptrs[depth][reference_pin->num_connectable_primtive_input_pins[depth]
					- 1] = current_pb_graph_pin;
		}
	}
}

/**
 * Clear scratch_pad for all fanout of pin
 */
static void unmark_fanout_intermediate_nodes(
		INOUTP t_pb_graph_pin *current_pb_graph_pin) {
	int i;
	if (current_pb_graph_pin->scratch_pad != OPEN) {
		current_pb_graph_pin->scratch_pad = OPEN;
		for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
			assert(current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
			unmark_fanout_intermediate_nodes(
					current_pb_graph_pin->output_edges[i]->output_pins[0]);
		}
	}
}

/**
 * Determine other primitive pins that belong to the same pin class as reference pin
 */
static void expand_pb_graph_node_and_load_pin_class_by_depth(
		INOUTP t_pb_graph_pin *current_pb_graph_pin,
		INP t_pb_graph_pin *reference_pb_graph_pin, INP int depth,
		OUTP int *input_count, OUTP int *output_count) {
	int i;
	int marker;
	int active_pin_class;

	if (reference_pb_graph_pin->port->type == IN_PORT) {
		marker = *input_count + 10;
		active_pin_class = *input_count;
	} else {
		marker = -10 - *output_count;
		active_pin_class = *output_count;
	}
	assert(reference_pb_graph_pin->parent_node->pb_type->num_modes == 0);
	assert(current_pb_graph_pin->parent_node->pb_type->depth >= depth);
	assert(current_pb_graph_pin->port->type != INOUT_PORT);
	if (current_pb_graph_pin->scratch_pad != marker) {
		if (current_pb_graph_pin->parent_node->pb_type->num_modes == 0) {
			current_pb_graph_pin->scratch_pad = marker;
			/* This is a primitive, determine what pins cans share the same pin class as the reference pin */
			if (current_pb_graph_pin->parent_pin_class[depth] == OPEN
					&& reference_pb_graph_pin->port->is_clock
							== current_pb_graph_pin->port->is_clock
					&& reference_pb_graph_pin->port->type
							== current_pb_graph_pin->port->type) {
				current_pb_graph_pin->parent_pin_class[depth] =
						active_pin_class;
			}
			for (i = 0; i < current_pb_graph_pin->num_input_edges; i++) {
				assert(
						current_pb_graph_pin->input_edges[i]->num_input_pins == 1);
				expand_pb_graph_node_and_load_pin_class_by_depth(
						current_pb_graph_pin->input_edges[i]->input_pins[0],
						reference_pb_graph_pin, depth, input_count,
						output_count);
			}
			for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
				assert(
						current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
				expand_pb_graph_node_and_load_pin_class_by_depth(
						current_pb_graph_pin->output_edges[i]->output_pins[0],
						reference_pb_graph_pin, depth, input_count,
						output_count);
			}
		} else if (current_pb_graph_pin->parent_node->pb_type->depth == depth) {
			current_pb_graph_pin->scratch_pad = marker;
			if (current_pb_graph_pin->port->type == OUT_PORT) {
				if (reference_pb_graph_pin->port->type == OUT_PORT) {
					/* This cluster's output pin can be driven by primitive outputs belonging to this pin class */
					current_pb_graph_pin->pin_class = active_pin_class;
				}
				for (i = 0; i < current_pb_graph_pin->num_input_edges; i++) {
					assert(
							current_pb_graph_pin->input_edges[i]->num_input_pins == 1);
					expand_pb_graph_node_and_load_pin_class_by_depth(
							current_pb_graph_pin->input_edges[i]->input_pins[0],
							reference_pb_graph_pin, depth, input_count,
							output_count);
				}
			}
			if (current_pb_graph_pin->port->type == IN_PORT) {
				if (reference_pb_graph_pin->port->type == IN_PORT) {
					/* This cluster's input pin can drive the primitive input pins belonging to this pin class */
					current_pb_graph_pin->pin_class = active_pin_class;
				}
				for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
					assert(
							current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
					expand_pb_graph_node_and_load_pin_class_by_depth(
							current_pb_graph_pin->output_edges[i]->output_pins[0],
							reference_pb_graph_pin, depth, input_count,
							output_count);
				}
			}
		} else if (current_pb_graph_pin->parent_node->pb_type->depth > depth) {
			/* Inside an intermediate cluster, traverse to either a primitive or to the cluster we're interested in populating */
			current_pb_graph_pin->scratch_pad = marker;
			for (i = 0; i < current_pb_graph_pin->num_input_edges; i++) {
				assert(
						current_pb_graph_pin->input_edges[i]->num_input_pins == 1);
				expand_pb_graph_node_and_load_pin_class_by_depth(
						current_pb_graph_pin->input_edges[i]->input_pins[0],
						reference_pb_graph_pin, depth, input_count,
						output_count);
			}
			for (i = 0; i < current_pb_graph_pin->num_output_edges; i++) {
				assert(
						current_pb_graph_pin->output_edges[i]->num_output_pins == 1);
				expand_pb_graph_node_and_load_pin_class_by_depth(
						current_pb_graph_pin->output_edges[i]->output_pins[0],
						reference_pb_graph_pin, depth, input_count,
						output_count);
			}
		}
	}
}

/* count up pin classes of the same number for the given cluster */
static void sum_pin_class(INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j;

	/* This is a primitive, for each pin in primitive, sum appropriate pin class */
	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			assert(
					pb_graph_node->input_pins[i][j].pin_class < pb_graph_node->num_input_pin_class);
			if (pb_graph_node->input_pins[i][j].pin_class == OPEN) {
				vpr_printf_warning(__FILE__, __LINE__, 
						"%s[%d].%s[%d] unconnected pin in architecture.\n",
						pb_graph_node->pb_type->name,
						pb_graph_node->placement_index,
						pb_graph_node->input_pins[i][j].port->name,
						pb_graph_node->input_pins[i][j].pin_number);
				continue;
			}
			pb_graph_node->input_pin_class_size[pb_graph_node->input_pins[i][j].pin_class]++;
		}
	}
	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			assert(
					pb_graph_node->output_pins[i][j].pin_class < pb_graph_node->num_output_pin_class);
			if (pb_graph_node->output_pins[i][j].pin_class == OPEN) {
				vpr_printf_warning(__FILE__, __LINE__, 
						"%s[%d].%s[%d] unconnected pin in architecture.\n",
						pb_graph_node->pb_type->name,
						pb_graph_node->placement_index,
						pb_graph_node->output_pins[i][j].port->name,
						pb_graph_node->output_pins[i][j].pin_number);
				continue;
			}
			pb_graph_node->output_pin_class_size[pb_graph_node->output_pins[i][j].pin_class]++;
		}
	}
	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			assert(
					pb_graph_node->clock_pins[i][j].pin_class < pb_graph_node->num_input_pin_class);
			if (pb_graph_node->clock_pins[i][j].pin_class == OPEN) {
				vpr_printf_warning(__FILE__, __LINE__, 
						"%s[%d].%s[%d] unconnected pin in architecture.\n",
						pb_graph_node->pb_type->name,
						pb_graph_node->placement_index,
						pb_graph_node->clock_pins[i][j].port->name,
						pb_graph_node->clock_pins[i][j].pin_number);
				continue;
			}
			pb_graph_node->input_pin_class_size[pb_graph_node->clock_pins[i][j].pin_class]++;
		}
	}
}

/* Recursively visit all pb_graph_pins and determine primitive output pins that connect to nothing else than one primitive input pin.  If a net maps to this output pin, then the primitive corresponding to that input must be used */
static void discover_all_forced_connections(INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;

	/* If primitive, allocate space, else go to primitive */
	if (pb_graph_node->pb_type->num_modes == 0) {
		for(i = 0; i < pb_graph_node->num_output_ports; i++) {
			for(j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
				pb_graph_node->output_pins[i][j].is_forced_connection = is_forced_connection(&pb_graph_node->output_pins[i][j]);
			}
		}
	} else {
		for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					discover_all_forced_connections(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
				}
			}
		}
	}
}

/**
 * Given an output pin, determine if it connects to only one input pin and nothing else. 
 */
static boolean is_forced_connection(INP t_pb_graph_pin *pb_graph_pin) {
	if(pb_graph_pin->num_output_edges > 1) {
		return FALSE;
	}
	if(pb_graph_pin->num_output_edges == 0) {
		if(pb_graph_pin->parent_node->pb_type->num_modes == 0) {
			/* Check that this pin belongs to a primitive */
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return is_forced_connection(pb_graph_pin->output_edges[0]->output_pins[0]);
}




