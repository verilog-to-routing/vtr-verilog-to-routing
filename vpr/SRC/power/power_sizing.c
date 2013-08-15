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
 * The functions in this file are used to count the transistors in the
 * FPGA, for physical size estimations
 */

/************************* INCLUDES *********************************/
#include <cstring>
using namespace std;

#include <assert.h>

#include "power_sizing.h"
#include "power.h"
#include "globals.h"
#include "power_util.h"
/************************* GLOBALS **********************************/
static double g_MTA_area;

/************************* FUNCTION DELCARATIONS ********************/
static double power_count_transistors_connectionbox(void);
static double power_count_transistors_mux(t_mux_arch * mux_arch);
static double power_count_transistors_mux_node(t_mux_node * mux_node,
		float transistor_size);
static void power_mux_node_max_inputs(t_mux_node * mux_node,
		float * max_inputs);
static double power_count_transistors_interc(t_interconnect * interc);
static double power_count_transistors_pb_node(t_pb_graph_node * pb_node);
static double power_count_transistors_switchbox(t_arch * arch);
static double power_count_transistors_primitive(t_pb_type * pb_type);
static double power_count_transistors_LUT(int LUT_inputs,
		float transistor_size);
static double power_count_transistors_FF(float size);
static double power_count_transistor_SRAM_bit(void);
static double power_count_transistors_inv(float size);
static double power_count_transistors_trans_gate(float size);
static double power_count_transistors_levr();
static double power_MTAs(float size);
static void power_size_pin_buffers_and_wires(t_pb_graph_pin * pin,
		boolean pin_is_an_input);
static double power_transistors_for_pb_node(t_pb_graph_node * pb_node);
static double power_transistors_per_tile(t_arch * arch);
static void power_size_pb(void);
static void power_size_pb_rec(t_pb_graph_node * pb_node);
static void power_size_pin_to_interconnect(t_interconnect * interc,
		int * fanout, float * wirelength);
static double power_MTAs(float W_size);
static double power_MTAs_L(float L_size);
/************************* FUNCTION DEFINITIONS *********************/

/**
 *  Counts the number of transistors in the largest connection box
 */
static double power_count_transistors_connectionbox(void) {
	double transistor_cnt = 0.;
	int CLB_inputs;
	float buffer_size;

	assert(FILL_TYPE->pb_graph_head->num_input_ports == 1);
	CLB_inputs = FILL_TYPE->pb_graph_head->num_input_pins[0];

	/* Buffers from Tracks */
	buffer_size = g_power_commonly_used->max_seg_to_IPIN_fanout
			* (g_power_commonly_used->NMOS_1X_C_d
					/ g_power_commonly_used->INV_1X_C_in)
			/ g_power_arch->logical_effort_factor;
	buffer_size = max(1.0F, buffer_size);
	transistor_cnt += g_solution_inf.channel_width
			* power_count_transistors_buffer(buffer_size);

	/* Muxes to IPINs */
	transistor_cnt += CLB_inputs
			* power_count_transistors_mux(
					power_get_mux_arch(g_power_commonly_used->max_IPIN_fanin,
							g_power_arch->mux_transistor_size));

	return transistor_cnt;
}

/**
 * Counts the number of transistors in a buffer.
 * - buffer_size: The final stage size of the buffer, relative to a minimum sized inverter
 */
double power_count_transistors_buffer(float buffer_size) {
	int stages;
	float effort;
	float stage_size;
	int stage_idx;
	double transistor_cnt = 0.;

	stages = power_calc_buffer_num_stages(buffer_size,
			g_power_arch->logical_effort_factor);
	effort = calc_buffer_stage_effort(stages, buffer_size);

	stage_size = 1;
	for (stage_idx = 0; stage_idx < stages; stage_idx++) {
		transistor_cnt += power_count_transistors_inv(stage_size);
		stage_size *= effort;
	}

	return transistor_cnt;
}

/**
 * Counts the number of transistors in a multiplexer
 * - mux_arch: The architecture of the multiplexer
 */
static double power_count_transistors_mux(t_mux_arch * mux_arch) {
	double transistor_cnt = 0.;
	int lvl_idx;
	float * max_inputs;

	/* SRAM bits */
	max_inputs = (float*) my_calloc(mux_arch->levels, sizeof(float));
	for (lvl_idx = 0; lvl_idx < mux_arch->levels; lvl_idx++) {
		max_inputs[lvl_idx] = 0.;
	}
	power_mux_node_max_inputs(mux_arch->mux_graph_head, max_inputs);

	for (lvl_idx = 0; lvl_idx < mux_arch->levels; lvl_idx++) {
		/* Assume there is decoder logic */
		transistor_cnt += ceil(log(max_inputs[lvl_idx]) / log((double) 2.0))
				* power_count_transistor_SRAM_bit();

		/*
		 if (mux_arch->encoding_types[lvl_idx] == ENCODING_DECODER) {
		 transistor_cnt += ceil(log2((float)max_inputs[lvl_idx]))
		 * power_cnt_transistor_SRAM_bit();
		 // TODO - Size of decoder
		 } else if (mux_arch->encoding_types[lvl_idx] == ENCODING_ONE_HOT) {
		 transistor_cnt += max_inputs[lvl_idx]
		 * power_cnt_transistor_SRAM_bit();
		 } else {
		 assert(0);
		 }
		 */
	}

	transistor_cnt += power_count_transistors_mux_node(mux_arch->mux_graph_head,
			mux_arch->transistor_size);
	free(max_inputs);
	return transistor_cnt;
}

/**
 *  This function is used recursively to determine the largest single-level
 *  multiplexer at each level of a multi-level multiplexer
 */
static void power_mux_node_max_inputs(t_mux_node * mux_node,
		float * max_inputs) {

	max_inputs[mux_node->level] = max(max_inputs[mux_node->level],
			static_cast<float>(mux_node->num_inputs));

	if (mux_node->level != 0) {
		int child_idx;

		for (child_idx = 0; child_idx < mux_node->num_inputs; child_idx++) {
			power_mux_node_max_inputs(&mux_node->children[child_idx],
					max_inputs);
		}
	}
}

/**
 * This function is used recursively to count the number of transistors in a multiplexer
 */
static double power_count_transistors_mux_node(t_mux_node * mux_node,
		float transistor_size) {
	int input_idx;
	double transistor_cnt = 0.;

	if (mux_node->num_inputs != 1) {
		for (input_idx = 0; input_idx < mux_node->num_inputs; input_idx++) {
			/* Single Pass transistor */
			transistor_cnt += power_MTAs(transistor_size);

			/* Child MUX */
			if (mux_node->level != 0) {
				transistor_cnt += power_count_transistors_mux_node(
						&mux_node->children[input_idx], transistor_size);
			}
		}
	}

	return transistor_cnt;
}

/**
 * This function returns the number of transistors in an interconnect structure
 */
static double power_count_transistors_interc(t_interconnect * interc) {
	double transistor_cnt = 0.;

	switch (interc->type) {
	case DIRECT_INTERC:
		/* No transistors */
		break;
	case MUX_INTERC:
	case COMPLETE_INTERC:
		/* Bus based interconnect:
		 * - Each output port requires a (num_input_ports:1) bus-based multiplexor.
		 * - The number of muxes required for bus based multiplexors is equivalent to
		 * the width of the bus (num_pins_per_port).
		 */
		transistor_cnt += interc->interconnect_power->num_output_ports
				* interc->interconnect_power->num_pins_per_port
				* power_count_transistors_mux(
						power_get_mux_arch(
								interc->interconnect_power->num_input_ports,
								g_power_arch->mux_transistor_size));
		break;
	default:
		assert(0);
	}

	interc->interconnect_power->transistor_cnt = transistor_cnt;
	return transistor_cnt;
}

void power_sizing_init(t_arch * arch) {
	float transistors_per_tile;

	// tech size = 2 lambda, so lambda^2/4.0 = tech^2
	g_MTA_area = ((POWER_MTA_L * POWER_MTA_W)/ 4.0)*pow(g_power_tech->tech_size,
			(float) 2.0);

	// Determines physical size of different PBs
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
}

/**
 * This functions counts the number of transistors in all structures in the FPGA.
 * It returns the number of transistors in a grid of the FPGA (logic block,
 * switch box, 2 connection boxes)
 */
static double power_transistors_per_tile(t_arch * arch) {
	double transistor_cnt = 0.;

	transistor_cnt += power_transistors_for_pb_node(FILL_TYPE->pb_graph_head);

	transistor_cnt += 2 * power_count_transistors_switchbox(arch);

	transistor_cnt += 2 * power_count_transistors_connectionbox();

	return transistor_cnt;
}

static double power_transistors_for_pb_node(t_pb_graph_node * pb_node) {
	return pb_node->pb_node_power->transistor_cnt_interc
			+ pb_node->pb_node_power->transistor_cnt_pb_children
			+ pb_node->pb_node_power->transistor_cnt_buffers;
}

/**
 * This function counts the number of transistors for a given physical block type
 */
static double power_count_transistors_pb_node(t_pb_graph_node * pb_node) {
	int mode_idx;
	int interc;
	int child;
	int pb_idx;

	double tc_children_max = 0;
	double tc_interc_max = 0;
	boolean ignore_interc = FALSE;

	t_pb_type * pb_type = pb_node->pb_type;

	/* Check if this is a leaf node, or whether it has children */
	if (pb_type->num_modes == 0) {
		/* Leaf node */
		tc_interc_max = 0;
		tc_children_max = power_count_transistors_primitive(pb_type);
	} else {
		/* Find max transistor count between all modes */
		for (mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {

			double tc_children = 0;
			double tc_interc = 0;

			t_mode * mode = &pb_type->modes[mode_idx];

			if (pb_type->class_type == LUT_CLASS) {
				/* LUTs will have a child node that is used for routing purposes
				 * For the routing algorithms it is completely connected; however,
				 * this interconnect does not exist in FPGA hardware and should
				 * be ignored for power calculations. */
				ignore_interc = TRUE;
			}

			/* Count Interconnect Transistors */
			if (!ignore_interc) {
				for (interc = 0; interc < mode->num_interconnect; interc++) {
					tc_interc += power_count_transistors_interc(
							&mode->interconnect[interc]);
				}
			}
			tc_interc_max = max(tc_interc_max, tc_interc);

			/* Count Child PB Types */
			for (child = 0; child < mode->num_pb_type_children; child++) {
				t_pb_type * child_type = &mode->pb_type_children[child];

				for (pb_idx = 0; pb_idx < child_type->num_pb; pb_idx++) {
					tc_children +=
							power_transistors_for_pb_node(
									&pb_node->child_pb_graph_nodes[mode_idx][child][pb_idx]);
				}
			}

			tc_children_max = max(tc_children_max, tc_children);
		}
	}

	pb_node->pb_node_power->transistor_cnt_interc = tc_interc_max;
	pb_node->pb_node_power->transistor_cnt_pb_children = tc_children_max;

	return (tc_interc_max + tc_children_max);
}

/**
 * This function counts the maximum number of transistors in a switch box
 */
static double power_count_transistors_switchbox(t_arch * arch) {
	double transistor_cnt = 0.;
	double transistors_per_buf_mux = 0.;
	int seg_idx;

	/* Buffer */
	transistors_per_buf_mux += power_count_transistors_buffer(
			(float) g_power_commonly_used->max_seg_fanout
					/ g_power_arch->logical_effort_factor);

	/* Multiplexor */
	transistors_per_buf_mux += power_count_transistors_mux(
			power_get_mux_arch(g_power_commonly_used->max_routing_mux_size,
					g_power_arch->mux_transistor_size));

	for (seg_idx = 0; seg_idx < arch->num_segments; seg_idx++) {
		/* In each switchbox, the different types of segments occur with relative freqencies.
		 * Thus the total number of wires of each segment type is (#tracks * freq * 2).
		 * The (x2) factor accounts for vertical and horizontal tracks.
		 * Of the wires of each segment type only (1/seglength) will have a mux&buffer.
		 */
		float freq_frac = (float) arch->Segments[seg_idx].frequency
				/ (float) MAX_CHANNEL_WIDTH;

		transistor_cnt += transistors_per_buf_mux * 2 * freq_frac
				* g_solution_inf.channel_width
				* (1 / (float) arch->Segments[seg_idx].length);
	}

	return transistor_cnt;
}

/**
 * This function calculates the number of transistors for a primitive physical block
 */
static double power_count_transistors_primitive(t_pb_type * pb_type) {
	double transistor_cnt;

	if (strcmp(pb_type->blif_model, ".names") == 0) {
		/* LUT */
		transistor_cnt = power_count_transistors_LUT(pb_type->num_input_pins,
				g_power_arch->LUT_transistor_size);
	} else if (strcmp(pb_type->blif_model, ".latch") == 0) {
		/* Latch */
		transistor_cnt = power_count_transistors_FF(g_power_arch->FF_size);
	} else {
		/* Other */
		char msg[BUFSIZE];

		sprintf(msg, "No transistor counter function for BLIF model: %s",
				pb_type->blif_model);
		power_log_msg(POWER_LOG_WARNING, msg);
		transistor_cnt = 0;
	}

	return transistor_cnt;
}

/**
 * Returns the transistor count of an SRAM cell
 */
static double power_count_transistor_SRAM_bit(void) {
	return g_power_arch->transistors_per_SRAM_bit;
}

static double power_count_transistors_levr() {
	double transistor_cnt = 0.;

	/* Each level restorer has a P/N=1/2 inverter and a W/L=1/2 PMOS */

	/* Inverter */
	transistor_cnt += power_MTAs(2); // NMOS
	transistor_cnt += 1.0; // PMOS

	/* Pull-up */
	transistor_cnt += power_MTAs_L(2.0); // Double length

	return transistor_cnt;
}

/**
 * Returns the transistor count for a LUT
 */
static double power_count_transistors_LUT(int LUT_inputs,
		float transistor_size) {
	double transistor_cnt = 0.;
	int level_idx;

	/* Each input driver has 1-1X and 2-2X inverters */
	transistor_cnt += (double) LUT_inputs
			* (power_count_transistors_inv(1.0)
					+ 2 * power_count_transistors_inv(2.0));

	/* SRAM bits */
	transistor_cnt += power_count_transistor_SRAM_bit() * (1 << LUT_inputs);

	for (level_idx = 0; level_idx < LUT_inputs; level_idx++) {

		/* Pass transistors */
		transistor_cnt += (1 << (LUT_inputs - level_idx))
				* power_MTAs(transistor_size);

		/* Add level restorer after every 2 stages (level_idx %2 == 1)
		 * But if there is an odd # of stages, just put one at the last
		 * stage (level_idx == LUT_size - 1) and not at the stage just before
		 * the last stage (level_idx != LUT_size - 2)
		 */
		if (((level_idx % 2 == 1) && (level_idx != LUT_inputs - 2))
				|| (level_idx == LUT_inputs - 1)) {
			transistor_cnt += power_count_transistors_levr();
		}
	}

	return transistor_cnt;
}

static double power_count_transistors_trans_gate(float size) {
	double transistor_cnt = 0.;

	transistor_cnt += power_MTAs(size);
	transistor_cnt += power_MTAs(size * g_power_tech->PN_ratio);

	return transistor_cnt;
}

static double power_count_transistors_inv(float size) {
	double transistor_cnt = 0.;

	/* NMOS */
	transistor_cnt += power_MTAs(size);

	/* PMOS */
	transistor_cnt += power_MTAs(g_power_tech->PN_ratio * size);

	return transistor_cnt;
}

/**
 * Returns the transistor count for a flip-flop
 */
static double power_count_transistors_FF(float size) {
	double transistor_cnt = 0.;

	/* 4 1X Inverters */
	transistor_cnt += 4 * power_count_transistors_inv(size);

	/* 2 Muxes = 4 transmission gates */
	transistor_cnt += 4 * power_count_transistors_trans_gate(size);

	return transistor_cnt;
}

double power_transistor_area(double num_MTAs) {
	return num_MTAs * g_MTA_area;
}

static double power_MTAs(float W_size) {
	return 1 + (W_size - 1) * (POWER_DRC_MIN_W / POWER_MTA_W);
}

static double power_MTAs_L(float L_size) {
	return 1 + (L_size - 1) * (POWER_DRC_MIN_L / POWER_MTA_L);
}

static void power_size_pb(void) {
	int type_idx;

	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_graph_head) {
			power_size_pb_rec(type_descriptors[type_idx].pb_graph_head);
		}
	}
}

static void power_size_pb_rec(t_pb_graph_node * pb_node) {
	int port_idx, pin_idx;
	int mode_idx, type_idx, pb_idx;
	boolean size_buffers_and_wires = TRUE;

	if (!power_method_is_transistor_level(
			pb_node->pb_type->pb_type_power->estimation_method)
			&& pb_node != FILL_TYPE->pb_graph_head) {
		/* Area information is only needed for:
		 *  1. Transistor-level estimation methods
		 *  2. the FILL_TYPE for tile size calculations
		 */
		return;
	}

	/* Recursive call for all child pb nodes */
	for (mode_idx = 0; mode_idx < pb_node->pb_type->num_modes; mode_idx++) {
		t_mode * mode = &pb_node->pb_type->modes[mode_idx];

		for (type_idx = 0; type_idx < mode->num_pb_type_children; type_idx++) {
			int num_pb = mode->pb_type_children[type_idx].num_pb;

			for (pb_idx = 0; pb_idx < num_pb; pb_idx++) {

				power_size_pb_rec(
						&pb_node->child_pb_graph_nodes[mode_idx][type_idx][pb_idx]);
			}
		}
	}

	/* Determine # of transistors for this node */
	power_count_transistors_pb_node(pb_node);

	if (pb_node->pb_type->class_type == LUT_CLASS) {
		/* LUTs will have a child node that is used for routing purposes
		 * For the routing algorithms it is completely connected; however,
		 * this interconnect does not exist in FPGA hardware and should
		 * be ignored for power calculations. */
		size_buffers_and_wires = FALSE;
	}

	if (!power_method_is_transistor_level(
			pb_node->pb_type->pb_type_power->estimation_method)) {
		size_buffers_and_wires = FALSE;
	}

	/* Size all local buffers and wires */
	if (size_buffers_and_wires) {
		for (port_idx = 0; port_idx < pb_node->num_input_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_input_pins[port_idx];
					pin_idx++) {
				power_size_pin_buffers_and_wires(
						&pb_node->input_pins[port_idx][pin_idx], TRUE);
			}
		}

		for (port_idx = 0; port_idx < pb_node->num_output_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_output_pins[port_idx];
					pin_idx++) {
				power_size_pin_buffers_and_wires(
						&pb_node->output_pins[port_idx][pin_idx], FALSE);
			}
		}

		for (port_idx = 0; port_idx < pb_node->num_clock_ports; port_idx++) {
			for (pin_idx = 0; pin_idx < pb_node->num_clock_pins[port_idx];
					pin_idx++) {
				power_size_pin_buffers_and_wires(
						&pb_node->clock_pins[port_idx][pin_idx], TRUE);
			}
		}
	}

}

/* Provides statistics about the connection between the pin and interconnect */
static void power_size_pin_to_interconnect(t_interconnect * interc,
		int * fanout, float * wirelength) {

	float this_interc_sidelength;

	/* Pin to interconnect wirelength */
	switch (interc->type) {
	case DIRECT_INTERC:
		*wirelength = 0;
		*fanout = 1;
		break;
	case MUX_INTERC:

	case COMPLETE_INTERC:
		/* The sidelength of this crossbar */
		this_interc_sidelength = sqrt(
				power_transistor_area(
						interc->interconnect_power->transistor_cnt));

		/* Assume that inputs to the crossbar have a structure like this:
		 *
		 * A   B|-----
		 * -----|---C-
		 *      |-----
		 *
		 * A - wire from pin to point of fanout (grows pb interconnect area)
		 * B - fanout wire (sidelength of this crossbar)
		 * C - fanouts to crossbar muxes (grows with pb interconnect area)
		 */

		*fanout = interc->interconnect_power->num_output_ports;
		*wirelength = this_interc_sidelength;
		//*wirelength = ((1 + *fanout) / 2.0) * g_power_arch->local_interc_factor
		//		* pb_interc_sidelength + this_interc_sidelength;
		break;
	default:
		assert(0);
		break;
	}

}

static void power_size_pin_buffers_and_wires(t_pb_graph_pin * pin,
		boolean pin_is_an_input) {
	int edge_idx;
	int list_cnt;
	t_interconnect ** list;
	boolean found;
	int i;

	float C_load;
	float this_pb_length;

	int fanout;
	float wirelength_out = 0;
	float wirelength_in = 0;

	int fanout_tmp;
	float wirelength_tmp;

	float this_pb_interc_sidelength = 0;
	float parent_pb_interc_sidelength = 0;
	boolean top_level_pb;

	t_pb_type * this_pb_type = pin->parent_node->pb_type;

	/*
	 if (strcmp(pin->parent_node->pb_type->name, "clb") == 0) {
	 //printf("here\n");
	 }*/

	this_pb_interc_sidelength = sqrt(
			power_transistor_area(
					pin->parent_node->pb_node_power->transistor_cnt_interc));
	if (pin->parent_node->parent_pb_graph_node == NULL) {
		top_level_pb = TRUE;
		parent_pb_interc_sidelength = 0.;
	} else {
		top_level_pb = FALSE;
		parent_pb_interc_sidelength =
				sqrt(
						power_transistor_area(
								pin->parent_node->parent_pb_graph_node->pb_node_power->transistor_cnt_interc));
	}

	/* Pins are connected to a wire that is connected to:
	 * 1. Interconnect structures belonging its pb_node, and/or
	 * 		- The pb_node may have one or more modes, each with a set of
	 * 		interconnect.  The capacitance is the worst-case capacitance
	 * 		between the different modes.
	 *
	 * 2. Interconnect structures belonging to its parent pb_node
	 */

	/* We want to estimate the physical pin fan-out, unfortunately it is not defined in the architecture file.
	 * Instead, we know the modes of operation, and the fanout may differ depending on the mode.  We assume
	 * that the physical fanout is the max of the connections to the various modes (although in reality it could
	 * be higher)*/

	/* Loop through all edges, building a list of interconnect that this pin drives */
	list = NULL;
	list_cnt = 0;
	for (edge_idx = 0; edge_idx < pin->num_output_edges; edge_idx++) {
		/* Check if its already in the list */
		found = FALSE;
		for (i = 0; i < list_cnt; i++) {
			if (list[i] == pin->output_edges[edge_idx]->interconnect) {
				found = TRUE;
				break;
			}
		}

		if (!found) {
			list_cnt++;
			list = (t_interconnect**) my_realloc(list,
					list_cnt * sizeof(t_interconnect*));
			list[list_cnt - 1] = pin->output_edges[edge_idx]->interconnect;
		}
	}

	/* Determine the:
	 * 1. Wirelength connected to the pin
	 * 2. Fanout of the pin
	 */
	if (pin_is_an_input) {
		/* Pin is an input to the PB.
		 * Thus, all interconnect it drives belong to the modes of the PB.
		 */
		int * fanout_per_mode;
		float * wirelength_out_per_mode;

		fanout_per_mode = (int*) my_calloc(this_pb_type->num_modes,
				sizeof(int));
		wirelength_out_per_mode = (float *) my_calloc(this_pb_type->num_modes,
				sizeof(float));

		for (i = 0; i < list_cnt; i++) {
			int mode_idx = list[i]->parent_mode_index;

			power_size_pin_to_interconnect(list[i], &fanout_tmp,
					&wirelength_tmp);

			fanout_per_mode[mode_idx] += fanout_tmp;
			wirelength_out_per_mode[mode_idx] += wirelength_tmp;
		}

		fanout = 0;
		wirelength_out = 0.;

		/* Find worst-case between modes*/
		for (i = 0; i < this_pb_type->num_modes; i++) {
			fanout = max(fanout, fanout_per_mode[i]);
			wirelength_out = max(wirelength_out, wirelength_out_per_mode[i]);
		}
		if (wirelength_out != 0) {
			wirelength_out += g_power_arch->local_interc_factor
					* this_pb_interc_sidelength;
		}

		free(fanout_per_mode);
		free(wirelength_out_per_mode);

		/* Input wirelength - from parent PB */
		if (!top_level_pb) {
			wirelength_in = g_power_arch->local_interc_factor
					* parent_pb_interc_sidelength;
		}

	} else {
		/* Pin is an output of the PB.
		 * Thus, all interconnect it drives belong to the parent PB.
		 */
		fanout = 0;
		wirelength_out = 0.;

		if (top_level_pb) {
			/* Outputs of top-level pb should not drive interconnect */
			assert(list_cnt == 0);
		}

		/* Loop through all interconnect that this pin drives */

		for (i = 0; i < list_cnt; i++) {
			power_size_pin_to_interconnect(list[i], &fanout_tmp,
					&wirelength_tmp);
			fanout += fanout_tmp;
			wirelength_out += wirelength_tmp;
		}
		if (wirelength_out != 0) {
			wirelength_out += g_power_arch->local_interc_factor
					* parent_pb_interc_sidelength;
		}

		/* Input wirelength - from this PB */
		wirelength_in = g_power_arch->local_interc_factor
				* this_pb_interc_sidelength;

	}
	free(list);

	/* Wirelength */
	switch (pin->port->port_power->wire_type) {
	case POWER_WIRE_TYPE_IGNORED:
		/* User is ignoring this wirelength */
		pin->pin_power->C_wire = 0.;
		break;
	case POWER_WIRE_TYPE_C:
		pin->pin_power->C_wire = pin->port->port_power->wire.C;
		break;
	case POWER_WIRE_TYPE_ABSOLUTE_LENGTH:
		pin->pin_power->C_wire = pin->port->port_power->wire.absolute_length
				* g_power_arch->C_wire_local;
		break;
	case POWER_WIRE_TYPE_RELATIVE_LENGTH:
		this_pb_length = sqrt(
				power_transistor_area(
						power_transistors_for_pb_node(pin->parent_node)));
		pin->pin_power->C_wire = pin->port->port_power->wire.relative_length
				* this_pb_length * g_power_arch->C_wire_local;
		break;

	case POWER_WIRE_TYPE_AUTO:
		pin->pin_power->C_wire += g_power_arch->C_wire_local
				* (wirelength_in + wirelength_out);
		break;
	case POWER_WIRE_TYPE_UNDEFINED:
	default:
		assert(0);
		break;
	}

	/* Buffer */
	switch (pin->port->port_power->buffer_type) {
	case POWER_BUFFER_TYPE_NONE:
		/* User assumes no buffer */
		pin->pin_power->buffer_size = 0.;
		break;
	case POWER_BUFFER_TYPE_ABSOLUTE_SIZE:
		pin->pin_power->buffer_size = pin->port->port_power->buffer_size;
		break;
	case POWER_BUFFER_TYPE_AUTO:
		/* Asume the buffer drives the wire & fanout muxes */
		C_load = pin->pin_power->C_wire
				+ (fanout) * g_power_commonly_used->INV_1X_C_in; //g_power_commonly_used->NMOS_1X_C_d;
		if (C_load > g_power_commonly_used->INV_1X_C_in) {
			pin->pin_power->buffer_size = power_buffer_size_from_logical_effort(
					C_load);
		} else {
			pin->pin_power->buffer_size = 0.;
		}
		break;
	case POWER_BUFFER_TYPE_UNDEFINED:
	default:
		assert(0);
	}

	pin->parent_node->pb_node_power->transistor_cnt_buffers +=
			power_count_transistors_buffer(pin->pin_power->buffer_size);
}
