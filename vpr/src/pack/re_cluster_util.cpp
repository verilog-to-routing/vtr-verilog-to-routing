#include "re_cluster_util.h"

#include "vpr_context.h"
#include "clustered_netlist_utils.h"

ClusterBlockId atom_to_cluster(const AtomBlockId& atom) {
    auto& atom_ctx = g_vpr_ctx.atom();
    return (atom_ctx.lookup.atom_clb(atom));
}

std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster) {
    ClusterAtomsLookup cluster_lookup;
    return (cluster_lookup.atoms_in_cluster(cluster));
}

bool remove_mol_from_cluster(const t_pack_molecule* molecule) {
	ClusterBlockId clb_ = atom_to_cluster(atom);


}