#include <cstdio>
#include <cstring>
#include <cmath>
#include <time.h>
#include <limits>
using namespace std;

#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_math.h"
#include "vtr_memory.h"

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
#include "echo_files.h"
#include "atom_netlist.h"
#include "rr_graph2.h"
#include "place_util.h"
// all functions in profiling:: namespace, which are only activated if PROFILE is defined
#include "route_profiling.h"

constexpr float UNINITIALIZED_DELTA = -1; //Indicates the delta delay value has not been calculated
constexpr float EMPTY_DELTA = -2; //Indicates delta delay from/to an EMPTY block
constexpr float IMPOSSIBLE_DELTA = std::numeric_limits<float>::infinity(); //Indicates there is no valid delta delay

/*To compute delay between blocks we calculate the delay between */
/*different nodes in the FPGA.  From this procedure we generate
 * a lookup table which tells us the delay between different locations in*/
/*the FPGA */

/*the delta arrays are used to contain the best case routing delay */
/*between different locations on the FPGA. */

static vtr::Matrix<float> f_delta_delay;

/*** Function Prototypes *****/
static void setup_chan_width(t_router_opts router_opts,
        t_chan_width_dist chan_width_dist);

static void alloc_routing_structs(t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        const t_direct_inf *directs,
        const int num_directs);

static void free_routing_structs();

static float route_connection_delay(int source_x_loc, int source_y_loc,
        int sink_x_loc, int sink_y_loc,
        t_router_opts router_opts);

static void alloc_delta_arrays();

static void free_delta_arrays();

static void generic_compute_matrix(vtr::Matrix<float>& matrix,
        int source_x, int source_y,
        int start_x, int start_y,
        int end_x, int end_y, t_router_opts router_opts);

static void compute_delta_delays(t_router_opts router_opts, size_t longest_length);

static void compute_delta_arrays(t_router_opts router_opts, int longest_length);

static bool verify_delta_delays();
static int get_best_class(enum e_pin_type pintype, t_type_ptr type);

static int get_longest_segment_length(
        t_det_routing_arch det_routing_arch, t_segment_inf * segment_inf);
static void reset_placement();

static void print_delta_delays_echo(const char* filename);

static void print_matrix(std::string filename, const vtr::Matrix<float>& array_to_print);

static void fix_empty_coordinates();

static float find_neightboring_average(vtr::Matrix<float> &matrix, int x, int y);

static bool calculate_delay(int source_node, int sink_node,
        float astar_fac, float bend_cost, float *net_delay);

static t_rt_node* setup_routing_resources_no_net(int source_node);

/******* Globally Accessible Functions **********/

void compute_delay_lookup_tables(t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        t_chan_width_dist chan_width_dist, const t_direct_inf *directs,
        const int num_directs) {

    VTR_LOG("\nStarting placement delay look-up...\n");
    clock_t begin = clock();

    int longest_length;

    reset_placement();

    setup_chan_width(router_opts, chan_width_dist);

    alloc_routing_structs(router_opts, det_routing_arch, segment_inf,
            directs, num_directs);

    longest_length = get_longest_segment_length((*det_routing_arch), segment_inf);

    /*now setup and compute the actual arrays */
    alloc_delta_arrays();

    compute_delta_arrays(router_opts, longest_length);

    /*free all data structures that are no longer needed */
    free_routing_structs();

    clock_t end = clock();

    float time = (float) (end - begin) / CLOCKS_PER_SEC;
    VTR_LOG("Placement delay look-up took %g seconds\n", time);
}

void free_place_lookup_structs() {

    free_delta_arrays();

}

float get_delta_delay(int delta_x, int delta_y) {
    return f_delta_delay[delta_x][delta_y];
}

/******* File Accessible Functions **********/

static int get_best_class(enum e_pin_type pintype, t_type_ptr type) {

    /*This function tries to identify the best pin class to hook up
     * for delay calculation.  The assumption is that we should pick
     * the pin class with the largest number of pins. This makes
     * sense, since it ensures we pick commonly used pins, and
     * removes order dependence on how the pins are specified
     * in the architecture (except in the case were the two largest pin classes
     * of a particular pintype have the same number of pins, in which case the
     * first pin class is used). */

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
        } else
            currpin += type->class_inf[i].num_pins;
    }
    VTR_ASSERT(best_class >= 0 && best_class < type->num_class);
    return (best_class);
}

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

static void reset_placement() {
    init_placement_context();
}

static void setup_chan_width(t_router_opts router_opts,
        t_chan_width_dist chan_width_dist) {
    /*we give plenty of tracks, this increases routability for the */
    /*lookup table generation */

    int width_fac;

    if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
        auto& device_ctx = g_vpr_ctx.device();

        auto type = find_most_common_block_type(device_ctx.grid);

        width_fac = 4 * type->num_pins;
        /*this is 2x the value that binary search starts */
        /*this should be enough to allow most pins to   */
        /*connect to tracks in the architecture */
    } else {
        width_fac = router_opts.fixed_channel_width;
    }

    init_chan(width_fac, chan_width_dist);
}

static void alloc_routing_structs(t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        const t_direct_inf *directs,
        const int num_directs) {

    int warnings;
    t_graph_type graph_type;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    free_rr_graph();

    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ?
                GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    create_rr_graph(graph_type,
            device_ctx.num_block_types, device_ctx.block_types,
            device_ctx.grid,
            &device_ctx.chan_width,
            device_ctx.num_arch_switches,
            det_routing_arch,
            segment_inf,
            router_opts.base_cost_type,
            router_opts.trim_empty_channels,
            router_opts.trim_obs_channels,
            directs, num_directs,
            &device_ctx.num_rr_switches,
            &warnings);

    alloc_and_load_rr_node_route_structs();

    alloc_route_tree_timing_structs();
}

static void free_routing_structs() {
    free_rr_graph();

    free_route_structs();
    free_trace_structs();

    free_route_tree_timing_structs();
}

static float route_connection_delay(int source_x, int source_y,
        int sink_x, int sink_y, t_router_opts router_opts) {
    //Routes between the source and sink locations and calculates the delay

    float net_delay_value = IMPOSSIBLE_DELTA; /*set to known value for debug purposes */

    auto& device_ctx = g_vpr_ctx.device();


    //Get the rr nodes to route between
    int driver_ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
    int source_rr_node = get_rr_node_index(device_ctx.rr_node_indices, source_x, source_y, SOURCE, driver_ptc);
    int sink_ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
    int sink_rr_node = get_rr_node_index(device_ctx.rr_node_indices, sink_x, sink_y, SINK, sink_ptc);

    bool successfully_routed = calculate_delay(source_rr_node, sink_rr_node,
            router_opts.astar_fac, router_opts.bend_cost,
            &net_delay_value);

    if (!successfully_routed) {
        VTR_LOG_WARN(
                "Unable to route between blocks at (%d,%d) and (%d,%d) to characterize delay (setting to %g)\n",
                source_x, source_y, sink_x, sink_y, net_delay_value);
    }

    return (net_delay_value);
}

static void alloc_delta_arrays() {

    auto& device_ctx = g_vpr_ctx.device();

    /*initialize all of the array locations to -1 */
    f_delta_delay.resize({device_ctx.grid.width(), device_ctx.grid.height()}, UNINITIALIZED_DELTA);
}

static void free_delta_arrays() {
    //Reclaim memory
    f_delta_delay.clear();
}

static void generic_compute_matrix(vtr::Matrix<float>& matrix,
        int source_x, int source_y,
        int start_x, int start_y,
        int end_x, int end_y,
        t_router_opts router_opts) {

    int delta_x, delta_y;
    int sink_x, sink_y;

    auto& device_ctx = g_vpr_ctx.device();

    for (sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (sink_y = start_y; sink_y <= end_y; sink_y++) {
            delta_x = abs(sink_x - source_x);
            delta_y = abs(sink_y - source_y);

            t_type_ptr src_type = device_ctx.grid[source_x][source_y].type;
            t_type_ptr sink_type = device_ctx.grid[sink_x][sink_y].type;

            bool src_or_target_empty = ( src_type == device_ctx.EMPTY_TYPE
                                        || sink_type == device_ctx.EMPTY_TYPE);

            if (src_or_target_empty) {

                if (matrix[delta_x][delta_y] == UNINITIALIZED_DELTA) {
                    //Only set empty target if we don't already have a valid delta delay
                    matrix[delta_x][delta_y] = EMPTY_DELTA;
#ifdef VERBOSE
                    VTR_LOG("Computed delay: %12s delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                                    "EMPTY",
                                    delta_x, delta_y,
                                    source_x, source_y,
                                    sink_x, sink_y);
#endif
                }
            } else {
                //Valid start/end

                float delay = route_connection_delay(source_x, source_y, sink_x, sink_y, router_opts);

#ifdef VERBOSE
                VTR_LOG("Computed delay: %12g delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                        delay,
                        delta_x, delta_y,
                        source_x, source_y,
                        sink_x, sink_y);
#endif
                if (matrix[delta_x][delta_y] >= 0.) { //Current is valid
                    //Keep the smallest delay
                    matrix[delta_x][delta_y] = std::min(matrix[delta_x][delta_y], delay);
                } else {
                    //First valid delay
                    matrix[delta_x][delta_y] = delay;
                }
            }
        }
    }
}

static void compute_delta_delays(t_router_opts router_opts, size_t longest_length) {

    //To avoid edge effects we place the source at least 'longest_length' away
    //from the device edge
    //and route from there for all possible delta values < dimension

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    size_t mid_x = vtr::nint(grid.width() / 2);
    size_t mid_y = vtr::nint(grid.width() / 2);

    size_t low_x = std::min(longest_length, mid_x);
    size_t low_y = std::min(longest_length, mid_y);

    //   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +        A        |                   B                   +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +                 |                                       +
    //   +-----------------*---------------------------------------+
    //   +                 |                                       +
    //   +        C        |                   D                   +
    //   +                 |                                       +
    //   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    //   * = (low_x, low_y)
    //   + = device edge

    //Find the lowest y location on the left edge with a non-empty block
    size_t y = 0;
    size_t x = 0;
    t_type_ptr src_type = nullptr;
    for (x = 0; x < grid.width(); ++x) {
        for (y = 0; y < grid.height(); ++y) {
            auto type = grid[x][y].type;

            if (type != device_ctx.EMPTY_TYPE) {
                src_type = type;
                break;
            }
        }
        if (src_type) {
            break;
        }
    }
    VTR_ASSERT(src_type != nullptr);

#ifdef VERBOSE
    VTR_LOG("Computing from lower left edge (%d,%d):\n", x, y);
#endif
    generic_compute_matrix(f_delta_delay,
            x, y,
            x, y,
            grid.width() - 1, grid.height() - 1,
            router_opts);

    //Find the lowest x location on the bottom edge with a non-empty block
    src_type = nullptr;
    for (y = 0; y < grid.height(); ++y) {
        for (x = 0; x < grid.width(); ++x) {
            auto type = grid[x][y].type;

            if (type != device_ctx.EMPTY_TYPE) {
                src_type = type;
                break;
            }
        }
        if (src_type) {
            break;
        }
    }
    VTR_ASSERT(src_type != nullptr);
#ifdef VERBOSE
    VTR_LOG("Computing from left bottom edge (%d,%d):\n",x, y);
#endif
    generic_compute_matrix(f_delta_delay,
            x, y,
            x, y,
            grid.width() - 1, grid.height() - 1,
            router_opts);


    //Since the other delta delay values may have suffered from edge effects,
    //we recalculate deltas within region B
#ifdef VERBOSE
    VTR_LOG("Computing from mid:\n");
#endif
    generic_compute_matrix(f_delta_delay,
            low_x, low_y,
            low_x, low_y,
            grid.width() - 1, grid.height() - 1,
            router_opts);
}

static void print_matrix(std::string filename, const vtr::Matrix<float>& matrix) {
    FILE* f = vtr::fopen(filename.c_str(), "w");

    fprintf(f, "         ");
    for (size_t delta_x = matrix.begin_index(0); delta_x < matrix.end_index(0); ++delta_x) {
        fprintf(f, " %9zu", delta_x);
    }
    fprintf(f, "\n");

    for (size_t delta_y = matrix.begin_index(1); delta_y < matrix.end_index(1); ++delta_y) {
        fprintf(f, "%9zu", delta_y);
        for (size_t delta_x = matrix.begin_index(0); delta_x < matrix.end_index(0); ++delta_x) {

            fprintf(f, " %9.2e", matrix[delta_x][delta_y]);
        }
        fprintf(f, "\n");
    }

    vtr::fclose(f);
}


/* Take the average of the valid neighboring values in the matrix.
 * use interpolation to get the right answer for empty blocks*/
static float find_neightboring_average(vtr::Matrix<float> &matrix, int x, int y) {
    float sum = 0;
    int counter = 0;
    int endx = matrix.end_index(0);
    int endy = matrix.end_index(1);

    int delx, dely;

    for (delx = x - 1; delx <= x + 1; delx++) {
        for (dely = y - 1; dely <= y + 1; dely++) {
            //check out of bounds
            if ((delx == x - 1 && dely == y + 1) || (delx == x + 1 && dely == y + 1) || (delx == x - 1 && dely == y - 1) || (delx == x + 1 && dely == y - 1)) {
                continue;
            }
            if (delx < 0 || dely < 0 || delx >= endx || dely >= endy || (delx == x && dely == y)) {
                continue;
            }
            if (matrix[delx][dely] == EMPTY_DELTA || matrix[delx][dely] == IMPOSSIBLE_DELTA) {
                continue;
            }
            counter++;
            sum += matrix[delx][dely];
        }
    }


    if (counter == 0) {
        //take in more values for more accuracy
        sum = 0;
        counter = 0;
        for (delx = x - 1; delx <= x + 1; delx++) {
            for (dely = y - 1; dely <= y + 1; dely++) {
                //check out of bounds
                if ((delx == x && dely == y) || delx < 0 || dely < 0 || delx >= endx || dely >= endy) {
                    continue;
                }
                if (matrix[delx][dely] == EMPTY_DELTA || matrix[delx][dely] == IMPOSSIBLE_DELTA) {
                    continue;
                }
                counter++;
                sum += matrix[delx][dely];
            }
        }
    }


    if (counter != 0) {
        return sum / (float) counter;
    } else {
        return 0;
    }
}

static void fix_empty_coordinates() {
    //Set any empty delta's to the average of it's neighbours
    for (size_t delta_x = 0; delta_x < f_delta_delay.dim_size(0); ++delta_x) {
        for (size_t delta_y = 0; delta_y < f_delta_delay.dim_size(1); ++delta_y) {
            if (f_delta_delay[delta_x][delta_y] == EMPTY_DELTA) {
                f_delta_delay[delta_x][delta_y] = find_neightboring_average(f_delta_delay, delta_x, delta_y);
            }
        }
    }
}

static void fix_uninitialized_coordinates() {
    //Set any empty delta's to the average of it's neighbours
    for (size_t delta_x = 0; delta_x < f_delta_delay.dim_size(0); ++delta_x) {
        for (size_t delta_y = 0; delta_y < f_delta_delay.dim_size(1); ++delta_y) {
            if (f_delta_delay[delta_x][delta_y] == UNINITIALIZED_DELTA) {
                f_delta_delay[delta_x][delta_y] = IMPOSSIBLE_DELTA;
            }
        }
    }
}

static void compute_delta_arrays(t_router_opts router_opts, int longest_length) {
    VTR_LOG("Computing delta delay lookup matrix, may take a few seconds, please wait...\n");
    compute_delta_delays(router_opts, longest_length);

    if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo("place_delay.pre_fix_empty.echo");
    }

    fix_uninitialized_coordinates();

    fix_empty_coordinates();


    if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
    }

    verify_delta_delays();
}

static bool verify_delta_delays() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    for(size_t x = 0; x < grid.width(); ++x) {
        for(size_t y = 0; y < grid.height(); ++y) {

            float delta_delay = get_delta_delay(x, y);

            if (delta_delay < 0.) {
                VPR_THROW(VPR_ERROR_PLACE,
                        "Found invaild negative delay %g for delta (%d,%d)",
                        delta_delay, x, y);
            }
        }
    }

    return true;
}

static void print_delta_delays_echo(const char* filename) {

    print_matrix(vtr::string_fmt(filename, "delta_delay"), f_delta_delay);
}


static t_rt_node* setup_routing_resources_no_net(int source_node) {

    /* Build and return a partial route tree from the legal connections from last iteration.
     * along the way do:
     * 	update pathfinder costs to be accurate to the partial route tree
     *	update the net's traceback to be accurate to the partial route tree
     * 	find and store the pins that still need to be reached in incremental_rerouting_resources.remaining_targets
     * 	find and store the rt nodes that have been reached in incremental_rerouting_resources.reached_rt_sinks
     *	mark the rr_node sinks as targets to be reached */

    t_rt_node* rt_root;

    // for nets below a certain size (min_incremental_reroute_fanout), rip up any old routing
    // otherwise, we incrementally reroute by reusing legal parts of the previous iteration
    // convert the previous iteration's traceback into the starting route tree for this iteration

    profiling::net_rerouted();

    //    // rip up the whole net
    //    pathfinder_update_path_cost(route_ctx.trace_head[inet], -1, 0);
    //    free_traceback(inet);

    rt_root = init_route_tree_to_source_no_net(source_node);

    // completed constructing the partial route tree and updated all other data structures to match

    return rt_root;
}

static bool calculate_delay(int source_node, int sink_node,
        float astar_fac, float bend_cost, float *net_delay) {

    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    t_rt_node* rt_root = setup_routing_resources_no_net(source_node);

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(1);

    //maximum bounding box for placement
    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;

    float target_criticality = 1;

    route_budgets budgeting_inf;


    init_heap(device_ctx.grid);

    std::vector<int> modified_rr_node_inf;
    RouterStats router_stats;
    t_heap* cheapest = timing_driven_route_connection(source_node, sink_node, target_criticality,
            astar_fac, bend_cost, rt_root, bounding_box, 1, budgeting_inf, 0, 0, 0, 0, modified_rr_node_inf, router_stats);

    if (cheapest == nullptr) {
        return false;
    }
    VTR_ASSERT(cheapest->index == sink_node);

    std::set<int> used_rr_nodes;
    t_rt_node* rt_node_of_sink = update_route_tree(cheapest);
    free_heap_data(cheapest);
    empty_heap();

    //Reset path costs for the next router call
    reset_path_costs(modified_rr_node_inf);

    // finished all sinks

    //find delay
    *net_delay = rt_node_of_sink->Tdel;

    if (*net_delay == 0) { // should be SOURCE->OPIN->IPIN->SINK
        VTR_ASSERT(device_ctx.rr_nodes[rt_node_of_sink->parent_node->parent_node->inode].type() == OPIN);
    }

    VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");

    // route tree is not kept persistent since building it from the traceback the next iteration takes almost 0 time
    free_route_tree(rt_root);
    return (true);
}

