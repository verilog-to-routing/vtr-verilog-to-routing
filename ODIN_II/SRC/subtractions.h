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
#ifndef SUBS_H
#define SUBS_H

#include "read_xml_arch_file.h"
#include "util.h"
#include "adders.h"

extern t_model *hard_subs;
extern struct s_linked_vptr *subs_list;
extern struct s_linked_vptr *sub_list;
extern struct s_linked_vptr *sub_one_node_list;

extern void init_sub_distribution();
extern void report_sub_distribution();
extern void declare_hard_adder_for_sub(nnode_t *node);
extern void instantiate_hard_adder_subtraction(nnode_t *node, short mark, netlist_t *netlist);
extern void find_hard_adders_for_sub();
extern void add_the_blackbox_for_subs(FILE *out);
extern void define_sub_function(nnode_t *node, short type, FILE *out);
extern void split_adder_for_sub(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist);
extern void iterate_adders_for_sub(netlist_t *netlist);
extern void clean_adders_for_sub();

#endif // SUBS_H

