/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef ADDERS_H
#define ADDERS_H

#include "read_xml_arch_file.h"
#include "odin_types.h"
#include <vector>

typedef struct s_adder
{
	int size_a;
	int size_b;
	int size_cin;
	int size_sumout;
	int size_cout;
	struct s_adder *next;
} t_adder;

typedef struct {
	signal_list_t *a;
	signal_list_t *b;
	signal_list_t *cin;
	signal_list_t *cout;
	signal_list_t *sumout;
} adder_signals;

extern t_model *hard_adders;
extern vtr::t_linked_vptr *add_list;
extern vtr::t_linked_vptr *chain_list;
extern vtr::t_linked_vptr *processed_adder_list;
extern int total;
extern int min_add;
extern int min_threshold_adder;
extern std::vector<adder_def_t*> list_of_adder_def;

extern void init_add_distribution();
extern void report_add_distribution();
extern void declare_hard_adder(nnode_t *node);
extern void instantiate_hard_adder(nnode_t *node, short mark, netlist_t *netlist);
extern void instantiate_simple_soft_adder(nnode_t *node, short mark, netlist_t *netlist);
extern void find_hard_adders();
extern void add_the_blackbox_for_adds(FILE *out);
extern void define_add_function(nnode_t *node, FILE *out);
extern void split_adder(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist);
extern void iterate_adders(netlist_t *netlist);
extern void clean_adders();
extern void reduce_operations(netlist_t *netlist, operation_list op);
extern void traverse_list(operation_list oper, vtr::t_linked_vptr *place);
extern void match_node(vtr::t_linked_vptr *place, operation_list oper);
extern int match_ports(nnode_t *node, nnode_t *next_node, operation_list oper);
extern void traverse_operation_node(ast_node_t *node, char *component[], operation_list op, int *mark);
//extern void find_leaf_children(ast_node_t *node, char *component[], operation_list op, int flag, int ids);
extern void merge_nodes(nnode_t *node, nnode_t *next_node);
extern void remove_list_node(vtr::t_linked_vptr *node, vtr::t_linked_vptr *place);
extern void remove_fanout_pins(nnode_t *node);
extern void reallocate_pins(nnode_t *node, nnode_t *next_node);
extern void free_op_nodes(nnode_t *node);
extern int match_pins(nnode_t *node, nnode_t *next_node);

extern void instantiate_add_w_carry_block(int *width, nnode_t *node, short mark, netlist_t *netlist, short subtraction);


#endif // ADDERS_H
