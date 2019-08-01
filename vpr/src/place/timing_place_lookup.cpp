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
#include "vtr_geometry.h"

#include "arch_util.h"

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
#include "place_delay_model.h"

/*To compute delay between blocks we calculate the delay between */
/*different nodes in the FPGA.  From this procedure we generate
 * a lookup table which tells us the delay between different locations in*/
/*the FPGA */

/*the delta arrays are used to contain the best case routing delay */
/*between different locations on the FPGA. */

//#define VERBOSE

constexpr float UNINITIALIZED_DELTA = -1;                                  //Indicates the delta delay value has not been calculated
constexpr float EMPTY_DELTA = -2;                                          //Indicates delta delay from/to an EMPTY block
constexpr float IMPOSSIBLE_DELTA = std::numeric_limits<float>::infinity(); //Indicates there is no valid delta delay

struct t_profile_loc {
    t_profile_loc(int x, int y, std::vector<vtr::Point<int>> delta_values)
        : root(x, y)
        , deltas(delta_values) {}

    vtr::Point<int> root;
    std::vector<vtr::Point<int>> deltas;
};

struct t_profile_info {
    std::vector<t_profile_loc> locations;

    int max_delta_x;
    int max_delta_y;
};

/*** Function Prototypes *****/
static t_chan_width setup_chan_width(const t_router_opts& router_opts,
                                     t_chan_width_dist chan_width_dist);

static float route_connection_delay(
    const RouterDelayProfiler& route_profiler,
    int source_x_loc,
    int source_y_loc,
    int sink_x_loc,
    int sink_y_loc,
    const t_router_opts& router_opts,
    bool measure_directconnect);

static void generic_compute_matrix(
    const RouterDelayProfiler& route_profiler,
    vtr::Matrix<std::vector<float>>& matrix,
    int source_x,
    int source_y,
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    const t_router_opts& router_opts,
    bool measure_directconnect);

static vtr::Matrix<float> compute_delta_delays(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& palcer_opts,
    const t_router_opts& router_opts,
    bool measure_directconnect,
    size_t longest_length);

float delay_reduce(std::vector<float>& delays, e_reducer reducer);

static vtr::Matrix<float> compute_delta_delay_model(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& placer_opts,
    const t_router_opts& router_opts,
    bool measure_directconnect,
    int longest_length);

static bool find_direct_connect_sample_locations(const t_direct_inf* direct,
                                                 t_type_ptr from_type,
                                                 int from_pin,
                                                 int from_pin_class,
                                                 t_type_ptr to_type,
                                                 int to_pin,
                                                 int to_pin_class,
                                                 int* src_rr,
                                                 int* sink_rr);

static bool verify_delta_delays(const vtr::Matrix<float>& delta_delays);

static int get_longest_segment_length(std::vector<t_segment_inf>& segment_inf);

static void fix_empty_coordinates(vtr::Matrix<float>& delta_delays);
static void fix_uninitialized_coordinates(vtr::Matrix<float>& delta_delays);

static float find_neightboring_average(vtr::Matrix<float>& matrix, int x, int y, int max_distance);

/******* Globally Accessible Functions **********/

std::unique_ptr<PlaceDelayModel> compute_place_delay_model(const t_placer_opts& placer_opts,
                                                           const t_router_opts& router_opts,
                                                           t_det_routing_arch* det_routing_arch,
                                                           std::vector<t_segment_inf>& segment_inf,
                                                           t_chan_width_dist chan_width_dist,
                                                           const t_direct_inf* directs,
                                                           const int num_directs) {
    vtr::ScopedStartFinishTimer timer("Computing placement delta delay look-up");

    init_placement_context();

    t_chan_width chan_width = setup_chan_width(router_opts, chan_width_dist);

    alloc_routing_structs(chan_width, router_opts, det_routing_arch, segment_inf,
                          directs, num_directs);

    const RouterLookahead* router_lookahead = get_cached_router_lookahead(
        router_opts.lookahead_type,
        router_opts.write_router_lookahead,
        router_opts.read_router_lookahead,
        segment_inf);
    RouterDelayProfiler route_profiler(router_lookahead);

    int longest_length = get_longest_segment_length(segment_inf);

    /*now setup and compute the actual arrays */
    std::unique_ptr<PlaceDelayModel> place_delay_model;
    if (placer_opts.delay_model_type == PlaceDelayModelType::DELTA) {
        place_delay_model = std::make_unique<DeltaDelayModel>();
    } else if (placer_opts.delay_model_type == PlaceDelayModelType::DELTA_OVERRIDE) {
        place_delay_model = std::make_unique<OverrideDelayModel>();
    } else {
        VTR_ASSERT_MSG(false, "Invalid placer delay model");
    }

    if (placer_opts.read_placement_delay_lookup.empty()) {
        place_delay_model->compute(route_profiler, placer_opts, router_opts, longest_length);
    } else {
        place_delay_model->read(placer_opts.read_placement_delay_lookup);
    }

    if (!placer_opts.write_placement_delay_lookup.empty()) {
        place_delay_model->write(placer_opts.write_placement_delay_lookup);
    }

    /*free all data structures that are no longer needed */
    free_routing_structs();

    return place_delay_model;
}

void DeltaDelayModel::compute(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& placer_opts,
    const t_router_opts& router_opts,
    int longest_length) {
    delays_ = compute_delta_delay_model(
        route_profiler,
        placer_opts, router_opts, /*measure_directconnect=*/true,
        longest_length);
}

void OverrideDelayModel::compute(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& placer_opts,
    const t_router_opts& router_opts,
    int longest_length) {
    auto delays = compute_delta_delay_model(
        route_profiler,
        placer_opts, router_opts, /*measure_directconnect=*/false,
        longest_length);

    base_delay_model_ = std::make_unique<DeltaDelayModel>(delays);

    compute_override_delay_model(route_profiler, router_opts);
}

/******* File Accessible Functions **********/

std::vector<int> get_best_classes(enum e_pin_type pintype, t_type_ptr type) {
    /*
     * This function tries to identify the best pin classes to hook up
     * for delay calculation.  The assumption is that we should pick
     * the pin class with the largest number of pins. This makes
     * sense, since it ensures we pick commonly used pins, and
     * removes order dependence on how the pins are specified
     * in the architecture (except in the case were the two largest pin classes
     * of a particular pintype have the same number of pins, in which case the
     * first pin class is used).
     */

    std::vector<int> best_classes;

    //Collect all classes of matching type which do not have all their pins ignored
    for (int i = 0; i < type->num_class; i++) {
        if (type->class_inf[i].type == pintype) {
            bool all_ignored = true;
            for (int ipin = 0; ipin < type->class_inf[i].num_pins; ++ipin) {
                int pin = type->class_inf[i].pinlist[ipin];
                if (!type->is_ignored_pin[pin]) {
                    all_ignored = false;
                    break;
                }
            }
            if (!all_ignored) {
                best_classes.push_back(i);
            }
        }
    }

    //Sort classe so largest pin class is first
    auto cmp_class = [&](int lhs, int rhs) {
        return type->class_inf[lhs].num_pins > type->class_inf[rhs].num_pins;
    };

    std::stable_sort(best_classes.begin(), best_classes.end(), cmp_class);

    return best_classes;
}

static int get_longest_segment_length(std::vector<t_segment_inf>& segment_inf) {
    int length;

    length = 0;
    for (size_t i = 0; i < segment_inf.size(); i++) {
        if (segment_inf[i].length > length)
            length = segment_inf[i].length;
    }
    return (length);
}

static t_chan_width setup_chan_width(const t_router_opts& router_opts,
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

static float route_connection_delay(
    const RouterDelayProfiler& route_profiler,
    int source_x,
    int source_y,
    int sink_x,
    int sink_y,
    const t_router_opts& router_opts,
    bool measure_directconnect) {
    //Routes between the source and sink locations and calculates the delay

    float net_delay_value = IMPOSSIBLE_DELTA; /*set to known value for debug purposes */

    auto& device_ctx = g_vpr_ctx.device();

    bool successfully_routed = false;

    //Get the rr nodes to route between
    auto best_driver_ptcs = get_best_classes(DRIVER, device_ctx.grid[source_x][source_y].type);
    auto best_sink_ptcs = get_best_classes(RECEIVER, device_ctx.grid[sink_x][sink_y].type);

    for (int driver_ptc : best_driver_ptcs) {
        VTR_ASSERT(driver_ptc != OPEN);

        int source_rr_node = get_rr_node_index(device_ctx.rr_node_indices, source_x, source_y, SOURCE, driver_ptc);

        VTR_ASSERT(source_rr_node != OPEN);

        for (int sink_ptc : best_sink_ptcs) {
            VTR_ASSERT(sink_ptc != OPEN);

            int sink_rr_node = get_rr_node_index(device_ctx.rr_node_indices, sink_x, sink_y, SINK, sink_ptc);

            VTR_ASSERT(sink_rr_node != OPEN);

            if (!measure_directconnect && directconnect_exists(source_rr_node, sink_rr_node)) {
                //Skip if we shouldn't measure direct connects and a direct connect exists
                continue;
            }

            {
                successfully_routed = route_profiler.calculate_delay(
                    source_rr_node, sink_rr_node,
                    router_opts,
                    &net_delay_value);
            }

            if (successfully_routed) break;
        }
        if (successfully_routed) break;
    }

    if (!successfully_routed) {
        VTR_LOG_WARN("Unable to route between blocks at (%d,%d) and (%d,%d) to characterize delay (setting to %g)\n",
                     source_x, source_y, sink_x, sink_y, net_delay_value);
    }

    return (net_delay_value);
}

static void generic_compute_matrix(
    const RouterDelayProfiler& route_profiler,
    vtr::Matrix<std::vector<float>>& matrix,
    int source_x,
    int source_y,
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    const t_router_opts& router_opts,
    bool measure_directconnect) {
    int delta_x, delta_y;
    int sink_x, sink_y;

    auto& device_ctx = g_vpr_ctx.device();

    for (sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (sink_y = start_y; sink_y <= end_y; sink_y++) {
            delta_x = abs(sink_x - source_x);
            delta_y = abs(sink_y - source_y);

            t_type_ptr src_type = device_ctx.grid[source_x][source_y].type;
            t_type_ptr sink_type = device_ctx.grid[sink_x][sink_y].type;

            bool src_or_target_empty = (src_type == device_ctx.EMPTY_TYPE
                                        || sink_type == device_ctx.EMPTY_TYPE);

            if (src_or_target_empty) {
                if (matrix[delta_x][delta_y].empty()) {
                    //Only set empty target if we don't already have a valid delta delay
                    matrix[delta_x][delta_y].push_back(EMPTY_DELTA);
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

                float delay = route_connection_delay(route_profiler, source_x, source_y, sink_x, sink_y, router_opts, measure_directconnect);

#ifdef VERBOSE
                VTR_LOG("Computed delay: %12g delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                        delay,
                        delta_x, delta_y,
                        source_x, source_y,
                        sink_x, sink_y);
#endif
                if (matrix[delta_x][delta_y].size() == 1 && matrix[delta_x][delta_y][0] == EMPTY_DELTA) {
                    //Overwrite empty delta
                    matrix[delta_x][delta_y][0] = delay;
                } else {
                    //Collect delta
                    matrix[delta_x][delta_y].push_back(delay);
                }
            }
        }
    }
}

static vtr::Matrix<float> compute_delta_delays(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& placer_opts,
    const t_router_opts& router_opts,
    bool measure_directconnect,
    size_t longest_length) {
    //To avoid edge effects we place the source at least 'longest_length' away
    //from the device edge
    //and route from there for all possible delta values < dimension

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    vtr::Matrix<std::vector<float>> sampled_delta_delays({grid.width(), grid.height()});

    size_t mid_x = vtr::nint(grid.width() / 2);
    size_t mid_y = vtr::nint(grid.height() / 2);

    size_t low_x = std::min(longest_length, mid_x);
    size_t low_y = std::min(longest_length, mid_y);
    size_t high_x = std::max(grid.width() - longest_length, mid_x);
    size_t high_y = std::max(grid.height() - longest_length, mid_y);

    //   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //   +                 |                       |               +
    //   +        A        |           B           |       C       +
    //   +                 |                       |               +
    //   +-----------------\-----------------------.---------------+
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +        D        |           E           |       F       +
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +                 |                       |               +
    //   +-----------------*-----------------------/---------------+
    //   +                 |                       |               +
    //   +        G        |           H           |       I       +
    //   +                 |                       |               +
    //   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    //   * = (low_x, low_y)
    //   . = (high_x, high_y)
    //   / = (high_x, low_y)
    //   \ = (low_x, high_y)
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
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           x, y,
                           x, y,
                           grid.width() - 1, grid.height() - 1,
                           router_opts,
                           measure_directconnect);

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
    VTR_LOG("Computing from left bottom edge (%d,%d):\n", x, y);
#endif
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           x, y,
                           x, y,
                           grid.width() - 1, grid.height() - 1,
                           router_opts,
                           measure_directconnect);

    //Since the other delta delay values may have suffered from edge effects,
    //we recalculate deltas within regions B, C, E, F
#ifdef VERBOSE
    VTR_LOG("Computing from low/low:\n");
#endif
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           low_x, low_y,
                           low_x, low_y,
                           grid.width() - 1, grid.height() - 1,
                           router_opts,
                           measure_directconnect);

    //Since the other delta delay values may have suffered from edge effects,
    //we recalculate deltas within regions D, E, G, H
#ifdef VERBOSE
    VTR_LOG("Computing from high/high:\n");
#endif
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           high_x, high_y,
                           0, 0,
                           high_x, high_y,
                           router_opts,
                           measure_directconnect);

    //Since the other delta delay values may have suffered from edge effects,
    //we recalculate deltas within regions A, B, D, E
#ifdef VERBOSE
    VTR_LOG("Computing from high/low:\n");
#endif
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           high_x, low_y,
                           0, low_y,
                           high_x, grid.height() - 1,
                           router_opts,
                           measure_directconnect);

    //Since the other delta delay values may have suffered from edge effects,
    //we recalculate deltas within regions E, F, H, I
#ifdef VERBOSE
    VTR_LOG("Computing from low/high:\n");
#endif
    generic_compute_matrix(route_profiler, sampled_delta_delays,
                           low_x, high_y,
                           low_x, 0,
                           grid.width() - 1, high_y,
                           router_opts,
                           measure_directconnect);

    vtr::Matrix<float> delta_delays({grid.width(), grid.height()});
    for (size_t dx = 0; dx < sampled_delta_delays.dim_size(0); ++dx) {
        for (size_t dy = 0; dy < sampled_delta_delays.dim_size(1); ++dy) {
            delta_delays[dx][dy] = delay_reduce(sampled_delta_delays[dx][dy], placer_opts.delay_model_reducer);
        }
    }

    return delta_delays;
}

float delay_reduce(std::vector<float>& delays, e_reducer reducer) {
    if (delays.size() == 0) {
        return IMPOSSIBLE_DELTA;
    } else if (delays.size() == 1) {
        return delays[0];
    }

    VTR_ASSERT(delays.size() > 1);

    float delay;

    if (reducer == e_reducer::MIN) {
        auto itr = std::min_element(delays.begin(), delays.end());
        delay = *itr;
    } else if (reducer == e_reducer::MAX) {
        auto itr = std::max_element(delays.begin(), delays.end());
        delay = *itr;
    } else if (reducer == e_reducer::MEDIAN) {
        std::sort(delays.begin(), delays.end());
        delay = vtr::median(delays.begin(), delays.end());
    } else if (reducer == e_reducer::ARITHMEAN) {
        delay = vtr::arithmean(delays.begin(), delays.end());
    } else if (reducer == e_reducer::GEOMEAN) {
        delay = vtr::geomean(delays.begin(), delays.end());
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Unrecognized delta delay reducer");
    }

    return delay;
}

/* We return the average placement estimated delay for a routing spanning (x,y).
 * We start with an averaging distance of 1 (i.e. from (x-1,y-1) to (x+1,y+1))
 * and look for legal delay values to average; if some are found we return the
 * average and if none are found we increase the distance to average over.
 *
 * If no legal values are found to average over with a range of max_distance,
 * we return IMPOSSIBLE_DELTA.
 */
static float find_neightboring_average(
    vtr::Matrix<float>& matrix,
    int x,
    int y,
    int max_distance) {
    float sum = 0;
    int counter = 0;
    int endx = matrix.end_index(0);
    int endy = matrix.end_index(1);

    int delx, dely;

    for (int distance = 1; distance <= max_distance; ++distance) {
        for (delx = x - distance; delx <= x + distance; delx++) {
            for (dely = y - distance; dely <= y + distance; dely++) {
                // Check distance constraint
                if (abs(delx - x) + abs(dely - y) > distance) {
                    continue;
                }

                //check out of bounds
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
        if (counter != 0) {
            return sum / (float)counter;
        }
    }

    return IMPOSSIBLE_DELTA;
}

static void fix_empty_coordinates(vtr::Matrix<float>& delta_delays) {
    // Set any empty delta's to the average of it's neighbours
    //
    // Empty coordinates may occur if the sampling location happens to not have
    // a connection at that location.  However a more through sampling likely
    // would return a result, so we fill in the empty holes with a small
    // neighbour average.
    constexpr int kMaxAverageDistance = 2;
    for (size_t delta_x = 0; delta_x < delta_delays.dim_size(0); ++delta_x) {
        for (size_t delta_y = 0; delta_y < delta_delays.dim_size(1); ++delta_y) {
            if (delta_delays[delta_x][delta_y] == EMPTY_DELTA) {
                delta_delays[delta_x][delta_y] = find_neightboring_average(delta_delays, delta_x, delta_y, kMaxAverageDistance);
            }
        }
    }
}

static void fix_uninitialized_coordinates(vtr::Matrix<float>& delta_delays) {
    // Set any empty delta's to the average of it's neighbours
    for (size_t delta_x = 0; delta_x < delta_delays.dim_size(0); ++delta_x) {
        for (size_t delta_y = 0; delta_y < delta_delays.dim_size(1); ++delta_y) {
            if (delta_delays[delta_x][delta_y] == UNINITIALIZED_DELTA) {
                delta_delays[delta_x][delta_y] = IMPOSSIBLE_DELTA;
            }
        }
    }
}

static void fill_impossible_coordinates(vtr::Matrix<float>& delta_delays) {
    // Set any impossible delta's to the average of it's neighbours
    //
    // Impossible coordinates may occur if an IPIN cannot be reached from the
    // sampling OPIN.  This might occur if the IPIN or OPIN used for sampling
    // is specialized, and therefore cannot be reached via the by the pins
    // sampled.  Leaving this value in the delay matrix will result in invalid
    // slacks if the delay matrix uses this value.
    //
    // A max average distance of 5 is used to provide increased effort in
    // filling these gaps.  It is more important to have a poor predication,
    // than a invalid value and causing a slack assertion.
    constexpr int kMaxAverageDistance = 5;
    for (size_t delta_x = 0; delta_x < delta_delays.dim_size(0); ++delta_x) {
        for (size_t delta_y = 0; delta_y < delta_delays.dim_size(1); ++delta_y) {
            if (delta_delays[delta_x][delta_y] == IMPOSSIBLE_DELTA) {
                delta_delays[delta_x][delta_y] = find_neightboring_average(
                    delta_delays, delta_x, delta_y, kMaxAverageDistance);
            }
        }
    }
}

static vtr::Matrix<float> compute_delta_delay_model(
    const RouterDelayProfiler& route_profiler,
    const t_placer_opts& placer_opts,
    const t_router_opts& router_opts,
    bool measure_directconnect,
    int longest_length) {
    vtr::ScopedStartFinishTimer timer("Computing delta delays");
    vtr::Matrix<float> delta_delays = compute_delta_delays(route_profiler,
                                                           placer_opts, router_opts, measure_directconnect, longest_length);

    fix_uninitialized_coordinates(delta_delays);

    fix_empty_coordinates(delta_delays);

    fill_impossible_coordinates(delta_delays);

    verify_delta_delays(delta_delays);

    return delta_delays;
}

//Finds a src_rr and sink_rr appropriate for measuring the delay of the current direct specification
static bool find_direct_connect_sample_locations(const t_direct_inf* direct,
                                                 t_type_ptr from_type,
                                                 int from_pin,
                                                 int from_pin_class,
                                                 t_type_ptr to_type,
                                                 int to_pin,
                                                 int to_pin_class,
                                                 int* src_rr,
                                                 int* sink_rr) {
    VTR_ASSERT(from_type != nullptr);
    VTR_ASSERT(to_type != nullptr);

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    //Search the grid for an instance of from/to blocks which satisfy this direct connect offsets,
    //and which has the appropriate pins
    int from_x = 0, from_y = 0, from_z = 0;
    int to_x = 0, to_y = 0, to_z = 0;
    bool found = false;
    for (from_x = 0; from_x < (int)grid.width(); ++from_x) {
        to_x = from_x + direct->x_offset;
        if (to_x < 0 || to_x >= (int)grid.width()) continue;

        for (from_y = 0; from_y < (int)grid.height(); ++from_y) {
            if (grid[from_x][from_y].type != from_type) continue;

            //Check that the from pin exists at this from location
            //(with multi-width/height blocks pins may not exist at all locations)
            bool from_pin_found = false;
            if (direct->from_side != NUM_SIDES) {
                int from_pin_rr = get_rr_node_index(device_ctx.rr_node_indices, from_x, from_y, OPIN, from_pin, direct->from_side);
                from_pin_found = (from_pin_rr != OPEN);
            } else {
                std::vector<int> from_pin_rrs = get_rr_node_indices(device_ctx.rr_node_indices, from_x, from_y, OPIN, from_pin);
                from_pin_found = !from_pin_rrs.empty();
            }
            if (!from_pin_found) continue;

            to_y = from_y + direct->y_offset;

            if (to_y < 0 || to_y >= (int)grid.height()) continue;
            if (grid[to_x][to_y].type != to_type) continue;

            //Check that the from pin exists at this from location
            //(with multi-width/height blocks pins may not exist at all locations)
            bool to_pin_found = false;
            if (direct->to_side != NUM_SIDES) {
                int to_pin_rr = get_rr_node_index(device_ctx.rr_node_indices, to_x, to_y, IPIN, to_pin, direct->to_side);
                to_pin_found = (to_pin_rr != OPEN);
            } else {
                std::vector<int> to_pin_rrs = get_rr_node_indices(device_ctx.rr_node_indices, to_x, to_y, IPIN, to_pin);
                to_pin_found = !to_pin_rrs.empty();
            }
            if (!to_pin_found) continue;

            for (from_z = 0; from_z < from_type->capacity; ++from_z) {
                to_z = from_z + direct->z_offset;

                if (to_z < 0 || to_z >= to_type->capacity) continue;

                found = true;
                break;
            }
            if (found) break;
        }
        if (found) break;
    }

    if (!found) {
        return false;
    }

    //Now have a legal instance of this direct connect
    VTR_ASSERT(grid[from_x][from_y].type == from_type);
    VTR_ASSERT(from_z < from_type->capacity);

    VTR_ASSERT(grid[to_x][to_y].type == to_type);
    VTR_ASSERT(to_z < to_type->capacity);

    VTR_ASSERT(from_x + direct->x_offset == to_x);
    VTR_ASSERT(from_y + direct->y_offset == to_y);
    VTR_ASSERT(from_z + direct->z_offset == to_z);

    //
    //Find a source/sink RR node associated with the pins of the direct
    //

    auto source_rr_nodes = get_rr_node_indices(device_ctx.rr_node_indices, from_x, from_y, SOURCE, from_pin_class);
    VTR_ASSERT(source_rr_nodes.size() > 0);

    auto sink_rr_nodes = get_rr_node_indices(device_ctx.rr_node_indices, to_x, to_y, SINK, to_pin_class);
    VTR_ASSERT(sink_rr_nodes.size() > 0);

    *src_rr = source_rr_nodes[0];
    *sink_rr = sink_rr_nodes[0];

    return true;
}

static bool verify_delta_delays(const vtr::Matrix<float>& delta_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            float delta_delay = delta_delays[x][y];

            if (delta_delay < 0.) {
                VPR_ERROR(VPR_ERROR_PLACE,
                          "Found invaild negative delay %g for delta (%d,%d)",
                          delta_delay, x, y);
            }
        }
    }

    return true;
}

void OverrideDelayModel::compute_override_delay_model(
    const RouterDelayProfiler& route_profiler,
    const t_router_opts& router_opts) {
    t_router_opts router_opts2 = router_opts;
    router_opts2.astar_fac = 0.;

    //Look at all the direct connections that exist, and add overrides to delay model
    auto& device_ctx = g_vpr_ctx.device();
    for (int idirect = 0; idirect < device_ctx.arch->num_directs; ++idirect) {
        const t_direct_inf* direct = &device_ctx.arch->Directs[idirect];

        InstPort from_port = parse_inst_port(direct->from_pin);
        InstPort to_port = parse_inst_port(direct->to_pin);

        t_type_ptr from_type = find_block_type_by_name(from_port.instance_name(), device_ctx.block_types, device_ctx.num_block_types);
        t_type_ptr to_type = find_block_type_by_name(to_port.instance_name(), device_ctx.block_types, device_ctx.num_block_types);

        int num_conns = from_port.port_high_index() - from_port.port_low_index() + 1;
        VTR_ASSERT_MSG(num_conns == to_port.port_high_index() - to_port.port_low_index() + 1, "Directs must have the same size to/from");

        //We now walk through all the connections associated with the current direct specification, measure
        //their delay and specify that value as an override in the delay model.
        //
        //Note that we need to check every connection in the direct to cover the case where the pins are not
        //equivalent.
        //
        //However, if the from/to ports are equivalent we could end up sampling the same RR SOURCE/SINK
        //paths multiple times (wasting CPU time) -- we avoid this by recording the sampled paths in
        //sampled_rr_pairs and skipping them if they occur multiple times.
        int missing_instances = 0;
        int missing_paths = 0;
        std::set<std::pair<int, int>> sampled_rr_pairs;
        for (int iconn = 0; iconn < num_conns; ++iconn) {
            //Find the associated pins
            int from_pin = find_pin(from_type, from_port.port_name(), from_port.port_low_index() + iconn);
            int to_pin = find_pin(to_type, to_port.port_name(), to_port.port_low_index() + iconn);

            VTR_ASSERT(from_pin != OPEN);
            VTR_ASSERT(to_pin != OPEN);

            int from_pin_class = find_pin_class(from_type, from_port.port_name(), from_port.port_low_index() + iconn, DRIVER);
            VTR_ASSERT(from_pin_class != OPEN);

            int to_pin_class = find_pin_class(to_type, to_port.port_name(), to_port.port_low_index() + iconn, RECEIVER);
            VTR_ASSERT(to_pin_class != OPEN);

            int src_rr = OPEN;
            int sink_rr = OPEN;
            bool found_sample_points = find_direct_connect_sample_locations(direct, from_type, from_pin, from_pin_class, to_type, to_pin, to_pin_class, &src_rr, &sink_rr);

            if (!found_sample_points) {
                ++missing_instances;
                continue;
            }

            //If some of the source/sink ports are logically equivalent we may have already
            //sampled the associated source/sink pair and don't need to do so again
            if (sampled_rr_pairs.count({src_rr, sink_rr})) continue;

            VTR_ASSERT(src_rr != OPEN);
            VTR_ASSERT(sink_rr != OPEN);

            float direct_connect_delay = std::numeric_limits<float>::quiet_NaN();
            bool found_routing_path = route_profiler.calculate_delay(src_rr, sink_rr, router_opts2, &direct_connect_delay);

            if (found_routing_path) {
                set_delay_override(from_type->index, from_pin_class, to_type->index, to_pin_class, direct->x_offset, direct->y_offset, direct_connect_delay);
            } else {
                ++missing_paths;
            }

            //Record that we've sampled this pair of source and sink nodes
            sampled_rr_pairs.insert({src_rr, sink_rr});
        }

        VTR_LOGV_WARN(missing_instances > 0, "Found no delta delay for %d bits of inter-block direct connect '%s' (no instances of this direct found)\n", missing_instances, direct->name);
        VTR_LOGV_WARN(missing_paths > 0, "Found no delta delay for %d bits of inter-block direct connect '%s' (no routing path found)\n", missing_paths, direct->name);
    }
}

bool directconnect_exists(int src_rr_node, int sink_rr_node) {
    //Returns true if there is a directconnect between the two RR nodes
    //
    //This is checked by looking for a SOURCE -> OPIN -> IPIN -> SINK path
    //which starts at src_rr_node and ends at sink_rr_node
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_nodes = device_ctx.rr_nodes;

    VTR_ASSERT(rr_nodes[src_rr_node].type() == SOURCE && rr_nodes[sink_rr_node].type() == SINK);

    //TODO: This is a constant depth search, but still may be too slow
    for (int i_src_edge = 0; i_src_edge < rr_nodes[src_rr_node].num_edges(); ++i_src_edge) {
        int opin_rr_node = rr_nodes[src_rr_node].edge_sink_node(i_src_edge);

        if (rr_nodes[opin_rr_node].type() != OPIN) continue;

        for (int i_opin_edge = 0; i_opin_edge < rr_nodes[opin_rr_node].num_edges(); ++i_opin_edge) {
            int ipin_rr_node = rr_nodes[opin_rr_node].edge_sink_node(i_opin_edge);

            if (rr_nodes[ipin_rr_node].type() != IPIN) continue;

            for (int i_ipin_edge = 0; i_ipin_edge < rr_nodes[ipin_rr_node].num_edges(); ++i_ipin_edge) {
                if (sink_rr_node == rr_nodes[ipin_rr_node].edge_sink_node(i_ipin_edge)) {
                    return true;
                }
            }
        }
    }
    return false;
}
