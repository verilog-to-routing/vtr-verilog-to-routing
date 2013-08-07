/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.

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
#include "cluster_router.h"

static void free_intra_lb_net(vector <t_intra_lb_net> *intra_lb_net);

/**
 Build data structures used by intra-logic block router
 */
t_lb_router_data *alloc_and_load_router_data(vector<t_lb_type_rr_node> *lb_type_graph) {
	t_lb_router_data *router_data = new t_lb_router_data;
	int size;

	router_data->lb_type_graph = lb_type_graph;
	size = router_data->lb_type_graph->size();
	router_data->lb_rr_node_stats = new t_lb_rr_node_stats[size];
	router_data->intra_lb_net = new vector<t_intra_lb_net>;

	return router_data;
}

/* free data used by router */
void free_router_data(t_lb_router_data *router_data) {
	if(router_data != NULL && router_data->lb_type_graph != NULL) {
		delete [] router_data->lb_rr_node_stats;
		router_data->lb_type_graph = NULL;
		router_data->lb_rr_node_stats = NULL;
		free_intra_lb_net(router_data->intra_lb_net);
		router_data->intra_lb_net = NULL;
		delete router_data;
	}
}

static void free_intra_lb_net(vector <t_intra_lb_net> *intra_lb_net) {
	/* jedit free later */
}


