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

	POWER_COMPONENT_PB, /* Logic Blocks, and other hard blocks */
	POWER_COMPONENT_PB_PRIMITIVES, /* Primitives (LUTs, FF, etc) */
	POWER_COMPONENT_PB_INTERC_MUXES, /* Local interconnect structures (muxes) */
	POWER_COMPONENT_PB_BUFS_WIRE, /* Local buffers and wire capacitance */

	POWER_COMPONENT_PB_OTHER, /* Power from other estimation methods - not transistor-level */

	POWER_COMPONENT_MAX_NUM
} e_power_component_type;

/************************* STRUCTS **********************************/
typedef struct s_power_breakdown t_power_components;

struct s_power_breakdown {
	t_power_usage * components;
};

/************************* FUNCTION DECLARATIONS ********************/
extern t_power_components g_power_by_component;

/************************* FUNCTION DECLARATIONS ********************/

void power_components_init(void);
void power_components_uninit(void);
void power_component_get_usage(t_power_usage * power_usage,
		e_power_component_type component_idx);
void power_component_add_usage(t_power_usage * power_usage,
		e_power_component_type component_idx);
float power_component_get_usage_sum(e_power_component_type component_idx);

void power_usage_ff(t_power_usage * power_usage, float size, float D_prob,
		float D_dens, float Q_prob, float Q_dens, float clk_prob,
		float clk_dens, float period);
void power_usage_lut(t_power_usage * power_usage, int LUT_size,
		float transistor_size, char * SRAM_values, float * input_densities,
		float * input_probabilities, float period);
void power_usage_local_interc_mux(t_power_usage * power_usage, t_pb * pb,
		t_interconnect_pins * interc_pins);
void power_usage_mux_multilevel(t_power_usage * power_usage,
		t_mux_arch * mux_arch, float * in_prob, float * in_dens,
		int selected_input, boolean output_level_restored, float period);
void power_usage_buffer(t_power_usage * power_usage, float size, float in_prob,
		float in_dens, boolean level_restored, float period);

#endif
