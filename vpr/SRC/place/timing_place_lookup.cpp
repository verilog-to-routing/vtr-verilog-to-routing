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
#include "netlist.h"
#include "echo_files.h"
#include "atom_netlist.h"
#include "rr_graph2.h"
// all functions in profiling:: namespace, which are only activated if PROFILE is defined
#include "route_profiling.h" 

/*To compute delay between blocks we calculate the delay between */
/*different nodes in the FPGA.  From this procedure we generate 
 * a lookup table which tells us the delay between different locations in*/
/*the FPGA */

/*the delta arrays are used to contain the best case routing delay */
/*between different locations on the FPGA. */

static vtr::Matrix<float> f_delta_io_to_clb;
static vtr::Matrix<float> f_delta_clb_to_clb;
static vtr::Matrix<float> f_delta_clb_to_io;
static vtr::Matrix<float> f_delta_io_to_io;

//These store the coordinates of the empty blocks in the delta matrix
static vector <pair<int, int>> io_to_clb_empty;
static vector <pair<int, int>> clb_to_clb_empty;
static vector <pair<int, int>> io_to_io_empty;
static vector <pair<int, int>> clb_to_io_empty;

/*** Function Prototypes *****/
static void setup_chan_width(t_router_opts router_opts,
        t_chan_width_dist chan_width_dist);

static void alloc_routing_structs(t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        const t_direct_inf *directs,
        const int num_directs);

static void free_routing_structs();

static float assign_blocks_and_route_net(int source_node, int sink_node, int source_x_loc, int source_y_loc,
        int sink_x_loc, int sink_y_loc, t_router_opts router_opts);

static void alloc_delta_arrays(void);

static void free_delta_arrays(void);

static void generic_compute_matrix(vtr::Matrix<float>& matrix, int source_x, int source_y, int start_x,
        int end_x, int start_y, int end_y, t_router_opts router_opts);

static void compute_delta_clb_to_clb(t_router_opts router_opts, int longest_length);

static void compute_delta_io_to_clb(t_router_opts router_opts);

static void compute_delta_clb_to_io(t_router_opts router_opts);

static void compute_delta_io_to_io(t_router_opts router_opts);

static void compute_delta_arrays(t_router_opts router_opts, int longest_length);

static int get_best_class(enum e_pin_type pintype, t_type_ptr type);

static int get_longest_segment_length(
        t_det_routing_arch det_routing_arch, t_segment_inf * segment_inf);
static void reset_placement(void);

static void print_delta_delays_echo(const char* filename);

static void print_matrix(std::string filename, const vtr::Matrix<float>& array_to_print);

static void fix_empty_coordinates(void);

static float find_neightboring_average(vtr::Matrix<float> &matrix, int x, int y);

static bool calculate_delay(int source_node, int sink_node,
        float astar_fac, float bend_cost, float *net_delay);

static t_rt_node* setup_routing_resources_no_net(int source_node);


/**************************************/
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

    int warnings;
    t_graph_type graph_type;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    /*calls routines that set up routing resource graph and associated structures */
    alloc_route_static_structs();

    free_rr_graph();

    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ?
                GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    create_rr_graph(graph_type, device_ctx.num_block_types, device_ctx.block_types, device_ctx.nx, device_ctx.ny, device_ctx.grid,
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
            directs, num_directs,
            det_routing_arch->dump_rr_structs_file,
            &det_routing_arch->wire_to_rr_ipin_switch,
            &device_ctx.num_rr_switches,
            &warnings, router_opts.write_rr_graph_name.c_str(),
            router_opts.read_rr_graph_name.c_str(), true);

    alloc_and_load_rr_node_route_structs();

    alloc_route_tree_timing_structs();
}

/**************************************/
static void free_routing_structs() {
    free_rr_graph();

    free_rr_node_route_structs();
    free_route_structs();
    free_trace_structs();

    free_route_tree_timing_structs();
}

/**************************************/
static float assign_blocks_and_route_net(int source_node, int sink_node, int source_x_loc, int source_y_loc,
        int sink_x_loc, int sink_y_loc, t_router_opts router_opts) {

    /*places blocks at the specified locations, and routes a net between them */
    /*returns the delay of this net */

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Only one block per tile */
    int source_z_loc = 0;
    int sink_z_loc = 0;

    float net_delay_value = IMPOSSIBLE; /*set to known value for debug purposes */

    bool successfully_routed = calculate_delay(source_node, sink_node,
            router_opts.astar_fac, router_opts.bend_cost,
            &net_delay_value);

    if (!successfully_routed) {
        vtr::printf_warning(__FILE__, __LINE__,
                "Unable to route between blocks at (%d,%d) and (%d,%d) to characterize delay (setting to inf)\n",
                source_x_loc, source_y_loc, sink_x_loc, sink_y_loc);

        //We set the delay to +inf, since this architecture will likely be unroutable
        net_delay_value = std::numeric_limits<float>::infinity();
    }

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
    f_delta_io_to_clb.resize({nx + 1, ny + 1}, IMPOSSIBLE);
    f_delta_clb_to_io.resize({nx + 1, ny + 1}, IMPOSSIBLE);
    f_delta_io_to_io.resize({nx + 2, ny + 2}, IMPOSSIBLE);
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
static void generic_compute_matrix(vtr::Matrix<float>& matrix,
        int source_x, int source_y, int start_x,
        int end_x, int start_y, int end_y, t_router_opts router_opts) {

    int delta_x, delta_y;
    int sink_x, sink_y;
    int start_node, end_node, ptc;

    auto& device_ctx = g_vpr_ctx.device();

    for (sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (sink_y = start_y; sink_y <= end_y; sink_y++) {
            delta_x = abs(sink_x - source_x);
            delta_y = abs(sink_y - source_y);

            if (delta_x == 0 && delta_y == 0)
                continue; /*do not compute distance from a block to itself     */
            /*if a value is desired, pre-assign it somewhere else */

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                matrix[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                io_to_clb_empty.push_back(empty_cell);

                continue;
            }

            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

            matrix[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
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
    auto& device_ctx = g_vpr_ctx.device();
    int start_node, end_node, ptc;

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

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                f_delta_clb_to_clb[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                clb_to_clb_empty.push_back(empty_cell);
                continue;
            }

            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

            f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
                    router_opts);
        }

    }

    sink_x = end_x - 1;
    sink_y = end_y - 1;

    for (source_x = start_x - 1; source_x >= 1; source_x--) {
        for (source_y = start_y; source_y <= end_y - 1; source_y++) {
            delta_x = abs(sink_x - source_x);
            delta_y = abs(sink_y - source_y);

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                f_delta_clb_to_clb[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                clb_to_clb_empty.push_back(empty_cell);
                continue;
            }

            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

            f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
                    router_opts);

        }
    }

    for (source_x = 1; source_x <= end_x - 1; source_x++) {
        for (source_y = 1; source_y < start_y; source_y++) {
            delta_x = abs(sink_x - source_x);
            delta_y = abs(sink_y - source_y);

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                f_delta_clb_to_clb[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                clb_to_clb_empty.push_back(empty_cell);
                continue;
            }

            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

            f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
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

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_clb_to_clb[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            clb_to_clb_empty.push_back(empty_cell);
            continue;
        }
        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

        f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);

    }

    sink_x = end_x;
    sink_y = end_y;
    source_y = 1;
    for (source_x = 1; source_x <= end_x; source_x++) {
        delta_x = abs(sink_x - source_x);
        delta_y = abs(sink_y - source_y);

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_clb_to_clb[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            clb_to_clb_empty.push_back(empty_cell);

            continue;
        }

        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

        f_delta_clb_to_clb[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);

    }
}

/**************************************/
static void compute_delta_io_to_clb(t_router_opts router_opts) {

    int source_x, source_y;
    int start_x, start_y, end_x, end_y;
    auto& device_ctx = g_vpr_ctx.device();

    f_delta_io_to_clb[0][0] = IMPOSSIBLE;
    f_delta_io_to_clb[device_ctx.nx][device_ctx.ny] = IMPOSSIBLE;

    source_x = 0;
    source_y = 1;

    start_x = 1;
    end_x = device_ctx.nx;
    start_y = 1;
    end_y = device_ctx.ny;
    generic_compute_matrix(f_delta_io_to_clb, source_x,
            source_y, start_x, end_x, start_y, end_y, router_opts);

    source_x = 1;
    source_y = 0;

    start_x = 1;
    end_x = 1;
    start_y = 1;
    end_y = device_ctx.ny;
    generic_compute_matrix(f_delta_io_to_clb, source_x,
            source_y, start_x, end_x, start_y, end_y, router_opts);

    start_x = 1;
    end_x = device_ctx.nx;
    start_y = device_ctx.ny;
    end_y = device_ctx.ny;
    generic_compute_matrix(f_delta_io_to_clb, source_x,
            source_y, start_x, end_x, start_y, end_y, router_opts);
}

/**************************************/
static void compute_delta_clb_to_io(t_router_opts router_opts) {
    int source_x, source_y, sink_x, sink_y;
    int delta_x, delta_y;
    int start_node, end_node, ptc;
    auto& device_ctx = g_vpr_ctx.device();

    f_delta_clb_to_io[0][0] = IMPOSSIBLE;
    f_delta_clb_to_io[device_ctx.nx][device_ctx.ny] = IMPOSSIBLE;

    sink_x = 0;
    sink_y = 1;
    for (source_x = 1; source_x <= device_ctx.nx; source_x++) {
        for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
            delta_x = abs(source_x - sink_x);
            delta_y = abs(source_y - sink_y);

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                f_delta_clb_to_io[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                clb_to_io_empty.push_back(empty_cell);
                continue;
            }
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
                    router_opts);
        }
    }

    sink_x = 1;
    sink_y = 0;
    source_x = 1;
    delta_x = abs(source_x - sink_x);
    for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
        delta_y = abs(source_y - sink_y);
        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_clb_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            clb_to_io_empty.push_back(empty_cell);
            continue;
        }

        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

        f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);

    }

    sink_x = 1;
    sink_y = 0;
    source_y = device_ctx.ny;
    delta_y = abs(source_y - sink_y);
    for (source_x = 2; source_x <= device_ctx.nx; source_x++) {
        delta_x = abs(source_x - sink_x);

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_clb_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            clb_to_io_empty.push_back(empty_cell);

            continue;
        }

        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);


        f_delta_clb_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);

    }
}

/**************************************/
static void compute_delta_io_to_io(t_router_opts router_opts) {
    int source_x, source_y, sink_x, sink_y;
    int delta_x, delta_y;
    int start_node, end_node, ptc;
    auto& device_ctx = g_vpr_ctx.device();

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

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_io_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            io_to_io_empty.push_back(empty_cell);
            continue;
        }
        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);


        f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);

    }

    source_x = 0;
    source_y = 1;
    sink_x = device_ctx.nx + 1;
    delta_x = abs(sink_x - source_x);

    for (sink_y = 1; sink_y <= device_ctx.ny; sink_y++) {
        delta_y = abs(sink_y - source_y);

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_io_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            io_to_io_empty.push_back(empty_cell);
            continue;
        }
        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);
        
        f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);
    }

    source_x = 1;
    source_y = 0;
    sink_y = 0;
    delta_y = abs(sink_y - source_y);

    for (sink_x = 2; sink_x <= device_ctx.nx; sink_x++) {
        delta_x = abs(sink_x - source_x);

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_io_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            io_to_io_empty.push_back(empty_cell);
            continue;
        }

        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);
        
        f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);
    }

    source_x = 1;
    source_y = 0;
    sink_y = device_ctx.ny + 1;
    delta_y = abs(sink_y - source_y);

    for (sink_x = 1; sink_x <= device_ctx.nx; sink_x++) {
        delta_x = abs(sink_x - source_x);

        if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
            f_delta_io_to_io[delta_x][delta_y] = EMPTY_DELTA;
            pair<int, int> empty_cell(delta_x, delta_y);
            io_to_io_empty.push_back(empty_cell);
            continue;
        }

        ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
        start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
        ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
        end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

        f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                source_x, source_y, sink_x, sink_y,
                router_opts);
    }

    source_x = 0;
    sink_y = device_ctx.ny + 1;
    for (source_y = 1; source_y <= device_ctx.ny; source_y++) {
        for (sink_x = 1; sink_x <= device_ctx.nx; sink_x++) {
            delta_y = abs(source_y - sink_y);
            delta_x = abs(source_x - sink_x);

            if (device_ctx.grid[source_x][source_y].type == device_ctx.EMPTY_TYPE ||
                    device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                f_delta_io_to_io[delta_x][delta_y] = EMPTY_DELTA;
                pair<int, int> empty_cell(delta_x, delta_y);
                io_to_io_empty.push_back(empty_cell);
                continue;
            }

            ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
            start_node = get_rr_node_index(source_x, source_y, SOURCE, ptc, device_ctx.rr_node_indices);
            ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
            end_node = get_rr_node_index(sink_x, sink_y, SINK, ptc, device_ctx.rr_node_indices);

            f_delta_io_to_io[delta_x][delta_y] = assign_blocks_and_route_net(start_node, end_node,
                    source_x, source_y, sink_x, sink_y,
                    router_opts);
        }
    }
}

/**************************************/
static void
print_matrix(std::string filename, const vtr::Matrix<float>& matrix) {
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

/**************************************/

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
            if (matrix[delx][dely] == EMPTY_DELTA || matrix[delx][dely] == IMPOSSIBLE) {
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
                if (matrix[delx][dely] == EMPTY_DELTA || matrix[delx][dely] == IMPOSSIBLE) {
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

static void fix_empty_coordinates(void) {
    unsigned i;
    for (i = 0; i < io_to_clb_empty.size(); i++) {
        f_delta_io_to_clb[io_to_clb_empty[i].first][io_to_clb_empty[i].second] = find_neightboring_average(f_delta_io_to_clb, io_to_clb_empty[i].first, io_to_clb_empty[i].second);
    }
    for (i = 0; i < io_to_io_empty.size(); i++) {
        f_delta_io_to_io[io_to_io_empty[i].first][io_to_io_empty[i].second] = find_neightboring_average(f_delta_io_to_io, io_to_io_empty[i].first, io_to_io_empty[i].second);
    }
    for (i = 0; i < clb_to_clb_empty.size(); i++) {
        f_delta_clb_to_clb[clb_to_clb_empty[i].first][clb_to_clb_empty[i].second] = find_neightboring_average(f_delta_clb_to_clb, clb_to_clb_empty[i].first, clb_to_clb_empty[i].second);
    }
    for (i = 0; i < clb_to_io_empty.size(); i++) {
        f_delta_clb_to_io[clb_to_io_empty[i].first][clb_to_io_empty[i].second] = find_neightboring_average(f_delta_clb_to_io, clb_to_io_empty[i].first, clb_to_io_empty[i].second);
    }
    
    if (!io_to_clb_empty.empty()) {
        io_to_clb_empty.clear();
    }
    if (!clb_to_clb_empty.empty()) {
        clb_to_clb_empty.clear();
    }
    if (!clb_to_io_empty.empty()) {
        clb_to_io_empty.clear();
    }
    if (!io_to_io_empty.empty()) {
        io_to_io_empty.clear();
    }
}

static void compute_delta_arrays(t_router_opts router_opts, int longest_length) {
    vtr::printf_info("Computing delta_io_to_io lookup matrix, may take a few seconds, please wait...\n");
    compute_delta_io_to_io(router_opts);
    vtr::printf_info("Computing delta_io_to_clb lookup matrix, may take a few seconds, please wait...\n");
    compute_delta_io_to_clb(router_opts);
    vtr::printf_info("Computing delta_clb_to_io lookup matrix, may take a few seconds, please wait...\n");
    compute_delta_clb_to_io(router_opts);
    vtr::printf_info("Computing delta_clb_to_clb lookup matrix, may take a few seconds, please wait...\n");
    compute_delta_clb_to_clb(router_opts, longest_length);

    fix_empty_coordinates();

    if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
    }
}

static void print_delta_delays_echo(const char* filename) {

    print_matrix(vtr::string_fmt(filename, "delta_clb_to_clb"), f_delta_clb_to_clb);
    print_matrix(vtr::string_fmt(filename, "delta_io_to_clb"), f_delta_io_to_clb);
    print_matrix(vtr::string_fmt(filename, "delta_clb_to_io"), f_delta_clb_to_io);
    print_matrix(vtr::string_fmt(filename, "delta_io_to_io"), f_delta_io_to_io);
}

/******* Globally Accessible Functions **********/

/**************************************/
void compute_delay_lookup_tables(t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        t_chan_width_dist chan_width_dist, const t_direct_inf *directs,
        const int num_directs) {

    vtr::printf_info("\nStarting placement delay look-up...\n");
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
    bounding_box.xmax = device_ctx.nx + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.ny + 1;

    float target_criticality = 1;
    // explore in order of decreasing criticality (no longer need sink_order array)

    t_heap* cheapest = timing_driven_route_connection(source_node, sink_node, target_criticality,
            astar_fac, bend_cost, rt_root, bounding_box, 1);

    if (cheapest == NULL) {
        return false;
    }
    t_rt_node* rt_node_of_sink = update_route_tree(cheapest);
    free_heap_data(cheapest);
    empty_heap();

    // need to gurantee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
    // do this by resetting all the path_costs that have been touched while routing to the current sink
    reset_path_costs();
    // finished all sinks

    //find delay
    *net_delay = rt_node_of_sink->Tdel;

    if (*net_delay == 0) { // should be SOURCE->OPIN->IPIN->SINK
        VTR_ASSERT(device_ctx.rr_nodes[rt_node_of_sink->parent_node->parent_node->inode].type() == OPIN);
    }

    VTR_ASSERT_MSG(route_ctx.rr_node_state[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");

    // route tree is not kept persistent since building it from the traceback the next iteration takes almost 0 time
    free_route_tree(rt_root);
    return (true);
}

