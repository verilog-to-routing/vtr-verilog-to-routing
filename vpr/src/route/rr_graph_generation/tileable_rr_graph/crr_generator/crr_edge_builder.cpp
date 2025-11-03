#include "crr_edge_builder.h"
#include "globals.h"

#include "crr_connection_builder.h"

static RRSwitchId find_crr_switch_id(const int delay_ps) {
    const auto& arch_switches = g_vpr_ctx.device().arch_switch_inf;
    std::string switch_name;
    if (delay_ps == 0) {
        switch_name = "sw_zero";
    } else {
        switch_name = "sw_" + std::to_string(delay_ps);
    }
    for (int sw_id = 0; sw_id < (int)arch_switches.size(); sw_id++) {
        if (arch_switches[sw_id].name == switch_name) {
            return RRSwitchId(sw_id);
        }
    }
    return RRSwitchId::INVALID();
}

void build_crr_gsb_track_to_track_edges(RRGraphBuilder& rr_graph_builder,
                                        const RRGSB& rr_gsb,
                                        const crrgenerator::CRRConnectionBuilder& connection_builder) {
    size_t gsb_x = rr_gsb.get_sb_x();
    size_t gsb_y = rr_gsb.get_sb_y();

    std::vector<crrgenerator::Connection> gsb_connections = connection_builder.get_tile_connections(gsb_x, gsb_y);
    for (const auto& connection : gsb_connections) {
        RRSwitchId rr_switch_id = find_crr_switch_id(connection.delay_ps());
        VTR_ASSERT(rr_switch_id != RRSwitchId::INVALID());
        rr_graph_builder.create_edge_in_cache(connection.src_node(), connection.sink_node(), rr_switch_id, false);
    }
}
