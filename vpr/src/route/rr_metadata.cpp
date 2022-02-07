#include "rr_metadata.h"

#include "globals.h"

namespace vpr {

const t_metadata_value* rr_node_metadata(int src_node, vtr::interned_string key) {
    auto& device_ctx = g_vpr_ctx.device();

    auto iter = device_ctx.rr_graph_builder.find_rr_node_metadata(src_node);
    if (iter == device_ctx.rr_graph_builder.end_rr_node_metadata()) {
        return nullptr;
    }
    return iter->second.one(key);
}

void add_rr_node_metadata(int src_node, vtr::interned_string key, vtr::interned_string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.rr_graph_builder.rr_node_metadata().add_metadata(src_node,
                                                                key,
                                                                value);
}

void add_rr_node_metadata(int src_node, vtr::string_view key, vtr::string_view value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.rr_graph_builder.rr_node_metadata().add_metadata(src_node,
                                                                device_ctx.arch->strings.intern_string(key),
                                                                device_ctx.arch->strings.intern_string(value));
}

const t_metadata_value* rr_edge_metadata(int src_node, int sink_id, short switch_id, vtr::interned_string key) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);

    auto iter = device_ctx.rr_graph_builder.find_rr_edge_metadata(rr_edge);
    if (iter == device_ctx.rr_graph_builder.end_rr_edge_metadata()) {
        return nullptr;
    }

    return iter->second.one(key);
}

void add_rr_edge_metadata(int src_node, int sink_id, short switch_id, vtr::string_view key, vtr::string_view value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);
    device_ctx.rr_graph_builder.rr_edge_metadata().add_metadata(rr_edge,
                                                                device_ctx.arch->strings.intern_string(key),
                                                                device_ctx.arch->strings.intern_string(value));
}

void add_rr_edge_metadata(int src_node, int sink_id, short switch_id, vtr::interned_string key, vtr::interned_string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);
    device_ctx.rr_graph_builder.rr_edge_metadata().add_metadata(rr_edge,
                                                                key,
                                                                value);
}

} // namespace vpr
