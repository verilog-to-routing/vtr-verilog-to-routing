#ifndef ROUTER_LOOKAHEAD_SAMPLING_H
#define ROUTER_LOOKAHEAD_SAMPLING_H

#include <vector>
#include "vtr_geometry.h"
#include "globals.h"

// a sample point for a segment type, contains all segments at the VPR location
struct SamplePoint {
    // canonical location
    vtr::Point<int> location;

    // nodes to expand
    std::vector<RRNodeId> nodes;
};

struct SampleRegion {
    // all nodes in `points' have this segment type
    int segment_type;

    // location on the sample grid
    vtr::Point<int> grid_location;

    // locations to try
    // The computation will keep expanding each of the points
    // until a number of paths (segment -> connection box) are found.
    std::vector<SamplePoint> points;

    // used to sort the regions to improve caching
    uint64_t order;
};

std::vector<SampleRegion> find_sample_regions(int num_segments);

#endif
