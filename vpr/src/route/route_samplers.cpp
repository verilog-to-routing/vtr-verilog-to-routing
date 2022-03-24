/** Bulky geometry code for route_samplers.h
 * TODO: Make the fns available in vtr_geometry.h? */

#include "route_samplers.h"

/** Cross product of v0v1 and v0p */
constexpr int det(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1){
    return (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
}

/** Which side of [v0, v1] has p? +1 is right, -1 is left */
constexpr int which_side(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1){
    return det(p, v0, v1) > 0 ? 1 : -1;
}

/** Perpendicular distance of p to v0v1 assuming |v0v1| = 1
 * (it's not, so only use to compare when v0 and v1 is the same for different p's) */
constexpr int dist(const SinkPoint& p, const SinkPoint& v0, const SinkPoint& v1){
    return abs(det(p, v0, v1));
}

/** Helper for quickhull() */
void find_hull(std::set<SinkPoint>& out, const std::vector<SinkPoint>& points, const SinkPoint& v0, const SinkPoint& v1, int side){
    int max_dist = 0;
    const SinkPoint* max_p = nullptr;
    for(auto& point: points){
        if(which_side(point, v0, v1) != side){
            continue;
        }
        int h = dist(point, v0, v1);
        if(h > max_dist){
            max_dist = h;
            max_p = &point;
        }
    }
    if(!max_p) /* no point */
        return;
    out.insert(*max_p);
    find_hull(out, points, v0, *max_p, -1);
    find_hull(out, points, *max_p, v1, -1);
}

/** Find convex hull. Doesn't work with <3 points.
 * See https://en.wikipedia.org/wiki/Quickhull */
std::vector<SinkPoint> quickhull(const std::vector<SinkPoint>& points){
    if(points.size() < 3)
        return std::vector<SinkPoint>();

    std::set<SinkPoint> out;

    int min_x = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    const SinkPoint* min_p, *max_p;
    for(auto& point: points){
        if(point.x <= min_x){
            min_x = point.x;
            min_p = &point;
        }
        if(point.x >= max_x){
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
