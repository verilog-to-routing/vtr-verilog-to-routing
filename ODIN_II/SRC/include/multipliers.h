/*
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
#ifndef MULTIPLIERS_H
#define MULTIPLIERS_H

#include "read_xml_arch_file.h"

struct t_multiplier {
    int size_a;
    int size_b;
    int size_out;
    struct t_multiplier* next;
};

enum class mult_port_stat_e {
    NOT_CONSTANT,         // neither of ports are constant
    MULTIPLIER_CONSTANT,  // first input port is constant
    MULTIPICAND_CONSTANT, // second input port is constant
    CONSTANT,             // both input ports are constant
    mult_port_stat_END
};

extern t_model* hard_multipliers;
extern vtr::t_linked_vptr* mult_list;
extern int min_mult;

extern void init_mult_distribution();
extern void report_mult_distribution();
extern void declare_hard_multiplier(nnode_t* node);
extern void instantiate_hard_multiplier(nnode_t* node, short mark, netlist_t* netlist);
extern void instantiate_simple_soft_multiplier(nnode_t* node, short mark, netlist_t* netlist);
extern void connect_constant_mult_outputs(nnode_t* node, signal_list_t* output_signal_list);
extern void find_hard_multipliers();
extern void add_the_blackbox_for_mults(FILE* out);
extern void define_mult_function(nnode_t* node, FILE* out);
extern void split_multiplier(nnode_t* node, int a0, int b0, int a1, int b1, netlist_t* netlist);
extern void iterate_multipliers(netlist_t* netlist);
extern bool check_constant_multipication(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void check_multiplier_port_size(nnode_t* node);
extern bool is_ast_multiplier(ast_node_t* node);
extern void clean_multipliers();
extern void free_multipliers();

#endif // MULTIPLIERS_H
