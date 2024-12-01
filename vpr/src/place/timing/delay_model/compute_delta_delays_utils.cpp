
#include "compute_delta_delays_utils.h"

#include "vtr_time.h"
#include "vtr_math.h"
#include "physical_types.h"
#include "globals.h"
#include "router_delay_profiling.h"

/// Indicates the delta delay value has not been calculated
static constexpr float UNINITIALIZED_DELTA = -1;
/// Indicates delta delay from/to an EMPTY block
static constexpr float EMPTY_DELTA = -2;
/// Indicates there is no valid delta delay
static constexpr float IMPOSSIBLE_DELTA = std::numeric_limits<float>::infinity();

static vtr::NdMatrix<float, 4> compute_delta_delays(RouterDelayProfiler& route_profiler,
                                                    const t_placer_opts& palcer_opts,
                                                    const t_router_opts& router_opts,
                                                    bool measure_directconnect,
                                                    size_t longest_length,
                                                    bool is_flat);

static void fix_empty_coordinates(vtr::NdMatrix<float, 4>& delta_delays);

static void fill_impossible_coordinates(vtr::NdMatrix<float, 4>& delta_delays);

static bool verify_delta_delays(const vtr::NdMatrix<float, 4>& delta_delays);

static void generic_compute_matrix_iterative_astar(RouterDelayProfiler& route_profiler,
                                                   vtr::Matrix<std::vector<float>>& matrix,
                                                   int from_layer_num,
                                                   int to_layer_num,
                                                   int source_x,
                                                   int source_y,
                                                   int start_x,
                                                   int start_y,
                                                   int end_x,
                                                   int end_y,
                                                   const t_router_opts& router_opts,
                                                   bool measure_directconnect,
                                                   const std::set<std::string>& allowed_types,
                                                   bool /*is_flat*/);

static void generic_compute_matrix_dijkstra_expansion(RouterDelayProfiler& route_profiler,
                                                      vtr::Matrix<std::vector<float>>& matrix,
                                                      int from_layer_num,
                                                      int to_layer_num,
                                                      int source_x,
                                                      int source_y,
                                                      int start_x,
                                                      int start_y,
                                                      int end_x,
                                                      int end_y,
                                                      const t_router_opts& router_opts,
                                                      bool measure_directconnect,
                                                      const std::set<std::string>& allowed_types,
                                                      bool is_flat);

/**
 * @brief Routes between a source and sink location to calculate the delay.
 *
 * This function computes the delay of a routed connection between a source and sink node
 * specified by their coordinates and layers. It iterates over the best driver and sink pin
 * classes to find a valid routing path and calculates the delay if a path exists.
 *
 * @param route_profiler Reference to the `RouterDelayProfiler` responsible for calculating routing delays.
 * @param source_x The x-coordinate of the source location.
 * @param source_y The y-coordinate of the source location.
 * @param source_layer The layer index of the source node.
 * @param sink_x The x-coordinate of the sink location.
 * @param sink_y The y-coordinate of the sink location.
 * @param sink_layer The layer index of the sink node.
 * @param router_opts Routing options used for delay calculation.
 * @param measure_directconnect If `true`, includes direct connect delays; otherwise, skips direct connections.
 *
 * @return The calculated routing delay. If routing fails, it returns `IMPOSSIBLE_DELTA`.
 */
static float route_connection_delay(RouterDelayProfiler& route_profiler,
                                    int source_x,
                                    int source_y,
                                    int source_layer,
                                    int sink_x,
                                    int sink_y,
                                    int sink_layer,
                                    const t_router_opts& router_opts,
                                    bool measure_directconnect);

/**
 * @brief Computes a reduced value from a vector of delay values using the specified reduction method.
 *
 * @param delays A reference to a vector of delay values. This vector may be modified
 *               (e.g., sorted) depending on the reducer used.
 * @param reducer The reduction method to be applied.
 *
 * @return The reduced delay value. If the input vector is empty, the function
 *         returns `IMPOSSIBLE_DELTA`.
 *
 * @throws VPR_FATAL_ERROR if the reducer is unrecognized.
 */
static float delay_reduce(std::vector<float>& delays, e_reducer reducer);

/**
 * @brief Adds a delay value to a 2D matrix of delay vectors.
 *
 * Updates the delay vector at position (`delta_x`, `delta_y`) in the matrix.
 * If the element contains only `EMPTY_DELTA`, it is replaced with the new delay;
 * otherwise, the delay is appended to the vector.
 *
 * @param matrix A 2D matrix of delay vectors.
 * @param delta_x The x-index in the matrix.
 * @param delta_y The y-index in the matrix.
 * @param delay The delay value to add.
 */
static void add_delay_to_matrix(vtr::Matrix<std::vector<float>>& matrix,
                                int delta_x,
                                int delta_y,
                                float delay);

/**
 * @brief Computes the average delay for a routing span.
 *
 * This function calculates the average placement delay for a routing span starting from a
 * given layer and spanning a region defined by delta x and delta y. It iteratively searches
 * for valid delay values within an expanding neighborhood  (starting from a distance of 1)
 * around the specified delta offsets and layer, until valid  values are found or
 * the maximum search distance (`max_distance`) is reached.
 *
 * @param matrix A 4D matrix of delay values indexed by `[from_layer][to_layer][delta_x][delta_y]`.
 * @param from_layer The starting layer index of the routing span.
 * @param to_tile_loc A structure holding the delta offsets (`x` and `y`) and the target layer index (`layer_num`).
 * @param max_distance The maximum neighborhood distance to search for valid delay values.
 *
 * @return The average of valid delay values within the search range. If no valid delays
 *         are found up to the maximum distance, the function returns `IMPOSSIBLE_DELTA`.
 *
 * @note The function performs a Manhattan-distance-based neighborhood search around the target location.
 */
static float find_neighboring_average(vtr::NdMatrix<float, 4>& matrix,
                                      int from_layer,
                                      t_physical_tile_loc to_tile_loc,
                                      int max_distance);

/***************************************************************************************/

static vtr::NdMatrix<float, 4> compute_delta_delays(RouterDelayProfiler& route_profiler,
                                                    const t_placer_opts& placer_opts,
                                                    const t_router_opts& router_opts,
                                                    bool measure_directconnect,
                                                    size_t longest_length,
                                                    bool is_flat) {


    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    const size_t num_layers = grid.get_num_layers();
    const size_t device_width = grid.width();
    const size_t device_height = grid.height();

    /* To avoid edge effects we place the source at least 'longest_length' away
     * from the device edge and route from there for all possible delta values < dimension
     */

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
    const size_t mid_x = vtr::nint(device_width / 2);
    const size_t mid_y = vtr::nint(device_height / 2);
    const size_t low_x = std::min(longest_length, mid_x);
    const size_t low_y = std::min(longest_length, mid_y);
    const size_t high_x = (longest_length <= device_width)  ? std::max(device_width - longest_length, mid_x) : mid_x;
    const size_t high_y = (longest_length <= device_height) ? std::max(device_width - longest_length, mid_y) : mid_y;

    vtr::NdMatrix<float, 4> delta_delays({num_layers, num_layers, device_width, device_height});

    std::set<std::string> allowed_types;
    if (!placer_opts.allowed_tiles_for_delay_model.empty()) {
        std::vector<std::string> allowed_types_vector = vtr::split(placer_opts.allowed_tiles_for_delay_model, ",");
        allowed_types = std::set(allowed_types_vector.begin(), allowed_types_vector.end());
    }

    for (int from_layer_num = 0; from_layer_num < (int)num_layers; from_layer_num++) {
        for (int to_layer_num = 0; to_layer_num < (int)num_layers; to_layer_num++) {
            vtr::NdMatrix<std::vector<float>, 2> sampled_delta_delays({device_width, device_height});

            // Find the lowest y location on the left edge with a non-empty block
            int y = 0;
            int x = 0;
            t_physical_tile_type_ptr src_type = nullptr;
            for (x = 0; x < (int)device_width; ++x) {
                for (y = 0; y < (int)device_height; ++y) {
                    t_physical_tile_type_ptr type = grid.get_physical_type({x, y, from_layer_num});

                    if (type != device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                        // check if the tile type is among the allowed types
                        if (!allowed_types.empty() && allowed_types.find(type->name) == allowed_types.end()) {
                            continue;
                        }
                        src_type = type;
                        break;
                    }
                }
                if (src_type != nullptr) {
                    break;
                }
            }
            VTR_ASSERT(src_type != nullptr);

            auto generic_compute_matrix = (placer_opts.place_delta_delay_matrix_calculation_method == e_place_delta_delay_algorithm::ASTAR_ROUTE) ? generic_compute_matrix_iterative_astar : generic_compute_matrix_dijkstra_expansion;

#ifdef VERBOSE
            VTR_LOG("Computing from lower left edge (%d,%d):\n", x, y);
#endif
            generic_compute_matrix(route_profiler, sampled_delta_delays,
                                   from_layer_num, to_layer_num,
                                   x, y,
                                   x, y,
                                   device_width - 1, device_height - 1,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);

            // Find the lowest x location on the bottom edge with a non-empty block
            src_type = nullptr;
            for (y = 0; y < (int)device_height; ++y) {
                for (x = 0; x < (int)device_width; ++x) {
                    t_physical_tile_type_ptr type = grid.get_physical_type({x, y, from_layer_num});

                    if (type != device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                        // check if the tile type is among the allowed types
                        if (!allowed_types.empty() && allowed_types.find(type->name) == allowed_types.end()) {
                            continue;
                        }
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
                                   from_layer_num, to_layer_num,
                                   x, y,
                                   x, y,
                                   device_width - 1, device_height - 1,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);

            //Since the other delta delay values may have suffered from edge effects,
            //we recalculate deltas within regions B, C, E, F
#ifdef VERBOSE
            VTR_LOG("Computing from low/low:\n");
#endif
            generic_compute_matrix(route_profiler, sampled_delta_delays,
                                   from_layer_num, to_layer_num,
                                   low_x, low_y,
                                   low_x, low_y,
                                   device_width - 1, device_height - 1,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);

            //Since the other delta delay values may have suffered from edge effects,
            //we recalculate deltas within regions D, E, G, H
#ifdef VERBOSE
            VTR_LOG("Computing from high/high:\n");
#endif
            generic_compute_matrix(route_profiler, sampled_delta_delays,
                                   from_layer_num, to_layer_num,
                                   high_x, high_y,
                                   0, 0,
                                   high_x, high_y,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);

            //Since the other delta delay values may have suffered from edge effects,
            //we recalculate deltas within regions A, B, D, E
#ifdef VERBOSE
            VTR_LOG("Computing from high/low:\n");
#endif
            generic_compute_matrix(route_profiler, sampled_delta_delays,
                                   from_layer_num, to_layer_num,
                                   high_x, low_y,
                                   0, low_y,
                                   high_x, device_height - 1,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);

            //Since the other delta delay values may have suffered from edge effects,
            //we recalculate deltas within regions E, F, H, I
#ifdef VERBOSE
            VTR_LOG("Computing from low/high:\n");
#endif
            generic_compute_matrix(route_profiler, sampled_delta_delays,
                                   from_layer_num, to_layer_num,
                                   low_x, high_y,
                                   low_x, 0,
                                   device_width - 1, high_y,
                                   router_opts,
                                   measure_directconnect, allowed_types,
                                   is_flat);
            for (size_t dx = 0; dx < sampled_delta_delays.dim_size(0); ++dx) {
                for (size_t dy = 0; dy < sampled_delta_delays.dim_size(1); ++dy) {
                    delta_delays[from_layer_num][to_layer_num][dx][dy] = delay_reduce(sampled_delta_delays[dx][dy], placer_opts.delay_model_reducer);
                }
            }
        }
    }

    return delta_delays;
}

static void fix_empty_coordinates(vtr::NdMatrix<float, 4>& delta_delays) {
    // Set any empty delta's to the average of its neighbours
    //
    // Empty coordinates may occur if the sampling location happens to not have
    // a connection at that location. However, a more thorough sampling likely
    // would return a result, so we fill in the empty holes with a small
    // neighbour average.
    constexpr int kMaxAverageDistance = 2;
    for (int from_layer = 0; from_layer < (int)delta_delays.dim_size(0); ++from_layer) {
        for (int to_layer = 0; to_layer < (int)delta_delays.dim_size(1); ++to_layer) {
            for (int delta_x = 0; delta_x < (int)delta_delays.dim_size(2); ++delta_x) {
                for (int delta_y = 0; delta_y < (int)delta_delays.dim_size(3); ++delta_y) {
                    if (delta_delays[from_layer][to_layer][delta_x][delta_y] == EMPTY_DELTA) {
                        delta_delays[from_layer][to_layer][delta_x][delta_y] =
                            find_neighboring_average(delta_delays,
                                                     from_layer,
                                                     {delta_x, delta_y, to_layer},
                                                     kMaxAverageDistance);
                    }
                }
            }
        }
    }
}

static void fill_impossible_coordinates(vtr::NdMatrix<float, 4>& delta_delays) {
    // Set any impossible delta's to the average of its neighbours
    //
    // Impossible coordinates may occur if an IPIN cannot be reached from the
    // sampling OPIN.  This might occur if the IPIN or OPIN used for sampling
    // is specialized, and therefore cannot be reached via the by the pins
    // sampled.  Leaving this value in the delay matrix will result in invalid
    // slacks if the delay matrix uses this value.
    //
    // A max average distance of 5 is used to provide increased effort in
    // filling these gaps.  It is more important to have a poor predication,
    // than an invalid value and causing a slack assertion.
    constexpr int kMaxAverageDistance = 5;
    for (int from_layer_num = 0; from_layer_num < (int)delta_delays.dim_size(0); ++from_layer_num) {
        for (int to_layer_num = 0; to_layer_num < (int)delta_delays.dim_size(1); ++to_layer_num) {
            for (int delta_x = 0; delta_x < (int)delta_delays.dim_size(2); ++delta_x) {
                for (int delta_y = 0; delta_y < (int)delta_delays.dim_size(3); ++delta_y) {
                    if (delta_delays[from_layer_num][to_layer_num][delta_x][delta_y] == IMPOSSIBLE_DELTA) {
                        delta_delays[from_layer_num][to_layer_num][delta_x][delta_y] = find_neighboring_average(
                            delta_delays, from_layer_num, {delta_x, delta_y, to_layer_num}, kMaxAverageDistance);
                    }
                }
            }
        }
    }
}

static bool verify_delta_delays(const vtr::NdMatrix<float, 4>& delta_delays) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    for (int from_layer_num = 0; from_layer_num < grid.get_num_layers(); ++from_layer_num) {
        for (int to_layer_num = 0; to_layer_num < grid.get_num_layers(); ++to_layer_num) {
            for (size_t x = 0; x < grid.width(); ++x) {
                for (size_t y = 0; y < grid.height(); ++y) {
                    float delta_delay = delta_delays[from_layer_num][to_layer_num][x][y];

                    if (delta_delay < 0.) {
                        VPR_ERROR(VPR_ERROR_PLACE,
                                  "Found invalid negative delay %g for delta [%d,%d,%d,%d]",
                                  delta_delay, from_layer_num, to_layer_num, x, y);
                    }
                }
            }
        }
    }

    return true;
}

static void generic_compute_matrix_iterative_astar(RouterDelayProfiler& route_profiler,
                                                   vtr::Matrix<std::vector<float>>& matrix,
                                                   int from_layer_num,
                                                   int to_layer_num,
                                                   int source_x,
                                                   int source_y,
                                                   int start_x,
                                                   int start_y,
                                                   int end_x,
                                                   int end_y,
                                                   const t_router_opts& router_opts,
                                                   bool measure_directconnect,
                                                   const std::set<std::string>& allowed_types,
                                                   bool /*is_flat*/) {
    const auto& device_ctx = g_vpr_ctx.device();

    for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
            const int delta_x = abs(sink_x - source_x);
            const int delta_y = abs(sink_y - source_y);

            t_physical_tile_type_ptr src_type = device_ctx.grid.get_physical_type({source_x, source_y, from_layer_num});
            t_physical_tile_type_ptr sink_type = device_ctx.grid.get_physical_type({sink_x, sink_y, to_layer_num});

            bool src_or_target_empty = (src_type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE
                                        || sink_type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE);

            bool is_allowed_type = allowed_types.empty() || allowed_types.find(src_type->name) != allowed_types.end();

            if (src_or_target_empty || !is_allowed_type) {
                if (matrix[delta_x][delta_y].empty()) {
                    // Only set empty target if we don't already have a valid delta delay
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
                // Valid start/end
                float delay = route_connection_delay(route_profiler,
                                                     source_x,
                                                     source_y,
                                                     from_layer_num,
                                                     sink_x,
                                                     sink_y,
                                                     to_layer_num,
                                                     router_opts,
                                                     measure_directconnect);

#ifdef VERBOSE
                VTR_LOG("Computed delay: %12g delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                        delay,
                        delta_x, delta_y,
                        source_x, source_y,
                        sink_x, sink_y);
#endif
                if (matrix[delta_x][delta_y].size() == 1 && matrix[delta_x][delta_y][0] == EMPTY_DELTA) {
                    // Overwrite empty delta
                    matrix[delta_x][delta_y][0] = delay;
                } else {
                    // Collect delta
                    matrix[delta_x][delta_y].push_back(delay);
                }
            }
        }
    }
}

static void generic_compute_matrix_dijkstra_expansion(RouterDelayProfiler& /*route_profiler*/,
                                                      vtr::Matrix<std::vector<float>>& matrix,
                                                      int from_layer_num,
                                                      int to_layer_num,
                                                      int source_x,
                                                      int source_y,
                                                      int start_x,
                                                      int start_y,
                                                      int end_x,
                                                      int end_y,
                                                      const t_router_opts& router_opts,
                                                      bool measure_directconnect,
                                                      const std::set<std::string>& allowed_types,
                                                      bool is_flat) {
    const auto& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr src_type = device_ctx.grid.get_physical_type({source_x, source_y, from_layer_num});
    bool is_allowed_type = allowed_types.empty() || allowed_types.find(src_type->name) != allowed_types.end();
    if (src_type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE || !is_allowed_type) {
        for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
            for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
                int delta_x = abs(sink_x - source_x);
                int delta_y = abs(sink_y - source_y);

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
            }
        }

        return;
    }

    vtr::Matrix<bool> found_matrix({matrix.dim_size(0), matrix.dim_size(1)}, false);

    auto best_driver_ptcs = get_best_classes(DRIVER, device_ctx.grid.get_physical_type({source_x, source_y, from_layer_num}));
    for (int driver_ptc : best_driver_ptcs) {
        VTR_ASSERT(driver_ptc != OPEN);
        RRNodeId source_rr_node = device_ctx.rr_graph.node_lookup().find_node(from_layer_num, source_x, source_y, SOURCE, driver_ptc);

        VTR_ASSERT(source_rr_node != RRNodeId::INVALID());
        auto delays = calculate_all_path_delays_from_rr_node(source_rr_node, router_opts, is_flat);

        bool path_to_all_sinks = true;
        for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
            for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
                int delta_x = abs(sink_x - source_x);
                int delta_y = abs(sink_y - source_y);

                if (found_matrix[delta_x][delta_y]) {
                    continue;
                }

                t_physical_tile_type_ptr sink_type = device_ctx.grid.get_physical_type({sink_x, sink_y, to_layer_num});
                if (sink_type == device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                    if (matrix[delta_x][delta_y].empty()) {
                        // Only set empty target if we don't already have a valid delta delay
                        matrix[delta_x][delta_y].push_back(EMPTY_DELTA);
#ifdef VERBOSE
                        VTR_LOG("Computed delay: %12s delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                                "EMPTY",
                                delta_x, delta_y,
                                source_x, source_y,
                                sink_x, sink_y);
#endif
                        found_matrix[delta_x][delta_y] = true;
                    }
                } else {
                    bool found_a_sink = false;
                    auto best_sink_ptcs = get_best_classes(RECEIVER, device_ctx.grid.get_physical_type({sink_x, sink_y, to_layer_num}));
                    for (int sink_ptc : best_sink_ptcs) {
                        VTR_ASSERT(sink_ptc != OPEN);
                        RRNodeId sink_rr_node = device_ctx.rr_graph.node_lookup().find_node(to_layer_num, sink_x, sink_y, SINK, sink_ptc);

                        if (sink_rr_node == RRNodeId::INVALID())
                            continue;

                        if (!measure_directconnect && directconnect_exists(source_rr_node, sink_rr_node)) {
                            // Skip if we shouldn't measure direct connects and a direct connect exists
                            continue;
                        }

                        if (std::isnan(delays[sink_rr_node])) {
                            // This sink was not found
                            continue;
                        }

#ifdef VERBOSE
                        VTR_LOG("Computed delay: %12g delta: %d,%d (src: %d,%d sink: %d,%d)\n",
                                delays[size_t(sink_rr_node)],
                                delta_x, delta_y,
                                source_x, source_y,
                                sink_x, sink_y);
#endif
                        found_matrix[delta_x][delta_y] = true;

                        add_delay_to_matrix(matrix, delta_x, delta_y, delays[sink_rr_node]);

                        found_a_sink = true;
                        break;
                    }

                    if (!found_a_sink) {
                        path_to_all_sinks = false;
                    }
                }
            }
        }

        if (path_to_all_sinks) {
            break;
        }
    }

    for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
            int delta_x = abs(sink_x - source_x);
            int delta_y = abs(sink_y - source_y);
            if (!found_matrix[delta_x][delta_y]) {
                add_delay_to_matrix(matrix, delta_x, delta_y, IMPOSSIBLE_DELTA);
                VTR_LOG_WARN("Unable to route between blocks at (%d,%d,%d) and (%d,%d,%d) to characterize delay (setting to %g)\n",
                             source_x,
                             source_y,
                             from_layer_num,
                             sink_x,
                             sink_y,
                             to_layer_num,
                             IMPOSSIBLE_DELTA);
            }
        }
    }
}

static float route_connection_delay(RouterDelayProfiler& route_profiler,
                                    int source_x,
                                    int source_y,
                                    int source_layer,
                                    int sink_x,
                                    int sink_y,
                                    int sink_layer,
                                    const t_router_opts& router_opts,
                                    bool measure_directconnect) {
    //Routes between the source and sink locations and calculates the delay

    // set to known value for debug purposes
    float net_delay_value = IMPOSSIBLE_DELTA;

    const auto& device_ctx = g_vpr_ctx.device();

    bool successfully_routed = false;

    // Get the rr nodes to route between
    auto best_driver_ptcs = get_best_classes(DRIVER, device_ctx.grid.get_physical_type({source_x, source_y, source_layer}));
    auto best_sink_ptcs = get_best_classes(RECEIVER, device_ctx.grid.get_physical_type({sink_x, sink_y, sink_layer}));

    for (int driver_ptc : best_driver_ptcs) {
        VTR_ASSERT(driver_ptc != OPEN);
        RRNodeId source_rr_node = device_ctx.rr_graph.node_lookup().find_node(source_layer, source_x, source_y, SOURCE, driver_ptc);

        VTR_ASSERT(source_rr_node != RRNodeId::INVALID());

        for (int sink_ptc : best_sink_ptcs) {
            VTR_ASSERT(sink_ptc != OPEN);
            RRNodeId sink_rr_node = device_ctx.rr_graph.node_lookup().find_node(sink_layer, sink_x, sink_y, SINK, sink_ptc);

            if (sink_rr_node == RRNodeId::INVALID())
                continue;

            if (!measure_directconnect && directconnect_exists(source_rr_node, sink_rr_node)) {
                // Skip if we shouldn't measure direct connects and a direct connect exists
                continue;
            }

            successfully_routed = route_profiler.calculate_delay(source_rr_node,
                                                                 sink_rr_node,
                                                                 router_opts,
                                                                 &net_delay_value);

            if (successfully_routed) break;
        }
        if (successfully_routed) break;
    }

    if (!successfully_routed) {
        VTR_LOG_WARN("Unable to route between blocks at (%d,%d,%d) and (%d,%d,%d) to characterize delay (setting to %g)\n",
                     source_x, source_y, source_layer, sink_x, sink_y, sink_layer, net_delay_value);
    }

    return net_delay_value;
}

static float delay_reduce(std::vector<float>& delays, e_reducer reducer) {
    if (delays.empty()) {
        return IMPOSSIBLE_DELTA;
    }

    if (delays.size() == 1) {
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
        std::stable_sort(delays.begin(), delays.end());
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

static void add_delay_to_matrix(vtr::Matrix<std::vector<float>>& matrix,
                                int delta_x,
                                int delta_y,
                                float delay) {
    if (matrix[delta_x][delta_y].size() == 1 && matrix[delta_x][delta_y][0] == EMPTY_DELTA) {
        // Overwrite empty delta
        matrix[delta_x][delta_y][0] = delay;
    } else {
        // Collect delta
        matrix[delta_x][delta_y].push_back(delay);
    }
}

static float find_neighboring_average(vtr::NdMatrix<float, 4>& matrix,
                                      int from_layer,
                                      t_physical_tile_loc to_tile_loc,
                                      int max_distance) {
    float sum = 0.f;
    int num_samples = 0;
    const int endx = matrix.end_index(2);
    const int endy = matrix.end_index(3);

    const int x = to_tile_loc.x;
    const int y = to_tile_loc.y;
    const int to_layer = to_tile_loc.layer_num;

    for (int distance = 1; distance <= max_distance; ++distance) {
        for (int delx = x - distance; delx <= x + distance; delx++) {
            for (int dely = y - distance; dely <= y + distance; dely++) {
                // Check distance constraint
                if (abs(delx - x) + abs(dely - y) > distance) {
                    continue;
                }

                //check out of bounds
                if (delx < 0 || dely < 0 || delx >= endx || dely >= endy || (delx == x && dely == y)) {
                    continue;
                }

                if (matrix[from_layer][to_layer][delx][dely] == EMPTY_DELTA || matrix[from_layer][to_layer][delx][dely] == IMPOSSIBLE_DELTA) {
                    continue;
                }

                sum += matrix[from_layer][to_layer][delx][dely];
                num_samples++;
            }
        }

        if (num_samples != 0) {
            return sum / (float)num_samples;
        }
    }

    return IMPOSSIBLE_DELTA;
}

/***************************************************************************************/

vtr::NdMatrix<float, 4> compute_delta_delay_model(RouterDelayProfiler& route_profiler,
                                                  const t_placer_opts& placer_opts,
                                                  const t_router_opts& router_opts,
                                                  bool measure_directconnect,
                                                  int longest_length,
                                                  bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Computing delta delays");
    vtr::NdMatrix<float, 4> delta_delays = compute_delta_delays(route_profiler,
                                                                placer_opts,
                                                                router_opts,
                                                                measure_directconnect,
                                                                longest_length,
                                                                is_flat);

    const size_t num_elements = delta_delays.size();

    // set uninitialized elements to infinity
    for (size_t i = 0; i < num_elements; i++) {
        if (delta_delays.get(i) == UNINITIALIZED_DELTA) {
            delta_delays.get(i) = IMPOSSIBLE_DELTA;
        }
    }

    fix_empty_coordinates(delta_delays);

    fill_impossible_coordinates(delta_delays);

    verify_delta_delays(delta_delays);

    return delta_delays;
}

//Finds a src_rr and sink_rr appropriate for measuring the delay of the current direct specification
bool find_direct_connect_sample_locations(const t_direct_inf* direct,
                                          t_physical_tile_type_ptr from_type,
                                          int from_pin,
                                          int from_pin_class,
                                          t_physical_tile_type_ptr to_type,
                                          int to_pin,
                                          int to_pin_class,
                                          RRNodeId& out_src_node,
                                          RRNodeId& out_sink_node) {
    VTR_ASSERT(from_type != nullptr);
    VTR_ASSERT(to_type != nullptr);

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    const auto& node_lookup = device_ctx.rr_graph.node_lookup();

    //Search the grid for an instance of from/to blocks which satisfy this direct connect offsets,
    //and which has the appropriate pins
    int from_x = -1;
    int from_y = -1;
    int from_sub_tile = -1;
    int to_x = 0, to_y = 0, to_sub_tile = 0;
    bool found = false;
    int found_layer_num = -1;
    //TODO: Function *FOR NOW* assumes that from/to blocks are at same die and have a same layer nums
    for (int layer_num = 0; layer_num < grid.get_num_layers() && !found; ++layer_num) {
        for (int x = 0; x < (int)grid.width() && !found; ++x) {
            to_x = x + direct->x_offset;
            if (to_x < 0 || to_x >= (int)grid.width()) continue;

            for (int y = 0; y < (int)grid.height() && !found; ++y) {
                if (grid.get_physical_type({x, y, layer_num}) != from_type) continue;

                //Check that the from pin exists at this from location
                //(with multi-width/height blocks pins may not exist at all locations)
                bool from_pin_found = false;
                if (direct->from_side != NUM_2D_SIDES) {
                    RRNodeId from_pin_rr = node_lookup.find_node(layer_num, x, y, OPIN, from_pin, direct->from_side);
                    from_pin_found = from_pin_rr.is_valid();
                } else {
                    from_pin_found = !(node_lookup.find_nodes_at_all_sides(layer_num, x, y, OPIN, from_pin).empty());
                }
                if (!from_pin_found) continue;

                to_y = y + direct->y_offset;

                if (to_y < 0 || to_y >= (int)grid.height()) continue;
                if (grid.get_physical_type({to_x, to_y, layer_num}) != to_type) continue;

                //Check that the from pin exists at this from location
                //(with multi-width/height blocks pins may not exist at all locations)
                bool to_pin_found = false;
                if (direct->to_side != NUM_2D_SIDES) {
                    RRNodeId to_pin_rr = node_lookup.find_node(layer_num, to_x, to_y, IPIN, to_pin, direct->to_side);
                    to_pin_found = (to_pin_rr != RRNodeId::INVALID());
                } else {
                    to_pin_found = !(node_lookup.find_nodes_at_all_sides(layer_num, to_x, to_y, IPIN, to_pin).empty());
                }
                if (!to_pin_found) continue;

                for (int sub_tile_num = 0; sub_tile_num < from_type->capacity; ++sub_tile_num) {
                    to_sub_tile = sub_tile_num + direct->sub_tile_offset;

                    if (to_sub_tile < 0 || to_sub_tile >= to_type->capacity) continue;

                    found = true;
                    found_layer_num = layer_num;
                    from_x = x;
                    from_y = y;
                    from_sub_tile = sub_tile_num;

                    break;
                }
            }
        }
    }

    if (!found) {
        return false;
    }

    //Now have a legal instance of this direct connect
    VTR_ASSERT(grid.get_physical_type({from_x, from_y, found_layer_num}) == from_type);
    VTR_ASSERT(from_sub_tile < from_type->capacity);

    VTR_ASSERT(grid.get_physical_type({to_x, to_y, found_layer_num}) == to_type);
    VTR_ASSERT(to_sub_tile < to_type->capacity);

    VTR_ASSERT(from_x + direct->x_offset == to_x);
    VTR_ASSERT(from_y + direct->y_offset == to_y);
    VTR_ASSERT(from_sub_tile + direct->sub_tile_offset == to_sub_tile);

    // Find a source/sink RR node associated with the pins of the direct
    {
        RRNodeId src_rr_candidate = node_lookup.find_node(found_layer_num, from_x, from_y, SOURCE, from_pin_class);
        VTR_ASSERT(src_rr_candidate);
        out_src_node = src_rr_candidate;
    }

    {
        RRNodeId sink_rr_candidate = node_lookup.find_node(found_layer_num, to_x, to_y, SINK, to_pin_class);
        VTR_ASSERT(sink_rr_candidate);
        out_sink_node = sink_rr_candidate;
    }

    return true;
}

std::vector<int> get_best_classes(enum e_pin_type pintype, t_physical_tile_type_ptr type) {
    std::vector<int> best_classes;

    //Record any non-zero Fc pins
    //
    //Note that we track non-zero Fc pins, since certain Fc overrides
    //may apply to only a subset of wire types. This ensures we record
    //which pins can potentially connect to global routing.
    std::unordered_set<int> non_zero_fc_pins;
    for (const t_fc_specification& fc_spec : type->fc_specs) {
        if (fc_spec.fc_value == 0) continue;

        non_zero_fc_pins.insert(fc_spec.pins.begin(), fc_spec.pins.end());
    }

    // Collect all classes of matching type which connect to general routing
    for (int i = 0; i < (int)type->class_inf.size(); i++) {
        if (type->class_inf[i].type == pintype) {
            //Check whether all pins in this class are ignored or have zero fc
            bool any_pins_connect_to_general_routing = false;
            for (int ipin = 0; ipin < type->class_inf[i].num_pins; ++ipin) {
                int pin = type->class_inf[i].pinlist[ipin];
                //If the pin isn't ignored, and has a non-zero Fc to some general
                //routing the class is suitable for delay profiling
                if (!type->is_ignored_pin[pin] && non_zero_fc_pins.count(pin)) {
                    any_pins_connect_to_general_routing = true;
                    break;
                }
            }

            // Skip if the pin class doesn't connect to general routing
            if (!any_pins_connect_to_general_routing) continue;

            // Record candidate class
            best_classes.push_back(i);
        }
    }

    // Sort classes so the largest pin class is first
    auto cmp_class = [&](int lhs, int rhs) {
        return type->class_inf[lhs].num_pins > type->class_inf[rhs].num_pins;
    };

    std::stable_sort(best_classes.begin(), best_classes.end(), cmp_class);

    return best_classes;
}