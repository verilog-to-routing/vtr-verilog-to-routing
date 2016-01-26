
/*

	Use A* to compute minimum delays from a subset of sources to random sinks at different Manhattan distances.
	Compute figures of merit for the delays at each Manhattan distance.

*/

#include <vector>
#include <queue>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
#include "vpr_types.h"
#include "vpr_utils.h"
#include "util.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "profile_routing_paths.h"

using namespace std;


#define MAX_DISTANCE 50 //100		//Analyze up to this Manhattan distance from sources
#define SOURCES_TO_ANALYZE 20		//Number of sources which will be analyzed
#define INCREMENT 1			//Distances analyzed will be (1 + n*INCREMENT) n = 0,1,2,...
#define MAX_SINKS 200			//The maximum number of sinks to analyze for any given source at some distance

bool f_profiling_done = false;

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
		this->set_costs(-1.0, -1.0);
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
		//printf("%.2e\n", this->delay);
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

static void get_sources_of_pb_type(int pb_index, vector<int> &pb_type_sources);
static void get_random_nodes(int num_random_nodes, vector<int> &nodes, vector<int> &selected_nodes);
static void get_sinks_at_distance(int source_ind, int distance, int pb_ind, vector<int> &sinks);
static void print_statistics( int distance, vector<Route_Costs> &route_costs );

/************ Function Definitnions ************/

/* Analyzes the consistency of sources having fast paths to sinks at different distances. Prints out corresponding statistics */
void profile_routing_paths(){

	/* make sure profiling is only run once per VPR run */
	if (f_profiling_done){
		return;
	}

	/* get list of possible sources */
	vector<int> lb_sources;
	get_sources_of_pb_type(FILL_TYPE->index, lb_sources);
	/* get a subset of possible sources */
	vector<int> analyze_sources;
	get_random_nodes(SOURCES_TO_ANALYZE, lb_sources, analyze_sources);

	/* Analyze LB->LB routing paths for now. Can add other block types later */	
	//auto target_pb_inds = {FILL_TYPE->index};

	//for (int pb_ind : target_pb_inds){
	for (int pb_ind = 0; pb_ind < num_types; pb_ind++){
		vpr_printf_info("==== %s to %s Routing Delay Profile ====\n", FILL_TYPE->name, type_descriptors[pb_ind].name);
		vpr_printf_info("\tdist \t#sinks \tmin \t\tmax \t\tmean \t\t(max-min)/mean*100 \tstandard deviation\n");
		for (int idist = 1; idist <= MAX_DISTANCE; idist += INCREMENT){
			vector<Route_Costs> route_costs;

			for (int source_ind : analyze_sources){
				vector<int> possible_sinks;
				get_sinks_at_distance(source_ind, idist, pb_ind, possible_sinks);
				
				int num_sinks_to_analyze = (int)possible_sinks.size();
				num_sinks_to_analyze = min( num_sinks_to_analyze, MAX_SINKS );
				if (num_sinks_to_analyze == 0){
					continue;
				}

				vector<int> analyze_sinks;
				get_random_nodes(num_sinks_to_analyze, possible_sinks, analyze_sinks);

				/* now analyze the sinks */
				for (int sink_ind : analyze_sinks){
					Route_Costs cost = route_nodes(source_ind, sink_ind);
					route_costs.push_back(cost);
				}
			}
			/* ... and print the statistics */
			print_statistics( idist, route_costs );
		}
	}

	f_profiling_done = false;
}


/* prints statistics for the specified routing costs vector */
static void print_statistics( int distance, vector<Route_Costs> &route_costs ){
	/* print min/max/mean delay and the std. deviation */
	
	int sinks_analyzed = (int)route_costs.size();

	if (sinks_analyzed == 0){
		return;
	}

	/* get min, max, mean delay */
	float min_delay = UNDEFINED;
	float max_delay = UNDEFINED;
	float mean_delay = 0;
	for (Route_Costs cost : route_costs){
		float delay = cost.timing_cost;
		if (delay < min_delay || min_delay == UNDEFINED){
			min_delay = delay;	
		}
		if (delay > max_delay || max_delay == UNDEFINED){
			max_delay = delay;
		}
		mean_delay += delay;
	}
	mean_delay /= sinks_analyzed;

	/* get standard deviation of delays */
	float delay_sdev = 0;
	for (Route_Costs cost : route_costs){
		float delay = cost.timing_cost;
		delay_sdev += (delay - mean_delay) * (delay - mean_delay);
	}
	delay_sdev /= sinks_analyzed;
	delay_sdev = sqrt(delay_sdev);

	float figure_merit = (max_delay - min_delay) / mean_delay * 100.0;

	vpr_printf_info("\t%d \t%d \t%.2e \t%.2e \t%.2e \t%f \t\t%.2e\n", distance, sinks_analyzed, min_delay, max_delay, mean_delay, figure_merit, delay_sdev);
}


/* returns a vector of rr node indices representing sources belonging to the specified pb type */
static void get_sources_of_pb_type(int pb_index, vector<int> &pb_type_sources){
	pb_type_sources.clear();

	for (int inode = 0; inode < num_rr_nodes; inode++){
		RR_Node &node = rr_node[inode];

		/* push this node index onto vector if it is a source that belongs to the specified pb type */
		if (node.type != SOURCE){
			continue;
		}

		int x = node.get_xlow();
		int y = node.get_xhigh();

		int node_pb_index = grid[x][y].type->index;

		if (node_pb_index == pb_index){
			pb_type_sources.push_back(inode);
		}
	}
}


/* returns a number of random nodes from "nodes" vector via the "selected_nodes" vector */
static void get_random_nodes(int num_random_nodes, vector<int> &nodes, vector<int> &selected_nodes){
	assert(num_random_nodes <= (int)nodes.size());
	selected_nodes.clear();

	/* use a random shuffle and then get the required number of nodes */
	random_shuffle(nodes.begin(), nodes.end());

	for (int inode = 0; inode < num_random_nodes; inode++){
		int node_ind = nodes[inode];
		selected_nodes.push_back(node_ind);
	}
}


/* fills the "sinks" vector with node indices of sinks that are a specified Manhattan distance away from the specified source node, and that
   belong to the specified PB type */
static void get_sinks_at_distance(int source_ind, int distance, int pb_ind, vector<int> &sinks){
	sinks.clear();

	RR_Node &source = rr_node[source_ind];
	int source_x = source.get_xlow();
	int source_y = source.get_ylow();

	/* iterate over every possible x,y that satisfies |x| + |y| = distance */
	for (int ix = -distance; ix <= distance; ix++){
		for (int mult : {-1, 1}){
			/* for every x coordinate, the y coordinate has two possible values */
			int iy = mult * (distance - abs(ix));

			int pb_x = source_x + ix;
			int pb_y = source_y + iy;

			/* make sure we aren't out of bounds */
			if (pb_x < 0 || pb_y < 0 || 
			    pb_x > nx+1 || pb_y > ny+1){
				continue;
			}

			/* check which physical block type is at that coordinate */
			if ( grid[pb_x][pb_y].type->index != pb_ind ){
				continue;
			}

			/* get all the sinks at that coordinate */
			t_ivec sink_list = rr_node_indices[SINK][pb_x][pb_y];
			if (sink_list.list == NULL){
				continue;
			}
			for (int isink = 0; isink < sink_list.nelem; isink++){
				int node_ind = sink_list.list[isink];
				if (rr_node[node_ind].type != SINK){
					continue;
				}
				if (rr_node[node_ind].get_xlow() != pb_x || rr_node[node_ind].get_ylow() != pb_y){
					continue;
				}

				int ptc = rr_node[node_ind].get_ptc_num();
				int num_corresponding_pins = type_descriptors[pb_ind].class_inf[ptc].num_pins;
				if (num_corresponding_pins == 0){
					continue;
				}
				int representative_pin = type_descriptors[pb_ind].class_inf[ptc].pinlist[0];

				if (type_descriptors[pb_ind].is_global_pin[representative_pin]){
					continue;
				}

				sinks.push_back( node_ind );
			}

			if (iy == 0){
				/* if iy=0, don't repeat sinks for same coordinate twice */
				break;
			}
		}
	}
}


/* uses A* to route from the specified source to the specified sink */
static Route_Costs route_nodes(int from_node, int to_node){
	Route_Costs route_costs;

	assert( rr_node[from_node].type == SOURCE );
	assert( rr_node[to_node].type == SINK );

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
			//printf("%.2e, %.2e\n", current.delay, current.congestion_upstream);
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
	//printf("%d, %d\n", parent_node.type, parent_node.get_num_edges());

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

