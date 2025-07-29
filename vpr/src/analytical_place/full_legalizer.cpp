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
#include <unordered_set>
#include <vector>
#include <execution>

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
static t_logical_block_type_ptr get_molecule_logical_block_type(PackMoleculeId mol_id,
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

static bool try_size_device_grid(const t_arch& arch,
                                 const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
                                 std::map<t_logical_block_type_ptr, float>& type_util,
                                 float target_device_utilization,
                                 const std::string& device_layout_name) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    //Build the device
    auto grid = create_device_grid(device_layout_name, arch.grid_layouts, num_type_instances, target_device_utilization);

    /*
     *Report on the device
     */
    VTR_LOG("FPGA sized to %zu x %zu (%s)\n", grid.width(), grid.height(), grid.name().c_str());

    bool fits_on_device = true;

    float device_utilization = calculate_device_utilization(grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) continue;

        auto itr = num_type_instances.find(&type);
        if (itr == num_type_instances.end()) continue;

        float num_instances = itr->second;
        float util = 0.;

        float num_total_instances = 0.;
        for (const auto& equivalent_tile : type.equivalent_tiles) {
            num_total_instances += device_ctx.grid.num_instances(equivalent_tile, -1);
        }

        if (num_total_instances != 0) {
            util = num_instances / num_total_instances;
        }
        type_util[&type] = util;

        if (util > 1.) {
            fits_on_device = false;
        }
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type.name.c_str());
    }
    VTR_LOG("\n");

    return fits_on_device;
}

std::map<t_physical_tile_loc, std::vector<PackMoleculeId>>
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

    // Sort the blocks: molecules of a long chain should come first, then
    // sort by descending external input pin counts.
    std::sort(sorted_blocks.begin(), sorted_blocks.end(),
              [](const BlockInformation& a, const BlockInformation& b) {
                  if (a.is_long_chain != b.is_long_chain) {
                      return a.is_long_chain > b.is_long_chain; // Long chains first
                  } else {
                      return a.ext_inps > b.ext_inps; // Then external inputs
                  }
              });

    // Group the molecules by root tile. Any non-zero offset gets
    // pulled back to its root.
    std::map<t_physical_tile_loc, std::vector<PackMoleculeId>> tile_blocks;
    for (const auto& [mol_id, ext_pins, is_long_chain, tile_loc] : sorted_blocks) {
        int width_offset = device_grid_.get_width_offset(tile_loc);
        int height_offset = device_grid_.get_height_offset(tile_loc);
        t_physical_tile_loc root_loc = {tile_loc.x - width_offset,
                                        tile_loc.y - height_offset,
                                        tile_loc.layer_num};
        tile_blocks[root_loc].push_back(mol_id);
    }

    return tile_blocks;
}

void FlatRecon::cluster_molecules_in_tile(const t_physical_tile_loc& tile_loc,
                                          const t_physical_tile_type_ptr& tile_type,
                                          const std::vector<PackMoleculeId>& tile_molecules,
                                          ClusterLegalizer& cluster_legalizer,
                                          const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                          std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>>& unclustered_blocks,
                                          std::unordered_map<LegalizationClusterId, t_pl_loc>& cluster_ids_to_check) {
    for (PackMoleculeId mol_id : tile_molecules) {
        // Get the block type for compatibility check.
        const auto block_type = get_molecule_logical_block_type(mol_id, prepacker_, primitive_candidate_block_types);

        // Try all subtiles in a single loop
        bool placed = false;
        for (int sub_tile = 0; sub_tile < tile_type->capacity; ++sub_tile) {
            const t_pl_loc loc{tile_loc.x, tile_loc.y, sub_tile, tile_loc.layer_num};
            auto cluster_it = loc_to_cluster_id_placed.find(loc);

            if (cluster_it != loc_to_cluster_id_placed.end()) {
                // Try adding to the existing cluster
                LegalizationClusterId cluster_id = cluster_it->second;
                if (!cluster_legalizer.is_molecule_compatible(mol_id, cluster_id))
                    continue;

                if (cluster_legalizer.add_mol_to_cluster(mol_id, cluster_id) == e_block_pack_status::BLK_PASSED) {
                    placed = true;
                    break;
                }
            } else if (is_tile_compatible(tile_type, block_type)) {
                // Create new cluster
                LegalizationClusterId new_id = create_new_cluster(mol_id, prepacker_, cluster_legalizer, primitive_candidate_block_types);
                cluster_ids_to_check[new_id] = loc;
                loc_to_cluster_id_placed[loc] = new_id;
                tile_loc_to_cluster_id_placed[{tile_loc.x, tile_loc.y, -1, tile_loc.layer_num}].push_back(new_id);
                placed = true;
                break;
            }
        }

        if (!placed) {
            unclustered_blocks.push_back({mol_id, tile_loc});
        }
    }
}

void FlatRecon::reconstruction_cluster_pass(ClusterLegalizer& cluster_legalizer,
                                            const DeviceGrid& device_grid,
                                            const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                            std::map<t_physical_tile_loc, std::vector<PackMoleculeId>>& tile_blocks,
                                            std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>>& unclustered_blocks) {
    vtr::ScopedStartFinishTimer reconstruction_pass_clustering("Reconstruction Pass Clustering");
    for (const auto& [key, value] : tile_blocks) {
        // Get tile and molecules aimed to be placed in that tile
        // TODO: Some of these data might be moved into cluster_molecules_in_tile
        t_physical_tile_loc tile_loc = key;
        std::vector<PackMoleculeId> tile_molecules = value;
        const t_physical_tile_type_ptr tile_type = device_grid.get_physical_type(tile_loc);

        // Try to create clusters with fast strategy checking the compatibility
        // with tile and its capacity. Store the cluster ids to check their legality.
        std::unordered_map<LegalizationClusterId, t_pl_loc> cluster_ids_to_check;
        cluster_molecules_in_tile(tile_loc,
                                  tile_type,
                                  tile_molecules,
                                  cluster_legalizer,
                                  primitive_candidate_block_types,
                                  unclustered_blocks,
                                  cluster_ids_to_check);

        // Check legality of clusters created with fast pass. Store the
        // illegal cluster molecules for full strategy pass.
        std::vector<PackMoleculeId> illegal_cluster_mols;
        for (auto it = cluster_ids_to_check.begin(); it != cluster_ids_to_check.end();) {
            if (!cluster_legalizer.check_cluster_legality(it->first)) {
                for (PackMoleculeId mol_id : cluster_legalizer.get_cluster_molecules(it->first)) {
                    illegal_cluster_mols.push_back(mol_id);
                }
                // Erase related data of illegal cluster
                loc_to_cluster_id_placed.erase(it->second);
                cluster_legalizer.destroy_cluster(it->first);
                // Erase from the new map as well.
                auto& vec = tile_loc_to_cluster_id_placed[{it->second.x, it->second.y, -1, it->second.layer}];
                vec.erase(std::remove(vec.begin(), vec.end(), it->first), vec.end());
                if (vec.empty()) {
                    tile_loc_to_cluster_id_placed.erase({it->second.x, it->second.y, -1, it->second.layer});
                }
                it = cluster_ids_to_check.erase(it);
            } else {
                ++it;
            }
        }
        // Set the legalization strategy to full and try to cluster the
        // unclustered molecules in same tile again.
        cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);
        cluster_molecules_in_tile(tile_loc,
                                  tile_type,
                                  illegal_cluster_mols,
                                  cluster_legalizer,
                                  primitive_candidate_block_types,
                                  unclustered_blocks,
                                  cluster_ids_to_check);

        // Clean all clusters created in that tile not to increase memory footprint.
        for (const auto& [cluster_id, loc] : cluster_ids_to_check) {
            cluster_legalizer.clean_cluster(cluster_id);
        }
        // Set the legalization strategy to fast check again for next the tile
        cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);
    }
    VTR_LOG("Unclustered molecules after reconstruction: %zu / %zu .\n", unclustered_blocks.size(), ap_netlist_.blocks().size());
}

void FlatRecon::neighbor_cluster_pass(ClusterLegalizer& cluster_legalizer,
                                      const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                      std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>>& unclustered_blocks,
                                      int search_radius) {
    std::string timer_label = "Neighbor Pass Clustering with search radius " + std::to_string(search_radius);
    vtr::ScopedStartFinishTimer neighbor_pass_clustering(timer_label);
    // For each unclustered molecule
    while (!unclustered_blocks.empty()) {
        // Pick the first seed molecule and erase it from unclustered data
        auto seed_it = unclustered_blocks.begin();
        PackMoleculeId seed_mol = seed_it->first;
        t_physical_tile_loc seed_loc = seed_it->second;
        unclustered_blocks.erase(seed_it);

        // Get molecues in our search radius.
        std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>> neighbor_molecules;
        for (const auto& [mol_id, loc] : unclustered_blocks) {
            int distance = std::abs(seed_loc.x - loc.x) + std::abs(seed_loc.y - loc.y) + std::abs(seed_loc.layer_num - loc.layer_num);
            if (distance <= search_radius) {
                neighbor_molecules.emplace_back(mol_id, loc);
            }
        }

        // Sort by Manhattan distance to seed.
        std::sort(neighbor_molecules.begin(), neighbor_molecules.end(),
                  [&](const auto& a, const auto& b) {
                      int da = std::abs(seed_loc.x - a.second.x) + std::abs(seed_loc.y - a.second.y) + std::abs(seed_loc.layer_num - a.second.layer_num);
                      int db = std::abs(seed_loc.x - b.second.x) + std::abs(seed_loc.y - b.second.y) + std::abs(seed_loc.layer_num - b.second.layer_num);
                      return da < db;
                  });

        // Try to cluster them
        LegalizationClusterId cluster_id = create_new_cluster(seed_mol, prepacker_, cluster_legalizer, primitive_candidate_block_types);
        neighbor_pass_clusters.push_back(cluster_id);
        for (const auto& [neighbor_mol, neighbor_loc] : neighbor_molecules) {
            if (!cluster_legalizer.is_molecule_compatible(neighbor_mol, cluster_id))
                continue;
            if (cluster_legalizer.add_mol_to_cluster(neighbor_mol, cluster_id) == e_block_pack_status::BLK_PASSED) {
                // If added, remove from unclustered data
                unclustered_blocks.erase(std::remove(unclustered_blocks.begin(), unclustered_blocks.end(), std::pair(neighbor_mol, neighbor_loc)), unclustered_blocks.end());
            }
        }
    }
}

ClusteredNetlist FlatRecon::create_clusters(ClusterLegalizer& cluster_legalizer,
                                            const PartialPlacement& p_placement) {
    vtr::ScopedStartFinishTimer creating_clusters("Creating Clusters");

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    VTR_LOG("Device (width, height): (%zu,%zu)\n", device_grid.width(), device_grid.height());

    // Sort the blocks according to their used external pin numbers while
    // prioritizing the long carry chain molecules respectively. Then,
    // group the sorted blocks by each tile.
    auto tile_blocks = sort_and_group_blocks_by_tile(p_placement);

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>> unclustered_blocks;

    reconstruction_cluster_pass(cluster_legalizer,
                                device_grid,
                                primitive_candidate_block_types,
                                tile_blocks,
                                unclustered_blocks);

    // Reconstruction pass clustering statistics collection
    std::unordered_map<std::string, size_t> cluster_type_count_first_pass;
    std::vector<LegalizationClusterId> first_pass_clusters;
    size_t total_atoms_in_first_pass_clusters = 0;
    size_t total_molecules_in_first_pass_clusters = 0;
    size_t total_clusters_in_first_pass = 0;
    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        if (!cluster_id.is_valid()) {
            continue;
        }
        total_clusters_in_first_pass++;
        first_pass_clusters.push_back(cluster_id);
        bool block_type_set = false;
        for (PackMoleculeId mol_id : cluster_legalizer.get_cluster_molecules(cluster_id)) {
            // We still need to iterate over the atom ids due to invalid ones.
            total_molecules_in_first_pass_clusters++;
            for (AtomBlockId atom_id : prepacker_.get_molecule(mol_id).atom_block_ids) {
                if (atom_id.is_valid()) {
                    total_atoms_in_first_pass_clusters++;
                }
            }
            // Set block type only once per cluster
            if (!block_type_set) {
                t_logical_block_type_ptr block_type = get_molecule_logical_block_type(mol_id, prepacker_, primitive_candidate_block_types);
                if (block_type) {
                    std::string block_name = block_type->name;
                    cluster_type_count_first_pass[block_name]++;
                    block_type_set = true;
                }
            }
        }
    }
    float avg_atom_number_in_first_pass =
        static_cast<float>(total_atoms_in_first_pass_clusters) / static_cast<float>(total_clusters_in_first_pass);
    float avg_mol_number_in_first_pass =
        static_cast<float>(total_molecules_in_first_pass_clusters) / static_cast<float>(total_clusters_in_first_pass);

    // Save unclustered blocks just in case we fall back
    unclustered_blocks_saved = unclustered_blocks;
    cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);

    // First attempt: Try neighbor clustering with radius 8
    neighbor_cluster_pass(cluster_legalizer,
                        primitive_candidate_block_types,
                        unclustered_blocks,
                        /*neighbor_search_radius=*/8);

    // Count usage after first neighbor pass
    //  num_used_type_instances: A map used to save the number of used
    //                           instances from each logical block type.
    std::map<t_logical_block_type_ptr, size_t> num_used_type_instances_first_attempt;
    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        if (!cluster_id.is_valid())
            continue;
        t_logical_block_type_ptr cluster_type = cluster_legalizer.get_cluster_type(cluster_id);
        num_used_type_instances_first_attempt[cluster_type]++;
    }

    std::map<t_logical_block_type_ptr, float> block_type_utils_first_attempt;
    bool fits_on_device_first_attempt = try_size_device_grid(arch_,
                                                            num_used_type_instances_first_attempt,
                                                            block_type_utils_first_attempt,
                                                            vpr_setup_.PackerOpts.target_device_utilization,
                                                            vpr_setup_.PackerOpts.device_layout);

    if (fits_on_device_first_attempt) {
        VTR_LOG("Clusters fit on device with initial neighbor pass (radius 8). Skipping Join-with-Neighbor and extended Neighbor passes.\n");

        // Proceed to finalization (it should be empty naturally but we will make sure its empty for now)
        // Since unclustered_blocks is empty, next 2 pass will be passed over simly.
        unclustered_blocks.clear();
    } else{
        VTR_LOG("Clusters did not fit on device, going to join with neighbor pass.\n");
        
        // Otherwise, destroy those neighbor pass clusters and continue to original flow
        for (LegalizationClusterId cluster_id: neighbor_pass_clusters) {
            if (!cluster_id.is_valid())
                continue; // we cant destroy a already destoyed cluster
            cluster_legalizer.destroy_cluster(cluster_id);
        }
        neighbor_pass_clusters.clear();   

        unclustered_blocks = unclustered_blocks_saved;
    }

    


    // The molecules that could not go back in their original clusters.
    // (Hope there will be none of them)
    std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>> broken_molecules;

    // MAKING THE STRATEGY SPECULATIVE AT START
    cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);

    size_t total_molecules_in_join_with_neighbor = 0;
    VTR_LOG("Join with Neighbor Pass...\n");
    
    // For each unplaced block, get its neighboring clusters created.
    for (const auto& [molecule_id, loc] : unclustered_blocks) {
        t_logical_block_type_ptr block_type = get_molecule_logical_block_type(molecule_id, prepacker_, primitive_candidate_block_types);
        VTR_ASSERT(block_type && "We need a blocks type");

        std::string block_name = block_type->name;
        if (block_name == "io") {
            VTR_LOG("Skipping io molecule of id %zu\n", molecule_id);
            broken_molecules.push_back({molecule_id, loc});
            continue;
        }
        
        // Use molecule_id and tile_loc here
        // VTR_LOG("Molecule ID: %zu\n", size_t(molecule_id));
        // VTR_LOG("Tile Location: (x=%d, y=%d, layer=%d, subtile=%d)\n", loc.x, loc.y, loc.layer_num);
    
        // Get its 8-neighbouring clusters
        std::vector<t_pl_loc> neighbor_tile_locs = {
            {loc.x-1, loc.y-1, -1, loc.layer_num},
            {loc.x-1, loc.y,   -1, loc.layer_num},
            {loc.x-1, loc.y+1, -1, loc.layer_num},
            {loc.x,   loc.y-1, -1, loc.layer_num},
            {loc.x,   loc.y+1, -1, loc.layer_num},
            {loc.x+1, loc.y-1, -1, loc.layer_num},
            {loc.x+1, loc.y,   -1, loc.layer_num},
            {loc.x+1, loc.y+1, -1, loc.layer_num},
        };

        // Sort the neighbor tile locations according to average mol count in clusters in that tile.
        std::unordered_map<t_pl_loc, double> avg_mols_in_tile;
        for (t_pl_loc neighbor_tile_loc: neighbor_tile_locs) {    
            if (tile_loc_to_cluster_id_placed.find(neighbor_tile_loc) == tile_loc_to_cluster_id_placed.end()) 
                continue;

            size_t total_molecules_in_tile = 0;
            size_t total_clusters_in_tile = 0;
            for (LegalizationClusterId cluster_id: tile_loc_to_cluster_id_placed[neighbor_tile_loc]) {
                total_molecules_in_tile += cluster_legalizer.get_num_molecules_in_cluster(cluster_id);
                total_clusters_in_tile ++;
            }
            if (total_clusters_in_tile) {
                avg_mols_in_tile[neighbor_tile_loc] = double(total_molecules_in_tile) / total_clusters_in_tile;
            }
        }
        // Sort tile locations by increasing avg mol count
        std::vector<t_pl_loc> sorted_neighbor_tile_locs;
        for (const auto& [tile_loc, _] : avg_mols_in_tile) {
            sorted_neighbor_tile_locs.push_back(tile_loc);
        }

        std::sort(sorted_neighbor_tile_locs.begin(), sorted_neighbor_tile_locs.end(),
                [&](const t_pl_loc& a, const t_pl_loc& b) {
                    return avg_mols_in_tile[a] < avg_mols_in_tile[b];
                });


        bool fit_in_a_neighbor = false;

        std::vector<LegalizationClusterId> neighbor_clusters;
        for (t_pl_loc neighbor_tile_loc: sorted_neighbor_tile_locs) {
            if (fit_in_a_neighbor) {
                break;
            }
            
            
            if (tile_loc_to_cluster_id_placed.find(neighbor_tile_loc) == tile_loc_to_cluster_id_placed.end()) 
                continue;

            auto& clusters = tile_loc_to_cluster_id_placed[neighbor_tile_loc]; // alias for clarity
            for (auto it = clusters.begin(); it != clusters.end(); /* no ++ here */) {
                LegalizationClusterId cluster_id = *it;

                if (!cluster_id.is_valid()) {
                    ++it;
                    continue;
                }

                neighbor_clusters.push_back(cluster_id);
                size_t num_mols_in_cluster = cluster_legalizer.get_num_molecules_in_cluster(cluster_id);
                // VTR_LOG("\tNumber of molecules in neighbor cluster at (%d, %d, %d): %zu\n", 
                //             neighbor_tile_loc.x, neighbor_tile_loc.y, neighbor_tile_loc.layer, num_mols_in_cluster);

                std::vector<PackMoleculeId> cluster_molecules = cluster_legalizer.get_cluster_molecules(cluster_id);
                
                // Destroy the old cluster and update data structures accordingly
                // Note: We may not need to update the data structures since the cluster ids will become invalid.
                cluster_legalizer.destroy_cluster(cluster_id);
                first_pass_clusters.erase(find(first_pass_clusters.begin(), first_pass_clusters.end(), cluster_id));
                
                PackMoleculeId seed_mol = cluster_molecules[0];
                LegalizationClusterId new_cluster_id = create_new_cluster(seed_mol, prepacker_, cluster_legalizer, primitive_candidate_block_types); 
                first_pass_clusters.push_back(new_cluster_id);
                // Remove the first element used as seed.
                cluster_molecules.erase(cluster_molecules.begin());

                // Add old molecules to the new cluster.
                for (PackMoleculeId mol_id: cluster_molecules) {
                    if (!cluster_legalizer.is_molecule_compatible(mol_id, new_cluster_id)){
                        // VTR_LOG("A molecule could not go its old cluster!\n");
                        broken_molecules.push_back({mol_id, {neighbor_tile_loc.x, neighbor_tile_loc.y, neighbor_tile_loc.layer}});
                        continue;
                    }
                    if (cluster_legalizer.add_mol_to_cluster(mol_id, new_cluster_id) != e_block_pack_status::BLK_PASSED) {
                        // VTR_LOG("A molecule could not go its old cluster!\n");
                        broken_molecules.push_back({mol_id, {neighbor_tile_loc.x, neighbor_tile_loc.y, neighbor_tile_loc.layer}});
                    }
                }

                // Set Legalization Strategy to FULL
                cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);


                // Check Cluster Legality for older molecules
                if (!cluster_legalizer.check_cluster_legality(new_cluster_id)) {
                    // VTR_LOG("An illegal cluster detected!\n");
                    cluster_legalizer.destroy_cluster(new_cluster_id);
                    new_cluster_id = create_new_cluster(seed_mol, prepacker_, cluster_legalizer, primitive_candidate_block_types); 
                    for (PackMoleculeId mol_id: cluster_molecules) {
                        if (!cluster_legalizer.is_molecule_compatible(mol_id, new_cluster_id)){
                            // VTR_LOG("A molecule could not go its old cluster!\n");
                            broken_molecules.push_back({mol_id, {neighbor_tile_loc.x, neighbor_tile_loc.y, neighbor_tile_loc.layer}});
                            continue;
                        }
                        if (cluster_legalizer.add_mol_to_cluster(mol_id, new_cluster_id) != e_block_pack_status::BLK_PASSED) {
                            // VTR_LOG("A molecule could not go its old cluster!\n");
                            broken_molecules.push_back({mol_id, {neighbor_tile_loc.x, neighbor_tile_loc.y, neighbor_tile_loc.layer}});
                        }
                    }
                }

                // Lastly, try to add the objective molecule to that cluster.
                if (cluster_legalizer.is_molecule_compatible(molecule_id, new_cluster_id)){
                    if (cluster_legalizer.add_mol_to_cluster(molecule_id, new_cluster_id) == e_block_pack_status::BLK_PASSED) {
                        // VTR_LOG("An unclustered neighbor fit in a neighbor cluster!\n");
                        fit_in_a_neighbor = true;
                    }
                }

                // Clean the current cluster
                cluster_legalizer.clean_cluster(new_cluster_id);

                // Set strategy to SPECULATIVE AGAIN
                cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);

                //it = clusters.erase(it); // returns next valid iterator
                // Put the new cluster to old ones place and go to next one to process.
                *it = new_cluster_id;
                ++it;
            }
        }   

        if (!fit_in_a_neighbor) {
            // VTR_LOG("Current cluster could not put in any neighbor cluster!\n");
            broken_molecules.push_back({molecule_id, loc});
        } else {
            total_molecules_in_join_with_neighbor++;
        }
    }

    // use the broken blocks as unclustered blocks
    unclustered_blocks = broken_molecules;

    VTR_LOG("Unclusterd Blocks Before Starting Neighbor Pass: %zu\n", unclustered_blocks.size());

    // Save unclustered blocks
    unclustered_blocks_saved = unclustered_blocks;
    
    // Neighbor Search Radius Values to Try
    std::vector<int> neighbor_search_radius_vector = {8, 16, static_cast<int>(device_grid.width() + device_grid.height())};

    bool fits_on_device = false;
    // Try all radiuses untill clusters fit into device
    for (int neighbor_search_radius: neighbor_search_radius_vector) {
        cluster_legalizer.set_legalization_strategy(ClusterLegalizationStrategy::FULL);
        neighbor_cluster_pass(cluster_legalizer,
                              primitive_candidate_block_types,
                              unclustered_blocks,
                              neighbor_search_radius);
        
        //  num_used_type_instances: A map used to save the number of used
        //                           instances from each logical block type.
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
        // Exit if clusters fit on device
        if (fits_on_device)
            break;
        
        VTR_LOG("Clusters did not fit on device with neighbor search radius of %d.\n", neighbor_search_radius);

        for (LegalizationClusterId cluster_id: neighbor_pass_clusters) {
            if (!cluster_id.is_valid())
                continue; // we cant destroy a already destoyed cluster
            cluster_legalizer.destroy_cluster(cluster_id);
        }
        
        unclustered_blocks = unclustered_blocks_saved;
        neighbor_pass_clusters.clear();        
    }

    if (!fits_on_device) {
        VPR_FATAL_ERROR(VPR_ERROR_AP, "Clusters could not fit on device.");
    }

    // Neighbor pass clustering statistics collection
    std::unordered_map<std::string, size_t> cluster_type_count_second_pass;
    size_t total_atoms_in_second_pass_clusters = 0;
    size_t total_molecules_in_second_pass_clusters = 0;
    size_t total_clusters_in_second_pass = 0;
    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        if (!cluster_id.is_valid()) {
            continue;
        }
        if (std::find(first_pass_clusters.begin(), first_pass_clusters.end(), cluster_id) != first_pass_clusters.end())
            continue;
        total_clusters_in_second_pass++;
        bool block_type_set = false;
        for (PackMoleculeId mol_id : cluster_legalizer.get_cluster_molecules(cluster_id)) {
            total_molecules_in_second_pass_clusters++;
            // We still need to iterate over the atom ids due to invalid ones.
            for (AtomBlockId atom_blk_id : prepacker_.get_molecule(mol_id).atom_block_ids) {
                if (atom_blk_id.is_valid()) {
                    total_atoms_in_second_pass_clusters++;
                }
            }
            // Set block type only once per cluster
            if (!block_type_set) {
                t_logical_block_type_ptr block_type = get_molecule_logical_block_type(mol_id, prepacker_, primitive_candidate_block_types);
                if (block_type) {
                    std::string block_name = block_type->name;
                    cluster_type_count_second_pass[block_name]++;
                    block_type_set = true;
                }
            }
        }
    }
    float avg_atom_number_in_neighbor_pass =
        static_cast<float>(total_atoms_in_second_pass_clusters) / static_cast<float>(total_clusters_in_second_pass);
    float avg_mol_number_in_neighbor_pass =
        static_cast<float>(total_molecules_in_second_pass_clusters) / static_cast<float>(total_clusters_in_second_pass);

    // Report clustering summary. If there are any type with non-zero clusters,
    // their individual cluster number will also be printed.
    VTR_LOG("-------------------------------------------------------\n");
    VTR_LOG("                   Clustering Summary                  \n");
    VTR_LOG("-------------------------------------------------------\n");
    VTR_LOG("Clusters created (Reconstruction pass)          : %zu\n", total_clusters_in_first_pass);
    for (const auto& [block_name, count] : cluster_type_count_first_pass) {
        VTR_LOG("    %-10s : %zu\n", block_name.c_str(), count);
    }
    VTR_LOG("Clusters created (Neighbor pass)                : %zu\n", total_clusters_in_second_pass);
    for (const auto& [block_name, count] : cluster_type_count_second_pass) {
        VTR_LOG("    %-10s : %zu\n", block_name.c_str(), count);
    }
    VTR_LOG("Total clusters                                  : %zu\n", total_clusters_in_first_pass + total_clusters_in_second_pass);
    VTR_LOG("Avg atoms per cluster (Reconstruction pass)     : %.2f\n", avg_atom_number_in_first_pass);
    VTR_LOG("Avg atoms per cluster (Neighbor pass)           : %.2f\n", avg_atom_number_in_neighbor_pass);
    VTR_LOG("Avg molecules per cluster (Reconstruction pass) : %.2f\n", avg_mol_number_in_first_pass);
    VTR_LOG("Avg molecules per cluster (Neighbor pass)       : %.2f\n", avg_mol_number_in_neighbor_pass);
    VTR_LOG("Percent of atoms clustered in Neighbor pass     : %.2f%%\n",
            100.0f * static_cast<float>(total_atoms_in_second_pass_clusters) / static_cast<float>(total_atoms_in_first_pass_clusters + total_atoms_in_second_pass_clusters));
    VTR_LOG("Percent of molecules clustered in Neighbor pass : %.2f%%\n",
            100.0f * static_cast<float>(total_molecules_in_second_pass_clusters) / static_cast<float>(total_molecules_in_first_pass_clusters + total_molecules_in_second_pass_clusters));
    VTR_LOG("Percent of clusters created in Neighbor pass    : %.2f%%\n",
            100.0f * static_cast<float>(total_clusters_in_second_pass) / static_cast<float>(total_clusters_in_first_pass + total_clusters_in_second_pass));
    VTR_LOG("-------------------------------------------------------\n\n");

    size_t total_mols_clustered = total_molecules_in_first_pass_clusters + total_molecules_in_join_with_neighbor + total_molecules_in_second_pass_clusters;
    VTR_LOG("Molecules Clustered in each stage breakdown: (Total of %zu)\n", total_mols_clustered);
    VTR_LOG("\tNormal Pass: %zu (%f)\n ", total_molecules_in_first_pass_clusters,
                                          static_cast<float>(total_molecules_in_first_pass_clusters) / static_cast<float>(total_mols_clustered));
    VTR_LOG("\tJoin with Neighbor Pass: %zu (%f)\n", total_molecules_in_join_with_neighbor,
                                          static_cast<float>(total_molecules_in_join_with_neighbor) / static_cast<float>(total_mols_clustered));
    VTR_LOG("\tOrphan Region Reconstruction Pass: %zu (%f)\n", total_molecules_in_second_pass_clusters,
                                          static_cast<float>(total_molecules_in_second_pass_clusters) / static_cast<float>(total_mols_clustered));


    // Check and output the clustering.
    cluster_legalizer.compress();
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(cluster_legalizer, vpr_setup_.PackerOpts, is_clock, &arch_);

    // Reset the cluster legalizer. This is required to load the packing.
    cluster_legalizer.reset();
    // Regenerate the clustered netlist from the file generated previously.
    // FIXME: This writing and loading from a file is wasteful. Should generate
    //        the clusters directly from the cluster legalizer.
    vpr_load_packing(vpr_setup_, arch_);
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Verify the packing
    check_netlist(vpr_setup_.PackerOpts.pack_verbosity);
    writeClusteredNetlistStats(vpr_setup_.FileNameOpts.write_block_usage);

    return clb_nlist;
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

    // Run the initial placer on the clusters created by the packer, using the
    // flat placement information from the global placer or using the read flat
    // placement to guide where to place the clusters.
    // TODO: Currently, the way initial placer sort the blocks to place is aligned
    //       how reconstruction pass clusters created, so there is no need to explicitely
    //       prioritize these clusters. However, if it changes in time, the atoms clustered
    //       in neighbor pass and atoms misplaced might not match exactly.
    if (g_vpr_ctx.atom().flat_placement_info().valid) {
        initial_placement(vpr_setup_.PlacerOpts,
                          vpr_setup_.PlacerOpts.constraints_file.c_str(),
                          vpr_setup_.NocOpts,
                          blk_loc_registry,
                          *g_vpr_ctx.placement().place_macros,
                          noc_cost_handler,
                          g_vpr_ctx.atom().flat_placement_info(),
                          rng);
        // Log some information on how good the reconstruction was.
        log_flat_placement_reconstruction_info(g_vpr_ctx.atom().flat_placement_info(),
                                               blk_loc_registry.block_locs(),
                                               g_vpr_ctx.clustering().atoms_lookup,
                                               g_vpr_ctx.atom().lookup(),
                                               atom_netlist_,
                                               g_vpr_ctx.clustering().clb_nlist);
    } else {
        // If a flat placement is not being read or casted at that point, cast
        // the partial placement to flat placement here. So that it can be used
        // to guide the initial placer and for logging results. This part is
        // added for using the FlatRecon with GP output.
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
    }

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

    // Perform clustering using partial placement
    const ClusteredNetlist& clb_nlist = create_clusters(cluster_legalizer, p_placement);

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

    // Perform the initial placement on created clusters
    place_clusters(p_placement);
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
}
