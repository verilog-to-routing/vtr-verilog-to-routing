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

/* This file provides functions used to verify the power estimations
 * againt SPICE.
 */

#ifndef __POWER_MISC_H__
#define __POWER_MISC_H__

/************************* INCLUDES *********************************/
#include "power.h"

/************************* DEFINES **********************************/
const float power_callib_period = 5e-9;

/************************* STRUCTS **********************************/
/************************* ENUMS ************************************/
typedef enum {
	POWER_CALLIB_COMPONENT_BUFFER = 0,
	POWER_CALLIB_COMPONENT_BUFFER_WITH_LEVR,
	POWER_CALLIB_COMPONENT_FF,
	POWER_CALLIB_COMPONENT_MUX,
	POWER_CALLIB_COMPONENT_LUT,
	POWER_CALLIB_COMPONENT_MAX
} e_power_callib_component;

/************************* FUNCTION DECLARATIONS ********************/
void power_print_spice_comparison(void);
void power_callibrate(void);
float power_usage_buf_for_callibration(int num_inputs, float transistor_size);
float power_usage_buf_levr_for_callibration(int num_inputs,
		float transistor_size);
float power_usage_mux_for_callibration(int num_inputs, float transistor_size);
float power_usage_lut_for_callibration(int num_inputs, float transistor_size);
float power_usage_ff_for_callibration(int num_inputs, float transistor_size);
void power_print_callibration(void);
#endif
