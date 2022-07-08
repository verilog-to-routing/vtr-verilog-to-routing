#include "router_lookahead_sampling.h"

#include <vector>

#include "globals.h"
#include "vtr_math.h"
#include "vtr_geometry.h"
#include "vtr_time.h"

// Sample based an NxN grid of starting segments, where N = SAMPLE_GRID_SIZE
static constexpr int SAMPLE_GRID_SIZE = 2;

// quantiles (like percentiles but 0-1) of segment count to use as a selection criteria
// choose locations with higher, but not extreme, counts of each segment type
static constexpr double kSamplingCountLowerQuantile = 0.5;
static constexpr double kSamplingCountUpperQuantile = 0.7;

// also known as the L1 norm
static int manhattan_distance(const vtr::Point<int>& a, const vtr::Point<int>& b) {
    return abs(b.x() - a.x()) + abs(b.y() - a.y());
}

// the smallest bounding box containing a node
// This builds a rectangle from (x, y) to (x+1, y+1)
static vtr::Rect<int> bounding_box_for_node(RRNodeId node) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& rr_graph = device_ctx.rr_graph;
    int x = rr_graph.node_xlow(node);
    int y = rr_graph.node_ylow(node);

    return vtr::Rect<int>(vtr::Point<int>(x, y));
}

// Returns a sub-rectangle from bounding_box split into an n x n grid,
// with sx and sy in [0, n-1] selecting the column and row, respectively.
static vtr::Rect<int> sample_window(const vtr::Rect<int>& bounding_box, int sx, int sy, int n) {
    return vtr::Rect<int>(sample(bounding_box, sx, sy, n),
                          sample(bounding_box, sx + 1, sy + 1, n));
}

// Chooses all points within the sample window within the given count range,
// sorted outward from the center by Manhattan distance, the result of which
// is stored in the points member of a SampleRegion.
static std::vector<SamplePoint> choose_points(const vtr::Matrix<int>& counts,
                                              const vtr::Rect<int>& window,
                                              int min_count,
                                              int max_count) {
    VTR_ASSERT(min_count <= max_count);
    std::vector<SamplePoint> points;
    for (int y = window.ymin(); y < window.ymax(); y++) {
        for (int x = window.xmin(); x < window.xmax(); x++) {
            if (counts[x][y] >= min_count && counts[x][y] <= max_count) {
                points.push_back(SamplePoint{/* .location = */ vtr::Point<int>(x, y),
                                             /* .nodes = */ {}});
            }
        }
    }

    vtr::Point<int> center = sample(window, 1, 1, 2);

    // sort by distance from center
    std::sort(points.begin(), points.end(),
              [&](const SamplePoint& a, const SamplePoint& b) {
                  return manhattan_distance(a.location, center) < manhattan_distance(b.location, center);
              });

    return points;
}

// histogram is a map from segment count to number of locations having that count
static int quantile(const std::map<int, int>& histogram, float ratio) {
    if (histogram.empty()) {
        return 0;
    }
    int sum = 0;
    for (const auto& entry : histogram) {
        sum += entry.second;
    }
    int limit = std::ceil(sum * ratio);
    for (const auto& entry : histogram) {
        limit -= entry.second;
        if (limit <= 0) {
            return entry.first;
        }
    }
    return 0;
}

// Computes a histogram of counts within the box, where counts are nodes of a given segment type in this case.
//
// This is used to avoid choosing starting points where there extremely low or extremely high counts,
// which lead to bad sampling or high runtime for little gain.
static std::map<int, int> count_histogram(const vtr::Rect<int>& box, const vtr::Matrix<int>& counts) {
    std::map<int, int> histogram;
    for (int y = box.ymin(); y < box.ymax(); y++) {
        for (int x = box.xmin(); x < box.xmax(); x++) {
            int count = counts[x][y];
            if (count > 0) {
                ++histogram[count];
            }
        }
    }
    return histogram;
}

// Used to calculate each region's `order.'
// A space-filling curve will order the regions so that
// nearby points stay close in order. A Hilbert curve might
// be better, but a Morton (Z)-order curve is easy to compute,
// because it's just interleaving binary bits, so this
// function interleaves with 0's so that the X and Y
// dimensions can then be OR'ed together.
//
// This is used to order sample points to increase cache locality.
// It achieves a ~5-10% run-time improvement.
static uint64_t interleave(uint32_t x) {
    uint64_t i = x;
    i = (i ^ (i << 16)) & 0x0000ffff0000ffff;
    i = (i ^ (i << 8)) & 0x00ff00ff00ff00ff;
    i = (i ^ (i << 4)) & 0x0f0f0f0f0f0f0f0f;
    i = (i ^ (i << 2)) & 0x3333333333333333;
    i = (i ^ (i << 1)) & 0x5555555555555555;
    return i;
}

// Used to get a valid segment index given an input rr_node.
// If the node is not interesting an invalid value is returned.
static std::tuple<int, int, int> get_node_info(const t_rr_node& node, int num_segments) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    RRNodeId rr_node = node.id();

    if (rr_graph.node_type(rr_node) != CHANX && rr_graph.node_type(rr_node) != CHANY) {
        return std::tuple<int, int, int>(OPEN, OPEN, OPEN);
    }

    if (rr_graph.node_capacity(rr_node) == 0 || rr_graph.num_edges(rr_node) == 0) {
        return std::tuple<int, int, int>(OPEN, OPEN, OPEN);
    }

    int x = rr_graph.node_xlow(rr_node);
    int y = rr_graph.node_ylow(rr_node);

    int seg_index = device_ctx.rr_indexed_data[rr_graph.node_cost_index(rr_node)].seg_index;

    VTR_ASSERT(seg_index != OPEN);
    VTR_ASSERT(seg_index < num_segments);

    return std::tuple<int, int, int>(seg_index, x, y);
}

// Fills the sample_regions vector
static void compute_sample_regions(std::vector<SampleRegion>& sample_regions,
                                   std::vector<vtr::Matrix<int>>& segment_counts,
                                   std::vector<vtr::Rect<int>>& bounding_box_for_segment,
                                   int num_segments) {
    // select sample points
    for (int i = 0; i < num_segments; i++) {
        const auto& counts = segment_counts[i];
        const auto& bounding_box = bounding_box_for_segment[i];
        if (bounding_box.empty()) continue;
        for (int y = 0; y < SAMPLE_GRID_SIZE; y++) {
            for (int x = 0; x < SAMPLE_GRID_SIZE; x++) {
                vtr::Rect<int> window = sample_window(bounding_box, x, y, SAMPLE_GRID_SIZE);
                if (window.empty()) continue;

                auto histogram = count_histogram(window, segment_counts[i]);
                SampleRegion region = {
                    /* .segment_type = */ i,
                    /* .grid_location = */ vtr::Point<int>(x, y),
                    /* .points = */ choose_points(counts, window, quantile(histogram, kSamplingCountLowerQuantile), quantile(histogram, kSamplingCountUpperQuantile)),
                    /* .order = */ 0};
                if (!region.points.empty()) {
                    /* In order to improve caching, the list of sample points are
                     * sorted to keep points that are nearby on the Euclidean plane also
                     * nearby in the vector of sample points.
                     *
                     * This means subsequent expansions on the same thread are likely
                     * to cover a similar set of nodes, so they are more likely to be
                     * cached. This improves performance by about 7%, which isn't a lot,
                     * but not a bad improvement for a few lines of code. */
                    vtr::Point<int> location = region.points[0].location;

                    // interleave bits of X and Y for a Z-curve ordering.
                    region.order = interleave(location.x()) | (interleave(location.y()) << 1);

                    sample_regions.push_back(region);
                }
            }
        }
    }
}

// for each segment type, find the nearest nodes to an equally spaced grid of points
// within the bounding box for that segment type
std::vector<SampleRegion> find_sample_regions(int num_segments) {
    vtr::ScopedStartFinishTimer timer("finding sample regions");
    std::vector<SampleRegion> sample_regions;
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    std::vector<vtr::Matrix<int>> segment_counts(num_segments);

    // compute bounding boxes for each segment type
    std::vector<vtr::Rect<int>> bounding_box_for_segment(num_segments, vtr::Rect<int>());
    for (auto& node : rr_graph.rr_nodes()) {
        if (rr_graph.node_type(node.id()) != CHANX && rr_graph.node_type(node.id()) != CHANY) continue;
        if (rr_graph.node_capacity(node.id()) == 0 || rr_graph.num_edges(node.id()) == 0) continue;
        int seg_index = device_ctx.rr_indexed_data[rr_graph.node_cost_index(node.id())].seg_index;

        VTR_ASSERT(seg_index != OPEN);
        VTR_ASSERT(seg_index < num_segments);

        bounding_box_for_segment[seg_index].expand_bounding_box(bounding_box_for_node(node.id()));
    }

    // initialize counts
    for (int seg = 0; seg < num_segments; seg++) {
        const auto& box = bounding_box_for_segment[seg];
        segment_counts[seg] = vtr::Matrix<int>({size_t(box.width()), size_t(box.height())}, 0);
    }

    // count sample points
    for (const auto& node : rr_graph.rr_nodes()) {
        int seg_index, x, y;
        std::tie(seg_index, x, y) = get_node_info(node, num_segments);

        if (seg_index == OPEN) continue;

        segment_counts[seg_index][x][y] += 1;
    }

    compute_sample_regions(sample_regions, segment_counts, bounding_box_for_segment, num_segments);

    // sort regions
    std::sort(sample_regions.begin(), sample_regions.end(),
              [](const SampleRegion& a, const SampleRegion& b) {
                  return a.order < b.order;
              });

    // build an index of sample points on segment type and location
    std::map<std::tuple<int, int, int>, SamplePoint*> sample_point_index;
    for (auto& region : sample_regions) {
        for (auto& point : region.points) {
            sample_point_index[std::make_tuple(region.segment_type, point.location.x(), point.location.y())] = &point;
        }
    }

    // collect the node indices for each segment type at the selected sample points
    for (const auto& node : rr_graph.rr_nodes()) {
        int seg_index, x, y;
        std::tie(seg_index, x, y) = get_node_info(node, num_segments);

        if (seg_index == OPEN) continue;

        auto point = sample_point_index.find(std::make_tuple(seg_index, x, y));
        if (point != sample_point_index.end()) {
            point->second->nodes.push_back(node.id());
        }
    }

    return sample_regions;
}
