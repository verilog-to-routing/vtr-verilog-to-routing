#include <cstdio>
#include <cstring>
#include <cmath>
#include <time.h>
using namespace std;

#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_log.h"
#include "vtr_util.h"

#include "vpr_types.h"
#include "globals.h"
#include "place_and_route.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "route_export.h"
#include "rr_graph.h"
#include "timing_place_lookup.h"
#include "read_xml_arch_file.h"
#include "netlist.h"
#include "ReadOptions.h"
#include "atom_netlist.h"

/*this file contains routines that generate the array containing*/
/*the delays between blocks, this is used in the timing driven  */
/*placement routines */

/*To compute delay between blocks we place temporary blocks at */
/*different locations in the FPGA and route nets  between      */
/*the blocks.  From this procedure we generate a lookup table  */
/*which tells us the delay between different locations in      */
/*the FPGA */

/*Note: these routines assume that there is a uniform and even */
/*distribution of the different wire segments. If this is not  */
/*the case, then this lookup table will be off */

/*Note: This code removes all heterogeneous types and creates an
 artificial 1x1 tile.  A good lookup for heterogeniety 
 requires more research */

#define NET_COUNT 1		/*we only use one net in these routines,   */
/*it is repeatedly routed and ripped up    */
/*to compute delays between different      */
/*locations, this value should not change  */
#define NET_USED 0		/*we use net at location zero of the net    */
/*structure                                 */
#define NET_USED_SOURCE_BLOCK 0	/*net.block[0] is source cluster_ctx.blocks */
#define NET_USED_SINK_BLOCK 1	/*net.block[1] is sink cluster_ctx.blocks */
#define SOURCE_BLOCK 0		/*cluster_ctx.blocks[0] is source */
#define SINK_BLOCK 1		/*cluster_ctx.blocks[1] is sink */

#define BLOCK_COUNT 2		/*use 2 blocks to compute delay between  */
/*the various FPGA locations             */
/*do not change this number unless you   */
/*really know what you are doing, it is  */
/*assumed that the net only connects to  */
/*two blocks */

#define NUM_TYPES_USED 3	/* number of types used in look up */

#define DEBUG_TIMING_PLACE_LOOKUP	/*initialize arrays to known state */

/*the delta arrays are used to contain the best case routing delay */
/*between different locations on the FPGA. */

static vtr::Matrix<float> f_delta_io_to_clb;
static vtr::Matrix<float> f_delta_clb_to_clb;
static vtr::Matrix<float> f_delta_clb_to_io;
static vtr::Matrix<float> f_delta_io_to_io;

/* I could have allocated these as local variables, and passed them all */
/* around, but was too lazy, since this is a small file, it should not  */
/* be a big problem */

static std::vector<std::vector<float>> f_net_delay;
static float *pin_criticality;
static int *sink_order;
static t_rt_node **rt_node_of_sink;
static t_type_ptr IO_TYPE_BACKUP;
static t_type_ptr EMPTY_TYPE_BACKUP;
static t_type_ptr FILL_TYPE_BACKUP;
static t_type_descriptor dummy_type_descriptors[NUM_TYPES_USED];
static t_type_descriptor *type_descriptors_backup;
static vtr::Matrix<t_grid_tile> grid_backup;
static int num_types_backup;

vtr::t_ivec **clb_opins_used_locally;

/*** Function Prototypes *****/

static void alloc_delay_lookup_netlists();
static void free_delay_lookup_netlists();

static void alloc_atom_netlist();

static void alloc_vnet(void);

static void alloc_block(void);
static void free_block(void);

static void load_simplified_device(void);
static void restore_original_device(void);

static void alloc_and_assign_internal_structures(t_block **original_block,
		int *original_num_blocks, t_netlist& original_clbs_nlist, AtomNetlist& original_atom_nlist);

static void free_and_reset_internal_structures(t_block *original_block,
		int original_num_blocks, t_netlist& original_clbs_nlist, AtomNetlist& original_atom_nlist);

static void setup_chan_width(t_router_opts router_opts,
		t_chan_width_dist chan_width_dist);

static void alloc_routing_structs(t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		const t_direct_inf *directs, 
		const int num_directs);

static void free_routing_structs();

static void assign_locations(t_type_ptr source_type, int source_x_loc,
		int source_y_loc, int source_z_loc, t_type_ptr sink_type,
		int sink_x_loc, int sink_y_loc, int sink_z_loc);

static float assign_blocks_and_route_net(t_type_ptr source_type,
		int source_x_loc, int source_y_loc, t_type_ptr sink_type,
		int sink_x_loc, int sink_y_loc, t_router_opts router_opts);

static void alloc_delta_arrays(void);

static void free_delta_arrays(void);

static void generic_compute_matrix(vtr::Matrix<float>& matrix, t_type_ptr source_type,
		t_type_ptr sink_type, int source_x, int source_y, int start_x,
		int end_x, int start_y, int end_y, t_router_opts router_opts);

static void compute_delta_clb_to_clb(t_router_opts router_opts, int longest_length);

static void compute_delta_io_to_clb(t_router_opts router_opts);

static void compute_delta_clb_to_io(t_router_opts router_opts);

static void compute_delta_io_to_io(t_router_opts router_opts);

static void compute_delta_arrays(t_router_opts router_opts, int longest_length);

static int get_best_pin(enum e_pin_type pintype, t_type_ptr type);

static int get_longest_segment_length(
		t_det_routing_arch det_routing_arch, t_segment_inf * segment_inf);
static void reset_placement(void);

static void print_delta_delays_echo(const char* filename);
static void print_matrix(std::string filename, const vtr::Matrix<float>& array_to_print);
/**************************************/

static int get_best_pin(enum e_pin_type pintype, t_type_ptr type) {

	/*This function tries to identify the best pin class to hook up
     * for delay calculation.  The assumption is that we should pick
     * the pin class with the largest number of pins. This makes
     * sense, since it ensures we pick commonly used pins, and 
     * removes order dependance on how the pins are specified 
     * in the architecture (except in the case were the two largest pin classes
     * of a particular pintype have the same number of pins, in which case the 
     * first pin class is used).
     * 
     * Also, global pins are not hooked up to the temporary net */

	int i, currpin, best_class_num_pins, best_class;
    
    best_class = -1;
    best_class_num_pins = 0;
	currpin = 0;
	for (i = 0; i < type->num_class; i++) {
		if (type->class_inf[i].type == pintype && !type->is_global_pin[currpin] &&
                type->class_inf[i].num_pins > best_class_num_pins) {
            //Save the best class
            best_class_num_pins = type->class_inf[i].num_pins;
            best_class = i;
        }
		else
			currpin += type->class_inf[i].num_pins;
	}

    VTR_ASSERT(best_class >= 0 && best_class < type->num_class);
    return (type->class_inf[best_class].pinlist[0]);
}

/**************************************/
static int get_longest_segment_length(
		t_det_routing_arch det_routing_arch, t_segment_inf * segment_inf) {

	int i, length;

	length = 0;
	for (i = 0; i < det_routing_arch.num_segment; i++) {
		if (segment_inf[i].length > length)
			length = segment_inf[i].length;
	}
	return (length);
}

/**************************************/
static void alloc_delay_lookup_netlists() {
    //Create the new temporary Atom netlist
    alloc_atom_netlist(); 

    //Create the new temporary CLB netlist
	alloc_vnet();
	alloc_block();
}

static void free_delay_lookup_netlists() {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    free_global_nlist_net(&cluster_ctx.clbs_nlist);
    free_block();
}

static void free_block() {
	//Free temporary CLB blocks data structure
    auto& cluster_ctx = g_vpr_ctx.clustering();

	for (int i = 0; i < BLOCK_COUNT; i++) {
		free(cluster_ctx.blocks[i].name);
		free(cluster_ctx.blocks[i].nets);
	}
	free(cluster_ctx.blocks);
}


static void alloc_atom_netlist() {
}

static void alloc_vnet(){

	int i, len;

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

	cluster_ctx.clbs_nlist.net.resize(NET_COUNT);
	for(i = 0; i < NET_COUNT; i++){
		len = strlen("TEMP_NET");
		cluster_ctx.clbs_nlist.net[i].name = (char *) vtr::malloc((len + 1) * sizeof(char));
		cluster_ctx.clbs_nlist.net[i].is_routed = false;
		cluster_ctx.clbs_nlist.net[i].is_fixed = false;
		cluster_ctx.clbs_nlist.net[i].is_global = false;
		strcpy(cluster_ctx.clbs_nlist.net[NET_USED].name, "TEMP_NET");
	
		cluster_ctx.clbs_nlist.net[i].pins.resize(BLOCK_COUNT);
		cluster_ctx.clbs_nlist.net[i].pins[NET_USED_SOURCE_BLOCK].block = NET_USED_SOURCE_BLOCK;
		cluster_ctx.clbs_nlist.net[i].pins[NET_USED_SINK_BLOCK].block = NET_USED_SINK_BLOCK;

		/*the values for nodes[].block_pin are assigned in assign_blocks_and_route_net */
	}

}

/**************************************/
static void alloc_block(void) {

	/*allocates cluster_ctx.blocks structure, and assigns values to known parameters */
	/*type and x,y fields are left undefined at this stage since they  */
	/*are not known until we start moving blocks through the clb array */

	int ix_b, ix_p, len, i;
	int max_pins;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	max_pins = 0;
	for (i = 0; i < NUM_TYPES_USED; i++) {
		max_pins = max(max_pins, device_ctx.block_types[i].num_pins);
	}

	cluster_ctx.num_blocks = BLOCK_COUNT;
	cluster_ctx.blocks = (t_block *) vtr::malloc(cluster_ctx.num_blocks * sizeof(t_block));

    place_ctx.block_locs.resize(BLOCK_COUNT);

	for (ix_b = 0; ix_b < BLOCK_COUNT; ix_b++) {
		len = strlen("TEMP_BLOCK");
		cluster_ctx.blocks[ix_b].name = (char *) vtr::malloc((len + 1) * sizeof(char));
		strcpy(cluster_ctx.blocks[ix_b].name, "TEMP_BLOCK");

		cluster_ctx.blocks[ix_b].nets = (int *) vtr::malloc(max_pins * sizeof(int));
		cluster_ctx.blocks[ix_b].nets[0] = 0;
		for (ix_p = 1; ix_p < max_pins; ix_p++)
			cluster_ctx.blocks[ix_b].nets[ix_p] = OPEN;
	}
}

/**************************************/
static void load_simplified_device(void) {
	int i, j;

    auto& device_ctx = g_vpr_ctx.mutable_device();

	/* Backup original globals */
	EMPTY_TYPE_BACKUP = device_ctx.EMPTY_TYPE;
	IO_TYPE_BACKUP = device_ctx.IO_TYPE;
	FILL_TYPE_BACKUP = device_ctx.FILL_TYPE;
	type_descriptors_backup = device_ctx.block_types;
	num_types_backup = device_ctx.num_block_types;
	device_ctx.num_block_types = NUM_TYPES_USED;

	/* Fill in homogeneous core type info */
	dummy_type_descriptors[0] = *device_ctx.EMPTY_TYPE;
	dummy_type_descriptors[0].index = 0;
	dummy_type_descriptors[1] = *device_ctx.IO_TYPE;
	dummy_type_descriptors[1].index = 1;
	dummy_type_descriptors[2] = *device_ctx.FILL_TYPE;
	dummy_type_descriptors[2].index = 2;
	device_ctx.block_types = dummy_type_descriptors;
	device_ctx.EMPTY_TYPE = &dummy_type_descriptors[0];
	device_ctx.IO_TYPE = &dummy_type_descriptors[1];
	device_ctx.FILL_TYPE = &dummy_type_descriptors[2];

	/* Fill in homogeneous core grid info */
    std::swap(device_ctx.grid, grid_backup);
    size_t nx = device_ctx.nx;
    size_t ny = device_ctx.ny;
	device_ctx.grid.resize({nx + 2, ny + 2});
	for (i = 0; i < device_ctx.nx + 2; i++) {
		for (j = 0; j < device_ctx.ny + 2; j++) {
			if ((i == 0 && j == 0) || (i == device_ctx.nx + 1 && j == 0)
					|| (i == 0 && j == device_ctx.ny + 1)
					|| (i == device_ctx.nx + 1 && j == device_ctx.ny + 1)) {
				device_ctx.grid[i][j].type = device_ctx.EMPTY_TYPE;
			} else if (i == 0 || i == device_ctx.nx + 1 || j == 0 || j == device_ctx.ny + 1) {
				device_ctx.grid[i][j].type = device_ctx.IO_TYPE;
			} else {
				device_ctx.grid[i][j].type = device_ctx.FILL_TYPE;
			}
			device_ctx.grid[i][j].width_offset = 0;
			device_ctx.grid[i][j].height_offset = 0;
		}
	}
}
static void restore_original_device(void) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

	/* restore previous globals */
	device_ctx.IO_TYPE = IO_TYPE_BACKUP;
	device_ctx.EMPTY_TYPE = EMPTY_TYPE_BACKUP;
	device_ctx.FILL_TYPE = FILL_TYPE_BACKUP;
	device_ctx.block_types = type_descriptors_backup;
	device_ctx.num_block_types = num_types_backup;

	/* free allocated data */
    std::swap(grid_backup, device_ctx.grid);
    grid_backup.clear();
}

/**************************************/
static void reset_placement(void) {
	int i, j, k;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	for (i = 0; i <= device_ctx.nx + 1; i++) {
		for (j = 0; j <= device_ctx.ny + 1; j++) {
			for (k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
				if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK) {
					place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK;
				}
			}
			place_ctx.grid_blocks[i][j].usage = 0;
		}
	}
}

/**************************************/
static void alloc_and_assign_internal_structures(t_block **original_block,
		int *original_num_blocks, t_netlist& original_clbs_nlist, AtomNetlist& original_atom_nlist) {

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    //Save the original atom netlist
    std::swap(original_atom_nlist, atom_ctx.nlist);

    //Save the original CLB nets
    std::swap(cluster_ctx.clbs_nlist, original_clbs_nlist);

    //Save the original CLB blocks
	*original_block = cluster_ctx.blocks;
	*original_num_blocks = cluster_ctx.num_blocks;

    //Create the new temporary atom and CLB netlists
    alloc_delay_lookup_netlists();

	/* [0..num_nets-1][1..num_pins-1] */
	f_net_delay.resize(NET_COUNT);
    for(auto& net_delay : f_net_delay) {
        net_delay.resize(BLOCK_COUNT);
    }

	reset_placement();
}

/**************************************/
static void free_and_reset_internal_structures(t_block *original_block, int original_num_blocks, t_netlist& original_clbs_nlist, AtomNetlist& original_atom_nlist) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    /*reset gloabal data structures to the state that they were in before these */
	/*lookup computation routines were called */
    free_delay_lookup_netlists();

    //Revert the atom netlist back to original
    std::swap(original_atom_nlist, atom_ctx.nlist);

    //Revert the CLBs netlist back to original
    std::swap(original_clbs_nlist, cluster_ctx.clbs_nlist);

    //Revert to the original blocks
	cluster_ctx.blocks = original_block;
	cluster_ctx.num_blocks = original_num_blocks;

    f_net_delay.clear();
}

/**************************************/
static void setup_chan_width(t_router_opts router_opts,
		t_chan_width_dist chan_width_dist) {
	/*we give plenty of tracks, this increases routability for the */
	/*lookup table generation */

	int width_fac;

	if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
        //We use the number of pins on the FILL_TYPE, since
        //while building the placement timing model we use a
        //uniformly filled FPGA architecture.
        auto& device_ctx = g_vpr_ctx.device();
		width_fac = 4 * device_ctx.FILL_TYPE->num_pins; 
        /*this is 2x the value that binary search starts */
        /*this should be enough to allow most pins to   */
        /*connect to tracks in the architecture */
    } else {
		width_fac = router_opts.fixed_channel_width;
    }

	init_chan(width_fac, chan_width_dist);
}

/**************************************/
static void alloc_routing_structs(t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		const t_direct_inf *directs, 
		const int num_directs) {

	int bb_factor;
	int warnings;
	t_graph_type graph_type;

    auto& device_ctx = g_vpr_ctx.mutable_device();

	/*calls routines that set up routing resource graph and associated structures */

	/*must set up dummy blocks for the first pass through to setup locally used opins */
	/* Only one block per tile */
	assign_locations(device_ctx.FILL_TYPE, 1, 1, 0, device_ctx.FILL_TYPE, device_ctx.nx, device_ctx.ny, 0);

	clb_opins_used_locally = alloc_route_structs();

	free_rr_graph();

	if (router_opts.route_type == GLOBAL) {
		graph_type = GRAPH_GLOBAL;
	} else {
		graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ?
				GRAPH_BIDIR : GRAPH_UNIDIR);
	}

	build_rr_graph(graph_type, device_ctx.num_block_types, dummy_type_descriptors, device_ctx.nx, device_ctx.ny, device_ctx.grid,
			&device_ctx.chan_width, det_routing_arch->switch_block_type,
			det_routing_arch->Fs, det_routing_arch->switchblocks,
			det_routing_arch->num_segment,
			device_ctx.num_arch_switches, segment_inf,
			det_routing_arch->global_route_switch,
			det_routing_arch->delayless_switch, 
			det_routing_arch->wire_to_arch_ipin_switch,
			router_opts.base_cost_type,
			router_opts.trim_empty_channels,
			router_opts.trim_obs_channels,
			directs, num_directs, /* Do send in direct connections because we care about some direct 
                        connections, such as directlinks between adjacent blocks. We don't need
                        to worry about special cases such as carry chains, since we do
                        delay estimation between the most numerous pins on the block,
                        which should not be carry chain pins. */
			true, 
			det_routing_arch->dump_rr_structs_file,
			&det_routing_arch->wire_to_rr_ipin_switch,
			&device_ctx.num_rr_switches,
			&warnings);

	alloc_and_load_rr_node_route_structs();

	alloc_timing_driven_route_structs(&pin_criticality, &sink_order,
			&rt_node_of_sink);

	bb_factor = device_ctx.nx + device_ctx.ny; /*set it to a huge value */
	init_route_structs(bb_factor);
}

/**************************************/
static void free_routing_structs() {
	int i;

    auto& cluster_ctx = g_vpr_ctx.clustering();

	free_rr_graph();

	free_rr_node_route_structs();
	free_route_structs();
	free_trace_structs();

	free_timing_driven_route_structs(pin_criticality, sink_order,
			rt_node_of_sink);
	
	if (clb_opins_used_locally != NULL) {
		for (i = 0; i < cluster_ctx.num_blocks; i++) {
			free_ivec_vector(clb_opins_used_locally[i], 0,
					cluster_ctx.blocks[i].type->num_class - 1);
		}
		free(clb_opins_used_locally);
		clb_opins_used_locally = NULL;
	}
}

/**************************************/
static void assign_locations(t_type_ptr source_type, int source_x_loc,
		int source_y_loc, int source_z_loc, t_type_ptr sink_type,
		int sink_x_loc, int sink_y_loc, int sink_z_loc) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	/*all routing occurs between block 0 (source) and block 1 (sink) */
	cluster_ctx.blocks[SOURCE_BLOCK].type = source_type;
	cluster_ctx.blocks[SOURCE_BLOCK].pb = nullptr;
	cluster_ctx.blocks[SOURCE_BLOCK].pb_route = nullptr;
	place_ctx.block_locs[SOURCE_BLOCK].x = source_x_loc;
	place_ctx.block_locs[SOURCE_BLOCK].y = source_y_loc;
	place_ctx.block_locs[SOURCE_BLOCK].z = source_z_loc;

	cluster_ctx.blocks[SINK_BLOCK].type = sink_type;
	cluster_ctx.blocks[SINK_BLOCK].pb = nullptr;
	cluster_ctx.blocks[SINK_BLOCK].pb_route = nullptr;
	place_ctx.block_locs[SINK_BLOCK].x = sink_x_loc;
	place_ctx.block_locs[SINK_BLOCK].y = sink_y_loc;
	place_ctx.block_locs[SINK_BLOCK].z = sink_z_loc;

	place_ctx.grid_blocks[source_x_loc][source_y_loc].blocks[source_z_loc] = SOURCE_BLOCK;
	place_ctx.grid_blocks[sink_x_loc][sink_y_loc].blocks[sink_z_loc] = SINK_BLOCK;

	cluster_ctx.clbs_nlist.net[NET_USED].pins[NET_USED_SOURCE_BLOCK].block_pin = get_best_pin(DRIVER, cluster_ctx.blocks[SOURCE_BLOCK].type);
	cluster_ctx.clbs_nlist.net[NET_USED].pins[NET_USED_SINK_BLOCK].block_pin = get_best_pin(RECEIVER, cluster_ctx.blocks[SINK_BLOCK].type);

	place_ctx.grid_blocks[source_x_loc][source_y_loc].usage += 1;
	place_ctx.grid_blocks[sink_x_loc][sink_y_loc].usage += 1;

}

/**************************************/
static float assign_blocks_and_route_net(t_type_ptr source_type,
		int source_x_loc, int source_y_loc, t_type_ptr sink_type,
		int sink_x_loc, int sink_y_loc, t_router_opts router_opts) {

	/*places blocks at the specified locations, and routes a net between them */
	/*returns the delay of this net */

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	/* Only one block per tile */
	int source_z_loc = 0;
	int sink_z_loc = 0;

	float net_delay_value = IMPOSSIBLE; /*set to known value for debug purposes */

	assign_locations(source_type, source_x_loc, source_y_loc, source_z_loc,
			sink_type, sink_x_loc, sink_y_loc, sink_z_loc);

	load_net_rr_terminals(device_ctx.rr_node_indices);

	int itry = 1;
	float pres_fac = 0.0; /* ignore congestion */

	CBRR dummy_connections_inf;
	dummy_connections_inf.prepare_routing_for_net(NET_USED);

    IntraLbPbPinLookup dummy_pb_pin_lookup(device_ctx.block_types, device_ctx.num_block_types);

	timing_driven_route_net(NET_USED, itry, pres_fac,
			router_opts.max_criticality, router_opts.criticality_exp,
			router_opts.astar_fac, router_opts.bend_cost, 
			dummy_connections_inf,
			pin_criticality, 
            router_opts.min_incremental_reroute_fanout, rt_node_of_sink, 
			f_net_delay[NET_USED].data(),
            dummy_pb_pin_lookup,
            nullptr); //We pass in no timing info, indicating we want a min-delay routing

	net_delay_value = f_net_delay[NET_USED][NET_USED_SINK_BLOCK];

	place_ctx.grid_blocks[source_x_loc][source_y_loc].usage = 0;
	place_ctx.grid_blocks[source_x_loc][source_y_loc].blocks[source_z_loc] = EMPTY_BLOCK;
	place_ctx.grid_blocks[sink_x_loc][sink_y_loc].usage = 0;
	place_ctx.grid_blocks[sink_x_loc][sink_y_loc].blocks[sink_z_loc] = EMPTY_BLOCK;

	return (net_delay_value);
}

/**************************************/
static void alloc_delta_arrays(void) {
    auto& device_ctx = g_vpr_ctx.device();

	/*initialize all of the array locations to -1 */
    size_t nx = device_ctx.nx;
    size_t ny = device_ctx.ny;
	f_delta_clb_to_clb.resize({nx, ny}, IMPOSSIBLE);
	f_delta_io_to_clb.resize({nx+1, ny+1}, IMPOSSIBLE);
	f_delta_clb_to_io.resize({nx+1, ny+1}, IMPOSSIBLE);
	f_delta_io_to_io.resize({nx+2, ny+2}, IMPOSSIBLE);
}

/**************************************/
static void free_delta_arrays(void) {
    //Reclaim memory
    f_delta_clb_to_clb.clear();
    f_delta_io_to_clb.clear();
    f_delta_clb_to_io.clear();
    f_delta_io_to_io.clear();
}

/**************************************/
static void generic_compute_matrix(vtr::Matrix<float>& matrix, t_type_ptr source_type,
		t_type_ptr sink_type, int source_x, int source_y, int start_x,
		int end_x, int start_y, int end_y, t_router_opts router_opts) {

	int delta_x, delta_y;
	int sink_x, sink_y;

	for (sink_x = start_x; sink_x <= end_x; sink_x++) {
		for (sink_y = start_y; sink_y <= end_y; sink_y++) {
			delta_x = abs(sink_x - source_x);
			delta_y = abs(sink_y - source_y);

			if (delta_x == 0 && delta_y == 0)
				continue; /*do not compute distance from a block to itself     */
			/*if a value is desired, pre-assign it somewhere else */

			matrix[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);
		}
	}
}

/**************************************/
static void compute_delta_clb_to_clb(t_router_opts router_opts, int longest_length) {

	/*this routine must compute delay values in a slightly different way than the */
	/*other compute routines. We cannot use a location close to the edge as the  */
	/*source location for the majority of the delay computations  because this   */
	/*would give gradually increasing delay values. To avoid this from happening */
	/*a clb that is at least longest_length away from an edge should be chosen   */
	/*as a source , if longest_length is more than 0.5 of the total size then    */
	/*choose a CLB at the center as the source CLB */

	int source_x, source_y, sink_x, sink_y;
	int start_x, start_y, end_x, end_y;
	int delta_x, delta_y;
	t_type_ptr source_type, sink_type;
    auto& device_ctx = g_vpr_ctx.device();

	source_type = device_ctx.FILL_TYPE;
	sink_type = device_ctx.FILL_TYPE;

	if (longest_length < 0.5 * (device_ctx.nx)) {
		start_x = longest_length;
	} else {
		start_x = (int) (0.5 * device_ctx.nx);
	}
	end_x = device_ctx.nx;
	source_x = start_x;

	if (longest_length < 0.5 * (device_ctx.ny)) {
		start_y = longest_length;
	} else {
		start_y = (int) (0.5 * device_ctx.ny);
	}
	end_y = device_ctx.ny;
	source_y = start_y;

	/*don't put the sink all the way to the corner, until it is necessary */
	for (sink_x = start_x; sink_x <= end_x - 1; sink_x++) {
		for (sink_y = start_y; sink_y <= end_y - 1; sink_y++) {
			delta_x = abs(sink_x - source_x);
			delta_y = abs(sink_y - source_y);

			if (delta_x == 0 && delta_y == 0) {
				f_delta_clb_to_clb[delta_x][delta_y] = 0.0;
				continue;
			}
			f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);
		}

	}

	sink_x = end_x - 1;
	sink_y = end_y - 1;

	for (source_x = start_x - 1; source_x >= 1; source_x--) {
		for (source_y = start_y; source_y <= end_y - 1; source_y++) {
			delta_x = abs(sink_x - source_x);
			delta_y = abs(sink_y - source_y);

			f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);
		}
	}

	for (source_x = 1; source_x <= end_x - 1; source_x++) {
		for (source_y = 1; source_y < start_y; source_y++) {
			delta_x = abs(sink_x - source_x);
			delta_y = abs(sink_y - source_y);

			f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);
		}
	}

	/*now move sink into the top right corner */
	sink_x = end_x;
	sink_y = end_y;
	source_x = 1;
	for (source_y = 1; source_y <= end_y; source_y++) {
		delta_x = abs(sink_x - source_x);
		delta_y = abs(sink_y - source_y);
		f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);

	}

	sink_x = end_x;
	sink_y = end_y;
	source_y = 1;
	for (source_x = 1; source_x <= end_x; source_x++) {
		delta_x = abs(sink_x - source_x);
		delta_y = abs(sink_y - source_y);

		f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);
	}
}

/**************************************/
static void compute_delta_io_to_clb(t_router_opts router_opts) {
	int source_x, source_y;
	int start_x, start_y, end_x, end_y;
	t_type_ptr source_type, sink_type;
    auto& device_ctx = g_vpr_ctx.device();

	source_type = device_ctx.IO_TYPE;
	sink_type = device_ctx.FILL_TYPE;

	f_delta_io_to_clb[0][0] = IMPOSSIBLE;
	f_delta_io_to_clb[device_ctx.nx][device_ctx.ny] = IMPOSSIBLE;

	source_x = 0;
	source_y = 1;

	start_x = 1;
	end_x = device_ctx.nx;
	start_y = 1;
	end_y = device_ctx.ny;
	generic_compute_matrix(f_delta_io_to_clb, source_type, sink_type, source_x,
			source_y, start_x, end_x, start_y, end_y, router_opts);

	source_x = 1;
	source_y = 0;

	start_x = 1;
	end_x = 1;
	start_y = 1;
	end_y = device_ctx.ny;
	generic_compute_matrix(f_delta_io_to_clb, source_type, sink_type, source_x,
			source_y, start_x, end_x, start_y, end_y, router_opts);

	start_x = 1;
	end_x = device_ctx.nx;
	start_y = device_ctx.ny;
	end_y = device_ctx.ny;
	generic_compute_matrix(f_delta_io_to_clb, source_type, sink_type, source_x,
			source_y, start_x, end_x, start_y, end_y, router_opts);
}

/**************************************/
static void compute_delta_clb_to_io(t_router_opts router_opts) {
	int source_x, source_y, sink_x, sink_y;
	int delta_x, delta_y;
	t_type_ptr source_type, sink_type;
    auto& device_ctx = g_vpr_ctx.device();

	source_type = device_ctx.FILL_TYPE;
	sink_type = device_ctx.IO_TYPE;

	f_delta_clb_to_io[0][0] = IMPOSSIBLE;
	f_delta_clb_to_io[device_ctx.nx][device_ctx.ny] = IMPOSSIBLE;

	sink_x = 0;
	sink_y = 1;
	for (source_x = 1; source_x <= device_ctx.nx; source_x++) {
		for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
			delta_x = abs(source_x - sink_x);
			delta_y = abs(source_y - sink_y);

			f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);
		}
	}

	sink_x = 1;
	sink_y = 0;
	source_x = 1;
	delta_x = abs(source_x - sink_x);
	for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
		delta_y = abs(source_y - sink_y);
		f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);
	}

	sink_x = 1;
	sink_y = 0;
	source_y = device_ctx.ny;
	delta_y = abs(source_y - sink_y);
	for (source_x = 2; source_x <= device_ctx.nx; source_x++) {
		delta_x = abs(source_x - sink_x);
		f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);
	}
}

/**************************************/
static void compute_delta_io_to_io(t_router_opts router_opts) {
	int source_x, source_y, sink_x, sink_y;
	int delta_x, delta_y;
	t_type_ptr source_type, sink_type;
    auto& device_ctx = g_vpr_ctx.device();

	source_type = device_ctx.IO_TYPE;
	sink_type = device_ctx.IO_TYPE;

	f_delta_io_to_io[0][0] = 0; /*delay to itself is 0 (this can happen) */
	f_delta_io_to_io[device_ctx.nx + 1][device_ctx.ny + 1] = IMPOSSIBLE;
	f_delta_io_to_io[0][device_ctx.ny] = IMPOSSIBLE;
	f_delta_io_to_io[device_ctx.nx][0] = IMPOSSIBLE;
	f_delta_io_to_io[device_ctx.nx][device_ctx.ny + 1] = IMPOSSIBLE;
	f_delta_io_to_io[device_ctx.nx + 1][device_ctx.ny] = IMPOSSIBLE;

	source_x = 0;
	source_y = 1;
	sink_x = 0;
	delta_x = abs(sink_x - source_x);

	for (sink_y = 2; sink_y <= device_ctx.ny; sink_y++) {
		delta_y = abs(sink_y - source_y);
		f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);

	}

	source_x = 0;
	source_y = 1;
	sink_x = device_ctx.nx + 1;
	delta_x = abs(sink_x - source_x);

	for (sink_y = 1; sink_y <= device_ctx.ny; sink_y++) {
		delta_y = abs(sink_y - source_y);
		f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);

	}

	source_x = 1;
	source_y = 0;
	sink_y = 0;
	delta_y = abs(sink_y - source_y);

	for (sink_x = 2; sink_x <= device_ctx.nx; sink_x++) {
		delta_x = abs(sink_x - source_x);
		f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);

	}

	source_x = 1;
	source_y = 0;
	sink_y = device_ctx.ny + 1;
	delta_y = abs(sink_y - source_y);

	for (sink_x = 1; sink_x <= device_ctx.nx; sink_x++) {
		delta_x = abs(sink_x - source_x);
		f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
				source_type, source_x, source_y, sink_type, sink_x, sink_y,
				router_opts);

	}

	source_x = 0;
	sink_y = device_ctx.ny + 1;
	for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
		for (sink_x = 1; sink_x <= device_ctx.nx; sink_x++) {
			delta_y = abs(source_y - sink_y);
			delta_x = abs(source_x - sink_x);
			f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(
					source_type, source_x, source_y, sink_type, sink_x, sink_y,
					router_opts);

		}
	}
}

/**************************************/
static void
print_matrix(std::string filename, const vtr::Matrix<float>& matrix) {
    FILE* f = vtr::fopen(filename.c_str(), "w");

    fprintf(f, "         ");
    for(size_t delta_x = matrix.begin_index(0); delta_x < matrix.end_index(0); ++delta_x) {
        fprintf(f, " %9zu", delta_x);
    }
    fprintf(f, "\n");

    for(size_t delta_y = matrix.begin_index(1); delta_y < matrix.end_index(1); ++delta_y) {
        fprintf(f, "%9zu", delta_y);
        for(size_t delta_x = matrix.begin_index(0); delta_x < matrix.end_index(0); ++delta_x) {
			fprintf(f, " %9.2e", matrix[delta_x][delta_y]);
        }
        fprintf(f, "\n");
    }

    vtr::fclose(f);
}

/**************************************/
static void compute_delta_arrays(t_router_opts router_opts, int longest_length) {

	vtr::printf_info("Computing delta_io_to_io lookup matrix, may take a few seconds, please wait...\n");
	compute_delta_io_to_io(router_opts);
	vtr::printf_info("Computing delta_io_to_clb lookup matrix, may take a few seconds, please wait...\n");
	compute_delta_io_to_clb(router_opts);
	vtr::printf_info("Computing delta_clb_to_io lookup matrix, may take a few seconds, please wait...\n");
	compute_delta_clb_to_io(router_opts);
	vtr::printf_info("Computing delta_clb_to_clb lookup matrix, may take a few seconds, please wait...\n");
	compute_delta_clb_to_clb(router_opts, longest_length);

    if(isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
    }
}

static void print_delta_delays_echo(const char* filename) {
	print_matrix(vtr::string_fmt(filename, "delta_clb_to_clb"), f_delta_clb_to_clb);
	print_matrix(vtr::string_fmt(filename, "delta_io_to_clb"), f_delta_io_to_clb);
	print_matrix(vtr::string_fmt(filename, "delta_clb_to_io"), f_delta_clb_to_io);
	print_matrix(vtr::string_fmt(filename, "delta_io_to_io"), f_delta_io_to_io);
}

/******* Globally Accessable Functions **********/

/**************************************/
void compute_delay_lookup_tables(t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_chan_width_dist chan_width_dist, const t_direct_inf *directs, 
		const int num_directs) {

	vtr::printf_info("\nStarting placement delay look-up...\n");
    clock_t begin = clock();

	/*the "real" nets in the circuit are. This is    */
	/*required because we are using the net structure */
	/*in these routines to find delays between blocks */
	t_netlist orig_clbs_nlist;
	t_block *original_blocks; /*same def as original_nets, but for cluster_ctx.blocks  */
	int original_num_blocks;
    AtomNetlist orig_atom_nlist;

	int longest_length;

	load_simplified_device();

	alloc_and_assign_internal_structures(&original_blocks, &original_num_blocks, orig_clbs_nlist, orig_atom_nlist);
	setup_chan_width(router_opts, chan_width_dist);

	alloc_routing_structs(router_opts, det_routing_arch, segment_inf,
			directs, num_directs);

	longest_length = get_longest_segment_length((*det_routing_arch), segment_inf);

	/*now setup and compute the actual arrays */
	alloc_delta_arrays();
	compute_delta_arrays(router_opts, longest_length);

	/*free all data structures that are no longer needed */
	free_routing_structs();

	restore_original_device();

	free_and_reset_internal_structures(original_blocks, original_num_blocks, orig_clbs_nlist, orig_atom_nlist);

    clock_t end = clock();

    float time = (float) (end - begin) / CLOCKS_PER_SEC;
	vtr::printf_info("Placement delay look-up took %g seconds\n", time);
}

/**************************************/
void free_place_lookup_structs(void) {

	free_delta_arrays();

}

float get_delta_io_to_clb(int delta_x, int delta_y) {
    return f_delta_io_to_clb[delta_x][delta_y];
}

float get_delta_clb_to_clb(int delta_x, int delta_y) {
    return f_delta_clb_to_clb[delta_x][delta_y];
}

float get_delta_clb_to_io(int delta_x, int delta_y) {
    return f_delta_clb_to_io[delta_x][delta_y];
}

float get_delta_io_to_io(int delta_x, int delta_y) {
    return f_delta_io_to_io[delta_x][delta_y];
}
