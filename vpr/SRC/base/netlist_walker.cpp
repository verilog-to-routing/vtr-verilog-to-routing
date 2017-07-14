#include "netlist_walker.h"
#include "globals.h"
#include "atom_netlist.h"

void NetlistWalker::walk() {
    
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    visitor_.visit_top(atom_ctx.nlist.netlist_name().c_str());

    for(int i = 0; i < (int) cluster_ctx.clb_nlist.blocks().size(); i++) {

        //Visit the top-level block
        visitor_.visit_clb(cluster_ctx.blocks[i].pb); 

        //Visit all the block's primitives
        walk_atoms(cluster_ctx.blocks[i].pb);
    }

    visitor_.finish();
}

void NetlistWalker::walk_atoms(const t_pb* pb) {
    //Recursively travers this pb calling visitor_.visit_atom()
    //on any of its primitive pb's

    if(pb == nullptr || pb->name == NULL) {
        //Empty pb
        return;
    }

    if(pb->child_pbs == NULL) {
        //Primitive pb
        visitor_.visit_atom(pb);
        return;
    }

    //Recurse
    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    if(pb_type->num_modes > 0) {
        for(int i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
            for(int j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
                walk_atoms(&pb->child_pbs[i][j]);
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

void NetlistVisitor::visit_clb_impl(const t_pb* /*clb*/) {
    //noop
}

void NetlistVisitor::visit_atom_impl(const t_pb* /*atom*/) {
    //noop
}

void NetlistVisitor::finish_impl() {
    //noop
}
