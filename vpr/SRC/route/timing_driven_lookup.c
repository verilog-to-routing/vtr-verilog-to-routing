//TODO: rename to 'timing_route_lookahead'


#include <cmath>
#include <vector>
#include <queue>
#include <assert.h>
#include "vpr_types.h"
#include "vpr_utils.h"
#include "util.h"
#include "globals.h"
#include "timing_driven_lookup.h"

using namespace std;

/* the cost map is computed by running a BFS from channel segment rr nodes at the specified reference coordinate */
#define REF_X 3
#define REF_Y 3


/* f_cost_map is an array of these cost entries that specifies delay/congestion estimates
   to travel relative x/y distances */
class Cost_Entry{
public:
	float delay;
	float congestion;	

	Cost_Entry(){
		delay = -1.0;
		congestion = -1.0;
	}
	Cost_Entry(float set_delay, float set_congestion){
		delay = set_delay;
		congestion = set_congestion;
	}
};


/* a class that stores delay/congestion information for a given relative coordinate during the BFS expansion. 
   since it stores multiple cost entries, it is later boiled down to a single representative cost entry to be stored 
   in the final lookahead cost map */
class BFS_Cost_Entry{
private:
	vector<Cost_Entry> cost_vector;

public:
	void add_cost_entry(float add_delay, float add_congestion){
		Cost_Entry cost_entry(add_delay, add_congestion);
		this->cost_vector.push_back(cost_entry);
	}
	void clear_cost_entries(){
		this->cost_vector.clear();
	}

	Cost_Entry get_representative_cost_entry(){
		if (cost_vector.empty()){
			return Cost_Entry();
		} else {
			//TODO: try some more complicated stuff like geomean of all entries
			return cost_vector[0];
			//return cost_vector[ cost_vector.size()-1 ];

			//TODO: 
			//	- compute map over a few more wires
			//	- try min over all entries
			//	- try median over all entries

			//Cost_Entry min_cost_entry = cost_vector[0];
			//for (auto entry : cost_vector){
			//	if (entry.delay < min_cost_entry.delay){
			//		min_cost_entry = entry;
			//	}
			//}

			//return min_cost_entry;


			//double geomean_delay = 0;
			//double geomean_congestion = 0;

			//for (auto cost_entry : cost_vector){
			//	geomean_delay += cost_entry.delay;
			//	geomean_congestion += cost_entry.congestion;
			//	//printf("%.3e %.3e\n", geomean_delay, geomean_congestion);
			//}
			////geomean_delay = pow( geomean_delay, 1./(float)cost_vector.size() );
			////geomean_congestion = pow( geomean_congestion, 1./(float)cost_vector.size() );
			////printf("\t\t%.3e %.3e\n", geomean_delay, geomean_congestion);
			//geomean_delay /= (float)cost_vector.size();
			//geomean_congestion /= (float)cost_vector.size();
			//return Cost_Entry(geomean_delay, geomean_congestion);
		}
	}
};


/* a class that represents an entry in the BFS expansion queue */
class BFS_Queue_Entry{
public:
	int rr_node_ind;		//index of rr_node that this entry represents
	float cost;				//the cost of the path to get to this node

	//store backward delay, R and congestion info
	float delay;
	float R_upstream;
	float congestion_upstream;

	BFS_Queue_Entry(int set_rr_node_ind, int switch_ind, float parent_delay, float parent_R_upstream, float parent_congestion_upstream){
		this->rr_node_ind = set_rr_node_ind;

		float new_R_upstream = 0;
		if (switch_ind != UNDEFINED){
			new_R_upstream = g_rr_switch_inf[switch_ind].R;	
			if (!g_rr_switch_inf[switch_ind].buffered){
				new_R_upstream += parent_R_upstream;
			}
		}
		
		/* get delay info for this node */
		this->delay = parent_delay + rr_node[set_rr_node_ind].C * (new_R_upstream + 0.5 * rr_node[set_rr_node_ind].R)
		                + g_rr_switch_inf[switch_ind].Tdel;
		new_R_upstream += rr_node[set_rr_node_ind].R;
		this->R_upstream = new_R_upstream;

		/* get congestion info for this node */
		int cost_index = rr_node[set_rr_node_ind].get_cost_index();
		this->congestion_upstream = parent_congestion_upstream + rr_indexed_data[cost_index].base_cost;

		/* set the cost of this node */
		this->cost = this->delay;
	}

	bool operator < (const BFS_Queue_Entry &obj) const{
		/* inserted into max priority queue so want queue entries with a lower cost to be greater */
		return (this->cost > obj.cost);
	}
};


/* provides delay/congestion estimates to travel specified distances 
   in the x/y direction */
typedef vector< vector< vector< vector<Cost_Entry> > > > t_cost_map;		//[0..1][[0..num_seg_types-1]0..nx][0..ny]	-- [0..1] entry is to 
                                                                            //distinguish between CHANX/CHANY start nodes respectively
/* used during BFS expansion to store delay/congestion info lists for each relative coordinate for a given segment and channel type. 
   the list at each coordinate is later boiled down to a single representative cost entry to be stored in the final cost map */
typedef vector< vector<BFS_Cost_Entry> > t_bfs_cost_map;		//[0..nx][0..ny]


/******** File-Scope Variables ********/
/* The cost map */
t_cost_map f_cost_map;


/******** File-Scope Functions ********/
static void alloc_cost_map(int num_segments);
static void free_cost_map();
/* returns index of a node from which to start BFS */
static int get_start_node_ind(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index);
/* runs BFS from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
   to that pin is stored is added to an entry in the bfs_cost_map */
static void run_bfs(int start_node_ind, int start_x, int start_y, t_bfs_cost_map &bfs_cost_map);
/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_bfs_neighbours(BFS_Queue_Entry parent_entry, vector<float> &node_visited_costs, vector<bool> &node_expanded, priority_queue<BFS_Queue_Entry> &pq);
/* sets the lookahead cost map entries based on representative cost entries from bfs_cost_map */
static void set_lookahead_map_costs(int segment_index, e_rr_type chan_type, t_bfs_cost_map &bfs_cost_map);
/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type);
/* returns a cost entry in the f_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static Cost_Entry get_nearby_cost_entry(int x, int y, int segment_index, int chan_index);


/******** Function Definitions ********/
/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
   from the specified source to the specified target */
float get_lookahead_map_cost(int from_node_ind, int to_node_ind, float criticality_fac, float &delay, float &cong){
	int from_x, from_y, to_x, to_y;

	e_rr_type from_type = rr_node[from_node_ind].type;
	int from_cost_index = rr_node[from_node_ind].get_cost_index();
	int from_seg_index = rr_indexed_data[from_cost_index].seg_index;

	assert(from_seg_index >= 0);

	// TODO: can probably make a more informed choice w.r.t. xlow/xhigh & ylow/yhigh
	// TODO: can try setting cost to 0 if target node is very close 
	from_x = rr_node[from_node_ind].get_xlow();
	from_y = rr_node[from_node_ind].get_ylow();
	
	to_x = rr_node[to_node_ind].get_xlow();
	to_y = rr_node[to_node_ind].get_ylow();

	int delta_x = abs(from_x - to_x);
	int delta_y = abs(from_y - to_y);


	/* now get the expected cost from our lookahead map */
	int from_chan_index = 0;
	if (from_type == CHANY){
		from_chan_index = 1;
	}

	//printf("from_chan_index %d  from_seg_index %d  delta_x %d  delta_y %d\n", from_chan_index, from_seg_index, delta_x, delta_y);
	Cost_Entry cost_entry = f_cost_map[from_chan_index][from_seg_index][delta_x][delta_y];
	float expected_delay = cost_entry.delay;
	float expected_congestion = cost_entry.congestion;

	float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;

	delay = expected_delay;
	cong = expected_congestion;

	return expected_cost;
}

/* Computes the lookahead map to be used by the router. If a map was computed prior to this, it will be cleared
   and a new one will be allocated. The rr graph must have been built before calling this function. */
void compute_timing_driven_lookahead(int num_segments){
	//TODO: account for bend cost??

	vpr_printf_info("Computing timing driven router look-ahead...\n");

	/* free previous delay map and allocate new one */
	free_cost_map();
	alloc_cost_map(num_segments);

	/* run BFS for each segment type & channel type combination */
	for (int iseg = 0; iseg < num_segments; iseg++){
		for (e_rr_type chan_type : {CHANX, CHANY}){
			/* allocate the BFS cost map for this iseg/chan_type */
			t_bfs_cost_map bfs_cost_map;
			bfs_cost_map.assign( nx+2, vector<BFS_Cost_Entry>(ny+2, BFS_Cost_Entry()) );

			/* get the rr node index from which to start BFS */
			int start_node_ind = get_start_node_ind(REF_X, REF_Y, nx, ny, chan_type, iseg);

			if (start_node_ind == UNDEFINED){
				assert(false);
				continue;
			}

			/* run BFS */
			run_bfs(start_node_ind, REF_X, REF_Y, bfs_cost_map);
			
			/* boil down the cost list in bfs_cost_map at each coordinate to a representative cost entry and store it in the lookahead
			   cost map */
			set_lookahead_map_costs(iseg, chan_type, bfs_cost_map);

			//assert(false);
			/* fill in missing entries in the lookahead cost map by copying the closest cost entries (BFS cost map was computed based on
			   a reference coordinate > (0,0) so some entries that represent a cross-chip distance have not been computed) */
			fill_in_missing_lookahead_entries(iseg, chan_type);
		}
	}

	//XXX printing out delay maps
	//for (int iseg = 0; iseg < num_segments; iseg++){
	//	for (int chan_index : {0,1}){
	//		for (int iy = 0; iy < ny+1; iy++){
	//			for (int ix = 0; ix < nx+1; ix++){
	//				printf("%.3e\t", f_cost_map[chan_index][iseg][ix][iy].delay);
	//			}
	//			printf("\n");
	//		}
	//		printf("\n\n");
	//	}
	//}
}


/* returns index of a node from which to start BFS */
static int get_start_node_ind(int start_x, int start_y, int target_x, int target_y, t_rr_type rr_type, int seg_index){

	int result = UNDEFINED;

	if (rr_type != CHANX && rr_type != CHANY){
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, "Must start lookahead BFS from CHANX or CHANY node\n");	
	}

	int chan_coord = start_x;
	int seg_coord = start_y;
	if (rr_type == CHANY){
		chan_coord = start_y;
		seg_coord = start_x;
	}

	/* determine which direction the wire should go in based on the start & target coordinates */
	e_direction direction = INC_DIRECTION;
	if ((rr_type == CHANX && target_x < start_x) ||
		    (rr_type == CHANY && target_y < start_y)){
		direction = DEC_DIRECTION;
	}

	t_ivec channel_node_list = rr_node_indices[rr_type][chan_coord][seg_coord];		//XXX: indexing by chan/seg is correct, right?

	//printf("num tracks: %d\n", channel_node_list.nelem);
	/* find first node in channel that has specified segment index and goes in the desired direction */
	for (int itrack = 0; itrack < channel_node_list.nelem; itrack++){
		int node_ind = channel_node_list.list[itrack];

		e_direction node_direction = rr_node[node_ind].get_direction();
		int node_cost_ind = rr_node[node_ind].get_cost_index();
		int node_seg_ind = rr_indexed_data[node_cost_ind].seg_index;

		//printf("\ttrack %d  cost_index %d  seg_ind %d\n", itrack, node_cost_ind, node_seg_ind);

		if ((node_direction == direction || node_direction == BI_DIRECTION) &&
			    node_seg_ind == seg_index){
			/* found first track that has the specified segment index and goes in the desired direction */
			result = node_ind;
			break;
		}
	}

	return result;
}


/* allocates space for cost map entries */
static void alloc_cost_map(int num_segments){
	vector<Cost_Entry> ny_entries( ny+2, Cost_Entry() );
	vector< vector<Cost_Entry> > nx_entries( nx+2, ny_entries );
	vector< vector< vector<Cost_Entry> > > segment_entries( num_segments, nx_entries );
	f_cost_map.assign( 2, segment_entries );
}


/* frees the cost map. duh. */
static void free_cost_map(){
	f_cost_map.clear();
}


/* runs BFS from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
   to that pin is stored is added to an entry in the bfs_cost_map */
static void run_bfs(int start_node_ind, int start_x, int start_y, t_bfs_cost_map &bfs_cost_map){
/*
	TODO: need structure and function for priority queue
*/

	/* a list of boolean flags (one for each rr node) to figure out if a certain node has already been expanded */
	vector<bool> node_expanded( num_rr_nodes, false );
	/* for each node keep a list of the cost with which that node has been visited (used to determine whether to push
	   a candidate node onto the BFS expansion queue */
	vector<float> node_visited_costs( num_rr_nodes, -1.0 );
	/* a priority queue for BFS expansion */
	priority_queue<BFS_Queue_Entry> pq;

	/* first entry has no upstream delay or congestion */
	BFS_Queue_Entry first_entry(start_node_ind, UNDEFINED, 0, 0, 0);

	pq.push( first_entry );

	/* now do BFS */
	while( !pq.empty() ){
		BFS_Queue_Entry current = pq.top();
		pq.pop();

		int node_ind = current.rr_node_ind;

		/* check that we haven't already expanded from this node */
		if (node_expanded[node_ind]){
			continue;
		}

		//printf("expanded %d\n", node_ind);

		/* if this node is an ipin record its congestion/delay in the bfs_cost_map */
		if (rr_node[node_ind].type == IPIN){
			int ipin_x = rr_node[node_ind].get_xlow();
			int ipin_y = rr_node[node_ind].get_ylow();

			if (ipin_x >= start_x && ipin_y >= start_y){
				int delta_x = ipin_x - start_x;
				int delta_y = ipin_y - start_y;

				//printf("\t%d %d\n", delta_x, delta_y);

				bfs_cost_map[delta_x][delta_y].add_cost_entry(current.delay, current.congestion_upstream);

				/* no need to expand to sink nodes */
				//continue;
			}
		}

		expand_bfs_neighbours(current, node_visited_costs, node_expanded, pq);
		node_expanded[node_ind] = true;
	}
}


/* iterates over the children of the specified node and selectively pushes them onto the priority queue */
static void expand_bfs_neighbours(BFS_Queue_Entry parent_entry, vector<float> &node_visited_costs, vector<bool> &node_expanded, priority_queue<BFS_Queue_Entry> &pq){

	int parent_ind = parent_entry.rr_node_ind;

	RR_Node &parent_node = rr_node[parent_ind];

	for (int iedge = 0; iedge < parent_node.get_num_edges(); iedge++){
		int child_node_ind = parent_node.edges[iedge];
		int switch_ind = parent_node.switches[iedge];

		/* skip this child if it has already been expanded from */
		if (node_expanded[child_node_ind]){
			continue;
		}

		BFS_Queue_Entry child_entry(child_node_ind, switch_ind, parent_entry.delay,
		                             parent_entry.R_upstream, parent_entry.congestion_upstream);

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


/* sets the lookahead cost map entries based on representative cost entries from bfs_cost_map */
static void set_lookahead_map_costs(int segment_index, e_rr_type chan_type, t_bfs_cost_map &bfs_cost_map){
	int chan_index = 0;
	if (chan_type == CHANY){
		chan_index = 1;
	}

	/* set the lookahead cost map entries with a representative cost entry from bfs_cost_map */
	for (unsigned ix = 0; ix < bfs_cost_map.size(); ix++){
		for (unsigned iy = 0; iy < bfs_cost_map[ix].size(); iy++){
			
			BFS_Cost_Entry &bfs_cost_entry = bfs_cost_map[ix][iy];

			//printf("%d %d  %f\n", ix, iy, bfs_cost_entry.get_representative_cost_entry());

			f_cost_map[chan_index][segment_index][ix][iy] = bfs_cost_entry.get_representative_cost_entry();
		}
	}
}


/* fills in missing lookahead map entries by copying the cost of the closest valid entry */
static void fill_in_missing_lookahead_entries(int segment_index, e_rr_type chan_type){
	int chan_index = 0;
	if (chan_type == CHANY){
		chan_index = 1;
	}

	//TODO: may need to set (0,0) entry manually to avoid ram block weirdness

	/* find missing cost entries and fill them in by copying a nearby cost entry */
	for (unsigned ix = 0; ix < f_cost_map[chan_index][segment_index].size(); ix++){
		for (unsigned iy = 0; iy < f_cost_map[chan_index][segment_index][ix].size(); iy++){
			Cost_Entry cost_entry = f_cost_map[chan_index][segment_index][ix][iy];

			if (cost_entry.delay < 0 && cost_entry.congestion < 0){
				//if (ix == 0 && iy == 0){
				//	Cost_Entry entry(0, 0);
				//	f_cost_map[chan_index][segment_index][ix][iy] = entry;
				//} else {
					Cost_Entry copied_entry = get_nearby_cost_entry(ix, iy, segment_index, chan_index);
					f_cost_map[chan_index][segment_index][ix][iy] = copied_entry;
				//}

			}
		}
	}
}


/* returns a cost entry in the f_cost_map that is near the specified coordinates (and preferably towards (0,0)) */
static Cost_Entry get_nearby_cost_entry(int x, int y, int segment_index, int chan_index){
	/* compute the slope from x,y to 0,0 and then move towards 0,0 by one unit to get the coordinates
	   of the cost entry to be copied */

	assert(x > 0 || y > 0);

	float slope;
	if (x == 0){
		slope = 1e12;	//just a really large number
	} else if (y == 0){
		slope = 1e-12;	//just a really small number		
	} else {
		slope = (float)y/(float)x;
	}

	int copy_x, copy_y;
	if (slope >= 1.0){
		copy_y = y-1;
		copy_x = nint( (float)y / slope );
	} else {
		copy_x = x-1;
		copy_y = nint( (float)x * slope );
	}

	//printf("%d %d  %f  %d %d\n", x, y, slope, copy_x, copy_y);
	assert(copy_x > 0 || copy_y > 0);

	Cost_Entry copy_entry = f_cost_map[chan_index][segment_index][copy_x][copy_y];

	/* if the entry to be copied is also empty, recurse */
	if (copy_entry.delay < 0 && copy_entry.congestion < 0){
		copy_entry = get_nearby_cost_entry(copy_x, copy_y, segment_index, chan_index);
	}

	return copy_entry;
}

