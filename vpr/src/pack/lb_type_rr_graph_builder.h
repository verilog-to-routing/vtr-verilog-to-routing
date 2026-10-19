#pragma once
/**
 * @file
 * @brief Builds the routing resource graph of a logical block type from its pb graph.
 */

#include <vector>

#include "lb_type_rr_graph.h"

class t_pb_graph_node;
struct t_logical_block_type;

/**
 * @brief Builds the LbTypeRrGraph of one logical block type from its pb graph.
 *
 * Nodes are built one at a time. begin_node_() opens a node, begin_mode_()
 * opens each of its mode ranges in ascending order, add_edge_() appends to the
 * open mode and end_node_() closes the node. Sinks of primitive pins are
 * appended between nodes as they are found.
 */
class LbTypeRrGraphBuilder {
  public:
    /// @brief Builds and returns the graph of lb_type.
    static LbTypeRrGraph build(const t_logical_block_type& lb_type);

  private:
    LbTypeRrGraphBuilder() = default;

    /// @brief Builds the external nodes and every pb graph pin of lb_type into graph_.
    void build_graph_(const t_logical_block_type& lb_type);

    /// @brief Sets the data of node and opens its edge ranges. The caller then
    ///        calls begin_mode_() num_modes times and end_node_() once.
    void begin_node_(LbRrNodeId node, e_lb_rr_type type, int capacity, int num_modes, const t_pb_graph_pin* pin);

    /// @brief Starts the edge range of the next mode of the open node.
    void begin_mode_();

    /// @brief Appends an edge to the open mode.
    void add_edge_(LbRrNodeId node_index, float cost);

    /// @brief Closes the edge ranges of the open node.
    void end_node_();

    /// @brief Appends a sink node with no edges and returns its id.
    LbRrNodeId add_sink_(int capacity);

    /// @brief Appends an edge of the given cost to every top level input pin and then to every top level clock pin.
    void add_edges_to_top_level_input_pins_(const t_pb_graph_node* pb_graph_head, float cost);

    /// @brief Builds the node of a pb graph pin whose edges are its pb graph output edges.
    void build_pb_pin_node_(const t_pb_graph_pin* pb_pin, e_lb_rr_type type, int num_modes);

    /// @brief Builds the nodes of the input or clock pins of a primitive and their sinks.
    void build_primitive_sink_pins_(t_pb_graph_pin* const* pins, int num_ports, const int* num_pins);

    /// @brief Builds the nodes of every pin of pb_graph_node and its children.
    void build_pb_graph_node_(const t_pb_graph_node* pb_graph_node);

    /// @brief The graph being built.
    LbTypeRrGraph graph_;

    /// @brief Node whose edge ranges are open, or invalid between nodes.
    LbRrNodeId open_node_;

    /// @brief Number of mode ranges opened so far for open_node_.
    int num_open_modes_ = 0;
};

/// @brief Builds the graph of every logical block type, indexed by type index.
std::vector<LbTypeRrGraph> alloc_and_load_all_lb_type_rr_graph();
