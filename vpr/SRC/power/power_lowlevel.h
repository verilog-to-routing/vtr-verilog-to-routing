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
 * This file provides functions that calculate the power of low-level
 * components (inverters, simple multiplexers, etc)
 */

#ifndef __POWER_LOW_LEVEL_H__
#define __POWER_LOW_LEVEL_H__

/************************* INCLUDES *********************************/
#include "power.h"

/************************* GLOBALS **********************************/

/************************* FUNCTION DECLARATION *********************/
void power_lowlevel_init();

void power_usage_inverter(t_power_usage * power_usage, float in_dens,
		float in_prob, float size, float period);
/*void power_calc_inverter_with_input(t_power_usage * power_usage,
 float * input_dynamic_power, float in_density, float in_prob,
 float size);*/
void power_usage_inverter_irregular(t_power_usage * power_usage,
		float * dyn_power_input, float in_density, float in_probability,
		float PMOS_size, float NMOS_size, float period);

void power_usage_wire(t_power_usage * power_usage, float capacitance,
		float density, float period);

void power_usage_mux_singlelevel_static(t_power_usage * power_usage,
		float * out_prob, float * out_dens, float * V_out, int num_inputs,
		int selected_idx, float * in_prob, float * in_dens, float * v_in,
		float transistor_size, boolean v_out_restored, float period);

void power_usage_MUX2_transmission(t_power_usage * power_usage, float size,
		float * in_dens, float * in_prob, float sel_dens, float out_dens,
		float period);

void power_usage_mux_singlelevel_dynamic(t_power_usage * power_usage,
		int num_inputs, float out_density, float v_out, float * in_prob,
		float * in_density, float * v_in, float sel_dens, float sel_prob,
		float transistor_size, float period);

void power_usage_level_restorer(t_power_usage * power_usage,
		float * dyn_power_in, float in_density, float in_probability,
		float period);

float power_calc_pb_switching_from_c_internal(t_pb * pb,
		t_pb_graph_node * pb_graph_node);

float power_calc_mux_v_out(int num_inputs, float transistor_size, float v_in,
		float in_prob_avg);

/*float power_calc_buffer_sc(int stages, float gain, boolean level_restored,
 int input_mux_size);*/

float power_calc_node_switching(float capacitance, float density, float period);

float power_calc_buffer_size_from_Cout(float C_out);

#endif
