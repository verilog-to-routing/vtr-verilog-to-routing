/* 
 jedit this may be deprecated, delete
  The logic block rr graph manages the intra-logic block routing for every logic block instance used in the FPGA.
  Each lb_rr_node_stats represents info on an instance of a lb_type_rr_node and shares same indices as the lb_type_rr_node.

  Common acryonyms:
      rr - routing resource
	  lb - logic block
	  pb - phsyical block (the top level physical block is the logic block, a leaf physical block is a primitive)

  Author: Jason Luu
  Date: August 2, 2013
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>
#include <vector>
#include <cmath>

#include "util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "pack_types.h"
#include "lb_rr_graph.h"


/**
   Returns array of t_lb_rr_node_stats that store the status of a logic block routing resource node.
   Index in array corresponds to the index of a node in the lb_type_rr_graph.
*/
t_lb_rr_node_stats* alloc_and_load_lb_rr_node_stats(const vector <t_lb_type_rr_node> & lb_type_rr_graph) {
	int size = lb_type_rr_graph.size();
	t_lb_rr_node_stats *lb_rr_node_stats;

	lb_rr_node_stats = new t_lb_rr_node_stats[size];

	return lb_rr_node_stats;
}

/**
   Free array of t_lb_rr_node_stats
*/
void free_lb_rr_node_stats(t_lb_rr_node_stats* lb_rr_node_stats) {
	if(lb_rr_node_stats != NULL) {
		delete [] lb_rr_node_stats;
	}
}

