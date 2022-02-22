#include "rr_graph_view.h"
#include "rr_node.h"
#include "physical_types.h"

RRGraphView::RRGraphView(const t_rr_graph_storage& node_storage,
                         const RRSpatialLookup& node_lookup,
                         const MetadataStorage<int>& rr_node_metadata,
                         const MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata,
                         const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                         const std::vector<t_rr_rc_data>& rr_rc_data,
                         const vtr::vector<RRSegmentId, t_segment_inf>& rr_segments,
                         const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf)
    : node_storage_(node_storage)
    , node_lookup_(node_lookup)
    , rr_node_metadata_(rr_node_metadata)
    , rr_edge_metadata_(rr_edge_metadata)
    , rr_indexed_data_(rr_indexed_data)
    , rr_rc_data_(rr_rc_data)
    , rr_segments_(rr_segments)
    , rr_switch_inf_(rr_switch_inf) {
}
