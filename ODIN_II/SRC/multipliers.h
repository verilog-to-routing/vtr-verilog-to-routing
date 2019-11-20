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
#ifndef MULTIPLIERS_H
#define MULTIPLIERS_H

#include "read_xml_arch_file.h"
#include "util.h"

typedef struct s_multiplier
{
	int size_a;
	int size_b;
	int size_out;
	struct s_multiplier *next;
} t_multiplier;

extern t_model *hard_multipliers;
extern struct s_linked_vptr *mult_list;
extern int min_mult;

extern void init_mult_distribution();
extern void report_mult_distribution();
extern void declare_hard_multiplier(nnode_t *node);
extern void instantiate_hard_multiplier(nnode_t *node, short mark, netlist_t *netlist);
extern void instantiate_simple_soft_multiplier(nnode_t *node, short mark, netlist_t *netlist);
extern void find_hard_multipliers();
extern void add_the_blackbox_for_mults(FILE *out);
extern void define_mult_function(nnode_t *node, short type, FILE *out);
extern void split_multiplier(nnode_t *node, int a0, int b0, int a1, int b1);
extern void iterate_multipliers(netlist_t *netlist);
extern void clean_multipliers();

#endif // MULTIPLIERS_H

