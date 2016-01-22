
/*

The idea is to run a Montecarlo A* search from a bunch of random sources to a bunch of random sinks.

Ideally we'd check every source to every sink, but that seems unreasonable. So for each source we can
check a representative number of random sinks at each "distance" according to some #define'd depth.

So I think the question is:
 - how do we pick out random sources?
 - how do we pick out random sinks?
 - what are the results?
 	- timing cost to route to the sink?
 - should we hold results for each sink touched?
 - what data structure should we use to hold the results?
 	- for each source, hold data for each sink touched?
 - how do we analyze the results?
 - how do we present the results?

 - can also analyze each level of depth separately so we don't have to hold as much intermediate information

 - need to do this for every physical block type? also, to every physical block type?


TODO:
- routing function from specified node to specified node that returns a (delay, base_cost) result.


*/

#include <vector>
#include <queue>
#include <cmath>
#include <assert.h>
#include "vpr_types.h"
#include "vpr_utils.h"
#include "util.h"
#include "globals.h"
#include "timing_driven_lookup.h"
#include "profile_routing_paths.h"



/* contains the timing and path cost associated with some source-->sink routing */
class Route_Costs{
public:
	float timing_cost;
	float path_cost;

	void set_costs(float set_timing_cost, float set_path_cost){
		this->timing_cost = set_timing_cost;
		this->path_cost = set_path_cost;
	}

	Route_Costs(){
		this->set_costs(0.0, 0.0);
	}
	Route_Costs(float set_timing_cost, float set_path_cost){
		this->set_costs(set_timing_cost, set_path_cost);
	}
};


/* a class that represents an entry in the A* expansion priority queue */
class PQ_Entry{
public:
	int rr_node_ind;		//index of rr_node that this entry represents
	float cost;			//the cost of this node during A* expansion

	/* store backward delay, R and congestion info */
	float delay;
	float R_upstream;
	float congestion_upstream;

	PQ_Entry(int set_rr_node_ind, int switch_ind, float parent_delay, float parent_R_upstream, float parent_congestion_upstream){
		this->rr_node_ind = set_rr_node_ind;

		float new_R_upstream = 0;
		if (switch_ind != UNDEFINED){
			new_R_upstream = g_rr_switch_inf[switch_ind].R;	
			if (!g_rr_switch_inf[switch_ind].buffered){
				new_R_upstream += parent_R_upstream;
			}
		}
		
		/* get delay info for this node */
		this->delay = parent_delay + rr_node[set_rr_node_ind].C * (new_R_upstream + 0.5 * rr_node[set_rr_node_ind].R);
		if (switch_ind != UNDEFINED){
			this->delay += g_rr_switch_inf[switch_ind].Tdel;
		}
		new_R_upstream += rr_node[set_rr_node_ind].R;
		this->R_upstream = new_R_upstream;

		/* get congestion info for this node */
		int cost_index = rr_node[set_rr_node_ind].get_cost_index();
		this->congestion_upstream = parent_congestion_upstream + rr_indexed_data[cost_index].base_cost;

		/* set the cost of this node */
		//this->cost = this->delay;
	}

	/* sets cost of this entry based on delay to this node + estimated cost to target */
	void set_cost_with_estimate(float remaining_cost_estimate){
		this->cost = this->delay + remaining_cost_estimate;
	}

	bool operator < (const PQ_Entry &obj) const{
		/* inserted into max priority queue so want queue entries with a lower cost to be greater */
		return (this->cost > obj.cost);
	}
};


/************ File-Scope Functions ************/
/* uses A* to route from the specified source to the specified sink */
static Route_Costs route_nodes(int from_node, int to_node);
/* iterates over the children of the specified node and selectively pushes them onto the expansion queue */
static void expand_neighbours(PQ_Entry parent_entry, int target_ind, vector<float> &node_visited_costs, vector<bool> &node_expanded, priority_queue<PQ_Entry> &pq);


/************ Function Definitnions ************/

/* uses A* to route from the specified source to the specified sink */
static Route_Costs route_nodes(int from_node, int to_node){
	Route_Costs route_costs;

	/* a list of boolean flags (one for each rr node) to figure out if a certain node has already been expanded */
	vector<bool> node_expanded( num_rr_nodes, false );
	/* for each node keep a list of the cost with which that node has been visited (used to determine whether to push
	   a candidate node onto the expansion queue */
	vector<float> node_visited_costs( num_rr_nodes, -1.0 );
	/* a priority queue for the A* expansion */
	priority_queue<PQ_Entry> pq;

	PQ_Entry first_entry(from_node, UNDEFINED, 0.0, 0.0, 0.0);
	pq.push(first_entry);

	/* now run A* */
	while ( !pq.empty() ){
		PQ_Entry current = pq.top();
		pq.pop();

		int node_ind = current.rr_node_ind;

		if (node_expanded[node_ind]){
			continue;
		}

		if (node_ind == to_node){
			/* Found target node. We're done here */
			route_costs.set_costs( current.delay, current.congestion_upstream );
			break;
		}

		expand_neighbours(current, to_node, node_visited_costs, node_expanded, pq);
		node_expanded[node_ind] = true;
	}

	return route_costs;
}

/* iterates over the children of the specified node and selectively pushes them onto the expansion queue */
static void expand_neighbours(PQ_Entry parent_entry, int target_ind, vector<float> &node_visited_costs, vector<bool> &node_expanded, priority_queue<PQ_Entry> &pq){

	int parent_ind = parent_entry.rr_node_ind;

	RR_Node &parent_node = rr_node[parent_ind];

	for (int iedge = 0; iedge < parent_node.get_num_edges(); iedge++){
		int child_node_ind = parent_node.edges[iedge];
		int switch_ind = parent_node.switches[iedge];
		t_rr_type child_type = rr_node[child_node_ind].type;

		/* skip this child if it has already been expanded from */
		if (node_expanded[child_node_ind]){
			continue;
		}

		PQ_Entry child_entry(child_node_ind, switch_ind, parent_entry.delay,
		                             parent_entry.R_upstream, parent_entry.congestion_upstream);

		/* set child cost */
		if (child_type == CHANX || child_type == CHANY){
			/* get an estimate of the remaining delay to sink */
			float dummy;
			float delay_estimate = get_lookahead_map_cost(child_node_ind, target_ind, 1.0, dummy, dummy);
			child_entry.set_cost_with_estimate(delay_estimate);
		} else {
			/* cost will not include any estimates of remaining delay */
			child_entry.set_cost_with_estimate(0.0);
		}

		assert(child_entry.cost >= 0);

		/* skip this child if it has been visited with smaller cost */
		if (node_visited_costs[child_node_ind] >= 0 && node_visited_costs[child_node_ind] < child_entry.cost){
			continue;
		}

		/* finally, record the cost with which the child was visited and put the child entry on the queue */
		node_visited_costs[child_node_ind] = child_entry.cost;
		pq.push(child_entry);
	}

}



