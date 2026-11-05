#pragma once
/**
 * @file
 * @brief Routing resource graph of the interconnect within one logical block type.
 *
 * Terms:
 *  - node: one per pb graph pin, plus the external nodes and the sinks of
 *    primitive pins.
 *  - edge: connects two nodes in a specific mode.
 *  - mode: a mode of the pb type. An edge can be used only while that pb type
 *    is in the edge's mode.
 *  - capacity: the number of nets that can use a node at once.
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

/// @brief Index of a node in an LbTypeRRGraph.
typedef vtr::StrongId<struct lb_rr_node_id_tag, int> LbRRNodeId;

/// @brief Types of intra-logic block routing resource nodes.
enum class e_lb_rr_type : uint8_t {
    /// @brief A node where a net can start: a primitive output pin or the external source.
    SOURCE = 0,
    /// @brief A node where a net ends: the sink of a primitive pin or the external sink.
    SINK,
    /// @brief A node a net passes through on the way from a source to a sink.
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
 * The graph is built by LbTypeRRGraphBuilder and read only after that.
 */
class LbTypeRRGraph {
    friend class LbTypeRRGraphBuilder;

  public:
    /// @brief Per node data read by the router search.
    struct t_node {
        /// @brief Index into mode_edge_start_ of the first mode of this node.
        uint32_t mode_start;

        /// @brief Number of nets that can use this node at the same time.
        uint16_t capacity;

        /// @brief Number of modes this node has edge ranges for.
        uint16_t num_modes;
    };
    static_assert(sizeof(t_node) == 8, "t_node is read in the router search and must stay small");

    /// @brief An out edge of a node.
    struct t_edge {
        /// @brief Node this edge drives.
        LbRRNodeId node_index;

        /// @brief Cost of using this edge.
        float cost;
    };
    static_assert(sizeof(t_edge) == 8, "t_edge is read in the router search and must stay small");

    typedef vtr::vector<LbRRNodeId, t_node>::key_range node_range;

    /// @brief Builds an empty graph with no nodes.
    LbTypeRRGraph() = default;

    size_t num_nodes() const { return nodes_.size(); }

    /// @brief All node ids in increasing order.
    node_range nodes() const { return nodes_.keys(); }

    uint16_t capacity(LbRRNodeId node) const { return nodes_[node].capacity; }

    uint16_t num_modes(LbRRNodeId node) const { return nodes_[node].num_modes; }

    /// @brief Out edges of node when it is in the given mode.
    std::span<const t_edge> edges(LbRRNodeId node, int mode) const {
        VTR_ASSERT_SAFE(mode >= 0 && mode < nodes_[node].num_modes);
        uint32_t first = mode_edge_start_[nodes_[node].mode_start + mode];
        uint32_t last = mode_edge_start_[nodes_[node].mode_start + mode + 1];
        return std::span<const t_edge>(edges_.data() + first, last - first);
    }

    size_t num_fanout(LbRRNodeId node, int mode) const { return edges(node, mode).size(); }

    e_lb_rr_type type(LbRRNodeId node) const { return types_[node]; }

    /// @brief The pb graph pin of node, or nullptr for the external nodes and sinks.
    const t_pb_graph_pin* pb_graph_pin(LbRRNodeId node) const { return pb_graph_pins_[node]; }

    /// @brief The lowest mode in which src has an edge to dst, or -1 if there is none.
    int edge_mode(LbRRNodeId src, LbRRNodeId dst) const;

    LbRRNodeId ext_source_node() const { return ext_source_node_; }

    LbRRNodeId ext_sink_node() const { return ext_sink_node_; }

    LbRRNodeId ext_interconnect_node() const { return ext_interconnect_node_; }

  private:
    // ========================================================================
    //      Hot data. Read for every node expanded in the cluster router.
    // ========================================================================

    /// @brief Per node data.
    vtr::vector<LbRRNodeId, t_node> nodes_;

    /// @brief Index into edges_ of the first edge of each (node, mode), plus
    ///        one entry per node that ends the range of its last mode.
    std::vector<uint32_t> mode_edge_start_;

    /// @brief All edges, grouped by node and then by mode.
    std::vector<t_edge> edges_;

    // ========================================================================
    //      Cold data. Read by router setup and diagnostics
    // ========================================================================

    /// @brief Type of each node.
    vtr::vector<LbRRNodeId, e_lb_rr_type> types_;

    /// @brief The pb graph pin of each node, or nullptr if it has none.
    vtr::vector<LbRRNodeId, const t_pb_graph_pin*> pb_graph_pins_;

    /// @brief Node that drives every top level input and clock pin.
    LbRRNodeId ext_source_node_;

    /// @brief Node driven by every top level output pin.
    LbRRNodeId ext_sink_node_;

    /// @brief Node that models the interconnect outside the block. It is driven
    ///        by every top level output pin and drives the external sink and
    ///        every top level input and clock pin.
    LbRRNodeId ext_interconnect_node_;
};

/// @brief Writes every graph to filename.
void echo_lb_type_rr_graphs(char* filename, const std::vector<LbTypeRRGraph>& lb_type_rr_graphs);
