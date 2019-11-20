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

#ifndef HARD_BLOCKS_H
#define HARD_BLOCKS_H

#include "types.h"

extern STRING_CACHE *hard_block_names;

extern void register_hard_blocks();
extern void deregister_hard_blocks();
extern t_model* find_hard_block(char *name);
extern void define_hard_block(nnode_t *node, short type, FILE *out);
extern void output_hard_blocks(FILE *out);
extern int hard_block_port_size(t_model *hb, char *pname);
extern enum PORTS hard_block_port_direction(t_model *hb, char *pname);
extern void instantiate_hard_block(nnode_t *node, short mark, netlist_t *netlist);
t_model_ports *get_model_port(t_model_ports *ports, char *name);


#endif

