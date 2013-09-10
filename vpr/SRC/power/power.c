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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cmath>
using namespace std;

#include <ctype.h>
#include <assert.h>

#include "power.h"
#include "power_components.h"
#include "power_util.h"
#include "power_lowlevel.h"
#include "power_sizing.h"
#include "power_callibrate.h"
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
typedef enum {
	POWER_BREAKDOWN_ENTRY_TYPE_TITLE = 0,
	POWER_BREAKDOWN_ENTRY_TYPE_MODE,
	POWER_BREAKDOWN_ENTRY_TYPE_COMPONENT,
	POWER_BREAKDOWN_ENTRY_TYPE_PB,
	POWER_BREAKDOWN_ENTRY_TYPE_INTERC,
	POWER_BREAKDOWN_ENTRY_TYPE_BUFS_WIRES
} e_power_breakdown_entry_type;

/************************* GLOBALS **********************************/
t_solution_inf g_solution_inf;
t_power_output * g_power_output;
t_power_commonly_used * g_power_commonly_used;
t_power_tech * g_power_tech;
t_power_arch * g_power_arch;
static t_rr_node_power * rr_node_power;
t_net_power * clb_net_power;

/************************* Function Declarations ********************/
/* Routing */
static void power_usage_routing(t_power_usage * power_usage,
		t_det_routing_arch * routing_arch, t_segment_inf * segment_inf);

/* Tiles */
static void power_usage_blocks(t_power_usage * power_usage);
static void power_usage_pb(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_node);
static void power_usage_primitive(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node);
static void power_reset_tile_usage(void);
static void power_reset_pb_type(t_pb_type * pb_type);
static void power_usage_local_buffers_and_wires(t_power_usage * power_usage,
		t_pb * pb, t_pb_graph_node * pb_node);

/* Clock */
static void power_usage_clock(t_power_usage * power_usage,
		t_clock_arch * clock_arch);
static void power_usage_clock_single(t_power_usage * power_usage,
		t_clock_network * clock_inf);

/* Init/Uninit */
static void dealloc_mux_graph(t_mux_node * node);
static void dealloc_mux_graph_rec(t_mux_node * node);

/* Printing */
static void power_print_breakdown_pb_rec(FILE * fp, t_pb_type * pb_type,
		int indent);
static void power_print_summary(FILE * fp, t_vpr_setup vpr_setup);
//static void power_print_stats(FILE * fp);
static void power_print_breakdown_summary(FILE * fp);
static void power_print_breakdown_entry(FILE * fp, int indent,
		e_power_breakdown_entry_type type, char * name, float power,
		float total_power, float perc_dyn, char * method);
static void power_print_breakdown_component(FILE * fp, char * name,
		e_power_component_type type, int indent_level);
static void power_print_breakdown_pb(FILE * fp);

static char * power_estimation_method_name(
		e_power_estimation_method power_method);

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
static void power_usage_primitive(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_graph_node) {
	t_power_usage sub_power_usage;

	power_zero_usage(power_usage);
	power_zero_usage(&sub_power_usage);

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
			SRAM_values = alloc_SRAM_values_from_truth_table(LUT_size, NULL);
		}
		power_usage_lut(&sub_power_usage, LUT_size,
				g_power_arch->LUT_transistor_size, SRAM_values,
				input_probabilities, input_densities, g_solution_inf.T_crit);
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

		power_usage_ff(&sub_power_usage, g_power_arch->FF_size, D_prob, D_dens,
				Q_prob, Q_dens, clk_prob, clk_dens, g_solution_inf.T_crit);
		power_add_usage(power_usage, &sub_power_usage);

	} else {
		char msg[BUFSIZE];
		sprintf(msg, "No dynamic power defined for BLIF model: %s",
				pb_graph_node->pb_type->blif_model);
		power_log_msg(POWER_LOG_WARNING, msg);

		sprintf(msg, "No leakage power defined for BLIF model: %s",
				pb_graph_node->pb_type->blif_model);
		power_log_msg(POWER_LOG_WARNING, msg);

	}
}

void power_usage_local_pin_toggle(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_pin * pin) {
	float scale_factor;

	power_zero_usage(power_usage);

	if (pin->pin_power->scaled_by_pin) {
		scale_factor = pin_prob(pb, pin->pin_power->scaled_by_pin);
		if (pin->port->port_power->reverse_scaled) {
			scale_factor = 1 - scale_factor;
		}
	} else {
		scale_factor = 1.0;
	}

	/* Divide by 2 because density is switches/cycle, but a toggle is 2 switches */
	power_usage->dynamic += scale_factor
			* pin->port->port_power->energy_per_toggle * pin_dens(pb, pin) / 2.0
			/ g_solution_inf.T_crit;
}

void power_usage_local_pin_buffer_and_wire(t_power_usage * power_usage,
		t_pb * pb, t_pb_graph_pin * pin) {
	t_power_usage sub_power_usage;
	float buffer_size = 0.;
	double C_wire;

	power_zero_usage(power_usage);

	/* Wire switching */
	C_wire = pin->pin_power->C_wire;
	power_usage_wire(&sub_power_usage, C_wire, pin_dens(pb, pin),
			g_solution_inf.T_crit);
	power_add_usage(power_usage, &sub_power_usage);

	/* Buffer power */
	buffer_size = pin->pin_power->buffer_size;
	if (buffer_size) {
		power_usage_buffer(&sub_power_usage, buffer_size, pin_prob(pb, pin),
				pin_dens(pb, pin), FALSE, g_solution_inf.T_crit);
		power_add_usage(power_usage, &sub_power_usage);
	}
}

static void power_usage_local_buffers_and_wires(t_power_usage * power_usage,
		t_pb * pb, t_pb_graph_node * pb_node) {
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

/** Calculates the power of a pb:
 * First checks if dynamic/static power is provided by user in arch file.  If not:
 * - Calculate power of all interconnect
 * - Call recursively for children
 * - If no children, must be a primitive.  Call primitive hander.
 */
static void power_usage_pb(t_power_usage * power_usage, t_pb * pb,
		t_pb_graph_node * pb_node) {

	t_power_usage power_usage_bufs_wires;
	t_power_usage power_usage_local_muxes;
	t_power_usage power_usage_children;
	t_power_usage power_usage_pin_toggle;
	t_power_usage power_usage_sub;

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
	boolean recursive_children;

	/* Get mode */
	if (pb) {
		pb_mode = pb->mode;
	} else {
		/* Default mode if not initialized (will only affect leakage power) */
		pb_mode = pb_type->pb_type_power->leakage_default_mode;
	}

	recursive_children = power_method_is_recursive(
			pb_node->pb_type->pb_type_power->estimation_method);

	power_zero_usage(&power_usage_sub);

	switch (pb_node->pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_IGNORE:
	case POWER_METHOD_SUM_OF_CHILDREN:
		break;

	case POWER_METHOD_ABSOLUTE:
		power_add_usage(power_usage, &pb_power->absolute_power_per_instance);
		power_component_add_usage(&pb_power->absolute_power_per_instance,
				POWER_COMPONENT_PB_OTHER);
		break;

	case POWER_METHOD_C_INTERNAL:
		power_zero_usage(&power_usage_sub);

		/* Just take the average density of inputs pins and use
		 * that with user-defined block capacitance and leakage */

		/* Average the activity of all pins */
		num_pins = 0;
		dens_avg = 0.;
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
		power_usage_sub.dynamic += power_calc_node_switching(
				pb_power->C_internal, dens_avg, g_solution_inf.T_crit);

		/* Leakage is an absolute */
		power_usage_sub.leakage +=
				pb_power->absolute_power_per_instance.leakage;

		/* Add to power of this PB */
		power_add_usage(power_usage, &power_usage_sub);

		// Add to component type
		power_component_add_usage(&power_usage_sub, POWER_COMPONENT_PB_OTHER);
		break;

	case POWER_METHOD_TOGGLE_PINS:
		power_zero_usage(&power_usage_pin_toggle);

		/* Add toggle power of each input pin */
		for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_usage_local_pin_toggle(&pin_power, pb,
						&pb_node->input_pins[port_idx][pin_idx]);
				power_add_usage(&power_usage_pin_toggle, &pin_power);
			}
		}

		/* Add toggle power of each output pin */
		for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_usage_local_pin_toggle(&pin_power, pb,
						&pb_node->output_pins[port_idx][pin_idx]);
				power_add_usage(&power_usage_pin_toggle, &pin_power);
			}
		}

		/* Add toggle power of each clock pin */
		for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
					pin_idx++) {
				t_power_usage pin_power;
				power_usage_local_pin_toggle(&pin_power, pb,
						&pb_node->clock_pins[port_idx][pin_idx]);
				power_add_usage(&power_usage_pin_toggle, &pin_power);
			}
		}

		/* Static is supplied as an absolute */
		power_usage_pin_toggle.leakage +=
				pb_power->absolute_power_per_instance.leakage;

		// Add to this PB power
		power_add_usage(power_usage, &power_usage_pin_toggle);

		// Add to component type power
		power_component_add_usage(&power_usage_pin_toggle,
				POWER_COMPONENT_PB_OTHER);
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
			power_usage_primitive(&power_usage_sub, pb, pb_node);

			// Add to power of this PB
			power_add_usage(power_usage, &power_usage_sub);

			// Add to power of component type
			power_component_add_usage(&power_usage_sub,
					POWER_COMPONENT_PB_PRIMITIVES);

		}

	} else {
		/* This node had children.  The power of this node is the sum of:
		 *  - Buffers/Wires in Interconnect from Parent to children
		 *  - Multiplexers in Interconnect from Parent to children
		 *  - Child nodes
		 */

		if (estimate_buffers_and_wire) {
			/* Check pins of all interconnect */
			power_usage_local_buffers_and_wires(&power_usage_bufs_wires, pb,
					pb_node);
			power_component_add_usage(&power_usage_bufs_wires,
					POWER_COMPONENT_PB_BUFS_WIRE);
			power_add_usage(
					&pb_node->pb_type->pb_type_power->power_usage_bufs_wires,
					&power_usage_bufs_wires);
			power_add_usage(power_usage, &power_usage_bufs_wires);
		}

		/* Interconnect Structures (multiplexers) */
		if (estimate_multiplexers) {
			power_zero_usage(&power_usage_local_muxes);
			for (interc_idx = 0;
					interc_idx < pb_type->modes[pb_mode].num_interconnect;
					interc_idx++) {
				power_usage_local_interc_mux(&power_usage_sub, pb,
						&pb_node->interconnect_pins[pb_mode][interc_idx]);
				power_add_usage(&power_usage_local_muxes, &power_usage_sub);

			}
			// Add to power of this PB
			power_add_usage(power_usage, &power_usage_local_muxes);

			// Add to component type power
			power_component_add_usage(&power_usage_local_muxes,
					POWER_COMPONENT_PB_INTERC_MUXES);

			// Add to power of this mode
			power_add_usage(
					&pb_node->pb_type->modes[pb_mode].mode_power->power_usage,
					&power_usage_local_muxes);
		}

		/* Add power for children */
		if (recursive_children) {
			power_zero_usage(&power_usage_children);
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

					power_usage_pb(&power_usage_sub, child_pb,
							child_pb_graph_node);
					power_add_usage(&power_usage_children, &power_usage_sub);
				}
			}
			// Add to power of this PB
			power_add_usage(power_usage, &power_usage_children);

			// Add to power of this mode
			power_add_usage(
					&pb_node->pb_type->modes[pb_mode].mode_power->power_usage,
					&power_usage_children);
		}
	}

	power_add_usage(&pb_node->pb_type->pb_type_power->power_usage, power_usage);
}

/* Resets the power stats for all physical blocks */
static void power_reset_pb_type(t_pb_type * pb_type) {
	int mode_idx;
	int child_idx;
	int interc_idx;

	power_zero_usage(&pb_type->pb_type_power->power_usage);
	power_zero_usage(&pb_type->pb_type_power->power_usage_bufs_wires);

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
static void power_usage_blocks(t_power_usage * power_usage) {
	int x, y, z;

	power_zero_usage(power_usage);

	power_reset_tile_usage();

	/* Loop through all grid locations */
	for (x = 0; x < nx + 2; x++) {
		for (y = 0; y < ny + 2; y++) {

			if ((grid[x][y].width_offset != 0)
					|| (grid[x][y].height_offset != 0)
					|| (grid[x][y].type == EMPTY_TYPE)) {
				continue;
			}

			for (z = 0; z < grid[x][y].type->capacity; z++) {
				t_pb * pb = NULL;
				t_power_usage pb_power;

				if (grid[x][y].blocks[z] != EMPTY
						&& grid[x][y].blocks[z] != INVALID) {
					pb = block[grid[x][y].blocks[z]].pb;
				}

				/* Calculate power of this CLB */
				power_usage_pb(&pb_power, pb, grid[x][y].type->pb_graph_head);
				power_add_usage(power_usage, &pb_power);
			}
		}
	}
	return;
}

/**
 * Calculates the total power usage from the clock network
 */
static void power_usage_clock(t_power_usage * power_usage,
		t_clock_arch * clock_arch) {
	int clock_idx;

	/* Initialization */
	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;

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

				// This will need to change for multi-clock
				clock_arch->clock_inf[clock_idx].period = g_solution_inf.T_crit;
			}
		}
		/* find the power dissipated by each clock network */
		power_usage_clock_single(&clock_power,
				&clock_arch->clock_inf[clock_idx]);
		power_add_usage(power_usage, &clock_power);
	}

	return;
}

/**
 * Calculates the power from a single spine-and-rib global clock
 */
static void power_usage_clock_single(t_power_usage * power_usage,
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
	int length;
	t_power_usage buffer_power;
	t_power_usage wire_power;
	float C_segment;
	float buffer_size;

	power_usage->dynamic = 0.;
	power_usage->leakage = 0.;

	/* Check if this clock is active - this is used for calculating leakage */
	if (single_clock->dens) {
	} else {
		assert(0);
	}

	C_segment = g_power_commonly_used->tile_length * single_clock->C_wire;
	if (single_clock->autosize_buffer) {
		buffer_size = 1 + C_segment / g_power_commonly_used->INV_1X_C_in;
	} else {
		buffer_size = single_clock->buffer_size;
	}

	/* Calculate the capacitance and leakage power for the clock buffer */
	power_usage_inverter(&clock_buffer_power, single_clock->dens,
			single_clock->prob, buffer_size, single_clock->period);

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

	power_usage_wire(&wire_power, length * C_segment, single_clock->dens,
			single_clock->period);
	power_add_usage(power_usage, &wire_power);
	power_component_add_usage(&wire_power, POWER_COMPONENT_CLOCK_WIRE);

	return;
}

/* Frees a multiplexer graph */
static void dealloc_mux_graph(t_mux_node * node) {
	dealloc_mux_graph_rec(node);
	free(node);
}

static void dealloc_mux_graph_rec(t_mux_node * node) {
	int child_idx;

	/* Dealloc Children */
	if (node->level != 0) {
		for (child_idx = 0; child_idx < node->num_inputs; child_idx++) {
			dealloc_mux_graph_rec(&node->children[child_idx]);
		}
		free(node->children);
	}
}

/**
 * Calculates the power of the entire routing fabric (not local routing
 */
static void power_usage_routing(t_power_usage * power_usage,
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

		for (trace = trace_head[net_idx]; trace != NULL; trace = trace->next) {
			rr_node_power[trace->index].visited = FALSE;
		}
	}

	/* Populate net indices into rr graph */
	for (net_idx = 0; net_idx < num_nets; net_idx++) {
		struct s_trace * trace;

		for (trace = trace_head[net_idx]; trace != NULL; trace = trace->next) {
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
		//float C_per_seg_split;
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
				power_usage_mux_multilevel(&sub_power_usage,
						power_get_mux_arch(node->fan_in,
								g_power_arch->mux_transistor_size),
						node_power->in_prob, node_power->in_dens,
						node_power->selected_input, TRUE,
						g_solution_inf.T_crit);
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

			wire_length = 0;
			if (node->type == CHANX) {
				wire_length = node->xhigh - node->xlow + 1;
			} else if (node->type == CHANY) {
				wire_length = node->yhigh - node->ylow + 1;
			}
			C_wire =
					wire_length
							* segment_inf[rr_indexed_data[node->cost_index].seg_index].Cmetal;
			//(double)g_power_commonly_used->tile_length);
			assert(node_power->selected_input < node->fan_in);

			/* Multiplexor */
			power_usage_mux_multilevel(&sub_power_usage,
					power_get_mux_arch(node->fan_in,
							g_power_arch->mux_transistor_size),
					node_power->in_prob, node_power->in_dens,
					node_power->selected_input, TRUE, g_solution_inf.T_crit);
			power_add_usage(power_usage, &sub_power_usage);
			power_component_add_usage(&sub_power_usage,
					POWER_COMPONENT_ROUTE_SB);

			/* Buffer Size */
			switch (switch_inf[node_power->driver_switch_type].power_buffer_type) {
			case POWER_BUFFER_TYPE_AUTO:
				/*
				 C_per_seg_split = ((float) node->num_edges
				 * g_power_commonly_used->INV_1X_C_in + C_wire);
				 // / (float) g_power_arch->seg_buffer_split;
				 buffer_size = power_buffer_size_from_logical_effort(
				 C_per_seg_split);
				 buffer_size = max(buffer_size, 1.0F);
				 */
				buffer_size = power_calc_buffer_size_from_Cout(
						switch_inf[node_power->driver_switch_type].Cout);
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
				buffer_size = 0.;
				assert(0);
				break;
			}

			g_power_commonly_used->num_sb_buffers++;
			g_power_commonly_used->total_sb_buffer_size += buffer_size;

			/*
			 g_power_commonly_used->num_sb_buffers +=
			 g_power_arch->seg_buffer_split;
			 g_power_commonly_used->total_sb_buffer_size += buffer_size
			 * g_power_arch->seg_buffer_split;
			 */

			/* Buffer */
			power_usage_buffer(&sub_power_usage, buffer_size,
					node_power->in_prob[node_power->selected_input],
					node_power->in_dens[node_power->selected_input], TRUE,
					g_solution_inf.T_crit);
			power_add_usage(power_usage, &sub_power_usage);
			power_component_add_usage(&sub_power_usage,
					POWER_COMPONENT_ROUTE_SB);

			/* Wire Capacitance */
			power_usage_wire(&sub_power_usage, C_wire,
					clb_net_density(node->net_num), g_solution_inf.T_crit);
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
						g_solution_inf.T_crit);
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
						FALSE, g_solution_inf.T_crit);
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

void power_alloc_and_init_pb_pin(t_pb_graph_pin * pin) {
	int port_idx;
	t_port * port_to_find;
	t_pb_graph_node * node = pin->parent_node;
	boolean found;

	pin->pin_power = (t_pb_graph_pin_power*) malloc(
			sizeof(t_pb_graph_pin_power));
	pin->pin_power->C_wire = 0.;
	pin->pin_power->buffer_size = 0.;
	pin->pin_power->scaled_by_pin = NULL;

	if (pin->port->port_power->scaled_by_port) {

		port_to_find = pin->port->port_power->scaled_by_port;

		/*pin->pin_power->scaled_by_pin =
		 get_pb_graph_node_pin_from_model_port_pin(
		 port_to_find->model_port,
		 pin->port->port_power->scaled_by_port_pin_idx,
		 pin->parent_node);*/
		/* Search input, output, clock ports */

		found = FALSE;
		for (port_idx = 0; port_idx < node->num_input_ports; port_idx++) {
			if (node->input_pins[port_idx][0].port == port_to_find) {
				pin->pin_power->scaled_by_pin =
						&node->input_pins[port_idx][pin->port->port_power->scaled_by_port_pin_idx];
				found = TRUE;
				break;
			}
		}
		if (!found) {
			for (port_idx = 0; port_idx < node->num_output_ports; port_idx++) {
				if (node->output_pins[port_idx][0].port == port_to_find) {
					pin->pin_power->scaled_by_pin =
							&node->output_pins[port_idx][pin->port->port_power->scaled_by_port_pin_idx];
					found = TRUE;
					break;
				}
			}
		}
		if (!found) {
			for (port_idx = 0; port_idx < node->num_clock_ports; port_idx++) {
				if (node->clock_pins[port_idx][0].port == port_to_find) {
					pin->pin_power->scaled_by_pin =
							&node->clock_pins[port_idx][pin->port->port_power->scaled_by_port_pin_idx];
					found = TRUE;
					break;
				}
			}
		}
		assert(found);

		assert(pin->pin_power->scaled_by_pin);
	}
}

void power_init_pb_pins_rec(t_pb_graph_node * pb_node) {
	int mode;
	int type;
	int pb;
	int port_idx;
	int pin_idx;

	for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
				pin_idx++) {
			power_alloc_and_init_pb_pin(
					&pb_node->input_pins[port_idx][pin_idx]);
		}
	}

	for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
				pin_idx++) {
			power_alloc_and_init_pb_pin(
					&pb_node->output_pins[port_idx][pin_idx]);
		}
	}

	for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
		for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
				pin_idx++) {
			power_alloc_and_init_pb_pin(
					&pb_node->clock_pins[port_idx][pin_idx]);
		}
	}

	for (mode = 0; mode < pb_node->pb_type->num_modes; mode++) {
		for (type = 0;
				type < pb_node->pb_type->modes[mode].num_pb_type_children;
				type++) {
			for (pb = 0;
					pb
							< pb_node->pb_type->modes[mode].pb_type_children[type].num_pb;
					pb++) {
				power_init_pb_pins_rec(
						&pb_node->child_pb_graph_nodes[mode][type][pb]);
			}
		}
	}
}

void power_pb_pins_init() {
	int type_idx;

	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_graph_head) {
			power_init_pb_pins_rec(type_descriptors[type_idx].pb_graph_head);
		}
	}
}

void power_routing_init(t_det_routing_arch * routing_arch) {
	int net_idx;
	int rr_node_idx;
	int max_fanin;
	int max_IPIN_fanin;
	int max_seg_to_IPIN_fanout;
	int max_seg_to_seg_fanout;
	int max_seg_fanout;

	/* Copy probability/density values to new netlist */
	if (!clb_net_power) {
		clb_net_power = (t_net_power*) my_calloc(num_nets, sizeof(t_net_power));
	}
	for (net_idx = 0; net_idx < num_nets; net_idx++) {
		clb_net_power[net_idx].probability =
				vpack_net_power[clb_to_vpack_net_mapping[net_idx]].probability;
		clb_net_power[net_idx].density =
				vpack_net_power[clb_to_vpack_net_mapping[net_idx]].density;
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
			max_IPIN_fanin = max(max_IPIN_fanin,
					static_cast<int>(node->fan_in));
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
			max_seg_to_IPIN_fanout = max(max_seg_to_IPIN_fanout,
					fanout_to_IPIN);
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
							rr_node_power[node->edges[edge_idx]].driver_switch_type
									== node->switches[edge_idx]);
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
}

/**
 * Initialization for all power-related functions
 */
boolean power_init(char * power_out_filepath,
		char * cmos_tech_behavior_filepath, t_arch * arch,
		t_det_routing_arch * routing_arch) {
	boolean error = FALSE;

	/* Set global power architecture & options */
	g_power_arch = arch->power;
	g_power_commonly_used = new t_power_commonly_used;
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
	power_tech_init(cmos_tech_behavior_filepath);

	/* Low-Level Initialization */
	power_lowlevel_init();

	/* Initialize sub-modules */
	power_components_init();

	/* Perform callibration */
	power_callibrate();

	/* Initialize routing information */
	power_routing_init(routing_arch);

// Allocates power structures for each pb pin
	power_pb_pins_init();

	/* Size all components */
	power_sizing_init(arch);

	//power_print_spice_comparison();
//	power_print_callibration();

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
	for (std::map<float, t_power_mux_info*>::iterator it =
			g_power_commonly_used->mux_info.begin();
			it != g_power_commonly_used->mux_info.end(); it++) {
		t_power_mux_info * mux_info = it->second;
		for (mux_size = 1; mux_size <= mux_info->mux_arch_max_size;
				mux_size++) {
			dealloc_mux_graph(mux_info->mux_arch[mux_size].mux_graph_head);
		}
		delete mux_info;
	}
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

#if 0
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

	float clb_power_total = power_component_get_usage_sum(
			POWER_COMPONENT_PB);
	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (!type_descriptors[type_idx].pb_type) {
			continue;
		}

		power_print_pb_usage_recursive(fp, type_descriptors[type_idx].pb_type,
				0, clb_power_total, clb_power_total);
	}
}
#endif

/*
 static void power_print_stats(FILE * fp) {
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
 */

static char * power_estimation_method_name(
		e_power_estimation_method power_method) {
	switch (power_method) {
	case POWER_METHOD_UNDEFINED:
		return "Undefined";
	case POWER_METHOD_IGNORE:
		return "Ignore";
	case POWER_METHOD_AUTO_SIZES:
		return "Transistor Auto-Size";
	case POWER_METHOD_SPECIFY_SIZES:
		return "Transistor Specify-Size";
	case POWER_METHOD_TOGGLE_PINS:
		return "Pin-Toggle";
	case POWER_METHOD_C_INTERNAL:
		return "C-Internal";
	case POWER_METHOD_ABSOLUTE:
		return "Absolute";
	case POWER_METHOD_SUM_OF_CHILDREN:
		return "Sum of Children";
	default:
		return "Unkown";
	}
}

static void power_print_breakdown_pb_rec(FILE * fp, t_pb_type * pb_type,
		int indent) {
	int mode_idx;
	int child_idx;
	int i;
	char buf[51];
	int child_indent;
	int interc_idx;
	t_mode * mode;
	t_power_usage interc_usage;
	e_power_estimation_method est_method =
			pb_type->pb_type_power->estimation_method;
	float total_power = power_component_get_usage_sum(POWER_COMPONENT_TOTAL);
	t_pb_type_power * pb_power = pb_type->pb_type_power;

	for (i = 0; i < indent; i++) {
		buf[i] = ' ';
	}
	strncpy(buf + indent, pb_type->name, 50 - indent);
	buf[50] = '\0';
	buf[strlen((pb_type->name)) + indent] = '\0';
	power_print_breakdown_entry(fp, indent, POWER_BREAKDOWN_ENTRY_TYPE_PB,
			pb_type->name, power_sum_usage(&pb_power->power_usage), total_power,
			power_perc_dynamic(&pb_power->power_usage),
			power_estimation_method_name(
					pb_type->pb_type_power->estimation_method));

	if (power_method_is_transistor_level(
			pb_type->pb_type_power->estimation_method)) {
		/* Local bufs and wires */
		power_print_breakdown_entry(fp, indent + 1,
				POWER_BREAKDOWN_ENTRY_TYPE_BUFS_WIRES, "Bufs/Wires",
				power_sum_usage(&pb_power->power_usage_bufs_wires), total_power,
				power_perc_dynamic(&pb_power->power_usage_bufs_wires), NULL);
	}

	if (power_method_is_recursive(est_method)) {
		if (pb_type->num_modes > 1) {
			child_indent = indent + 2;
		} else {
			child_indent = indent + 1;
		}

		for (mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
			mode = &pb_type->modes[mode_idx];

			if (pb_type->num_modes > 1) {
				power_print_breakdown_entry(fp, indent + 1,
						POWER_BREAKDOWN_ENTRY_TYPE_MODE, mode->name,
						power_sum_usage(&mode->mode_power->power_usage),
						total_power,
						power_perc_dynamic(&mode->mode_power->power_usage),
						NULL);
			}

			/* Interconnect Power */
			power_zero_usage(&interc_usage);

			/* Sum the interconnect */
			if (power_method_is_transistor_level(est_method)) {
				for (interc_idx = 0; interc_idx < mode->num_interconnect;
						interc_idx++) {
					power_add_usage(&interc_usage,
							&mode->interconnect[interc_idx].interconnect_power->power_usage);
				}
				if (mode->num_interconnect) {
					power_print_breakdown_entry(fp, child_indent,
							POWER_BREAKDOWN_ENTRY_TYPE_INTERC, "Interc:",
							power_sum_usage(&interc_usage), total_power,
							power_perc_dynamic(&interc_usage), NULL);
				}

				/* Print Interconnect Breakdown */
				for (interc_idx = 0; interc_idx < mode->num_interconnect;
						interc_idx++) {
					t_interconnect * interc = &mode->interconnect[interc_idx];
					if (interc->type == DIRECT_INTERC) {
						// no power - skip
					} else {
						power_print_breakdown_entry(fp, child_indent + 1,
								POWER_BREAKDOWN_ENTRY_TYPE_INTERC, interc->name,
								power_sum_usage(
										&interc->interconnect_power->power_usage),
								total_power,
								power_perc_dynamic(
										&interc->interconnect_power->power_usage),
								NULL);
					}
				}
			}

			for (child_idx = 0;
					child_idx < pb_type->modes[mode_idx].num_pb_type_children;
					child_idx++) {
				power_print_breakdown_pb_rec(fp,
						&pb_type->modes[mode_idx].pb_type_children[child_idx],
						child_indent);
			}
		}
	}
}

static void power_print_summary(FILE * fp, t_vpr_setup vpr_setup) {
	char * arch;
	char * arch_new;

	/* Extract filename from full path */
	arch = vpr_setup.FileNameOpts.ArchFile;

	arch_new = strrchr(vpr_setup.FileNameOpts.ArchFile, '/');
	if (arch_new)
		arch = arch_new + 1;

	arch_new = strrchr(vpr_setup.FileNameOpts.ArchFile, '\\');
	if (arch_new)
		arch = arch_new + 1;

	fprintf(g_power_output->out, "Circuit: %s\n",
			vpr_setup.FileNameOpts.CircuitName);
	fprintf(g_power_output->out, "Architecture: %s\n", arch);
	fprintf(fp, "Technology (nm): %.0f\n",
			g_power_tech->tech_size * CONVERT_NM_PER_M);
	fprintf(fp, "Voltage: %.2f\n", g_power_tech->Vdd);
	fprintf(fp, "Temperature: %g\n", g_power_tech->temperature);
	fprintf(fp, "Critical Path: %g\n", g_solution_inf.T_crit);
	fprintf(fp, "Size of FPGA: %d x %d\n", nx, ny);
	fprintf(fp, "Channel Width: %d\n", g_solution_inf.channel_width);
	fprintf(fp, "\n");
}

/*
 * Top-level function for the power module.
 * Calculates the average power of the entire FPGA (watts),
 * and prints it to the output file
 * - run_time_s: (Return value) The total runtime in seconds (us accuracy)
 */
e_power_ret_code power_total(float * run_time_s, t_vpr_setup vpr_setup,
		t_arch * arch, t_det_routing_arch * routing_arch) {
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
	/* Routing */
	power_usage_routing(&sub_power_usage, routing_arch, arch->Segments);
	power_add_usage(&total_power, &sub_power_usage);
	power_component_add_usage(&sub_power_usage, POWER_COMPONENT_ROUTING);

	/* Clock  */
	power_usage_clock(&sub_power_usage, arch->clocks);
	power_add_usage(&total_power, &sub_power_usage);
	power_component_add_usage(&sub_power_usage, POWER_COMPONENT_CLOCK);

	/* CLBs */
	power_usage_blocks(&clb_power_usage);
	power_add_usage(&total_power, &clb_power_usage);
	power_component_add_usage(&clb_power_usage, POWER_COMPONENT_PB);

	power_component_add_usage(&total_power, POWER_COMPONENT_TOTAL);

	power_print_title(g_power_output->out, "Summary");
	power_print_summary(g_power_output->out, vpr_setup);

	/* Print Error & Warning Logs */
	output_logs(g_power_output->out, g_power_output->logs,
			g_power_output->num_logs);

	//power_print_title(g_power_output->out, "Statistics");
	//power_print_stats(g_power_output->out);

	power_print_title(g_power_output->out, "Power Breakdown");
	power_print_breakdown_summary(g_power_output->out);

	power_print_title(g_power_output->out, "Power Breakdown by PB");
	power_print_breakdown_pb(g_power_output->out);

	//power_print_title(g_power_output->out, "Spice Comparison");
	//power_print_spice_comparison();

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

/**
 * Prints the power usage for all components
 * - fp: File descripter to print out to
 */
static void power_print_breakdown_summary(FILE * fp) {
	power_print_breakdown_entry(fp, 0, POWER_BREAKDOWN_ENTRY_TYPE_TITLE, NULL,
			0., 0., 0., NULL);
	power_print_breakdown_component(fp, "Total", POWER_COMPONENT_TOTAL, 0);
	fprintf(fp, "\n");
}

static void power_print_breakdown_pb(FILE * fp) {
	fprintf(fp,
			"This sections provides a detailed breakdown of power usage by PB (physical\n"
					"block). For each PB, the power is listed, which is the sum power of all\n"
					"instances of the block.  It also indicates its percentage of total power (entire\n"
					"FPGA), as well as the percentage of its power that is dynamic (vs. static).  It\n"
					"also indicates the method used for power estimation.\n\n"
					"The data includes:\n"
					"\tModes:\t\tWhen a pb contains multiple modes, each mode is "
					"listed, with\n\t\t\t\tits power statistics.\n"
					"\tBufs/Wires:\tPower of all local "
					"buffers and local wire switching\n"
					"\t\t\t\t(transistor-level estimation only).\n"
					"\tInterc:\t\tPower of local interconnect multiplexers (transistor-\n"
					"\t\t\t\tlevel estimation only)\n\n"
					"Description of Estimation Methods:\n"
					"\tTransistor Auto-Size: Transistor-level power estimation. Local buffers and\n"
					"\t\twire lengths are automatically sized. This is the default estimation\n"
					"\t\tmethod.\n"
					"\tTransistor Specify-Size: Transistor-level power estimation. Local buffers\n"
					"\t\tand wire lengths are only inserted where specified by the user in the\n"
					"\t\tarchitecture file.\n"
					"\tPin-Toggle: Dynamic power is calculated using enery-per-toggle of the PB\n"
					"\t\tinput pins. Static power is absolute.\n"
					"\tC-Internal: Dynamic power is calculated using an internal equivalent\n"
					"\t\tcapacitance for PB type. Static power is absolute.\n"
					"\tAbsolute: Dynamic and static power are absolutes from the architecture file.\n"
					"\tSum of Children: Power of PB is only the sum of all child PBs; interconnect\n"
					"\t\tbetween the PB and its children is ignored.\n"
					"\tIgnore: Power of PB is ignored.\n\n\n");

	power_print_breakdown_entry(fp, 0, POWER_BREAKDOWN_ENTRY_TYPE_TITLE, NULL,
			0., 0., 0., NULL);

	for (int type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_type) {
			power_print_breakdown_pb_rec(g_power_output->out,
					type_descriptors[type_idx].pb_type, 0);
		}
	}
	fprintf(fp, "\n");
}

/**
 * Internal recurseive function, used by power_component_print_usage
 */
static void power_print_breakdown_component(FILE * fp, char * name,
		e_power_component_type type, int indent_level) {
	power_print_breakdown_entry(fp, indent_level,
			POWER_BREAKDOWN_ENTRY_TYPE_COMPONENT, name,
			power_sum_usage(&g_power_by_component.components[type]),
			power_sum_usage(
					&g_power_by_component.components[POWER_COMPONENT_TOTAL]),
			power_perc_dynamic(&g_power_by_component.components[type]), NULL);

	switch (type) {
	case (POWER_COMPONENT_TOTAL):
		power_print_breakdown_component(fp, "Routing", POWER_COMPONENT_ROUTING,
				indent_level + 1);
		power_print_breakdown_component(fp, "PB Types", POWER_COMPONENT_PB,
				indent_level + 1);
		power_print_breakdown_component(fp, "Clock", POWER_COMPONENT_CLOCK,
				indent_level + 1);
		break;
	case (POWER_COMPONENT_ROUTING):
		power_print_breakdown_component(fp, "Switch Box",
				POWER_COMPONENT_ROUTE_SB, indent_level + 1);
		power_print_breakdown_component(fp, "Connection Box",
				POWER_COMPONENT_ROUTE_CB, indent_level + 1);
		power_print_breakdown_component(fp, "Global Wires",
				POWER_COMPONENT_ROUTE_GLB_WIRE, indent_level + 1);
		break;
	case (POWER_COMPONENT_CLOCK):
		/*
		 power_print_breakdown_component(fp, "Clock Buffers",
		 POWER_COMPONENT_CLOCK_BUFFER, indent_level + 1);
		 power_print_breakdown_component(fp, "Clock Wires",
		 POWER_COMPONENT_CLOCK_WIRE, indent_level + 1);
		 */
		break;
	case (POWER_COMPONENT_PB):
		power_print_breakdown_component(fp, "Primitives",
				POWER_COMPONENT_PB_PRIMITIVES, indent_level + 1);
		power_print_breakdown_component(fp, "Interc Structures",
				POWER_COMPONENT_PB_INTERC_MUXES, indent_level + 1);
		power_print_breakdown_component(fp, "Buffers and Wires",
				POWER_COMPONENT_PB_BUFS_WIRE, indent_level + 1);
		power_print_breakdown_component(fp, "Other Estimation Methods",
				POWER_COMPONENT_PB_OTHER, indent_level + 1);
		break;
	default:
		break;
	}
}

static void power_print_breakdown_entry(FILE * fp, int indent,
		e_power_breakdown_entry_type type, char * name, float power,
		float total_power, float perc_dyn, char * method) {
	const int buf_size = 32;
	char buf[buf_size];

	switch (type) {
	case POWER_BREAKDOWN_ENTRY_TYPE_TITLE:
		fprintf(fp, "%-*s%-12s%-12s%-12s%-12s\n\n", buf_size, "Component",
				"Power (W)", "%-Total", "%-Dynamic", "Method");
		break;
	case POWER_BREAKDOWN_ENTRY_TYPE_MODE:
		for (int i = 0; i < indent; i++)
			buf[i] = ' ';
		strcpy(buf + indent, "Mode:");
		strncpy(buf + indent + 5, name, buf_size - indent - 6);
		fprintf(fp, "%-*s%-12.4g%-12.4g%-12.4g\n", buf_size, buf, power,
				power / total_power, perc_dyn);
		break;
	case POWER_BREAKDOWN_ENTRY_TYPE_COMPONENT:
	case POWER_BREAKDOWN_ENTRY_TYPE_INTERC:
	case POWER_BREAKDOWN_ENTRY_TYPE_BUFS_WIRES:
		for (int i = 0; i < indent; i++)
			buf[i] = ' ';
		strncpy(buf + indent, name, buf_size - indent - 1);
		buf[buf_size - 1] = '\0';

		fprintf(fp, "%-*s%-12.4g%-12.4g%-12.4g\n", buf_size, buf, power,
				power / total_power, perc_dyn);
		break;
	case POWER_BREAKDOWN_ENTRY_TYPE_PB:
		for (int i = 0; i < indent; i++)
			buf[i] = ' ';
		strncpy(buf + indent, name, buf_size - indent - 1);
		buf[buf_size - 1] = '\0';

		fprintf(fp, "%-*s%-12.4g%-12.4g%-12.4g%-12s\n", buf_size, buf, power,
				power / total_power, perc_dyn, method);
		break;
	default:
		break;
	}
}
