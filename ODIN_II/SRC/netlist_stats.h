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
#ifndef _NETLIST_STATS_H
#define _NETLIST_STATS_H 1

void netlist_stats(netlist_t *netlist, char *path, char *name);

void depth_first_traverse_stats(FILE *out, short marker_value, netlist_t *netlist);
void depth_first_traversal_graphcrunch_stats(nnode_t *node, FILE *fp, int traverse_mark_number);

void display_per_node_stats(FILE *fp, netlist_t *netlist);
void display_node_stats(FILE *fp, nnode_t* node);

void calculate_avg_fanout(netlist_t *netlist);
void calculate_avg_fanin(netlist_t *netlist);

int num_fanouts_on_output_pin(nnode_t *node, int output_pin_idx);
void add_to_distribution(int **distrib_ptr, int *distrib_size, int new_element);
void calculate_combinational_shapes(netlist_t *netlist);

#endif 
