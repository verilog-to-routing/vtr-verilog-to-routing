/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NETLIST_STATISTIC_HPP
#define NETLIST_STATISTIC_HPP

#include "netlist_utils.h"

static const unsigned int traversal_id = 0;
static const uintptr_t mult_optimization_traverse_value = (uintptr_t)&traversal_id;
static const uintptr_t adder_optimization_traverse_value = (uintptr_t)&traversal_id;

stat_t* get_stats(signal_list_t* input_signals, signal_list_t* output_signals, netlist_t* netlist, uintptr_t traverse_mark_number);
stat_t* get_stats(nnode_t** node_list, long long node_count, netlist_t* netlist, uintptr_t traverse_mark_number);
stat_t* get_stats(nnet_t** net_list, long long net_count, netlist_t* netlist, uintptr_t traverse_mark_number);
stat_t* get_stats(nnode_t* node, netlist_t* netlist, uintptr_t traverse_mark_number);
stat_t* get_stats(nnet_t* net, netlist_t* netlist, uintptr_t traverse_mark_number);

stat_t* delete_stat(stat_t* stat);

void init_stat(netlist_t* netlist);
void compute_statistics(netlist_t* netlist, bool display);

/**
 * @brief This function will calculate and assign weights related
 * to mixing hard and soft logic implementation for certain kind
 * of logic blocks
 * @param node 
 * The node that needs its weight to be assigned
 * @param netlist 
 * netlist, has to be passed to the counting functions
 */
void mixing_optimization_stats(nnode_t* node, netlist_t* netlist);

#endif // NETLIST_STATISTIC_HPP
