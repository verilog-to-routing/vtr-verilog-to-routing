#include "crr_edge_builder.h"

#include "crr_connection_builder.h"

void build_gsb_track_to_track_map(RRGraphBuilder& rr_graph_builder,
                                  const RRGSB& rr_gsb,
                                  const crrgenerator::CRRConnectionBuilder& connection_builder) {
    size_t gsb_x = rr_gsb.get_sb_x();
    size_t gsb_y = rr_gsb.get_sb_y();

    std::vector<crrgenerator::Connection> gsb_connections = connection_builder.get_tile_connections(gsb_x, gsb_y);
    for (const auto& connection : gsb_connections) {
        RRSwitchId rr_switch_id = RRSwitchId(connection.switch_id());
        rr_graph_builder.create_edge_in_cache(connection.src_node(), connection.sink_node(), rr_switch_id, false);
    }
}
