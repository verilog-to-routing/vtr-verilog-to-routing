
#pragma once

#include "vtr_ndmatrix.h"
#include "physical_types.h"
#include "rr_graph_fwd.h"

struct t_placer_opts;
struct t_router_opts;
class RouterDelayProfiler;

vtr::NdMatrix<float, 4> compute_delta_delay_model(RouterDelayProfiler& route_profiler,
                                                  const t_placer_opts& placer_opts,
                                                  const t_router_opts& router_opts,
                                                  bool measure_directconnect,
                                                  int longest_length,
                                                  bool is_flat);

bool find_direct_connect_sample_locations(const t_direct_inf* direct,
                                          t_physical_tile_type_ptr from_type,
                                          int from_pin,
                                          int from_pin_class,
                                          t_physical_tile_type_ptr to_type,
                                          int to_pin,
                                          int to_pin_class,
                                          RRNodeId& out_src_node,
                                          RRNodeId& out_sink_node);

/**
 * @brief Identifies the best pin classes for delay calculation based on pin count and connectivity.
 *
 * This function selects pin classes of a specified type (`pintype`) from a physical tile type (`type`)
 * that are suitable for delay calculations. It prioritizes pin classes with the largest number of pins
 * that connect to general routing, ensuring commonly used pins are chosen for delay profiling.
 *
 * @param pintype The type of pins to filter.
 * @param type Pointer to the physical tile type containing pin and class information.
 *
 * @return A vector of indices representing the selected pin classes. The classes are sorted
 *         in descending order based on the number of pins they contain.
 *
 * @details
 * - A pin class is eligible if its type matches `pintype` and it contains at least one pin
 *   that connects to general routing (non-zero Fc).
 * - Non-zero Fc pins are determined by inspecting the tile's `fc_specs`.
 * - Classes are sorted so that the class with the largest number of pins appears first.
 *   If multiple classes have the same pin count, their order depends on their initial appearance
 *   in the architecture file.
 *
 * @note
 * - Pins explicitly marked as ignored in `type->is_ignored_pin` are excluded.
 * - The function ensures stability in sorting, preserving the input order for classes
 *   with the same number of pins.
 */

std::vector<int> get_best_classes(enum e_pin_type pintype, t_physical_tile_type_ptr type);