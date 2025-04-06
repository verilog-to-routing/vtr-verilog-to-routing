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

#include "physical_types_util.h"

std::unique_ptr<FullLegalizer> make_full_legalizer(e_ap_full_legalizer full_legalizer_type,
                                                   const APNetlist& ap_netlist,
                                                   const AtomNetlist& atom_netlist,
                                                   const Prepacker& prepacker,
                                                   t_vpr_setup& vpr_setup,
                                                   const t_arch& arch,
                                                   const DeviceGrid& device_grid) {
    switch (full_legalizer_type) {
        case e_ap_full_legalizer::Naive:
            return std::make_unique<NaiveFullLegalizer>(ap_netlist,
                                                        atom_netlist,
                                                        prepacker,
                                                        vpr_setup,
                                                        arch,
                                                        device_grid);
        case e_ap_full_legalizer::APPack:
            return std::make_unique<APPack>(ap_netlist,
                                            atom_netlist,
                                            prepacker,
                                            vpr_setup,
                                            arch,
                                            device_grid);
        case e_ap_full_legalizer::Basic_Min_Disturbance:
            VTR_LOG("Basic Minimum Disturbance Full Legalizer selected!\n");
            return std::make_unique<BasicMinDisturbance>(ap_netlist,
                                                        atom_netlist,
                                                        prepacker,
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
struct device_tile_id_tag {};
typedef vtr::StrongId<device_tile_id_tag, size_t> DeviceTileId;

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
    bool place_cluster_reconstruction(ClusterBlockId clb_blk_id,
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
        
        to_loc.sub_tile = sub_tile;
        return try_place_macro(pl_macro, to_loc, blk_loc_registry);
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
                                                const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
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
    const t_model* root_model = atom_ctx.netlist().block_model(root_atom);

    auto itr = primitive_candidate_block_types.find(root_model);
    VTR_ASSERT(itr != primitive_candidate_block_types.end());
    const std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

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

/*
Initializes the grids to hold the LegalizationCluster's created
*/
void BasicMinDisturbance::initialize_cluster_grids() {
    VTR_LOG("You are in initialize_cluster_grids()\n");

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;

    size_t grid_width = device_grid.width();
    size_t grid_height = device_grid.height();
    size_t num_layers = device_grid.get_num_layers();

    cluster_grids = ClusterGridReconstruction(num_layers, grid_width, grid_height);
    tile_type.resize({num_layers, grid_width, grid_height});

    for (size_t layer_num = 0; layer_num < num_layers; layer_num++) {
        for (size_t x = 0; x < grid_width; x++) {
            for (size_t y = 0; y < grid_height; y++) {
                t_physical_tile_loc tile_loc = {(int)x, (int)y, (int)layer_num};
                auto type = device_grid.get_physical_type(tile_loc);
                int num_subtiles = type->capacity;

                cluster_grids.initialize_tile(layer_num, x, y, num_subtiles);
                tile_type[layer_num][x][y] = type;
            }
        }
    }

    VTR_LOG("Cluster grids initialized with dimensions: layers=%zu, width=%zu, height=%zu\n",
            num_layers, grid_width, grid_height);
}


// Idea is to get the logical block type of a molecule and implementation is inspired by the create_new_cluster function
t_logical_block_type_ptr get_molecule_logical_block_type(
    PackMoleculeId mol_id,
    const Prepacker& prepacker,
    const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {

    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const t_pack_molecule& molecule = prepacker.get_molecule(mol_id);

    // Get the root atom of the molecule
    AtomBlockId root_atom = molecule.atom_block_ids[molecule.root];
    if (!root_atom.is_valid()) {
        VTR_LOG_WARN("Molecule ID %zu does not have a valid root atom!\n", size_t(mol_id));
        return nullptr;
    }

    // Get the t_model* of the root atom
    const t_model* root_model = atom_ctx.netlist().block_model(root_atom);
    if (!root_model) {
        VTR_LOG_WARN("Molecule ID %zu has an invalid root model!\n", size_t(mol_id));
        return nullptr;
    }

    // Find the candidate logical block types for this model
    auto itr = primitive_candidate_block_types.find(root_model);
    if (itr == primitive_candidate_block_types.end()) {
        VTR_LOG_WARN("No candidate block type found for molecule ID %zu with model %s!\n", size_t(mol_id), root_model->name);
        return nullptr;
    }

    // Return the first valid logical block type
    const std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;
    if (!candidate_types.empty()) {
        return candidate_types.front();  // Return the first candidate type
    }

    VTR_LOG_WARN("Molecule ID %zu has no valid logical block type!\n", size_t(mol_id));
    return nullptr;  // No valid type found
}


std::vector<APBlockId> BasicMinDisturbance::pack_recontruction_pass(ClusterLegalizer& cluster_legalizer,
                                                                         const PartialPlacement& p_placement) 
{
    /* Sort the blocks as molecule ext pin num before processing */
    
    // Create a vector of APBlockId with associated ext_inps count
    std::vector<std::pair<APBlockId, int>> sorted_blocks;
    sorted_blocks.reserve(ap_netlist_.blocks().size());

    // Populate the vector
    for (APBlockId ap_blk_id: ap_netlist_.blocks()) {
        // Get the molecule that AP block represents
        PackMoleculeId mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& mol = prepacker_.get_molecule(mol_id);

        // Compute external inputs used by this molecule
        t_molecule_stats molecule_stats = prepacker_.calc_molecule_stats(mol_id, atom_netlist_);
        int ext_inps = molecule_stats.num_used_ext_inputs;

        // Store block along with its external input count
        sorted_blocks.emplace_back(ap_blk_id, ext_inps);
    }

    // Sort in descending order based on ext_inps
    std::sort(sorted_blocks.begin(), sorted_blocks.end(), 
            [](const std::pair<APBlockId, int>& a, const std::pair<APBlockId, int>& b) {
                return a.second > b.second; // Sort in descending order
            });
    
    //  Create the legalized clusters per tile.
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    // molecule ids that cannot be placed for any reason
    std::vector<APBlockId> unclustered_blocks; 


    vtr::ScopedStartFinishTimer first_pass_timer("First Pass Inspecting");


    //iterate over the sorted blocks instead of ap netlist order
    for (const auto& [ap_blk_id, ext_inps] : sorted_blocks) {
        // Get the molecule that AP block represents
        PackMoleculeId mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& mol = prepacker_.get_molecule(mol_id);

        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(ap_blk_id);
        int sub_tile = p_placement.block_sub_tiles[ap_blk_id];

        LegalizationClusterId cluster_id = cluster_grids.get(tile_loc.layer_num, tile_loc.x, tile_loc.y, sub_tile);

        /*DEBUG*/
        // if (mol.is_chain()){
        //     VTR_LOG("Molecule of ID %zu is in a chain of ID %zu.\n", mol_id, mol.chain_id);
        // }

        if (cluster_id.is_valid()) {
            // Cluster exists at this location
            // VTR_LOG("Valid cluster found at tile (%d, %d, layer %d, sub_tile %d), Cluster ID: %zu\n",
            //     tile_loc.x, tile_loc.y, tile_loc.layer_num, sub_tile, cluster_id);

            // try to grow that cluster with the current molecule
            if (!cluster_legalizer.is_molecule_compatible(mol_id, cluster_id)) {
                //VTR_LOG("Current molecule of ID %zu is not compatible with created cluster of ID %zu.\n", mol_id, cluster_id);
                unclustered_blocks.push_back(ap_blk_id);
                continue;
            } 

            // we now it is compatible, try to add.
            e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(mol_id, cluster_id);  // currently most time consuming part.
            if (pack_status == e_block_pack_status::BLK_PASSED) {
                cluster_grids.set(tile_loc.layer_num, tile_loc.x, tile_loc.y, sub_tile, cluster_id);
                cluster_location_map[cluster_id] = std::make_tuple(tile_loc.x, tile_loc.y, tile_loc.layer_num, sub_tile);
            } else {
                //VTR_LOG("Current molecule of ID %zu could not added to cluster of ID %zu.\n", mol_id, cluster_id);
                unclustered_blocks.push_back(ap_blk_id);
            }

        } else {
            // VTR_LOG("No valid cluster at tile (%d, %d, layer %d, sub_tile %d). Need to create a new one.\n",
            //     tile_loc.x, tile_loc.y, tile_loc.layer_num, sub_tile);


            // I want to check if i can start a cluster with that type at that location (instead of creating blindly).
            // That way, addition to the cluster will be checked by the cluster_legalizer and I hope placement will be 
            // just locking each cluster to specified location.
            
            t_logical_block_type_ptr block_type = get_molecule_logical_block_type(mol_id, prepacker_, primitive_candidate_block_types);

            if (!block_type) {
                VPR_FATAL_ERROR(VPR_ERROR_AP, "Could not determine block type for molecule ID %zu\n", size_t(mol_id));
                unclustered_blocks.push_back(ap_blk_id);
                continue;
            }

            auto type = tile_type[tile_loc.layer_num][tile_loc.x][tile_loc.y];
            if (is_tile_compatible(type, block_type)) {
                LegalizationClusterId new_cluster_id = create_new_cluster(mol_id, prepacker_, cluster_legalizer, primitive_candidate_block_types);
                cluster_grids.set(tile_loc.layer_num, tile_loc.x, tile_loc.y, sub_tile, new_cluster_id);
                cluster_location_map[new_cluster_id] = std::make_tuple(tile_loc.x, tile_loc.y, tile_loc.layer_num, sub_tile);
            } else {
                // VTR_LOG("Current molecule of ID %zu (block type: %s) could not start a new cluster at (x: %d, y: %d, layer: %d) due to tile incompatibility.\n",
                //         size_t(mol_id), block_type->name.c_str(), tile_loc.x, tile_loc.y, tile_loc.layer_num);
                unclustered_blocks.push_back(ap_blk_id);
                continue;
            }        



        }
    }   






    VTR_LOG("Number of molecules that coud not clusterd at first iteration is %zu out of %zu.\n", unclustered_blocks.size(), ap_netlist_.blocks().size());

    VTR_LOG("Stats after first pass: Elapsed Time = %f, max rss mid = %f, delta rss mid = %f\n" ,first_pass_timer.elapsed_sec(), first_pass_timer.max_rss_mib(), first_pass_timer.delta_max_rss_mib());

    VPR_FATAL_ERROR(VPR_ERROR_AP, "Dur kardesim nereye.\n");
    
    return unclustered_blocks;
}

void BasicMinDisturbance::place_clusters(const ClusteredNetlist& clb_nlist,
                                         const PlaceMacros& place_macros,
                                         std::unordered_map<AtomBlockId, LegalizationClusterId> atom_to_legalization_map) {
    
    VTR_LOG("=== BasicMinDisturbance::place_clusters ===\n");
                                            
    // try to find a way to map the LegalizationClusterId to ClusterBlockId
    // idea: iterate over the ClusterBlockId's and get atoms in it. Then find the LegalizationClusterId containing these atoms.    
    vtr::vector<LegalizationClusterId, ClusterBlockId> legalize_idx_to_clb_idx(clb_nlist.blocks().size());
    std::vector<ClusterBlockId> unplaced_clusters;
    APClusterPlacer ap_cluster_placer(place_macros, vpr_setup_.PlacerOpts.constraints_file.c_str());
    for (ClusterBlockId clb_index: clb_nlist.blocks()) {
        ClusterAtomsLookup cluster_lookup;
        std::vector<AtomBlockId> atom_ids = cluster_lookup.atoms_in_cluster(clb_index);
        //VTR_LOG("In ClusterBlockId %zu there are %zu atoms.\n", clb_index, atom_ids.size());
        LegalizationClusterId legalization_clb_index = atom_to_legalization_map[atom_ids[0]];
        legalize_idx_to_clb_idx[legalization_clb_index] = clb_index;
        //VTR_LOG("   Corresponds to LegalizationClusterId %zu.\n", legalization_clb_index);

        
        int x = -1, y = -1, layer = -1, sub_tile = -1;  // Initialize with invalid values

        auto it = cluster_location_map.find(legalization_clb_index);
        if (it != cluster_location_map.end()) {
            std::tie(x, y, layer, sub_tile) = it->second;  // Assign values from the map
            //VTR_LOG("    Cluster %zu is at (x:%d, y:%d, layer:%d, sub_tile:%d)\n", legalization_clb_index, x, y, layer, sub_tile);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_AP, "    Cluster %zu not found in the grid.\n", legalization_clb_index);
        }

        t_physical_tile_loc tile_loc = {x,y,layer};
        bool placed = ap_cluster_placer.place_cluster_reconstruction(clb_index, tile_loc, sub_tile);
        if (!placed) {
            // Add to list of unplaced clusters.
            unplaced_clusters.push_back(clb_index);
        }
    }


    if (unplaced_clusters.size() > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_AP, "BasicMinDisturbance unplaced cluster policy is not implemented yet. Number of unplaced clusters is %zu\n.", unplaced_clusters.size());
    }
}

void BasicMinDisturbance::legalize(const PartialPlacement& p_placement) {
    // Create a scoped timer for the full legalizer
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Full Legalizer");
    
    VTR_LOG("Entered the legalize function of BasicMinDisturbance.\n");

    /*
    Data structure to keep track of the clusters created at locations.
    

    grids[layer][x][y] -> vector<int sub_tile, LegalizationCluster created_cluster>

    Lets say we have a molecule that want to go x, y, layer, sub_tile. If there is a cluster created already, 
    there will be a element in grids[layer][x][y] vector with first element being the given sub_tile. 
    If there is a cluster already, we will try to add teh current molecule there. Otherwise, we will try to
    create a new one.

    By trying, we mean that the physical block at that location is compatible with logical block we have and
    there is enough space.
    
    */
    initialize_cluster_grids();

    cluster_location_map.reserve(ap_netlist_.blocks().size()); //over estimate

    std::vector<std::string> target_ext_pin_util = {"1.0"};
    
    t_pack_high_fanout_thresholds high_fanout_thresholds(vpr_setup_.PackerOpts.high_fanout_threshold);
    ClusterLegalizer cluster_legalizer(atom_netlist_,
                                       prepacker_,
                                       vpr_setup_.PackerRRGraph,
                                       target_ext_pin_util,
                                       high_fanout_thresholds,
                                       ClusterLegalizationStrategy::FULL, //Change this to skip one
                                       vpr_setup_.PackerOpts.enable_pin_feasibility_filter,
                                       vpr_setup_.PackerOpts.pack_verbosity);

    

    // molecule ids that cannot be placed for any reason
    std::vector<APBlockId> unclusterd_blocks = pack_recontruction_pass(cluster_legalizer, p_placement); 


    if (unclusterd_blocks.size() != 0) {
        VPR_FATAL_ERROR(VPR_ERROR_AP, "BasicMinDisturbance unplaced molecule policy is not implemented yet.");
    }
    
    // Check and output the clustering.
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(cluster_legalizer, vpr_setup_.PackerOpts, is_clock, &arch_);

    // save the LegalizationClusterId's of atoms for placing
    std::unordered_map<AtomBlockId, LegalizationClusterId> atom_to_legalization_map;
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        PackMoleculeId blk_mol_id = ap_netlist_.block_molecule(ap_blk_id);
        const t_pack_molecule& blk_mol = prepacker_.get_molecule(blk_mol_id);
        for (AtomBlockId atom_blk_id : blk_mol.atom_block_ids) {
            if (!atom_blk_id.is_valid())
                continue;
            // Ensure that this block is not in any other AP block. That would
            // be weird.
            VTR_ASSERT(!atom_to_legalization_map[atom_blk_id].is_valid());
            LegalizationClusterId cluser_id = cluster_legalizer.get_atom_cluster(atom_blk_id);
            VTR_ASSERT(cluser_id.is_valid());
            atom_to_legalization_map[atom_blk_id] = cluser_id;    
        }
    }

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
    
    // implement the placement of cluster.
    // Get the clustering from the global context.
    // TODO: Eventually should be returned from the create_clusters method.
    //const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Initialize the placement context.
    g_vpr_ctx.mutable_placement().init_placement_context(vpr_setup_.PlacerOpts,
                                                         arch_.directs);

    const PlaceMacros& place_macros = *g_vpr_ctx.placement().place_macros;

    // Update the floorplanning context with the macro information.
    g_vpr_ctx.mutable_floorplanning().update_floorplanning_context_pre_place(place_macros);

    // Place the clusters based on where the atoms want to be placed.
    // TODO: Instead of using p_placement, use your cluster_grids locations. So that you can use this method for your own created clusters as well.   ***IMPORTANT***
    
    place_clusters(clb_nlist, place_macros, atom_to_legalization_map);
    //place_clusters_naive(clb_nlist, place_macros, p_placement);
    
    //cluster_legalizer.reset();

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
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>>
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
    try_pack(&vpr_setup_.PackerOpts,
             &vpr_setup_.AnalysisOpts,
             arch_,
             vpr_setup_.RoutingArch,
             vpr_setup_.PackerRRGraph,
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
