#pragma once
/**
 * @file
 * @brief Routing resource graph of the interconnect within one logical block type.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "vtr_assert.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

class t_pb_graph_pin;

/// @brief Index of a node in an LbTypeRrGraph.
typedef vtr::StrongId<struct lb_rr_node_id_tag, int> LbRrNodeId;

/// @brief Types of intra-logic block routing resource nodes.
enum class e_lb_rr_type {
    SOURCE = 0,
    SINK,
    INTERMEDIATE,
    NUM_TYPES
};

/// @brief Names of each e_lb_rr_type, plus "INVALID" for NUM_TYPES.
inline constexpr std::array<const char*, (size_t)e_lb_rr_type::NUM_TYPES + 1> lb_rr_type_str{
    "SOURCE",
    "SINK",
    "INTERMEDIATE",
    "INVALID",
};

/**
 * @brief Routing resource graph of the interconnect within one logical block type.
 *
 * Nodes are laid out as follows:
 *  - [0, total_pb_pins): the pb graph pin whose pin_count_in_cluster is the
 *    node index. Capacity 1.
 *  - total_pb_pins: the external source. It drives every top level input and
 *    clock pin. Its capacity is the number of those pins.
 *  - total_pb_pins + 1: the external sink. It is driven by every top level
 *    output pin. Its capacity is the number of those pins.
 *  - total_pb_pins + 2: the external interconnect. It is driven by every top
 *    level output pin and feeds them to the external sink or back into the
 *    block. Its capacity is the number of top level output pins.
 *  - The rest: sinks driven by primitive input and clock pins. The pins of an
 *    equivalent port share one sink whose capacity is the port size. Every
 *    other pin has its own sink with capacity 1.
 *
 * Every node has at least one mode. The out edges of a node are stored
 * contiguously, grouped by mode in ascending order, and within one mode in the
 * order of the pb graph pin's output edges.
 *
 * The graph is built by LbTypeRrGraphBuilder and read only after that.
 */
class LbTypeRrGraph {
    friend class LbTypeRrGraphBuilder;

  public:
    /// @brief Per node data read by the router search.
    struct t_node {
        /// @brief Index into mode_edge_start_ of the first mode of this node.
        int32_t mode_start;

        /// @brief Number of nets that can use this node at the same time.
        int16_t capacity;

        /// @brief Number of modes this node has edge ranges for.
        int16_t num_modes;
    };
    static_assert(sizeof(t_node) == 8, "t_node is read in the router search and must stay small");

    /// @brief An out edge of a node.
    struct t_edge {
        /// @brief Node this edge drives.
        LbRrNodeId node_index;

        /// @brief Cost of using this edge.
        float cost;
    };
    static_assert(sizeof(t_edge) == 8, "t_edge is read in the router search and must stay small");

    typedef vtr::vector<LbRrNodeId, t_node>::key_range node_range;

    /// @brief Builds an empty graph with no nodes.
    LbTypeRrGraph() = default;

    size_t num_nodes() const { return nodes_.size(); }

    /// @brief All node ids in increasing order.
    node_range nodes() const { return nodes_.keys(); }

    int capacity(LbRrNodeId node) const { return nodes_[node].capacity; }

    int num_modes(LbRrNodeId node) const { return nodes_[node].num_modes; }

    /// @brief Out edges of node when it is in the given mode.
    std::span<const t_edge> edges(LbRrNodeId node, int mode) const {
        VTR_ASSERT_SAFE(mode >= 0 && mode < nodes_[node].num_modes);
        int32_t first = mode_edge_start_[nodes_[node].mode_start + mode];
        int32_t last = mode_edge_start_[nodes_[node].mode_start + mode + 1];
        return std::span<const t_edge>(edges_.data() + first, last - first);
    }

    int num_fanout(LbRrNodeId node, int mode) const { return (int)edges(node, mode).size(); }

    e_lb_rr_type type(LbRrNodeId node) const { return types_[node]; }

    /// @brief The pb graph pin of node, or nullptr for the external nodes and sinks.
    const t_pb_graph_pin* pb_graph_pin(LbRrNodeId node) const { return pb_graph_pins_[node]; }

    /// @brief The lowest mode in which src has an edge to dst, or -1 if there is none.
    int edge_mode(LbRrNodeId src, LbRrNodeId dst) const;

    LbRrNodeId ext_source_node() const { return ext_source_node_; }

    LbRrNodeId ext_sink_node() const { return ext_sink_node_; }

  private:
    // Hot. Read by the router search.

    /// @brief Per node data.
    vtr::vector<LbRrNodeId, t_node> nodes_;

    /// @brief Index into edges_ of the first edge of each (node, mode), plus
    ///        one entry per node that ends the range of its last mode.
    std::vector<int32_t> mode_edge_start_;

    /// @brief All edges, grouped by node and then by mode.
    std::vector<t_edge> edges_;

    // Cold. Read by setup and diagnostics.

    /// @brief Type of each node.
    vtr::vector<LbRrNodeId, e_lb_rr_type> types_;

    /// @brief The pb graph pin of each node, or nullptr if it has none.
    vtr::vector<LbRrNodeId, const t_pb_graph_pin*> pb_graph_pins_;

    /// @brief Node that drives every top level input and clock pin.
    LbRrNodeId ext_source_node_;

    /// @brief Node driven by every top level output pin.
    LbRrNodeId ext_sink_node_;
};

/// @brief Writes every graph to filename.
void echo_lb_type_rr_graphs(char* filename, const std::vector<LbTypeRrGraph>& lb_type_rr_graphs);
