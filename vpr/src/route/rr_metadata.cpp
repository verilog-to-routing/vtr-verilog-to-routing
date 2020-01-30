#include "rr_metadata.h"

#include "globals.h"

namespace vpr {

const t_metadata_value* rr_node_metadata(int src_node, std::string key) {
    auto& device_ctx = g_vpr_ctx.device();

    if (device_ctx.rr_node_metadata.size() == 0 || device_ctx.rr_node_metadata.count(src_node) == 0) {
        return nullptr;
    }
    auto& data = device_ctx.rr_node_metadata.at(src_node);
    return data.one(key);
}

void add_rr_node_metadata(int src_node, std::string key, std::string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (device_ctx.rr_node_metadata.count(src_node) == 0) {
        device_ctx.rr_node_metadata.emplace(src_node, t_metadata_dict());
    }
    auto& data = device_ctx.rr_node_metadata.at(src_node);
    data.add(key, value);
}

const t_metadata_value* rr_edge_metadata(int src_node, int sink_id, short switch_id, std::string key) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);

    auto iter = device_ctx.rr_edge_metadata.find(rr_edge);
    if (iter == device_ctx.rr_edge_metadata.end()) {
        return nullptr;
    }

    return iter->second.one(key);
}

void add_rr_edge_metadata(int src_node, int sink_id, short switch_id, std::string key, std::string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto rr_edge = std::make_tuple(src_node, sink_id, switch_id);
    if (device_ctx.rr_edge_metadata.count(rr_edge) == 0) {
        device_ctx.rr_edge_metadata.emplace(rr_edge, t_metadata_dict());
    }
    auto& data = device_ctx.rr_edge_metadata.at(rr_edge);
    data.add(key, value);
}

} // namespace vpr
