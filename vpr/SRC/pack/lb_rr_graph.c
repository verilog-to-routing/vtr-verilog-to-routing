/* 
  Creates a lb_rr_node graph to represent interconnect within a logic block.

  Author: Jason Luu
  Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "pack_types.h"
#include "lb_rr_graph.h"

static t_lb_type_rr_node* alloc_and_load_lb_type_rr_graph_for_type(t_type_ptr lb_type);



/* Populate each logic block type (type_descriptor) with a directed graph that respresents the interconnect within it.
*/
void alloc_and_load_all_lb_type_rr_graph() {
	
	for(int i = 0; i < num_types; i++) {
		if(&type_descriptors[i] != EMPTY_TYPE) {
			type_descriptors[i].lb_type_rr_node = alloc_and_load_lb_type_rr_graph_for_type(&type_descriptors[i]);
		}
	}
}


static t_lb_type_rr_node* alloc_and_load_lb_type_rr_graph_for_type(t_type_ptr lb_type) {
	return NULL;
}

