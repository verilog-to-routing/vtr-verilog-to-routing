#include <cmath>

#include "util.hpp"

using tatum::EdgeId;
using tatum::Time;

float relative_error(float A, float B) {
    if (A == B) {
        return 0.;
    }

    if (fabs(B) > fabs(A)) {
        return fabs((A - B) / B);
    } else {
        return fabs((A - B) / A);
    }
}

void remap_delay_calculator(const tatum::TimingGraph& tg, tatum::FixedDelayCalculator& dc, const tatum::util::linear_map<EdgeId,EdgeId>& edge_id_map) {

    tatum::util::linear_map<EdgeId,Time> max_edge_delays(edge_id_map.size());
    tatum::util::linear_map<EdgeId,Time> setup_times(edge_id_map.size());
    tatum::util::linear_map<EdgeId,Time> min_edge_delays(edge_id_map.size());
    tatum::util::linear_map<EdgeId,Time> hold_times(edge_id_map.size());

    //Re-map
    for(size_t iedge = 0; iedge < edge_id_map.size(); ++iedge) {
        EdgeId old_edge(iedge);
        EdgeId new_edge = edge_id_map[old_edge];

        max_edge_delays[new_edge] = dc.max_edge_delay(tg, old_edge);
        setup_times[new_edge] = dc.setup_time(tg, old_edge);
        min_edge_delays[new_edge] = dc.min_edge_delay(tg, old_edge);
        hold_times[new_edge] = dc.hold_time(tg, old_edge);

    }

    //Update
    dc = tatum::FixedDelayCalculator(max_edge_delays, setup_times, min_edge_delays, hold_times);
}
