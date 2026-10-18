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

class t_pb_graph_pin;
class t_pb_graph_node;
struct t_logical_block_type;

/// @brief Types of intra-logic block routing resource nodes.
enum e_lb_rr_type {
    LB_SOURCE = 0,
    LB_SINK,
    LB_INTERMEDIATE,
    NUM_LB_RR_TYPES
};

/// @brief Names of each e_lb_rr_type, plus "INVALID" for NUM_LB_RR_TYPES.
inline constexpr std::array<const char*, NUM_LB_RR_TYPES + 1> lb_rr_type_str{
    "LB_SOURCE",
    "LB_SINK",
    "LB_INTERMEDIATE",
    "INVALID",
};

/**
 * @brief Routing resource graph of the interconnect within one logical block type.
 *
 * Node i < total_pb_pins is the pb graph pin whose pin_count_in_cluster is i.
 * Node total_pb_pins is the external source that drives every top level input
 * and clock pin. Node total_pb_pins + 1 is the external sink driven by every
 * top level output pin. Node total_pb_pins + 2 models the external interconnect
 * that feeds top level output pins back into the block. Every node after that
 * is a sink driven by the input or clock pins of one primitive.
 *
 * Every node has at least one mode. The out edges of a node are stored
 * contiguously, grouped by mode in ascending order, and within one mode in the
 * order of the pb graph pin's output edges.
 */
class LbTypeRrGraph {
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
        int32_t node_index;

        /// @brief Cost of using this edge.
        float cost;
    };
    static_assert(sizeof(t_edge) == 8, "t_edge is read in the router search and must stay small");

    /// @brief Builds an empty graph with no nodes.
    LbTypeRrGraph() = default;

    /// @brief Builds the graph of lb_type from its pb graph.
    explicit LbTypeRrGraph(const t_logical_block_type& lb_type);

    size_t num_nodes() const { return nodes_.size(); }

    int capacity(int node) const { return nodes_[node].capacity; }

    int num_modes(int node) const { return nodes_[node].num_modes; }

    /// @brief Out edges of node when it is in the given mode.
    std::span<const t_edge> edges(int node, int mode) const {
        VTR_ASSERT_SAFE(mode >= 0 && mode < nodes_[node].num_modes);
        int32_t first = mode_edge_start_[nodes_[node].mode_start + mode];
        int32_t last = mode_edge_start_[nodes_[node].mode_start + mode + 1];
        return std::span<const t_edge>(edges_.data() + first, last - first);
    }

    int num_fanout(int node, int mode) const { return (int)edges(node, mode).size(); }

    e_lb_rr_type type(int node) const { return types_[node]; }

    /// @brief The pb graph pin of node, or nullptr for the external nodes and sinks.
    const t_pb_graph_pin* pb_graph_pin(int node) const { return pb_graph_pins_[node]; }

    /// @brief The lowest mode in which src has an edge to dst, or -1 if there is none.
    int edge_mode(int src, int dst) const;

    int ext_source_index() const { return ext_source_index_; }

    int ext_sink_index() const { return ext_sink_index_; }

  private:
    /// @brief Sets the data of node and opens its edge ranges. The caller then
    ///        calls begin_mode_() num_modes times and end_node_() once.
    void begin_node_(int node, e_lb_rr_type type, int capacity, int num_modes, const t_pb_graph_pin* pin);

    /// @brief Starts the edge range of the next mode of the node being built.
    void begin_mode_();

    /// @brief Appends an edge to the mode being built.
    void add_edge_(int node_index, float cost);

    /// @brief Closes the edge ranges of node.
    void end_node_(int node);

    /// @brief Appends a sink node with no edges and returns its index.
    int add_sink_(int capacity);

    /// @brief Appends an edge of the given cost to every top level input pin and then to every top level clock pin.
    void add_edges_to_top_level_input_pins_(const t_pb_graph_node* pb_graph_head, float cost);

    /// @brief Builds the node of a pb graph pin whose edges are its pb graph output edges.
    void build_pb_pin_node_(const t_pb_graph_pin* pb_pin, e_lb_rr_type type, int num_modes);

    /// @brief Builds the nodes of the input or clock pins of a primitive and their sinks.
    void build_primitive_sink_pins_(t_pb_graph_pin* const* pins, int num_ports, const int* num_pins);

    /// @brief Builds the nodes of every pin of pb_graph_node and its children.
    void build_pb_graph_node_(const t_pb_graph_node* pb_graph_node, int ext_rr_index);

    // Hot. Read by the router search.

    /// @brief Per node data, indexed by node.
    std::vector<t_node> nodes_;

    /// @brief Index into edges_ of the first edge of each (node, mode), plus
    ///        one entry per node that ends the range of its last mode.
    std::vector<int32_t> mode_edge_start_;

    /// @brief All edges, grouped by node and then by mode.
    std::vector<t_edge> edges_;

    // Cold. Read by setup and diagnostics.

    /// @brief Type of each node.
    std::vector<e_lb_rr_type> types_;

    /// @brief The pb graph pin of each node, or nullptr if it has none.
    std::vector<const t_pb_graph_pin*> pb_graph_pins_;

    /// @brief Node that drives every top level input and clock pin.
    int ext_source_index_ = -1;

    /// @brief Node driven by every top level output pin.
    int ext_sink_index_ = -1;
};

/// @brief Builds the graph of every logical block type, indexed by type index.
std::vector<LbTypeRrGraph> alloc_and_load_all_lb_type_rr_graph();

/// @brief Writes every graph to filename.
void echo_lb_type_rr_graphs(char* filename, const std::vector<LbTypeRrGraph>& lb_type_rr_graphs);
