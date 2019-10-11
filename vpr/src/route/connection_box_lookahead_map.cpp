#include "connection_box_lookahead_map.h"

#include <vector>
#include <queue>

#include "connection_box.h"
#include "rr_node.h"
#include "router_lookahead_map_utils.h"
#include "globals.h"
#include "vtr_math.h"
#include "vtr_time.h"
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

/* we're profiling routing cost over many tracks for each wire type, so we'll
 * have many cost entries at each |dx|,|dy| offset. There are many ways to
 * "boil down" the many costs at each offset to a single entry for a given
 * (wire type, chan_type) combination we can take the smallest cost, the
 * average, median, etc. This define selects the method we use.
 *
 * See e_representative_entry_method */
#define REPRESENTATIVE_ENTRY_METHOD SMALLEST

#define REF_X 25
#define REF_Y 23

static int signum(int x) {
    if (x > 0) return 1;
    if (x < 0)
        return -1;
    else
        return 0;
}

typedef std::vector<std::tuple<std::pair<int, int>, std::pair<int, int>, Cost_Entry, ConnectionBoxId>> t_routing_cost_map;
static void run_dijkstra(int start_node_ind,
                         t_routing_cost_map* routing_cost_map);

class CostMap {
  public:
    void set_counts(size_t seg_count, size_t box_count) {
        cost_map_.clear();
        offset_.clear();
        cost_map_.resize({seg_count, box_count});
        offset_.resize({seg_count, box_count});

        const auto& device_ctx = g_vpr_ctx.device();
        segment_map_.resize(device_ctx.rr_nodes.size());
        for (size_t i = 0; i < segment_map_.size(); ++i) {
            auto& from_node = device_ctx.rr_nodes[i];

            int from_cost_index = from_node.cost_index();
            int from_seg_index = device_ctx.rr_indexed_data[from_cost_index].seg_index;

            segment_map_[i] = from_seg_index;
        }
    }

    int node_to_segment(int from_node_ind) {
        return segment_map_[from_node_ind];
    }

    Cost_Entry find_cost(int from_seg_index, ConnectionBoxId box_id, int delta_x, int delta_y) const {
        VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
        int dx = delta_x - offset_[from_seg_index][size_t(box_id)].first;
        int dy = delta_y - offset_[from_seg_index][size_t(box_id)].second;
        const auto& cost_map = cost_map_[from_seg_index][size_t(box_id)];

        if (dx < 0) {
            dx = 0;
        }
        if (dy < 0) {
            dy = 0;
        }

        if (dx >= (ssize_t)cost_map.dim_size(0)) {
            dx = cost_map.dim_size(0) - 1;
        }
        if (dy >= (ssize_t)cost_map.dim_size(1)) {
            dy = cost_map.dim_size(1) - 1;
        }

        return cost_map_[from_seg_index][size_t(box_id)][dx][dy];
    }

    void set_cost_map(int from_seg_index,
                      const t_routing_cost_map& cost_map,
                      e_representative_entry_method method) {
        const auto& device_ctx = g_vpr_ctx.device();
        for (size_t box_id = 0;
             box_id < device_ctx.connection_boxes.num_connection_box_types();
             ++box_id) {
            set_cost_map(from_seg_index, ConnectionBoxId(box_id), cost_map, method);
        }
    }

    void set_cost_map(int from_seg_index, ConnectionBoxId box_id, const t_routing_cost_map& cost_map, e_representative_entry_method method) {
        VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());

        // Find coordinate offset for this segment.
        int min_dx = 0;
        int min_dy = 0;
        int max_dx = 0;
        int max_dy = 0;
        for (const auto& entry : cost_map) {
            if (std::get<3>(entry) != box_id) {
                continue;
            }
            min_dx = std::min(std::get<1>(entry).first, min_dx);
            min_dy = std::min(std::get<1>(entry).second, min_dy);

            max_dx = std::max(std::get<1>(entry).first, max_dx);
            max_dy = std::max(std::get<1>(entry).second, max_dy);
        }

        offset_[from_seg_index][size_t(box_id)].first = min_dx;
        offset_[from_seg_index][size_t(box_id)].second = min_dy;
        size_t dim_x = max_dx - min_dx + 1;
        size_t dim_y = max_dy - min_dy + 1;

        vtr::NdMatrix<Expansion_Cost_Entry, 2> expansion_cost_map(
            {dim_x, dim_y});

        for (const auto& entry : cost_map) {
            if (std::get<3>(entry) != box_id) {
                continue;
            }
            int x = std::get<1>(entry).first - min_dx;
            int y = std::get<1>(entry).second - min_dy;
            expansion_cost_map[x][y].add_cost_entry(
                method, std::get<2>(entry).delay,
                std::get<2>(entry).congestion);
        }

        cost_map_[from_seg_index][size_t(box_id)] = vtr::NdMatrix<Cost_Entry, 2>(
            {dim_x, dim_y});

        /* set the lookahead cost map entries with a representative cost
         * entry from routing_cost_map */
        for (unsigned ix = 0; ix < expansion_cost_map.dim_size(0); ix++) {
            for (unsigned iy = 0; iy < expansion_cost_map.dim_size(1); iy++) {
                cost_map_[from_seg_index][size_t(box_id)][ix][iy] = expansion_cost_map[ix][iy].get_representative_cost_entry(method);
            }
        }

        /* find missing cost entries and fill them in by copying a nearby cost entry */
        for (unsigned ix = 0; ix < expansion_cost_map.dim_size(0); ix++) {
            for (unsigned iy = 0; iy < expansion_cost_map.dim_size(1); iy++) {
                Cost_Entry cost_entry = cost_map_[from_seg_index][size_t(box_id)][ix][iy];

                if (!cost_entry.valid()) {
                    Cost_Entry copied_entry = get_nearby_cost_entry(
                        from_seg_index,
                        box_id,
                        offset_[from_seg_index][size_t(box_id)].first + ix,
                        offset_[from_seg_index][size_t(box_id)].second + iy);
                    cost_map_[from_seg_index][size_t(box_id)][ix][iy] = copied_entry;
                }
            }
        }
    }

    Cost_Entry get_nearby_cost_entry(int segment_index, ConnectionBoxId box_id, int x, int y) {
        /* compute the slope from x,y to 0,0 and then move towards 0,0 by one
         * unit to get the coordinates of the cost entry to be copied */

        float slope;
        int copy_x, copy_y;
        if (x == 0 || y == 0) {
            slope = std::numeric_limits<float>::infinity();
            copy_x = x - signum(x);
            copy_y = y - signum(y);
        } else {
            slope = (float)y / (float)x;
            if (slope >= 1.0) {
                copy_y = y - signum(y);
                copy_x = vtr::nint((float)y / slope);
            } else {
                copy_x = x - signum(x);
                copy_y = vtr::nint((float)x * slope);
            }
        }

        Cost_Entry copy_entry = find_cost(segment_index, box_id, copy_x, copy_y);

        /* if the entry to be copied is also empty, recurse */
        if (copy_entry.valid()) {
            return copy_entry;
        } else if (copy_x == 0 && copy_y == 0) {
            return Cost_Entry();
        }

        return get_nearby_cost_entry(segment_index, box_id, copy_x, copy_y);
    }

    void read(const std::string& file);
    void write(const std::string& file) const;

  private:
    vtr::NdMatrix<vtr::NdMatrix<Cost_Entry, 2>, 2> cost_map_;
    vtr::NdMatrix<std::pair<int, int>, 2> offset_;
    std::vector<int> segment_map_;
};

static CostMap g_cost_map;

static const std::vector<int>& get_rr_node_indcies(t_rr_type rr_type, int start_x, int start_y) {
    const auto& device_ctx = g_vpr_ctx.device();
    if (rr_type == CHANX) {
        return device_ctx.rr_node_indices[rr_type][start_y][start_x][0];
    } else if (rr_type == CHANY) {
        return device_ctx.rr_node_indices[rr_type][start_x][start_y][0];
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown channel type %d", rr_type);
    }
}

class StartNode {
  public:
    StartNode(int start_x, int start_y, t_rr_type rr_type, int seg_index)
        : start_x_(start_x)
        , start_y_(start_y)
        , rr_type_(rr_type)
        , seg_index_(seg_index)
        , index_(0) {}
    int get_next_node() {
        const auto& device_ctx = g_vpr_ctx.device();
        const std::vector<int>& channel_node_list = get_rr_node_indcies(
            rr_type_, start_x_, start_y_);

        for (; index_ < channel_node_list.size(); index_++) {
            int node_ind = channel_node_list[index_];

            if (node_ind == OPEN || device_ctx.rr_nodes[node_ind].capacity() == 0) {
                continue;
            }

            const std::pair<size_t, size_t>* loc = device_ctx.connection_boxes.find_canonical_loc(node_ind);
            if (loc == nullptr) {
                continue;
            }

            int node_cost_ind = device_ctx.rr_nodes[node_ind].cost_index();
            int node_seg_ind = device_ctx.rr_indexed_data[node_cost_ind].seg_index;
            if (node_seg_ind == seg_index_) {
                index_ += 1;
                return node_ind;
            }
        }

        return UNDEFINED;
    }

  private:
    int start_x_;
    int start_y_;
    t_rr_type rr_type_;
    int seg_index_;
    size_t index_;
};

// Minimum size of search for channels to profile.  kMinProfile results
// in searching x = [0, kMinProfile], and y = [0, kMinProfile[.
//
// Making this value larger will increase the sample size, but also the runtime
// to produce the lookahead.
static constexpr int kMinProfile = 1;

// Maximum size of search for channels to profile.  Once search is outside of
// kMinProfile distance, lookahead will stop searching once:
//  - At least one channel has been profiled
//  - kMaxProfile is exceeded.
static constexpr int kMaxProfile = 7;

static int search_at(int iseg, int start_x, int start_y, t_routing_cost_map* cost_map) {
    const auto& device_ctx = g_vpr_ctx.device();

    int count = 0;
    int dx = 0;
    int dy = 0;

    while ((count == 0 && dx < kMaxProfile) || dy <= kMinProfile) {
        if (start_x + dx >= device_ctx.grid.width()) {
            break;
        }
        if (start_y + dy >= device_ctx.grid.height()) {
            break;
        }

        for (e_rr_type chan_type : {CHANX, CHANY}) {
            StartNode start_node(start_x + dx, start_y + dy, chan_type, iseg);
            VTR_LOG("Searching for %d at (%d, %d)\n", iseg, start_x + dx, start_y + dy);

            for (int start_node_ind = start_node.get_next_node();
                 start_node_ind != UNDEFINED;
                 start_node_ind = start_node.get_next_node()) {
                count += 1;

                /* run Dijkstra's algorithm */
                run_dijkstra(start_node_ind, cost_map);
            }
        }

        if (dy < dx) {
            dy += 1;
        } else {
            dx += 1;
        }
    }

    return count;
}

static void compute_connection_box_lookahead(
    const std::vector<t_segment_inf>& segment_inf) {
    size_t num_segments = segment_inf.size();
    vtr::ScopedStartFinishTimer timer("Computing connection box lookahead map");

    /* free previous delay map and allocate new one */
    auto& device_ctx = g_vpr_ctx.device();
    g_cost_map.set_counts(segment_inf.size(),
                          device_ctx.connection_boxes.num_connection_box_types());

    /* run Dijkstra's algorithm for each segment type & channel type combination */
    for (int iseg = 0; iseg < (ssize_t)num_segments; iseg++) {
        VTR_LOG("Creating cost map for %s(%d)\n",
                segment_inf[iseg].name.c_str(), iseg);
        /* allocate the cost map for this iseg/chan_type */
        t_routing_cost_map cost_map;

        int count = 0;
        count += search_at(iseg, REF_X, REF_Y, &cost_map);
        count += search_at(iseg, REF_Y, REF_X, &cost_map);
        count += search_at(iseg, 1, 1, &cost_map);
        count += search_at(iseg, 76, 1, &cost_map);
        count += search_at(iseg, 25, 25, &cost_map);
        count += search_at(iseg, 25, 27, &cost_map);
        count += search_at(iseg, 75, 26, &cost_map);

        if (count == 0) {
            VTR_LOG_WARN("Segment %s(%d) found no start_node_ind\n",
                         segment_inf[iseg].name.c_str(), iseg);
        }

#if 0
        for(const auto & e : cost_map) {
            VTR_LOG("%d -> %d (%d, %d): %g, %g\n",
                    std::get<0>(e).first, std::get<0>(e).second,
                    std::get<1>(e).first, std::get<1>(e).second,
                    std::get<2>(e).delay, std::get<2>(e).congestion);
        }
#endif

        /* boil down the cost list in routing_cost_map at each coordinate to a
         * representative cost entry and store it in the lookahead cost map */
        g_cost_map.set_cost_map(iseg, cost_map,
                                REPRESENTATIVE_ENTRY_METHOD);
    }
}

static float get_connection_box_lookahead_map_cost(int from_node_ind,
                                                   int to_node_ind,
                                                   float criticality_fac) {
    if (from_node_ind == to_node_ind) {
        return 0.f;
    }

    auto& device_ctx = g_vpr_ctx.device();

    std::pair<size_t, size_t> from_location;
    std::pair<size_t, size_t> to_location;
    auto to_node_type = device_ctx.rr_nodes[to_node_ind].type();

    if (to_node_type == SINK) {
        const auto& sink_to_ipin = device_ctx.connection_boxes.find_sink_connection_boxes(to_node_ind);
        if (sink_to_ipin.ipin_count > 1) {
            float cost = std::numeric_limits<float>::infinity();
            // Find cheapest cost from from_node_ind to IPINs for this SINK.
            for (int i = 0; i < sink_to_ipin.ipin_count; ++i) {
                cost = std::min(cost,
                                get_connection_box_lookahead_map_cost(
                                    from_node_ind,
                                    sink_to_ipin.ipin_nodes[i], criticality_fac));
            }

            return cost;
        } else if (sink_to_ipin.ipin_count == 1) {
            to_node_ind = sink_to_ipin.ipin_nodes[0];
            if (from_node_ind == to_node_ind) {
                return 0.f;
            }
        } else {
            return std::numeric_limits<float>::infinity();
        }
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

    int from_seg_index = g_cost_map.node_to_segment(from_node_ind);
    Cost_Entry cost_entry = g_cost_map.find_cost(from_seg_index, box_id, dx, dy);
    float expected_delay = cost_entry.delay;
    float expected_congestion = cost_entry.congestion;

    float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;
    return expected_cost;
}

/* runs Dijkstra's algorithm from specified node until all nodes have been
 * visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored to an entry in the routing_cost_map */
static void run_dijkstra(int start_node_ind,
                         t_routing_cost_map* routing_cost_map) {
    auto& device_ctx = g_vpr_ctx.device();

    /* a list of boolean flags (one for each rr node) to figure out if a
     * certain node has already been expanded */
    std::vector<bool> node_expanded(device_ctx.rr_nodes.size(), false);
    /* for each node keep a list of the cost with which that node has been
     * visited (used to determine whether to push a candidate node onto the
     * expansion queue */
    std::vector<float> node_visited_costs(device_ctx.rr_nodes.size(), -1.0);
    /* a priority queue for expansion */
    std::priority_queue<util::PQ_Entry> pq;

    /* first entry has no upstream delay or congestion */
    util::PQ_Entry first_entry(start_node_ind, UNDEFINED, 0, 0, 0, true);

    pq.push(first_entry);

    const std::pair<size_t, size_t>* from_canonical_loc = device_ctx.connection_boxes.find_canonical_loc(start_node_ind);
    if (from_canonical_loc == nullptr) {
        VPR_THROW(VPR_ERROR_ROUTE, "No canonical location of node %d",
                  start_node_ind);
    }

    /* now do routing */
    while (!pq.empty()) {
        util::PQ_Entry current = pq.top();
        pq.pop();

        int node_ind = current.rr_node_ind;

        /* check that we haven't already expanded from this node */
        if (node_expanded[node_ind]) {
            continue;
        }

        /* if this node is an ipin record its congestion/delay in the routing_cost_map */
        if (device_ctx.rr_nodes[node_ind].type() == IPIN) {
            ConnectionBoxId box_id;
            std::pair<size_t, size_t> box_location;
            float site_pin_delay;
            bool found = device_ctx.connection_boxes.find_connection_box(
                node_ind, &box_id, &box_location, &site_pin_delay);
            if (!found) {
                VPR_THROW(VPR_ERROR_ROUTE, "No connection box for IPIN %d", node_ind);
            }

            int delta_x = ssize_t(from_canonical_loc->first) - ssize_t(box_location.first);
            int delta_y = ssize_t(from_canonical_loc->second) - ssize_t(box_location.second);

            routing_cost_map->push_back(std::make_tuple(
                std::make_pair(start_node_ind, node_ind),
                std::make_pair(delta_x, delta_y),
                Cost_Entry(
                    current.delay,
                    current.congestion_upstream),
                box_id));
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq);
        node_expanded[node_ind] = true;
    }
}

void ConnectionBoxMapLookahead::compute(const std::vector<t_segment_inf>& segment_inf) {
    compute_connection_box_lookahead(segment_inf);
}

float ConnectionBoxMapLookahead::get_expected_cost(
    int current_node,
    int target_node,
    const t_conn_cost_params& params,
    float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_nodes[current_node].type();

    if (rr_type == CHANX || rr_type == CHANY) {
        return get_connection_box_lookahead_map_cost(
            current_node, target_node, params.criticality);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
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
    g_cost_map.read(file);
}
void ConnectionBoxMapLookahead::write(const std::string& file) const {
    g_cost_map.write(file);
}

static void ToCostEntry(Cost_Entry* out, const VprCostEntry::Reader& in) {
    out->delay = in.getDelay();
    out->congestion = in.getCongestion();
}

static void FromCostEntry(VprCostEntry::Builder* out, const Cost_Entry& in) {
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

static void ToMatrixCostEntry(vtr::NdMatrix<Cost_Entry, 2>* out,
                              const Matrix<VprCostEntry>::Reader& in) {
    ToNdMatrix<2, VprCostEntry, Cost_Entry>(out, in, ToCostEntry);
}

static void FromMatrixCostEntry(
    Matrix<VprCostEntry>::Builder* out,
    const vtr::NdMatrix<Cost_Entry, 2>& in) {
    FromNdMatrix<2, VprCostEntry, Cost_Entry>(
        out, in, FromCostEntry);
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
        ToNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<Cost_Entry, 2>>(
            &cost_map_, cost_maps, ToMatrixCostEntry);
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
        FromNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<Cost_Entry, 2>>(
            &cost_maps, cost_map_, FromMatrixCostEntry);
    }

    writeMessageToFile(file, &builder);
}
#endif
