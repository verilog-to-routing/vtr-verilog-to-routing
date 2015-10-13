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
#include "ace.h"

/*---------------------------------------------------------------------------
 * (function: alloc_and_init_ace_structs )
 *
 * Allocates and initializes Ace Objects for a given NetList
 *
 *---------------------------------------------------------------------------*/
void alloc_and_init_ace_structs(netlist_t *net) {
  int x,y;

  // Input Nodes
  for ( x = 0; x < net->num_top_input_nodes; x++ ) {
    for ( y = 0; y < net->top_input_nodes[x]->num_output_pins; y++ ) {
      net->top_input_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object(); 
    }
  }

  // FF Nodes
  for ( x = 0; x < net->num_ff_nodes; x++ ) {
    for ( y = 0; y < net->ff_nodes[x]->num_output_pins; y++ ) {
      net->ff_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object(); 
    }
  }

  // Internal Nodes
  for ( x = 0; x < net->num_internal_nodes; x++ ) {
    for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
      net->internal_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object(); 
    }
  }

}


/*---------------------------------------------------------------------------
 * (function: alloc_and_init_ace_info_object )
 *
 * Allocates, initializes and returns an Ace Object
 *
 *---------------------------------------------------------------------------*/
ace_obj_info_t* alloc_and_init_ace_info_object() {

  ace_obj_info_t *new_node;

  new_node =  (ace_obj_info_t *)my_malloc_struct(sizeof(ace_obj_info_t)); 

  new_node->value = -1;
  new_node->num_toggles = 0;
  new_node->num_ones = 0;

  return new_node;
}
