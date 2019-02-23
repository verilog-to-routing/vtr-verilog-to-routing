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

#ifndef MEMORIES_H
#define MEMORIES_H

extern vtr::t_linked_vptr *sp_memory_list;
extern vtr::t_linked_vptr *dp_memory_list;
extern vtr::t_linked_vptr *memory_instances;
extern vtr::t_linked_vptr *memory_port_size_list;
extern t_model *single_port_rams;
extern t_model *dual_port_rams;

#define HARD_RAM_ADDR_LIMIT 33
#define SOFT_RAM_ADDR_LIMIT 10

typedef struct s_memory
{
	long size_d1;
	long size_d2;
	long size_addr1;
	long size_addr2;
	long size_out1;
	long size_out2;
	struct s_memory *next;
} t_memory;

typedef struct s_memory_port_sizes
{
	long size;
	char *name;
} t_memory_port_sizes;

typedef struct {
	signal_list_t *addr;
	signal_list_t *data;
	signal_list_t *out;
	npin_t *we;
	npin_t *clk;
} sp_ram_signals;

typedef struct {
	signal_list_t *addr1;
	signal_list_t *addr2;
	signal_list_t *data1;
	signal_list_t *data2;
	signal_list_t *out1;
	signal_list_t *out2;
	npin_t *we1;
	npin_t *we2;
	npin_t *clk;
} dp_ram_signals;

long get_sp_ram_split_depth();
long get_dp_ram_split_depth();

sp_ram_signals *get_sp_ram_signals(nnode_t *node);
void free_sp_ram_signals(sp_ram_signals *signalsvar);

dp_ram_signals *get_dp_ram_signals(nnode_t *node);
void free_dp_ram_signals(dp_ram_signals *signalsvar);

char is_sp_ram(nnode_t *node);
char is_dp_ram(nnode_t *node);

char is_ast_sp_ram(ast_node_t *node);
char is_ast_dp_ram(ast_node_t *node);

void init_memory_distribution();
void check_memories_and_report_distribution();

long get_memory_port_size(const char *name);

long get_sp_ram_depth(nnode_t *node);
long get_dp_ram_depth(nnode_t *node);
long get_sp_ram_width(nnode_t *node);
long get_dp_ram_width(nnode_t *node);

void split_sp_memory_depth(nnode_t *node, int split_size);
void split_dp_memory_depth(nnode_t *node, int split_size);
void split_sp_memory_width(nnode_t *node, int target_size);
void split_dp_memory_width(nnode_t *node, int target_size);
void iterate_memories(netlist_t *netlist);
void free_memory_lists();

void instantiate_soft_single_port_ram(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_soft_dual_port_ram(nnode_t *node, short mark, netlist_t *netlist);

signal_list_t *create_decoder(nnode_t *node, short mark, signal_list_t *input_list);

void add_input_port_to_memory(nnode_t *node, signal_list_t *signalsvar, const char *port_name);
void add_output_port_to_memory(nnode_t *node, signal_list_t *signalsvar, const char *port_name);

#endif // MEMORIES_H

