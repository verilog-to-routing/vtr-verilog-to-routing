/*
 * Builds the routing resource graph that represents the interconnect within a logic block type.
 *
 * Each pb_graph_pin is a node in this graph and is indexed by the pin number of the pb_graph pin.
 * External nets that drive into the logic block come from one source indexed by the total number of pins.
 * External nets that the logic block drives end at one sink indexed by the total number of pins + 1.
 * Each primitive input pin drives a sink. Since the input pin has its own node, the sink it drives
 * needs a different index, so sinks are appended after the external nodes.
 * Each primitive output pin is a source.
 *
 * Common acronyms:
 * rr - routing resource
 * lb - logic block
 * pb - physical block (the top level physical block is the logic block, a leaf physical block is a primitive)
 */

#include <cstdio>
#include <limits>
#include <vector>

#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "lb_type_rr_graph.h"

/// @brief Cost of an edge that joins two pb graph pins.
static float get_cost_of_pb_edge(const t_pb_graph_edge* edge);

/// @brief Prints one graph to fp.
static void print_lb_type_rr_graph(FILE* fp, const LbTypeRrGraph& lb_type_rr_graph);

/*****************************************************************************************
 * Constructor functions
 ******************************************************************************************/

std::vector<LbTypeRrGraph> alloc_and_load_all_lb_type_rr_graph() {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    std::vector<LbTypeRrGraph> lb_type_rr_graphs(device_ctx.logical_block_types.size());
    for (const t_logical_block_type& type : device_ctx.logical_block_types) {
        if (&type != device_ctx.EMPTY_LOGICAL_BLOCK_TYPE) {
            lb_type_rr_graphs[type.index] = LbTypeRrGraph(type);
        }
    }
    return lb_type_rr_graphs;
}

LbTypeRrGraph::LbTypeRrGraph(const t_logical_block_type& lb_type) {
    const t_pb_type* pb_type = lb_type.pb_type;
    const t_pb_graph_node* pb_graph_head = lb_type.pb_graph_head;
    int total_pb_pins = pb_graph_head->total_pb_pins;

    // Nodes [0, total_pb_pins) are the pb graph pins and the next three are the
    // external nodes. The sinks of primitive pins are appended as they are found.
    nodes_.resize(total_pb_pins + 3);
    types_.resize(total_pb_pins + 3, NUM_LB_RR_TYPES);
    pb_graph_pins_.resize(total_pb_pins + 3, nullptr);
    ext_source_node_ = LbRrNodeId(total_pb_pins);
    ext_sink_node_ = LbRrNodeId(total_pb_pins + 1);
    LbRrNodeId ext_rr_node = LbRrNodeId(total_pb_pins + 2);

    // The external source drives every top level input and clock pin.
    int num_top_level_input_pins = pb_type->num_input_pins + pb_type->num_clock_pins;
    begin_node_(ext_source_node_, LB_SOURCE, num_top_level_input_pins, 1, nullptr);
    begin_mode_();
    add_edges_to_top_level_input_pins_(pb_graph_head, 1);
    end_node_(ext_source_node_);
    VTR_ASSERT(num_fanout(ext_source_node_, 0) == num_top_level_input_pins);

    // The external sink is driven by every top level output pin and has no edges.
    begin_node_(ext_sink_node_, LB_SINK, pb_type->num_output_pins, 1, nullptr);
    begin_mode_();
    end_node_(ext_sink_node_);

    // The external interconnect node is driven by every top level output pin.
    // It leads to the external sink and, at a high cost, back to every top level
    // input and clock pin, so feedback through the external interconnect is
    // used only when nothing else routes.
    begin_node_(ext_rr_node, LB_INTERMEDIATE, pb_type->num_output_pins, 1, nullptr);
    begin_mode_();
    add_edge_(ext_sink_node_, 1);
    add_edges_to_top_level_input_pins_(pb_graph_head, 1000);
    end_node_(ext_rr_node);

    build_pb_graph_node_(pb_graph_head, ext_rr_node);

    // Every pb graph pin must have been given a node.
    for (LbRrNodeId inode : nodes_.keys()) {
        VTR_ASSERT(types_[inode] != NUM_LB_RR_TYPES);
    }

    nodes_.shrink_to_fit();
    mode_edge_start_.shrink_to_fit();
    edges_.shrink_to_fit();
    types_.shrink_to_fit();
    pb_graph_pins_.shrink_to_fit();
}

/*****************************************************************************************
 * Accessor functions
 ******************************************************************************************/

int LbTypeRrGraph::edge_mode(LbRrNodeId src, LbRrNodeId dst) const {
    for (int imode = 0; imode < num_modes(src); imode++) {
        for (const t_edge& edge : edges(src, imode)) {
            if (edge.node_index == dst) {
                return imode;
            }
        }
    }
    return -1;
}

/*****************************************************************************************
 * Debug functions
 ******************************************************************************************/

void echo_lb_type_rr_graphs(char* filename, const std::vector<LbTypeRrGraph>& lb_type_rr_graphs) {
    FILE* fp = vtr::fopen(filename, "w");

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    for (const t_logical_block_type& type : device_ctx.logical_block_types) {
        if (!is_empty_type(&type)) {
            fprintf(fp, "--------------------------------------------------------------\n");
            fprintf(fp, "Intra-Logic Block Routing Resource For Type %s\n", type.name.c_str());
            fprintf(fp, "--------------------------------------------------------------\n");
            fprintf(fp, "\n");
            print_lb_type_rr_graph(fp, lb_type_rr_graphs[type.index]);
        }
    }

    fclose(fp);
}

/******************************************************************************************
 * Internal functions
 ******************************************************************************************/

void LbTypeRrGraph::begin_node_(LbRrNodeId node, e_lb_rr_type type, int capacity, int num_modes, const t_pb_graph_pin* pin) {
    VTR_ASSERT_MSG(types_[node] == NUM_LB_RR_TYPES, "Node built twice");
    VTR_ASSERT(num_modes >= 1 && num_modes <= std::numeric_limits<int16_t>::max());
    VTR_ASSERT(capacity >= 0 && capacity <= std::numeric_limits<int16_t>::max());

    nodes_[node].mode_start = mode_edge_start_.size();
    nodes_[node].capacity = capacity;
    nodes_[node].num_modes = num_modes;
    types_[node] = type;
    pb_graph_pins_[node] = pin;
}

void LbTypeRrGraph::begin_mode_() {
    mode_edge_start_.push_back(edges_.size());
}

void LbTypeRrGraph::add_edge_(LbRrNodeId node_index, float cost) {
    edges_.push_back({node_index, cost});
}

void LbTypeRrGraph::end_node_(LbRrNodeId node) {
    mode_edge_start_.push_back(edges_.size());
    VTR_ASSERT_MSG((int)mode_edge_start_.size() == nodes_[node].mode_start + nodes_[node].num_modes + 1,
                   "Number of modes built does not match the number declared");
}

LbRrNodeId LbTypeRrGraph::add_sink_(int capacity) {
    LbRrNodeId sink_index = LbRrNodeId(nodes_.size());
    nodes_.emplace_back();
    types_.push_back(NUM_LB_RR_TYPES);
    pb_graph_pins_.push_back(nullptr);

    // A sink has one mode with no edges so that it can be expanded like any other node.
    begin_node_(sink_index, LB_SINK, capacity, 1, nullptr);
    begin_mode_();
    end_node_(sink_index);
    return sink_index;
}

void LbTypeRrGraph::add_edges_to_top_level_input_pins_(const t_pb_graph_node* pb_graph_head, float cost) {
    for (int iport = 0; iport < pb_graph_head->num_input_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_head->num_input_pins[iport]; ipin++) {
            add_edge_(LbRrNodeId(pb_graph_head->input_pins[iport][ipin].pin_count_in_cluster), cost);
        }
    }

    for (int iport = 0; iport < pb_graph_head->num_clock_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_head->num_clock_pins[iport]; ipin++) {
            add_edge_(LbRrNodeId(pb_graph_head->clock_pins[iport][ipin].pin_count_in_cluster), cost);
        }
    }
}

void LbTypeRrGraph::build_pb_pin_node_(const t_pb_graph_pin* pb_pin, e_lb_rr_type type, int num_modes) {
    LbRrNodeId pin_index = LbRrNodeId(pb_pin->pin_count_in_cluster);
    begin_node_(pin_index, type, 1, num_modes, pb_pin);

    // Each mode holds the edges whose interconnect belongs to that mode, in pb graph order.
    for (int imode = 0; imode < num_modes; imode++) {
        begin_mode_();
        for (int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
            const t_pb_graph_edge* edge = pb_pin->output_edges[iedge];
            VTR_ASSERT(edge->num_output_pins == 1);
            int edge_mode = edge->interconnect->parent_mode->index;
            VTR_ASSERT(edge_mode >= 0 && edge_mode < num_modes);
            if (edge_mode != imode) {
                continue;
            }
            add_edge_(LbRrNodeId(edge->output_pins[0]->pin_count_in_cluster), get_cost_of_pb_edge(edge));
        }
    }

    end_node_(pin_index);
}

void LbTypeRrGraph::build_primitive_sink_pins_(t_pb_graph_pin* const* pins, int num_ports, const int* num_pins) {
    for (int iport = 0; iport < num_ports; iport++) {
        // The pins of an equivalent port share one sink whose capacity is the size of the port.
        LbRrNodeId sink_index = LbRrNodeId::INVALID();
        for (int ipin = 0; ipin < num_pins[iport]; ipin++) {
            const t_pb_graph_pin* pb_pin = &pins[iport][ipin];
            bool port_equivalent = pb_pin->port->equivalent != PortEquivalence::NONE;

            // The sink is appended before the pin node so that the pin's edge range stays contiguous.
            if (!port_equivalent || !sink_index.is_valid()) {
                sink_index = add_sink_(port_equivalent ? pb_pin->port->num_pins : 1);
            }

            LbRrNodeId pin_index = LbRrNodeId(pb_pin->pin_count_in_cluster);
            begin_node_(pin_index, LB_INTERMEDIATE, 1, 1, pb_pin);
            begin_mode_();
            add_edge_(sink_index, 1);
            end_node_(pin_index);
        }
    }
}

void LbTypeRrGraph::build_pb_graph_node_(const t_pb_graph_node* pb_graph_node, LbRrNodeId ext_rr_node) {
    const t_pb_type* pb_type = pb_graph_node->pb_type;
    const t_pb_graph_node* parent_node = pb_graph_node->parent_pb_graph_node;

    if (pb_type->is_primitive()) {
        // Input and clock pins of a primitive each drive a sink. Output pins are
        // sources whose edges depend on the mode of the parent.
        build_primitive_sink_pins_(pb_graph_node->input_pins, pb_graph_node->num_input_ports, pb_graph_node->num_input_pins);

        for (int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
                build_pb_pin_node_(&pb_graph_node->output_pins[iport][ipin], LB_SOURCE, parent_node->pb_type->num_modes);
            }
        }

        build_primitive_sink_pins_(pb_graph_node->clock_pins, pb_graph_node->num_clock_ports, pb_graph_node->num_clock_pins);
        return;
    }

    // Children are built before the pins of this node, which numbers the sinks in post-order.
    for (int imode = 0; imode < pb_type->num_modes; imode++) {
        for (int ipb_type = 0; ipb_type < pb_type->modes[imode].num_pb_type_children; ipb_type++) {
            for (int ipb = 0; ipb < pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
                build_pb_graph_node_(&pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb], ext_rr_node);
            }
        }
    }

    // Input pins fan out according to the mode of this node.
    for (int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
            build_pb_pin_node_(&pb_graph_node->input_pins[iport][ipin], LB_INTERMEDIATE, pb_type->num_modes);
        }
    }

    if (parent_node == nullptr) {
        // Top level output pins leave the block through the external interconnect node.
        for (int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
                const t_pb_graph_pin* pb_pin = &pb_graph_node->output_pins[iport][ipin];
                LbRrNodeId pin_index = LbRrNodeId(pb_pin->pin_count_in_cluster);
                begin_node_(pin_index, LB_INTERMEDIATE, 1, 1, pb_pin);
                begin_mode_();
                add_edge_(ext_rr_node, 1);
                end_node_(pin_index);
            }
        }
    } else {
        // Output pins of a sub cluster fan out according to the mode of the parent.
        for (int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
                build_pb_pin_node_(&pb_graph_node->output_pins[iport][ipin], LB_INTERMEDIATE, parent_node->pb_type->num_modes);
            }
        }
    }

    // Clock pins fan out according to the mode of this node.
    for (int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
        for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
            build_pb_pin_node_(&pb_graph_node->clock_pins[iport][ipin], LB_INTERMEDIATE, pb_type->num_modes);
        }
    }
}

static float get_cost_of_pb_edge(const t_pb_graph_edge* /*edge*/) {
    return 1;
}

static void print_lb_type_rr_graph(FILE* fp, const LbTypeRrGraph& lb_type_rr_graph) {
    for (LbRrNodeId inode : lb_type_rr_graph.nodes()) {
        fprintf(fp, "Node %zu\n", size_t(inode));
        const t_pb_graph_pin* pb_graph_pin = lb_type_rr_graph.pb_graph_pin(inode);
        if (pb_graph_pin != nullptr) {
            const t_pb_graph_node* pb_graph_node = pb_graph_pin->parent_node;
            fprintf(fp, "\t%s[%d].%s[%d]\n", pb_graph_node->pb_type->name,
                    pb_graph_node->placement_index,
                    pb_graph_pin->port->name,
                    pb_graph_pin->pin_number);
        }
        fprintf(fp, "\tType: %s\n", lb_rr_type_str[(int)lb_type_rr_graph.type(inode)]);
        fprintf(fp, "\tCapacity: %d\n", lb_type_rr_graph.capacity(inode));
        fprintf(fp, "\tNumber Modes: %d\n", lb_type_rr_graph.num_modes(inode));
        for (int imode = 0; imode < lb_type_rr_graph.num_modes(inode); imode++) {
            std::span<const LbTypeRrGraph::t_edge> edges = lb_type_rr_graph.edges(inode, imode);
            fprintf(fp, "\tMode: %d   # Outedges: %zu", imode, edges.size());
            int count = 0;
            for (const LbTypeRrGraph::t_edge& edge : edges) {
                // Break long lines
                if (count % 5 == 0) {
                    fprintf(fp, "\n\t\t");
                }
                count++;
                fprintf(fp, "(%zu, %g) ", size_t(edge.node_index), edge.cost);
            }
            fprintf(fp, "\n");
        }

        fprintf(fp, "\n");
    }
}
