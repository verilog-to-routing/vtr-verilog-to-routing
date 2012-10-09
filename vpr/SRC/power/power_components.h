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
 *  This file offers functions to estimate power of major components
 * within the FPGA (flip-flops, LUTs, interconnect structures, etc).
 */

#ifndef __POWER_COMPONENTS_H__
#define __POWER_COMPONENTS_H__

/************************* INCLUDES *********************************/
#include "power.h"

/************************* Defines **********************************/

/* This controlls the level of accuracy used for transition density
 * calculations of internal LUT nodes.  The more detailed model
 * looks at SRAM values in deciding if toggles on the inputs will
 * toggle the internal nets.  The fast method uses only signal
 * probabilities.
 */
#define POWER_LUT_SLOW
#if (!(defined(POWER_LUT_SLOW) || defined(POWER_LUT_FAST)))
#define POWER_LUT_SLOW
#endif

/************************* ENUMS ************************************/
typedef enum {
	POWER_COMPONENT_IGNORE = 0, /* */
	POWER_COMPONENT_TOTAL, /* Total power for entire FPGA */

	POWER_COMPONENT_ROUTING, /* Power for routing fabric (not local routing) */
	POWER_COMPONENT_ROUTE_SB, /* Switch-box */
	POWER_COMPONENT_ROUTE_CB, /* Connection box*/
	POWER_COMPONENT_ROUTE_GLB_WIRE, /* Wires */

	POWER_COMPONENT_CLOCK, /* Clock network */
	POWER_COMPONENT_CLOCK_BUFFER, /* Buffers in clock network */
	POWER_COMPONENT_CLOCK_WIRE, /* Wires in clock network */

	POWER_COMPONENT_CLB, /* Logic Blocks, and other hard blocks */
	POWER_COMPONENT_LOCAL_INTERC, /* Local interconnect structures */
	POWER_COMPONENT_LOCAL_WIRE, /* Local interconnect wires */
	POWER_COMPONENT_FF, /* Flip-flops */
	POWER_COMPONENT_LUT, /* LUTs */
	POWER_COMPONENT_LUT_DRIVER, /* LUT input drivers */
	POWER_COMPONENT_LUT_MUX, /* All 2-muxs in the LUTs */
	POWER_COMPONENT_LUT_BUFFERS, /* All level-restoring buffers in LUTs */

	POWER_COMPONENT_MAX_NUM
} e_power_component_type;

/************************* FUNCTION DECLARATIONS ********************/

void power_components_init(void);
void power_components_uninit(void);
void power_component_get_usage(t_power_usage * power_usage,
		e_power_component_type component_idx);
void power_component_add_usage(t_power_usage * power_usage,
		e_power_component_type component_idx);
float power_component_get_usage_sum(e_power_component_type component_idx);

void power_component_print_usage(FILE * fp);

void power_calc_FF(t_power_usage * power_usage, float D_prob, float D_dens,
		float Q_prob, float Q_dens, float clk_prob, float clk_dens);
void power_calc_LUT(t_power_usage * power_usage, int LUT_size,
		char * SRAM_values, float * input_densities,
		float * input_probabilities);
void power_calc_interconnect(t_power_usage * power_usage, t_pb * pb,
		t_interconnect_pins * interc_pins, float interc_length);
void power_calc_mux_multilevel(t_power_usage * power_usage,
		t_mux_arch * mux_arch, float * in_prob, float * in_dens,
		int selected_input, boolean output_level_restored);
void power_calc_buffer(t_power_usage * power_usage, float size, float in_prob,
		float in_dens, boolean level_restored, int input_mux_size);

#endif
