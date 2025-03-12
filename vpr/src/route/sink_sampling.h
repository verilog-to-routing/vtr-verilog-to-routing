#pragma once

/** @file Sink downsamplers for parallel routing. 
 *
 * These are used to get a "initial routing tree" for a net.
 * The rest of the routing is delegated to child tasks using \ref VirtualNets.
 * They will work with a strictly limited bounding box, so it's necessary
 * that the initial routing provides enough hints while routing to as
 * few sinks as possible. */

#include <cmath>
#include <limits>
#include <vector>
#include "globals.h"
#include "partition_tree.h"
#include "route_common.h"
#include "router_lookahead_sampling.h"

/** Sink container for geometry operations */
struct SinkPoint {
    /** Position inside grid (x) */
    int x;
    /** Position inside grid (y) */
    int y;
    /** Sink number [1..# of pins]*/
    int isink;

    /** Operators to make this work with std::set */
    bool operator==(const SinkPoint& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator<(const SinkPoint& rhs) const {
        if (x < rhs.x)
            return true;
        if (x > rhs.x)
            return false;
        if (y < rhs.y)
            return true;
        if (y > rhs.y)
            return false;
        return isink < rhs.isink;
    }
};

/* Namespace the following to keep these generic fn names available */
namespace sink_sampling {

/* TODO: Replace this convex hull fn with something better
 * it was cobbled together quickly, probably not the most efficient code */

/** Cross product of v0v1 and v0p */
constexpr int det(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1) {
    return (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
}

/** Which side of [v0, v1] has p? +1 is right, -1 is left */
constexpr int side_of(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1) {
    return det(p, v0, v1) > 0 ? 1 : -1;
}

/** Perpendicular distance of p to v0v1 assuming |v0v1| = 1
 * (it's not, so only use to compare when v0 and v1 is the same for different p's) */
inline int dist(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1) {
    return abs(det(p, v0, v1));
}

/** Helper for quickhull() */
inline void find_hull(std::set<SinkPoint>& out, const std::vector<SinkPoint>& points, const SinkPoint& v0, const SinkPoint& v1, int side) {
    int max_dist = 0;
    const SinkPoint* max_p = nullptr;
    for (auto& point : points) {
        if (side_of(point, v0, v1) != side) {
            continue;
        }
        int h = dist(point, v0, v1);
        if (h > max_dist) {
            max_dist = h;
            max_p = &point;
        }
    }
    if (!max_p) /* no point */
        return;
    out.insert(*max_p);
    find_hull(out, points, v0, *max_p, -1);
    find_hull(out, points, *max_p, v1, -1);
}

/** Find convex hull. Doesn't work with <3 points.
 * See https://en.wikipedia.org/wiki/Quickhull */
inline std::vector<SinkPoint> quickhull(const std::vector<SinkPoint>& points) {
    if (points.size() < 3)
        return std::vector<SinkPoint>();

    std::set<SinkPoint> out;

    int min_x = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    const SinkPoint *min_p = nullptr, *max_p = nullptr;
    for (auto& point : points) {
        if (point.x <= min_x) {
            min_x = point.x;
            min_p = &point;
        }
        if (point.x >= max_x) {
            max_x = point.x;
            max_p = &point;
        }
    }
    out.insert(*min_p);
    out.insert(*max_p);
    find_hull(out, points, *min_p, *max_p, -1);
    find_hull(out, points, *min_p, *max_p, 1);
    return std::vector<SinkPoint>(out.begin(), out.end());
}

} // namespace sink_sampling

/** Which side of the cutline is this RRNode on?
 * Cutlines are always assumed to be at cutline_axis = (cutline_pos + 0.5). */
inline Side which_side(RRNodeId inode, Axis cutline_axis, int cutline_pos) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (cutline_axis == Axis::X) {
        int x = rr_graph.node_xlow(inode);
        return Side(x > cutline_pos); /* 1 is RIGHT */
    } else {
        int y = rr_graph.node_ylow(inode);
        return Side(y > cutline_pos);
    }
}

/** Sample sinks on the convex hull of the set {source + sinks}.
 * Works with regular and virtual nets. Pass vnet.clipped_bb for \p net_bb in case of a virtual net. */
inline void convex_hull_downsample(ParentNetId net_id, const t_bb& net_bb, vtr::dynamic_bitset<>& out) {
    const auto& route_ctx = g_vpr_ctx.routing();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();
    std::vector<SinkPoint> sink_points;

    /* i = 0 corresponds to the source */
    for (size_t i = 0; i < tree.num_sinks() + 1; i++) {
        RRNodeId rr_sink = route_ctx.net_rr_terminals[net_id][i];
        if (!inside_bb(rr_sink, net_bb))
            continue;
        int x = rr_graph.node_xlow(rr_sink);
        int y = rr_graph.node_ylow(rr_sink);
        SinkPoint point{x, y, int(i)};
        sink_points.push_back(point);
    }

    auto hull = sink_sampling::quickhull(sink_points);

    auto& is_isink_reached = tree.get_is_isink_reached();

    /* Sample if not source */
    for (auto& point : hull) {
        if (point.isink == 0) /* source */
            continue;
        if(is_isink_reached.get(point.isink))
            continue;
        out.set(point.isink, true);
    }
}
