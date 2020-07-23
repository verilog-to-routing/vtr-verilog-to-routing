#include "router_lookahead_cost_map.h"

#include "router_lookahead_map_utils.h"
#include "globals.h"
#include "echo_files.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "extended_map_lookahead.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif

// Distance penalties filling are calculated based on available samples, but can be adjusted with this factor.
static constexpr float PENALTY_FACTOR = 1.f;
static constexpr float PENALTY_MIN = 1e-12f;

// also known as the L1 norm
static int manhattan_distance(const vtr::Point<int>& a, const vtr::Point<int>& b) {
    return abs(b.x() - a.x()) + abs(b.y() - a.y());
}

template<class T>
static constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
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
    cost_map_.resize({1, seg_count});
    offset_.resize({1, seg_count});
    penalty_.resize({1, seg_count});
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

// get a cost entry for a segment type, chan type, and offset
util::Cost_Entry CostMap::find_cost(int from_seg_index, int delta_x, int delta_y) const {
    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
    const auto& cost_map = cost_map_[0][from_seg_index];
    if (cost_map.dim_size(0) == 0 || cost_map.dim_size(1) == 0) {
        return util::Cost_Entry();
    }

    vtr::Point<int> coord(delta_x - offset_[0][from_seg_index].first,
                          delta_y - offset_[0][from_seg_index].second);
    vtr::Rect<int> bounds(0, 0, cost_map.dim_size(0), cost_map.dim_size(1));
    auto closest = closest_point_in_rect(bounds, coord);
    auto cost = cost_map_[0][from_seg_index][closest.x()][closest.y()];
    float penalty = penalty_[0][from_seg_index];
    auto distance = manhattan_distance(closest, coord);
    return penalize(cost, distance, penalty);
}

// set the cost map for a segment type and chan type, filling holes
void CostMap::set_cost_map(const util::RoutingCosts& delay_costs, const util::RoutingCosts& base_costs) {
    // calculate the bounding boxes
    vtr::Matrix<vtr::Rect<int>> bounds({1, seg_count_});
    for (const auto& entry : delay_costs) {
        bounds[0][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }
    for (const auto& entry : base_costs) {
        bounds[0][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }

    // store bounds
    for (size_t seg = 0; seg < seg_count_; seg++) {
        const auto& seg_bounds = bounds[0][seg];
        if (seg_bounds.empty()) {
            // Didn't find any sample routes, so routing isn't possible between these segment/chan types.
            offset_[0][seg] = std::make_pair(0, 0);
            cost_map_[0][seg] = vtr::NdMatrix<util::Cost_Entry, 2>(
                {size_t(0), size_t(0)});
            continue;
        } else {
            offset_[0][seg] = std::make_pair(seg_bounds.xmin(), seg_bounds.ymin());
            cost_map_[0][seg] = vtr::NdMatrix<util::Cost_Entry, 2>(
                {size_t(seg_bounds.width()), size_t(seg_bounds.height())});
        }
    }

    // store entries into the matrices
    for (const auto& entry : delay_costs) {
        int seg = entry.first.seg_index;
        const auto& seg_bounds = bounds[0][seg];
        int x = entry.first.delta.x() - seg_bounds.xmin();
        int y = entry.first.delta.y() - seg_bounds.ymin();
        cost_map_[0][seg][x][y].delay = entry.second;
    }
    for (const auto& entry : base_costs) {
        int seg = entry.first.seg_index;
        const auto& seg_bounds = bounds[0][seg];
        int x = entry.first.delta.x() - seg_bounds.xmin();
        int y = entry.first.delta.y() - seg_bounds.ymin();
        cost_map_[0][seg][x][y].congestion = entry.second;
    }

    // fill the holes
    for (size_t seg = 0; seg < seg_count_; seg++) {
        penalty_[0][seg] = std::numeric_limits<float>::infinity();
        const auto& seg_bounds = bounds[0][seg];
        if (seg_bounds.empty()) {
            continue;
        }
        auto& matrix = cost_map_[0][seg];

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
        penalty_[0][seg] = delay_penalty;

        // find missing cost entries and fill them in by copying a nearby cost entry
        std::vector<std::tuple<unsigned, unsigned, util::Cost_Entry>> missing;
        bool couldnt_fill = false;
        auto shifted_bounds = vtr::Rect<int>(0, 0, seg_bounds.width(), seg_bounds.height());
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
            VTR_LOG_WARN("Couldn't fill holes in the cost matrix for %ld, %d x %d bounding box\n",
                         seg, seg_bounds.width(), seg_bounds.height());
            for (unsigned y = 0; y < matrix.dim_size(1); y++) {
                for (unsigned x = 0; x < matrix.dim_size(0); x++) {
                    VTR_ASSERT(!matrix[x][y].valid());
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
    auto& matrix = cost_map_[0][iseg];
    if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) {
        VTR_LOG("cost EMPTY");
    }
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

// list segment type and chan type pairs that have empty cost maps (debug)
std::vector<std::pair<int, int>> CostMap::list_empty() const {
    std::vector<std::pair<int, int>> results;
    for (int iseg = 0; iseg < (int)cost_map_.dim_size(0); iseg++) {
        auto& matrix = cost_map_[0][iseg];
        if (matrix.dim_size(0) == 0 || matrix.dim_size(1) == 0) results.push_back(std::make_pair(0, iseg));
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
