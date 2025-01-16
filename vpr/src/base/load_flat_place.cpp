/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   Implementation of utility functions for reading and writing flat
 *          (primitive-level) placements.
 */

#include "load_flat_place.h"

#include <unordered_set>
#include "clustered_netlist.h"
#include "globals.h"
#include "vpr_context.h"
#include "vpr_types.h"

/**
 * @brief Prints flat placement file entries for the atoms in one placed
 *        cluster.
 *
 *  @param fp
 *              File pointer to the file the cluster is printed to.
 *  @param blk_id
 *              The ID of the cluster block to print.
 *  @param block_locs
 *              The locations of all cluster blocks.
 *  @param atoms_lookup
 *              A lookup between all clusters and the atom blocks that they
 *              contain.
 */
static void print_flat_cluster(FILE* fp,
                               ClusterBlockId blk_id,
                               const vtr::vector_map<ClusterBlockId, t_block_loc> &block_locs,
                               const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup) {
    // Atom context used to get the atom_pb for each atom in the cluster.
    // NOTE: This is only used for getting the flat site index.
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // Get the location of this cluster.
    const t_pl_loc& blk_loc = block_locs[blk_id].loc;

    // Print a line for each atom.
    for (AtomBlockId atom : atoms_lookup[blk_id]) {
        // Get the atom pb graph node.
        t_pb_graph_node* atom_pbgn = atom_ctx.lookup.atom_pb(atom)->pb_graph_node;

        // Print the flat placement information for this atom.
        fprintf(fp, "%s  %d %d %d %d #%zu %s\n",
                atom_ctx.nlist.block_name(atom).c_str(),
                blk_loc.x, blk_loc.y, blk_loc.sub_tile,
                atom_pbgn->flat_site_index,
                static_cast<size_t>(blk_id),
                atom_pbgn->pb_type->name);
    }
}

void write_flat_placement(const char* flat_place_file_path,
                          const ClusteredNetlist& cluster_netlist,
                          const vtr::vector_map<ClusterBlockId, t_block_loc> &block_locs,
                          const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup) {
    // Writes the flat placement to the given flat_place_file_path.

    // Only print a flat placement if the clusters have been placed.
    if (block_locs.empty())
        return;

    // Create a file in write mode for the flat placement.
    FILE* fp = fopen(flat_place_file_path, "w");

    // For each cluster, write out the atoms in the cluster at this cluster's
    // location.
    for (ClusterBlockId iblk : cluster_netlist.blocks()) {
        print_flat_cluster(fp, iblk, block_locs, atoms_lookup);
    }

    // Close the file.
    fclose(fp);
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

