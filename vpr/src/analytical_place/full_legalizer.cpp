/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Implements the full legalizer in the AP flow. The Full Legalizer
 *          takes a partial placement and fully legalizes it. This involves
 *          creating legal clusters and placing them into valid tile sites.
 */

#include "full_legalizer.h"

#include <cstring>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

#include "PreClusterTimingManager.h"
#include "ShowSetup.h"
#include "ap_flow_enums.h"
#include "ap_netlist_fwd.h"
#include "blk_loc_registry.h"
#include "check_netlist.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "clustered_netlist.h"
#include "device_grid.h"
#include "flat_placement_types.h"
#include "globals.h"
#include "initial_placement.h"
#include "load_flat_place.h"
#include "logic_types.h"
#include "noc_place_utils.h"
#include "pack.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "place.h"
#include "place_and_route.h"
#include "place_constraints.h"
#include "place_macro.h"
#include "prepack.h"
#include "read_place.h"
#include "verify_clustering.h"
#include "verify_placement.h"
#include "vpr_api.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_ndmatrix.h"
#include "vtr_random.h"
#include "vtr_strong_id.h"
#include "vtr_time.h"
#include "vtr_vector.h"

#include "setup_grid.h"
#include "stats.h"
#ifndef NO_GRAPHICS
#include "draw_global.h"
#endif

std::unique_ptr<FullLegalizer> make_full_legalizer(e_ap_full_legalizer full_legalizer_type,
                                                   const APNetlist& ap_netlist,
                                                   const AtomNetlist& atom_netlist,
                                                   const Prepacker& prepacker,
                                                   const PreClusterTimingManager& pre_cluster_timing_manager,
                                                   const t_vpr_setup& vpr_setup,
                                                   const t_arch& arch,
                                                   const DeviceGrid& device_grid) {
    switch (full_legalizer_type) {
        case e_ap_full_legalizer::Naive:
            return std::make_unique<NaiveFullLegalizer>(ap_netlist,
                                                        atom_netlist,
                                                        prepacker,
                                                        pre_cluster_timing_manager,
                                                        vpr_setup,
                                                        arch,
                                                        device_grid);
        case e_ap_full_legalizer::APPack:
            return std::make_unique<APPack>(ap_netlist,
                                            atom_netlist,
                                            prepacker,
                                            pre_cluster_timing_manager,
                                            vpr_setup,
                                            arch,
                                            device_grid);
        case e_ap_full_legalizer::FlatRecon:
            return std::make_unique<FlatRecon>(ap_netlist,
                                               atom_netlist,
                                               prepacker,
                                               pre_cluster_timing_manager,
                                               vpr_setup,
                                               arch,
                                               device_grid);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized full legalizer type");
    }
}

namespace {

/// @brief A unique ID for each root tile on the device.
///
/// This is used for putting the molecules in bins for packing.
// FIXME: Bring this into the device_grid.
//  - Maybe this can be called DeviceRootTileId or something.
typedef vtr::StrongId<struct device_tile_id_tag, size_t> DeviceTileId;

/**
 * @brief Helper class to place cluster in the AP context.
 *
 * A lot of this code was lifted from the Initial Placer within the placement
 * flow.
 * TODO: Should try to do the same thing we did with the ClusterLegalizer to
 *       unify the two flows and make it more stable!
 */
class APClusterPlacer {
  private:
    // Get the macro for the given cluster block.
    t_pl_macro get_macro(ClusterBlockId clb_blk_id) {
        // Basically stolen from initial_placement.cpp:place_one_block
        // TODO: Make this a cleaner interface and share the code.
        int imacro = place_macros_.get_imacro_from_iblk(clb_blk_id);

        // If this block is part of a macro, return it.
        if (imacro != -1) {
            return place_macros_[imacro];
        }
        // If not, create a "fake" macro with a single element.
        t_pl_macro_member macro_member;
        t_pl_offset block_offset(0, 0, 0, 0);
        macro_member.blk_index = clb_blk_id;
        macro_member.offset = block_offset;

        t_pl_macro pl_macro;
        pl_macro.members.push_back(macro_member);
        return pl_macro;
    }

    const PlaceMacros& place_macros_;

  public:
    /**
     * @brief Constructor for the APClusterPlacer
     *
     * Initializes internal and global state necessary to place clusters on the
     * FPGA device.
     */
    APClusterPlacer(const PlaceMacros& place_macros,
                    const char* constraints_file)
        : place_macros_(place_macros) {
        // Initialize the block loc registry.
        auto& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();
        blk_loc_registry.init();

        // Place the fixed blocks and mark them as fixed.
        mark_fixed_blocks(blk_loc_registry);

        // Read the constraint file and place fixed blocks.
        if (strlen(constraints_file) != 0) {
            read_constraints(constraints_file, blk_loc_registry);
        }

        // Update the block loc registry with the fixed / moveable blocks.
        // We can do this here since the fixed blocks will not change beyond
        // this point.
        blk_loc_registry.alloc_and_load_movable_blocks();
    }

    /**
     * @brief Given a cluster and tile it wants to go into, try to place the
     *        cluster at this tile's postion.
     */
    bool place_cluster(ClusterBlockId clb_blk_id,
                       const t_physical_tile_loc& tile_loc,
                       int sub_tile) {
        const DeviceContext& device_ctx = g_vpr_ctx.device();
        const FloorplanningContext& floorplanning_ctx = g_vpr_ctx.floorplanning();
        const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
        const auto& block_locs = g_vpr_ctx.placement().block_locs();
        auto& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();
        // If this block has already been placed, just return true.
        // TODO: This should be investigated further. What I think is happening
        //       is that a macro is being placed which contains another cluster.
        //       This must be a carry chain. May need to rewrite the algorithm
        //       below to use macros instead of clusters.
        if (is_block_placed(clb_blk_id, block_locs))
            return true;
        VTR_ASSERT(!is_block_placed(clb_blk_id, block_locs) && "Block already placed. Is this intentional?");
        t_pl_macro pl_macro = get_macro(clb_blk_id);
        t_pl_loc to_loc;
        to_loc.x = tile_loc.x;
        to_loc.y = tile_loc.y;
        to_loc.layer = tile_loc.layer_num;
        // Special case where the tile has no sub-tiles. It just cannot be placed.
        if (device_ctx.grid.get_physical_type(tile_loc)->sub_tiles.size() == 0)
            return false;
        VTR_ASSERT(sub_tile >= 0 && sub_tile < device_ctx.grid.get_physical_type(tile_loc)->capacity);
        // Check if this cluster is constrained and this location is legal.
        if (is_cluster_constrained(clb_blk_id)) {
            const auto& cluster_constraints = floorplanning_ctx.cluster_constraints;
            if (cluster_constraints[clb_blk_id].is_loc_in_part_reg(to_loc))
                return false;
        }
        // If the location is legal, try to exhaustively place it at this tile
        // location. This should try all sub_tiles.
        PartitionRegion pr;
        vtr::Rect<int> rect(tile_loc.x, tile_loc.y, tile_loc.x, tile_loc.y);
        pr.add_to_part_region(Region(rect, to_loc.layer));
        const ClusteredNetlist& clb_nlist = cluster_ctx.clb_nlist;
        t_logical_block_type_ptr block_type = clb_nlist.block_type(clb_blk_id);
        enum e_pad_loc_type pad_loc_type = g_vpr_ctx.device().pad_loc_type;
        // FIXME: This currently ignores the sub_tile. Was running into issues
        //        with trying to force clusters to specific sub_tiles.
        return try_place_macro_exhaustively(pl_macro, pr, block_type,
                                            pad_loc_type, blk_loc_registry);
    }

    // This is not the best way of doing things, but its the simplest. Given a
    // cluster, just find somewhere for it to go.
    // TODO: Make this like the initial placement code where we first try
    //       centroid, then random, then exhaustive.
    bool exhaustively_place_cluster(ClusterBlockId clb_blk_id) {
        const auto& block_locs = g_vpr_ctx.placement().block_locs();
        auto& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();
        // If this block has already been placed, just return true.
        // TODO: See similar comment above.
        if (is_block_placed(clb_blk_id, block_locs))
            return true;
        VTR_ASSERT(!is_block_placed(clb_blk_id, block_locs) && "Block already placed. Is this intentional?");
        t_pl_macro pl_macro = get_macro(clb_blk_id);
        const PartitionRegion& pr = is_cluster_constrained(clb_blk_id) ? g_vpr_ctx.floorplanning().cluster_constraints[clb_blk_id] : get_device_partition_region();
        t_logical_block_type_ptr block_type = g_vpr_ctx.clustering().clb_nlist.block_type(clb_blk_id);
        // FIXME: We really should get this from the place context, not the device context.
        //      - Stealing it for now to get this to work.
        enum e_pad_loc_type pad_loc_type = g_vpr_ctx.device().pad_loc_type;
        return try_place_macro_exhaustively(pl_macro, pr, block_type, pad_loc_type, blk_loc_registry);
    }
};

} // namespace

/**
 * @brief Create a new cluster for the given seed molecule using the cluster
 *        legalizer.
 *
 *  @param seed_molecule                    The molecule to use as a starting
 *                                          point for the cluster.
 *  @param cluster_legalizer                A cluster legalizer object to build
 *                                          the cluster.
 *  @param primitive_candidate_block_types  A list of candidate block types for
 *                                          the given molecule.
 */
static LegalizationClusterId create_new_cluster(PackMoleculeId seed_molecule_id,
                                                const Prepacker& prepacker,
                                                ClusterLegalizer& cluster_legalizer,
                                                const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    // This was stolen from pack/cluster_util.cpp:start_new_cluster
    // It tries to find a block type and mode for the given molecule.
    // TODO: This should take into account the tile this molecule wants to be
    //       placed into.
    // TODO: The original implementation sorted based on balance. Perhaps this
    //       should do the same.
    VTR_ASSERT(seed_molecule_id.is_valid());
    const t_pack_molecule& seed_molecule = prepacker.get_molecule(seed_molecule_id);
    AtomBlockId root_atom = seed_molecule.atom_block_ids[seed_molecule.root];
    LogicalModelId root_model_id = atom_ctx.netlist().block_model(root_atom);

    VTR_ASSERT(root_model_id.is_valid());
    VTR_ASSERT(!primitive_candidate_block_types[root_model_id].empty());
    const std::vector<t_logical_block_type_ptr>& candidate_types = primitive_candidate_block_types[root_model_id];

    for (t_logical_block_type_ptr type : candidate_types) {
        int num_modes = type->pb_graph_head->pb_type->num_modes;
        for (int mode = 0; mode < num_modes; mode++) {
            e_block_pack_status pack_status = e_block_pack_status::BLK_STATUS_UNDEFINED;
            LegalizationClusterId new_cluster_id;
            std::tie(pack_status, new_cluster_id) = cluster_legalizer.start_new_cluster(seed_molecule_id, type, mode);
            if (pack_status == e_block_pack_status::BLK_PASSED)
                return new_cluster_id;
        }
    }
    // This should never happen.
    VPR_FATAL_ERROR(VPR_ERROR_AP,
                    "Unable to create a cluster for the given seed molecule");
    return LegalizationClusterId();
}

/**
 * @brief Get the logical block type of a given molecule.
 *
 *  @param mol_id                           The molecule id to get its logical block type.
 *  @param prepacker                        The prepacker used to get molecule from its id.
 *  @param primitive_candidate_block_types  Candidate logical block types for the given molecule.
 *
 */
static t_logical_block_type_ptr infer_molecule_logical_block_type(PackMoleculeId mol_id,
                                                                  const Prepacker& prepacker,
                                                                  const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    // Get the root atom and its model id. Ensure that both is valid.
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const t_pack_molecule& molecule = prepacker.get_molecule(mol_id);
    AtomBlockId root_atom = molecule.atom_block_ids[molecule.root];
    VTR_ASSERT(root_atom.is_valid());
    LogicalModelId root_model_id = atom_ctx.netlist().block_model(root_atom);
    VTR_ASSERT(root_model_id.is_valid());

    // Get the first candidate type.
    const auto& candidate_types = primitive_candidate_block_types[root_model_id];
    if (!candidate_types.empty()) {
        return candidate_types.front();
    }

    // A valid block type should have been found at that point.
    VPR_FATAL_ERROR(VPR_ERROR_AP, "Could not determine block type for molecule ID %zu\n", size_t(mol_id));
}

std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>
FlatRecon::sort_and_group_blocks_by_tile(const PartialPlacement& p_placement) {
    vtr::ScopedStartFinishTimer pack_reconstruction_timer("Sorting and Grouping Blocks by Tile");
    // Block sorting information. This can be altered easily to try different sorting strategies.
    struct BlockInformation {
        PackMoleculeId mol_id;
        int ext_inps;
        bool is_long_chain;
        t_physical_tile_loc tile_loc;
    };

    // Collect the sorting information and tile information.
    std::vector<BlockInformation> sorted_blocks;
    sorted_blocks.reserve(ap_netlist_.blocks().size());
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PackMoleculeId mol_id = ap_netlist_.block_molecule(blk_id);
        const auto& mol = prepacker_.get_molecule(mol_id);

        int num_ext_inputs = prepacker_.calc_molecule_stats(mol_id, atom_netlist_, arch_.models).num_used_ext_inputs;
        bool long_chain = mol.is_chain() && prepacker_.get_molecule_chain_info(mol.chain_id).is_long_chain;
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);

        sorted_blocks.push_back({mol_id, num_ext_inputs, long_chain, tile_loc});
    }

    // Sort the blocks so that:
    // 1) Long carry-chain molecules are placed first. They have strict placement
    //    constraints, so we want to place them before the layout becomes constrained
    //    by other blocks. This will avoid increasing the displacement.
    // 2) Within the same category (both long-chain or both non-long-chain), sort
    //    by descending number of external input pins. (empirically best for reconstructing
    //    dense clusters compared to external outputs or external total pins).
    std::sort(sorted_blocks.begin(), sorted_blocks.end(),
              [](const BlockInformation& a, const BlockInformation& b) {
                  // Long chains should always come before non-long chains
                  if (a.is_long_chain && !b.is_long_chain)
                      return true;
                  if (!a.is_long_chain && b.is_long_chain)
                      return false;

                  // If both blocks are chains / not chains, sort in decreasing order of external inputs.
                  return a.ext_inps > b.ext_inps;
              });

    // Group the molecules by root tile. Any non-zero offset gets
    // pulled back to its root.
    std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>> tile_blocks;
    mol_desired_physical_tile_loc.reserve(ap_netlist_.blocks().size());
    for (const auto& [mol_id, ext_pins, is_long_chain, tile_loc] : sorted_blocks) {
        int width_offset = device_grid_.get_width_offset(tile_loc);
        int height_offset = device_grid_.get_height_offset(tile_loc);
        t_physical_tile_loc root_loc = {tile_loc.x - width_offset,
                                        tile_loc.y - height_offset,
                                        tile_loc.layer_num};
        tile_blocks[root_loc].push_back(mol_id);
        mol_desired_physical_tile_loc[mol_id] = root_loc;
    }

    return tile_blocks;
}

std::unordered_set<LegalizationClusterId>
FlatRecon::cluster_molecules_in_tile(const t_physical_tile_loc& tile_loc,
                                     const t_physical_tile_type_ptr& tile_type,
                                     const std::vector<PackMoleculeId>& tile_molecules,
                                     ClusterLegalizer& cluster_legalizer,
                                     const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    std::unordered_set<LegalizationClusterId> created_clusters;
    for (PackMoleculeId mol_id : tile_molecules) {
        // Get the block type for compatibility check.
        t_logical_block_type_ptr block_type = infer_molecule_logical_block_type(mol_id, prepacker_, primitive_candidate_block_types);

        // Go over all subtiles at this tile, trying to insert each molecule that
        // is supposed to be in that tile in the first existing cluster that can
        // accommodate it; if none can, create a new cluster if there is a subtile
        // that has no cluster yet.
        for (int sub_tile = 0; sub_tile < tile_type->capacity; ++sub_tile) {
            const t_pl_loc loc{tile_loc.x, tile_loc.y, sub_tile, tile_loc.layer_num};
            auto cluster_it = loc_to_cluster_id_placed.find(loc);

            if (cluster_it != loc_to_cluster_id_placed.end()) {
                // Try adding to the existing cluster
                LegalizationClusterId cluster_id = cluster_it->second;
                if (!cluster_legalizer.is_molecule_compatible(mol_id, cluster_id))
                    continue;

                e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(mol_id, cluster_id);
                if (pack_status == e_block_pack_status::BLK_PASSED)
                    break;
            } else if (is_tile_compatible(tile_type, block_type)) {
                // Create new cluster
                LegalizationClusterId new_id = create_new_cluster(mol_id, prepacker_, cluster_legalizer, primitive_candidate_block_types);
                created_clusters.insert(new_id);
                cluster_locs[new_id] = loc;
                loc_to_cluster_id_placed[loc] = new_id;
                tile_clusters_matrix[tile_loc.layer_num][tile_loc.x][tile_loc.y].insert(new_id);
                break;
            }
        }
    }
    return created_clusters;
}

void FlatRecon::self_clustering(ClusterLegalizer& cluster_legalizer,
                                const DeviceGrid& device_grid,
                                const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>& tile_blocks) {
    vtr::ScopedStartFinishTimer reconstruction_pass_clustering("Reconstruction Pass Clustering");
    for (const auto& [tile_loc, tile_molecules] : tile_blocks) {
        // Get tile type of current tile location.
        const t_physical_tile_type_ptr tile_type = device_grid.get_physical_type(tile_loc);

        // Try to create clusters with fast strategy checking the compatibility
        // with tile and its capacity. Store the cluster ids to check their legality.
        cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);
        std::unordered_set<LegalizationClusterId> created_clusters = cluster_molecules_in_tile(tile_loc,
                                                                                               tile_type,
                                                                                               tile_molecules,
                                                                                               cluster_legalizer,
                                                                                               primitive_candidate_block_types);
        // Check legality of clusters created with fast pass. Store the
        // illegal cluster molecules for full strategy pass.
        std::vector<PackMoleculeId> illegal_cluster_mols;
        for (LegalizationClusterId cluster_id : created_clusters) {
            if (!cluster_legalizer.check_cluster_legality(cluster_id)) {
                for (PackMoleculeId mol_id : cluster_legalizer.get_cluster_molecules(cluster_id)) {
                    illegal_cluster_mols.push_back(mol_id);
                }
                // Erase related data of illegal cluster
                loc_to_cluster_id_placed.erase(cluster_locs[cluster_id]);
                cluster_legalizer.destroy_cluster(cluster_id);
                tile_clusters_matrix[tile_loc.layer_num][tile_loc.x][tile_loc.y].erase(cluster_id);
            } else {
                cluster_legalizer.clean_cluster(cluster_id);
            }
        }

        // If there are any illegal molecules, set the legalization strategy to
        // full and try to cluster the unclustered molecules in same tile again.
        if (!illegal_cluster_mols.empty()) {
            cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);
            created_clusters = cluster_molecules_in_tile(tile_loc,
                                                         tile_type,
                                                         illegal_cluster_mols,
                                                         cluster_legalizer,
                                                         primitive_candidate_block_types);
            // Clean clusters created with full strategy not to increase memory footprint.
            for (LegalizationClusterId cluster_id : created_clusters) {
                cluster_legalizer.clean_cluster(cluster_id);
            }
        }
    }
}

std::unordered_set<PackMoleculeId>
FlatRecon::neighbor_clustering(ClusterLegalizer& cluster_legalizer,
                               const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    vtr::ScopedStartFinishTimer neigh_pass_clustering("Neighbor Pass Clustering");

    // Iterate over molecules and try to join the unclustered ones to their
    // already created 8-neighboring tile clusters.
    std::unordered_set<PackMoleculeId> mols_clustered;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        // Get unclustered block and its location.
        PackMoleculeId molecule_id = ap_netlist_.block_molecule(blk_id);
        t_physical_tile_loc loc = mol_desired_physical_tile_loc[molecule_id];

        // Skip the already clustered molecules.
        if (cluster_legalizer.is_mol_clustered(molecule_id))
            continue;

        // Get 8-neighbouring tile locations of the current molecule in the same layer.
        std::vector<t_physical_tile_loc> neighbor_tile_locs;
        neighbor_tile_locs.reserve(8);
        auto [layers, width, height] = device_grid_.dim_sizes();
        for (int dx : {-1, 0, 1}) {
            for (int dy : {-1, 0, 1}) {
                if (dx == 0 && dy == 0) continue;
                int neighbor_x = loc.x + dx, neighbor_y = loc.y + dy;
                if (neighbor_x < 0 || neighbor_x >= (int)width || neighbor_y < 0 || neighbor_y >= (int)height)
                    continue;
                neighbor_tile_locs.push_back({neighbor_x, neighbor_y, loc.layer_num});
            }
        }

        // Get the average molecule count in each neighbor tile location.
        // Also remove empty neighbor tiles from neighbor_tile_locs.
        std::unordered_map<t_physical_tile_loc, double> avg_mols_in_tile;
        avg_mols_in_tile.reserve(neighbor_tile_locs.size());
        for (auto it = neighbor_tile_locs.begin(); it != neighbor_tile_locs.end();) {
            const std::unordered_set<LegalizationClusterId>& clusters = tile_clusters_matrix[it->layer_num][it->x][it->y];
            if (clusters.empty()) {
                it = neighbor_tile_locs.erase(it);
                continue;
            }
            size_t total_molecules_in_tile = 0;
            for (const LegalizationClusterId& cluster_id : clusters) {
                total_molecules_in_tile += cluster_legalizer.get_num_molecules_in_cluster(cluster_id);
            }
            avg_mols_in_tile[*it] = double(total_molecules_in_tile) / clusters.size();
            ++it;
        }

        // Sort tile locations by increasing average molecule count.
        std::sort(neighbor_tile_locs.begin(), neighbor_tile_locs.end(),
                  [&](const t_physical_tile_loc& a, const t_physical_tile_loc& b) {
                      return avg_mols_in_tile[a] < avg_mols_in_tile[b];
                  });

        // Try to fit the unclustered molecule to sorted neighbor tile clusters.
        // Note: This pass opens a cluster, try to add one molecule to it, then close it again. This might cost CPU
        // time if many molecules are packed in the same cluster in this pass, vs. just opening it once and adding
        // them all.
        bool fit_in_a_neighbor = false;
        for (const t_physical_tile_loc& neighbor_tile_loc : neighbor_tile_locs) {
            // Get the current neighbor tile clusters.
            std::unordered_set<LegalizationClusterId>& clusters = tile_clusters_matrix[neighbor_tile_loc.layer_num][neighbor_tile_loc.x][neighbor_tile_loc.y];

            // Iterate over the current tile clusters until unclustered molecule fit in one.
            for (auto it = clusters.begin(); it != clusters.end() && !fit_in_a_neighbor;) {
                LegalizationClusterId cluster_id = *it;
                if (!cluster_id.is_valid()) {
                    ++it;
                    continue;
                }

                // Get the cluster molecules and destroy the old cluster.
                std::vector<PackMoleculeId> cluster_molecules = cluster_legalizer.get_cluster_molecules(cluster_id);
                cluster_legalizer.destroy_cluster(cluster_id);

                // Set the legalization strategy to speculative for fast try.
                cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);

                // Use the first molecule as seed to recreate the cluster.
                PackMoleculeId seed_mol = cluster_molecules[0];
                LegalizationClusterId new_cluster_id = create_new_cluster(seed_mol, prepacker_, cluster_legalizer, primitive_candidate_block_types);

                // Add remaining old molecules to the new cluster.
                for (PackMoleculeId mol_id : cluster_molecules) {
                    if (mol_id == seed_mol)
                        continue;
                    if (!cluster_legalizer.is_molecule_compatible(mol_id, new_cluster_id))
                        continue;
                    cluster_legalizer.add_mol_to_cluster(mol_id, new_cluster_id);
                }

                // Set the legalization strategy to full for adding new unclustered molecule.
                // Also if recreated clusters if illegal, try to create with full strategy.
                cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);

                // If recreated cluster is illegal, try again with full strategy.
                if (!cluster_legalizer.check_cluster_legality(new_cluster_id)) {
                    cluster_legalizer.destroy_cluster(new_cluster_id);
                    new_cluster_id = create_new_cluster(seed_mol, prepacker_, cluster_legalizer, primitive_candidate_block_types);
                    for (PackMoleculeId mol_id : cluster_molecules) {
                        if (mol_id == seed_mol)
                            continue;
                        if (!cluster_legalizer.is_molecule_compatible(mol_id, new_cluster_id))
                            continue;
                        cluster_legalizer.add_mol_to_cluster(mol_id, new_cluster_id);
                    }
                }

                // Lastly, try to add the new unclustered molecule to the recreated cluster.
                if (cluster_legalizer.is_molecule_compatible(molecule_id, new_cluster_id)) {
                    e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(molecule_id, new_cluster_id);
                    if (pack_status == e_block_pack_status::BLK_PASSED)
                        fit_in_a_neighbor = true;
                }

                // Clean the new cluster to avoid increasing memory footprint.
                cluster_legalizer.clean_cluster(new_cluster_id);

                // Erase old cluster id and add new one.
                it = clusters.erase(it);
                clusters.insert(new_cluster_id);
            }
            // Stop iterating neighbor tiles if current molecule already fit in a neighbor cluster.
            if (fit_in_a_neighbor) {
                mols_clustered.insert(molecule_id);
                break;
            }
        }
    }
    return mols_clustered;
}

std::unordered_set<LegalizationClusterId>
FlatRecon::orphan_window_clustering(ClusterLegalizer& cluster_legalizer,
                                    const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                    int search_radius) {
    std::string timer_label = "Orphan Window Clustering with search radius " + std::to_string(search_radius);
    vtr::ScopedStartFinishTimer orphan_window_clustering(timer_label);

    // Create unclustered blocks spatial data. It stores a vector of molecules ids
    // for each tile location of [layer][x][y].
    auto [layer_num, width, height] = device_grid_.dim_sizes();
    vtr::NdMatrix<std::unordered_set<PackMoleculeId>, 3> unclustered_tile_molecules({layer_num, width, height});
    std::vector<PackMoleculeId> unclustered_blocks;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PackMoleculeId mol_id = ap_netlist_.block_molecule(blk_id);
        if (cluster_legalizer.is_mol_clustered(mol_id))
            continue;
        t_physical_tile_loc tile_loc = mol_desired_physical_tile_loc[mol_id];
        unclustered_tile_molecules[tile_loc.layer_num][tile_loc.x][tile_loc.y].insert(mol_id);
        unclustered_blocks.push_back(mol_id);
    }

    // Sort unclustered blocks by highest external input pins.
    std::sort(unclustered_blocks.begin(), unclustered_blocks.end(),
              [this](const PackMoleculeId& a, const PackMoleculeId& b) {
                  int ext_pins_a = prepacker_.calc_molecule_stats(a, atom_netlist_, arch_.models).num_used_ext_inputs;
                  int ext_pins_b = prepacker_.calc_molecule_stats(b, atom_netlist_, arch_.models).num_used_ext_inputs;
                  return ext_pins_a > ext_pins_b;
              });

    std::unordered_set<LegalizationClusterId> created_clusters;
    for (PackMoleculeId seed_mol_id : unclustered_blocks) {
        if (cluster_legalizer.is_mol_clustered(seed_mol_id))
            continue;

        // Start the new cluster with seed molecule using full strategy.
        // Note: This could waste time vs. using the fast strategy first and falling back
        // to full, but currently orphan clustering doesn't take that long as few molecules are clustered.
        cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);
        LegalizationClusterId cluster_id = create_new_cluster(seed_mol_id, prepacker_, cluster_legalizer, primitive_candidate_block_types);
        created_clusters.insert(cluster_id);

        // Get the physical tile location of the current molecules and delete
        // the seed molecule from unclustered search data.
        t_physical_tile_loc seed_tile_loc = mol_desired_physical_tile_loc[seed_mol_id];
        unclustered_tile_molecules[seed_tile_loc.layer_num][seed_tile_loc.x][seed_tile_loc.y].erase(seed_mol_id);

        // Keep track of the visited tile locations.
        vtr::NdMatrix<bool, 3> visited({layer_num, width, height}, false);
        std::queue<t_physical_tile_loc> loc_queue;
        loc_queue.push(seed_tile_loc);

        while (!loc_queue.empty()) {
            // Get the first location and try to add molecules in that tile to cluster created with seed molecule.
            t_physical_tile_loc current_tile_loc = loc_queue.front();
            loc_queue.pop();

            // Skip this location if it is already visited.
            if (visited[current_tile_loc.layer_num][current_tile_loc.x][current_tile_loc.y])
                continue;
            visited[current_tile_loc.layer_num][current_tile_loc.x][current_tile_loc.y] = true;

            // Skip this location if it is out of our range. This will stop expanding beyond scope.
            int distance = std::abs(seed_tile_loc.x - current_tile_loc.x) + std::abs(seed_tile_loc.y - current_tile_loc.y) + std::abs(seed_tile_loc.layer_num - current_tile_loc.layer_num);
            if (distance > search_radius)
                continue;

            // Try to add each unclustered molecule in that tile to the current cluster.
            std::unordered_set<PackMoleculeId>& tile_molecules = unclustered_tile_molecules[current_tile_loc.layer_num][current_tile_loc.x][current_tile_loc.y];
            for (auto it = tile_molecules.begin(); it != tile_molecules.end();) {
                if (!cluster_legalizer.is_molecule_compatible(*it, cluster_id)) {
                    ++it;
                    continue;
                }
                if (cluster_legalizer.add_mol_to_cluster(*it, cluster_id) == e_block_pack_status::BLK_PASSED) {
                    // If added, remove from unclustered spatial data.
                    it = tile_molecules.erase(it);
                } else {
                    ++it;
                }
            }

            // Push the neighbor tile locations onto queue (in the same layer).
            // This will push the neighbors left, right, above, and below the current
            // location. The code above will check if these locations are already
            // been visited or in our search radius.
            if (current_tile_loc.x > 0) {
                t_physical_tile_loc new_tile_loc = t_physical_tile_loc(current_tile_loc.x - 1,
                                                                       current_tile_loc.y,
                                                                       current_tile_loc.layer_num);
                loc_queue.push(new_tile_loc);
            }
            if (current_tile_loc.x < (int)width - 1) {
                t_physical_tile_loc new_tile_loc = t_physical_tile_loc(current_tile_loc.x + 1,
                                                                       current_tile_loc.y,
                                                                       current_tile_loc.layer_num);
                loc_queue.push(new_tile_loc);
            }
            if (current_tile_loc.y > 0) {
                t_physical_tile_loc new_tile_loc = t_physical_tile_loc(current_tile_loc.x,
                                                                       current_tile_loc.y - 1,
                                                                       current_tile_loc.layer_num);
                loc_queue.push(new_tile_loc);
            }
            if (current_tile_loc.y < (int)height - 1) {
                t_physical_tile_loc new_tile_loc = t_physical_tile_loc(current_tile_loc.x,
                                                                       current_tile_loc.y + 1,
                                                                       current_tile_loc.layer_num);
                loc_queue.push(new_tile_loc);
            }
        }
        // Clean the new created cluster to avoid memory footprint increase.
        cluster_legalizer.clean_cluster(cluster_id);
    }
    return created_clusters;
}

void FlatRecon::report_clustering_summary(ClusterLegalizer& cluster_legalizer,
                                          std::unordered_set<PackMoleculeId>& neighbor_pass_molecules,
                                          std::unordered_set<LegalizationClusterId>& orphan_window_clusters) {
    // Define stat collection variables: key is a block_type string and value
    // is the number of clusters of that type created.
    std::unordered_map<std::string, size_t> cluster_type_count_self_pass,
        cluster_type_count_orphan_window_pass;
    size_t num_of_mols_clustered_in_self_pass = 0,
           num_of_mols_clustered_in_orphan_window_pass = 0;
    size_t num_of_clusters_in_self_pass = 0,
           num_of_clusters_in_orphan_window_pass = 0;

    // Collect statistics.
    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        if (!cluster_id.is_valid()) continue;

        t_logical_block_type_ptr block_type = cluster_legalizer.get_cluster_type(cluster_id);
        std::string block_name = block_type->name;

        if (orphan_window_clusters.count(cluster_id)) {
            num_of_clusters_in_orphan_window_pass++;
            num_of_mols_clustered_in_orphan_window_pass += cluster_legalizer.get_cluster_molecules(cluster_id).size();
            cluster_type_count_orphan_window_pass[block_name]++;
        } else {
            num_of_clusters_in_self_pass++;
            num_of_mols_clustered_in_self_pass += cluster_legalizer.get_cluster_molecules(cluster_id).size();
            cluster_type_count_self_pass[block_name]++;
        }
    }

    // Note: neighbor-pass molecules were initially counted in 'self' because they
    // joined clusters created in the self pass. To report disjoint pass totals,
    // remove them from the self-pass count here.
    size_t num_of_mols_clustered_in_neighbor_pass = neighbor_pass_molecules.size();
    num_of_mols_clustered_in_self_pass -= num_of_mols_clustered_in_neighbor_pass;

    size_t total_mols_clustered = num_of_mols_clustered_in_self_pass + num_of_mols_clustered_in_neighbor_pass + num_of_mols_clustered_in_orphan_window_pass;

    // Report clustering summary. If there are any type with non-zero clusters,
    // their individual cluster number will also be printed.
    VTR_LOG("----------------------------------------------------------------\n");
    VTR_LOG("                       Clustering Summary                       \n");
    VTR_LOG("----------------------------------------------------------------\n");
    VTR_LOG("Clusters created (Self Clustering)                       : %zu\n", num_of_clusters_in_self_pass);
    for (const auto& [block_name, count] : cluster_type_count_self_pass) {
        VTR_LOG("    %-10s : %zu\n", block_name.c_str(), count);
    }
    VTR_LOG("Clusters created (Orphan Window Clustering)              : %zu\n", num_of_clusters_in_orphan_window_pass);
    for (const auto& [block_name, count] : cluster_type_count_orphan_window_pass) {
        VTR_LOG("    %-10s : %zu\n", block_name.c_str(), count);
    }
    VTR_LOG("Total clusters                                           : %zu\n", num_of_clusters_in_self_pass + num_of_clusters_in_orphan_window_pass);
    VTR_LOG("Percent of clusters created in Orphan Window Clustering  : %.2f%%\n",
            100.0f * static_cast<float>(num_of_clusters_in_orphan_window_pass) / static_cast<float>(num_of_clusters_in_self_pass + num_of_clusters_in_orphan_window_pass));

    VTR_LOG("Molecules clustered in each stage breakdown  : (Total of %zu)\n", total_mols_clustered);
    VTR_LOG("\tSelf Clustering          (%f)  : %zu\n ",
            static_cast<float>(num_of_mols_clustered_in_self_pass) / static_cast<float>(total_mols_clustered), num_of_mols_clustered_in_self_pass);
    VTR_LOG("\tNeighbor Clustering      (%f)  : %zu\n",
            static_cast<float>(num_of_mols_clustered_in_neighbor_pass) / static_cast<float>(total_mols_clustered), num_of_mols_clustered_in_neighbor_pass);
    VTR_LOG("\tOrphan Window Clustering (%f)  : %zu\n",
            static_cast<float>(num_of_mols_clustered_in_orphan_window_pass) / static_cast<float>(total_mols_clustered), num_of_mols_clustered_in_orphan_window_pass);
    VTR_LOG("----------------------------------------------------------------\n\n");
}

void FlatRecon::create_clusters(ClusterLegalizer& cluster_legalizer,
                                const PartialPlacement& p_placement) {
    vtr::ScopedStartFinishTimer creating_clusters("Creating Clusters");

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    VTR_LOG("Device (width, height): (%zu,%zu)\n", device_grid.width(), device_grid.height());

    // Sort the blocks according to their used external pin numbers while
    // prioritizing the long carry chain molecules respectively. Then,
    // group the sorted blocks by each tile.
    auto tile_blocks = sort_and_group_blocks_by_tile(p_placement);

    // Initialize the tile clusters matrix to be updated in reconstruction pass
    // and to be used in neighbor pass.
    auto [layers, width, height] = device_grid_.dim_sizes();
    tile_clusters_matrix = vtr::NdMatrix<std::unordered_set<LegalizationClusterId>, 3>({layers, width, height});

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    // Perform self clustering pass.
    self_clustering(cluster_legalizer,
                    device_grid,
                    primitive_candidate_block_types,
                    tile_blocks);

    // Perform neighbor clustering pass.
    std::unordered_set<PackMoleculeId> neighbor_pass_molecules = neighbor_clustering(cluster_legalizer,
                                                                                     primitive_candidate_block_types);

    // Perform orphan window clustering pass.
    // We retry orphan-window clustering with progressively larger Manhattan radii,
    // trading fewer clusters for higher displacement. Radius 8 was chosen
    // empirically as a good starting point (low displacement, reasonable cluster
    // count). If those clusters still don't fit the device, we retry with 16, and
    // finally with the whole device.
    std::vector<int> orphan_window_search_radii = {8, 16, static_cast<int>(device_grid.width() + device_grid.height())};
    bool fits_on_device = false;
    std::unordered_set<LegalizationClusterId> orphan_window_clusters;
    for (int orphan_window_search_radius : orphan_window_search_radii) {
        orphan_window_clusters = orphan_window_clustering(cluster_legalizer,
                                                          primitive_candidate_block_types,
                                                          orphan_window_search_radius);

        // Count used instances per block type.
        std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;
        for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
            if (!cluster_id.is_valid())
                continue;
            t_logical_block_type_ptr cluster_type = cluster_legalizer.get_cluster_type(cluster_id);
            num_used_type_instances[cluster_type]++;
        }

        std::map<t_logical_block_type_ptr, float> block_type_utils;
        fits_on_device = try_size_device_grid(arch_,
                                              num_used_type_instances,
                                              block_type_utils,
                                              vpr_setup_.PackerOpts.target_device_utilization,
                                              vpr_setup_.PackerOpts.device_layout);
        // Exit if clusters fit on device.
        if (fits_on_device)
            break;

        // Destroy the orphan window clusters to recreate with bigger search radius.
        VTR_LOG("Clusters did not fit on device with orphan window search radius of %d.\n", orphan_window_search_radius);
        for (LegalizationClusterId cluster_id : orphan_window_clusters) {
            if (!cluster_id.is_valid())
                continue;
            cluster_legalizer.destroy_cluster(cluster_id);
        }
        orphan_window_clusters.clear();
    }

    if (!fits_on_device) {
        VPR_FATAL_ERROR(VPR_ERROR_AP, "Created clusters could not fit on device.");
    }

    // Report the clustering summary.
    report_clustering_summary(cluster_legalizer,
                              neighbor_pass_molecules,
                              orphan_window_clusters);

    // Check and output the clustering.
    cluster_legalizer.compress();
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(cluster_legalizer, vpr_setup_.PackerOpts, is_clock, &arch_);

    // Clear the data structures that uses LegalizationClusterIds
    // since compress has invalidated them.
    loc_to_cluster_id_placed.clear();
    cluster_locs.clear();
    tile_clusters_matrix.clear();

    // Reset the cluster legalizer. This is required to load the packing.
    cluster_legalizer.reset();
    // Regenerate the clustered netlist from the file generated previously.
    // FIXME: This writing and loading from a file is wasteful. Should generate
    //        the clusters directly from the cluster legalizer.
    vpr_load_packing(vpr_setup_, arch_);

    // Verify the packing
    check_netlist(vpr_setup_.PackerOpts.pack_verbosity);
    writeClusteredNetlistStats(vpr_setup_.FileNameOpts.write_block_usage);
}

void FlatRecon::place_clusters(const PartialPlacement& p_placement) {
    // Setup the global variables for placement.
    g_vpr_ctx.mutable_placement().init_placement_context(vpr_setup_.PlacerOpts, arch_.directs);
    g_vpr_ctx.mutable_floorplanning().update_floorplanning_context_pre_place(*g_vpr_ctx.placement().place_macros);

    // The placement will be stored in the global block loc registry.
    BlkLocRegistry& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();

    // Create the noc cost handler used in the initial placer.
    std::optional<NocCostHandler> noc_cost_handler;
    if (vpr_setup_.NocOpts.noc)
        noc_cost_handler.emplace(blk_loc_registry.block_locs());

    // Create the RNG container for the initial placer.
    vtr::RngContainer rng(vpr_setup_.PlacerOpts.seed);

    // The partial placement has been set by the GP (if using internal VTR AP),
    // or by reading in the flat placement file. Cast / copy it to the flat
    // placement data structures so we can always use them.
    FlatPlacementInfo flat_placement_info(atom_netlist_);
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        PackMoleculeId mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& mol = prepacker_.get_molecule(mol_id);
        for (AtomBlockId atom_blk_id : mol.atom_block_ids) {
            if (!atom_blk_id.is_valid())
                continue;
            flat_placement_info.blk_x_pos[atom_blk_id] = p_placement.block_x_locs[ap_blk_id];
            flat_placement_info.blk_y_pos[atom_blk_id] = p_placement.block_y_locs[ap_blk_id];
            flat_placement_info.blk_layer[atom_blk_id] = p_placement.block_layer_nums[ap_blk_id];
            flat_placement_info.blk_sub_tile[atom_blk_id] = p_placement.block_sub_tiles[ap_blk_id];
        }
    }

    // Run the initial placer on the clusters created.
    // TODO: Currently, the way initial placer sorts the blocks to place is aligned
    //       how self clustering passes the clusters created, so there is no need to explicitly
    //       prioritize these clusters. However, if it changes in time, the atoms clustered
    //       in neighbor pass and atoms misplaced might not match exactly. It might be safer
    //       to write FlatRecon's own placer in that case.
    initial_placement(vpr_setup_.PlacerOpts,
                      vpr_setup_.PlacerOpts.constraints_file.c_str(),
                      vpr_setup_.NocOpts,
                      blk_loc_registry,
                      *g_vpr_ctx.placement().place_macros,
                      noc_cost_handler,
                      flat_placement_info,
                      rng);

    // Log some information on how good the reconstruction was.
    log_flat_placement_reconstruction_info(flat_placement_info,
                                           blk_loc_registry.block_locs(),
                                           g_vpr_ctx.clustering().atoms_lookup,
                                           g_vpr_ctx.atom().lookup(),
                                           atom_netlist_,
                                           g_vpr_ctx.clustering().clb_nlist);

    // Verify that the placement is valid for the VTR flow.
    unsigned num_errors = verify_placement(blk_loc_registry,
                                           *g_vpr_ctx.placement().place_macros,
                                           g_vpr_ctx.clustering().clb_nlist,
                                           g_vpr_ctx.device().grid,
                                           g_vpr_ctx.floorplanning().cluster_constraints);
    if (num_errors != 0) {
        VPR_ERROR(VPR_ERROR_AP,
                  "\nCompleted placement consistency check, %d errors found.\n"
                  "Aborting program.\n",
                  num_errors);
    }

    // Synchronize the pins in the clusters after placement.
    post_place_sync();
}

void FlatRecon::legalize(const PartialPlacement& p_placement) {
    // Start a scoped timer for the Full Legalizer stage.
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Full Legalizer");

    // The target external pin utilization is set to 1.0 to avoid over-restricting
    // reconstruction due to conservative pin feasibility. The SKIP_INTRA_LB_ROUTE
    // strategy speeds up reconstruction by skipping intra-LB routing checks.
    std::vector<std::string> target_ext_pin_util = {"1.0"};
    t_pack_high_fanout_thresholds high_fanout_thresholds(vpr_setup_.PackerOpts.high_fanout_threshold);

    ClusterLegalizer cluster_legalizer(
        atom_netlist_,
        prepacker_,
        vpr_setup_.PackerRRGraph,
        target_ext_pin_util,
        high_fanout_thresholds,
        ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
        vpr_setup_.PackerOpts.enable_pin_feasibility_filter,
        arch_.models,
        vpr_setup_.PackerOpts.pack_verbosity);

    // Perform clustering using partial placement.
    create_clusters(cluster_legalizer, p_placement);

    // Verify that the clustering created by the full legalizer is valid.
    unsigned num_clustering_errors = verify_clustering(g_vpr_ctx);
    if (num_clustering_errors == 0) {
        VTR_LOG("Completed clustering consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_AP,
                  "Completed placement consistency check, %u errors found.\n"
                  "Aborting program.\n",
                  num_clustering_errors);
    }

    // Perform the initial placement on created clusters.
    place_clusters(p_placement);
    update_drawing_data_structures();
}

void NaiveFullLegalizer::create_clusters(const PartialPlacement& p_placement) {
    // PACKING:
    // Initialize the cluster legalizer (Packing)
    // FIXME: The legalization strategy is currently set to full. Should handle
    //        this better to make it faster.
    t_pack_high_fanout_thresholds high_fanout_thresholds(vpr_setup_.PackerOpts.high_fanout_threshold);
    ClusterLegalizer cluster_legalizer(atom_netlist_,
                                       prepacker_,
                                       vpr_setup_.PackerRRGraph,
                                       vpr_setup_.PackerOpts.target_external_pin_util,
                                       high_fanout_thresholds,
                                       ClusterLegalizationStrategy::FULL,
                                       vpr_setup_.PackerOpts.enable_pin_feasibility_filter,
                                       arch_.models,
                                       vpr_setup_.PackerOpts.pack_verbosity);
    // Create clusters for each tile.
    //  Start by giving each root tile a unique ID.
    size_t grid_width = device_grid_.width();
    size_t grid_height = device_grid_.height();
    vtr::NdMatrix<DeviceTileId, 2> tile_grid({grid_width, grid_height});
    size_t num_device_tiles = 0;
    for (size_t x = 0; x < grid_width; x++) {
        for (size_t y = 0; y < grid_height; y++) {
            // Ignoring 3D placement for now.
            t_physical_tile_loc tile_loc(x, y, 0);
            // Ignore non-root locations
            size_t width_offset = device_grid_.get_width_offset(tile_loc);
            size_t height_offset = device_grid_.get_height_offset(tile_loc);
            if (width_offset != 0 || height_offset != 0) {
                tile_grid[x][y] = tile_grid[x - width_offset][y - height_offset];
                continue;
            }
            tile_grid[x][y] = DeviceTileId(num_device_tiles);
            num_device_tiles++;
        }
    }
    //  Next, collect the AP blocks which will go into each root tile
    VTR_ASSERT_SAFE(p_placement.verify_locs(ap_netlist_, grid_width, grid_height));
    vtr::vector<DeviceTileId, std::vector<APBlockId>> blocks_in_tiles(num_device_tiles);
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        // FIXME: Add these conversions to the PartialPlacement class.
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(ap_blk_id);
        VTR_ASSERT(p_placement.block_layer_nums[ap_blk_id] == 0);
        DeviceTileId tile_id = tile_grid[tile_loc.x][tile_loc.y];
        blocks_in_tiles[tile_id].push_back(ap_blk_id);
    }
    //  Create the legalized clusters per tile.
    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();
    for (size_t tile_id_idx = 0; tile_id_idx < num_device_tiles; tile_id_idx++) {
        DeviceTileId tile_id = DeviceTileId(tile_id_idx);
        // Create the molecule list
        std::list<PackMoleculeId> mol_list;
        for (APBlockId ap_blk_id : blocks_in_tiles[tile_id]) {
            mol_list.push_back(ap_netlist_.block_molecule(ap_blk_id));
        }
        // Clustering algorithm: Create clusters one at a time.
        while (!mol_list.empty()) {
            // Arbitrarily choose the first molecule as a seed molecule.
            PackMoleculeId seed_mol_id = mol_list.front();
            mol_list.pop_front();
            // Use the seed molecule to create a cluster for this tile.
            LegalizationClusterId new_cluster_id = create_new_cluster(seed_mol_id, prepacker_, cluster_legalizer, primitive_candidate_block_types);
            // Insert all molecules that you can into the cluster.
            // NOTE: If the mol_list was somehow sorted, we can just stop at
            //       first failure!
            auto it = mol_list.begin();
            while (it != mol_list.end()) {
                PackMoleculeId mol_id = *it;
                if (!cluster_legalizer.is_molecule_compatible(mol_id, new_cluster_id)) {
                    ++it;
                    continue;
                }
                // Try to insert it. If successful, remove from list.
                e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(mol_id, new_cluster_id);
                if (pack_status == e_block_pack_status::BLK_PASSED) {
                    it = mol_list.erase(it);
                } else {
                    ++it;
                }
            }
            // Once all molecules have been inserted, clean the cluster.
            cluster_legalizer.clean_cluster(new_cluster_id);
        }
    }

    // Check and output the clustering.
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(cluster_legalizer, vpr_setup_.PackerOpts, is_clock, &arch_);
    // Reset the cluster legalizer. This is required to load the packing.
    cluster_legalizer.reset();
    // Regenerate the clustered netlist from the file generated previously.
    // FIXME: This writing and loading from a file is wasteful. Should generate
    //        the clusters directly from the cluster legalizer.
    vpr_load_packing(vpr_setup_, arch_);
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Verify the packing and print some info
    check_netlist(vpr_setup_.PackerOpts.pack_verbosity);
    writeClusteredNetlistStats(vpr_setup_.FileNameOpts.write_block_usage);
    print_pb_type_count(clb_nlist);
}

void NaiveFullLegalizer::place_clusters(const ClusteredNetlist& clb_nlist,
                                        const PlaceMacros& place_macros,
                                        const PartialPlacement& p_placement) {
    // PLACING:
    // Create a lookup from the AtomBlockId to the APBlockId
    vtr::vector<AtomBlockId, APBlockId> atom_to_ap_block(atom_netlist_.blocks().size());
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        PackMoleculeId blk_mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& blk_mol = prepacker_.get_molecule(blk_mol_id);
        for (AtomBlockId atom_blk_id : blk_mol.atom_block_ids) {
            // See issue #2791, some of the atom_block_ids may be invalid. They
            // can safely be ignored.
            if (!atom_blk_id.is_valid())
                continue;
            // Ensure that this block is not in any other AP block. That would
            // be weird.
            VTR_ASSERT(!atom_to_ap_block[atom_blk_id].is_valid());
            atom_to_ap_block[atom_blk_id] = ap_blk_id;
        }
    }
    // Move the clusters to where they want to be first.
    // TODO: The fixed clusters should probably be moved first for legality
    //       reasons.
    APClusterPlacer ap_cluster_placer(place_macros, vpr_setup_.PlacerOpts.constraints_file.c_str());
    std::vector<ClusterBlockId> unplaced_clusters;
    for (ClusterBlockId cluster_blk_id : clb_nlist.blocks()) {
        // Assume that the cluster will always want to be placed wherever the
        // first atom in the cluster wants to be placed.
        // FIXME: This assumption does not always hold! Will need to unify the
        //        cluster legalizer and the clustered netlist!
        const std::unordered_set<AtomBlockId>& atoms_in_cluster = g_vpr_ctx.clustering().atoms_lookup[cluster_blk_id];
        VTR_ASSERT(atoms_in_cluster.size() > 0);
        AtomBlockId first_atom_blk = *atoms_in_cluster.begin();
        APBlockId first_ap_blk = atom_to_ap_block[first_atom_blk];
        size_t blk_sub_tile = p_placement.block_sub_tiles[first_ap_blk];
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(first_ap_blk);
        bool placed = ap_cluster_placer.place_cluster(cluster_blk_id, tile_loc, blk_sub_tile);
        if (placed)
            continue;

        // Add to list of unplaced clusters.
        unplaced_clusters.push_back(cluster_blk_id);
    }

    // Any clusters that were not placed previously are exhaustively placed.
    for (ClusterBlockId clb_blk_id : unplaced_clusters) {
        bool success = ap_cluster_placer.exhaustively_place_cluster(clb_blk_id);
        if (!success) {
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unable to find valid place for cluster in AP placement!");
        }
    }

    // Print some statistics about what happened here. This will be useful to
    // improve other algorithms.
    VTR_LOG("Number of clusters which needed to be moved: %zu\n", unplaced_clusters.size());

    // TODO: Print a breakdown per block type. We may find that specific block
    //       types are always conflicting.

    // FIXME: Allocate and load moveable blocks?
    //      - This may be needed to perform SA. Not needed right now.
}

void NaiveFullLegalizer::legalize(const PartialPlacement& p_placement) {
    // Create a scoped timer for the full legalizer
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Full Legalizer");

    // Pack the atoms into clusters based on the partial placement.
    create_clusters(p_placement);
    // Verify that the clustering created by the full legalizer is valid.
    unsigned num_clustering_errors = verify_clustering(g_vpr_ctx);
    if (num_clustering_errors == 0) {
        VTR_LOG("Completed clustering consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_AP,
                  "Completed placement consistency check, %u errors found.\n"
                  "Aborting program.\n",
                  num_clustering_errors);
    }
    // Get the clustering from the global context.
    // TODO: Eventually should be returned from the create_clusters method.
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Initialize the placement context.
    g_vpr_ctx.mutable_placement().init_placement_context(vpr_setup_.PlacerOpts,
                                                         arch_.directs);

    const PlaceMacros& place_macros = *g_vpr_ctx.placement().place_macros;

    // Update the floorplanning context with the macro information.
    g_vpr_ctx.mutable_floorplanning().update_floorplanning_context_pre_place(place_macros);

    // Place the clusters based on where the atoms want to be placed.
    place_clusters(clb_nlist, place_macros, p_placement);

    // Verify that the placement created by the full legalizer is valid.
    unsigned num_placement_errors = verify_placement(g_vpr_ctx);
    if (num_placement_errors == 0) {
        VTR_LOG("Completed placement consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_AP,
                  "Completed placement consistency check, %u errors found.\n"
                  "Aborting program.\n",
                  num_placement_errors);
    }

    // TODO: This was taken from vpr_api. Not sure why it is needed. Should be
    //       made part of the placement and verify placement should check for
    //       it.
    post_place_sync();
    update_drawing_data_structures();
}

void APPack::legalize(const PartialPlacement& p_placement) {
    // Create a scoped timer for the full legalizer
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Full Legalizer");

    // Convert the Partial Placement (APNetlist) to a flat placement (AtomNetlist).
    FlatPlacementInfo flat_placement_info(atom_netlist_);
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        PackMoleculeId mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& mol = prepacker_.get_molecule(mol_id);
        for (AtomBlockId atom_blk_id : mol.atom_block_ids) {
            if (!atom_blk_id.is_valid())
                continue;
            flat_placement_info.blk_x_pos[atom_blk_id] = p_placement.block_x_locs[ap_blk_id];
            flat_placement_info.blk_y_pos[atom_blk_id] = p_placement.block_y_locs[ap_blk_id];
            flat_placement_info.blk_layer[atom_blk_id] = p_placement.block_layer_nums[ap_blk_id];
            flat_placement_info.blk_sub_tile[atom_blk_id] = p_placement.block_sub_tiles[ap_blk_id];
        }
    }

    // Run the Packer stage with the flat placement as a hint.
    try_pack(vpr_setup_.PackerOpts,
             vpr_setup_.AnalysisOpts,
             vpr_setup_.APOpts,
             arch_,
             vpr_setup_.PackerRRGraph,
             prepacker_,
             pre_cluster_timing_manager_,
             flat_placement_info);

    // The Packer stores the clusters into a .net file. Load the packing file.
    // FIXME: This should be removed. Reading from a file is strange.
    vpr_load_packing(vpr_setup_, arch_);

    // Setup the global variables for placement.
    g_vpr_ctx.mutable_placement().init_placement_context(vpr_setup_.PlacerOpts, arch_.directs);
    g_vpr_ctx.mutable_floorplanning().update_floorplanning_context_pre_place(*g_vpr_ctx.placement().place_macros);

    // The placement will be stored in the global block loc registry.
    BlkLocRegistry& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();

    // Create the noc cost handler used in the initial placer.
    std::optional<NocCostHandler> noc_cost_handler;
    if (vpr_setup_.NocOpts.noc)
        noc_cost_handler.emplace(blk_loc_registry.block_locs());

    // Create the RNG container for the initial placer.
    vtr::RngContainer rng(vpr_setup_.PlacerOpts.seed);

    // Run the initial placer on the clusters created by the packer, using the
    // flat placement information from the global placer to guide where to place
    // the clusters.
    initial_placement(vpr_setup_.PlacerOpts,
                      vpr_setup_.PlacerOpts.constraints_file.c_str(),
                      vpr_setup_.NocOpts,
                      blk_loc_registry,
                      *g_vpr_ctx.placement().place_macros,
                      noc_cost_handler,
                      flat_placement_info,
                      rng);

    // Log some information on how good the reconstruction was.
    log_flat_placement_reconstruction_info(flat_placement_info,
                                           blk_loc_registry.block_locs(),
                                           g_vpr_ctx.clustering().atoms_lookup,
                                           g_vpr_ctx.atom().lookup(),
                                           atom_netlist_,
                                           g_vpr_ctx.clustering().clb_nlist);

    // Verify that the placement is valid for the VTR flow.
    unsigned num_errors = verify_placement(blk_loc_registry,
                                           *g_vpr_ctx.placement().place_macros,
                                           g_vpr_ctx.clustering().clb_nlist,
                                           g_vpr_ctx.device().grid,
                                           g_vpr_ctx.floorplanning().cluster_constraints);
    if (num_errors != 0) {
        VPR_ERROR(VPR_ERROR_AP,
                  "\nCompleted placement consistency check, %d errors found.\n"
                  "Aborting program.\n",
                  num_errors);
    }

    // Synchronize the pins in the clusters after placement.
    post_place_sync();
    update_drawing_data_structures();
}

void FullLegalizer::update_drawing_data_structures() {
#ifndef NO_GRAPHICS
    // update graphic resources incase of clustering changes
    if (get_draw_state_vars()) {
        get_draw_state_vars()->refresh_graphic_resources_after_cluster_change();
    }
#endif
}