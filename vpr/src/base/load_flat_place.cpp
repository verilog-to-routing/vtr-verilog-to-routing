#include "globals.h"
#include "load_flat_place.h"
#include "clustered_netlist_utils.h"


/* @brief Prints flat placement file entries for the atoms in one placed cluster. */
static void print_flat_cluster(FILE* fp, ClusterBlockId iblk,
                               std::vector<AtomBlockId>& atoms);

static void print_flat_cluster(FILE* fp, ClusterBlockId iblk,
                               std::vector<AtomBlockId>& atoms) {

    auto& atom_ctx = g_vpr_ctx.atom();
    t_pl_loc loc = g_vpr_ctx.placement().block_locs[iblk].loc;
    size_t bnum = size_t(iblk);

    for (auto atom : atoms) {
        t_pb_graph_node* atom_pbgn = atom_ctx.lookup.atom_pb(atom)->pb_graph_node;
        fprintf(fp, "%s  %d %d %d %d #%zu %s\n", atom_ctx.nlist.block_name(atom).c_str(),
                                                loc.x, loc.y, loc.sub_tile,
                                                atom_pbgn->flat_site_index,
                                                bnum,
                                                atom_pbgn->pb_type->name);
    }
}

/* prints a flat placement file */
void print_flat_placement(const char* flat_place_file) {

    FILE* fp;

    ClusterAtomsLookup atoms_lookup;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (!g_vpr_ctx.placement().block_locs.empty()) {
        fp = fopen(flat_place_file, "w");
        for (auto iblk : cluster_ctx.clb_nlist.blocks()) {
            auto atoms = atoms_lookup.atoms_in_cluster(iblk);
            print_flat_cluster(fp, iblk, atoms);
        }
        fclose(fp);
    }

}

/* ingests and legalizes a flat placement file  */
bool load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch) {
    VTR_LOG("load_flat_placement(); when implemented, this function:");
    VTR_LOG("\n\tLoads flat placement file: %s, ", vpr_setup.FileNameOpts.FlatPlaceFile.c_str());
    VTR_LOG("\n\tArch id: %s, ", arch.architecture_id);
    VTR_LOG("\n\tPrints clustered netlist file: %s, ", vpr_setup.FileNameOpts.NetFile.c_str());
    VTR_LOG("\n\tPrints fix clusters file: %s\n", vpr_setup.FileNameOpts.write_constraints_file.c_str());

    return false;
}
