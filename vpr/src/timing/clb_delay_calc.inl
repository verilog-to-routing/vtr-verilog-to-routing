#include "clb_delay_calc.h"

#include "globals.h"

/*
 * ClbDelayCalc
 */

inline ClbDelayCalc::ClbDelayCalc()
    : intra_lb_pb_pin_lookup_(g_vpr_ctx.device().block_types, g_vpr_ctx.device().num_block_types) {}

inline float ClbDelayCalc::clb_input_to_internal_sink_delay(const ClusterBlockId block_id, const int pin_index, int internal_sink_pin, DelayType delay_type) const {
    int pb_ipin = find_clb_pb_pin(block_id, pin_index);
    return trace_delay(block_id, pb_ipin, internal_sink_pin, delay_type);
}

inline float ClbDelayCalc::internal_src_to_clb_output_delay(const ClusterBlockId block_id, const int pin_index, int internal_src_pin, DelayType delay_type) const {
    int pb_opin = find_clb_pb_pin(block_id, pin_index);
    return trace_delay(block_id, internal_src_pin, pb_opin, delay_type);
}

inline float ClbDelayCalc::internal_src_to_internal_sink_delay(const ClusterBlockId clb, int internal_src_pin, int internal_sink_pin, DelayType delay_type) const {
    return trace_delay(clb, internal_src_pin, internal_sink_pin, delay_type);
}

inline float ClbDelayCalc::trace_delay(ClusterBlockId clb, int src_pb_route_id, int sink_pb_route_id, DelayType delay_type) const {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    VTR_ASSERT(src_pb_route_id < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->total_pb_pins);
    VTR_ASSERT(sink_pb_route_id < cluster_ctx.clb_nlist.block_pb(clb)->pb_graph_node->total_pb_pins);

    const auto& pb_routes = cluster_ctx.clb_nlist.block_pb(clb)->pb_route;

    VTR_ASSERT_SAFE(pb_routes.count(src_pb_route_id));

    AtomNetId atom_net = pb_routes[src_pb_route_id].atom_net_id;

    VTR_ASSERT_MSG(atom_net, "Source pin must be connected to a valid net");
    VTR_ASSERT_MSG(atom_net == pb_routes[sink_pb_route_id].atom_net_id, "Source pin and sink pin must connect to the same net");

    float delay = 0.;

    //Trace back the internal routing from the sink to the source pin
    int curr_pb_route_id = sink_pb_route_id;
    while(pb_routes[curr_pb_route_id].driver_pb_pin_id >= 0) {
        VTR_ASSERT_MSG(atom_net == pb_routes[curr_pb_route_id].atom_net_id, "Internal routing must connect the same net");

        delay += pb_route_delay(clb, curr_pb_route_id, delay_type);

        curr_pb_route_id = pb_routes[curr_pb_route_id].driver_pb_pin_id;
    }

    VTR_ASSERT_MSG(curr_pb_route_id == src_pb_route_id, "Sink pin should trace back to source pin");

    return delay;
}

inline float ClbDelayCalc::pb_route_delay(ClusterBlockId clb, int pb_route_idx, DelayType delay_type) const {
    const t_pb_graph_edge* pb_edge = find_pb_graph_edge(clb, pb_route_idx);

    if(pb_edge) {
        if (delay_type == DelayType::MAX) {
            return pb_edge->delay_max;
        } else {
            VTR_ASSERT(delay_type == DelayType::MIN);
            return pb_edge->delay_min;
        }
    } else {
        return 0.;
    }
}

inline const t_pb_graph_edge* ClbDelayCalc::find_pb_graph_edge(ClusterBlockId clb, int pb_route_idx) const {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int type_index = cluster_ctx.clb_nlist.block_type(clb)->index;

    const t_pb* pb = cluster_ctx.clb_nlist.block_pb(clb);
    if (pb->pb_route.count(pb_route_idx)) {
        int upstream_pb_route_idx = pb->pb_route[pb_route_idx].driver_pb_pin_id;

        if(upstream_pb_route_idx >= 0) {

            const t_pb_graph_pin* pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, pb_route_idx);
            const t_pb_graph_pin* upstream_pb_gpin = intra_lb_pb_pin_lookup_.pb_gpin(type_index, upstream_pb_route_idx);

            return find_pb_graph_edge(upstream_pb_gpin, pb_gpin); 
        }
    }

    return nullptr;
}

inline const t_pb_graph_edge* ClbDelayCalc::find_pb_graph_edge(const t_pb_graph_pin* driver, const t_pb_graph_pin* sink) const {
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

