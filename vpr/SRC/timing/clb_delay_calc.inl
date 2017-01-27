#include "clb_delay_calc.h"

#include "globals.h"

/*
 * ClbDelayCalc
 */
inline ClbDelayCalc::ClbDelayCalc()
    : intra_lb_pb_pin_lookup_(type_descriptors, num_types) {}

inline float ClbDelayCalc::clb_input_to_internal_sink_delay(const t_net_pin* clb_input_pin, int internal_sink_pin) const {
    return trace_max_delay(clb_input_pin->block, clb_input_pin->block_pin, internal_sink_pin);
}

inline float ClbDelayCalc::internal_src_to_clb_output_delay(int internal_src_pin, const t_net_pin* clb_output_pin) const {
    return trace_max_delay(clb_output_pin->block, internal_src_pin, clb_output_pin->block_pin);
}

inline float ClbDelayCalc::internal_src_to_internal_sink_delay(int clb, int internal_src_pin, int internal_sink_pin) const {
    return trace_max_delay(clb, internal_src_pin, internal_sink_pin);
}

inline float ClbDelayCalc::clb_input_to_clb_output_delay(const t_net_pin* clb_input_pin, const t_net_pin* clb_output_pin) const {
    VTR_ASSERT_MSG(clb_input_pin->block == clb_output_pin->block, "Route through clb input/output pins must be on the same CLB");
    return trace_max_delay(clb_input_pin->block, clb_input_pin->block_pin, clb_output_pin->block_pin);
}

inline float ClbDelayCalc::trace_max_delay(int clb, int src_pb_route_id, int sink_pb_route_id) const {
    const t_pb_route* pb_routes = block[clb].pb_route;

    AtomNetId atom_net = pb_routes[src_pb_route_id].atom_net_id;

    VTR_ASSERT_MSG(atom_net, "Source pin must be connected to a valid net");
    VTR_ASSERT_MSG(atom_net == pb_routes[sink_pb_route_id].atom_net_id, "Source pin and sink pin must connect to the same net");

    float delay = 0.;

    //Trace back the internal routing from the sink to the source pin
    int curr_pb_route_id = src_pb_route_id;
    while(pb_routes[curr_pb_route_id].driver_pb_pin_id >= 0) {
        VTR_ASSERT_MSG(atom_net == pb_routes[curr_pb_route_id].atom_net_id, "Internal routing must connect the same net");

        delay += pb_route_max_delay(clb, curr_pb_route_id);

        curr_pb_route_id = pb_routes[curr_pb_route_id].driver_pb_pin_id;
    }

    VTR_ASSERT_MSG(curr_pb_route_id == src_pb_route_id, "Sink pin should trace back to source pin");

    return delay;
}

inline float ClbDelayCalc::pb_route_max_delay(int clb_block, int pb_route_idx) const {
    const t_pb_graph_edge* pb_edge = find_pb_graph_edge(clb_block, pb_route_idx);

    if(pb_edge) {
        return pb_edge->delay_max;
    } else {
        return 0.;
    }
}

const t_pb_graph_edge* ClbDelayCalc::find_pb_graph_edge(int clb_block, int pb_route_idx) const {
    int type_index = block[clb_block].type->index;

    int upstream_pb_route_idx = block[clb_block].pb_route[pb_route_idx].driver_pb_pin_id;

    if(upstream_pb_route_idx >= 0) {

        const t_pb_graph_pin* pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, pb_route_idx);
        const t_pb_graph_pin* upstream_pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, upstream_pb_route_idx);

        return find_pb_graph_edge(upstream_pb_gpin, pb_gpin); 
    } else {
        return nullptr;
    }
}

const t_pb_graph_edge* ClbDelayCalc::find_pb_graph_edge(const t_pb_graph_pin* driver, const t_pb_graph_pin* sink) const {
    VTR_ASSERT(driver);
    VTR_ASSERT(sink);

    const t_pb_graph_edge* pb_edge = nullptr;
    for(int iedge = 0; iedge < driver->num_output_edges; ++iedge) {
        const t_pb_graph_edge* check_edge = driver->output_edges[iedge];
        VTR_ASSERT(check_edge);

        VTR_ASSERT(check_edge->num_output_pins == 1);
        if(check_edge->output_pins[0] == sink) {
            pb_edge = check_edge;
        }

    }
    VTR_ASSERT_MSG(pb_edge, "Should find pb_graph_edge connecting PB pins");

    return pb_edge;
}

/*
 * CachingClbDelayCalc
 */
inline float CachingClbDelayCalc::clb_input_to_internal_sink_delay(const t_net_pin* clb_input_pin, int internal_sink_pin) const {
    return trace_max_delay(clb_input_pin->block, clb_input_pin->block_pin, internal_sink_pin);
}

inline float CachingClbDelayCalc::internal_src_to_clb_output_delay(int internal_src_pin, const t_net_pin* clb_output_pin) const {
    return trace_max_delay(clb_output_pin->block, internal_src_pin, clb_output_pin->block_pin);
}

inline float CachingClbDelayCalc::internal_src_to_internal_sink_delay(int clb, int internal_src_pin, int internal_sink_pin) const {
    return trace_max_delay(clb, internal_src_pin, internal_sink_pin);
}

inline void CachingClbDelayCalc::invalidate_delay(int clb, int src_pb_route_pin, int sink_pb_route_pin) {
    auto key = std::make_tuple(clb, src_pb_route_pin, sink_pb_route_pin);
    delay_cache_.erase(key);
}

inline void CachingClbDelayCalc::invalidate_all_delays() {
    delay_cache_.clear();
}

inline float CachingClbDelayCalc::trace_max_delay(int clb, int src_pb_route_pin, int sink_pb_route_pin) const {
    auto key = std::make_tuple(clb, src_pb_route_pin, sink_pb_route_pin);
    auto iter = delay_cache_.find(key);

    if(iter != delay_cache_.end()) {
        //Hit
        return iter->second;
    }

    //Miss

    //Calculate it
    float delay = raw_delay_calc_.trace_max_delay(clb, src_pb_route_pin, sink_pb_route_pin);

    //Save in cache
    delay_cache_.insert(std::make_pair(key, delay));

    return delay;
}
