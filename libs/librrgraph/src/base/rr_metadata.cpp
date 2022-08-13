#include "rr_metadata.h"

namespace vpr {

const t_metadata_value* rr_node_metadata(const RRGraphBuilder& rr_graph_builder, int src_node, vtr::interned_string key) {
    auto iter = rr_graph_builder.find_rr_node_metadata(src_node);
    if (iter == rr_graph_builder.end_rr_node_metadata()) {
        return nullptr;
    }
    return iter->second.one(key);
}

void add_rr_node_metadata(MetadataStorage<int>& rr_node_metadata, int src_node, vtr::interned_string key, vtr::interned_string value) {
    rr_node_metadata.add_metadata(src_node,
                                  key,
                                  value);
}

void add_rr_node_metadata(MetadataStorage<int>& rr_node_metadata, int src_node, vtr::string_view key, vtr::string_view value, const t_arch* arch) {
    rr_node_metadata.add_metadata(src_node,
                                  arch->strings.intern_string(key),
                                  arch->strings.intern_string(value));
}

const t_metadata_value* rr_edge_metadata(const RRGraphBuilder& rr_graph_builder, int src_node, int sink_id, short switch_id, vtr::interned_string key) {
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);

    auto iter = rr_graph_builder.find_rr_edge_metadata(rr_edge);
    if (iter == rr_graph_builder.end_rr_edge_metadata()) {
        return nullptr;
    }

    return iter->second.one(key);
}

void add_rr_edge_metadata(MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata, int src_node, int sink_id, short switch_id, vtr::string_view key, vtr::string_view value, const t_arch* arch) {
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);
    rr_edge_metadata.add_metadata(rr_edge,
                                  arch->strings.intern_string(key),
                                  arch->strings.intern_string(value));
}

void add_rr_edge_metadata(MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata, int src_node, int sink_id, short switch_id, vtr::interned_string key, vtr::interned_string value) {
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);
    rr_edge_metadata.add_metadata(rr_edge,
                                  key,
                                  value);
}

} // namespace vpr
