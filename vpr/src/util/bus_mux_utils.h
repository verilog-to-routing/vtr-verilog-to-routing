#pragma once
/**
 * @file
 * @brief Helpers for bus-based muxes (<mux bus="true">).
 */
#include <unordered_map>
#include <vector>

#include "physical_types.h"

/// @brief True if edge implements one bit of a bus-based mux.
bool is_bus_mux_edge(const t_pb_graph_edge* edge);

/// @brief True if pin is an output bit of a bus-based mux, i.e. one of its
///        incoming edges is a bus-mux edge.
bool is_bus_mux_output_pin(const t_pb_graph_pin* pin);

/**
 * @brief The pb_graph_node instance that owns the mux of a bus-mux edge, i.e.
 *        the node whose mode contains the mux's <interconnect>.
 */
const t_pb_graph_node* get_bus_mux_owner(const t_pb_graph_edge* edge);

/// @brief True if pb_type or any of its descendants contains a bus-based mux.
bool pb_type_has_bus_mux(const t_pb_type* pb_type);

/// @brief Identity of one bus-based mux instance inside a logic block type.
struct t_bus_mux_key {
    const t_interconnect* interconnect = nullptr;
    const t_pb_graph_node* owner = nullptr;

    bool operator==(const t_bus_mux_key& other) const {
        return interconnect == other.interconnect && owner == other.owner;
    }
};

struct t_bus_mux_key_hash {
    size_t operator()(const t_bus_mux_key& key) const noexcept {
        size_t h1 = std::hash<const void*>()(key.interconnect);
        size_t h2 = std::hash<const void*>()(key.owner);
        return h1 ^ (h2 << 1);
    }
};
