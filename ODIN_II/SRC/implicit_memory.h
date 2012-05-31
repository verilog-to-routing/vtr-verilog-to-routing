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
#ifndef _IMPLICIT_MEMORY_H_
#define _IMPLICIT_MEMORY_H_

// Hashes the implicit memory name to the implicit_memory structure.
hashtable_t *implicit_memories;
// Hashes the implicit memory input name to the implicit_memory structure.
hashtable_t *implicit_memory_inputs;

/*
 * Contains a pointer to the implicit memory node as well as other
 * information which is used in creating the implicit memory.
 */
typedef struct {
	nnode_t *node;
	int data_width;
	int addr_width;
	char clock_added;
	char output_added;
	char *name;
} implicit_memory;

void add_input_port_to_implicit_memory(implicit_memory *memory, signal_list_t *signals, char *port_name);
void add_output_port_to_implicit_memory(implicit_memory *memory, signal_list_t *signals, char *port_name);

implicit_memory *lookup_implicit_memory_reference_ast(char *instance_name_prefix, ast_node_t *node);

char is_valid_implicit_memory_reference_ast(char *instance_name_prefix, ast_node_t *node);

implicit_memory *create_implicit_memory_block(int data_width, long long words, char *name, char *instance_name_prefix);

implicit_memory *lookup_implicit_memory_input(char *name);

void register_implicit_memory_input(char *name, implicit_memory *memory);

void init_implicit_memory_index();
void free_implicit_memory_index_and_finalize_memories();

#endif
