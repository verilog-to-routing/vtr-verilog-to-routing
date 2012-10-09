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
#include <assert.h>
#include <string.h>

#include "power_transistor_cnt.h"
#include "power.h"
#include "globals.h"
#include "power_util.h"
/************************* GLOBALS **********************************/

/************************* FUNCTION DELCARATIONS ********************/
static float power_count_transistors_connectionbox(void);
static float power_count_transistors_mux(t_mux_arch * mux_arch);
static float power_count_transistors_mux_node(t_mux_node * mux_node,
		float * transistor_sizes);
static void power_mux_node_max_inputs(t_mux_node * mux_node, float * max_inputs);
static float power_count_transistors_interconnect(t_interconnect * interc);
static float power_count_transistors_pb_type(t_pb_type * pb_type);
static float power_count_transistors_switchbox(t_arch * arch);
static float power_count_transistors_blif_primitive(t_pb_type * pb_type);
static float power_count_transistors_LUT(int LUT_size);
static float power_count_transistors_FF(void);
static float power_cnt_transistor_SRAM_bit(void);

/************************* FUNCTION DEFINITIONS *********************/

/**
 *  Counts the number of transistors in the largest connection box
 */
static float power_count_transistors_connectionbox(void) {
	float transistor_cnt = 0.;
	int CLB_inputs;
	float buffer_size;

	assert(FILL_TYPE->pb_graph_head->num_input_ports == 1);
	CLB_inputs = FILL_TYPE->pb_graph_head->num_input_pins[0];

	/* Buffers from Tracks */
	buffer_size =
			g_power_commonly_used->max_seg_to_IPIN_fanout
					* (g_power_commonly_used->NMOS_1X_C_d
							/ g_power_commonly_used->INV_1X_C_in)/ POWER_BUFFER_STAGE_GAIN;
	buffer_size = max(1, buffer_size);
	transistor_cnt += g_solution_inf.channel_width
			* power_count_transistors_buffer(buffer_size);

	/* Muxes to IPINs */
	transistor_cnt +=
			CLB_inputs
					* power_count_transistors_mux(
							power_get_mux_arch(g_power_commonly_used->max_IPIN_fanin));

	return transistor_cnt;
}

/**
 * Counts the number of transistors in a buffer.
 * - buffer_size: The final stage size of the buffer, relative to a minimum sized inverter
 */
float power_count_transistors_buffer(float buffer_size) {
	int stages;
	float effort;
	float stage_size;
	int stage_idx;
	float transistor_cnt = 0.;

	stages = calc_buffer_num_stages(buffer_size, POWER_BUFFER_STAGE_GAIN);
	effort = calc_buffer_stage_effort(stages, buffer_size);

	stage_size = 1;
	for (stage_idx = 0; stage_idx < stages; stage_idx++) {
		transistor_cnt += stage_size * (1 + g_power_tech->PN_ratio);
		stage_size *= effort;
	}

	return transistor_cnt;
}

/**
 * Counts the number of transistors in a multiplexer
 * - mux_arch: The architecture of the multiplexer
 */
static float power_count_transistors_mux(t_mux_arch * mux_arch) {
	float transistor_cnt = 0.;
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
		transistor_cnt += ceil(log2((float) max_inputs[lvl_idx]))
				* power_cnt_transistor_SRAM_bit();

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
			mux_arch->transistor_sizes);

	return transistor_cnt;
}

/**
 *  This function is used recursively to determine the largest single-level
 *  multiplexer at each level of a multi-level multiplexer
 */
static void power_mux_node_max_inputs(t_mux_node * mux_node, float * max_inputs) {

	max_inputs[mux_node->level] =
			max(max_inputs[mux_node->level], mux_node->num_inputs);

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
static float power_count_transistors_mux_node(t_mux_node * mux_node,
		float * transistor_sizes) {
	int input_idx;
	float transistor_cnt = 0.;

	if (mux_node->num_inputs != 1) {
		for (input_idx = 0; input_idx < mux_node->num_inputs; input_idx++) {
			/* Single Pass transistor */
			transistor_cnt += transistor_sizes[mux_node->level];

			/* Child MUX */
			if (mux_node->level != 0) {
				transistor_cnt += power_count_transistors_mux_node(
						&mux_node->children[input_idx], transistor_sizes);
			}
		}
	}

	return transistor_cnt;
}

/**
 * This function returns the number of transistors in an interconnect structure
 */
static float power_count_transistors_interconnect(t_interconnect * interc) {
	float transistor_cnt = 0.;

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
		transistor_cnt +=
				interc->num_output_ports * interc->num_pins_per_port
						* power_count_transistors_mux(
								power_get_mux_arch(interc->num_input_ports));
		break;
	default:
		assert(0);
	}

	return transistor_cnt;
}

/**
 * This functions counts the number of transistors in all structures in the FPGA.
 * It returns the number of transistors in a grid of the FPGA (logic block,
 * switch box, 2 connection boxes)
 */
float power_count_transistors(t_arch * arch) {
	int type_idx;
	float transistor_cnt = 0.;

	for (type_idx = 0; type_idx < num_types; type_idx++) {
		if (type_descriptors[type_idx].pb_type) {
			power_count_transistors_pb_type(type_descriptors[type_idx].pb_type);
		}
	}

	transistor_cnt += FILL_TYPE->pb_type->transistor_cnt;

	transistor_cnt += 2 * power_count_transistors_switchbox(arch);

	transistor_cnt += 2 * power_count_transistors_connectionbox();

	return transistor_cnt;
}

/**
 * This function counts the number of transistors for a given physical block type
 */
static float power_count_transistors_pb_type(t_pb_type * pb_type) {
	int mode_idx;
	float transistor_cnt_max = 0;
	float transistor_cnt_interc = 0;
	boolean ignore_interc = FALSE;

	/* Check if this is a leaf node, or whether it has children */
	if (pb_type->num_modes == 0) {
		/* Leaf node */
		transistor_cnt_interc = 0;
		transistor_cnt_max = power_count_transistors_blif_primitive(pb_type);
	} else {
		/* Find max transistor count between all modes */
		for (mode_idx = 0; mode_idx < pb_type->num_modes; mode_idx++) {
			int interc_idx;
			int child_type_idx;
			float transistor_cnt_mode;
			float transistor_cnt_children = 0;
			float transistor_cnt_mode_interc = 0;

			if (pb_type->class_type == LUT_CLASS) {
				/* LUTs will have a child node that is used for routing purposes
				 * For the routing algorithms it is completely connected; however,
				 * this interconnect does not exist in FPGA hardware and should
				 * be ignored for power calculations. */
				ignore_interc = TRUE;
			}

			/* Count Interconnect Transistors */
			if (!ignore_interc) {
				for (interc_idx = 0;
						interc_idx < pb_type->modes[mode_idx].num_interconnect;
						interc_idx++) {
					transistor_cnt_mode_interc +=
							power_count_transistors_interconnect(
									&pb_type->modes[mode_idx].interconnect[interc_idx]);
				}
			}

			/* Count Child Transistors */
			for (child_type_idx = 0;
					child_type_idx
							< pb_type->modes[mode_idx].num_pb_type_children;
					child_type_idx++) {
				t_pb_type * child_type =
						&pb_type->modes[mode_idx].pb_type_children[child_type_idx];
				transistor_cnt_children += child_type->num_pb
						* power_count_transistors_pb_type(child_type);
			}

			transistor_cnt_mode = (transistor_cnt_mode_interc
					+ transistor_cnt_children);

			if (transistor_cnt_mode > transistor_cnt_max) {
				transistor_cnt_max = transistor_cnt_mode;
				transistor_cnt_interc = transistor_cnt_mode_interc;
			}
		}
	}

	pb_type->transistor_cnt = transistor_cnt_max;
	pb_type->transistor_cnt_interc = transistor_cnt_interc;

	return transistor_cnt_max;
}

/**
 * This function counts the maximum number of transistors in a switch box
 */
static float power_count_transistors_switchbox(t_arch * arch) {
	float transistor_cnt = 0.;
	float transistors_per_buf_mux = 0.;
	int seg_idx;

	/* Buffer */
	transistors_per_buf_mux += power_count_transistors_buffer(
			(float) g_power_commonly_used->max_seg_fanout
					/ POWER_BUFFER_STAGE_GAIN);

	/* Multiplexor */
	transistors_per_buf_mux +=
			power_count_transistors_mux(
					power_get_mux_arch(g_power_commonly_used->max_routing_mux_size));

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
static float power_count_transistors_blif_primitive(t_pb_type * pb_type) {
	float transistor_cnt;

	if (strcmp(pb_type->blif_model, ".names") == 0) {
		/* LUT */
		transistor_cnt = power_count_transistors_LUT(pb_type->num_input_pins);
	} else if (strcmp(pb_type->blif_model, ".latch") == 0) {
		/* Latch */
		transistor_cnt = power_count_transistors_FF();
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
static float power_cnt_transistor_SRAM_bit(void) {
	/* The SRAM cell consists of:
	 *  - 2 weak PMOS for pullup (size 1)
	 *  - 2 strong NMOS for pulldown (size 4)
	 *  - 2 moderate NMOS for access (size 2)
	 *
	 *  This information is from Weste/Harris "CMOS VLSI Design"
	 */

	return (1 + 1 + 4 + 4 + 2 + 2);
}

/**
 * Returns the transistor count for a LUT
 */
static float power_count_transistors_LUT(int LUT_size) {
	float transistor_cnt;
	int level_idx;

	/* Each input driver has 1-1X and 2-2X inverters */
	transistor_cnt = (float) LUT_size
			* ((1 + g_power_tech->PN_ratio)
					+ 2 * (2 + 2 * g_power_tech->PN_ratio));
	/*transistor_cnt = (float) LUT_size
	 * ((1 + g_power_arch->PN_ratio)
	 + 2 * (2 + 2 * g_power_arch->P_to_N_size_ratio));*/

	/* SRAM bits */
	transistor_cnt += power_cnt_transistor_SRAM_bit() * pow(2, LUT_size);

	for (level_idx = 0; level_idx < LUT_size; level_idx++) {

		/* Pass transistors */
		transistor_cnt += pow(2, LUT_size - level_idx);

		/* Add level restorer after every 2 stages (level_idx %2 == 1)
		 * But if there is an odd # of stages, just put one at the last
		 * stage (level_idx == LUT_size - 1) and not at the stage just before
		 * the last stage (level_idx != LUT_size - 2)
		 */
		if (((level_idx % 2 == 1) && (level_idx != LUT_size - 2))
				|| (level_idx == LUT_size - 1)) {
			/* Each level restorer has a P/N=1/2 inverter and a W/L=1/2 PMOS */
			transistor_cnt += pow(2, LUT_size - level_idx - 1) * (3 + 2);
		}
	}

	return transistor_cnt;
}

/**
 * Returns the transistor count for a flip-flop
 */
static float power_count_transistors_FF(void) {
	float transistor_cnt = 0.;

	/* 4 1X Inverters */
	transistor_cnt += 4 * (1. + g_power_tech->PN_ratio);

	/* 2 Muxes = 4 transmission gates */
	transistor_cnt += 4 * (1. + g_power_tech->PN_ratio);

	return transistor_cnt;
}

float power_transistor_area(float num_transistors) {
	/* Calculate transistor area, assuming:
	 *  - Min transistor size is Wx6L
	 *  - Overhead to space transistors is: POWER_TRANSISTOR_SPACING_FACTOR */

	return num_transistors * POWER_TRANSISTOR_SPACING_FACTOR
			* (g_power_tech->tech_size * g_power_tech->tech_size * 6);
}
