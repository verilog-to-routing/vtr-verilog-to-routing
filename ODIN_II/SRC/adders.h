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
#include "util.h"

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
extern struct s_linked_vptr *add_list;
extern struct s_linked_vptr *chain_list;
extern int total;
extern int min_add;
extern int min_threshold_adder;

extern void init_add_distribution();
extern void report_add_distribution();
extern void declare_hard_adder(nnode_t *node);
extern void instantiate_hard_adder(nnode_t *node, short mark, netlist_t *netlist);
extern void instantiate_simple_soft_adder(nnode_t *node, short mark, netlist_t *netlist);
extern void find_hard_adders();
extern void add_the_blackbox_for_adds(FILE *out);
extern void define_add_function(nnode_t *node, short type, FILE *out);
extern void split_adder(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist);
extern void iterate_adders(netlist_t *netlist);
extern void clean_adders();

#endif // ADDERS_H

