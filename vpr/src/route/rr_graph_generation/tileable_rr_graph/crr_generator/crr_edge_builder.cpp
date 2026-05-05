#include "crr_edge_builder.h"
#include "globals.h"

#include <unordered_map>

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
 * @return CRR switch
 */
static t_arch_switch_inf create_crr_switch(const int delay_ps) {
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

    return arch_switch_inf;
}

std::unordered_map<int, RRSwitchId> pre_create_crr_switches(const int min_delay_ps,
                                                            const int max_delay_ps,
                                                            const int verbosity) {
    std::map<int, t_arch_switch_inf>& all_sw_inf = g_vpr_ctx.mutable_device().all_sw_inf;
    std::unordered_map<int, RRSwitchId> delay_to_switch_id;

    // If the delay range is invalid, it means delay is not specified
    // in the switch pattern files, and we get the delay from the switch list
    // in the architecture file.
    if (min_delay_ps > max_delay_ps) {
        return delay_to_switch_id;
    }

    delay_to_switch_id.reserve(static_cast<size_t>(max_delay_ps - min_delay_ps + 1));
    for (int delay_ps = min_delay_ps; delay_ps <= max_delay_ps; ++delay_ps) {
        int sw_id = static_cast<int>(all_sw_inf.size());
        all_sw_inf.emplace(sw_id, create_crr_switch(delay_ps));
        delay_to_switch_id.emplace(delay_ps, RRSwitchId(sw_id));

        VTR_LOGV(verbosity > 1, "Created new CRR switch: delay=%d ps, sw_id=%d\n", delay_ps, sw_id);
    }

    VTR_LOGV(verbosity > 1,
             "Pre-created %zu CRR switches for delay range [%d, %d] ps\n",
             delay_to_switch_id.size(),
             min_delay_ps,
             max_delay_ps);

    return delay_to_switch_id;
}

void build_crr_gsb_edges(RRGraphBuilder& rr_graph_builder,
                         size_t& num_edges_to_create,
                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                         const RRGSB& rr_gsb,
                         const crrgenerator::CRRConnectionBuilder& connection_builder,
                         const std::unordered_map<int, RRSwitchId>& delay_to_switch_id,
                         const int /*verbosity*/) {
    if (g_vpr_ctx.device().grid.get_num_layers() != 1) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "CRR only supports 2D architectures (num_layers must be 1)\n");
    }

    size_t gsb_x = rr_gsb.get_sb_x();
    size_t gsb_y = rr_gsb.get_sb_y();

    std::vector<crrgenerator::Connection> gsb_connections = connection_builder.get_tile_connections(gsb_x, gsb_y);
    for (const auto& connection : gsb_connections) {
        RRSwitchId rr_switch_id;
        int delay_ps = connection.delay_ps();
        // delay_ps == -1 means the switch should be retrieved from the
        // architecture-defined driver switch for the sink node.
        if (delay_ps == -1) {
            rr_switch_id = rr_node_driver_switches[connection.sink_node()];
        } else {
            auto it = delay_to_switch_id.find(delay_ps);
            VTR_ASSERT_MSG(it != delay_to_switch_id.end(),
                           "CRR connection delay was not pre-created in delay_to_switch_id map");
            rr_switch_id = it->second;
        }
        VTR_ASSERT(rr_switch_id != RRSwitchId::INVALID());
        rr_graph_builder.create_edge_in_cache(connection.src_node(),
                                              connection.sink_node(),
                                              rr_switch_id,
                                              false);
        num_edges_to_create++;
    }
}
