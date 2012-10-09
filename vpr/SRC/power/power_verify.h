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

/* This controls whether the tools is run in verification mode.
 * IN VERIFICATION MODE, THE INPUT CIRCUIT WILL BE IGNORED.
 */
#define PRINT_SPICE_COMPARISON 0

/************************* FUNCTION DECLARATIONS ********************/
void power_print_spice_comparison(void);

#endif
