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

typedef struct s_memory
{
	int size_d1;
	int size_d2;
	int size_addr1;
	int size_addr2;
	int size_out1;
	int size_out2;
	struct s_memory *next;
} t_memory;

typedef struct s_memory_port_sizes
{
	int size;
	char *name;
} t_memory_port_sizes;

extern struct s_linked_vptr *sp_memory_list;
extern struct s_linked_vptr *dp_memory_list;
extern struct s_linked_vptr *memory_instances;
extern struct s_linked_vptr *memory_port_size_list;
extern int split_size;

extern void init_memory_distribution();
extern void report_memory_distribution();

extern int get_memory_port_size(char *name);
extern void split_sp_memory_depth(nnode_t *node);
extern void split_dp_memory_depth(nnode_t *node);
extern void split_sp_memory_width(nnode_t *node);
extern void split_dp_memory_width(nnode_t *node);
extern void iterate_memories(netlist_t *netlist);
extern void clean_memories();

#endif // MEMORIES_H

