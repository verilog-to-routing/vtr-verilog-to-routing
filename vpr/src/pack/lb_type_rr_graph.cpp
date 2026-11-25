/**
 * @file
 * @brief Routing resource graph that represents the interconnect within a logic block type.
 *
 * @see lb_type_rr_graph_builder.cpp for how it is built from the pb graph.
 */

#include <cstdio>
#include <vector>

#include "vpr_utils.h"
#include "vtr_util.h"
#include "physical_types.h"
#include "globals.h"
#include "lb_type_rr_graph.h"

/// @brief Prints one graph to fp.
static void print_lb_type_rr_graph(FILE* fp, const LbTypeRRGraph& lb_type_rr_graph);

int LbTypeRRGraph::edge_mode(LbRRNodeId src, LbRRNodeId dst) const {
    for (int imode = 0; imode < num_modes(src); imode++) {
        for (const t_edge& edge : edges(src, imode)) {
            if (edge.node_index == dst) {
                return imode;
            }
        }
    }
    return -1;
}

void echo_lb_type_rr_graphs(char* filename, const std::vector<LbTypeRRGraph>& lb_type_rr_graphs) {
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

static void print_lb_type_rr_graph(FILE* fp, const LbTypeRRGraph& lb_type_rr_graph) {
    for (LbRRNodeId inode : lb_type_rr_graph.nodes()) {
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
            std::span<const LbTypeRRGraph::t_edge> edges = lb_type_rr_graph.edges(inode, imode);
            fprintf(fp, "\tMode: %d   # Outedges: %zu", imode, edges.size());
            int count = 0;
            for (const LbTypeRRGraph::t_edge& edge : edges) {
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
