#include "router_lookahead_map.h"

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
#    include "connection_map.capnp.h"
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

#define X_OFFSET 2
#define Y_OFFSET 2

#define MAX_EXPANSION_LEVEL 1

// Don't continue storing a path after hitting a lower-or-same cost entry.
static constexpr bool BREAK_ON_MISS = false;

// Distance penalties filling are calculated based on available samples, but can be adjusted with this factor.
static constexpr float PENALTY_FACTOR = 1.f;
static constexpr float PENALTY_MIN = 1e-12f;

static constexpr int MIN_PATH_COUNT = 1000;

template<typename Entry>
static std::pair<float, int> run_dijkstra(int start_node_ind,
                                          std::vector<bool>* node_expanded,
                                          std::vector<util::Search_Path>* paths,
                                          RoutingCosts* routing_costs);

// also known as the L1 norm
static int manhattan_distance(const vtr::Point<int>& a, const vtr::Point<int>& b) {
    return abs(b.x() - a.x()) + abs(b.y() - a.y());
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return std::min(std::max(v, lo), hi);
}

template<typename T>
static vtr::Point<T> closest_point_in_rect(const vtr::Rect<T>& r, const vtr::Point<T>& p) {
    if (r.empty()) {
        return vtr::Point<T>(0, 0);
    } else {
        return vtr::Point<T>(clamp<T>(p.x(), r.xmin(), r.xmax() - 1),
                             clamp<T>(p.y(), r.ymin(), r.ymax() - 1));
    }
}

// deltas calculation
static void get_xy_deltas(const RRNodeId from_node, const RRNodeId to_node, int* delta_x, int* delta_y);
static void adjust_rr_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_pin_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_wire_position(const RRNodeId rr, int& x, int& y);
static void adjust_rr_src_sink_position(const RRNodeId rr, int& x, int& y);

// additional lookups for IPIN/OPIN missing delays
struct t_reachable_wire_inf {
    e_rr_type wire_rr_type;
    int wire_seg_index;

    //Costs to reach the wire type from the current node
    float congestion;
    float delay;
};

typedef std::vector<std::vector<std::map<int, t_reachable_wire_inf>>> t_src_opin_delays; //[0..device_ctx.physical_tile_types.size()-1][0..max_ptc-1][wire_seg_index]
                                                                                         // ^                                           ^             ^
                                                                                         // |                                           |             |
                                                                                         // physical block type index                   |             Reachable wire info
                                                                                         //                                             |
                                                                                         //                                             SOURCE/OPIN ptc

typedef std::vector<std::vector<std::map<int, t_reachable_wire_inf>>> t_chan_ipins_delays; //[0..device_ctx.physical_tile_types.size()-1][0..max_ptc-1][wire_seg_index]
                                                                                           // ^                                           ^             ^
                                                                                           // |                                           |             |
                                                                                           // physical block type index                   |             Wire to IPIN segment info
                                                                                           //                                             |
                                                                                           //                                             SINK/IPIN ptc

//Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
t_src_opin_delays f_src_opin_delays;

//Look-up table from CHANX/CHANY to SINK/IPIN of various types
t_chan_ipins_delays f_chan_ipins_delays;

constexpr int DIRECT_CONNECT_SPECIAL_SEG_TYPE = -1;

static void compute_router_src_opin_lookahead();
static void compute_router_chan_ipin_lookahead();
static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type, vtr::Point<int> prev);
static void dijkstra_flood_to_wires(int itile, RRNodeId node, t_src_opin_delays& src_opin_delays);
static void dijkstra_flood_to_ipins(RRNodeId node, t_chan_ipins_delays& chan_ipins_delays);

// build the segment map
void CostMap::build_segment_map() {
    const auto& device_ctx = g_vpr_ctx.device();
    segment_map_.resize(device_ctx.rr_nodes.size());
    for (size_t i = 0; i < segment_map_.size(); ++i) {
        auto& from_node = device_ctx.rr_nodes[i];

        int from_cost_index = from_node.cost_index();
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        segment_map_[i] = from_seg_index;
    }
}

// resize internal data structures
void CostMap::set_counts(size_t seg_count) {
    cost_map_.clear();
    offset_.clear();
    penalty_.clear();
    cost_map_.resize({2, seg_count});
    offset_.resize({2, seg_count});
    penalty_.resize({2, seg_count});
    seg_count_ = seg_count;
}

// cached node -> segment map
int CostMap::node_to_segment(int from_node_ind) const {
    return segment_map_[from_node_ind];
}

static util::Cost_Entry penalize(const util::Cost_Entry& entry, int distance, float penalty) {
    penalty = std::max(penalty, PENALTY_MIN);
    return util::Cost_Entry(entry.delay + distance * penalty * PENALTY_FACTOR,
                            entry.congestion, entry.fill);
}

// get a cost entry for a segment type, connection box type, and offset
util::Cost_Entry CostMap::find_cost(int from_seg_index, e_rr_type rr_type, int delta_x, int delta_y) const {
    int chan_index = 0;
    if (rr_type == CHANY) {
        chan_index = 1;
    }

    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
    const auto& cost_map = cost_map_[chan_index][from_seg_index];
    if (cost_map.dim_size(0) == 0 || cost_map.dim_size(1) == 0) {
        return util::Cost_Entry();
    }

    vtr::Point<int> coord(delta_x - offset_[chan_index][from_seg_index].first,
                          delta_y - offset_[chan_index][from_seg_index].second);
    vtr::Rect<int> bounds(0, 0, cost_map.dim_size(0), cost_map.dim_size(1));
    auto closest = closest_point_in_rect(bounds, coord);
    auto cost = cost_map_[chan_index][from_seg_index][closest.x()][closest.y()];
    float penalty = penalty_[chan_index][from_seg_index];
    auto distance = manhattan_distance(closest, coord);
    return penalize(cost, distance, penalty);
}

// set the cost map for a segment type and connection box type, filling holes
void CostMap::set_cost_map(const RoutingCosts& delay_costs, const RoutingCosts& base_costs) {
    // calculate the bounding boxes
    vtr::Matrix<vtr::Rect<int>> bounds({2, seg_count_});
    for (const auto& entry : delay_costs) {
        bounds[entry.first.chan_index][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }
    for (const auto& entry : base_costs) {
        bounds[entry.first.chan_index][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }

    // store bounds
    for (size_t chan = 0; chan < 2; chan++) {
        for (size_t seg = 0; seg < seg_count_; seg++) {
            const auto& chan_seg_bounds = bounds[chan][seg];
            if (chan_seg_bounds.empty()) {
                // Didn't find any sample routes, so routing isn't possible between these segment/connection box types.
                offset_[chan][seg] = std::make_pair(0, 0);
                cost_map_[chan][seg] = vtr::NdMatrix<util::Cost_Entry, 2>(
                    {size_t(0), size_t(0)});
                continue;
            } else {
                offset_[chan][seg] = std::make_pair(chan_seg_bounds.xmin(), chan_seg_bounds.ymin());
                cost_map_[chan][seg] = vtr::NdMatrix<util::Cost_Entry, 2>(
                    {size_t(chan_seg_bounds.width()), size_t(chan_seg_bounds.height())});
            }
        }
    }

    // store entries into the matrices
    for (const auto& entry : delay_costs) {
        int seg = entry.first.seg_index;
        int chan = entry.first.chan_index;
        const auto& chan_seg_bounds = bounds[chan][seg];
        int x = entry.first.delta.x() - chan_seg_bounds.xmin();
        int y = entry.first.delta.y() - chan_seg_bounds.ymin();
        cost_map_[chan][seg][x][y].delay = entry.second;
    }
    for (const auto& entry : base_costs) {
        int seg = entry.first.seg_index;
        int chan = entry.first.chan_index;
        const auto& chan_seg_bounds = bounds[chan][seg];
        int x = entry.first.delta.x() - chan_seg_bounds.xmin();
        int y = entry.first.delta.y() - chan_seg_bounds.ymin();
        cost_map_[chan][seg][x][y].congestion = entry.second;
    }

    // fill the holes
    for (size_t chan = 0; chan < 2; chan++) {
        for (size_t seg = 0; seg < seg_count_; seg++) {
            penalty_[chan][seg] = std::numeric_limits<float>::infinity();
            const auto& chan_seg_bounds = bounds[chan][seg];
            if (chan_seg_bounds.empty()) {
                continue;
            }
            auto& matrix = cost_map_[chan][seg];

            // calculate delay penalty
            float min_delay = std::numeric_limits<float>::infinity(), max_delay = 0.f;
            vtr::Point<int> min_location(0, 0), max_location(0, 0);
            for (unsigned ix = 0; ix < matrix.dim_size(0); ix++) {
                for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
                    util::Cost_Entry& cost_entry = matrix[ix][iy];
                    if (cost_entry.valid()) {
                        if (cost_entry.delay < min_delay) {
                            min_delay = cost_entry.delay;
                            min_location = vtr::Point<int>(ix, iy);
                        }
                        if (cost_entry.delay > max_delay) {
                            max_delay = cost_entry.delay;
                            max_location = vtr::Point<int>(ix, iy);
                        }
                    }
                }
            }
            float delay_penalty = (max_delay - min_delay) / static_cast<float>(std::max(1, manhattan_distance(max_location, min_location)));
            penalty_[chan][seg] = delay_penalty;

            // find missing cost entries and fill them in by copying a nearby cost entry
            std::vector<std::tuple<unsigned, unsigned, util::Cost_Entry>> missing;
            bool couldnt_fill = false;
            auto shifted_bounds = vtr::Rect<int>(0, 0, chan_seg_bounds.width(), chan_seg_bounds.height());
            int max_fill = 0;
            for (unsigned ix = 0; ix < matrix.dim_size(0); ix++) {
                for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
                    util::Cost_Entry& cost_entry = matrix[ix][iy];
                    if (!cost_entry.valid()) {
                        // maximum search radius
                        util::Cost_Entry filler;
                        int distance;
                        std::tie(filler, distance) = get_nearby_cost_entry(matrix, ix, iy, shifted_bounds);
                        if (filler.valid()) {
                            missing.push_back(std::make_tuple(ix, iy, penalize(filler, distance, delay_penalty)));
                            max_fill = std::max(max_fill, distance);
                        } else {
                            couldnt_fill = true;
                        }
                    }
                }
                if (couldnt_fill) {
                    // give up trying to fill an empty matrix
                    break;
                }
            }

            if (!couldnt_fill) {
                VTR_LOG("At %d: max_fill = %d, delay_penalty = %e\n", seg, max_fill, delay_penalty);
            }

            // write back the missing entries
            for (auto& xy_entry : missing) {
                matrix[std::get<0>(xy_entry)][std::get<1>(xy_entry)] = std::get<2>(xy_entry);
            }

            if (couldnt_fill) {
                VTR_LOG_WARN("Couldn't fill holes in the cost matrix for %d -> %ld, %d x %d bounding box\n",
                             chan, seg, chan_seg_bounds.width(), chan_seg_bounds.height());
                for (unsigned y = 0; y < matrix.dim_size(1); y++) {
                    for (unsigned x = 0; x < matrix.dim_size(0); x++) {
                        VTR_ASSERT(!matrix[x][y].valid());
                    }
                }
            }
        }
    }
}

// prints an ASCII diagram of each cost map for a segment type (debug)
// o => above average
// . => at or below average
// * => invalid (missing)
void CostMap::print(int iseg) const {
    for (size_t chan_index = 0;
         chan_index < 2;
         chan_index++) {
        auto& matrix = cost_map_[chan_index][iseg];
        if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) {
            VTR_LOG("cost EMPTY for chan_index = %lu\n", chan_index);
            continue;
        }
        VTR_LOG("cost for chan_index = %lu (%zu, %zu)\n", chan_index, matrix.dim_size(0), matrix.dim_size(1));
        double sum = 0.0;
        for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
            for (unsigned ix = 0; ix < matrix.dim_size(0); ix++) {
                const auto& entry = matrix[ix][iy];
                if (entry.valid()) {
                    sum += entry.delay;
                }
            }
        }
        double avg = sum / ((double)matrix.dim_size(0) * (double)matrix.dim_size(1));
        for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
            for (unsigned ix = 0; ix < matrix.dim_size(0); ix++) {
                const auto& entry = matrix[ix][iy];
                if (!entry.valid()) {
                    VTR_LOG("*");
                } else if (entry.delay * 4 > avg * 5) {
                    VTR_LOG("O");
                } else if (entry.delay > avg) {
                    VTR_LOG("o");
                } else if (entry.delay * 4 > avg * 3) {
                    VTR_LOG(".");
                } else {
                    VTR_LOG(" ");
                }
            }
            VTR_LOG("\n");
        }
    }
}

// list segment type and connection box type pairs that have empty cost maps (debug)
std::vector<std::pair<int, int>> CostMap::list_empty() const {
    std::vector<std::pair<int, int>> results;
    for (int chan_index = 0; chan_index < (int)cost_map_.dim_size(1); chan_index++) {
        for (int iseg = 0; iseg < (int)cost_map_.dim_size(0); iseg++) {
            auto& matrix = cost_map_[chan_index][iseg];
            if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) results.push_back(std::make_pair(chan_index, iseg));
        }
    }
    return results;
}

static void assign_min_entry(util::Cost_Entry* dst, const util::Cost_Entry& src) {
    if (src.delay < dst->delay) {
        dst->delay = src.delay;
    }
    if (src.congestion < dst->congestion) {
        dst->congestion = src.congestion;
    }
}

// find the minimum cost entry from the nearest manhattan distance neighbor
std::pair<util::Cost_Entry, int> CostMap::get_nearby_cost_entry(const vtr::NdMatrix<util::Cost_Entry, 2>& matrix,
                                                                int cx,
                                                                int cy,
                                                                const vtr::Rect<int>& bounds) {
    // spiral around (cx, cy) looking for a nearby entry
    bool in_bounds = bounds.contains(vtr::Point<int>(cx, cy));
    if (!in_bounds) {
        return std::make_pair(util::Cost_Entry(), 0);
    }
    int n = 0;
    util::Cost_Entry fill(matrix[cx][cy]);
    fill.fill = true;

    while (in_bounds && !fill.valid()) {
        n++;
        in_bounds = false;
        util::Cost_Entry min_entry;
        for (int ox = -n; ox <= n; ox++) {
            int x = cx + ox;
            int oy = n - abs(ox);
            int yp = cy + oy;
            int yn = cy - oy;
            if (bounds.contains(vtr::Point<int>(x, yp))) {
                assign_min_entry(&min_entry, matrix[x][yp]);
                in_bounds = true;
            }
            if (bounds.contains(vtr::Point<int>(x, yn))) {
                assign_min_entry(&min_entry, matrix[x][yn]);
                in_bounds = true;
            }
        }
        if (!std::isfinite(fill.delay)) {
            fill.delay = min_entry.delay;
        }
        if (!std::isfinite(fill.congestion)) {
            fill.congestion = min_entry.congestion;
        }
    }
    return std::make_pair(fill, n);
}

std::pair<float, float> MapLookahead::get_src_opin_delays(RRNodeId from_node, int delta_x, int delta_y, float criticality_fac) const {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    e_rr_type from_type = rr_graph.node_type(from_node);
    if (from_type == SOURCE || from_type == OPIN) {
        //When estimating costs from a SOURCE/OPIN we look-up to find which wire types (and the
        //cost to reach them) in f_src_opin_delays. Once we know what wire types are
        //reachable, we query the f_wire_cost_map (i.e. the wire lookahead) to get the final
        //delay to reach the sink.

        t_physical_tile_type_ptr tile_type = device_ctx.grid[rr_graph.node_xlow(from_node)][rr_graph.node_ylow(from_node)].type;
        auto tile_index = std::distance(&device_ctx.physical_tile_types[0], tile_type);

        auto from_ptc = rr_graph.node_ptc_num(from_node);

        if (f_src_opin_delays[tile_index][from_ptc].empty()) {
            //During lookahead profiling we were unable to find any wires which connected
            //to this PTC.
            //
            //This can sometimes occur at very low channel widths (e.g. during min W search on
            //small designs) where W discretization combined with fraction Fc may cause some
            //pins/sources to be left disconnected.
            //
            //Such RR graphs are of course unroutable, but that should be determined by the
            //router. So just return an arbitrary value here rather than error.

            return std::pair<float, float>(0.f, 0.f);
        } else {
            //From the current SOURCE/OPIN we look-up the wiretypes which are reachable
            //and then add the estimates from those wire types for the distance of interest.
            //If there are multiple options we use the minimum value.

            float delay = 0;
            float congestion = 0;
            float expected_cost = std::numeric_limits<float>::infinity();

            for (const auto& kv : f_src_opin_delays[tile_index][from_ptc]) {
                const t_reachable_wire_inf& reachable_wire_inf = kv.second;

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
                    cost_entry = cost_map_.find_cost(reachable_wire_inf.wire_seg_index, reachable_wire_inf.wire_rr_type, delta_x, delta_y);
                }

                float this_cost = (criticality_fac) * (reachable_wire_inf.delay + cost_entry.delay)
                                  + (1. - criticality_fac) * (reachable_wire_inf.congestion + cost_entry.congestion);

                if (this_cost < expected_cost) {
                    delay = reachable_wire_inf.delay;
                    congestion = reachable_wire_inf.congestion;
                }
            }

            return std::pair<float, float>(delay, congestion);
        }

        VTR_ASSERT_SAFE_MSG(false,
                            vtr::string_fmt("Lookahead failed to estimate cost from %s: %s",
                                            rr_node_arch_name(size_t(from_node)).c_str(),
                                            describe_rr_node(size_t(from_node)).c_str())
                                .c_str());
    }

    return std::pair<float, float>(0.f, 0.f);
}

// derive a cost from the map between two nodes
float MapLookahead::get_map_cost(int from_node_ind,
                                 int to_node_ind,
                                 float criticality_fac) const {
    if (from_node_ind == to_node_ind) {
        return 0.f;
    }

    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    auto from_node_type = device_ctx.rr_nodes[to_node_ind].type();

    RRNodeId to_node(to_node_ind);
    RRNodeId from_node(from_node_ind);

    int dx, dy;
    get_xy_deltas(from_node, to_node, &dx, &dy);

    float extra_delay, extra_congestion;
    std::tie(extra_delay, extra_congestion) = this->get_src_opin_delays(from_node, dx, dy, criticality_fac);

    int from_seg_index = cost_map_.node_to_segment(from_node_ind);
    util::Cost_Entry cost_entry = cost_map_.find_cost(from_seg_index, from_node_type, dx, dy);

    if (!cost_entry.valid()) {
        // there is no route
        VTR_LOGV_DEBUG(f_router_debug,
                       "Not connected %d (%s, %d) -> %d (%s)\n",
                       from_node_ind, device_ctx.rr_nodes[from_node_ind].type_string(), from_seg_index,
                       to_node_ind, device_ctx.rr_nodes[to_node_ind].type_string());
        return std::numeric_limits<float>::infinity();
    }

    auto to_tile_type = device_ctx.grid[rr_graph.node_xlow(to_node)][rr_graph.node_ylow(to_node)].type;
    auto to_tile_index = to_tile_type->index;

    auto to_ptc = rr_graph.node_ptc_num(to_node);

    float site_pin_delay = std::numeric_limits<float>::infinity();
    if (f_chan_ipins_delays[to_tile_index].size() != 0) {
        for (const auto& kv : f_chan_ipins_delays[to_tile_index][to_ptc]) {
            const t_reachable_wire_inf& reachable_wire_inf = kv.second;

            float this_delay = reachable_wire_inf.delay;
            site_pin_delay = std::min(this_delay, site_pin_delay);
        }
    }

    float expected_delay = cost_entry.delay + extra_delay;
    float expected_congestion = cost_entry.congestion + extra_congestion;

    expected_delay += site_pin_delay;

    float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;

    VTR_LOGV_DEBUG(f_router_debug, "Requested lookahead from node \n\t%s to \n\t%s\n",
                   describe_rr_node(from_node_ind).c_str(),
                   describe_rr_node(to_node_ind).c_str());
    const std::string& segment_name = device_ctx.segment_inf[from_seg_index].name;
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead returned %s (%d) with distance (%zd, %zd)\n",
                   segment_name.c_str(), from_seg_index,
                   dx, dy);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead delay: %g\n", expected_delay);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead congestion: %g\n", expected_congestion);
    VTR_LOGV_DEBUG(f_router_debug, "Criticality: %g\n", criticality_fac);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead cost: %g\n", expected_cost);
    VTR_LOGV_DEBUG(f_router_debug, "Site pin delay: %g\n", site_pin_delay);

    /* XXX: temporarily disable this for further debugging as it appears that some costs are set to infinity
    if (!std::isfinite(expected_cost) {
        VTR_LOG_ERROR("infinite cost for segment %d at (%d, %d)\n", from_seg_index, (int)dx, (int)dy);
        VTR_ASSERT(0);
    } */

    if (expected_cost < 0.f) {
        VTR_LOG_ERROR("negative cost for segment %d at (%d, %d)\n", from_seg_index, (int)dx, (int)dy);
        VTR_ASSERT(0);
    }

    return expected_cost;
}

// add a best cost routing path from start_node_ind to node_ind to routing costs
template<typename Entry>
static bool add_paths(int start_node_ind,
                      Entry current,
                      const std::vector<util::Search_Path>& paths,
                      RoutingCosts* routing_costs) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    int node_ind = current.rr_node_ind;
    RRNodeId node(node_ind);

    bool new_sample_found = false;

    auto to_tile_type = device_ctx.grid[rr_graph.node_xlow(node)][rr_graph.node_ylow(node)].type;
    auto to_tile_index = to_tile_type->index;

    auto to_ptc = rr_graph.node_ptc_num(node);

    float site_pin_delay = std::numeric_limits<float>::infinity();
    if (f_chan_ipins_delays[to_tile_index].size() != 0) {
        for (const auto& kv : f_chan_ipins_delays[to_tile_index][to_ptc]) {
            const t_reachable_wire_inf& reachable_wire_inf = kv.second;

            float this_delay = reachable_wire_inf.delay;
            site_pin_delay = std::min(this_delay, site_pin_delay);
        }
    }

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

        auto chan_type = rr_graph.node_type(node);

        int ichan = 0;
        if (chan_type == CHANY) {
            ichan = 1;
        }

        int delta_x, delta_y;
        get_xy_deltas(this_node, node, &delta_x, &delta_y);

        vtr::Point<int> delta(delta_x, delta_y);

        RoutingCostKey key = {
            ichan,
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

/* returns the absolute delta_x and delta_y offset required to reach to_node from from_node */
static void get_xy_deltas(const RRNodeId from_node, const RRNodeId to_node, int* delta_x, int* delta_y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    e_rr_type from_type = rr_graph.node_type(from_node);
    e_rr_type to_type = rr_graph.node_type(to_node);

    if (!is_chan(from_type) && !is_chan(to_type)) {
        //Alternate formulation for non-channel types
        int from_x = 0;
        int from_y = 0;
        adjust_rr_position(from_node, from_x, from_y);

        int to_x = 0;
        int to_y = 0;
        adjust_rr_position(to_node, to_x, to_y);

        *delta_x = to_x - from_x;
        *delta_y = to_y - from_y;
    } else {
        //Traditional formulation

        /* get chan/seg coordinates of the from/to nodes. seg coordinate is along the wire,
         * chan coordinate is orthogonal to the wire */
        int from_seg_low;
        int from_seg_high;
        int from_chan;
        int to_seg;
        int to_chan;
        if (from_type == CHANY) {
            from_seg_low = rr_graph.node_ylow(from_node);
            from_seg_high = rr_graph.node_yhigh(from_node);
            from_chan = rr_graph.node_xlow(from_node);
            to_seg = rr_graph.node_ylow(to_node);
            to_chan = rr_graph.node_xlow(to_node);
        } else {
            from_seg_low = rr_graph.node_xlow(from_node);
            from_seg_high = rr_graph.node_xhigh(from_node);
            from_chan = rr_graph.node_ylow(from_node);
            to_seg = rr_graph.node_xlow(to_node);
            to_chan = rr_graph.node_ylow(to_node);
        }

        /* now we want to count the minimum number of *channel segments* between the from and to nodes */
        int delta_seg, delta_chan;

        /* orthogonal to wire */
        int no_need_to_pass_by_clb = 0; //if we need orthogonal wires then we don't need to pass by the target CLB along the current wire direction
        if (to_chan > from_chan + 1) {
            /* above */
            delta_chan = to_chan - from_chan;
            no_need_to_pass_by_clb = 1;
        } else if (to_chan < from_chan) {
            /* below */
            delta_chan = from_chan - to_chan + 1;
            no_need_to_pass_by_clb = 1;
        } else {
            /* adjacent to current channel */
            delta_chan = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* along same direction as wire. */
        if (to_seg > from_seg_high) {
            /* ahead */
            delta_seg = to_seg - from_seg_high - no_need_to_pass_by_clb;
        } else if (to_seg < from_seg_low) {
            /* behind */
            delta_seg = from_seg_low - to_seg - no_need_to_pass_by_clb;
        } else {
            /* along the span of the wire */
            delta_seg = 0;
        }

        /* account for wire direction. lookahead map was computed by looking up and to the right starting at INC wires. for targets
         * that are opposite of the wire direction, let's add 1 to delta_seg */
        e_direction from_dir = rr_graph.node_direction(from_node);
        if (is_chan(from_type)
            && ((to_seg < from_seg_low && from_dir == INC_DIRECTION) || (to_seg > from_seg_high && from_dir == DEC_DIRECTION))) {
            delta_seg++;
        }

        if (from_type == CHANY) {
            *delta_x = delta_chan;
            *delta_y = delta_seg;
        } else {
            *delta_x = delta_seg;
            *delta_y = delta_chan;
        }
    }

    VTR_ASSERT_SAFE(std::abs(*delta_x) < (int)device_ctx.grid.width());
    VTR_ASSERT_SAFE(std::abs(*delta_y) < (int)device_ctx.grid.height());
}

static void adjust_rr_position(const RRNodeId rr, int& x, int& y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    e_rr_type rr_type = rr_graph.node_type(rr);

    if (is_chan(rr_type)) {
        adjust_rr_wire_position(rr, x, y);
    } else if (is_pin(rr_type)) {
        adjust_rr_pin_position(rr, x, y);
    } else {
        VTR_ASSERT_SAFE(is_src_sink(rr_type));
        adjust_rr_src_sink_position(rr, x, y);
    }
}

static void adjust_rr_pin_position(const RRNodeId rr, int& x, int& y) {
    /*
     * VPR uses a co-ordinate system where wires above and to the right of a block
     * are at the same location as the block:
     *
     *
     *       <-----------C
     *    D
     *    |  +---------+  ^
     *    |  |         |  |
     *    |  |  (1,1)  |  |
     *    |  |         |  |
     *    V  +---------+  |
     *                    A
     *     B----------->
     *
     * So wires are located as follows:
     *
     *      A: (1, 1) CHANY
     *      B: (1, 0) CHANX
     *      C: (1, 1) CHANX
     *      D: (0, 1) CHANY
     *
     * But all pins incident on the surrounding channels
     * would be at (1,1) along with a relevant side.
     *
     * In the following, we adjust the positions of pins to
     * account for the channel they are incident too.
     *
     * Note that blocks at (0,*) such as IOs, may have (unconnected)
     * pins on the left side, so we also clip the minimum x to zero.
     * Similarly for blocks at (*,0) we clip the minimum y to zero.
     */
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_pin(rr_graph.node_type(rr)));
    VTR_ASSERT_SAFE(rr_graph.node_xlow(rr) == rr_graph.node_xhigh(rr));
    VTR_ASSERT_SAFE(rr_graph.node_ylow(rr) == rr_graph.node_yhigh(rr));

    x = rr_graph.node_xlow(rr);
    y = rr_graph.node_ylow(rr);

    e_side rr_side = rr_graph.node_side(rr);

    if (rr_side == LEFT) {
        x -= 1;
        x = std::max(x, 0);
    } else if (rr_side == BOTTOM && y > 0) {
        y -= 1;
        y = std::max(y, 0);
    }
}

static void adjust_rr_wire_position(const RRNodeId rr, int& x, int& y) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_chan(rr_graph.node_type(rr)));

    e_direction rr_dir = rr_graph.node_direction(rr);

    if (rr_dir == DEC_DIRECTION) {
        x = rr_graph.node_xhigh(rr);
        y = rr_graph.node_yhigh(rr);
    } else if (rr_dir == INC_DIRECTION) {
        x = rr_graph.node_xlow(rr);
        y = rr_graph.node_ylow(rr);
    } else {
        VTR_ASSERT_SAFE(rr_dir == BI_DIRECTION);
        //Not sure what to do here...
        //Try average for now.
        x = vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.);
        y = vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.);
    }
}

static void adjust_rr_src_sink_position(const RRNodeId rr, int& x, int& y) {
    //SOURCE/SINK nodes assume the full dimensions of their
    //associated block
    //
    //Use the average position.
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    VTR_ASSERT_SAFE(is_src_sink(rr_graph.node_type(rr)));

    x = vtr::nint((rr_graph.node_xlow(rr) + rr_graph.node_xhigh(rr)) / 2.);
    y = vtr::nint((rr_graph.node_ylow(rr) + rr_graph.node_yhigh(rr)) / 2.);
}

/* Runs Dijkstra's algorithm from specified node until all nodes have been
 * visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored to an entry in the routing_cost_map.
 *
 * Returns the maximum (last) minimum cost path stored, and
 * the number of paths from start_node_ind stored. */
template<typename Entry>
static std::pair<float, int> run_dijkstra(int start_node_ind,
                                          std::vector<bool>* node_expanded,
                                          std::vector<util::Search_Path>* paths,
                                          RoutingCosts* routing_costs) {
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
            add_paths<Entry>(start_node_ind, current, *paths, routing_costs);
        } else {
            util::expand_dijkstra_neighbours(device_ctx.rr_nodes,
                                             current, paths, node_expanded, &pq);
            (*node_expanded)[node_ind] = true;
        }
    }
    return std::make_pair(max_cost, path_count);
}

// compute the cost maps for lookahead
void MapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    compute_router_src_opin_lookahead();
    compute_router_chan_ipin_lookahead();

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
    RoutingCosts all_delay_costs;
    RoutingCosts all_base_costs;

    /* run Dijkstra's algorithm for each segment type & channel type combination */
#if defined(VPR_USE_TBB)
    tbb::mutex all_costs_mutex;
    tbb::parallel_for_each(sample_regions, [&](const SampleRegion& region) {
#else
    for (const auto& region : sample_regions) {
#endif
        // holds the cost entries for a run
        RoutingCosts delay_costs;
        RoutingCosts base_costs;
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

            /*
             * if (path_count == 0) {
             * for (auto node_ind : point.nodes) {
             * VTR_LOG("Expanded node %s\n", describe_rr_node(node_ind).c_str());
             * }
             * }
             */

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

static void compute_router_src_opin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing src/opin lookahead");
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    f_src_opin_delays.clear();

    f_src_opin_delays.resize(device_ctx.physical_tile_types.size());

    std::vector<int> rr_nodes_at_loc;

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (size_t itile = 0; itile < device_ctx.physical_tile_types.size(); ++itile) {
        for (e_rr_type rr_type : {SOURCE, OPIN}) {
            vtr::Point<int> sample_loc(-1, -1);

            size_t num_sampled_locs = 0;
            bool ptcs_with_no_delays = true;
            while (ptcs_with_no_delays) { //Haven't found wire connected to ptc
                ptcs_with_no_delays = false;

                sample_loc = pick_sample_tile(&device_ctx.physical_tile_types[itile], sample_loc);

                if (sample_loc.x() == -1 && sample_loc.y() == -1) {
                    //No untried instances of the current tile type left
                    VTR_LOG_WARN("Found no %ssample locations for %s in %s\n",
                                 (num_sampled_locs == 0) ? "" : "more ",
                                 rr_node_typename[rr_type],
                                 device_ctx.physical_tile_types[itile].name);
                    break;
                }

                //VTR_LOG("Sampling %s at (%d,%d)\n", device_ctx.physical_tile_types[itile].name, sample_loc.x(), sample_loc.y());

                rr_nodes_at_loc.clear();

                get_rr_node_indices(device_ctx.rr_node_indices, sample_loc.x(), sample_loc.y(), rr_type, &rr_nodes_at_loc);
                for (int inode : rr_nodes_at_loc) {
                    if (inode < 0) continue;

                    RRNodeId node_id(inode);

                    int ptc = rr_graph.node_ptc_num(node_id);

                    if (ptc >= int(f_src_opin_delays[itile].size())) {
                        f_src_opin_delays[itile].resize(ptc + 1); //Inefficient but functional...
                    }

                    //Find the wire types which are reachable from inode and record them and
                    //the cost to reach them
                    dijkstra_flood_to_wires(itile, node_id, f_src_opin_delays);

                    if (f_src_opin_delays[itile][ptc].empty()) {
                        VTR_LOGV_DEBUG(f_router_debug, "Found no reachable wires from %s (%s) at (%d,%d)\n",
                                       rr_node_typename[rr_type],
                                       rr_node_arch_name(inode).c_str(),
                                       sample_loc.x(),
                                       sample_loc.y());

                        ptcs_with_no_delays = true;
                    }
                }

                ++num_sampled_locs;
            }
            if (ptcs_with_no_delays) {
                VPR_ERROR(VPR_ERROR_ROUTE, "Some SOURCE/OPINs have no reachable wires\n");
            }
        }
    }
}

static void compute_router_chan_ipin_lookahead() {
    vtr::ScopedStartFinishTimer timer("Computing chan/ipin lookahead");
    auto& device_ctx = g_vpr_ctx.device();

    f_chan_ipins_delays.clear();

    f_chan_ipins_delays.resize(device_ctx.physical_tile_types.size());

    std::vector<int> rr_nodes_at_loc;

    //We assume that the routing connectivity of each instance of a physical tile is the same,
    //and so only measure one instance of each type
    for (auto tile_type : device_ctx.physical_tile_types) {
        vtr::Point<int> sample_loc(-1, -1);

        sample_loc = pick_sample_tile(&tile_type, sample_loc);

        if (sample_loc.x() == -1 && sample_loc.y() == -1) {
            //No untried instances of the current tile type left
            VTR_LOG_WARN("Found no sample locations for %s\n",
                         tile_type.name);
            continue;
        }

        int min_x = std::max(0, sample_loc.x() - X_OFFSET);
        int min_y = std::max(0, sample_loc.y() - Y_OFFSET);
        int max_x = std::min(int(device_ctx.grid.width()), sample_loc.x() + X_OFFSET);
        int max_y = std::min(int(device_ctx.grid.height()), sample_loc.y() + Y_OFFSET);

        for (int ix = min_x; ix < max_x; ix++) {
            for (int iy = min_y; iy < max_y; iy++) {
                for (auto rr_type : {CHANX, CHANY}) {
                    rr_nodes_at_loc.clear();

                    get_rr_node_indices(device_ctx.rr_node_indices, ix, iy, rr_type, &rr_nodes_at_loc);
                    for (int inode : rr_nodes_at_loc) {
                        if (inode < 0) continue;

                        RRNodeId node_id(inode);

                        //Find the IPINs which are reachable from the wires within the bounding box
                        //around the selected tile location
                        dijkstra_flood_to_ipins(node_id, f_chan_ipins_delays);
                    }
                }
            }
        }
    }
}

static vtr::Point<int> pick_sample_tile(t_physical_tile_type_ptr tile_type, vtr::Point<int> prev) {
    //Very simple for now, just pick the fist matching tile found
    vtr::Point<int> loc(OPEN, OPEN);

    //VTR_LOG("Prev: %d,%d\n", prev.x(), prev.y());

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    int y_init = prev.y() + 1; //Start searching next element above prev

    for (int x = prev.x(); x < int(grid.width()); ++x) {
        if (x < 0) continue;

        //VTR_LOG("  x: %d\n", x);

        for (int y = y_init; y < int(grid.height()); ++y) {
            if (y < 0) continue;

            //VTR_LOG("   y: %d\n", y);
            if (grid[x][y].type->index == tile_type->index) {
                loc.set_x(x);
                loc.set_y(y);
                return loc;
            }
        }

        if (loc.x() != OPEN && loc.y() != OPEN) {
            break;
        } else {
            y_init = 0; //Prepare to search next column
        }
    }
    //VTR_LOG("Next: %d,%d\n", loc.x(), loc.y());

    return loc;
}

static void dijkstra_flood_to_wires(int itile, RRNodeId node, t_src_opin_delays& src_opin_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    struct t_pq_entry {
        float delay;
        float congestion;
        RRNodeId node;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry> pq;

    t_pq_entry root;
    root.congestion = 0.;
    root.delay = 0.;
    root.node = node;

    int ptc = rr_graph.node_ptc_num(node);

    /*
     * Perform Djikstra from the SOURCE/OPIN of interest, stopping at the the first
     * reachable wires (i.e until we hit the inter-block routing network), or a SINK
     * (via a direct-connect).
     *
     * Note that typical RR graphs are structured :
     *
     *      SOURCE ---> OPIN --> CHANX/CHANY
     *              |
     *              --> OPIN --> CHANX/CHANY
     *              |
     *             ...
     *
     *   possibly with direct-connects of the form:
     *
     *      SOURCE --> OPIN --> IPIN --> SINK
     *
     * and there is a small number of fixed hops from SOURCE/OPIN to CHANX/CHANY or
     * to a SINK (via a direct-connect), so this runs very fast (i.e. O(1))
     */
    pq.push(root);
    while (!pq.empty()) {
        t_pq_entry curr = pq.top();
        pq.pop();

        e_rr_type curr_rr_type = rr_graph.node_type(curr.node);
        if (curr_rr_type == CHANX || curr_rr_type == CHANY || curr_rr_type == SINK) {
            //We stop expansion at any CHANX/CHANY/SINK
            int seg_index;
            if (curr_rr_type != SINK) {
                //It's a wire, figure out its type
                int cost_index = rr_graph.node_cost_index(curr.node);
                seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;
            } else {
                //This is a direct-connect path between an IPIN and OPIN,
                //which terminated at a SINK.
                //
                //We treat this as a 'special' wire type
                seg_index = DIRECT_CONNECT_SPECIAL_SEG_TYPE;
            }

            //Keep costs of the best path to reach each wire type
            if (!src_opin_delays[itile][ptc].count(seg_index)
                || curr.delay < src_opin_delays[itile][ptc][seg_index].delay) {
                src_opin_delays[itile][ptc][seg_index].wire_rr_type = curr_rr_type;
                src_opin_delays[itile][ptc][seg_index].wire_seg_index = seg_index;
                src_opin_delays[itile][ptc][seg_index].delay = curr.delay;
                src_opin_delays[itile][ptc][seg_index].congestion = curr.congestion;
            }

        } else if (curr_rr_type == SOURCE || curr_rr_type == OPIN || curr_rr_type == IPIN) {
            //We allow expansion through SOURCE/OPIN/IPIN types
            int cost_index = rr_graph.node_cost_index(curr.node);
            float incr_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.edge_switch(edge);
                float incr_delay = device_ctx.rr_switch_inf[iswitch].Tdel;

                RRNodeId next_node = rr_graph.edge_sink_node(edge);

                t_pq_entry next;
                next.congestion = curr.congestion + incr_cong; //Of current node
                next.delay = curr.delay + incr_delay;          //To reach next node
                next.node = next_node;

                pq.push(next);
            }
        } else {
            VPR_ERROR(VPR_ERROR_ROUTE, "Unrecognized RR type");
        }
    }
}

static void dijkstra_flood_to_ipins(RRNodeId node, t_chan_ipins_delays& chan_ipins_delays) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_nodes;

    struct t_pq_entry {
        float delay;
        float congestion;
        RRNodeId node;
        int level;
        int prev_seg_index;

        bool operator<(const t_pq_entry& rhs) const {
            return this->delay < rhs.delay;
        }
    };

    std::priority_queue<t_pq_entry> pq;

    t_pq_entry root;
    root.congestion = 0.;
    root.delay = 0.;
    root.node = node;
    root.level = 0;
    root.prev_seg_index = OPEN;

    /*
     * Perform Djikstra from the CHAN of interest, stopping at the the first
     * reachable IPIN
     *
     * Note that typical RR graphs are structured :
     *
     *      CHANX/CHANY --> CHANX/CHANY --> ... --> CHANX/CHANY --> IPIN --> SINK
     *                  |
     *                  --> CHANX/CHANY --> ... --> CHANX/CHANY --> IPIN --> SINK
     *                  |
     *                  ...
     *
     * and there is a variable number of hops from a given CHANX/CHANY  to IPIN.
     * To avoid impacting on run-time, a fixed number of hops is performed. This
     * should be enough to find the delay from the last CAHN to IPIN connection.
     */
    pq.push(root);
    while (!pq.empty()) {
        t_pq_entry curr = pq.top();
        pq.pop();

        e_rr_type curr_rr_type = rr_graph.node_type(curr.node);
        if (curr_rr_type == IPIN) {
            int seg_index = curr.prev_seg_index;

            int node_x = rr_graph.node_xlow(curr.node);
            int node_y = rr_graph.node_ylow(curr.node);

            auto tile_type = device_ctx.grid[node_x][node_y].type;
            int itile = tile_type->index;

            int ptc = rr_graph.node_ptc_num(curr.node);

            if (ptc >= int(chan_ipins_delays[itile].size())) {
                chan_ipins_delays[itile].resize(ptc + 1); //Inefficient but functional...
            }

            //Keep costs of the best path to reach each wire type
            chan_ipins_delays[itile][ptc][seg_index].wire_rr_type = curr_rr_type;
            chan_ipins_delays[itile][ptc][seg_index].wire_seg_index = seg_index;
            chan_ipins_delays[itile][ptc][seg_index].delay = curr.delay;
            chan_ipins_delays[itile][ptc][seg_index].congestion = curr.congestion;
        } else if (curr_rr_type == CHANX || curr_rr_type == CHANY) {
            if (curr.level >= MAX_EXPANSION_LEVEL) {
                continue;
            }

            //We allow expansion through SOURCE/OPIN/IPIN types
            int cost_index = rr_graph.node_cost_index(curr.node);
            float new_cong = device_ctx.rr_indexed_data[cost_index].base_cost; //Current nodes congestion cost
            int seg_index = device_ctx.rr_indexed_data[cost_index].seg_index;

            for (RREdgeId edge : rr_graph.edge_range(curr.node)) {
                int iswitch = rr_graph.edge_switch(edge);
                float new_delay = device_ctx.rr_switch_inf[iswitch].Tdel;

                RRNodeId next_node = rr_graph.edge_sink_node(edge);

                t_pq_entry next;
                next.congestion = new_cong; //Of current node
                next.delay = new_delay;     //To reach next node
                next.node = next_node;
                next.level = curr.level + 1;
                next.prev_seg_index = seg_index;

                pq.push(next);
            }
        } else {
            VPR_ERROR(VPR_ERROR_ROUTE, "Unrecognized RR type");
        }
    }
}

// get an expected minimum cost for routing from the current node to the target node
float MapLookahead::get_expected_cost(
    int current_node,
    int target_node,
    const t_conn_cost_params& params,
    float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_nodes[current_node].type();

    if (rr_type == CHANX || rr_type == CHANY) {
        return get_map_cost(
            current_node, target_node, params.criticality);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

#ifndef VTR_ENABLE_CAPNPROTO

void MapLookahead::read(const std::string& file) {
    VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::read not implemented");
}
void MapLookahead::write(const std::string& file) const {
    VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::write not implemented");
}

#else

void MapLookahead::read(const std::string& file) {
    cost_map_.read(file);

    compute_router_src_opin_lookahead();
    compute_router_chan_ipin_lookahead();
}
void MapLookahead::write(const std::string& file) const {
    cost_map_.write(file);
}

static void ToCostEntry(util::Cost_Entry* out, const VprCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
    out->fill = in.getFill();
}

static void FromCostEntry(VprCostEntry::Builder* out, const util::Cost_Entry& in) {
    out->setDelay(in.delay);
    out->setCongestion(in.congestion);
    out->setFill(in.fill);
}

static void ToVprVector2D(std::pair<int, int>* out, const VprVector2D::Reader& in) {
    *out = std::make_pair(in.getX(), in.getY());
}

static void FromVprVector2D(VprVector2D::Builder* out, const std::pair<int, int>& in) {
    out->setX(in.first);
    out->setY(in.second);
}

static void ToMatrixCostEntry(vtr::NdMatrix<util::Cost_Entry, 2>* out,
                              const Matrix<VprCostEntry>::Reader& in) {
    ToNdMatrix<2, VprCostEntry, util::Cost_Entry>(out, in, ToCostEntry);
}

static void FromMatrixCostEntry(
    Matrix<VprCostEntry>::Builder* out,
    const vtr::NdMatrix<util::Cost_Entry, 2>& in) {
    FromNdMatrix<2, VprCostEntry, util::Cost_Entry>(
        out, in, FromCostEntry);
}

static void ToFloat(float* out, const VprFloatEntry::Reader& in) {
    // Getting a scalar field is always "get<field name>()".
    *out = in.getValue();
}

static void FromFloat(VprFloatEntry::Builder* out, const float& in) {
    // Setting a scalar field is always "set<field name>(value)".
    out->setValue(in);
}

void CostMap::read(const std::string& file) {
    build_segment_map();
    MmapFile f(file);

    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto cost_map = reader.getRoot<VprCostMap>();
    {
        const auto& offset = cost_map.getOffset();
        ToNdMatrix<2, VprVector2D, std::pair<int, int>>(
            &offset_, offset, ToVprVector2D);
    }

    {
        const auto& cost_maps = cost_map.getCostMap();
        ToNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<util::Cost_Entry, 2>>(
            &cost_map_, cost_maps, ToMatrixCostEntry);
    }

    {
        const auto& penalty = cost_map.getPenalty();
        ToNdMatrix<2, VprFloatEntry, float>(
            &penalty_, penalty, ToFloat);
    }
}

void CostMap::write(const std::string& file) const {
    ::capnp::MallocMessageBuilder builder;

    auto cost_map = builder.initRoot<VprCostMap>();

    {
        auto offset = cost_map.initOffset();
        FromNdMatrix<2, VprVector2D, std::pair<int, int>>(
            &offset, offset_, FromVprVector2D);
    }

    {
        auto cost_maps = cost_map.initCostMap();
        FromNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<util::Cost_Entry, 2>>(
            &cost_maps, cost_map_, FromMatrixCostEntry);
    }

    {
        auto penalty = cost_map.initPenalty();
        FromNdMatrix<2, VprFloatEntry, float>(
            &penalty, penalty_, FromFloat);
    }

    writeMessageToFile(file, &builder);
}
#endif
