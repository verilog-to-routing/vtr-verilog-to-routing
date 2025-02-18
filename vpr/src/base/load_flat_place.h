/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   Utility functions for reading and writing flat placements.
 *
 * Flat placements are atom-level placements. These utilities can read and write
 * flat placement information to different parts of the VPR flow.
 */

#pragma once

#include <string>
#include <unordered_set>
#include "vtr_vector_map.h"
#include "vtr_vector.h"

// Forward declarations
class AtomBlockId;
class AtomLookup;
class AtomNetlist;
class ClusterBlockId;
class ClusteredNetlist;
class FlatPlacementInfo;
class Prepacker;
struct t_arch;
struct t_block_loc;
struct t_vpr_setup;

/**
 * @brief A function that writes a flat placement file after clustering and
 *        placement.
 *
 *  @param flat_place_file_path
 *                  Path to the file to write the flat placement to.
 *  @param cluster_netlist
 *                  The clustered netlist to write to the file path.
 *  @param block_locs
 *                  The locations of all of the blocks in the cluster_netlist.
 *  @param atoms_lookup
 *                  A lookup between each cluster and the atoms within it.
 */
void write_flat_placement(const char* flat_place_file_path,
                          const ClusteredNetlist& cluster_netlist,
                          const vtr::vector_map<ClusterBlockId, t_block_loc> &block_locs,
                          const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup);

/**
 * @brief Reads a flat placement file generated from a previous run of VTR or
 *        externally generated.
 *
 *  @param read_flat_place_file_path
 *                  Path to the file to read the flat placement from.
 *  @param atom_netlist
 *                  The netlist of atom blocks in the circuit.
 */
FlatPlacementInfo read_flat_placement(const std::string& read_flat_place_file_path,
                                      const AtomNetlist& atom_netlist);

/**
 * @brief A function that loads and legalizes a flat placement file
 */
bool load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch);

/**
 * @brief Logs information on the quality of the clustering and placement
 *        reconstruction of the given flat placement.
 *
 *  @param flat_placement_info
 *                  The flat placement to log,
 *  @param block_locs
 *                  The location of each cluster in the netlist.
 *  @param atoms_lookup
 *                  A lookup between each cluster and the atoms it contains.
 *  @param cluster_of_atom_lookup
 *                  A lookup between each atom and the cluster that contains it.
 *  @param atom_netlist
 *                  The netlist of atoms the flat placement was over.
 *  @param clustered_netlist
 *                  The clustered netlist that the flat placement was used to
 *                  generate.
 */
void log_flat_placement_reconstruction_info(
                const FlatPlacementInfo& flat_placement_info,
                const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup,
                const AtomLookup& cluster_of_atom_lookup,
                const AtomNetlist& atom_netlist,
                const ClusteredNetlist& clustered_netlist);

