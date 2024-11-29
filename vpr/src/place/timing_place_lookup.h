#ifndef TIMING_PLACE_LOOKUP_H
#define TIMING_PLACE_LOOKUP_H
#include "place_delay_model.h"

std::unique_ptr<PlaceDelayModel> compute_place_delay_model(const t_placer_opts& placer_opts,
                                                           const t_router_opts& router_opts,
                                                           const Netlist<>& net_list,
                                                           t_det_routing_arch* det_routing_arch,
                                                           std::vector<t_segment_inf>& segment_inf,
                                                           t_chan_width_dist chan_width_dist,
                                                           const std::vector<t_direct_inf>& directs,
                                                           bool is_flat);

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

bool directconnect_exists(RRNodeId src_rr_node, RRNodeId sink_rr_node);

#endif
