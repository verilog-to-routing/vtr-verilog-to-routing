#include "rr_graph_view.h"
#include "rr_node.h"
#include "physical_types.h"

RRGraphView::RRGraphView(const t_rr_graph_storage& node_storage, const RRSpatialLookup& node_lookup, const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data, const std::vector<t_segment_inf>& rr_segments)
    : node_storage_(node_storage)
    , node_lookup_(node_lookup)
    , rr_indexed_data_(rr_indexed_data)
    , rr_segments_(rr_segments) {
}
