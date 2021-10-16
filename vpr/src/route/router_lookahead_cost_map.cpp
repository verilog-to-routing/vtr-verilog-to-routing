#include "router_lookahead_cost_map.h"

#include "router_lookahead_map_utils.h"
#include "globals.h"
#include "echo_files.h"
#include "vtr_geometry.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "extended_map_lookahead.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif

// Lookahead penalties constants
// Penalities are added for deltas that are outside of a specific segment bounding box region.
// These penalties are calculated based on the distance of the requested delta to a valid closest point of the bounding
// box.

///@brief Factor to adjust the penalty calculation for deltas outside the segment bounding box:
//      factor < 1.0: penalty has less impact on the final returned delay
//      factor > 1.0: penalty has more impact on the final returned delay
static constexpr float PENALTY_FACTOR = 1.f;

///@brief Minimum penalty cost that is added when penalizing a delta outside the segment bounding box.
static constexpr float PENALTY_MIN = 1e-12f;

// also known as the L1 norm
static int manhattan_distance(const vtr::Point<int>& a, const vtr::Point<int>& b) {
    return abs(b.x() - a.x()) + abs(b.y() - a.y());
}

template<typename T>
static vtr::Point<T> closest_point_in_rect(const vtr::Rect<T>& r, const vtr::Point<T>& p) {
    if (r.empty()) {
        return vtr::Point<T>(0, 0);
    } else {
        return vtr::Point<T>(vtr::clamp<T>(p.x(), r.xmin(), r.xmax() - 1),
                             vtr::clamp<T>(p.y(), r.ymin(), r.ymax() - 1));
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

/**
 * @brief Gets the segment index corresponding to a node index
 * @param from_node_ind node index
 * @return segment index corresponding to the input node
 */
int CostMap::node_to_segment(int from_node_ind) const {
    const auto& device_ctx = g_vpr_ctx.device();
    auto from_cost_index = device_ctx.rr_graph.node_cost_index(RRNodeId(from_node_ind));
    return device_ctx.rr_indexed_data[from_cost_index].seg_index;
}

/**
 * @brief Penalizes a specific cost entry based on its distance to the cost map bounds.
 * @param entry cost entry to penalize
 * @param distance distance to the cost map bounds (can be zero in case the entry is within the bounds)
 * @param penalty penalty factor relative to the current segment type
 *
 * If a specific (delta_x, delta_y) coordinates pair falls out of a segment bounding box, the returned cost is a result of the following equation:
 *
 * delay + distance * penalty * PENALTY_FACTOR
 *
 * delay    : delay of the closest point in the bounding box for the specific wire segment
 * distance : this can assume two values:
 *              - 0     : in case the deltas fall within the bounding box (in this case no penalty is added)
 *              - non-0 : in case the point is out of the bounding box, the value is the manhattan distance between the point and the closest point within the bounding box.
 * penalty  : value determined when building the lookahead and relative to a specific segment type (meaning there is one penalty value for each segment type).
 *            The base delay_penalty value is a calculated as follows:
 *              (max_delay - min_delay) / max(1, manhattan_distance(max_location, min_location))
 *
 * PENALTY_FACTOR: impact of the penalty on the total delay cost.
 */
static util::Cost_Entry penalize(const util::Cost_Entry& entry, int distance, float penalty) {
    penalty = std::max(penalty, PENALTY_MIN);
    return util::Cost_Entry(entry.delay + distance * penalty * PENALTY_FACTOR,
                            entry.congestion, entry.fill);
}

/**
 * @brief Gets a cost entry for a segment type, and delta_x, delta_y coordinates.
 * @note Cost entries are pre-computed during the lookahead map generation.
 *
 *   @param from_seg_index Index of the segment corresponding to the current expansion node
 *   @param delta_x x coordinate corresponding to the distance between the source and destination nodes
 *   @param delta_y y coordinate corresponding to the distance between the source and destination nodes
 *   @return The requested cost entry in the lookahead map.
 *
 * */
util::Cost_Entry CostMap::find_cost(int from_seg_index, int delta_x, int delta_y) const {
    VTR_ASSERT(from_seg_index >= 0 && from_seg_index < (ssize_t)offset_.size());
    const auto& cost_map = cost_map_[0][from_seg_index];
    // Check whether the cost map corresponding to the input segment is empty.
    // This can be due to an absence of samples during the lookahead generation.
    // This check is required to avoid unexpected behavior when querying an empty map.
    if (cost_map.dim_size(0) == 0 || cost_map.dim_size(1) == 0) {
        return util::Cost_Entry();
    }

    // Delta coordinate with the offset adjusted to fit the segment bounding box
    vtr::Point<int> coord(delta_x - offset_[0][from_seg_index].first,
                          delta_y - offset_[0][from_seg_index].second);
    vtr::Rect<int> bounds(0, 0, cost_map.dim_size(0), cost_map.dim_size(1));

    // Get the closest point in the bounding box:
    //      - If the coordinate is within the bounding box, the closest point will coincide with the original coordinates.
    //      - If the coordinate is outside the bounding box, the chosen point will be the one within the bounding box that is
    //        closest to the original coordinates.
    auto closest = closest_point_in_rect(bounds, coord);

    // Get the cost entry from the cost map at the deltas values
    auto cost = cost_map_[0][from_seg_index][closest.x()][closest.y()];

    // Get the base penalty corresponding to the current segment.
    float penalty = penalty_[0][from_seg_index];

    // Get the distance between the closest point in the bounding box and the original coordinates.
    // Note that if the original coordinates are within the bounding box, the distance will be equal to zero.
    auto distance = manhattan_distance(closest, coord);

    // Return the penalized cost (no penalty is added if the coordinates are within the bounding box).
    return penalize(cost, distance, penalty);
}

// finds the penalty delay corresponding to a segment
float CostMap::get_penalty(vtr::NdMatrix<util::Cost_Entry, 2>& matrix) const {
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

    return delay_penalty;
}

// fills the holes in the cost map matrix
void CostMap::fill_holes(vtr::NdMatrix<util::Cost_Entry, 2>& matrix, int seg_index, int bounding_box_width, int bounding_box_height, float delay_penalty) {
    // find missing cost entries and fill them in by copying a nearby cost entry
    std::vector<std::tuple<unsigned, unsigned, util::Cost_Entry>> missing;
    bool couldnt_fill = false;
    auto shifted_bounds = vtr::Rect<int>(0, 0, bounding_box_width, bounding_box_height);
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
        VTR_LOG("At %d: max_fill = %d, delay_penalty = %e\n", seg_index, max_fill, delay_penalty);
    }

    // write back the missing entries
    for (auto& xy_entry : missing) {
        matrix[std::get<0>(xy_entry)][std::get<1>(xy_entry)] = std::get<2>(xy_entry);
    }

    if (couldnt_fill) {
        VTR_LOG_WARN("Couldn't fill holes in the cost matrix for %ld, %d x %d bounding box\n",
                     seg_index, bounding_box_width, bounding_box_height);
        for (unsigned y = 0; y < matrix.dim_size(1); y++) {
            for (unsigned x = 0; x < matrix.dim_size(0); x++) {
                VTR_ASSERT(!matrix[x][y].valid());
            }
        }
    }
}

// set the cost map for a segment type and chan type, filling holes
void CostMap::set_cost_map(const util::RoutingCosts& delay_costs, const util::RoutingCosts& base_costs) {
    // Calculate the bounding boxes
    // Bounding boxes are used to reduce the cost map size. They are generated based on the minimum
    // and maximum coordinates of the x/y delta delays obtained for each segment.
    //
    // There is one bounding box for each segment type.
    //
    // In case the lookahead is queried to return a cost that is outside of the bounding box, the closest
    // cost within the bounding box is returned, with the addition of a calculated penalty cost.
    //
    // The bounding boxes do have two dimensions and are specified as matrices.
    // The first dimension though is unused and set to be constant as it is required to keep consistency
    // with the lookahead map. The extended lookahead map and the normal one index the various costs as follows:
    //      - Lookahead map         : [0..chan_index][seg_index][dx][dy]
    //      - Extended lookahead map: [0][seg_index][dx][dy]
    //
    // The extended lookahead map does not require the first channel index distinction, therefore the first dimension
    // remains unused.
    vtr::Matrix<vtr::Rect<int>> bounds({1, seg_count_});
    for (const auto& entry : delay_costs) {
        bounds[0][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }
    for (const auto& entry : base_costs) {
        bounds[0][entry.first.seg_index].expand_bounding_box(vtr::Rect<int>(entry.first.delta));
    }

    // Adds the above generated bounds to the cost map.
    // Also the offset_ data is stored, to allow the application of the adjustment factor to the
    // delta x/y coordinates.
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

    // Fill the cost map entries with the delay and congestion costs obtained in the dijkstra expansion step.
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

    // Adjust the cost map in two steps.
    //      1. penalty calculation: value used to add a penalty cost to the delta x/y that fall
    //                              outside of the bounding box for a specific segment
    //      2. holes filling: some entries might miss delay/congestion data. These holes are being
    //                        filled in a spiral fashion, starting from the hole, to find the nearby
    //                        valid cost.
    for (size_t seg = 0; seg < seg_count_; seg++) {
        penalty_[0][seg] = std::numeric_limits<float>::infinity();
        const auto& seg_bounds = bounds[0][seg];
        if (seg_bounds.empty()) {
            continue;
        }
        auto& matrix = cost_map_[0][seg];

        // Penalty factor calculation for the current segment
        float delay_penalty = this->get_penalty(matrix);
        penalty_[0][seg] = delay_penalty;

        // Holes filling
        this->fill_holes(matrix, seg, seg_bounds.width(), seg_bounds.height(), delay_penalty);
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

/*
 * The following static functions are used to have a fast read and write access to
 * the cost map data structures, exploiting the capnp serialization.
 */

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
