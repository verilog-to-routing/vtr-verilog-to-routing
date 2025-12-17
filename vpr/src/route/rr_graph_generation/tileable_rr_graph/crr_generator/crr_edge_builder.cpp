#include "crr_edge_builder.h"
#include "globals.h"

#include "physical_types.h"
#include "crr_connection_builder.h"

/**
 * @brief Get the name of a CRR switch
 * @param delay_ps Delay in picoseconds
 * @return Name of the switch
 */
static std::string get_crr_switch_name(const int delay_ps) {
    if (delay_ps == 0) {
        return "sw_zero";
    } else {
        return "sw_" + std::to_string(delay_ps);
    }
}

/**
 * @brief Create an architecture switch for CRR
 * @param delay_ps Delay in picoseconds
 * @param sw_template_id Template ID of the switch
 * @return CRR switch
 */
static t_arch_switch_inf create_crr_switch(const int delay_ps, const std::string& sw_template_id) {
    std::string switch_name = get_crr_switch_name(delay_ps);

    t_arch_switch_inf arch_switch_inf;
    arch_switch_inf.set_type(e_switch_type::MUX);
    arch_switch_inf.name = switch_name;
    arch_switch_inf.R = 0.;
    arch_switch_inf.Cin = 0.;
    arch_switch_inf.Cout = 0;
    arch_switch_inf.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, static_cast<float>(delay_ps) * 1e-12);
    arch_switch_inf.power_buffer_type = POWER_BUFFER_TYPE_NONE;
    arch_switch_inf.mux_trans_size = 0.;
    arch_switch_inf.buf_size_type = e_buffer_size::ABSOLUTE;
    arch_switch_inf.buf_size = 0.;
    arch_switch_inf.intra_tile = false;
    arch_switch_inf.template_id = sw_template_id;

    return arch_switch_inf;
}

/**
 * @brief Find or create a CRR switch ID
 * @param delay_ps Delay in picoseconds
 * @param sw_template_id Template ID of the switch
 * @return CRR switch ID
 */
static RRSwitchId find_or_create_crr_switch_id(const int delay_ps,
                                               const std::string& sw_template_id,
                                               const int verbosity) {

    std::map<int, t_arch_switch_inf>& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;

    int found_sw_id = -1;

    // Iterate over map entries (O(n), no accidental inserts)
    for (const auto& [sw_id, sw_inf] : all_sw_inf) {
        if (sw_inf.template_id == sw_template_id) {
            found_sw_id = sw_id;
            break;
        }
    }

    if (found_sw_id == -1) {
        t_arch_switch_inf new_arch_switch_inf = create_crr_switch(delay_ps, sw_template_id);

        found_sw_id = static_cast<int>(all_sw_inf.size());
        all_sw_inf.emplace(found_sw_id, std::move(new_arch_switch_inf));

        VTR_LOGV(verbosity > 1, "Created new CRR switch: delay=%d ps, template id=%s\n", delay_ps, sw_template_id.c_str());
    }

    return RRSwitchId(found_sw_id);
}

void build_crr_gsb_edges(RRGraphBuilder& rr_graph_builder,
                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                         const RRGSB& rr_gsb,
                         const crrgenerator::CRRConnectionBuilder& connection_builder,
                         const int verbosity) {
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
            rr_switch_id = find_or_create_crr_switch_id(delay_ps,
                                                        connection.sw_template_id(),
                                                        verbosity);
        }
        VTR_ASSERT(rr_switch_id != RRSwitchId::INVALID());
        rr_graph_builder.create_edge_in_cache(connection.src_node(),
                                              connection.sink_node(),
                                              rr_switch_id,
                                              false);
    }
}
