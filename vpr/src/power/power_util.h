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
 * This file provides utility functions used by power estimation.
 */

#ifndef __POWER_UTIL_H__
#define __POWER_UTIL_H__

/************************* INCLUDES *********************************/
#include "power.h"
#include "power_components.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"

/************************* FUNCTION DECLARATIONS ********************/

/* Pins */
float pin_dens(t_pb* pb, t_pb_graph_pin* pin, ClusterBlockId iblk);
float pin_prob(t_pb* pb, t_pb_graph_pin* pin, ClusterBlockId iblk);
int power_calc_pin_fanout(t_pb_graph_pin* pin, int mode_idx);
void pb_foreach_pin(t_pb_graph_node* pb_node,
                    void (*fn)(t_pb_graph_pin*, void*),
                    void* context);

/* Power Usage */
void power_zero_usage(t_power_usage* power_usage);
void power_add_usage(t_power_usage* dest, const t_power_usage* src);
void power_scale_usage(t_power_usage* power_usage, float scale_factor);
float power_sum_usage(t_power_usage* power_usage);
float power_perc_dynamic(t_power_usage* power_usage);

/* Message Logger */
void power_log_msg(e_power_log_type log_type, const char* msg);

/* Buffers */
int power_calc_buffer_num_stages(float final_stage_size, float desired_stage_effort);
float calc_buffer_stage_effort(int N, float final_stage_size);
float power_buffer_size_from_logical_effort(float C_load);

/* Multiplexers */
bool mux_find_selector_values(int* selector_values, t_mux_node* mux_node, int selected_input_pin);
t_mux_arch* power_get_mux_arch(int num_mux_inputs, float transistor_size);
void mux_arch_fix_levels(t_mux_arch* mux_arch);

/* Power Methods */
bool power_method_is_transistor_level(e_power_estimation_method estimation_method);
bool power_method_is_recursive(e_power_estimation_method method);

const char* transistor_type_name(e_tx_type type);
char* alloc_SRAM_values_from_truth_table(int LUT_size,
                                         const AtomNetlist::TruthTable& truth_table);
float clb_net_density(ClusterNetId net_idx);
const char* interconnect_type_name(enum e_interconnect type);
float clb_net_prob(ClusterNetId net_idx);

void output_log(t_log* log_ptr, FILE* fp);

void output_logs(FILE* fp, t_log* logs, int num_logs);

void power_print_title(FILE* fp, const char* title);

#endif
