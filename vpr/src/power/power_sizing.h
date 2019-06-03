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

#ifndef __POWER_TRANSISTOR_CNT_H__
#define __POWER_TRANSISTOR_CNT_H__

/************************* INCLUDES *********************************/
#include "physical_types.h"

/************************* DEFINES **********************************/

/* Design rules, values in LAMBDA, where tech size = 2 LAMBDA */
#define POWER_DRC_MIN_L 2.0
#define POWER_DRC_MIN_W 5.0
#define POWER_DRC_MIN_DIFF_L 5.5
#define POWER_DRC_SPACING 3.0
#define POWER_DRC_POLY_OVERHANG 2.5

#define POWER_MTA_W (POWER_DRC_MIN_W + POWER_DRC_POLY_OVERHANG + POWER_DRC_SPACING)
#define POWER_MTA_L (POWER_DRC_MIN_L + 2 * POWER_DRC_MIN_DIFF_L + POWER_DRC_SPACING)

/************************* FUNCTION DECLARATION *********************/
void power_sizing_init(const t_arch* arch);
double power_count_transistors_buffer(float buffer_size);
double power_transistor_area(double num_transistors);
#endif
