#include "connection_box_lookahead_map.h"

#include <vector>
#include <queue>

#include "connection_box.h"
#include "rr_node.h"
#include "router_lookahead_map_utils.h"
#include "globals.h"
#include "vtr_math.h"
#include "vtr_time.h"
#include "vtr_geometry.h"
#include "echo_files.h"

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

// Sample based an NxN grid of starting segments, where N = SAMPLE_GRID_SIZE
static constexpr int SAMPLE_GRID_SIZE = 3;

// Don't continue storing a path after hitting a lower-or-same cost entry.
static constexpr bool BREAK_ON_MISS = true;

// Distance penalties filling are calculated based on available samples, but can be adjusted with this factor.
static constexpr float PENALTY_FACTOR = 1.f;

// a sample point for a segment type, contains all segments at the VPR location
struct SamplePoint {
    uint64_t order; // used to order sample points
    vtr::Point<int> location;
    std::vector<ssize_t> samples;
    SamplePoint()
        : location(0, 0) {}
};

// a grid of sample points
struct SampleGrid {
    SamplePoint point[SAMPLE_GRID_SIZE][SAMPLE_GRID_SIZE];
};

template<typename Entry>
static std::pair<float, int> run_dijkstra(int start_node_ind,
                                          RoutingCosts* routing_costs);

static void find_inodes_for_segment_types(std::vector<SampleGrid>* inodes_for_segment);

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

// resize internal data structures
void CostMap::set_counts(size_t seg_count, size_t box_count) {
    cost_map_.clear();
    offset_.clear();
    penalty_.clear();
    cost_map_.resize({seg_count, box_count});
    offset_.resize({seg_count, box_count});
    penalty_.resize({seg_count, box_count});
    seg_count_ = seg_count;
    box_count_ = box_count;

    const auto& device_ctx = g_vpr_ctx.device();
    segment_map_.resize(device_ctx.rr_nodes.size());
    for (size_t i = 0; i < segment_map_.size(); ++i) {
        auto& from_node = device_ctx.rr_nodes[i];

        int from_cost_index = from_node.cost_index();
        int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

        segment_map_[i] = from_seg_index;
    }
}

// cached node -> segment map
int CostMap::node_to_segment(int from_node_ind) const {
    return segment_map_[from_node_ind];
}

static util::Cost_Entry penalize(const util::Cost_Entry& entry, int distance, float penalty) {
    return util::Cost_Entry(entry.delay + distance * penalty * PENALTY_FACTOR,
                            entry.congestion);
}

// get a cost entry for a segment type, connection box type, and offset
util::Cost_Entry CostMap::find_cost(int from_seg_index, ConnectionBoxId box_id, int delta_x, int delta_y) const {
    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
    const auto& cost_map = cost_map_[from_seg_index][size_t(box_id)];
    if (cost_map.dim_size(0) == 0 || cost_map.dim_size(1) == 0) {
        return util::Cost_Entry();
    }

    vtr::Point<int> coord(delta_x - offset_[from_seg_index][size_t(box_id)].first,
                          delta_y - offset_[from_seg_index][size_t(box_id)].second);
    vtr::Rect<int> bounds(0, 0, cost_map.dim_size(0), cost_map.dim_size(1));
    auto closest = closest_point_in_rect(bounds, coord);
    auto cost = cost_map_[from_seg_index][size_t(box_id)][closest.x()][closest.y()];
    float penalty = penalty_[from_seg_index][size_t(box_id)];
    auto distance = manhattan_distance(closest, coord);
    return penalize(cost, distance, penalty);
}

// set the cost map for a segment type and connection box type, filling holes
void CostMap::set_cost_map(const RoutingCosts& delay_costs, const RoutingCosts& base_costs) {
    // calculate the bounding boxes
    vtr::Matrix<vtr::Rect<int>> bounds({seg_count_, box_count_});
    for (const auto& entry : delay_costs) {
        bounds[entry.first.seg_index][size_t(entry.first.box_id)].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }
    for (const auto& entry : base_costs) {
        bounds[entry.first.seg_index][size_t(entry.first.box_id)].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }

    // store bounds
    for (size_t seg = 0; seg < seg_count_; seg++) {
        for (size_t box = 0; box < box_count_; box++) {
            const auto& seg_box_bounds = bounds[seg][box];
            if (seg_box_bounds.empty()) {
                // Didn't find any sample routes, so routing isn't possible between these segment/connection box types.
                offset_[seg][box] = std::make_pair(0, 0);
                cost_map_[seg][box] = vtr::NdMatrix<util::Cost_Entry, 2>(
                    {size_t(0), size_t(0)});
                continue;
            } else {
                offset_[seg][box] = std::make_pair(seg_box_bounds.xmin(), seg_box_bounds.ymin());
                cost_map_[seg][box] = vtr::NdMatrix<util::Cost_Entry, 2>(
                    {size_t(seg_box_bounds.width()), size_t(seg_box_bounds.height())});
            }
        }
    }

    // store entries into the matrices
    for (const auto& entry : delay_costs) {
        int seg = entry.first.seg_index;
        int box = size_t(entry.first.box_id);
        const auto& seg_box_bounds = bounds[seg][box];
        int x = entry.first.delta.x() - seg_box_bounds.xmin();
        int y = entry.first.delta.y() - seg_box_bounds.ymin();
        cost_map_[seg][box][x][y].delay = entry.second;
    }
    for (const auto& entry : base_costs) {
        int seg = entry.first.seg_index;
        int box = size_t(entry.first.box_id);
        const auto& seg_box_bounds = bounds[seg][box];
        int x = entry.first.delta.x() - seg_box_bounds.xmin();
        int y = entry.first.delta.y() - seg_box_bounds.ymin();
        cost_map_[seg][box][x][y].congestion = entry.second;
    }

    // fill the holes
    for (size_t seg = 0; seg < seg_count_; seg++) {
        for (size_t box = 0; box < box_count_; box++) {
            penalty_[seg][box] = std::numeric_limits<float>::infinity();
            const auto& seg_box_bounds = bounds[seg][box];
            if (seg_box_bounds.empty()) {
                continue;
            }
            auto& matrix = cost_map_[seg][box];

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
            penalty_[seg][box] = delay_penalty;

            // find missing cost entries and fill them in by copying a nearby cost entry
            std::vector<std::tuple<unsigned, unsigned, util::Cost_Entry>> missing;
            bool couldnt_fill = false;
            auto shifted_bounds = vtr::Rect<int>(0, 0, seg_box_bounds.width(), seg_box_bounds.height());
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
                VTR_LOG("At %d -> %d: max_fill = %d, delay_penalty = %e\n", seg, box, max_fill, delay_penalty);
            }

            // write back the missing entries
            for (auto& xy_entry : missing) {
                matrix[std::get<0>(xy_entry)][std::get<1>(xy_entry)] = std::get<2>(xy_entry);
            }

            if (couldnt_fill) {
                VTR_LOG_WARN("Couldn't fill holes in the cost matrix for %d -> %ld, %d x %d bounding box\n",
                             seg, box, seg_box_bounds.width(), seg_box_bounds.height());
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
    const auto& device_ctx = g_vpr_ctx.device();
    for (size_t box_id = 0;
         box_id < device_ctx.connection_boxes.num_connection_box_types();
         box_id++) {
        auto& matrix = cost_map_[iseg][box_id];
        if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) {
            VTR_LOG("cost EMPTY for box_id = %lu\n", box_id);
            continue;
        }
        VTR_LOG("cost for box_id = %lu (%zu, %zu)\n", box_id, matrix.dim_size(0), matrix.dim_size(1));
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
    for (int iseg = 0; iseg < (int)cost_map_.dim_size(0); iseg++) {
        for (int box_id = 0; box_id < (int)cost_map_.dim_size(1); box_id++) {
            auto& matrix = cost_map_[iseg][box_id];
            if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) results.push_back(std::make_pair(iseg, box_id));
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
            if (!std::isfinite(fill.delay)) {
                fill.delay = min_entry.delay;
            }
            if (!std::isfinite(fill.congestion)) {
                fill.congestion = min_entry.congestion;
            }
        }
    }
    return std::make_pair(fill, n);
}

// derive a cost from the map between two nodes
float ConnectionBoxMapLookahead::get_map_cost(int from_node_ind,
                                              int to_node_ind,
                                              float criticality_fac) const {
    if (from_node_ind == to_node_ind) {
        return 0.f;
    }

    auto& device_ctx = g_vpr_ctx.device();

    auto to_node_type = device_ctx.rr_nodes[to_node_ind].type();

    if (to_node_type == SINK) {
        const auto& sink_to_ipin = device_ctx.connection_boxes.find_sink_connection_boxes(to_node_ind);
        float cost = std::numeric_limits<float>::infinity();

        // Find cheapest cost from from_node_ind to IPINs for this SINK.
        for (int i = 0; i < sink_to_ipin.ipin_count; ++i) {
            cost = std::min(cost,
                            get_map_cost(
                                from_node_ind,
                                sink_to_ipin.ipin_nodes[i], criticality_fac));
            if (cost <= 0.f) break;
        }

        return cost;
    }

    if (device_ctx.rr_nodes[to_node_ind].type() != IPIN) {
        VPR_THROW(VPR_ERROR_ROUTE, "Not an IPIN/SINK, is %d", to_node_ind);
    }
    ConnectionBoxId box_id;
    std::pair<size_t, size_t> box_location;
    float site_pin_delay;
    bool found = device_ctx.connection_boxes.find_connection_box(
        to_node_ind, &box_id, &box_location, &site_pin_delay);
    if (!found) {
        VPR_THROW(VPR_ERROR_ROUTE, "No connection box for IPIN %d", to_node_ind);
    }

    const std::pair<size_t, size_t>* from_canonical_loc = device_ctx.connection_boxes.find_canonical_loc(from_node_ind);
    if (from_canonical_loc == nullptr) {
        VPR_THROW(VPR_ERROR_ROUTE, "No canonical loc for %d (to %d)",
                  from_node_ind, to_node_ind);
    }

    ssize_t dx = ssize_t(from_canonical_loc->first) - ssize_t(box_location.first);
    ssize_t dy = ssize_t(from_canonical_loc->second) - ssize_t(box_location.second);

    int from_seg_index = cost_map_.node_to_segment(from_node_ind);
    util::Cost_Entry cost_entry = cost_map_.find_cost(from_seg_index, box_id, dx, dy);

    if (!cost_entry.valid()) {
        // there is no route
        VTR_LOGV_DEBUG(f_router_debug,
                       "Not connected %d (%s, %d) -> %d (%s, %d, (%d, %d))\n",
                       from_node_ind, device_ctx.rr_nodes[from_node_ind].type_string(), from_seg_index,
                       to_node_ind, device_ctx.rr_nodes[to_node_ind].type_string(),
                       (int)size_t(box_id), (int)box_location.first, (int)box_location.second);
        return std::numeric_limits<float>::infinity();
    }

    float expected_delay = cost_entry.delay;
    float expected_congestion = cost_entry.congestion;

    expected_delay += site_pin_delay;

    float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;

    VTR_LOGV_DEBUG(f_router_debug, "Lookahead congestion: %g\n", expected_congestion);
    VTR_LOGV_DEBUG(f_router_debug, "Criticality: %g\n", criticality_fac);
    VTR_LOGV_DEBUG(f_router_debug, "Lookahead cost: %g\n", expected_cost);
    VTR_LOGV_DEBUG(f_router_debug, "Site pin delay: %g\n", site_pin_delay);

    if (!std::isfinite(expected_cost) || expected_cost < 0.f) {
        VTR_LOG_ERROR("invalid cost for segment %d to connection box %d at (%d, %d)\n", from_seg_index, (int)size_t(box_id), (int)dx, (int)dy);
        VTR_ASSERT(0);
    }

    return expected_cost;
}

// add a best cost routing path from start_node_ind to node_ind to routing costs
template<typename Entry>
static void add_paths(int start_node_ind,
                      Entry current,
                      std::unordered_map<int, util::Search_Path>* paths,
                      RoutingCosts* routing_costs) {
    auto& device_ctx = g_vpr_ctx.device();
    ConnectionBoxId box_id;
    std::pair<size_t, size_t> box_location;
    float site_pin_delay;
    int node_ind = current.rr_node_ind;
    bool found = device_ctx.connection_boxes.find_connection_box(
        node_ind, &box_id, &box_location, &site_pin_delay);
    if (!found) {
        VPR_THROW(VPR_ERROR_ROUTE, "No connection box for IPIN %d", node_ind);
    }

    // reconstruct the path
    std::vector<int> path;
    for (int i = (*paths)[node_ind].parent; i != start_node_ind; i = (*paths)[i].parent) {
        path.push_back(i);
    }
    path.push_back(start_node_ind);

    current.adjust_Tsw(-site_pin_delay);

    // add each node along the path subtracting the incremental costs from the current costs
    Entry start_to_here(start_node_ind, UNDEFINED, nullptr);
    int parent = start_node_ind;
    for (auto it = path.rbegin(); it != path.rend(); it++) {
        auto& here = device_ctx.rr_nodes[*it];
        int seg_index = device_ctx.rr_indexed_data[here.cost_index()].seg_index;
        const std::pair<size_t, size_t>* from_canonical_loc = device_ctx.connection_boxes.find_canonical_loc(*it);
        if (from_canonical_loc == nullptr) {
            VPR_THROW(VPR_ERROR_ROUTE, "No canonical location of node %d",
                      parent);
        }

        vtr::Point<int> delta(ssize_t(from_canonical_loc->first) - ssize_t(box_location.first),
                              ssize_t(from_canonical_loc->second) - ssize_t(box_location.second));
        RoutingCostKey key = {
            seg_index,
            box_id,
            delta};

        if (*it != start_node_ind) {
            auto& parent_node = device_ctx.rr_nodes[parent];
            start_to_here = Entry(*it, parent_node.edge_switch((*paths)[*it].edge), &start_to_here);
            parent = *it;
        }

        float cost = current.cost() - start_to_here.cost();
        if (cost < 0.f && cost > -1e-15 /* 1 femtosecond */) {
            cost = 0.f;
        }

        VTR_ASSERT(cost >= 0.f);

        // NOTE: implements REPRESENTATIVE_ENTRY_METHOD == SMALLEST
        auto result = routing_costs->insert(std::make_pair(key, cost));
        if (!result.second) {
            if (cost < result.first->second) {
                result.first->second = cost;
            } else if (BREAK_ON_MISS) {
                break;
            }
        }
    }
}

/* runs Dijkstra's algorithm from specified node until all nodes have been
 * visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored to an entry in the routing_cost_map */
template<typename Entry>
static std::pair<float, int> run_dijkstra(int start_node_ind,
                                          RoutingCosts* routing_costs) {
    auto& device_ctx = g_vpr_ctx.device();
    int path_count = 0;

    /* a list of boolean flags (one for each rr node) to figure out if a
     * certain node has already been expanded */
    std::vector<bool> node_expanded(device_ctx.rr_nodes.size(), false);
    /* For each node keep a list of the cost with which that node has been
     * visited (used to determine whether to push a candidate node onto the
     * expansion queue.
     * Also store the parent node so we can reconstruct a specific path. */
    std::unordered_map<int, util::Search_Path> paths;
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

        // the last cost should be the highest
        max_cost = current.cost();

        /* check that we haven't already expanded from this node */
        if (node_expanded[node_ind]) {
            continue;
        }

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (device_ctx.rr_nodes[node_ind].type() == IPIN) {
            path_count++;
            add_paths<Entry>(start_node_ind, current, &paths, routing_costs);
        } else {
            util::expand_dijkstra_neighbours(device_ctx.rr_nodes,
                                             current, paths, node_expanded, pq);
            node_expanded[node_ind] = true;
        }
    }
    return std::make_pair(max_cost, path_count);
}

static uint64_t interleave(uint32_t x) {
    uint64_t i = x;
    i = (i ^ (i << 16)) & 0x0000ffff0000ffff;
    i = (i ^ (i << 8)) & 0x00ff00ff00ff00ff;
    i = (i ^ (i << 4)) & 0x0f0f0f0f0f0f0f0f;
    i = (i ^ (i << 2)) & 0x3333333333333333;
    i = (i ^ (i << 1)) & 0x5555555555555555;
    return i;
}

// compute the cost maps for lookahead
void ConnectionBoxMapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Computing connection box lookahead map");

    size_t num_segments = segment_inf.size();
    std::vector<SamplePoint> sample_points;
    {
        std::vector<SampleGrid> inodes_for_segment(num_segments);
        find_inodes_for_segment_types(&inodes_for_segment);

        // collapse into a vector
        for (auto& grid : inodes_for_segment) {
            for (int y = 0; y < SAMPLE_GRID_SIZE; y++) {
                for (int x = 0; x < SAMPLE_GRID_SIZE; x++) {
                    auto& point = grid.point[y][x];
                    if (!point.samples.empty()) {
                        point.order = interleave(point.location.x()) | (interleave(point.location.y()) << 1);
                        sample_points.push_back(point);
                    }
                }
            }
        }
    }

    // sort by VPR coordinate
    std::sort(sample_points.begin(), sample_points.end(),
              [](const SamplePoint& a, const SamplePoint& b) {
                  return a.order < b.order;
              });

    /* free previous delay map and allocate new one */
    auto& device_ctx = g_vpr_ctx.device();
    cost_map_.set_counts(segment_inf.size(),
                         device_ctx.connection_boxes.num_connection_box_types());

    VTR_ASSERT(REPRESENTATIVE_ENTRY_METHOD == util::SMALLEST);
    RoutingCosts all_delay_costs;
    RoutingCosts all_base_costs;

    /* run Dijkstra's algorithm for each segment type & channel type combination */
#if defined(VPR_USE_TBB)
    tbb::mutex all_costs_mutex;
    tbb::parallel_for_each(sample_points, [&](const SamplePoint& point) {
#else
    for (const auto& point : sample_points) {
#endif
        // holds the cost entries for a run
        RoutingCosts delay_costs;
        RoutingCosts base_costs;

        // statistics
        vtr::Timer run_timer;
        int path_count = 0;
        float max_delay_cost = 0.f;
        float max_base_cost = 0.f;

        for (auto node_ind : point.samples) {
            {
                auto result = run_dijkstra<util::PQ_Entry_Delay>(node_ind, &delay_costs);
                max_delay_cost = std::max(max_delay_cost, result.first);
                path_count += result.second;
            }
            {
                auto result = run_dijkstra<util::PQ_Entry_Base_Cost>(node_ind, &base_costs);
                max_base_cost = std::max(max_base_cost, result.first);
                path_count += result.second;
            }
        }

#if defined(VPR_USE_TBB)
        all_costs_mutex.lock();
#endif
        /*
         * for (auto node_ind : point.samples) {
         * VTR_LOG("Expanded node %s\n", describe_rr_node(node_ind).c_str());
         * }
         */
        VTR_LOG("Expanded %d paths starting at (%d, %d) max_cost %e %e (%g paths/sec)\n",
                path_count,
                point.location.x(), point.location.y(),
                max_delay_cost, max_base_cost,
                path_count / run_timer.elapsed_sec());

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
        int iseg, box_id;
        std::tie(iseg, box_id) = p;
        VTR_LOG("cost map for %s(%d), connection box %d EMPTY\n",
                segment_inf[iseg].name.c_str(), iseg, box_id);
    }
#endif
}

// get an expected minimum cost for routing from the current node to the target node
float ConnectionBoxMapLookahead::get_expected_cost(
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

// the smallest bounding box containing a node
static vtr::Rect<int> bounding_box_for_node(const ConnectionBoxes& connection_boxes, int node_ind) {
    const std::pair<size_t, size_t>* loc = connection_boxes.find_canonical_loc(node_ind);
    if (loc == nullptr) {
        return vtr::Rect<int>();
    } else {
        return vtr::Rect<int>(vtr::Point<int>(loc->first, loc->second));
    }
}

static vtr::Point<int> choose_point(const vtr::Matrix<int>& counts, const vtr::Rect<int>& bounding_box, int sx, int sy, int n) {
    vtr::Rect<int> window(sample(bounding_box, sx, sy, n),
                          sample(bounding_box, sx + 1, sy + 1, n));
    vtr::Point<int> center = sample(window, 1, 1, 2);
    vtr::Point<int> sample_point = center;
    int sample_distance = 0;
    int sample_count = counts[sample_point.x()][sample_point.y()];
    for (int y = window.ymin(); y < window.ymax(); y++) {
        for (int x = window.xmin(); x < window.xmax(); x++) {
            vtr::Point<int> here(x, y);
            int count = counts[x][y];
            if (count < sample_count) continue;
            int distance = manhattan_distance(center, here);
            if (count > sample_count || (count == sample_count && distance < sample_distance)) {
                sample_point = here;
                sample_count = count;
                sample_distance = distance;
            }
        }
    }
    return sample_point;
}

// linear lookup, so consider something more sophisticated for large SAMPLE_GRID_SIZEs
static std::pair<int, int> grid_lookup(const SampleGrid& grid, vtr::Point<int> point) {
    for (int sy = 0; sy < SAMPLE_GRID_SIZE; sy++) {
        for (int sx = 0; sx < SAMPLE_GRID_SIZE; sx++) {
            if (grid.point[sy][sx].location == point) {
                return std::make_pair(sx, sy);
            }
        }
    }
    return std::make_pair(-1, -1);
}

// for each segment type, find the nearest nodes to an equally spaced grid of points
// within the bounding box for that segment type
static void find_inodes_for_segment_types(std::vector<SampleGrid>* inodes_for_segment) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_nodes = device_ctx.rr_nodes;
    const int num_segments = inodes_for_segment->size();
    std::vector<vtr::Matrix<int>> segment_counts(num_segments);

    // compute bounding boxes for each segment type
    std::vector<vtr::Rect<int>> bounding_box_for_segment(num_segments, vtr::Rect<int>());
    for (size_t i = 0; i < rr_nodes.size(); i++) {
        auto& node = rr_nodes[i];
        if (node.type() != CHANX && node.type() != CHANY) continue;
        int seg_index = device_ctx.rr_indexed_data[node.cost_index()].seg_index;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);

        bounding_box_for_segment[seg_index].expand_bounding_box(bounding_box_for_node(device_ctx.connection_boxes, i));
    }

    // initialize counts
    for (int seg = 0; seg < num_segments; seg++) {
        const auto& box = bounding_box_for_segment[seg];
        segment_counts[seg] = vtr::Matrix<int>({size_t(box.width()), size_t(box.height())}, 0);
    }

    // initialize the samples vector for each sample point
    inodes_for_segment->clear();
    inodes_for_segment->resize(num_segments);
    for (auto& grid : *inodes_for_segment) {
        for (int y = 0; y < SAMPLE_GRID_SIZE; y++) {
            for (int x = 0; x < SAMPLE_GRID_SIZE; x++) {
                grid.point[y][x].samples = std::vector<ssize_t>();
            }
        }
    }

    // count sample points
    for (size_t i = 0; i < rr_nodes.size(); i++) {
        auto& node = rr_nodes[i];
        if (node.type() != CHANX && node.type() != CHANY) continue;
        if (node.capacity() == 0) continue;
        const std::pair<size_t, size_t>* loc = device_ctx.connection_boxes.find_canonical_loc(i);
        if (loc == nullptr) continue;

        int seg_index = device_ctx.rr_indexed_data[node.cost_index()].seg_index;
        segment_counts[seg_index][loc->first][loc->second] += 1;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);
    }

    // select sample points
    for (int i = 0; i < num_segments; i++) {
        const auto& counts = segment_counts[i];
        const auto& bounding_box = bounding_box_for_segment[i];
        if (bounding_box.empty()) continue;
        auto& grid = (*inodes_for_segment)[i];
        for (int y = 0; y < SAMPLE_GRID_SIZE; y++) {
            for (int x = 0; x < SAMPLE_GRID_SIZE; x++) {
                grid.point[y][x].location = choose_point(counts, bounding_box, x, y, SAMPLE_GRID_SIZE);
            }
        }
    }

    // select an inode near the center of the bounding box for each segment type
    for (size_t i = 0; i < rr_nodes.size(); i++) {
        auto& node = rr_nodes[i];
        if (node.type() != CHANX && node.type() != CHANY) continue;
        if (node.capacity() == 0) continue;
        const std::pair<size_t, size_t>* loc = device_ctx.connection_boxes.find_canonical_loc(i);
        if (loc == nullptr) continue;

        int seg_index = device_ctx.rr_indexed_data[node.cost_index()].seg_index;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);

        auto& grid = (*inodes_for_segment)[seg_index];
        auto grid_loc = grid_lookup(grid, vtr::Point<int>(loc->first, loc->second));
        if (grid_loc.first >= 0) {
            grid.point[grid_loc.first][grid_loc.second].samples.push_back(i);
        }
    }
}

#ifndef VTR_ENABLE_CAPNPROTO

void ConnectionBoxMapLookahead::read(const std::string& file) {
    VPR_THROW(VPR_ERROR_ROUTE, "ConnectionBoxMapLookahead::read not implemented");
}
void ConnectionBoxMapLookahead::write(const std::string& file) const {
    VPR_THROW(VPR_ERROR_ROUTE, "ConnectionBoxMapLookahead::write not implemented");
}

#else

void ConnectionBoxMapLookahead::read(const std::string& file) {
    cost_map_.read(file);
}
void ConnectionBoxMapLookahead::write(const std::string& file) const {
    cost_map_.write(file);
}

static void ToCostEntry(util::Cost_Entry* out, const VprCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
}

static void FromCostEntry(VprCostEntry::Builder* out, const util::Cost_Entry& in) {
    out->setDelay(in.delay);
    out->setCongestion(in.congestion);
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
    MmapFile f(file);

    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto cost_map = reader.getRoot<VprCostMap>();

    {
        const auto& segment_map = cost_map.getSegmentMap();
        segment_map_.resize(segment_map.size());
        auto dst_iter = segment_map_.begin();
        for (const auto& src : segment_map) {
            *dst_iter++ = src;
        }
    }

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
        auto segment_map = cost_map.initSegmentMap(segment_map_.size());
        for (size_t i = 0; i < segment_map_.size(); ++i) {
            segment_map.set(i, segment_map_[i]);
        }
    }

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
