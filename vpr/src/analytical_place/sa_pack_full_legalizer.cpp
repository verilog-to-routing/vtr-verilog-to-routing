/**
 * @file
 * @author  Alex Singer
 * @date    May 2026
 * @brief   Implements the SAPack full legalizer.
 */

#include "sa_pack_full_legalizer.h"

#include <optional>
#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>

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
#include "place_and_route.h"
#include "prepack.h"
#include "show_setup.h"
#include "verify_placement.h"
#include "vpr_api.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_random.h"
#include "vtr_time.h"
#include "vtr_vector.h"

// ============================================================================
// SAPackDeviceGrid
// ============================================================================

SAPackDeviceGrid::SAPackDeviceGrid(const DeviceGrid& device_grid) {
    auto [num_layers, width, height] = device_grid.dim_sizes();
    clusters_ = vtr::NdMatrix<std::vector<SAPackCluster>, 3>({num_layers, width, height});

    for (const t_physical_tile_loc& tile_loc : device_grid.all_locations()) {
        // Ignore non-root locations
        if (!device_grid.is_root_location(tile_loc))
            continue;
        int tile_capacity = device_grid.get_physical_type(tile_loc)->capacity;
        clusters_[tile_loc.layer_num][tile_loc.x][tile_loc.y].resize(tile_capacity);
    }
}

std::vector<SAPackCluster>& SAPackDeviceGrid::get_clusters(t_physical_tile_loc tile_loc) {
    return clusters_[tile_loc.layer_num][tile_loc.x][tile_loc.y];
}

const std::vector<SAPackCluster>& SAPackDeviceGrid::get_clusters(t_physical_tile_loc tile_loc) const {
    return clusters_[tile_loc.layer_num][tile_loc.x][tile_loc.y];
}

// ============================================================================
// SAPack
// ============================================================================

bool SAPack::try_place_mol_in_tile(PackMoleculeId mol_id, t_physical_tile_loc tile_loc) {
    t_physical_tile_type_ptr tile_type = device_grid_.get_physical_type(tile_loc);

    VTR_ASSERT(mol_id.is_valid());
    const t_pack_molecule& seed_molecule = prepacker_.get_molecule(mol_id);
    AtomBlockId root_atom = seed_molecule.atom_block_ids[seed_molecule.root];
    LogicalModelId root_model_id = g_vpr_ctx.atom().netlist().block_model(root_atom);

    VTR_ASSERT(root_model_id.is_valid());
    VTR_ASSERT(!primitive_candidate_block_types_[root_model_id].empty());
    const std::vector<t_logical_block_type_ptr>& candidate_cluster_types = primitive_candidate_block_types_[root_model_id];

    // Try each sub-tile from bottom to top to try and place the molecules.
    std::vector<SAPackCluster>& clusters_in_tile = sa_pack_grid_->get_clusters(tile_loc);
    for (SAPackCluster& cluster : clusters_in_tile) {
        if (cluster.is_finalized)
            continue;
        // Check if a cluster has been created.
        if (cluster.cluster_id.is_valid()) {
            if (cluster_legalizer_->is_molecule_compatible(mol_id, cluster.cluster_id)) {
                if (cluster_legalizer_->add_mol_to_cluster(mol_id, cluster.cluster_id) == e_block_pack_status::BLK_PASSED)
                    return true;
            }
        } else {
            // See if we can create one here.
            // TODO: This search can be made much more efficient.
            // FIXME: This subtraction is not very clean and should not be used.
            size_t sub_tile = &cluster - clusters_in_tile.data();
            std::vector<t_logical_block_type_ptr> equivalent_sites;
            for (const t_sub_tile& sub_tile_val : tile_type->sub_tiles) {
                if (sub_tile_val.capacity.is_in_range(sub_tile)) {
                    equivalent_sites = sub_tile_val.equivalent_sites;
                }
            }
            for (t_logical_block_type_ptr cluster_type : equivalent_sites) {
                for (t_logical_block_type_ptr candidate_cluster_type : candidate_cluster_types) {
                    if (cluster_type->index == candidate_cluster_type->index) {
                        int num_modes = candidate_cluster_type->pb_graph_head->pb_type->num_modes;
                        for (int mode = 0; mode < num_modes; mode++) {
                            e_block_pack_status pack_status = e_block_pack_status::BLK_STATUS_UNDEFINED;
                            LegalizationClusterId new_cluster_id;
                            std::tie(pack_status, new_cluster_id) = cluster_legalizer_->start_new_cluster(mol_id, candidate_cluster_type, mode);
                            if (pack_status == e_block_pack_status::BLK_PASSED) {
                                cluster.cluster_id = new_cluster_id;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    // If the molecule could not be placed here, return false.
    return false;
}

bool SAPack::try_place_mol_in_nearest_tile(PackMoleculeId mol_id, t_physical_tile_loc tile_loc) {
    std::queue<t_physical_tile_loc> search_queue;
    vtr::NdMatrix<bool, 3> visited({device_grid_.get_num_layers(),
                                    device_grid_.width(),
                                    device_grid_.height()},
                                    false);

    search_queue.push(tile_loc);

    while (!search_queue.empty()) {
        t_physical_tile_loc current_loc = search_queue.front();
        search_queue.pop();
        if (!device_grid_.is_loc_on_device(current_loc))
            continue;
        if (visited[current_loc.layer_num][current_loc.x][current_loc.y])
            continue;
        visited[current_loc.layer_num][current_loc.x][current_loc.y] = true;

        if (try_place_mol_in_tile(mol_id, current_loc))
            return true;

        search_queue.push(t_physical_tile_loc(current_loc.x - 1, current_loc.y, current_loc.layer_num));
        search_queue.push(t_physical_tile_loc(current_loc.x + 1, current_loc.y, current_loc.layer_num));
        search_queue.push(t_physical_tile_loc(current_loc.x, current_loc.y - 1, current_loc.layer_num));
        search_queue.push(t_physical_tile_loc(current_loc.x, current_loc.y + 1, current_loc.layer_num));
        search_queue.push(t_physical_tile_loc(current_loc.x, current_loc.y, current_loc.layer_num - 1));
        search_queue.push(t_physical_tile_loc(current_loc.x, current_loc.y, current_loc.layer_num + 1));
    }

    return false;
}

void SAPack::place_molecules(const PartialPlacement& p_placement) {
    std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>> unplaced_mols;

    // Go through each atom in the APNetlist and place them in their tiles.
    // For now, we never use full legalization.
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        // Get the root tile location that this block wants to go to.
        t_physical_tile_loc tile_loc(p_placement.block_x_locs[ap_blk_id],
                                     p_placement.block_y_locs[ap_blk_id],
                                     p_placement.block_layer_nums[ap_blk_id]);

        tile_loc = device_grid_.get_nearest_loc_on_device(tile_loc);
        tile_loc = device_grid_.get_root_location(tile_loc);

        // FIXME: We need to handle things like RAM blocks here when creating
        //        the clusters. Maybe able to ignore if SA proves to be better.
        for (PackMoleculeId mol_id : ap_netlist_.block_molecules(ap_blk_id)) {
            if (!try_place_mol_in_tile(mol_id, tile_loc))
                unplaced_mols.push_back(std::make_pair(mol_id, tile_loc));
        }
    }

    // Place the rest of the molecules by searching for the nearest place to put
    // them.
    for (auto p : unplaced_mols) {
        if (!try_place_mol_in_nearest_tile(p.first, p.second)) {
            // In the future, we should fall back on APPack
            VPR_FATAL_ERROR(VPR_ERROR_AP, "Cannot find anywhere to place molecule.");
        }
    }
}

std::vector<PackMoleculeId> SAPack::finalize_cluster(SAPackCluster& cluster) {
    VTR_ASSERT_SAFE(cluster.cluster_id.is_valid());
    if (cluster_legalizer_->check_cluster_legality(cluster.cluster_id)) {
        cluster.is_finalized = true;
        cluster_legalizer_->clean_cluster(cluster.cluster_id);
        return {};
    }

    // Rebuild the cluster from scratch.
    std::vector<PackMoleculeId> molecules = cluster_legalizer_->get_cluster_molecules(cluster.cluster_id);
    t_logical_block_type_ptr cluster_type = cluster_legalizer_->get_cluster_type(cluster.cluster_id);
    cluster_legalizer_->destroy_cluster(cluster.cluster_id);
    cluster.cluster_id = LegalizationClusterId::INVALID();

    // FIXME: Add safety assert here.
    PackMoleculeId seed_mol_id = molecules[0];

    cluster_legalizer_->set_legalization_strategy(ClusterLegalizationStrategy::FULL);
    int num_modes = cluster_type->pb_graph_head->pb_type->num_modes;
    for (int mode = 0; mode < num_modes; mode++) {
        e_block_pack_status pack_status = e_block_pack_status::BLK_STATUS_UNDEFINED;
        LegalizationClusterId new_cluster_id;
        std::tie(pack_status, new_cluster_id) = cluster_legalizer_->start_new_cluster(seed_mol_id, cluster_type, mode);
        if (pack_status == e_block_pack_status::BLK_PASSED) {
            cluster.cluster_id = new_cluster_id;
            break;
        }
    }

    VTR_ASSERT(cluster.cluster_id.is_valid());

    std::vector<PackMoleculeId> rejected_mols;
    for (size_t i = 1; i < molecules.size(); i++) {
        if (cluster_legalizer_->add_mol_to_cluster(molecules[i], cluster.cluster_id) != e_block_pack_status::BLK_PASSED) {
            rejected_mols.push_back(molecules[i]);
        }
    }
    cluster_legalizer_->clean_cluster(cluster.cluster_id);
    cluster.is_finalized = true;
    cluster_legalizer_->set_legalization_strategy(ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE);

    return rejected_mols;
}

void SAPack::legalize(const PartialPlacement& p_placement) {
    // Start a scoped timer for the Full Legalizer stage.
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Full Legalizer");

    // The target external pin utilization is set to 1.0 to avoid over-restricting
    // reconstruction due to conservative pin feasibility. The SKIP_INTRA_LB_ROUTE
    // strategy speeds up reconstruction by skipping intra-LB routing checks.
    std::vector<std::string> target_ext_pin_util = {"1.0"};
    t_pack_high_fanout_thresholds high_fanout_thresholds(vpr_setup_.PackerOpts.high_fanout_threshold);

    cluster_legalizer_.emplace(
        atom_netlist_,
        prepacker_,
        vpr_setup_.PackerRRGraph,
        target_ext_pin_util,
        high_fanout_thresholds,
        ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
        vpr_setup_.PackerOpts.enable_pin_feasibility_filter,
        false, // --memoize_cluster_packings is not yet supported for flat-recon
        arch_.models,
        vpr_setup_.PackerOpts.pack_verbosity);

    const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
    VTR_LOG("Device (width, height): (%zu,%zu)\n", device_grid.width(), device_grid.height());

    sa_pack_grid_.emplace(device_grid_);

    primitive_candidate_block_types_ = identify_primitive_candidate_block_types();

    place_molecules(p_placement);

    // Next, fully legalize the clusters one-by-one. When a legalization fails,
    // the atoms should be moved to the nearest cluster that can support them.
    // TODO: It would be a much better idea to legalize the densest clusters
    //       first; however this algorithm is likely to change.
    // FIXME: This while loop is terrible.
    bool done = false;
    while (!done) {
        done = true;
        for (const t_physical_tile_loc& tile_loc : device_grid.all_locations()) {
            if (!device_grid_.is_root_location(tile_loc))
                continue;

            // Try intra-lb route. If it fails, rebuild the cluster and place
            // any remaining molecules nearby using the prior method.
            std::vector<SAPackCluster>& clusters_in_tile = sa_pack_grid_->get_clusters(tile_loc);
            for (SAPackCluster& cluster : clusters_in_tile) {
                if (!cluster.cluster_id.is_valid())
                    continue;

                std::vector<PackMoleculeId> rejected_mols = finalize_cluster(cluster);
                // If a molecule was rejected, it could be placed anywhere on the grid.
                // Need to look for it.
                if (!rejected_mols.empty()) {
                    done = false;
                }
                
                for (PackMoleculeId rejected_mol : rejected_mols) {
                    if (!try_place_mol_in_nearest_tile(rejected_mol, tile_loc)) {
                        // In the future, we should fall back on APPack
                        VPR_FATAL_ERROR(VPR_ERROR_AP, "Cannot find anywhere to place molecule.");
                    }
                }
            }
        }
    }

    // Check and output the clustering.
    std::unordered_set<AtomNetId> is_clock = alloc_and_load_is_clock();
    check_and_output_clustering(*cluster_legalizer_, vpr_setup_.PackerOpts, is_clock, &arch_);
    // Reset the cluster legalizer. This is required to load the packing.
    cluster_legalizer_->reset();
    // Regenerate the clustered netlist from the file generated previously.
    // FIXME: This writing and loading from a file is wasteful. Should generate
    //        the clusters directly from the cluster legalizer.
    vpr_load_packing(vpr_setup_, arch_);
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    // Verify the packing and print some info
    check_netlist(vpr_setup_.PackerOpts.pack_verbosity);
    write_clustered_netlist_stats(vpr_setup_.FileNameOpts.write_block_usage);
    print_pb_type_count(clb_nlist);

    // Use the initial placer to place the atoms, but we should use their current
    // positions as our guess for where they should go. This should handle any
    // weird macro cases thay will come up.

    // Convert the Partial Placement (APNetlist) to a flat placement (AtomNetlist).
    // FIXME: Update this to be the locations AFTER clustering.
    FlatPlacementInfo flat_placement_info(atom_netlist_);
    for (APBlockId ap_blk_id : ap_netlist_.blocks()) {
        for (PackMoleculeId mol_id : ap_netlist_.block_molecules(ap_blk_id)) {
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
    }

    // If auto device sizing is used, recreate the device grid after final
    // clustering and before initial placement. The earlier grid was based on
    // a pre-clustering estimate, but the required device size can change once
    // the final clustering is known. Rebuilding the grid also invalidates the
    // RR graph generated for the estimated device size as required.
    if (vpr_setup_.PackerOpts.device_layout == "auto") {
        vpr_create_device_grid(vpr_setup_, arch_);
    }

    // Setup NoCs
    // TODO: We have some flow divergence. When the device grid is created the
    //       final time, we should set this up.
    vpr_setup_noc(vpr_setup_, arch_);

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
