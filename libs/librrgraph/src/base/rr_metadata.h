#ifndef RR_METADATA_H_
#define RR_METADATA_H_

#include "physical_types.h"
#include "rr_graph_builder.h"

namespace vpr {

const t_metadata_value* rr_node_metadata(const RRGraphBuilder& rr_graph_builder, int src_node, vtr::interned_string key);
void add_rr_node_metadata(MetadataStorage<int>& rr_node_metadata, int src_node, vtr::interned_string key, vtr::interned_string value);
void add_rr_node_metadata(MetadataStorage<int>& rr_node_metadata, int src_node, vtr::string_view key, vtr::string_view value, const t_arch* arch);

const t_metadata_value* rr_edge_metadata(const RRGraphBuilder& rr_graph_builder, int src_node, int sink_node, short switch_id, vtr::interned_string key);
void add_rr_edge_metadata(MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata, int src_node, int sink_node, short switch_id, vtr::interned_string key, vtr::interned_string value);
void add_rr_edge_metadata(MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata, int src_node, int sink_node, short switch_id, vtr::string_view key, vtr::string_view value, const t_arch* arch);

} // namespace vpr

#endif // RR_METADATA_H_
