/*********************************************************************
 *  The following code is part of the power modelling feature of VTR.
 *
 * For support:
 * http://code.google.com/p/vtr-verilog-to-routing/wiki/Power
 *
 * or email:
 * vtr.power.estimation@gmail.com
 *
 * If you are using power estimation for your researach please cite:
 *
 * Jeffrey Goeders and Steven Wilton.  VersaPower: Power Estimation
 * for Diverse FPGA Architectures.  In International Conference on
 * Field Programmable Technology, 2012.
 *
 ********************************************************************/

/**
 * This is the top-level file for power estimation in VTR
 */

/************************* INCLUDES *********************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "power.h"
#include "power_components.h"
#include "power_util.h"
#include "power_lowlevel.h"
#include "power_sizing.h"
#include "power_verify.h"
#include "power_cmos_tech.h"

#include "physical_types.h"
#include "globals.h"
#include "rr_graph.h"
#include "vpr_utils.h"
#include "ezxml.h"
#include "read_xml_util.h"

/************************* DEFINES **********************************/
#define CONVERT_NM_PER_M 1000000000
#define CONVERT_UM_PER_M 1000000

/************************* ENUMS ************************************/

/************************* GLOBALS **********************************/
t_solution_inf g_solution_inf;
t_power_output * g_power_output;
t_power_commonly_used * g_power_commonly_used;
t_power_tech * g_power_tech;
t_power_arch * g_power_arch;
static t_rr_node_power * rr_node_power;

/************************* Function Declarations ********************/
/* Routing */
static void power_calc_routing(t_power_usage * power_usage,
		t_det_routing_arch * routing_arch, t_segment_inf * segment_inf);

/* Tiles */
static void power_calc_blocks(t_power_usage * power_usage);
static void power_calc_pb(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node);
static void power_calc_pb_rec(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_node);
static void power_calc_primitive(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node);
static void power_reset_tile_usage(void);
static void power_reset_pb_type(t_pb_type * pb_type);

/* Clock */
static void power_calc_clock(t_power_usage * power_usage,
		t_clock_arch * clock_arch);
static void power_calc_clock_single(t_power_usage * power_usage,
		t_clock_network * clock_inf);

/* Init/Uninit */
static void dealloc_mux_graph(t_mux_node * node);
static void dealloc_mux_graph_recursive(t_mux_node * node);

/* Printing */
static void power_print_clb_detailed(FILE * fp);
static void power_print_pb_usage_recursive(FILE * fp, t_pb_type * type,
		int indent_level, float parent_power, float total_power);
static void power_print_clb_summary_rec(FILE * fp, t_pb_type * pb_type,
		int indent);

/************************* FUNCTION DEFINITIONS *********************/
/**
 *  This function calculates the power of primitives (ff, lut, etc),
 *  by calling the appropriate primitive function.
 *  - power_usage: (Return value)
 *  - pb: The pysical block
 *  - pb_graph_node: The physical block graph node
 *  - calc_dynamic: Calculate dynamic power? Otherwise ignore
 *  - calc_static: Calculate static power? Otherwise ignore
 */
static void power_calc_primitive(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node) {
	t_power_usage sub_power_usage;
	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;

	if (strcmp(pb_graph_node->pb_type->blif_model, ".names") == 0) {
		/* LUT */

		char * SRAM_values;
		float * input_probabilities;
		float * input_densities;
		int LUT_size;
		int pin_idx;

		assert(pb_graph_node->num_input_ports == 1);

		LUT_size = pb_graph_node->num_input_pins[0];

		input_probabilities = (float*) my_calloc(LUT_size, sizeof(float));
		input_densities = (float*) my_calloc(LUT_size, sizeof(float));

		for (pin_idx = 0; pin_idx < LUT_size; pin_idx++) {
			t_pb_graph_pin * pin = &pb_graph_node->input_pins[0][pin_idx];

			input_probabilities[pin_idx] = pin_prob(pb, pin);
			input_densities[pin_idx] = pin_dens(pb, pin);
		}

		if (pb) {
			SRAM_values = alloc_SRAM_values_from_truth_table(LUT_size,
					logical_block[pb->logical_block].truth_table);
		} else {
			SRAM_values = alloc_SRAM_values_from_truth_table(LUT_size, NULL );
		}
		power_calc_LUT(&sub_power_usage, LUT_size, SRAM_values,
				input_probabilities, input_densities);
		power_add_usage(power_usage, &sub_power_usage);
		free(SRAM_values);
		free(input_probabilities);
		free(input_densities);
	} else if (strcmp(pb_graph_node->pb_type->blif_model, ".latch") == 0) {
		/* Flip-Flop */

		t_pb_graph_pin * D_pin = &pb_graph_node->input_pins[0][0];
		t_pb_graph_pin * Q_pin = &pb_graph_node->output_pins[0][0];

		float D_dens = 0.;
		float D_prob = 0.;
		float Q_prob = 0.;
		float Q_dens = 0.;
		float clk_dens = 0.;
		float clk_prob = 0.;

		D_dens = pin_dens(pb, D_pin);
		D_prob = pin_prob(pb, D_pin);
		Q_dens = pin_dens(pb, Q_pin);
		Q_prob = pin_prob(pb, Q_pin);

		clk_prob = g_clock_arch->clock_inf[0].prob;
		clk_dens = g_clock_arch->clock_inf[0].dens;

		power_calc_FF(&sub_power_usage, D_prob, D_dens, Q_prob, Q_dens,
				clk_prob, clk_dens);
		power_add_usage(power_usage, &sub_power_usage);

	} else {
		char msg[BUFSIZE];
		power_usage->dynamic = 0.;
		power_usage->leakage = 0.;

		sprintf(msg, "No dynamic power defined for BLIF model: %s",
				pb_graph_node->pb_type->blif_model);
		power_log_msg(POWER_LOG_WARNING, msg);

		sprintf(msg, "No leakage power defined for BLIF model: %s",
				pb_graph_node->pb_type->blif_model);
		power_log_msg(POWER_LOG_WARNING, msg);

	}

}

void power_calc_local_pin_toggle(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_pin * pin) {
	power_zero_usage(power_usage);

	/* Divide by 2 because density is switches/cycle, but a toggle is 2 switches */
	power_usage->dynamic += pin->port->port_power->energy_per_toggle
			* pin_dens(pb, pin) / 2.0;
}

void power_usage_local_pin_buffer_and_wire(t_power_usage * power_usage,
		t_pb * pb, t_pb_graph_pin * pin) {
	t_power_usage sub_power_usage;
	float buffer_size = 0.;
	double C_wire;

	power_zero_usage(power_usage);

	/* Wire switching */
	C_wire = pin->pin_power->C_wire;
	power_usage_wire(&sub_power_usage, C_wire, pin_dens(pb, pin));
	power_add_usage(power_usage, &sub_power_usage);

	/* Buffer power */
	if (buffer_size) {
		power_usage_buffer(&sub_power_usage, buffer_size, pin_prob(pb, pin),
				pin_dens(pb, pin), FALSE, 0);
		power_add_usage(power_usage, &sub_power_usage);
	}
}

void power_usage_local_buffers_and_wires(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_node) {
	int port_idx;
	int pin_idx;
	t_power_usage pin_power;

	power_zero_usage(power_usage);

	/* Input pins */
	for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
				pin_idx++) {
			power_usage_local_pin_buffer_and_wire(&pin_power, pb,
					&pb_node->input_pins[port_idx][pin_idx]);
			power_add_usage(power_usage, &pin_power);
		}
	}

	/* Output pins */
	for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
				pin_idx++) {
			power_usage_local_pin_buffer_and_wire(&pin_power, pb,
					&pb_node->output_pins[port_idx][pin_idx]);
			power_add_usage(power_usage, &pin_power);
		}
	}

	/* Clock pins */
	for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
				pin_idx++) {
			power_usage_local_pin_buffer_and_wire(&pin_power, pb,
					&pb_node->clock_pins[port_idx][pin_idx]);
			power_add_usage(power_usage, &pin_power);
		}
	}
}

/**
 * Calculate the power of a pysical block
 */
static void power_calc_pb(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node) {
	power_calc_pb_rec(power_usage, pb, pb_graph_node);
}

/** Calculates the power of a pb:
 * First checks if dynamic/static power is provided by user in arch file.  If not:
 * - Calculate power of all interconnect
 * - Call recursively for children
 * - If no children, must be a primitive.  Call primitive hander.
 */
static void power_calc_pb_rec(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_node) {
	t_power_usage sub_power_usage;
	int pb_type_idx;
	int pb_idx;
	int interc_idx;
	int pb_mode;
	int port_idx;
	int pin_idx;
	float dens_avg;
	int num_pins;

	power_zero_usage(power_usage);

	t_pb_type * pb_type = pb_node->pb_type;
	t_pb_type_power * pb_power = pb_node->pb_type->pb_type_power;

	boolean estimate_buffers_and_wire = FALSE;
	boolean estimate_multiplexers = FALSE;
	boolean estimate_primitives = FALSE;

	/* Get mode */
	if (pb) {
		pb_mode = pb->mode;
	} else {
		/* Default mode if not initialized (will only affect leakage power) */
		pb_mode = pb_type->pb_type_power->leakage_default_mode;
	}

	switch (pb_node->pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_ABSOLUTE:
		power_add_usage(power_usage, &pb_power->absolute_power_per_instance);
		break;

	case POWER_METHOD_C_INTERNAL:
		/* Just take the average density of inputs pins and use
		 * that with user-defined block capacitance and leakage */

		/* Average the activity of all pins */
		num_pins = 0;
		for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
					pin_idx++) {
				dens_avg += pin_dens(pb,
						&pb_node->input_pins[port_idx][pin_idx]);
				num_pins++;
			}
		}
		if (num_pins != 0) {
			dens_avg = dens_avg / num_pins;
		}
		power_usage->dynamic += power_calc_node_switching(pb_power->C_internal,
				dens_avg);

		/* Leakage is an absolute */
		power_usage->leakage += pb_power->absolute_power_per_instance.leakage;
		break;

	case POWER_METHOD_TOGGLE_PINS:
		/* Static is supplied as an absolute */
		power_usage->leakage += pb_power->absolute_power_per_instance.leakage;

		/* Add toggle power of each input pin */
		for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_calc_local_pin_toggle(&pin_power, pb,
						&pb_node->input_pins[port_idx][pin_idx]);
				power_add_usage(power_usage, &pin_power);
			}
		}

		/* Add toggle power of each output pin */
		for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_calc_local_pin_toggle(&pin_power, pb,
						&pb_node->output_pins[port_idx][pin_idx]);
				power_add_usage(power_usage, &pin_power);
			}
		}

		/* Add toggle power of each clock pin */
		for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_calc_local_pin_toggle(&pin_power, pb,
						&pb_node->clock_pins[port_idx][pin_idx]);
				power_add_usage(power_usage, &pin_power);
			}
		}
		break;
	case POWER_METHOD_SPECIFY_SIZES:
		estimate_buffers_and_wire = TRUE;
		estimate_multiplexers = TRUE;
		estimate_primitives = TRUE;
		break;
	case POWER_METHOD_AUTO_SIZES:
		estimate_buffers_and_wire = TRUE;
		estimate_multiplexers = TRUE;
		estimate_primitives = TRUE;
		break;
	case POWER_METHOD_UNDEFINED:
	default:
		assert(0);
		break;
	}

	if (pb_node->pb_type->class_type == LUT_CLASS) {
		/* LUTs will have a child node that is used to indicate pin
		 * equivalence for routing purposes.
		 * There is a crossbar to the child node; however,
		 * this interconnect does not exist in FPGA hardware and should
		 * be ignored for power calculations. */
		estimate_buffers_and_wire = FALSE;
		estimate_multiplexers = FALSE;
	}

	if (pb_node->pb_type->num_modes == 0) {
		/* This is a leaf node, which is a primitive (lut, ff, etc) */
		if (estimate_primitives) {
			assert(pb_node->pb_type->blif_model);

			power_calc_primitive(&sub_power_usage, pb, pb_node);
			power_add_usage(power_usage, &sub_power_usage);
		}

	} else {
		/* This node had children.  The power of this node is the sum of:
		 *  - Buffers/Wires in Interconnect from Parent to children
		 *  - Multiplexers in Interconnect from Parent to children
		 *  - Child nodes
		 */

		if (estimate_buffers_and_wire) {
			/* Check pins of all interconnect */
			power_usage_local_buffers_and_wires(&sub_power_usage, pb, pb_node);
			power_add_usage(power_usage, &sub_power_usage);
			power_component_add_usage(&sub_power_usage,
					POWER_COMPONENT_LOCAL_BUFFERS_AND_WIRE);
		}

		/* Interconnect Structures (multiplexers) */
		if (estimate_multiplexers) {

			for (interc_idx = 0;
					interc_idx < pb_type->modes[pb_mode].num_interconnect;
					interc_idx++) {

				power_calc_local_interc_mux(&sub_power_usage, pb,
						&pb_node->interconnect_pins[pb_mode][interc_idx]);
				power_add_usage(power_usage, &sub_power_usage);
				power_component_add_usage(&sub_power_usage,
						POWER_COMPONENT_LOCAL_INTERC_MUXES);
			}
		}

		/* Add power for children */
		for (pb_type_idx = 0;
				pb_type_idx
						< pb_node->pb_type->modes[pb_mode].num_pb_type_children;
				pb_type_idx++) {
			for (pb_idx = 0;
					pb_idx
							< pb_node->pb_type->modes[pb_mode].pb_type_children[pb_type_idx].num_pb;
					pb_idx++) {
				t_pb * child_pb = NULL;
				t_pb_graph_node * child_pb_graph_node;

				if (pb && pb->child_pbs[pb_type_idx][pb_idx].name) {
					/* Child is initialized */
					child_pb = &pb->child_pbs[pb_type_idx][pb_idx];
				}
				child_pb_graph_node =
						&pb_node->child_pb_graph_nodes[pb_mode][pb_type_idx][pb_idx];

				power_calc_pb_rec(&sub_power_usage, child_pb,
						child_pb_graph_node);
				power_add_usage(power_usage, &sub_power_usage);
			}
		}
		power_add_usage(
				&pb_node->pb_type->modes[pb_mode].mode_power->power_usage,
				power_usage);
	}

	power_add_usage(&pb_node->pb_type->pb_type_power->power_usage, power_usage);
}

/* Resets the power stats for all physical blocks */
static void power_reset_pb_type(t_pb_type * pb_type) {
	int mode_idx;
	int child_idx;
	int interc_idx;

	power_zero_usage(&pb_type->pb_type_power->power_usage);

	for (mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
		power_zero_usage(&pb_type->modes[mode_idx].mode_power->power_usage);

		for (child_idx = 0;
				child_idx < pb_type->modes[mode_idx].num_pb_type_children;
				child_idx++) {
			power_reset_pb_type(
					&pb_type->modes[mode_idx].pb_type_children[child_idx]);
		}
		for (interc_idx = 0;
				interc_idx < pb_type->modes[mode_idx].num_interconnect;
				interc_idx++) {
			power_zero_usage(
					&pb_type->modes[mode_idx].interconnect[interc_idx].interconnect_power->power_usage);
		}
	}
}

/**
 * Resets the power usage for all tile types
 */
static void power_reset_tile_usage(void) {
	int type_idx;

	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_type) {
			power_reset_pb_type(type_descriptors[type_idx].pb_type);
		}
	}
}

/*
 * Calcultes the power usage of all tiles in the FPGA
 */
static void power_calc_blocks(t_power_usage * power_usage) {
	int x, y, z;
	int type_idx;

	power_zero_usage(power_usage);

	power_reset_tile_usage();

	/* Loop through all grid locations */
	for (x = 0; x < nx + 2; x++) {
		for (y = 0; y < ny + 2; y++) {
			type_idx = grid[x][y].type->index;

			if ((grid[x][y].offset != 0) || (grid[x][y].type == EMPTY_TYPE)) {
				continue;
			}

			for (z = 0; z < grid[x][y].type->capacity; z++) {
				t_pb * pb = NULL;
				t_power_usage pb_power;

				if (grid[x][y].blocks[z] != OPEN) {
					pb = block[grid[x][y].blocks[z]].pb;
				}

				/* Calculate power of this CLB */
				power_calc_pb(&pb_power, pb, grid[x][y].type->pb_graph_head);
				power_add_usage(power_usage, &pb_power);
			}
		}
	}
	return;
}

/**
 * Calculates the total power usage from the clock network
 */
static void power_calc_clock(t_power_usage * power_usage,
		t_clock_arch * clock_arch) {
	float Clock_power_dissipation;
	int total_clock_buffers, total_clock_segments;
	int clock_idx;

	/* Initialization */
	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;

	Clock_power_dissipation = 0;
	total_clock_buffers = 0;
	total_clock_segments = 0;

	/* if no global clock, then return */
	if (clock_arch->num_global_clocks == 0) {
		return;
	}

	for (clock_idx = 0; clock_idx < clock_arch->num_global_clocks;
			clock_idx++) {
		t_power_usage clock_power;

		/* Assume the global clock is active even for combinational circuits */
		if (clock_arch->num_global_clocks == 1) {
			if (clock_arch->clock_inf[clock_idx].dens == 0) {
				clock_arch->clock_inf[clock_idx].dens = 2;
				clock_arch->clock_inf[clock_idx].prob = 0.5;
			}
		}
		/* find the power dissipated by each clock network */
		power_calc_clock_single(&clock_power,
				&clock_arch->clock_inf[clock_idx]);
		power_add_usage(power_usage, &clock_power);
	}

	return;
}

/**
 * Calculates the power from a single spine-and-rib global clock
 */
static void power_calc_clock_single(t_power_usage * power_usage,
		t_clock_network * single_clock) {

	/*
	 *
	 * The following code assumes a spine-and-rib clock network as shown below.
	 * This is comprised of 3 main combonents:
	 * 	1. A single wire from the io pad to the center of the chip
	 * 	2. A H-structure which provides a 'spine' to all 4 quadrants
	 * 	3. Ribs connect each spine with an entire column of blocks

	 ___________________
	 |                 |
	 | |_|_|_2__|_|_|_ |
	 | | | |  | | | |  |
	 | |3| |  | | | |  |
	 |        |        |
	 | | | |  | | | |  |
	 | |_|_|__|_|_|_|_ |
	 | | | |  | | | |  |
	 |_______1|________|
	 * It is assumed that there are a single-inverter buffers placed along each wire,
	 * with spacing equal to the FPGA block size (1 buffer/block) */
	t_power_usage clock_buffer_power;
	boolean clock_used;
	int length;
	t_power_usage buffer_power;
	t_power_usage wire_power;
	float C_segment;
	float buffer_size;

	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;

	/* Check if this clock is active - this is used for calculating leakage */
	if (single_clock->dens) {
		clock_used = TRUE;
	} else {
		clock_used = FALSE;
		assert(0);
	}

	C_segment = g_power_commonly_used->tile_length * single_clock->C_wire;
	if (single_clock->autosize_buffer) {
		buffer_size = 1 + C_segment / g_power_commonly_used->INV_1X_C_in;
	} else {
		buffer_size = single_clock->buffer_size;
	}

	/* Calculate the capacitance and leakage power for the clock buffer */
	power_calc_inverter(&clock_buffer_power, single_clock->dens,
			single_clock->prob, buffer_size);

	length = 0;

	/* 1. IO to chip center */
	length += (ny + 2) / 2;

	/* 2. H-Tree to 4 quadrants */
	length += ny / 2 + 2 * nx;

	/* 3. Ribs - to */
	length += nx / 2 * ny;

	buffer_power.dynamic = length * clock_buffer_power.dynamic;
	buffer_power.leakage = length * clock_buffer_power.leakage;

	power_add_usage(power_usage, &buffer_power);
	power_component_add_usage(&buffer_power, POWER_COMPONENT_CLOCK_BUFFER);

	power_usage_wire(&wire_power, length * C_segment, single_clock->dens);
	power_add_usage(power_usage, &wire_power);
	power_component_add_usage(&wire_power, POWER_COMPONENT_CLOCK_WIRE);

	return;
}

/* Frees a multiplexer graph */
static void dealloc_mux_graph(t_mux_node * node) {
	dealloc_mux_graph_recursive(node);
	free(node);
}

static void dealloc_mux_graph_recursive(t_mux_node * node) {
	int child_idx;

	/* Dealloc Children */
	if (node->level != 0) {
		for (child_idx = 0; child_idx < node->num_inputs; child_idx++) {
			dealloc_mux_graph_recursive(&node->children[child_idx]);
		}
		free(node->children);
	}
}

/**
 * Calculates the power of the entire routing fabric (not local routing
 */
static void power_calc_routing(t_power_usage * power_usage,
		t_det_routing_arch * routing_arch, t_segment_inf * segment_inf) {
	int rr_node_idx;
	int net_idx;
	int edge_idx;

	power_zero_usage(power_usage);

	/* Reset routing statistics */
	g_power_commonly_used->num_sb_buffers = 0;
	g_power_commonly_used->total_sb_buffer_size = 0.;
	g_power_commonly_used->num_cb_buffers = 0;
	g_power_commonly_used->total_cb_buffer_size = 0.;

	/* Reset rr graph net indices */
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		rr_node[rr_node_idx].net_num = OPEN;
		rr_node_power[rr_node_idx].num_inputs = 0;
		rr_node_power[rr_node_idx].selected_input = 0;
	}

	/* Populate net indices into rr graph */
	for (net_idx = 0; net_idx < num_nets; net_idx++) {
		struct s_trace * trace;

		for (trace = trace_head[net_idx]; trace != NULL ; trace = trace->next) {
			rr_node_power[trace->index].visited = FALSE;
		}
	}

	/* Populate net indices into rr graph */
	for (net_idx = 0; net_idx < num_nets; net_idx++) {
		struct s_trace * trace;

		for (trace = trace_head[net_idx]; trace != NULL ; trace = trace->next) {
			t_rr_node * node = &rr_node[trace->index];
			t_rr_node_power * node_power = &rr_node_power[trace->index];

			if (node_power->visited) {
				continue;
			}

			node->net_num = net_idx;

			for (edge_idx = 0; edge_idx < node->num_edges; edge_idx++) {
				if (node->edges[edge_idx] != OPEN) {
					t_rr_node * next_node = &rr_node[node->edges[edge_idx]];
					t_rr_node_power * next_node_power =
							&rr_node_power[node->edges[edge_idx]];

					switch (next_node->type) {
					case CHANX:
					case CHANY:
					case IPIN:
						if (next_node->net_num == node->net_num) {
							next_node_power->selected_input =
									next_node_power->num_inputs;
						}
						next_node_power->in_dens[next_node_power->num_inputs] =
								clb_net_density(node->net_num);
						next_node_power->in_prob[next_node_power->num_inputs] =
								clb_net_prob(node->net_num);
						next_node_power->num_inputs++;
						if (next_node_power->num_inputs > next_node->fan_in) {
							printf("%d %d\n", next_node_power->num_inputs,
									next_node->fan_in);
							fflush(0);
							assert(0);
						}
						break;
					default:
						/* Do nothing */
						break;
					}
				}
			}
			node_power->visited = TRUE;
		}
	}

	/* Calculate power of all routing entities */
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		t_power_usage sub_power_usage;
		t_rr_node * node = &rr_node[rr_node_idx];
		t_rr_node_power * node_power = &rr_node_power[rr_node_idx];
		float C_wire;
		float buffer_size;
		int switch_idx;
		int connectionbox_fanout;
		int switchbox_fanout;
		float C_per_seg_split;
		int i;
		int wire_length;

		switch (node->type) {
		case SOURCE:
		case SINK:
		case OPIN:
			/* No power usage for these types */
			break;
		case IPIN:
			/* This is part of the connectionbox.  The connection box is comprised of:
			 *  - Driver (accounted for at end of CHANX/Y - see below)
			 *  - Multiplexor */

			if (node->fan_in) {
				assert(node_power->in_dens);
				assert(node_power->in_prob);

				/* Multiplexor */
				power_calc_mux_multilevel(&sub_power_usage,
						power_get_mux_arch(node->fan_in), node_power->in_prob,
						node_power->in_dens, node_power->selected_input, TRUE);
				power_add_usage(power_usage, &sub_power_usage);
				power_component_add_usage(&sub_power_usage,
						POWER_COMPONENT_ROUTE_CB);
			}
			break;
		case CHANX:
		case CHANY:
			/* This is a wire driven by a switchbox, which includes:
			 * 	- The Multiplexor at the beginning of the wire
			 * 	- A buffer, after the mux to drive the wire
			 * 	- The wire itself
			 * 	- A buffer at the end of the wire, going to switchbox/connectionbox */
			assert(node_power->in_dens);
			assert(node_power->in_prob);

			if (node->type == CHANX) {
				wire_length = node->xhigh - node->xlow + 1;
			} else if (node->type == CHANY) {
				wire_length = node->yhigh - node->ylow + 1;
			}
			C_wire =
					wire_length
							* segment_inf[rr_indexed_data[node->cost_index].seg_index].Cmetal_per_m
							* g_power_commonly_used->tile_length;
			assert(node_power->selected_input < node->fan_in);

			/* Multiplexor */
			power_calc_mux_multilevel(&sub_power_usage,
					power_get_mux_arch(node->fan_in), node_power->in_prob,
					node_power->in_dens, node_power->selected_input, TRUE);
			power_add_usage(power_usage, &sub_power_usage);
			power_component_add_usage(&sub_power_usage,
					POWER_COMPONENT_ROUTE_SB);

			/* Buffer Size */
			switch (switch_inf[node_power->driver_switch_type].power_buffer_type) {
			case POWER_BUFFER_TYPE_AUTO:
				C_per_seg_split = ((float) node->num_edges
						* g_power_commonly_used->INV_1X_C_in + C_wire)
						/ (float) g_power_arch->seg_buffer_split;
				buffer_size = power_buffer_size_from_logical_effort(
						C_per_seg_split);
				buffer_size = max(buffer_size, 1.0F);
				break;
			case POWER_BUFFER_TYPE_ABSOLUTE_SIZE:
				buffer_size =
						switch_inf[node_power->driver_switch_type].power_buffer_size;
				buffer_size = max(buffer_size, 1.0F);
				break;
			case POWER_BUFFER_TYPE_NONE:
				buffer_size = 0.;
				break;
			default:
				assert(0);
				break;
			}

			g_power_commonly_used->num_sb_buffers +=
					g_power_arch->seg_buffer_split;
			g_power_commonly_used->total_sb_buffer_size += buffer_size
					* g_power_arch->seg_buffer_split;

			/* Buffers */
			for (i = 0; i < g_power_arch->seg_buffer_split; i++) {
				if (i == 0) {
					power_usage_buffer(&sub_power_usage, buffer_size,
							node_power->in_prob[node_power->selected_input],
							node_power->in_dens[node_power->selected_input],
							TRUE,
							power_get_mux_arch(node->fan_in)->mux_graph_head->num_inputs);
					power_add_usage(power_usage, &sub_power_usage);
					power_component_add_usage(&sub_power_usage,
							POWER_COMPONENT_ROUTE_SB);
				} else {
					power_usage_buffer(&sub_power_usage, buffer_size,
							node_power->in_prob[node_power->selected_input],
							node_power->in_dens[node_power->selected_input],
							FALSE, 0);
					power_add_usage(power_usage, &sub_power_usage);
					power_component_add_usage(&sub_power_usage,
							POWER_COMPONENT_ROUTE_SB);
				}
			}

			/* Wire Capacitance */
			power_usage_wire(&sub_power_usage, C_wire,
					clb_net_density(node->net_num));
			power_add_usage(power_usage, &sub_power_usage);
			power_component_add_usage(&sub_power_usage,
					POWER_COMPONENT_ROUTE_GLB_WIRE);

			/* Determine types of switches that this wire drives */
			connectionbox_fanout = 0;
			switchbox_fanout = 0;
			for (switch_idx = 0; switch_idx < node->num_edges; switch_idx++) {
				if (node->switches[switch_idx]
						== routing_arch->wire_to_ipin_switch) {
					connectionbox_fanout++;
				} else if (node->switches[switch_idx]
						== routing_arch->delayless_switch) {
					/* Do nothing */
				} else {
					switchbox_fanout++;
				}
			}

			/* Buffer to next Switchbox */
			if (switchbox_fanout) {
				buffer_size = power_buffer_size_from_logical_effort(
						switchbox_fanout * g_power_commonly_used->NMOS_1X_C_d);
				power_usage_buffer(&sub_power_usage, buffer_size,
						1 - node_power->in_prob[node_power->selected_input],
						node_power->in_dens[node_power->selected_input], FALSE,
						0);
				power_add_usage(power_usage, &sub_power_usage);
				power_component_add_usage(&sub_power_usage,
						POWER_COMPONENT_ROUTE_SB);
			}

			/* Driver for ConnectionBox */
			if (connectionbox_fanout) {

				buffer_size = power_buffer_size_from_logical_effort(
						connectionbox_fanout
								* g_power_commonly_used->NMOS_1X_C_d);

				power_usage_buffer(&sub_power_usage, buffer_size,
						node_power->in_dens[node_power->selected_input],
						1 - node_power->in_prob[node_power->selected_input],
						FALSE, 0);
				power_add_usage(power_usage, &sub_power_usage);
				power_component_add_usage(&sub_power_usage,
						POWER_COMPONENT_ROUTE_CB);

				g_power_commonly_used->num_cb_buffers++;
				g_power_commonly_used->total_cb_buffer_size += buffer_size;
			}
			break;
		case INTRA_CLUSTER_EDGE:
			assert(0);
			break;
		default:
			power_log_msg(POWER_LOG_WARNING,
					"The global routing-resource graph contains an unknown node type.");
			break;
		}
	}
}

void power_pb_pin_alloc_rec(t_pb_graph_node * pb_node) {
	int mode_idx;
	int type_idx;
	int pb_idx;
	int port_idx;
	int pin_idx;

	for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
				pin_idx++) {

			pb_node->input_pins[port_idx][pin_idx].pin_power =
					(t_pb_graph_pin_power*) my_calloc(1,
							sizeof(t_pb_graph_pin_power));
		}
	}

	for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
				pin_idx++) {
			pb_node->output_pins[port_idx][pin_idx].pin_power =
					(t_pb_graph_pin_power*) my_calloc(1,
							sizeof(t_pb_graph_pin_power));
		}
	}

	for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
				pin_idx++) {
			pb_node->clock_pins[port_idx][pin_idx].pin_power =
					(t_pb_graph_pin_power*) my_calloc(1,
							sizeof(t_pb_graph_pin_power));
		}
	}

	for (mode_idx = 0; mode_idx < pb_node->pb_type->num_modes; mode_idx++) {
		for (type_idx = 0;
				type_idx
						< pb_node->pb_type->modes[mode_idx].num_pb_type_children;
				type_idx++) {
			for (pb_idx = 0;
					pb_idx
							< pb_node->pb_type->modes[mode_idx].pb_type_children[type_idx].num_pb;
					pb_idx++) {
				power_pb_pin_alloc_rec(
						&pb_node->child_pb_graph_nodes[mode_idx][type_idx][pb_idx]);
			}
		}
	}
}

void power_pb_pin_alloc() {
	int type_idx;

	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_graph_head) {
			power_pb_pin_alloc_rec(type_descriptors[type_idx].pb_graph_head);
		}
	}
}

/**
 * Initialization for all power-related functions
 */
boolean power_init(char * power_out_filepath,
		char * cmos_tech_behavior_filepath, t_arch * arch,
		t_det_routing_arch * routing_arch) {
	boolean error = FALSE;
	int net_idx;
	int rr_node_idx;
	int max_fanin;
	int max_IPIN_fanin;
	int max_seg_to_IPIN_fanout;
	int max_seg_to_seg_fanout;
	int max_seg_fanout;
	float transistors_per_tile;

	/* Set global power architecture & options */
	g_power_arch = arch->power;
	g_power_commonly_used = (t_power_commonly_used*) my_malloc(
			sizeof(t_power_commonly_used));
	g_power_tech = (t_power_tech*) my_malloc(sizeof(t_power_tech));
	g_power_output = (t_power_output*) my_malloc(sizeof(t_power_output));

	/* Set up Logs */
	g_power_output->num_logs = POWER_LOG_NUM_TYPES;
	g_power_output->logs = (t_log*) my_calloc(g_power_output->num_logs,
			sizeof(t_log));
	g_power_output->logs[POWER_LOG_ERROR].name = my_strdup("Errors");
	g_power_output->logs[POWER_LOG_WARNING].name = my_strdup("Warnings");

	/* Initialize output file */
	if (!error) {
		g_power_output->out = NULL;
		g_power_output->out = my_fopen(power_out_filepath, "w", 0);
		if (!g_power_output->out) {
			error = TRUE;
		}
	}

	/* Load technology properties */
	power_tech_load_xml_file(cmos_tech_behavior_filepath);

	/* Initialize sub-modules */
	power_components_init();
	power_lowlevel_init();

	/* Initialize Commonly Used Values */
	g_power_commonly_used->mux_arch = NULL;
	g_power_commonly_used->mux_arch_max_size = 0;

	/* Copy probability/density values to new netlist */
	for (net_idx = 0; net_idx < num_nets; net_idx++) {
		clb_net[net_idx].probability =
				vpack_net[clb_to_vpack_net_mapping[net_idx]].probability;
		clb_net[net_idx].density =
				vpack_net[clb_to_vpack_net_mapping[net_idx]].density;
	}

	/* Initialize RR Graph Structures */
	rr_node_power = (t_rr_node_power*) my_calloc(num_rr_nodes,
			sizeof(t_rr_node_power));
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		rr_node_power[rr_node_idx].driver_switch_type = OPEN;

	}

	/* Initialize Mux Architectures */
	max_fanin = 0;
	max_IPIN_fanin = 0;
	max_seg_to_seg_fanout = 0;
	max_seg_to_IPIN_fanout = 0;
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		int switch_idx;
		int fanout_to_IPIN = 0;
		int fanout_to_seg = 0;
		t_rr_node * node = &rr_node[rr_node_idx];
		t_rr_node_power * node_power = &rr_node_power[rr_node_idx];

		switch (node->type) {
		case IPIN:
			max_IPIN_fanin = max(max_IPIN_fanin, static_cast<int>(node->fan_in));
			max_fanin = max(max_fanin, static_cast<int>(node->fan_in));

			node_power->in_dens = (float*) my_calloc(node->fan_in,
					sizeof(float));
			node_power->in_prob = (float*) my_calloc(node->fan_in,
					sizeof(float));
			break;
		case CHANX:
		case CHANY:
			for (switch_idx = 0; switch_idx < node->num_edges; switch_idx++) {
				if (node->switches[switch_idx]
						== routing_arch->wire_to_ipin_switch) {
					fanout_to_IPIN++;
				} else if (node->switches[switch_idx]
						!= routing_arch->delayless_switch) {
					fanout_to_seg++;
				}
			}
			max_seg_to_IPIN_fanout =
					max(max_seg_to_IPIN_fanout, fanout_to_IPIN);
			max_seg_to_seg_fanout = max(max_seg_to_seg_fanout, fanout_to_seg);
			max_fanin = max(max_fanin, static_cast<int>(node->fan_in));

			node_power->in_dens = (float*) my_calloc(node->fan_in,
					sizeof(float));
			node_power->in_prob = (float*) my_calloc(node->fan_in,
					sizeof(float));
			break;
		default:
			/* Do nothing */
			break;
		}
	}
	g_power_commonly_used->max_routing_mux_size = max_fanin;
	g_power_commonly_used->max_IPIN_fanin = max_IPIN_fanin;
	g_power_commonly_used->max_seg_to_seg_fanout = max_seg_to_seg_fanout;
	g_power_commonly_used->max_seg_to_IPIN_fanout = max_seg_to_IPIN_fanout;

#if (PRINT_SPICE_COMPARISON)
	g_power_commonly_used->max_routing_mux_size =
	max(g_power_commonly_used->max_routing_mux_size, 26);
#endif

	/* Populate driver switch type */
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		t_rr_node * node = &rr_node[rr_node_idx];
		int edge_idx;

		for (edge_idx = 0; edge_idx < node->num_edges; edge_idx++) {
			if (node->edges[edge_idx] != OPEN) {
				if (rr_node_power[node->edges[edge_idx]].driver_switch_type
						== OPEN) {
					rr_node_power[node->edges[edge_idx]].driver_switch_type =
							node->switches[edge_idx];
				} else {
					assert(
							rr_node_power[node->edges[edge_idx]].driver_switch_type == node->switches[edge_idx]);
				}
			}
		}
	}

	/* Find Max Fanout of Routing Buffer	 */
	max_seg_fanout = 0;
	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		t_rr_node * node = &rr_node[rr_node_idx];

		switch (node->type) {
		case CHANX:
		case CHANY:
			if (node->num_edges > max_seg_fanout) {
				max_seg_fanout = node->num_edges;
			}
			break;
		default:
			/* Do nothing */
			break;
		}
	}
	g_power_commonly_used->max_seg_fanout = max_seg_fanout;

	power_pb_pin_alloc();
	power_size_pb();

	/* Find # of transistors in each tile type */
	transistors_per_tile = power_transistors_per_tile(arch);

	/* Calculate CLB tile size
	 *  - Assume a square tile
	 *  - Assume min transistor size is Wx6L
	 *  - Assume an overhead to space transistors
	 */
	g_power_commonly_used->tile_length = sqrt(
			power_transistor_area(transistors_per_tile));

	return error;
}

/**
 * Uninitialize power module
 */
boolean power_uninit(void) {
	int mux_size;
	int log_idx;
	int rr_node_idx;
	int msg_idx;
	boolean error = FALSE;

	for (rr_node_idx = 0; rr_node_idx < num_rr_nodes; rr_node_idx++) {
		t_rr_node * node = &rr_node[rr_node_idx];
		t_rr_node_power * node_power = &rr_node_power[rr_node_idx];

		switch (node->type) {
		case CHANX:
		case CHANY:
		case IPIN:
			if (node->fan_in) {
				free(node_power->in_dens);
				free(node_power->in_prob);
			}
			break;
		default:
			/* Do nothing */
			break;
		}
	}
	free(rr_node_power);

	/* Free mux architectures */
	for (mux_size = 1; mux_size <= g_power_commonly_used->mux_arch_max_size;
			mux_size++) {
		//free(g_power_commonly_used->mux_arch[mux_size].encoding_types);
		free(g_power_commonly_used->mux_arch[mux_size].transistor_sizes);
		dealloc_mux_graph(
				g_power_commonly_used->mux_arch[mux_size].mux_graph_head);
	}
	free(g_power_commonly_used->mux_arch);
	free(g_power_commonly_used);

	if (g_power_output->out) {
		fclose(g_power_output->out);
	}

	/* Free logs */
	for (log_idx = 0; log_idx < g_power_output->num_logs; log_idx++) {
		for (msg_idx = 0; msg_idx < g_power_output->logs[log_idx].num_messages;
				msg_idx++) {
			free(g_power_output->logs[log_idx].messages[msg_idx]);
		}
		free(g_power_output->logs[log_idx].messages);
		free(g_power_output->logs[log_idx].name);
	}
	free(g_power_output->logs);
	free(g_power_output);

	return error;
}

/**
 * Prints the power of all pb structures, in an xml format that matches the archicture file
 */
static void power_print_pb_usage_recursive(FILE * fp, t_pb_type * type,
		int indent_level, float parent_power, float total_power) {
	int mode_idx;
	int mode_indent;
	int child_idx;
	int interc_idx;
	float pb_type_power;

	pb_type_power = type->pb_type_power->power_usage.dynamic
			+ type->pb_type_power->power_usage.leakage;

	print_tabs(fp, indent_level);
	fprintf(fp,
			"<pb_type name=\"%s\" P=\"%.4g\" P_parent=\"%.3g\" P_total=\"%.3g\" P_dyn=\"%.3g\" >\n",
			type->name, pb_type_power, pb_type_power / parent_power * 100,
			pb_type_power / total_power * 100,
			type->pb_type_power->power_usage.dynamic / pb_type_power);

	mode_indent = 0;
	if (type->num_modes > 1) {
		mode_indent = 1;
	}

	for (mode_idx = 0; mode_idx < type->num_modes; mode_idx++) {
		float mode_power;
		mode_power = type->modes[mode_idx].mode_power->power_usage.dynamic
				+ type->modes[mode_idx].mode_power->power_usage.leakage;

		if (type->num_modes > 1) {
			print_tabs(fp, indent_level + mode_indent);
			fprintf(fp,
					"<mode name=\"%s\" P=\"%.4g\" P_parent=\"%.3g\" P_total=\"%.3g\" P_dyn=\"%.3g\">\n",
					type->modes[mode_idx].name, mode_power,
					mode_power / pb_type_power * 100,
					mode_power / total_power * 100,
					type->modes[mode_idx].mode_power->power_usage.dynamic
							/ mode_power);
		}

		if (type->modes[mode_idx].num_interconnect) {
			/* Sum the interconnect power */
			t_power_usage interc_power_usage;
			float interc_total_power;

			power_zero_usage(&interc_power_usage);
			for (interc_idx = 0;
					interc_idx < type->modes[mode_idx].num_interconnect;
					interc_idx++) {
				power_add_usage(&interc_power_usage,
						&type->modes[mode_idx].interconnect[interc_idx].interconnect_power->power_usage);
			}
			interc_total_power = interc_power_usage.dynamic
					+ interc_power_usage.leakage;

			/* All interconnect */
			print_tabs(fp, indent_level + mode_indent + 1);
			fprintf(fp,
					"<interconnect P=\"%.4g\" P_parent=\"%.3g\" P_total=\"%.3g\" P_dyn=\"%.3g\">\n",
					interc_total_power, interc_total_power / mode_power * 100,
					interc_total_power / total_power * 100,
					interc_power_usage.dynamic / interc_total_power);
			for (interc_idx = 0;
					interc_idx < type->modes[mode_idx].num_interconnect;
					interc_idx++) {
				float interc_power =
						type->modes[mode_idx].interconnect[interc_idx].interconnect_power->power_usage.dynamic
								+ type->modes[mode_idx].interconnect[interc_idx].interconnect_power->power_usage.leakage;
				/* Each interconnect */
				print_tabs(fp, indent_level + mode_indent + 2);
				fprintf(fp,
						"<%s name=\"%s\" P=\"%.4g\" P_parent=\"%.3g\" P_total=\"%.3g\" P_dyn=\"%.3g\"/>\n",
						interconnect_type_name(
								type->modes[mode_idx].interconnect[interc_idx].type),
						type->modes[mode_idx].interconnect[interc_idx].name,
						interc_power, interc_power / interc_total_power * 100,
						interc_power / total_power * 100,
						type->modes[mode_idx].interconnect[interc_idx].interconnect_power->power_usage.dynamic
								/ interc_power);
			}
			print_tabs(fp, indent_level + mode_indent + 1);
			fprintf(fp, "</interconnect>\n");
		}

		for (child_idx = 0;
				child_idx < type->modes[mode_idx].num_pb_type_children;
				child_idx++) {
			power_print_pb_usage_recursive(fp,
					&type->modes[mode_idx].pb_type_children[child_idx],
					indent_level + mode_indent + 1,
					type->modes[mode_idx].mode_power->power_usage.dynamic
							+ type->modes[mode_idx].mode_power->power_usage.leakage,
					total_power);
		}

		if (type->num_modes > 1) {
			print_tabs(fp, indent_level + mode_indent);
			fprintf(fp, "</mode>\n");
		}
	}

	print_tabs(fp, indent_level);
	fprintf(fp, "</pb_type>\n");
}

static void power_print_clb_detailed(FILE * fp) {
	int type_idx;

	float clb_power_total = power_component_get_usage_sum(POWER_COMPONENT_CLB);
	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (!type_descriptors[type_idx].pb_type) {
			continue;
		}

		power_print_pb_usage_recursive(fp, type_descriptors[type_idx].pb_type,
				0, clb_power_total, clb_power_total);
	}
}

void power_print_stats(FILE * fp) {
	fprintf(fp, "Technology (nm): %.0f\n",
			g_power_tech->tech_size * CONVERT_NM_PER_M);
	fprintf(fp, "Voltage: %.2f\n", g_power_tech->Vdd);
	fprintf(fp, "Operating temperature: %g\n", g_power_tech->temperature);

	fprintf(fp, "Critical Path: %g\n", g_solution_inf.T_crit);
	fprintf(fp, "Layout of FPGA: %d x %d\n", nx, ny);

	fprintf(fp, "Max Segment Fanout: %d\n",
			g_power_commonly_used->max_seg_fanout);
	fprintf(fp, "Max Segment->Segment Fanout: %d\n",
			g_power_commonly_used->max_seg_to_seg_fanout);
	fprintf(fp, "Max Segment->IPIN Fanout: %d\n",
			g_power_commonly_used->max_seg_to_IPIN_fanout);
	fprintf(fp, "Max IPIN fanin: %d\n", g_power_commonly_used->max_IPIN_fanin);
	fprintf(fp, "Average SB Buffer Size: %.1f\n",
			g_power_commonly_used->total_sb_buffer_size
					/ (float) g_power_commonly_used->num_sb_buffers);
	fprintf(fp, "SB Buffer Transistors: %g\n",
			power_count_transistors_buffer(
					g_power_commonly_used->total_sb_buffer_size
							/ (float) g_power_commonly_used->num_sb_buffers));
	fprintf(fp, "Average CB Buffer Size: %.1f\n",
			g_power_commonly_used->total_cb_buffer_size
					/ (float) g_power_commonly_used->num_cb_buffers);
	fprintf(fp, "Tile length (um): %.2f\n",
			g_power_commonly_used->tile_length * CONVERT_UM_PER_M);
	fprintf(fp, "1X Inverter C_in: %g\n", g_power_commonly_used->INV_1X_C_in);
	fprintf(fp, "\n");

}

void power_print_clb_summary(FILE * fp) {
	int i;

	fprintf(fp, "%-50s%-15s%-15s\n", "PB Type", "Power", "Est. Method");
	fprintf(fp, "%-50s%-15s%-15s\n", "-------", "-----", "-----------");
	for (i = 0; i < num_types; i++) {
		if (type_descriptors[i].pb_type) {
			power_print_clb_summary_rec(fp, type_descriptors[i].pb_type, 0);
		}
	}
	fprintf(fp, "\n");
}

const char * power_estimation_method_name(
		e_power_estimation_method power_method) {
	switch (power_method) {
	case POWER_METHOD_UNDEFINED:
		return "Undefined";
	case POWER_METHOD_IGNORE:
		return "Ignored";
	case POWER_METHOD_AUTO_SIZES:
		return "Auto-Size";
	case POWER_METHOD_SPECIFY_SIZES:
		return "Specify-Size";
	case POWER_METHOD_TOGGLE_PINS:
		return "Pin-Toggle";
	case POWER_METHOD_C_INTERNAL:
		return "C-Internal";
	case POWER_METHOD_ABSOLUTE:
		return "Absolute";
	default:
		return "Unkown";
	}
}

static void power_print_clb_summary_rec(FILE * fp, t_pb_type * pb_type,
		int indent) {
	int mode_idx;
	int child_idx;
	int i;
	char buf[51];

	for (i = 0; i < indent; i++) {
		buf[i] = ' ';
	}
	strncpy(buf + indent, pb_type->name, 50 - indent);
	buf[51] = '\0';
	buf[strlen(pb_type->name) + indent] = '\0';
	fprintf(fp, "%-50s%-15g%-15s\n", buf,
			power_usage_sum(&pb_type->pb_type_power->power_usage),
			power_estimation_method_name(
					pb_type->pb_type_power->estimation_method));

	for (mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
		for (child_idx = 0;
				child_idx < pb_type->modes[mode_idx].num_pb_type_children;
				child_idx++) {
			power_print_clb_summary_rec(fp,
					&pb_type->modes[mode_idx].pb_type_children[child_idx],
					indent + 1);
		}

	}

}

/*
 * Top-level function for the power module.
 * Calculates the average power of the entire FPGA (watts),
 * and prints it to the output file
 * - run_time_s: (Return value) The total runtime in seconds (us accuracy)
 */
e_power_ret_code power_total(float * run_time_s, t_arch * arch,
		t_det_routing_arch * routing_arch) {
	t_power_usage total_power;
	t_power_usage sub_power_usage;
	clock_t t_start;
	clock_t t_end;
	t_power_usage clb_power_usage;

	t_start = clock();

	power_zero_usage(&total_power);

	if (routing_arch->directionality == BI_DIRECTIONAL) {
		power_log_msg(POWER_LOG_ERROR,
				"Cannot calculate routing power for bi-directional architectures");
		return POWER_RET_CODE_ERRORS;
	}

	/* Calculate Power */
	if (1) {

		/* Routing */
		power_calc_routing(&sub_power_usage, routing_arch, arch->Segments);
		power_add_usage(&total_power, &sub_power_usage);
		power_component_add_usage(&sub_power_usage, POWER_COMPONENT_ROUTING);

		/* Clock  */
		power_calc_clock(&sub_power_usage, arch->clocks);
		power_add_usage(&total_power, &sub_power_usage);
		power_component_add_usage(&sub_power_usage, POWER_COMPONENT_CLOCK);

		/* CLBs */
		power_calc_blocks(&clb_power_usage);
		power_add_usage(&total_power, &clb_power_usage);
		power_component_add_usage(&clb_power_usage, POWER_COMPONENT_CLB);

		power_component_add_usage(&total_power, POWER_COMPONENT_TOTAL);
	}

	/* Print Error & Warning Logs */
	output_logs(g_power_output->out, g_power_output->logs,
			g_power_output->num_logs);

	power_print_title(g_power_output->out, "Statistics");
	power_print_stats(g_power_output->out);

	power_print_title(g_power_output->out, "Power By Element");
	power_component_print_usage(g_power_output->out);

	power_print_title(g_power_output->out, "CLB Power Summary");
	power_print_clb_summary(g_power_output->out);

	if (0) {
		power_print_title(g_power_output->out, "Spice Comparison");
		power_print_spice_comparison();
	}

	power_print_title(g_power_output->out, "CLB Detailed Breakdown");
	power_print_clb_detailed(g_power_output->out);

	t_end = clock();

	*run_time_s = (float) (t_end - t_start) / CLOCKS_PER_SEC;

	/* Return code */
	if (g_power_output->logs[POWER_LOG_ERROR].num_messages) {
		return POWER_RET_CODE_ERRORS;
	} else if (g_power_output->logs[POWER_LOG_WARNING].num_messages) {
		return POWER_RET_CODE_WARNINGS;
	} else {
		return POWER_RET_CODE_SUCCESS;
	}
}

