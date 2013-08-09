/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
  
  Global Inputs: Architecture and netlist
  Input arguments: clustering info for one cluster (t_pb info)
  Working data set: t_routing_data contains intermediate work
  Output: Routable? True/False.  If true, store/return the routed solution.

  Routing algorithm used is Pathfinder.

  Author: Jason Luu
  Date: July 22, 2013
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
#include "lb_type_rr_graph.h"
#include "cluster_router.h"


/*****************************************************************************************
* Internal functions declarations
******************************************************************************************/
static void free_intra_lb_net(vector <t_intra_lb_net> *intra_lb_net);
static void free_lb_trace(t_lb_trace *lb_trace);

/*****************************************************************************************
* Constructor/Destructor functions 
******************************************************************************************/

/**
 Build data structures used by intra-logic block router
 */
t_lb_router_data *alloc_and_load_router_data(INP vector<t_lb_type_rr_node> *lb_type_graph) {
	t_lb_router_data *router_data = new t_lb_router_data;
	int size;

	router_data->lb_type_graph = lb_type_graph;
	size = router_data->lb_type_graph->size();
	router_data->lb_rr_node_stats = new t_lb_rr_node_stats[size];
	router_data->intra_lb_net = new vector<t_intra_lb_net>;

	return router_data;
}

/* free data used by router */
void free_router_data(INOUTP t_lb_router_data *router_data) {
	if(router_data != NULL && router_data->lb_type_graph != NULL) {
		delete [] router_data->lb_rr_node_stats;
		router_data->lb_type_graph = NULL;
		router_data->lb_rr_node_stats = NULL;
		free_intra_lb_net(router_data->intra_lb_net);
		router_data->intra_lb_net = NULL;
		delete router_data;
	}
}


/*****************************************************************************************
* Routing Functions
******************************************************************************************/

/* Add pins of netlist atom to to current routing drivers/targets */
void add_atom_as_target(INOUTP t_lb_router_data *router_data, INP int iatom) {
	t_pb *pb;
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;

	pb = logical_block[iatom].pb;
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;	

	//set_pb_modes(router_data, pb);
}

/* Remove pins of netlist atom from current routing drivers/targets */
void remove_atom_from_target(INOUTP t_lb_router_data *router_data, INP int iatom) {
	t_pb *pb;
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;

	pb = logical_block[iatom].pb;
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;		

	//reset_pb_modes(router_data, pb);
}

/* Set mode of rr nodes to the pb used. */
void set_pb_modes(INOUTP t_lb_router_data *router_data, INP t_pb *pb) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	int mode = pb->mode;
	int inode;
		
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* Input and clock pin modes are based on current pb mode */
	for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
			inode = pb_graph_node->input_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = mode;
		}
	}

	for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
			inode = pb_graph_node->clock_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = mode;
		}
	}

	/* Output pin modes are based on parent pb, so set children to have new mode */


	
}

/* Mode of rr nodes corresponding to pb is reset */
void reset_pb_modes(INOUTP t_lb_router_data *router_data, INP t_pb *pb) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;

	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;
}


/* Save current route solution 
   Requires that current route solution is correct
*/
void save_current_route(INOUTP t_lb_router_data *router_data) {
	assert(router_data->is_routed == TRUE);
	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_net;
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_trace(lb_nets[inet].saved_rt_tree);
		lb_nets[inet].saved_rt_tree = (*router_data->intra_lb_net)[inet].rt_tree;
		lb_nets[inet].rt_tree = NULL;
	}	
}

/* Restore previous route solution 
   Assumes previous route solution is correct
*/
void restore_previous_route(INOUTP t_lb_router_data *router_data) {
	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_net;
	router_data->is_routed = TRUE;
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_trace(lb_nets[inet].rt_tree);
		lb_nets[inet].rt_tree = (*router_data->intra_lb_net)[inet].saved_rt_tree;
		lb_nets[inet].saved_rt_tree = NULL;
	}	
}


/* Attempt to route routing driver/targets on the current architecture */
boolean try_route(INOUTP t_lb_router_data *router_data) {
	return FALSE;
}

/*****************************************************************************************
* Accessor Functions
******************************************************************************************/

/* Returns an array representing the final route.  This array is sized [0..num_lb_type_rr_nodes=1].
   The index of the array stores the net (or OPEN if no net is used) that occupies the rr node for the logic
   block instance

   Note: This function assumes that the atoms packed inside the cluster has already been successfully routed
*/
t_lb_traceback *get_final_route(INOUTP t_lb_router_data *router_data) {
	return NULL;
}





/***************************************************************************
Internal Functions
****************************************************************************/


static void free_intra_lb_net(vector <t_intra_lb_net> *intra_lb_net) {
	/* jedit free later */
	delete intra_lb_net;
}

/* Free trace for intra-logic block routing */
static void free_lb_trace(t_lb_trace *lb_trace) {
	if(lb_trace != NULL) {
		delete (lb_trace);
	}
}

