/** Sink downsamplers for parallel routing. 
 *
 * These are used to get a "minimal skeleton routing" from the main task. 
 * Rest of the routing is delegated to child tasks. They will work with a
 * strictly limited bounding box, so it's necessary that the initial routing
 * provides enough hints while routing to as few sinks as possible to limit
 * the serial bottleneck. */
#pragma once

#include <cmath>
#include <limits>
#include <vector>
#include "globals.h"
#include "partition_tree.h"
#include "route_common.h"
#include "router_lookahead_sampling.h"

/** Minimum bin size when spatially sampling decomposition sinks. (I know, doesn't make much sense.)
 * The parallel router tries to decompose nets by building a "skeleton routing" from the main task
 * and then delegating the remaining work to its child tasks. This minimum bin size determines how much
 * time the main thread spends building the skeleton.
 * Less is more effort -> less speedup, better quality.
 * See get_decomposition_isinks() for more info. */
constexpr size_t MIN_DECOMP_BIN_WIDTH = 5;

/** Sink container for geometry operations */
struct SinkPoint {
    int x;
    int y;
    int isink;

    bool operator==(const SinkPoint& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator<(const SinkPoint& rhs) const {
        if(x < rhs.x)
            return true;
        if(x > rhs.x)
            return false;
        if(y < rhs.y)
            return true;
        if(y > rhs.y)
            return false;
        return isink < rhs.isink;
    }
};

/** Find convex hull. Doesn't work with <3 points.
 * See https://en.wikipedia.org/wiki/Quickhull */
std::vector<SinkPoint> quickhull(const std::vector<SinkPoint>& points);

/** Which side of the cutline is this RRNode?
 * Cutlines are always assumed to be at cutline_axis = (cutline_pos + 0.5).
 * In the context of the parallel router, a RR node is considered to be inside a bounding
 * box if its top left corner (xlow, ylow) is inside it. */
inline Side which_side(RRNodeId inode, int cutline_pos, Axis axis) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (axis == Axis::X) {
        return Side(rr_graph.node_xlow(inode) > cutline_pos); /* 1 is RIGHT */
    } else {
        return Side(rr_graph.node_ylow(inode) > cutline_pos);
    }
}

/** Sample most critical sink in every MIN_DECOMP_BIN_WIDTH-wide bin. Bins are grown to absorb fractional bins.
 * Skip a bin if already reached by existing routing. */
inline std::vector<int> min_voxel_downsample(ParentNetId net_id, const std::vector<int>& remaining_targets) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    std::vector<int> out;

    /* Set up sampling bins. If we are sampling from W = 22 with minimum width 6, then we have
     * 3 bins and real width is 22/3 + 1 = 8. Then x=0 goes to bin 0, x=8 goes to bin 1 etc. */
    const t_bb& net_bb = route_ctx.route_bb[net_id];
    size_t width = net_bb.xmax - net_bb.xmin + 1;
    size_t height = net_bb.ymax - net_bb.ymin + 1;
    size_t bins_x = width / MIN_DECOMP_BIN_WIDTH;
    size_t bins_y = height / MIN_DECOMP_BIN_WIDTH;
    size_t samples_to_find = bins_x * bins_y;
    size_t bin_width_x = width / bins_x + 1;
    size_t bin_width_y = height / bins_y + 1;

    /* The sample for each bin, indexed by [x][y]. Set to -1 if reached by existing routing,
     * 0 if not found yet. */
    std::vector<std::vector<int>> samples(bins_x, std::vector<int>(bins_y));
    constexpr int REACHED = -1;
    constexpr int NONE = 0;

    /* Mark bins with already reached sinks. */
    for (int isink : tree.get_reached_isinks()) {
        if (samples_to_find == 0)
            return out;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        size_t x = (rr_graph.node_xlow(rr_sink) - net_bb.xmin) / bin_width_x;
        size_t y = (rr_graph.node_ylow(rr_sink) - net_bb.ymin) / bin_width_y;
        if (samples[x][y] != REACHED) {
            samples[x][y] = REACHED;
            samples_to_find--;
        }
    }

    /* Spatially sample remaining targets. This should be already sorted by pin criticality,
     * so we sample the most critical sink in the bin right away. */
    for (int isink : remaining_targets) {
        if (samples_to_find == 0)
            return out;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        size_t x = (rr_graph.node_xlow(rr_sink) - net_bb.xmin) / bin_width_x;
        size_t y = (rr_graph.node_ylow(rr_sink) - net_bb.ymin) / bin_width_y;
        if (samples[x][y] == NONE) {
            samples[x][y] = isink;
            out.push_back(isink);
            samples_to_find--;
        }
    }

    return out;
}

/** Sample sinks on the convex hull of the set {source + sinks}. Skip sinks if already reached. */
inline void convex_hull_downsample(ParentNetId net_id, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    std::vector<SinkPoint> sink_points;

    /* i = 0 corresponds to the source */
    for(size_t i = 0; i < tree.num_sinks()+1; i++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][i];
        SinkPoint point {rr_graph.node_xlow(rr_sink), rr_graph.node_ylow(rr_sink), int(i)};
        sink_points.push_back(point);
    }

    auto hull = quickhull(sink_points);

    auto& is_isink_reached = tree.get_is_isink_reached();
    /* Sample if not reached and not source */
    for(auto& point: hull){
        if(point.isink == 0) /* source */
            continue;
        if(!is_isink_reached[point.isink])
            out.insert(point.isink);
    }
}

/** Clip bb to one side of the cutline given the axis and position of the cutline.
 * Note that cutlines are assumed to be at axis = cutline_pos + 0.5. */
inline t_bb clip_to_side2(const t_bb& bb, Axis axis, int cutline_pos, Side side) {
    t_bb out = bb;
    if (axis == Axis::X && side == Side::LEFT)
        out.xmax = cutline_pos;
    else if (axis == Axis::X && side == Side::RIGHT)
        out.xmin = cutline_pos + 1;
    else if (axis == Axis::Y && side == Side::LEFT)
        out.ymax = cutline_pos;
    else if (axis == Axis::Y && side == Side::RIGHT)
        out.ymin = cutline_pos + 1;
    else
        VTR_ASSERT_MSG(false, "Unreachable");
    return out;
}

inline int dist2(int x1, int y1, int x2, int y2){
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

/** Sample one sink closest to each bbox's epicenter. The rationale is that the 
 * sinks around the cutline will be sampled by the sink thickness rule anyway. */
inline void sample_both_epicenters(ParentNetId net_id, int cutline_pos, Axis cutline_axis, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();

    int num_sinks = tree.num_sinks();
    auto& is_isink_reached = tree.get_is_isink_reached();
    const t_bb& net_bb = route_ctx.route_bb[net_id];
    t_bb left_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::LEFT);
    t_bb right_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::RIGHT);
    int left_epi_x = (left_bb.xmin + left_bb.xmax) / 2;
    int left_epi_y = (left_bb.ymin + left_bb.ymax) / 2;
    int right_epi_x = (right_bb.xmin + right_bb.xmax) / 2;
    int right_epi_y = (right_bb.ymin + right_bb.ymax) / 2;
    int best_score_left = std::numeric_limits<int>::max();
    int best_score_right = std::numeric_limits<int>::max();
    int best_left_isink = 0;
    int best_right_isink = 0;

    for(int isink=1; isink<num_sinks+1; isink++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        int x = rr_graph.node_xlow(rr_sink);
        int y = rr_graph.node_ylow(rr_sink);
        if(inside_bb(rr_sink, left_bb)){
            int score = dist2(x, y, left_epi_x, left_epi_y);
            if(score < best_score_left){
                best_score_left = score;
                best_left_isink = isink;
            }
        }else{
            int score = dist2(x, y, right_epi_x, right_epi_y);
            if(score < best_score_right){
                best_score_right = score;
                best_right_isink = isink;
            }
        }
    }

    if(best_left_isink && !is_isink_reached[best_left_isink])
        out.insert(best_left_isink);
    if(best_right_isink && !is_isink_reached[best_right_isink])
        out.insert(best_right_isink);
}

/** Sample one sink closest to each bbox's epicenter. The rationale is that the 
 * sinks around the cutline will be sampled by the sink thickness rule anyway. */
inline void sample_both_epicenters_vnet(const VirtualNet& vnet, int cutline_pos, Axis cutline_axis, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();

    int num_sinks = tree.num_sinks();
    auto& is_isink_reached = tree.get_is_isink_reached();
    const t_bb& net_bb = vnet.clipped_bb;
    t_bb left_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::LEFT);
    t_bb right_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::RIGHT);
    int left_epi_x = (left_bb.xmin + left_bb.xmax) / 2;
    int left_epi_y = (left_bb.ymin + left_bb.ymax) / 2;
    int right_epi_x = (right_bb.xmin + right_bb.xmax) / 2;
    int right_epi_y = (right_bb.ymin + right_bb.ymax) / 2;
    int best_score_left = std::numeric_limits<int>::max();
    int best_score_right = std::numeric_limits<int>::max();
    int best_left_isink = 0;
    int best_right_isink = 0;

    for(int isink=1; isink<num_sinks+1; isink++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][isink];
        int x = rr_graph.node_xlow(rr_sink);
        int y = rr_graph.node_ylow(rr_sink);
        if(inside_bb(rr_sink, left_bb)){
            int score = dist2(x, y, left_epi_x, left_epi_y);
            if(score < best_score_left){
                best_score_left = score;
                best_left_isink = isink;
            }
        }else if(inside_bb(rr_sink, right_bb)){
            int score = dist2(x, y, right_epi_x, right_epi_y);
            if(score < best_score_right){
                best_score_right = score;
                best_right_isink = isink;
            }
        }
    }

    if(best_left_isink && !is_isink_reached[best_left_isink])
        out.insert(best_left_isink);
    if(best_right_isink && !is_isink_reached[best_right_isink])
        out.insert(best_right_isink);
}

/** Sample sinks on the convex hull of the set {source + sinks}. Skip sinks if already reached. */
inline void convex_hull_downsample_vnet(const VirtualNet& vnet, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    std::vector<SinkPoint> sink_points;

    /* i = 0 corresponds to the source */
    for(size_t i = 0; i < tree.num_sinks()+1; i++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][i];
        if(!inside_bb(rr_sink, vnet.clipped_bb))
            continue;
        SinkPoint point {rr_graph.node_xlow(rr_sink), rr_graph.node_ylow(rr_sink), int(i)};
        sink_points.push_back(point);
    }

    auto hull = quickhull(sink_points);

    auto& is_isink_reached = tree.get_is_isink_reached();
    /* Sample if not reached and not source */
    for(auto& point: hull){
        if(point.isink == 0) /* source */
            continue;
        if(!is_isink_reached[point.isink])
            out.insert(point.isink);
    }
}

/** Sample sinks on the *sink side* of the convex hull of the set {source + sinks}.
 * Skip sinks if already reached. */
inline std::vector<int> half_convex_hull_downsample(ParentNetId net_id, int cutline_pos, Axis cutline_axis) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    std::vector<int> out;
    std::vector<SinkPoint> sink_points;

    /* i = 0 corresponds to the source */
    for(size_t i = 0; i < tree.num_sinks()+1; i++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][i];
        SinkPoint point {rr_graph.node_xlow(rr_sink), rr_graph.node_ylow(rr_sink), int(i)};
        sink_points.push_back(point);
    }

    auto hull = quickhull(sink_points);

    auto& is_isink_reached = tree.get_is_isink_reached();
    RRNodeId rr_source = route_ctx.net_rr_terminals[net_id][0];
    Side source_side = which_side(rr_source, cutline_pos, cutline_axis);
    /* Sample if not reached and not source */
    for(auto& point: hull){
        if(point.isink == 0 || is_isink_reached[point.isink]) /* source or reached */
            continue;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][point.isink];
        if(which_side(rr_sink, cutline_pos, cutline_axis) == source_side) /* on source side */
            continue;
        out.push_back(point.isink);
    }

    return out;
}

/** Sample sinks on the *sink side* of the convex hull of the set {source + sinks}.
 * Skip sinks if already reached. */
inline std::vector<int> half_convex_hull_downsample_vnet(const VirtualNet& vnet, int cutline_pos, Axis cutline_axis) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    std::vector<int> out;
    std::vector<SinkPoint> sink_points;

    /* i = 0 corresponds to the source */
    for(size_t i = 0; i < tree.num_sinks()+1; i++){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][i];
        if(!inside_bb(rr_sink, vnet.clipped_bb))
            continue;
        SinkPoint point {rr_graph.node_xlow(rr_sink), rr_graph.node_ylow(rr_sink), int(i)};
        sink_points.push_back(point);
    }

    auto hull = quickhull(sink_points);

    auto& is_isink_reached = tree.get_is_isink_reached();
    RRNodeId rr_source = route_ctx.net_rr_terminals[vnet.net_id][0];
    Side source_side = which_side(rr_source, cutline_pos, cutline_axis);
    /* Sample if not reached and not source */
    for(auto& point: hull){
        if(point.isink == 0 || is_isink_reached[point.isink]) /* source or reached */
            continue;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][point.isink];
        if(which_side(rr_sink, cutline_pos, cutline_axis) == source_side) /* on source side */
            continue;
        out.push_back(point.isink);
    }

    return out;
}

/** Sample the most critical sink on the other side of the cutline.
 * Sample nothing if that's already reached. */
inline std::vector<int> sample_single_sink(ParentNetId net_id, const std::vector<float>& pin_criticality, int cutline_pos, Axis cutline_axis) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    auto& is_isink_reached = tree.get_is_isink_reached();

    std::vector<int> isinks(tree.num_sinks());
    std::iota(isinks.begin(), isinks.end(), 1);
    std::sort(isinks.begin(), isinks.end(), [&](int i, int j){
        return pin_criticality[i] > pin_criticality[j];
    });

    RRNodeId rr_source = route_ctx.net_rr_terminals[net_id][0];
    Side source_side = which_side(rr_source, cutline_pos, cutline_axis);
    for(int isink: isinks){
        if(is_isink_reached[isink])
            continue;
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        if(which_side(rr_sink, cutline_pos, cutline_axis) != source_side){
            if(is_isink_reached[isink])
                return {};
            else
                return {isink};
        }
    }

    return {};
}

inline bool is_close_to_cutline2(RRNodeId inode, int cutline_pos, Axis cutline_axis, int thickness){
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    /* Cutlines are considered to be at x + 0.5, set a thickness of +1 here by checking for equality */
    if(cutline_axis == Axis::X){
        return rr_graph.node_xlow(inode) - thickness <= cutline_pos && rr_graph.node_xhigh(inode) + thickness >= cutline_pos;
    } else {
        return rr_graph.node_ylow(inode) - thickness <= cutline_pos && rr_graph.node_yhigh(inode) + thickness >= cutline_pos;
    }
}

/** Is \p inode too close to this bb? (Assuming it's inside)
 * We assign some "thickness" to the node and check for collision */
inline bool is_close_to_bb2(RRNodeId inode, const t_bb& bb, int thickness){
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int xlow = rr_graph.node_xlow(inode) - thickness;
    int ylow = rr_graph.node_ylow(inode) - thickness;
    int xhigh = rr_graph.node_xhigh(inode) + thickness;
    int yhigh = rr_graph.node_yhigh(inode) + thickness;

    return (xlow <= bb.xmin && xhigh >= bb.xmin)
        || (ylow <= bb.ymin && yhigh >= bb.ymin)
        || (xlow <= bb.xmax && xhigh >= bb.xmax)
        || (ylow <= bb.ymax && yhigh >= bb.ymax);
}

/** Sample the most critical sinks on both sides. Omit reached sinks. */
inline void sample_two_sinks(ParentNetId net_id, const std::vector<float>& pin_criticality, int cutline_pos, Axis cutline_axis, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    auto& is_isink_reached = tree.get_is_isink_reached();

    std::vector<int> isinks(tree.num_sinks());
    std::iota(isinks.begin(), isinks.end(), 1);
    std::sort(isinks.begin(), isinks.end(), [&](int i, int j){
        return pin_criticality[i] > pin_criticality[j];
    });

    int left_isink = -1;
    int right_isink = -1;
    const t_bb& net_bb = route_ctx.route_bb[net_id];
    t_bb left_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::LEFT);
    t_bb right_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::RIGHT);

    for(int isink: isinks){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][isink];
        if(is_close_to_cutline2(rr_sink, cutline_pos, cutline_axis, 3))
            continue;
        if(inside_bb(rr_sink, left_bb)){
            left_isink = isink;
        }else if(inside_bb(rr_sink, right_bb)){
            right_isink = isink;
        }
        if(left_isink > -1 && right_isink > -1)
            break;
    }

    if(left_isink > -1 && !is_isink_reached[left_isink])
        out.insert(left_isink);
    if(right_isink > -1 && !is_isink_reached[right_isink])
        out.insert(right_isink);
}

/** Sample the most critical sinks on both sides. Omit reached sinks. */
inline void sample_two_sinks_vnet(const VirtualNet& vnet, const std::vector<float>& pin_criticality, int cutline_pos, Axis cutline_axis, std::set<int>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[vnet.net_id].value();
    auto& is_isink_reached = tree.get_is_isink_reached();

    std::vector<int> isinks(tree.num_sinks());
    std::iota(isinks.begin(), isinks.end(), 1);
    std::sort(isinks.begin(), isinks.end(), [&](int i, int j){
        return pin_criticality[i] > pin_criticality[j];
    });

    int left_isink = -1;
    int right_isink = -1;
    const t_bb& net_bb = vnet.clipped_bb;
    t_bb left_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::LEFT);
    t_bb right_bb = clip_to_side2(net_bb, cutline_axis, cutline_pos, Side::RIGHT);

    for(int isink: isinks){
        RRNodeId rr_sink = route_ctx.net_rr_terminals[vnet.net_id][isink];
        if(inside_bb(rr_sink, left_bb) && !is_close_to_cutline2(rr_sink, cutline_pos, cutline_axis, 3) && !is_close_to_bb2(rr_sink, left_bb, 1)){
            left_isink = isink;
        }else if(inside_bb(rr_sink, right_bb) && !is_close_to_cutline2(rr_sink, cutline_pos, cutline_axis, 3) && !is_close_to_bb2(rr_sink, right_bb, 1)){
            right_isink = isink;
        }
        if(left_isink > -1 && right_isink > -1)
            break;
    }

    if(left_isink > -1 && !is_isink_reached[left_isink])
        out.insert(left_isink);
    if(right_isink > -1 && !is_isink_reached[right_isink])
        out.insert(right_isink);
}