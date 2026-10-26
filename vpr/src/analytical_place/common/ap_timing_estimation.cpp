/**
 * @file
 * @author  Alex Singer
 * @date    October 2026
 * @brief   Implementation of methods for estimating the delays of timing arcs
 *          using a flat placement.
 */

#include "ap_timing_estimation.h"
#include <algorithm>
#include <cstdlib>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "PreClusterDelayCalculator.h"
#include "ap_netlist.h"
#include "clb2clb_directs.h"
#include "device_grid.h"
#include "globals.h"
#include "partial_placement.h"
#include "pb_type_graph.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "place_delay_model.h"
#include "router_lookahead_constants.h"
#include "vtr_assert.h"
#include "vtr_hash.h"

/**
 * @brief Queries the delay model at a single reference tile pair (grid center
 *        to center+1) to get a representative delay-per-tile estimate.
 *
 * Used as a fallback when the delay model returns ROUTER_LOOKAHEAD_NO_PATH_SENTINEL
 * for a driver/sink pair. Returns 0.0f if the reference point is also missing.
 *
 * TODO: It is possible that the tile at the center of the device has no possible
 *       routes within one tile unit. We should have a more systematic way of
 *       doing this. For now this is better than just returning 0.0.
 */
static float get_delay_per_tile(const PlaceDelayModel& place_delay_model,
                                const DeviceGrid& device_grid) {
    int cx = (int)device_grid.width() / 2;
    int cy = (int)device_grid.height() / 2;
    t_physical_tile_loc from_loc(cx, cy, 0);
    t_physical_tile_loc to_loc(cx + 1, cy, 0);
    float d = place_delay_model.delay(from_loc, 0, to_loc, 0);
    if (d >= ROUTER_LOOKAHEAD_NO_PATH_SENTINEL)
        return 0.0f;
    return d;
}

/**
 * @brief Get the root location of the tile which contains the given block in
 *        the given flat placement.
 *
 * The root location uniquely identifies a tile, even for tiles larger than 1x1.
 */
static t_physical_tile_loc get_containing_tile_root_loc(APBlockId blk_id,
                                                        const PartialPlacement& p_placement,
                                                        const DeviceGrid& device_grid) {
    t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);
    VTR_ASSERT_SAFE(device_grid.is_valid_tile_loc(tile_loc));
    return device_grid.get_root_location(tile_loc);
}

FlatPlacementArcDelayEstimator::FlatPlacementArcDelayEstimator(const APNetlist& ap_netlist,
                                                               const PreClusterDelayCalculator& delay_calc,
                                                               const PlaceDelayModel& place_delay_model,
                                                               const DeviceGrid& device_grid)
    : ap_netlist_(ap_netlist)
    , place_delay_model_(place_delay_model)
    , device_grid_(device_grid) {
    delay_per_tile_ = get_delay_per_tile(place_delay_model_, device_grid_);

    // Pre-compute the intra-cluster delays of every AP pin. These only depend
    // on the pb_graph pins that the primitive pins are expected to be
    // implemented by, not on the placement.
    // Many primitive pins share the same pb_graph pin, so cache the searches
    // through the pb_graph to save time.
    std::unordered_map<const t_pb_graph_pin*, float> to_boundary_cache;
    std::unordered_map<const t_pb_graph_pin*, float> from_boundary_cache;
    std::unordered_map<std::pair<const t_pb_graph_pin*, const t_pb_graph_pin*>, float, vtr::hash_pair> path_cache;

    pin_cluster_boundary_delay_.resize(ap_netlist_.pins().size(), 0.0f);
    sink_pin_intra_cluster_delay_.resize(ap_netlist_.pins().size(), -1.0f);
    for (APPinId pin_id : ap_netlist_.pins()) {
        const t_pb_graph_pin* gpin = delay_calc.find_pb_graph_pin(ap_netlist_.pin_atom_pin(pin_id));

        // Compute the delay between the pin and the boundary of its cluster.
        // The expected pb_graph pin may not be connected to the boundary of
        // its cluster (for example, a LUT within an arithmetic mode which only
        // drives an adder), even though the primitive will be packed somewhere
        // that is. Use the minimum delay over the equivalent pins, the same as
        // the intra-cluster delays below. If no path to the boundary is found,
        // assume that the delay is small.
        float boundary_delay;
        if (ap_netlist_.pin_type(pin_id) == PinType::DRIVER) {
            auto [it, inserted] = to_boundary_cache.try_emplace(gpin, 0.0f);
            if (inserted)
                it->second = calc_min_equivalent_pin_delay_to_root_pin(gpin);
            boundary_delay = it->second;
        } else {
            auto [it, inserted] = from_boundary_cache.try_emplace(gpin, 0.0f);
            if (inserted)
                it->second = calc_min_equivalent_pin_delay_from_root_pin(gpin);
            boundary_delay = it->second;
        }
        pin_cluster_boundary_delay_[pin_id] = std::max(boundary_delay, 0.0f);

        // For sink pins, compute the intra-cluster delay from the driver of
        // the net, in case the driver and sink end up in the same cluster.
        // The driver and sink may be packed into different primitives than
        // the ones they are expected to be implemented by (for example, a LUT
        // driving an adder may be packed into the LUT that directly drives the
        // adder), so use the minimum path delay between equivalent pins.
        if (ap_netlist_.pin_type(pin_id) != PinType::SINK)
            continue;
        APPinId driver_pin_id = ap_netlist_.net_driver(ap_netlist_.pin_net(pin_id));
        if (!driver_pin_id.is_valid())
            continue;
        const t_pb_graph_pin* driver_gpin = delay_calc.find_pb_graph_pin(ap_netlist_.pin_atom_pin(driver_pin_id));
        auto [it, inserted] = path_cache.try_emplace({driver_gpin, gpin}, -1.0f);
        if (inserted)
            it->second = calc_min_equivalent_pin_path_delay(driver_gpin, gpin);
        sink_pin_intra_cluster_delay_[pin_id] = it->second;
    }

    precompute_direct_arcs_(delay_calc);
}

/// @brief A pin of a physical tile type.
using t_tile_pin = std::pair<t_physical_tile_type_ptr, int>;

/// @brief Switch index used for direct connections which do not specify a
///        switch, which use the delayless switch.
static constexpr int DIRECT_DELAYLESS_SWITCH = -1;

/**
 * @brief Find the physical tile pins which are connected to the given
 *        primitive pb_graph pin, or any pin equivalent to it, within the
 *        cluster.
 *
 * @param pin                       The primitive pb_graph pin.
 * @param forward                   If true, find the tile pins which can be
 *                                  reached from the pin (for driver pins);
 *                                  otherwise, find the tile pins which can
 *                                  reach the pin (for sink pins).
 * @param root_to_logical_block     The logical block type of each root
 *                                  pb_graph node.
 */
static std::set<t_tile_pin> find_connected_tile_pins(const t_pb_graph_pin* pin,
                                                     bool forward,
                                                     const std::unordered_map<const t_pb_graph_node*, t_logical_block_type_ptr>& root_to_logical_block) {
    std::set<t_tile_pin> tile_pins;
    for (const t_pb_graph_pin* equivalent_pin : find_equivalent_pb_graph_pins(pin)) {
        std::vector<const t_pb_graph_pin*> stack = {equivalent_pin};
        std::unordered_set<const t_pb_graph_pin*> visited = {equivalent_pin};
        while (!stack.empty()) {
            const t_pb_graph_pin* cur_pin = stack.back();
            stack.pop_back();

            // Root block pins are on the boundary of the cluster. Find the
            // pins of every physical tile that can implement the cluster.
            if (cur_pin->is_root_block_pin()) {
                t_logical_block_type_ptr logical_block = root_to_logical_block.at(cur_pin->parent_node);
                for (t_physical_tile_type_ptr physical_tile : logical_block->equivalent_tiles) {
                    int physical_pin = get_physical_pin(physical_tile, logical_block, cur_pin->pin_count_in_cluster);
                    tile_pins.insert({physical_tile, physical_pin});
                }
                continue;
            }

            const std::vector<t_pb_graph_edge*>& edges = forward ? cur_pin->output_edges : cur_pin->input_edges;
            for (const t_pb_graph_edge* edge : edges) {
                int num_pins = forward ? edge->num_output_pins : edge->num_input_pins;
                for (int ipin = 0; ipin < num_pins; ipin++) {
                    const t_pb_graph_pin* next_pin = forward ? edge->output_pins[ipin] : edge->input_pins[ipin];
                    if (visited.insert(next_pin).second)
                        stack.push_back(next_pin);
                }
            }
        }
    }
    return tile_pins;
}

void FlatPlacementArcDelayEstimator::precompute_direct_arcs_(const PreClusterDelayCalculator& delay_calc) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const std::vector<t_direct_inf>& directs = device_ctx.arch->directs;
    if (directs.empty())
        return;

    // Resolve the tile pins of each direct connection.
    std::vector<t_clb_to_clb_directs> clb_to_clb_directs = alloc_and_load_clb_to_clb_directs(directs, DIRECT_DELAYLESS_SWITCH);
    VTR_ASSERT(clb_to_clb_directs.size() == directs.size());

    std::unordered_map<const t_pb_graph_node*, t_logical_block_type_ptr> root_to_logical_block;
    for (const t_logical_block_type& logical_block : device_ctx.logical_block_types) {
        if (logical_block.pb_graph_head != nullptr)
            root_to_logical_block[logical_block.pb_graph_head] = &logical_block;
    }

    // Many primitive pins share the same pb_graph pins, so cache the
    // connected tile pins and the direct arcs of each pair of pb_graph pins.
    std::unordered_map<const t_pb_graph_pin*, std::set<t_tile_pin>> driver_tile_pins_cache;
    std::unordered_map<const t_pb_graph_pin*, std::set<t_tile_pin>> sink_tile_pins_cache;
    std::unordered_map<std::pair<const t_pb_graph_pin*, const t_pb_graph_pin*>, std::vector<t_direct_arc>, vtr::hash_pair> direct_arcs_cache;

    for (APPinId sink_pin_id : ap_netlist_.pins()) {
        if (ap_netlist_.pin_type(sink_pin_id) != PinType::SINK)
            continue;
        APPinId driver_pin_id = ap_netlist_.net_driver(ap_netlist_.pin_net(sink_pin_id));
        if (!driver_pin_id.is_valid())
            continue;
        const t_pb_graph_pin* driver_gpin = delay_calc.find_pb_graph_pin(ap_netlist_.pin_atom_pin(driver_pin_id));
        const t_pb_graph_pin* sink_gpin = delay_calc.find_pb_graph_pin(ap_netlist_.pin_atom_pin(sink_pin_id));

        auto [it, inserted] = direct_arcs_cache.try_emplace({driver_gpin, sink_gpin});
        if (inserted) {
            auto [driver_it, driver_inserted] = driver_tile_pins_cache.try_emplace(driver_gpin);
            if (driver_inserted)
                driver_it->second = find_connected_tile_pins(driver_gpin, /*forward=*/true, root_to_logical_block);
            auto [sink_it, sink_inserted] = sink_tile_pins_cache.try_emplace(sink_gpin);
            if (sink_inserted)
                sink_it->second = find_connected_tile_pins(sink_gpin, /*forward=*/false, root_to_logical_block);
            const std::set<t_tile_pin>& driver_tile_pins = driver_it->second;
            const std::set<t_tile_pin>& sink_tile_pins = sink_it->second;

            // The arc may use a direct connection if some source pin of the
            // direct is connected to the driver and the corresponding sink
            // pin of the direct is connected to the sink.
            for (size_t idirect = 0; idirect < directs.size(); idirect++) {
                const t_clb_to_clb_directs& clb_direct = clb_to_clb_directs[idirect];
                int num_pins = std::abs(clb_direct.from_clb_pin_end_index - clb_direct.from_clb_pin_start_index) + 1;
                int from_step = clb_direct.from_clb_pin_end_index >= clb_direct.from_clb_pin_start_index ? 1 : -1;
                int to_step = clb_direct.to_clb_pin_end_index >= clb_direct.to_clb_pin_start_index ? 1 : -1;
                for (int ipin = 0; ipin < num_pins; ipin++) {
                    int from_pin = clb_direct.from_clb_pin_start_index + ipin * from_step;
                    int to_pin = clb_direct.to_clb_pin_start_index + ipin * to_step;
                    if (driver_tile_pins.count({clb_direct.from_clb_type, from_pin}) == 0
                        || sink_tile_pins.count({clb_direct.to_clb_type, to_pin}) == 0)
                        continue;
                    // The direct connection is the only driver of its sink
                    // pin, so use a fan-in of 1 if the switch delay depends
                    // on the fan-in.
                    float delay = 0.0f;
                    if (clb_direct.switch_index != DIRECT_DELAYLESS_SWITCH) {
                        const t_arch_switch_inf& direct_switch = device_ctx.arch_switch_inf[clb_direct.switch_index];
                        delay = direct_switch.fixed_Tdel() ? direct_switch.Tdel() : direct_switch.Tdel(1);
                    }
                    it->second.push_back({directs[idirect].x_offset, directs[idirect].y_offset, delay});
                    break;
                }
            }
        }

        if (!it->second.empty())
            sink_pin_direct_arcs_[sink_pin_id] = it->second;
    }
}

float FlatPlacementArcDelayEstimator::get_direct_delay_(APPinId sink_pin_id,
                                                        const t_physical_tile_loc& driver_loc,
                                                        const t_physical_tile_loc& sink_loc) const {
    auto it = sink_pin_direct_arcs_.find(sink_pin_id);
    if (it == sink_pin_direct_arcs_.end() || driver_loc.layer_num != sink_loc.layer_num)
        return -1.0f;
    for (const t_direct_arc& direct_arc : it->second) {
        if (sink_loc.x - driver_loc.x == direct_arc.dx && sink_loc.y - driver_loc.y == direct_arc.dy)
            return direct_arc.delay;
    }
    return -1.0f;
}

float FlatPlacementArcDelayEstimator::get_reference_routing_delay_(const t_physical_tile_loc& driver_loc,
                                                                   const t_physical_tile_loc& sink_loc) const {
    int dx = std::abs(driver_loc.x - sink_loc.x);
    int dy = std::abs(driver_loc.y - sink_loc.y);

    // Query the delay model for the same distance from the center of the
    // device. Go in whichever direction stays within the device, clamping to
    // the edge of the device if neither direction fits.
    int width = static_cast<int>(device_grid_.width());
    int height = static_cast<int>(device_grid_.height());
    int ref_x = width / 2;
    int ref_y = height / 2;
    auto offset_within = [](int ref, int delta, int size) {
        if (ref + delta < size)
            return ref + delta;
        if (ref - delta >= 0)
            return ref - delta;
        return std::clamp(ref + delta, 0, size - 1);
    };
    t_physical_tile_loc ref_driver_loc(ref_x, ref_y, driver_loc.layer_num);
    t_physical_tile_loc ref_sink_loc(offset_within(ref_x, dx, width),
                                     offset_within(ref_y, dy, height),
                                     sink_loc.layer_num);
    float ref_delay = place_delay_model_.delay(ref_driver_loc, 0 /*from_pin*/, ref_sink_loc, 0 /*to_pin*/);
    if (ref_delay < ROUTER_LOOKAHEAD_NO_PATH_SENTINEL)
        return ref_delay;

    // If the reference location also has no entry, fall back on a simple
    // distance-based estimate. This is pessimistic for long distances, since
    // it does not account for long wires.
    return (dx + dy) * delay_per_tile_;
}

float FlatPlacementArcDelayEstimator::estimate_inter_cluster_arc_delay_(APPinId driver_pin_id,
                                                                        APPinId sink_pin_id,
                                                                        const PartialPlacement& p_placement) const {
    // Get the root locations of the tiles containing the driver and sink
    // blocks. The placer queries the delay model using the root locations of
    // the clusters, so we do the same here.
    // TODO: The from and to pins are not known since they are cluster-level
    //       pins, and the clusters have not been created yet. The delay models
    //       that we care about do not use the pins yet.
    t_physical_tile_loc driver_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(driver_pin_id),
                                                                  p_placement,
                                                                  device_grid_);
    t_physical_tile_loc sink_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(sink_pin_id),
                                                                p_placement,
                                                                device_grid_);
    float routing_delay = place_delay_model_.delay(driver_loc,
                                                   0 /*from_pin*/,
                                                   sink_loc,
                                                   0 /*to_pin*/);

    // The delay model returns ROUTER_LOOKAHEAD_NO_PATH_SENTINEL when it has
    // no entry for this driver/sink pair (a gap in the model). For example,
    // the default delay model looks up the delay by the type of the driver's
    // tile, and the table for IO tiles may be missing some distances. Use the
    // delay of the same distance from a reference location instead, so the
    // arc is not treated as free.
    if (routing_delay >= ROUTER_LOOKAHEAD_NO_PATH_SENTINEL)
        routing_delay = get_reference_routing_delay_(driver_loc, sink_loc);

    return pin_cluster_boundary_delay_[driver_pin_id]
           + routing_delay
           + pin_cluster_boundary_delay_[sink_pin_id];
}

e_flat_placement_arc_type FlatPlacementArcDelayEstimator::get_arc_type(APPinId sink_pin_id,
                                                                       const PartialPlacement& p_placement) const {
    VTR_ASSERT_SAFE(ap_netlist_.pin_type(sink_pin_id) == PinType::SINK);
    APNetId net_id = ap_netlist_.pin_net(sink_pin_id);

    // Global nets are not routed through the general routing network, so
    // these arcs only have intra-cluster delays.
    // TODO: This is only true for ideal clock modeling. With other clock
    //       modeling options (e.g. route or dedicated_network) clock nets
    //       are routed and will have a routing delay.
    // NOTE: The AP netlist speculatively marks any net connected to a clock
    //       port or a non-clock global port as global.
    if (ap_netlist_.net_is_global(net_id))
        return e_flat_placement_arc_type::UNROUTED_GLOBAL;

    // Constant nets which are not routed (see --constant_net_method) have no
    // routing delay.
    if (ap_netlist_.net_is_constant(net_id) && ap_netlist_.net_is_ignored(net_id))
        return e_flat_placement_arc_type::UNROUTED_CONSTANT;

    // If the driver and sink blocks are in the same tile, assume that they
    // will be packed into the same cluster and use the intra-cluster delay
    // between them.
    // NOTE: This ignores the case where two blocks in the same tile are in
    //       different sub-tiles (for example, two IO blocks placed in the same
    //       IO tile). In that case there is usually no intra-cluster path
    //       between the pins, and the arc is routed between clusters.
    if (sink_pin_intra_cluster_delay_[sink_pin_id] >= 0.0f) {
        APPinId driver_pin_id = ap_netlist_.net_driver(net_id);
        VTR_ASSERT_SAFE_MSG(driver_pin_id.is_valid(),
                            "Cannot estimate the delay of an arc without a driver");
        APBlockId driver_blk_id = ap_netlist_.pin_block(driver_pin_id);
        APBlockId sink_blk_id = ap_netlist_.pin_block(sink_pin_id);
        t_physical_tile_loc driver_loc = get_containing_tile_root_loc(driver_blk_id, p_placement, device_grid_);
        t_physical_tile_loc sink_loc = get_containing_tile_root_loc(sink_blk_id, p_placement, device_grid_);
        if (driver_loc == sink_loc)
            return e_flat_placement_arc_type::INTRA_CLUSTER;
    }

    // If the driver and sink tiles are at the offset of a direct connection
    // which can implement this arc, assume that the direct connection is used.
    if (sink_pin_direct_arcs_.count(sink_pin_id) != 0) {
        APPinId driver_pin_id = ap_netlist_.net_driver(net_id);
        VTR_ASSERT_SAFE(driver_pin_id.is_valid());
        t_physical_tile_loc driver_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(driver_pin_id), p_placement, device_grid_);
        t_physical_tile_loc sink_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(sink_pin_id), p_placement, device_grid_);
        if (get_direct_delay_(sink_pin_id, driver_loc, sink_loc) >= 0.0f)
            return e_flat_placement_arc_type::INTER_CLUSTER_DIRECT;
    }

    // Otherwise, the arc must be routed between clusters.
    return e_flat_placement_arc_type::INTER_CLUSTER;
}

float FlatPlacementArcDelayEstimator::estimate_arc_delay(APPinId sink_pin_id,
                                                         const PartialPlacement& p_placement) const {
    switch (get_arc_type(sink_pin_id, p_placement)) {
        case e_flat_placement_arc_type::UNROUTED_GLOBAL: {
            // Global nets are not routed, but the arc still goes through the
            // driver and sink clusters. This matches the post-cluster delay
            // calculator, which gives these arcs the intra-cluster delays to
            // and from the cluster pins with no routing delay between them.
            APPinId driver_pin_id = ap_netlist_.net_driver(ap_netlist_.pin_net(sink_pin_id));
            if (!driver_pin_id.is_valid())
                return pin_cluster_boundary_delay_[sink_pin_id];
            return pin_cluster_boundary_delay_[driver_pin_id] + pin_cluster_boundary_delay_[sink_pin_id];
        }
        case e_flat_placement_arc_type::UNROUTED_CONSTANT:
            return 0.0f;
        case e_flat_placement_arc_type::INTRA_CLUSTER:
            return sink_pin_intra_cluster_delay_[sink_pin_id];
        case e_flat_placement_arc_type::INTER_CLUSTER_DIRECT: {
            // The arc goes through the driver and sink clusters and the
            // switch of the direct connection between them.
            APPinId driver_pin_id = ap_netlist_.net_driver(ap_netlist_.pin_net(sink_pin_id));
            VTR_ASSERT_SAFE(driver_pin_id.is_valid());
            t_physical_tile_loc driver_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(driver_pin_id), p_placement, device_grid_);
            t_physical_tile_loc sink_loc = get_containing_tile_root_loc(ap_netlist_.pin_block(sink_pin_id), p_placement, device_grid_);
            float direct_delay = get_direct_delay_(sink_pin_id, driver_loc, sink_loc);
            VTR_ASSERT_SAFE(direct_delay >= 0.0f);
            return pin_cluster_boundary_delay_[driver_pin_id]
                   + direct_delay
                   + pin_cluster_boundary_delay_[sink_pin_id];
        }
        case e_flat_placement_arc_type::INTER_CLUSTER:
        default: {
            APPinId driver_pin_id = ap_netlist_.net_driver(ap_netlist_.pin_net(sink_pin_id));
            VTR_ASSERT_SAFE_MSG(driver_pin_id.is_valid(),
                                "Cannot estimate the delay of an arc without a driver");
            return estimate_inter_cluster_arc_delay_(driver_pin_id, sink_pin_id, p_placement);
        }
    }
}

void FlatPlacementArcDelayEstimator::update_arc_delays(const PartialPlacement& p_placement,
                                                       PreClusterDelayCalculator& delay_calc) const {
    // For each AP sink pin, update the delay of the timing arc going through it.
    // The delay calculator operates on the Atom netlist; however, by construction
    // of the AP netlist, every atom pin corresponds 1to1 to an AP pin.
    for (APPinId ap_pin_id : ap_netlist_.pins()) {
        // Timing arcs are uniquely identified by the sink pin. Only update
        // timing for sink pins.
        if (ap_netlist_.pin_type(ap_pin_id) != PinType::SINK)
            continue;

        float delay = estimate_arc_delay(ap_pin_id, p_placement);
        delay_calc.set_arc_delay(ap_netlist_.pin_atom_pin(ap_pin_id), delay);
    }
}
