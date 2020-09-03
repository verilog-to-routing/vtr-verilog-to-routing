#include "router_lookahead_extended_map.h"

#include <vector>
#include <queue>

#include "rr_node.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_sampling.h"
#include "globals.h"
#include "vtr_math.h"
#include "vtr_time.h"
#include "vtr_geometry.h"
#include "echo_files.h"
#include "rr_graph.h"

#include "route_timing.h"
#include "route_common.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "extended_map_lookahead.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif

#if defined(VPR_USE_TBB)
#    include <tbb/parallel_for_each.h>
#    include <tbb/mutex.h>
#endif

/* we're profiling routing cost over many tracks for each wire type, so we'll
 * have many cost entries at each |dx|,|dy| offset. There are many ways to
 * "boil down" the many costs at each offset to a single entry for a given
 * (wire type, chan_type) combination we can take the smallest cost, the
 * average, median, etc. This define selects the method we use.
 *
 * NOTE: Currently, only SMALLEST is supported.
 *
 * See e_representative_entry_method */
#define REPRESENTATIVE_ENTRY_METHOD util::SMALLEST

#define CONNECTION_BOX_LOOKAHEAD_MAP_PRINT_COST_MAPS

// Don't continue storing a path after hitting a lower-or-same cost entry.
static constexpr bool BREAK_ON_MISS = false;

///@brief Threshold to set a limit the paths exploration during the dijkstra expansion for a particualr sample region.
static constexpr int MIN_PATH_COUNT = 1000;

template<typename Entry>
static std::pair<float, int> run_dijkstra(int start_node_ind,
                                          std::vector<bool>* node_expanded,
                                          std::vector<util::Search_Path>* paths,
                                          util::RoutingCosts* routing_costs);

float ExtendedMapLookahead::get_src_opin_cost(RRNodeId from_node, int delta_x, int delta_y, float criticality_fac) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    //When estimating costs from a SOURCE/OPIN we look-up to find which wire types (and the
    //cost to reach them) in f_src_opin_delays. Once we know what wire types are
    //reachable, we query the f_wire_cost_map (i.e. the wire lookahead) to get the final
    //delay to reach the sink.

    t_physical_tile_type_ptr tile_type = device_ctx.grid[rr_graph.node_xlow(from_node)][rr_graph.node_ylow(from_node)].type;
    auto tile_index = std::distance(&device_ctx.physical_tile_types[0], tile_type);

    auto from_ptc = rr_graph.node_ptc_num(from_node);

    if (this->src_opin_delays[tile_index][from_ptc].empty()) {
        //During lookahead profiling we were unable to find any wires which connected
        //to this PTC.
        //
        //This can sometimes occur at very low channel widths (e.g. during min W search on
        //small designs) where W discretization combined with fraction Fc may cause some
        //pins/sources to be left disconnected.
        //
        //Such RR graphs are of course unroutable, but that should be determined by the
        //router. So just return an arbitrary value here rather than error.

        //We choose to return the largest (non-infinite) value possible, but scaled
        //down by a large factor to maintain some dynaimc range in case this value ends
        //up being processed (e.g. by the timing analyzer).
        //
        //The cost estimate should still be *extremely* large compared to a typical delay, and
        //so should ensure that the router de-prioritizes exploring this path, but does not
        //forbid the router from trying.
        return std::numeric_limits<float>::max() / 1e12;
    } else {
        //From the current SOURCE/OPIN we look-up the wiretypes which are reachable
        //and then add the estimates from those wire types for the distance of interest.
        //If there are multiple options we use the minimum value.
        float expected_cost = std::numeric_limits<float>::infinity();

        for (const auto& kv : this->src_opin_delays[tile_index][from_ptc]) {
            const util::t_reachable_wire_inf& reachable_wire_inf = kv.second;

            util::Cost_Entry cost_entry;
            if (reachable_wire_inf.wire_rr_type == SINK) {
                //Some pins maybe reachable via a direct (OPIN -> IPIN) connection.
                //In the lookahead, we treat such connections as 'special' wire types
                //with no delay/congestion cost
                cost_entry.delay = 0;
                cost_entry.congestion = 0;
            } else {
                //For an actual accessible wire, we query the wire look-up to get it's
                //delay and congestion cost estimates
                cost_entry = cost_map_.find_cost(reachable_wire_inf.wire_seg_index, delta_x, delta_y);
            }

            float this_cost = (criticality_fac) * (reachable_wire_inf.congestion + cost_entry.congestion)
                              + (1. - criticality_fac) * (reachable_wire_inf.delay + cost_entry.delay);

            expected_cost = std::min(this_cost, expected_cost);
        }

        VTR_ASSERT(std::isfinite(expected_cost));
        return expected_cost;
    }

    VTR_ASSERT_SAFE_MSG(false,
                        vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                        rr_node_arch_name(size_t(from_node)).c_str(),
                                        describe_rr_node(size_t(from_node)).c_str())
                            .c_str());
}

float ExtendedMapLookahead::get_chan_ipin_delays(RRNodeId to_node) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    auto to_tile_type = device_ctx.grid[rr_graph.node_xlow(to_node)][rr_graph.node_ylow(to_node)].type;
    auto to_tile_index = to_tile_type->index;

    auto to_ptc = rr_graph.node_ptc_num(to_node);

    float site_pin_delay = 0.f;
    if (this->chan_ipins_delays[to_tile_index].size() != 0) {
        auto reachable_wire_inf = this->chan_ipins_delays[to_tile_index][to_ptc];

        site_pin_delay = reachable_wire_inf.delay;
    }

    return site_pin_delay;
}

// derive a cost from the map between two nodes
float ExtendedMapLookahead::get_map_cost(int from_node_ind,
                                         int to_node_ind,
                                         float criticality_fac) const {
    if (from_node_ind == to_node_ind) {
        return 0.f;
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    RRNodeId to_node(to_node_ind);
    RRNodeId from_node(from_node_ind);

    int from_x = rr_graph.node_xlow(from_node);
    int from_y = rr_graph.node_ylow(from_node);

    int to_x = rr_graph.node_xlow(to_node);
    int to_y = rr_graph.node_ylow(to_node);

    int dx, dy;
    dx = to_x - from_x;
    dy = to_y - from_y;

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == SOURCE || from_type == OPIN) {
        return this->get_src_opin_cost(from_node, dx, dy, criticality_fac);
    }

    int from_seg_index = cost_map_.node_to_segment(from_node_ind);
    util::Cost_Entry cost_entry = cost_map_.find_cost(from_seg_index, dx, dy);

    if (!cost_entry.valid()) {
        // there is no route
        VTR_LOGV_DEBUG(f_router_debug,
                       "Not connected %d (%s, %d) -> %d (%s)\n",
                       from_node_ind, device_ctx.rr_nodes[from_node_ind].type_string(), from_seg_index,
                       to_node_ind, device_ctx.rr_nodes[to_node_ind].type_string());
        return std::numeric_limits<float>::infinity();
    }

    float expected_delay = cost_entry.delay;
    float expected_congestion = cost_entry.congestion;

    float site_pin_delay = this->get_chan_ipin_delays(to_node);
    expected_delay += site_pin_delay;

    float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;

    VTR_LOGV_DEBUG(f_router_debug, "Requested lookahead from node %d to %d\n", from_node_ind, to_node_ind);
    const std::string& segment_name = device_ctx.rr_segments[from_seg_index].name;
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead returned %s (%d) with distance (%zd, %zd)\n",
                   segment_name.c_str(), from_seg_index,
                   dx, dy);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead delay: %g\n", expected_delay);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead congestion: %g\n", expected_congestion);
    VTR_LOGV_DEBUG(f_router_debug, "Criticality: %g\n", criticality_fac);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead cost: %g\n", expected_cost);
    VTR_LOGV_DEBUG(f_router_debug, "Site pin delay: %g\n", site_pin_delay);

    if (!std::isfinite(expected_cost)) {
        VTR_LOG_ERROR("infinite cost for segment %d at (%d, %d)\n", from_seg_index, (int)dx, (int)dy);
        VTR_ASSERT(0);
    }

    if (expected_cost < 0.f) {
        VTR_LOG_ERROR("negative cost for segment %d at (%d, %d)\n", from_seg_index, (int)dx, (int)dy);
        VTR_ASSERT(0);
    }

    return expected_cost;
}

// add a best cost routing path from start_node_ind to node_ind to routing costs
template<typename Entry>
bool ExtendedMapLookahead::add_paths(int start_node_ind,
                                     Entry current,
                                     const std::vector<util::Search_Path>& paths,
                                     util::RoutingCosts* routing_costs) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    int node_ind = current.rr_node_ind;
    RRNodeId node(node_ind);

    bool new_sample_found = false;

    float site_pin_delay = this->get_chan_ipin_delays(node);

    // reconstruct the path
    std::vector<int> path;
    for (int i = paths[node_ind].parent; i != start_node_ind; i = paths[i].parent) {
        VTR_ASSERT(i != -1);
        path.push_back(i);
    }
    path.push_back(start_node_ind);

    current.adjust_Tsw(-site_pin_delay);

    // add each node along the path subtracting the incremental costs from the current costs
    Entry start_to_here(start_node_ind, UNDEFINED, nullptr);
    int parent = start_node_ind;
    for (auto it = path.rbegin(); it != path.rend(); it++) {
        RRNodeId this_node(*it);
        auto& here = device_ctx.rr_nodes[*it];
        int seg_index = device_ctx.rr_indexed_data[here.cost_index()].seg_index;

        int from_x = rr_graph.node_xlow(this_node);
        int from_y = rr_graph.node_ylow(this_node);

        int to_x = rr_graph.node_xlow(node);
        int to_y = rr_graph.node_ylow(node);

        int delta_x, delta_y;
        delta_x = to_x - from_x;
        delta_y = to_y - from_y;

        vtr::Point<int> delta(delta_x, delta_y);

        util::RoutingCostKey key = {
            seg_index,
            delta};

        if (*it != start_node_ind) {
            auto& parent_node = device_ctx.rr_nodes[parent];
            start_to_here = Entry(*it, parent_node.edge_switch(paths[*it].edge), &start_to_here);
            parent = *it;
        }

        float cost = current.cost() - start_to_here.cost();
        if (cost < 0.f && cost > -10e-15 /* 10 femtosecond */) {
            cost = 0.f;
        }

        VTR_ASSERT(cost >= 0.f);

        // NOTE: implements REPRESENTATIVE_ENTRY_METHOD == SMALLEST
        auto result = routing_costs->insert(std::make_pair(key, cost));
        if (!result.second) {
            if (cost < result.first->second) {
                result.first->second = cost;
                new_sample_found = true;
            } else if (BREAK_ON_MISS) {
                break;
            }
        } else {
            new_sample_found = true;
        }
    }
    return new_sample_found;
}

/* Runs Dijkstra's algorithm from specified node until all nodes have been
 * visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored to an entry in the routing_cost_map.
 *
 * Returns the maximum (last) minimum cost path stored, and
 * the number of paths from start_node_ind stored. */
template<typename Entry>
std::pair<float, int> ExtendedMapLookahead::run_dijkstra(int start_node_ind,
                                                         std::vector<bool>* node_expanded,
                                                         std::vector<util::Search_Path>* paths,
                                                         util::RoutingCosts* routing_costs) {
    auto& device_ctx = g_vpr_ctx.device();
    int path_count = 0;

    /* a list of boolean flags (one for each rr node) to figure out if a
     * certain node has already been expanded */
    std::fill(node_expanded->begin(), node_expanded->end(), false);
    /* For each node keep a list of the cost with which that node has been
     * visited (used to determine whether to push a candidate node onto the
     * expansion queue.
     * Also store the parent node so we can reconstruct a specific path. */
    std::fill(paths->begin(), paths->end(), util::Search_Path{std::numeric_limits<float>::infinity(), -1, -1});
    /* a priority queue for expansion */
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

    /* first entry has no upstream delay or congestion */
    Entry first_entry(start_node_ind, UNDEFINED, nullptr);
    float max_cost = first_entry.cost();

    pq.push(first_entry);

    /* now do routing */
    while (!pq.empty()) {
        auto current = pq.top();
        pq.pop();

        int node_ind = current.rr_node_ind;

        /* check that we haven't already expanded from this node */
        if ((*node_expanded)[node_ind]) {
            continue;
        }

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (device_ctx.rr_nodes[node_ind].type() == IPIN) {
            // the last cost should be the highest
            max_cost = current.cost();

            path_count++;
            this->add_paths<Entry>(start_node_ind, current, *paths, routing_costs);
        } else {
            util::expand_dijkstra_neighbours(device_ctx.rr_nodes,
                                             current, paths, node_expanded, &pq);
            (*node_expanded)[node_ind] = true;
        }
    }
    return std::make_pair(max_cost, path_count);
}

// compute the cost maps for lookahead
void ExtendedMapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    this->src_opin_delays = util::compute_router_src_opin_lookahead();
    this->chan_ipins_delays = util::compute_router_chan_ipin_lookahead();

    vtr::ScopedStartFinishTimer timer("Computing connection box lookahead map");

    // Initialize rr_node_route_inf if not already
    alloc_and_load_rr_node_route_structs();

    size_t num_segments = segment_inf.size();
    std::vector<SampleRegion> sample_regions = find_sample_regions(num_segments);

    /* free previous delay map and allocate new one */
    auto& device_ctx = g_vpr_ctx.device();
    cost_map_.set_counts(segment_inf.size());
    cost_map_.build_segment_map();

    VTR_ASSERT(REPRESENTATIVE_ENTRY_METHOD == util::SMALLEST);
    util::RoutingCosts all_delay_costs;
    util::RoutingCosts all_base_costs;

    /* run Dijkstra's algorithm for each segment type & channel type combination */
#if defined(VPR_USE_TBB)
    tbb::mutex all_costs_mutex;
    tbb::parallel_for_each(sample_regions, [&](const SampleRegion& region) {
#else
    for (const auto& region : sample_regions) {
#endif
        // holds the cost entries for a run
        util::RoutingCosts delay_costs;
        util::RoutingCosts base_costs;
        int total_path_count = 0;
        std::vector<bool> node_expanded(device_ctx.rr_nodes.size());
        std::vector<util::Search_Path> paths(device_ctx.rr_nodes.size());

        for (auto& point : region.points) {
            // statistics
            vtr::Timer run_timer;
            float max_delay_cost = 0.f;
            float max_base_cost = 0.f;
            int path_count = 0;
            for (auto node_ind : point.nodes) {
                {
                    auto result = run_dijkstra<util::PQ_Entry_Delay>(node_ind, &node_expanded, &paths, &delay_costs);
                    max_delay_cost = std::max(max_delay_cost, result.first);
                    path_count += result.second;
                }
                {
                    auto result = run_dijkstra<util::PQ_Entry_Base_Cost>(node_ind, &node_expanded, &paths, &base_costs);
                    max_base_cost = std::max(max_base_cost, result.first);
                    path_count += result.second;
                }
            }

            if (path_count > 0) {
                VTR_LOG("Expanded %d paths of segment type %s(%d) starting at (%d, %d) from %d segments, max_cost %e %e (%g paths/sec)\n",
                        path_count, segment_inf[region.segment_type].name.c_str(), region.segment_type,
                        point.location.x(), point.location.y(),
                        (int)point.nodes.size(),
                        max_delay_cost, max_base_cost,
                        path_count / run_timer.elapsed_sec());
            }

            total_path_count += path_count;
            if (total_path_count > MIN_PATH_COUNT) {
                break;
            }
        }

#if defined(VPR_USE_TBB)
        all_costs_mutex.lock();
#endif

        if (total_path_count == 0) {
            VTR_LOG_WARN("No paths found for sample region %s(%d, %d)\n",
                         segment_inf[region.segment_type].name.c_str(), region.grid_location.x(), region.grid_location.y());
        }

        // combine the cost map from this run with the final cost maps for each segment
        for (const auto& cost : delay_costs) {
            const auto& val = cost.second;
            auto result = all_delay_costs.insert(std::make_pair(cost.first, val));
            if (!result.second) {
                // implements REPRESENTATIVE_ENTRY_METHOD == SMALLEST
                result.first->second = std::min(result.first->second, val);
            }
        }
        for (const auto& cost : base_costs) {
            const auto& val = cost.second;
            auto result = all_base_costs.insert(std::make_pair(cost.first, val));
            if (!result.second) {
                // implements REPRESENTATIVE_ENTRY_METHOD == SMALLEST
                result.first->second = std::min(result.first->second, val);
            }
        }

#if defined(VPR_USE_TBB)
        all_costs_mutex.unlock();
    });
#else
    }
#endif

    VTR_LOG("Combining results\n");
    /* boil down the cost list in routing_cost_map at each coordinate to a
     * representative cost entry and store it in the lookahead cost map */
    cost_map_.set_cost_map(all_delay_costs, all_base_costs);

// diagnostics
#if defined(CONNECTION_BOX_LOOKAHEAD_MAP_PRINT_COST_ENTRIES)
    for (auto& cost : all_costs) {
        const auto& key = cost.first;
        const auto& val = cost.second;
        VTR_LOG("%d -> %d (%d, %d): %g, %g\n",
                val.from_node, val.to_node,
                key.delta.x(), key.delta.y(),
                val.cost_entry.delay, val.cost_entry.congestion);
    }
#endif

#if defined(CONNECTION_BOX_LOOKAHEAD_MAP_PRINT_COST_MAPS)
    for (int iseg = 0; iseg < (ssize_t)num_segments; iseg++) {
        VTR_LOG("cost map for %s(%d)\n",
                segment_inf[iseg].name.c_str(), iseg);
        cost_map_.print(iseg);
    }
#endif

#if defined(CONNECTION_BOX_LOOKAHEAD_MAP_PRINT_EMPTY_MAPS)
    for (std::pair<int, int> p : cost_map_.list_empty()) {
        int ichan, iseg;
        std::tie(ichan, iseg) = p;
        VTR_LOG("cost map for %s(%d), chan %d EMPTY\n",
                segment_inf[iseg].name.c_str(), iseg, box_id);
    }
#endif
}

// get an expected minimum cost for routing from the current node to the target node
float ExtendedMapLookahead::get_expected_cost(
    int current_node,
    int target_node,
    const t_conn_cost_params& params,
    float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_nodes[current_node].type();

    if (rr_type == CHANX || rr_type == CHANY || rr_type == SOURCE || rr_type == OPIN) {
        return get_map_cost(
            current_node, target_node, params.criticality);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

#ifndef VTR_ENABLE_CAPNPROTO

void ExtendedMapLookahead::read(const std::string& file) {
    VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::read not implemented");
}
void ExtendedMapLookahead::write(const std::string& file) const {
    VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::write not implemented");
}

#else

void ExtendedMapLookahead::read(const std::string& file) {
    cost_map_.read(file);

    this->src_opin_delays = util::compute_router_src_opin_lookahead();
    this->chan_ipins_delays = util::compute_router_chan_ipin_lookahead();
}
void ExtendedMapLookahead::write(const std::string& file) const {
    cost_map_.write(file);
}

#endif
