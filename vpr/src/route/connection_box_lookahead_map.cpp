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

static constexpr int SAMPLE_GRID_SIZE = 4;

typedef std::array<std::array<std::vector<ssize_t>, SAMPLE_GRID_SIZE>, SAMPLE_GRID_SIZE> SampleGrid;

static void run_dijkstra(int start_node_ind,
                         std::vector<CostMap::routing_cost>& routing_costs);

void CostMap::set_counts(size_t seg_count, size_t box_count) {
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

int CostMap::node_to_segment(int from_node_ind) const {
    return segment_map_[from_node_ind];
}

Cost_Entry CostMap::find_cost(int from_seg_index, ConnectionBoxId box_id, int delta_x, int delta_y) const {
    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
    const auto& cost_map = cost_map_[from_seg_index][size_t(box_id)];
    if (cost_map.dim_size(0) == 0 || cost_map.dim_size(1) == 0) {
        return Cost_Entry();
    }

    int dx = delta_x - offset_[from_seg_index][size_t(box_id)].first;
    int dy = delta_y - offset_[from_seg_index][size_t(box_id)].second;

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

void CostMap::set_cost_map(int from_seg_index,
                           const std::vector<routing_cost>& costs,
                           e_representative_entry_method method) {
    // sort the entries
    const auto& device_ctx = g_vpr_ctx.device();
    std::vector<std::vector<routing_cost>> costs_per_box(device_ctx.connection_boxes.num_connection_box_types());
    for (const auto& entry : costs) {
        costs_per_box[size_t(std::get<3>(entry))].push_back(entry);
    }

    for (size_t box_id = 0;
         box_id < device_ctx.connection_boxes.num_connection_box_types();
         ++box_id) {
        set_cost_map(from_seg_index, ConnectionBoxId(box_id), costs_per_box[box_id], method);
    }
}

void CostMap::set_cost_map(int from_seg_index, ConnectionBoxId box_id, const std::vector<routing_cost>& costs, e_representative_entry_method method) {
    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());

    if (costs.empty()) {
        offset_[from_seg_index][size_t(box_id)] = std::make_pair(0, 0);
        cost_map_[from_seg_index][size_t(box_id)] = vtr::NdMatrix<Cost_Entry, 2>(
            {size_t(0), size_t(0)});
        return;
    }

    // calculate the bounding box
    vtr::Rect<int> bounds;
    for (const auto& entry : costs) {
        bounds |= vtr::Rect<int>(std::get<1>(entry), 1);
    }

    offset_[from_seg_index][size_t(box_id)] = std::make_pair(bounds.xmin(), bounds.ymin());

    vtr::NdMatrix<Expansion_Cost_Entry, 2> expansion_cost_map(
        {size_t(bounds.width()), size_t(bounds.height())});

    for (const auto& entry : costs) {
        int x = std::get<1>(entry).x() - bounds.xmin();
        int y = std::get<1>(entry).y() - bounds.ymin();
        expansion_cost_map[x][y].add_cost_entry(
            method, std::get<2>(entry).delay,
            std::get<2>(entry).congestion);
    }

    cost_map_[from_seg_index][size_t(box_id)] = vtr::NdMatrix<Cost_Entry, 2>(
        {size_t(bounds.width()), size_t(bounds.height())});
    auto& matrix = cost_map_[from_seg_index][size_t(box_id)];

    /* set the lookahead cost map entries with a representative cost
     * entry from routing_cost_map */
    for (unsigned ix = 0; ix < expansion_cost_map.dim_size(0); ix++) {
        for (unsigned iy = 0; iy < expansion_cost_map.dim_size(1); iy++) {
            matrix[ix][iy] = expansion_cost_map[ix][iy].get_representative_cost_entry(method);
        }
    }

    std::list<std::tuple<unsigned, unsigned, Cost_Entry> > missing;
    bool couldnt_fill = false;
    auto shifted_bounds = vtr::Rect<int>(0, 0, bounds.width(), bounds.height());
    /* find missing cost entries and fill them in by copying a nearby cost entry */
    for (unsigned ix = 0; ix < matrix.dim_size(0) && !couldnt_fill; ix++) {
        for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
            Cost_Entry& cost_entry = matrix[ix][iy];
            if (!cost_entry.valid()) {
                // maximum search radius
                Cost_Entry filler = get_nearby_cost_entry(matrix, ix, iy, shifted_bounds);
                if (filler.valid()) {
                    missing.push_back(std::make_tuple(ix, iy, filler));
                } else {
                    couldnt_fill = true;
                }
            }
        }
    }

    // write back the missing entries
    for (auto& xy_entry : missing) {
        matrix[std::get<0>(xy_entry)][std::get<1>(xy_entry)] = std::get<2>(xy_entry);
    }

    if (couldnt_fill) {
        VTR_LOG_WARN("Couldn't fill holes in the cost matrix for %d -> %ld\n",
                     from_seg_index, size_t(box_id));
        for (unsigned y = 0; y < matrix.dim_size(1); y++) {
            for (unsigned x = 0; x < matrix.dim_size(0); x++) {
                VTR_ASSERT(!matrix[x][y].valid());
            }
        }
#if 0
        for (unsigned iy = 0; iy < matrix.dim_size(1); iy++) {
            for (unsigned ix = 0; ix < matrix.dim_size(0); ix++) {
                if(matrix[ix][iy].valid()) {
                    printf("O");
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
#endif
    }
}

void CostMap::print(int iseg) const {
    const auto& device_ctx = g_vpr_ctx.device();
    for (size_t box_id = 0;
         box_id < device_ctx.connection_boxes.num_connection_box_types();
         box_id++) {
        auto& matrix = cost_map_[iseg][box_id];
        if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) {
            printf("cost EMPTY for box_id = %lu\n", box_id);
            continue;
        }
        printf("cost for box_id = %lu\n", box_id);
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
                if (entry.valid() && entry.delay > avg) {
                    printf("o");
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
    }
}

std::list<std::pair<int, int> > CostMap::list_empty() const {
    std::list<std::pair<int, int> > results;
    for (int iseg = 0; iseg < (int)cost_map_.dim_size(0); iseg++) {
        for (int box_id = 0; box_id < (int)cost_map_.dim_size(1); box_id++) {
            auto& matrix = cost_map_[iseg][box_id];
            if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) results.push_back(std::make_pair(iseg, box_id));
        }
    }
    return results;
}

static void assign_min_entry(Cost_Entry& dst, const Cost_Entry& src) {
    if (src.delay < dst.delay) dst.delay = src.delay;
    if (src.congestion < dst.congestion) dst.congestion = src.congestion;
}

Cost_Entry CostMap::get_nearby_cost_entry(const vtr::NdMatrix<Cost_Entry, 2>& matrix,
                                          int cx,
                                          int cy,
                                          const vtr::Rect<int>& bounds) {
    // spiral around (cx, cy) looking for a nearby entry
    int n = 1, x, y;
    bool in_bounds;
    Cost_Entry entry;

    do {
        in_bounds = false;
        y = cy - n; // top
        // left -> right
        for (x = cx - n; x < cx + n; x++) {
            if (bounds.contains(vtr::Point<int>(x, y))) {
                assign_min_entry(entry, matrix[x][y]);
                in_bounds = true;
            }
        }
        x = cx + n; // right
        // top -> bottom
        for (; y < cy + n; y++) {
            if (bounds.contains(vtr::Point<int>(x, y))) {
                assign_min_entry(entry, matrix[x][y]);
                in_bounds = true;
            }
        }
        y = cy + n; // bottom
        // right -> left
        for (; x > cx - n; x--) {
            if (bounds.contains(vtr::Point<int>(x, y))) {
                assign_min_entry(entry, matrix[x][y]);
                in_bounds = true;
            }
        }
        x = cx - n; // left
        // bottom -> top
        for (; y > cy - n; y--) {
            if (bounds.contains(vtr::Point<int>(x, y))) {
                assign_min_entry(entry, matrix[x][y]);
                in_bounds = true;
            }
        }
        if (entry.valid()) return entry;
        n++;
    } while (in_bounds);
    return Cost_Entry();
}

template<typename T>
vtr::Point<T> sample(const vtr::Rect<T>& r, T x, T y, T d) {
  return vtr::Point<T>((r.xmin() * (d - x) + r.xmax() * x + d / 2) / d,
                       (r.ymin() * (d - y) + r.ymax() * y + d / 2) / d);
}

float ConnectionBoxMapLookahead::get_map_cost(int from_node_ind,
                                              int to_node_ind,
                                              float criticality_fac) const {
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
                                get_map_cost(
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
    bool found = device_ctx.connection_boxes.find_connection_box(
        to_node_ind, &box_id, &box_location);
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
    Cost_Entry cost_entry = cost_map_.find_cost(from_seg_index, box_id, dx, dy);

    if (!cost_entry.valid()) {
        // there is no route
        return std::numeric_limits<float>::infinity();
    }

    float expected_delay = cost_entry.delay;
    float expected_congestion = cost_entry.congestion;

    float expected_cost = criticality_fac * expected_delay + (1.0 - criticality_fac) * expected_congestion;
    VTR_ASSERT(std::isfinite(expected_cost) && expected_cost >= 0.f);
    return expected_cost;
}

/* runs Dijkstra's algorithm from specified node until all nodes have been
 * visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored to an entry in the routing_cost_map */
static void run_dijkstra(int start_node_ind,
                         std::vector<CostMap::routing_cost>& routing_costs) {
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
            bool found = device_ctx.connection_boxes.find_connection_box(
                node_ind, &box_id, &box_location);
            if (!found) {
                VPR_THROW(VPR_ERROR_ROUTE, "No connection box for IPIN %d", node_ind);
            }

            int delta_x = ssize_t(from_canonical_loc->first) - ssize_t(box_location.first);
            int delta_y = ssize_t(from_canonical_loc->second) - ssize_t(box_location.second);

            routing_costs.push_back(std::make_tuple(
                std::make_pair(start_node_ind, node_ind),
                vtr::Point<int>(delta_x, delta_y),
                Cost_Entry(
                    current.delay,
                    current.congestion_upstream),
                box_id));
        }

        expand_dijkstra_neighbours(current, node_visited_costs, node_expanded, pq);
        node_expanded[node_ind] = true;
    }
}

void ConnectionBoxMapLookahead::compute(const std::vector<t_segment_inf>& segment_inf,
                                        const std::string& lookahead_search_locations) {
    vtr::ScopedStartFinishTimer timer("Computing connection box lookahead map");

    size_t num_segments = segment_inf.size();
    std::vector<SampleGrid> inodes_for_segment(num_segments);
    find_inodes_for_segment_types(&inodes_for_segment);

    /* free previous delay map and allocate new one */
    auto& device_ctx = g_vpr_ctx.device();
    cost_map_.set_counts(segment_inf.size(),
                         device_ctx.connection_boxes.num_connection_box_types());

    /* run Dijkstra's algorithm for each segment type & channel type combination */
    for (int iseg = 0; iseg < (ssize_t)num_segments; iseg++) {
        VTR_LOG("Creating cost map for %s(%d)\n",
                segment_inf[iseg].name.c_str(), iseg);
        /* allocate the cost map for this iseg/chan_type */
        std::vector<CostMap::routing_cost> costs;
        bool found = false;
        for (const auto& row : inodes_for_segment[iseg]) {
            for (auto cell : row) {
                for (auto node_ind : cell) {
                    found = true;
                    run_dijkstra(node_ind, costs);
                }
            }
        }
        if (!found) {
            VTR_LOG_WARN("Segment %s(%d) found no start_node_ind\n",
                         segment_inf[iseg].name.c_str(), iseg);
        } else {
            /* boil down the cost list in routing_cost_map at each coordinate to a
             * representative cost entry and store it in the lookahead cost map */
            cost_map_.set_cost_map(iseg, costs,
                                   REPRESENTATIVE_ENTRY_METHOD);
        }
    }

    for (int iseg = 0; iseg < (ssize_t)num_segments; iseg++) {
#if 0
        for (auto &e : cost_map_per_segment[iseg]) {
            VTR_LOG("%d -> %d (%d, %d): %g, %g\n",
                    std::get<0>(e).first, std::get<0>(e).second,
                    std::get<1>(e).first, std::get<1>(e).second,
                    std::get<2>(e).delay, std::get<2>(e).congestion);
        }
#endif
        //VTR_LOG("cost map for %s(%d)\n",
        //        segment_inf[iseg].name.c_str(), iseg);
        //cost_map_.print(iseg);
    }

    for(std::pair<int, int> p : cost_map_.list_empty()) {
        int iseg, box_id;
        std::tie(iseg, box_id) = p;
        VTR_LOG("cost map for %s(%d), connection box %d EMPTY\n",
                segment_inf[iseg].name.c_str(), iseg, box_id);
    }
}

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

static int manhattan_distance(const t_rr_node& node, int x, int y) {
    int node_center_x = (node.xhigh() + node.xlow()) / 2;
    int node_center_y = (node.yhigh() + node.ylow()) / 2;
    return abs(node_center_x - x) + abs(node_center_y - y);
}

static vtr::Rect<int> bounding_box_for_node(const t_rr_node& node) {
    return vtr::Rect<int>(node.xlow(), node.ylow(),
                          node.xhigh() + 1, node.yhigh() + 1);
}

static void find_inodes_for_segment_types(std::vector<SampleGrid> *inodes_for_segment) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_nodes = device_ctx.rr_nodes;
    const int num_segments = inodes_for_segment->size();

    // compute bounding boxes for each segment type
    std::vector<vtr::Rect<int>> bounding_box_for_segment(num_segments, vtr::Rect<int>());
    for (size_t i = 0; i < rr_nodes.size(); i++) {
        auto& node = rr_nodes[i];
        if (node.type() != CHANX && node.type() != CHANY) continue;
        int seg_index = device_ctx.rr_indexed_data[node.cost_index()].seg_index;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);

        bounding_box_for_segment[seg_index] |= bounding_box_for_node(node);
    }

    // select an inode near the center of the bounding box for each segment type
    inodes_for_segment->clear();
    inodes_for_segment->resize(num_segments);
    for (auto& grid : *inodes_for_segment) {
        for (auto& row : grid) {
            for (auto& cell : row) {
                cell = std::vector<ssize_t>();
            }
        }
    }

    for (size_t i = 0; i < rr_nodes.size(); i++) {
        auto& node = rr_nodes[i];
        if (node.type() != CHANX && node.type() != CHANY) continue;
        if (node.capacity() == 0 || device_ctx.connection_boxes.find_canonical_loc(i) == nullptr) continue;

        int seg_index = device_ctx.rr_indexed_data[node.cost_index()].seg_index;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);

        auto& grid = (*inodes_for_segment)[seg_index];
        for (int sy = 0; sy < SAMPLE_GRID_SIZE; sy++) {
            for (int sx = 0; sx < SAMPLE_GRID_SIZE; sx++) {
                auto& stored_inodes = grid[sy][sx];
                if (stored_inodes.empty()) {
                    stored_inodes.push_back(i);
                    goto next_rr_node;
                }

                auto& first_stored_node = rr_nodes[stored_inodes.front()];
                if (first_stored_node.xhigh() >= node.xhigh() && first_stored_node.xlow() <= node.xlow() && first_stored_node.yhigh() >= node.yhigh() && first_stored_node.ylow() <= node.ylow()) {
                    stored_inodes.push_back(i);
                    goto next_rr_node;
                }

                vtr::Point<int> target = sample(bounding_box_for_segment[seg_index], sx + 1, sy + 1, SAMPLE_GRID_SIZE + 1);
                int distance_new = manhattan_distance(node, target.x(), target.y());
                int distance_stored = manhattan_distance(first_stored_node, target.x(), target.y());
                if (distance_new < distance_stored) {
                    stored_inodes.clear();
                    stored_inodes.push_back(i);
                    goto next_rr_node;
                }
            }
        }
    next_rr_node:
        continue;
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

    /* Increase reader limit to 1G words. */
    ::capnp::ReaderOptions opts = ::capnp::ReaderOptions();
    opts.traversalLimitInWords = 1024 * 1024 * 1024;
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
        ToNdMatrix<2, VprVector2D, std::pair<int, int> >(
            &offset_, offset, ToVprVector2D);
    }

    {
        const auto& cost_maps = cost_map.getCostMap();
        ToNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<Cost_Entry, 2> >(
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
        FromNdMatrix<2, VprVector2D, std::pair<int, int> >(
            &offset, offset_, FromVprVector2D);
    }

    {
        auto cost_maps = cost_map.initCostMap();
        FromNdMatrix<2, Matrix<VprCostEntry>, vtr::NdMatrix<Cost_Entry, 2> >(
            &cost_maps, cost_map_, FromMatrixCostEntry);
    }

    writeMessageToFile(file, &builder);
}
#endif
