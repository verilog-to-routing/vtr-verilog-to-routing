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

#include <unordered_set>
#include "vtr_vector_map.h"
#include "vtr_vector.h"

// Forward declarations
class AtomBlockId;
class ClusterBlockId;
class ClusteredNetlist;
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
 * @brief A function that loads and legalizes a flat placement file
 */
bool load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch);

