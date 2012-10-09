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

/************************* FUNCTION DECLARATION *********************/
float power_count_transistors(t_arch * arch);
float power_count_transistors_buffer(float buffer_size);
float power_transistor_area(float num_transistors);
#endif
