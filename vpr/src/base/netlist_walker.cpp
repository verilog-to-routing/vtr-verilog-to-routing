#include "netlist_walker.h"
#include "globals.h"
#include "atom_netlist.h"

void NetlistWalker::walk() {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    visitor_.visit_top(atom_ctx.nlist.netlist_name().c_str());

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        const auto* pb = cluster_ctx.clb_nlist.block_pb(blk_id);

        //Visit the top-level block
        visitor_.visit_clb(blk_id, pb);

        //Visit all the block's primitives
        walk_blocks(pb->pb_route, pb);
    }

    visitor_.finish();
}

void NetlistWalker::walk_blocks(const t_pb_routes& top_pb_route, const t_pb* pb) {
    // Recursively travers this pb calling visitor_.visit_atom() or
    // visitor_.visit_open() on any of its primitive pb's.

    VTR_ASSERT(pb != nullptr);
    VTR_ASSERT(pb->pb_graph_node != nullptr);

    visitor_.visit_all(top_pb_route, pb);
    if (pb->child_pbs == nullptr) {
        //Primitive pb
        if (pb->name != NULL) {
            visitor_.visit_atom(pb);
        } else {
            visitor_.visit_route_through(pb);
        }
        return;
    }

    //Recurse
    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    if (pb_type->num_modes > 0) {
        const t_mode* mode = &pb_type->modes[pb->mode];
        for (int i = 0; i < mode->num_pb_type_children; i++) {
            for (int j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                walk_blocks(top_pb_route, &pb->child_pbs[i][j]);
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

void NetlistVisitor::visit_route_through_impl(const t_pb* /*atom*/) {
    //noop
}

void NetlistVisitor::visit_atom_impl(const t_pb* /*atom*/) {
    //noop
}

void NetlistVisitor::visit_all_impl(const t_pb_routes& /*top_pb_route*/, const t_pb* /* pb */) {
    //noop
}

void NetlistVisitor::finish_impl() {
    //noop
}
