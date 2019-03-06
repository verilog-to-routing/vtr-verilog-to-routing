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
#include "vtr_time.h"

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
#include "router_delay_profiling.h"

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
static t_chan_width setup_chan_width(t_router_opts router_opts,
        t_chan_width_dist chan_width_dist);

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

static void adjust_delta_delays(t_placer_opts placer_opts);

/******* Globally Accessible Functions **********/

void compute_delay_lookup_tables(
        t_placer_opts placer_opts,
        t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        t_chan_width_dist chan_width_dist, const t_direct_inf *directs,
        const int num_directs) {

    vtr::ScopedStartFinishTimer timer("Computing placement delta delay look-up");

    reset_placement();

    t_chan_width chan_width = setup_chan_width(router_opts, chan_width_dist);

    alloc_routing_structs(chan_width, router_opts, det_routing_arch, segment_inf,
            directs, num_directs);

    int longest_length = get_longest_segment_length((*det_routing_arch), segment_inf);

    /*now setup and compute the actual arrays */
    alloc_delta_arrays();

    compute_delta_arrays(router_opts, longest_length);

    adjust_delta_delays(placer_opts);

    if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
    }

    /*free all data structures that are no longer needed */
    free_routing_structs();
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

    VTR_ASSERT(best_class < type->num_class);
    return best_class;
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

static t_chan_width setup_chan_width(t_router_opts router_opts,
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

    return init_chan(width_fac, chan_width_dist);
}

static float route_connection_delay(int source_x, int source_y,
        int sink_x, int sink_y, t_router_opts router_opts) {
    //Routes between the source and sink locations and calculates the delay

    float net_delay_value = IMPOSSIBLE_DELTA; /*set to known value for debug purposes */

    auto& device_ctx = g_vpr_ctx.device();

    //Get the rr nodes to route between
    int driver_ptc = get_best_class(DRIVER, device_ctx.grid[source_x][source_y].type);
    int source_rr_node = -1;
    if (driver_ptc > -1)
        source_rr_node = get_rr_node_index(device_ctx.rr_node_indices, source_x, source_y, SOURCE, driver_ptc);

    int sink_ptc = get_best_class(RECEIVER, device_ctx.grid[sink_x][sink_y].type);
    int sink_rr_node = -1;
    if (sink_ptc > -1)
        sink_rr_node = get_rr_node_index(device_ctx.rr_node_indices, sink_x, sink_y, SINK, sink_ptc);

    bool successfully_routed = false;
    if (source_rr_node >= 0 && sink_rr_node >= 0) {
        successfully_routed = calculate_delay(
            source_rr_node, sink_rr_node,
            router_opts,
            &net_delay_value);
    }

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
    vtr::ScopedStartFinishTimer timer("Computing delta delays");
    compute_delta_delays(router_opts, longest_length);

    if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
        print_delta_delays_echo("place_delay.pre_fix_empty.echo");
    }

    fix_uninitialized_coordinates();

    fix_empty_coordinates();


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


static void adjust_delta_delays(t_placer_opts placer_opts) {
    //Apply a constant offset to the delay model
    for (size_t x = 0; x < f_delta_delay.dim_size(0); ++x) {
        for (size_t y = 0; y < f_delta_delay.dim_size(1); ++y) {
            f_delta_delay[x][y] += placer_opts.delay_offset;
        }
    }

    if (placer_opts.delay_ramp_delta_threshold >= 0) {
        //For each delta-x/delta-y value beyond delay_ramp_delta_threshold
        //apply an extra (linear) delay penalty based on delay_ramp_slope
        //and the distance beyond the delta threshold

        for (size_t x = 0; x < f_delta_delay.dim_size(0); ++x) {
            for (size_t y = 0; y < f_delta_delay.dim_size(1); ++y) {
                int dx = x - placer_opts.delay_ramp_delta_threshold;
                if (dx > 0) {
                    f_delta_delay[x][y] += dx * placer_opts.delay_ramp_slope;
                }

                int dy = y - placer_opts.delay_ramp_delta_threshold;
                if (dy > 0) {
                    f_delta_delay[x][y] += dy * placer_opts.delay_ramp_slope;
                }
            }
        }
    }
}

