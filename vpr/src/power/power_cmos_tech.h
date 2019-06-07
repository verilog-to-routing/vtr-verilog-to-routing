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
 * This file provides functions relating to the cmos technology.  It
 * includes functions to read the transistor characteristics from the
 * xml file into data structures, and functions to search within
 * these data structures.
 */

#ifndef __POWER_CMOS_TECH_H__
#define __POWER_CMOS_TECH_H__

/************************* INCLUDES *********************************/
#include "power.h"

/************************* FUNCTION DECLARATIONS ********************/
void power_tech_init(const char* cmos_tech_behavior_filepath);
bool power_find_transistor_info(t_transistor_size_inf** lower,
                                t_transistor_size_inf** upper,
                                e_tx_type type,
                                float size);
void power_find_mux_volt_inf(t_power_mux_volt_pair** lower,
                             t_power_mux_volt_pair** upper,
                             t_power_mux_volt_inf* volt_inf,
                             float v_in);
void power_find_nmos_leakage(t_power_nmos_leakage_inf* nmos_leakage_info,
                             t_power_nmos_leakage_pair** lower,
                             t_power_nmos_leakage_pair** upper,
                             float v_ds);
void power_find_buffer_strength_inf(t_power_buffer_strength_inf** lower,
                                    t_power_buffer_strength_inf** upper,
                                    t_power_buffer_size_inf* size_inf,
                                    float stage_gain);
void power_find_buffer_sc_levr(t_power_buffer_sc_levr_inf** lower,
                               t_power_buffer_sc_levr_inf** upper,
                               t_power_buffer_strength_inf* buffer_sc,
                               int input_mux_size);
#endif
