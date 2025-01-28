/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   Implementation of utility functions for reading and writing flat
 *          (primitive-level) placements.
 */

#include "load_flat_place.h"

#include <fstream>
#include <unordered_set>
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "FlatPlacementInfo.h"
#include "globals.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_log.h"

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
        fprintf(fp, "%s  %d %d %d %d %d #%zu %s\n",
                atom_ctx.nlist.block_name(atom).c_str(),
                blk_loc.x, blk_loc.y, blk_loc.layer,
                blk_loc.sub_tile,
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

FlatPlacementInfo read_flat_placement(const std::string& read_flat_place_file_path,
                                      const AtomNetlist& atom_netlist) {
    // Try to open the file, crash if we cannot open the file.
    std::ifstream flat_place_file(read_flat_place_file_path);
    if (!flat_place_file.is_open()) {
        VPR_ERROR(VPR_ERROR_OTHER, "Unable to open flat placement file: %s\n",
                  read_flat_place_file_path.c_str());
    }

    // Create a FlatPlacementInfo object to hold the flat placement.
    FlatPlacementInfo flat_placement_info(atom_netlist);

    // Read each line of the flat placement file.
    unsigned line_num = 0;
    std::string line;
    while (std::getline(flat_place_file, line)) {
        // Split the line into tokens (using spaces, tabs, etc. as delimiters).
        std::vector<std::string> tokens = vtr::split(line);
        // Skip empty lines
        if (tokens.empty())
            continue;
        // Skip lines that are only comments.
        if (tokens[0][0] == '#')
            continue;
        // Skip lines with too few arguments.
        //  Required arguments:
        //      - Atom name
        //      - Atom x-pos
        //      - Atom y-pos
        //      - Atom layer
        //      - Atom sub-tile
        if (tokens.size() < 5) {
            VTR_LOG_WARN("Flat placement file, line %d has too few arguments. "
                         "Requires at least: <atom_name> <x> <y> <layer> <sub_tile>\n",
                         line_num);
            continue;
        }

        // Get the atom name, which should be the first argument.
        AtomBlockId atom_blk_id = atom_netlist.find_block(tokens[0]);
        if (!atom_blk_id.is_valid()) {
            VTR_LOG_WARN("Flat placement file, line %d atom name does not match "
                         "any atoms in the atom netlist.\n",
                         line_num);
            continue;
        }

        // Check if this atom already has a flat placement
        // Using the x_pos and y_pos as identifiers.
        if (flat_placement_info.blk_x_pos[atom_blk_id] != FlatPlacementInfo::UNDEFINED_POS ||
            flat_placement_info.blk_y_pos[atom_blk_id] != FlatPlacementInfo::UNDEFINED_POS) {
            VTR_LOG_WARN("Flat placement file, line %d, atom %s has multiple "
                         "placement definitions in the flat placement file.\n",
                         line_num, atom_netlist.block_name(atom_blk_id).c_str());
            continue;
        }

        // Get the (x, y, layer) position of the atom. These functions have
        // error checking built in. We parse these as floats to allow for
        // reading in more global atom positions.
        flat_placement_info.blk_x_pos[atom_blk_id] = vtr::atof(tokens[1]);
        flat_placement_info.blk_y_pos[atom_blk_id] = vtr::atof(tokens[2]);
        flat_placement_info.blk_layer[atom_blk_id] = vtr::atof(tokens[3]);

        // Parse the sub-tile as an integer.
        flat_placement_info.blk_sub_tile[atom_blk_id] = vtr::atoi(tokens[4]);

        // If a site index is given, parse the site index as an integer.
        if (tokens.size() >= 6 && tokens[5][0] != '#')
            flat_placement_info.blk_site_idx[atom_blk_id] = vtr::atoi(tokens[5]);

        // Ignore any further tokens.

        line_num++;
    }

    // Return the flat placement info loaded from the file.
    return flat_placement_info;
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

