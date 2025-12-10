#include "crr_edge_builder.h"
#include "globals.h"

#include "physical_types.h"
#include "crr_connection_builder.h"

static t_arch_switch_inf create_crr_switch(const int delay_ps) {
    std::string switch_name;
    if (delay_ps == 0) {
        switch_name = "sw_zero";
    } else {
        switch_name = "sw_" + std::to_string(delay_ps);
    }
    t_arch_switch_inf arch_switch_inf;
    arch_switch_inf.set_type(e_switch_type::MUX);
    arch_switch_inf.name = switch_name;
    arch_switch_inf.R = 0.;
    arch_switch_inf.Cin = 0.;
    arch_switch_inf.Cout = 0;
    arch_switch_inf.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, delay_ps);
    arch_switch_inf.power_buffer_type = POWER_BUFFER_TYPE_NONE;
    arch_switch_inf.mux_trans_size = 0.;
    arch_switch_inf.buf_size_type = e_buffer_size::ABSOLUTE;
    arch_switch_inf.buf_size = 0.;
    arch_switch_inf.intra_tile = false;

    return arch_switch_inf;
}

static RRSwitchId find_or_create_crr_switch_id(const int delay_ps) {
    auto& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;
    std::string switch_name;
    int found_sw_id = -1;
    if (delay_ps == 0) {
        switch_name = "sw_zero";
    } else {
        switch_name = "sw_" + std::to_string(delay_ps);
    }
    for (int sw_id = 0; sw_id < (int)all_sw_inf.size(); sw_id++) {
        if (all_sw_inf[sw_id].name == switch_name) {
            found_sw_id = sw_id;
            break;
        }
    }

    if (found_sw_id == -1) {
        t_arch_switch_inf new_arch_switch_inf = create_crr_switch(delay_ps);
        found_sw_id = (int)all_sw_inf.size();
        all_sw_inf.insert(std::make_pair(found_sw_id, new_arch_switch_inf));
        VTR_LOG("Created new CRR switch: %s with ID: %d\n", switch_name.c_str(), found_sw_id);
    }
    return RRSwitchId(found_sw_id);
}

void build_crr_gsb_edges(RRGraphBuilder& rr_graph_builder,
                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                         const RRGSB& rr_gsb,
                         const crrgenerator::CRRConnectionBuilder& connection_builder) {
    size_t gsb_x = rr_gsb.get_sb_x();
    size_t gsb_y = rr_gsb.get_sb_y();

    std::vector<crrgenerator::Connection> gsb_connections = connection_builder.get_tile_connections(gsb_x, gsb_y);
    for (const auto& connection : gsb_connections) {
        RRSwitchId rr_switch_id;
        int delay_ps = connection.delay_ps();
        // If the delay is -1, it means the switch type should be determined from the switches defined in the architecture file.
        if (delay_ps == -1) {
            rr_switch_id = rr_node_driver_switches[connection.sink_node()];
        } else {
            rr_switch_id = find_or_create_crr_switch_id(delay_ps);
        }
        VTR_ASSERT(rr_switch_id != RRSwitchId::INVALID());
        rr_graph_builder.create_edge_in_cache(connection.src_node(),
                                              connection.sink_node(),
                                              rr_switch_id,
                                              false,
                                              std::make_optional(connection.sw_template_id()));
    }
}
