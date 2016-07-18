#include "netlist_walker.h"
#include "globals.h"

void NetlistWalker::walk() {
    
    visitor_.visit_top(blif_circuit_name);

    for(int i = 0; i < num_blocks; i++) {

        //Visit the top-level block
        visitor_.visit_clb(block[i].pb); 

        //Visit all the blocks primitives
        walk_atoms(block[i].pb);
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
