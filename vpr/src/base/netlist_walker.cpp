#include "netlist_walker.h"
#include "globals.h"
#include "atom_netlist.h"

void NetlistWalker::walk() {

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    visitor_.visit_top(atom_ctx.nlist.netlist_name().c_str());

    for(auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        const auto pb = cluster_ctx.clb_nlist.block_pb(blk_id);
        visitor_.visit_clb(blk_id, pb);
        walk_blocks(cluster_ctx.clb_nlist.block_pb(blk_id)->pb_route, pb, pb->pb_graph_node);
    }

    visitor_.finish();
}

void NetlistWalker::walk_blocks(const t_pb_route *top_pb_route, const t_pb *pb, const t_pb_graph_node *pb_graph_node) {
    VTR_ASSERT(top_pb_route != nullptr);
    VTR_ASSERT(pb == nullptr || pb_graph_node == pb->pb_graph_node);
    VTR_ASSERT(pb_graph_node != nullptr);

    visitor_.visit_all(top_pb_route, pb, pb_graph_node);
    if (pb != nullptr) {
        if (pb->name != NULL) {
            visitor_.visit_atom(pb);
        } else {
            visitor_.visit_open(pb);
        }
    }

    auto pb_type = pb_graph_node->pb_type;

    int mode_index = 0;
    if (pb != nullptr) {
        mode_index = pb->mode;
    }

    auto mode = &pb_type->modes[mode_index];
    if (pb_type->num_modes > 0) {
        for (int i = 0; i < mode->num_pb_type_children; i++) {
            for (int j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                //if ((pb != nullptr) && (pb->child_pbs[i] != nullptr) && (pb->child_pbs[i][j].name != NULL)) {
                if ((pb != nullptr)
                        && (pb->child_pbs != nullptr)
                        && (pb->child_pbs[i] != nullptr)
                        && (pb->child_pbs[i][j].pb_graph_node != nullptr)
                        ) {
                    walk_blocks(top_pb_route, &pb->child_pbs[i][j], pb->child_pbs[i][j].pb_graph_node);
                } else {
                    walk_blocks(top_pb_route, nullptr, &pb_graph_node->child_pb_graph_nodes[mode_index][i][j]);
                }
            }
        }
    }
}

void NetlistVisitor::start_impl() {
    //noop
}

void NetlistVisitor::visit_top_impl(const char* /*top_level_name*/) {
    //noop
}

void NetlistVisitor::visit_clb_impl(ClusterBlockId /*blk_id*/, const t_pb* /*clb*/) {
    //noop
}

void NetlistVisitor::visit_open_impl(const t_pb* /*atom*/) {
    //noop
}

void NetlistVisitor::visit_atom_impl(const t_pb* /*atom*/) {
    //noop
}

void NetlistVisitor::visit_all_impl(const t_pb_route* /*top_pb_route*/, const t_pb* /* pb */, const t_pb_graph_node* /* pb_graph_node */) {
    //noop
}

void NetlistVisitor::finish_impl() {
    //noop
}
