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
#define POWER_DR_MIN_L 2.0
#define POWER_DR_MIN_W 2.0
#define POWER_DR_MIN_WELL 12.0
#define POWER_DR_MIN_WELL_TO_WELL 6.0
#define POWER_DR_MIN_POLY_EXT 2.5
#define POWER_DR_MIN_POLY_TO_POLY 3.0

#define POWER_TRANSISTOR_AREA (POWER_DR_MIN_WELL * POWER_DR_MIN_W)
#define POWER_TRANSISTOR_LAYOUT_AREA ((POWER_DR_MIN_WELL + POWER_DR_MIN_WELL_TO_WELL) * (POWER_DR_MIN_W+POWER_DR_MIN_POLY_EXT+POWER_DR_MIN_POLY_TO_POLY))

#define POWER_TRANSISTOR_AREA_SPACING_FACTOR (POWER_TRANSISTOR_AREA / POWER_TRANSISTOR_LAYOUT_AREA)

/************************* FUNCTION DECLARATION *********************/
void power_size_pb(void);
double power_transistors_per_tile(t_arch * arch);
double power_count_transistors_buffer(float buffer_size);
double power_transistor_area(double num_transistors);
#endif
