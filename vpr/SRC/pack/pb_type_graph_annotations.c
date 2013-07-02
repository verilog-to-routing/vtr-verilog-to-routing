/*	Jason Luu
 April 15, 2011
 Loads statistical information (min/max delays, power) onto the pb_graph.  */

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "arch_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "pb_type_graph.h"
#include "token.h"
#include "pb_type_graph_annotations.h"
#include "read_xml_arch_file.h"

static void load_pack_pattern_annotations(INP int line_num, INOUTP t_pb_graph_node *pb_graph_node,
		INP int mode, INP char *annot_in_pins, INP char *annot_out_pins,
		INP char *value);

static void load_critical_path_annotations(INP int line_num, 
		INOUTP t_pb_graph_node *pb_graph_node, INP int mode,
		INP enum e_pin_to_pin_annotation_format input_format,
		INP enum e_pin_to_pin_delay_annotations delay_type,
		INP char *annot_in_pins, INP char *annot_out_pins, INP char* value);

void load_pb_graph_pin_to_pin_annotations(INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k, m;
	const t_pb_type *pb_type;
	t_pin_to_pin_annotation *annotations;

	pb_type = pb_graph_node->pb_type;

	/* Load primitive critical path delays */
	if (pb_type->num_modes == 0) {
		annotations = pb_type->annotations;
		for (i = 0; i < pb_type->num_annotations; i++) {
			if (annotations[i].type == E_ANNOT_PIN_TO_PIN_DELAY) {
				for (j = 0; j < annotations[i].num_value_prop_pairs; j++) {
					if (annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_MAX
							|| annotations[i].prop[j]
									== E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX
							|| annotations[i].prop[j]
									== E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) {
										load_critical_path_annotations(annotations[i].line_num, pb_graph_node, OPEN,
								annotations[i].format, (enum e_pin_to_pin_delay_annotations)annotations[i].prop[j],
								annotations[i].input_pins,
								annotations[i].output_pins,
								annotations[i].value[j]);
					} else {
						assert(
								annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_MIN || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN || annotations[i].prop[j] == E_ANNOT_PIN_TO_PIN_DELAY_THOLD);
					}
				}
			} else {
				/* Todo:
				 load_hold_time_constraints_annotations(pb_graph_node); 
				 load_power_annotations(pb_graph_node);
				 */
			}
		}
	} else {
		/* Load interconnect delays */
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_interconnect; j++) {
				annotations = pb_type->modes[i].interconnect[j].annotations;
				for (k = 0;
						k < pb_type->modes[i].interconnect[j].num_annotations;
						k++) {
					if (annotations[k].type == E_ANNOT_PIN_TO_PIN_DELAY) {
						for (m = 0; m < annotations[k].num_value_prop_pairs;
								m++) {
							if (annotations[k].prop[m]
									== E_ANNOT_PIN_TO_PIN_DELAY_MAX
									|| annotations[k].prop[m]
											== E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX
									|| annotations[k].prop[m]
											== E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) {
									load_critical_path_annotations(annotations[k].line_num, pb_graph_node, i,
										annotations[k].format,
										(enum e_pin_to_pin_delay_annotations)annotations[k].prop[m],
										annotations[k].input_pins,
										annotations[k].output_pins,
										annotations[k].value[m]);
							} else {
								assert(
										annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_MIN || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN || annotations[k].prop[m] == E_ANNOT_PIN_TO_PIN_DELAY_THOLD);
							}
						}
					} else if (annotations[k].type
							== E_ANNOT_PIN_TO_PIN_PACK_PATTERN) {
						assert(annotations[k].num_value_prop_pairs == 1);
						load_pack_pattern_annotations(annotations[k].line_num, pb_graph_node, i,
								annotations[k].input_pins,
								annotations[k].output_pins,
								annotations[k].value[0]);
					} else {
						/* Todo:
						 load_hold_time_constraints_annotations(pb_graph_node); 
						 load_power_annotations(pb_graph_node);
						 */
					}
				}
			}
		}
	}

	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				load_pb_graph_pin_to_pin_annotations(
						&pb_graph_node->child_pb_graph_nodes[i][j][k]);
			}
		}
	}
}

/*
 Add the pattern name to the pack_pattern field for each pb_graph_edge that is used in a pack pattern
 */
static void load_pack_pattern_annotations(INP int line_num, INOUTP t_pb_graph_node *pb_graph_node,
		INP int mode, INP char *annot_in_pins, INP char *annot_out_pins,
		INP char *value) {
	int i, j, k, m, n, p, iedge;
	t_pb_graph_pin ***in_port, ***out_port;
	int *num_in_ptrs, *num_out_ptrs, num_in_sets, num_out_sets;
	t_pb_graph_node **children = NULL;

	children = pb_graph_node->child_pb_graph_nodes[mode];
	in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node, children,
			annot_in_pins, &num_in_ptrs, &num_in_sets, FALSE, FALSE);
	out_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node, children,
			annot_out_pins, &num_out_ptrs, &num_out_sets, FALSE, FALSE);

	/* Discover edge then annotate edge with name of pack pattern */
	k = 0;
	for (i = 0; i < num_in_sets; i++) {
		for (j = 0; j < num_in_ptrs[i]; j++) {
			p = 0;
			for (m = 0; m < num_out_sets; m++) {
				for (n = 0; n < num_out_ptrs[m]; n++) {
					for (iedge = 0; iedge < in_port[i][j]->num_output_edges;
							iedge++) {
						if (in_port[i][j]->output_edges[iedge]->output_pins[0]
								== out_port[m][n]) {
							break;
						}
					}
					/* jluu Todo: This is inefficient, I know the interconnect so I know what edges exist
					 can use this info to only annotate existing edges */
					if (iedge != in_port[i][j]->num_output_edges) {
						in_port[i][j]->output_edges[iedge]->num_pack_patterns++;
						in_port[i][j]->output_edges[iedge]->pack_pattern_names = (char**)
								my_realloc(
										in_port[i][j]->output_edges[iedge]->pack_pattern_names,
										sizeof(char*)
												* in_port[i][j]->output_edges[iedge]->num_pack_patterns);
						in_port[i][j]->output_edges[iedge]->pack_pattern_names[in_port[i][j]->output_edges[iedge]->num_pack_patterns
								- 1] = value;
					}
					p++;
				}
			}
			k++;
		}
	}

	if (in_port != NULL) {
		for (i = 0; i < num_in_sets; i++) {
			free(in_port[i]);
		}
		free(in_port);
		free(num_in_ptrs);
	}
	if (out_port != NULL) {
		for (i = 0; i < num_out_sets; i++) {
			free(out_port[i]);
		}
		free(out_port);
		free(num_out_ptrs);
	}
}

static void load_critical_path_annotations(INP int line_num, 
		INOUTP t_pb_graph_node *pb_graph_node, INP int mode,
		INP enum e_pin_to_pin_annotation_format input_format,
		INP enum e_pin_to_pin_delay_annotations delay_type,
		INP char *annot_in_pins, INP char *annot_out_pins, INP char* value) {

	int i, j, k, m, n, p, iedge;
	t_pb_graph_pin ***in_port, ***out_port;
	int *num_in_ptrs, *num_out_ptrs, num_in_sets, num_out_sets;
	float **delay_matrix;
	t_pb_graph_node **children = NULL;

	int count, prior_offset;
	int num_inputs, num_outputs;
	int num_entries_in_matrix;

	in_port = out_port = NULL;
	num_out_sets = num_in_sets = 0;
	num_out_ptrs = num_in_ptrs = NULL;

	/* Primarily 3 kinds of delays that affect critical path:
	 1.  Intrablock interconnect delays
	 2.  Combinational primitives (pin-to-pin delays of primitive)
	 3.  Sequential primitives (setup and clock-to-q times)

	 Note:	Proper I/O modelling requires knowledge of the extra-chip world (eg. the load that pin is driving, drive strength, etc)
	 For now, I/O delays are modelled as a constant in the architecture file by setting the pad-I/O block interconnect delay to be a constant I/O delay

	 Algorithm: Intrablock and combinational primitive delays apply to edges
	 Sequential delays apply to pins
	 1.  Determine if delay applies to pin or edge
	 2.  Format the delay information
	 3.  Load delay information
	 */

	/* Determine what pins to read based on delay type */
	num_inputs = num_outputs = 0;
	if (mode == OPEN) {
		children = NULL;
	} else {
		children = pb_graph_node->child_pb_graph_nodes[mode];
	}
	if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) {
		assert(pb_graph_node->pb_type->blif_model != NULL);
		in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
				children, annot_in_pins, &num_in_ptrs, &num_in_sets, FALSE,
				FALSE);
	} else if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) {
		assert(pb_graph_node->pb_type->blif_model != NULL);
		in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
				children, annot_in_pins, &num_in_ptrs, &num_in_sets, FALSE,
				FALSE);
	} else {
		assert(delay_type == E_ANNOT_PIN_TO_PIN_DELAY_MAX);
		in_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
				children, annot_in_pins, &num_in_ptrs, &num_in_sets, FALSE,
				FALSE);
		out_port = alloc_and_load_port_pin_ptrs_from_string(line_num, pb_graph_node,
				children, annot_out_pins, &num_out_ptrs, &num_out_sets, FALSE,
				FALSE);
	}

	num_inputs = 0;
	for (i = 0; i < num_in_sets; i++) {
		num_inputs += num_in_ptrs[i];
	}

	if (out_port != NULL) {
		num_outputs = 0;
		for (i = 0; i < num_out_sets; i++) {
			num_outputs += num_out_ptrs[i];
		}
	} else {
		num_outputs = 1;
	}

	delay_matrix = (float**)my_malloc(sizeof(float*) * num_inputs);
	for (i = 0; i < num_inputs; i++) {
		delay_matrix[i] = (float*)my_malloc(sizeof(float) * num_outputs);
	}

	if (input_format == E_ANNOT_PIN_TO_PIN_MATRIX) {
		if(!check_my_atof_2D(num_inputs, num_outputs, value, &num_entries_in_matrix)){
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), line_num,
					"Number of entries in matrix (%d) does not match number of pin-to-pin"
					"connections (%d).", num_entries_in_matrix, num_inputs * num_outputs);
		}
		my_atof_2D(delay_matrix, num_inputs, num_outputs, value);
	} else {
		assert(input_format == E_ANNOT_PIN_TO_PIN_CONSTANT);
		for (i = 0; i < num_inputs; i++) {
			for (j = 0; j < num_outputs; j++) {
				delay_matrix[i][j] = atof(value);
			}
		}
	}

	if (delay_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
			|| delay_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) {
		k = 0;
		for (i = 0; i < num_in_sets; i++) {
			for (j = 0; j < num_in_ptrs[i]; j++) {
				in_port[i][j]->tsu_tco = delay_matrix[k][0];
				k++;
			}
		}
	} else {
		if (pb_graph_node->pb_type->num_modes != 0) {
			/* Not a primitive, find pb_graph_edge */
			k = 0;
			for (i = 0; i < num_in_sets; i++) {
				for (j = 0; j < num_in_ptrs[i]; j++) {
					p = 0;
					for (m = 0; m < num_out_sets; m++) {
						for (n = 0; n < num_out_ptrs[m]; n++) {
							for (iedge = 0;
									iedge < in_port[i][j]->num_output_edges;
									iedge++) {
								if (in_port[i][j]->output_edges[iedge]->output_pins[0]
										== out_port[m][n]) {
									assert(
											in_port[i][j]->output_edges[iedge]->delay_max == 0);
									break;
								}
							}
							/* jluu Todo: This is inefficient, I know the interconnect so I know what edges exist
							 can use this info to only annotate existing edges */
							if (iedge != in_port[i][j]->num_output_edges) {
								in_port[i][j]->output_edges[iedge]->delay_max =
										delay_matrix[k][p];
							}
							p++;
						}
					}
					k++;
				}
			}
		} else {
			/* Primitive, allocate appropriate nodes */
			k = 0;
			for (i = 0; i < num_in_sets; i++) {
				for (j = 0; j < num_in_ptrs[i]; j++) {
					count = p = 0;
					for (m = 0; m < num_out_sets; m++) {
						for (n = 0; n < num_out_ptrs[m]; n++) {
							/* OPEN indicates that connection does not exist */
							if (delay_matrix[k][p] != OPEN) {
								count++;
							}
							p++;
						}
					}
					prior_offset = in_port[i][j]->num_pin_timing;
					in_port[i][j]->num_pin_timing = prior_offset + count;
					in_port[i][j]->pin_timing_del_max = (float*) my_realloc(in_port[i][j]->pin_timing_del_max,
							sizeof(float) * in_port[i][j]->num_pin_timing);
					in_port[i][j]->pin_timing = (t_pb_graph_pin**)my_realloc(in_port[i][j]->pin_timing,
							sizeof(t_pb_graph_pin*) * in_port[i][j]->num_pin_timing);
					p = 0;
					count = 0;
					for (m = 0; m < num_out_sets; m++) {
						for (n = 0; n < num_out_ptrs[m]; n++) {
							if (delay_matrix[k][p] != OPEN) {
								in_port[i][j]->pin_timing_del_max[prior_offset + count] =
										delay_matrix[k][p];
								in_port[i][j]->pin_timing[prior_offset + count] =
										out_port[m][n];
								count++;
							}
							p++;
						}
					}
					assert(in_port[i][j]->num_pin_timing == prior_offset + count);
					k++;
				}
			}
		}
	}
	if (in_port != NULL) {
		for (i = 0; i < num_in_sets; i++) {
			free(in_port[i]);
		}
		free(in_port);
		free(num_in_ptrs);
	}
	if (out_port != NULL) {
		for (i = 0; i < num_out_sets; i++) {
			free(out_port[i]);
		}
		free(out_port);
		free(num_out_ptrs);
	}
	for (i = 0; i < num_inputs; i++) {
		free(delay_matrix[i]);
	}
	free(delay_matrix);
}
