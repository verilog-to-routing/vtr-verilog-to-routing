#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Defines the FullLegalizer class which takes a partial AP placement
 *          and generates a fully legal clustering and placement which can be
 *          routed by VTR.
 */

#include <memory>
#include <unordered_set>
#include "ap_flow_enums.h"
#include "cluster_legalizer.h"

// Forward declarations
class APNetlist;
class AtomNetlist;
class ClusteredNetlist;
class DeviceGrid;
struct PartialPlacement;
class PlaceMacros;
class PreClusterTimingManager;
class Prepacker;
struct t_arch;
struct t_vpr_setup;

/**
 * @brief The full legalizer in an AP flow
 *
 * Given a valid partial placement (of any level of legality), will produce a
 * fully legal clustering and clustered placement for use in the rest of the
 * VTR flow.
 */
class FullLegalizer {
  public:
    virtual ~FullLegalizer() {}

    FullLegalizer(const APNetlist& ap_netlist,
                  const AtomNetlist& atom_netlist,
                  const Prepacker& prepacker,
                  const PreClusterTimingManager& pre_cluster_timing_manager,
                  const t_vpr_setup& vpr_setup,
                  const t_arch& arch,
                  const DeviceGrid& device_grid)
        : ap_netlist_(ap_netlist)
        , atom_netlist_(atom_netlist)
        , prepacker_(prepacker)
        , pre_cluster_timing_manager_(pre_cluster_timing_manager)
        , vpr_setup_(vpr_setup)
        , arch_(arch)
        , device_grid_(device_grid) {}

    /**
     * @brief Perform legalization on the given partial placement solution
     *
     *  @param p_placement  A valid partial placement (passes verify method).
     *                      This implies that all blocks are placed on the
     *                      device grid and fixed blocks are observed.
     */
    virtual void legalize(const PartialPlacement& p_placement) = 0;

  protected:
    /// @brief The AP Netlist to fully legalize the flat placement of.
    const APNetlist& ap_netlist_;

    /// @brief The Atom Netlist used to generate the AP Netlist.
    const AtomNetlist& atom_netlist_;

    /// @brief The Prepacker used to create molecules from the Atom Netlist.
    const Prepacker& prepacker_;

    /// @brief Pre-Clustering timing manager, hold pre-computed delay information
    ///        at the primitive level prior to packing.
    const PreClusterTimingManager& pre_cluster_timing_manager_;

    /// @brief The VPR setup options passed into the VPR flow. This must be
    ///        mutable since some parts of packing modify the options.
    const t_vpr_setup& vpr_setup_;

    /// @brief Information on the architecture of the FPGA.
    const t_arch& arch_;

    /// @brief The device grid which records where clusters can be placed.
    const DeviceGrid& device_grid_;
};

/**
 * @brief A factory method which creates a Full Legalizer of the given type.
 */
std::unique_ptr<FullLegalizer> make_full_legalizer(e_ap_full_legalizer full_legalizer_type,
                                                   const APNetlist& ap_netlist,
                                                   const AtomNetlist& atom_netlist,
                                                   const Prepacker& prepacker,
                                                   const PreClusterTimingManager& pre_cluster_timing_manager,
                                                   const t_vpr_setup& vpr_setup,
                                                   const t_arch& arch,
                                                   const DeviceGrid& device_grid);

/**
 * @brief FlatRecon: The Flat Placement Reconstruction Full Legalizer.
 *
 * The idea of the FlatRecon is to reconstruct (pack and place) a given flat
 * placement with minimum disturbance. It can be used with the flat placement
 * either being read from an '.fplace' file or with the GP output. However, in
 * both cases it expects the given flat placement to be close to legal.
 *
 * Before packing, molecules are sorted so that long carry chain molecules are
 * priorizited. For molecules in the same priority group, higher external pins
 * are used as a tie-breaker. It then groups the molecules according to the tiles
 * determined from their flat placement.
 *
 * The packing consists of two passes: reconstruction and neighbor. In the
 * reconstruction pass, it iterates over each tile and tries to create least
 * amount of clusters with the molecules that want to be in that tile. It
 * first tries to cluster with SKIP_INTRA_LB_ROUTE strategy and then tries with
 * the FULL strategy with the failing molecules in that tile. Any molecule that
 * could not clustered in that pass (either by not being compatible with the
 * desired tile or not being able to add created clusters) is passed to neighbor
 * pass. In the neighbor pass, the first molecule inserted in the reconstruction
 * pass is popped and selected as the seed molecule. Then, its N-neighboring
 * molecules that are at most N tiles away (in Manhattan distance) are selected
 * to be candidate molecules. If candidate molecules are added successfully,
 * they are pooped as well. Then, the next unclustered molecule is popped and it
 * continues until all molecules are clustered. The neighbor search radius
 * is set to 8 based on experiments on Titan benchmarks. Radius 8 was giving
 * the least amount of clusters without hurting the maximum displacement.
 *
 * After cluster creation, each cluster is placed by the initial placer at the
 * grid location nearest to the centroid of its atoms.
 *
 */
class FlatRecon : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    /**
     * @brief Perform the FlatRecon full legalization.
     */
    void legalize(const PartialPlacement& p_placement) final;

  private:
    /// @brief Mapping from subtile location to legalization cluster id to keep
    ///        track of clusters created.
    /// TODO: It would make sense to store this as a 3D NDMatrix of arrays where we can
    ///       index into the [layer][x][y][subtile] and get the cluster ID at that location.
    ///       It will be faster than using an unordered map and likely more space efficient.
    std::unordered_map<t_pl_loc, LegalizationClusterId> loc_to_cluster_id_placed;

    /// @brief Mapping from a molecule id to its desired physical tile location.
    std::unordered_map<PackMoleculeId, t_physical_tile_loc> mol_desired_physical_tile_loc;

    /// @brief Mapping from physical tile location to legalization cluster ids
    ///        to keep track of clusters created.
    std::unordered_map<t_pl_loc, std::vector<LegalizationClusterId>> tile_loc_to_cluster_id_placed;

    /// @brief Unclustered blocks after first pass
    std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>> unclustered_blocks_saved;

    /// @brief Vector of neighbor pass clusters
    std::vector<LegalizationClusterId> neighbor_pass_clusters;

    /**
     * @brief Helper method to sort and group molecules by desired tile location.
     *        It first sorts by being in a long carry chain, then by external input
     *        pin count.
     * @return Mapping from tile location to sorted vector of molecules that
     *         wants to be in that tile.
     */
    std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>
    sort_and_group_blocks_by_tile(const PartialPlacement& p_placement);

    /**
     * @brief Helper method to create clusters at a given tile location using
     *        given vector of molecules.
     *
     * Iterates over each subtile in the same order each time, hence trying to
     * create least amount of clusters in that tile. It also checks the compatibility
     * of the molecules with the tile before creating a cluster. Stores the cluster
     * ids' to check their legality or clean afterwards if needed.
     */
    void cluster_molecules_in_tile(const t_physical_tile_loc& tile_loc,
                                   const t_physical_tile_type_ptr& tile_type,
                                   const std::vector<PackMoleculeId>& tile_molecules,
                                   ClusterLegalizer& cluster_legalizer,
                                   const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                   std::unordered_map<LegalizationClusterId, t_pl_loc>& cluster_ids_to_check);

    /**
     * @brief Helper method to perform reconstruction clustering pass.
     *
     * Iterates over each tile and first tries to create least amount of
     * cluster in that tile with SKIP_INTRA_LB_ROUTE strategy. For the illegal
     * cluster molecules, tries with FULL strategy once before going to next tile.
     */
    void reconstruction_cluster_pass(ClusterLegalizer& cluster_legalizer,
                                     const DeviceGrid& device_grid,
                                     const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                     std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>& tile_blocks);

    /**
     * @brief Helper method to perform neighbor clustering.
     *
     * TODO: Add detailed description.
     *
     */
    void neighbor_clustering(ClusterLegalizer& cluster_legalizer,
                             const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                             std::vector<LegalizationClusterId>& first_pass_clusters,
                             size_t& total_molecules_in_join_with_neighbor);
    /**
     * @brief Helper method to perform orphan window clustering.
     *
     * Pops and selects the first molecule as seed for the cluster. Selects seed
     * molecule's neighboring molecules that are at most N tiles away (in Manhattan
     * distance) from it. Starting from the nearest neighbor, tries to add neighbor
     * molecules to the cluster created by seed molecule. Continues until no
     * unclustered molecules left.
     */
    void orphan_window_clustering(ClusterLegalizer& cluster_legalizer,
                                  const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                                  std::vector<std::pair<PackMoleculeId, t_physical_tile_loc>>& unclustered_blocks,
                                  int search_radius);

    /**
     * @brief Helper method to create clusters with reconstruction and neighbor pass.
     *
     * This will call sorting and grouping molecules by tile, reconstruction
     * clustering pass, and neighbor clustering pass.
     */
    void create_clusters(ClusterLegalizer& cluster_legalizer,
                                     const PartialPlacement& p_placement);

    /**
     * @brief Helper method to perform initial placement on clusters created.
     *
     * It uses the initial placement in the AP flow. It is guided to place the
     * clusters according to where its atoms are desired to be placed and can
     * also be used with GP output.
     */
    void place_clusters(const PartialPlacement& p_placement);
};

/**
 * @brief The Naive Full Legalizer.
 *
 * This Full Legalizer will try to create clusters exactly where they want to
 * according to the Partial Placement. It will grow clusters from atoms that
 * are placed in the same tile, then it will try to place the cluster at that
 * location. If a location cannot be found, once all other clusters have tried
 * to be placed, it will try to find anywhere the cluster will fit and place it
 * there.
 */
class NaiveFullLegalizer : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    /**
     * @brief Perform naive full legalization.
     */
    void legalize(const PartialPlacement& p_placement) final;

  private:
    /**
     * @brief Helper method to create the clusters from the given partial
     *        placement.
     * TODO: Should return a ClusteredNetlist object, but need to wait until
     *       it is separated from load_cluster_constraints.
     */
    void create_clusters(const PartialPlacement& p_placement);

    /**
     * @brief Helper method to place the clusters based on the given partial
     *        placement.
     */
    void place_clusters(const ClusteredNetlist& clb_nlist,
                        const PlaceMacros& place_macros,
                        const PartialPlacement& p_placement);
};

/**
 * @brief APPack: A flat-placement-informed Packer Placer.
 *
 * The idea of APPack is to use the flat-placement information generated by the
 * AP Flow to guide the Packer and Placer to a better solution.
 *
 * In the Packer, the flat-placement can provide more context for the clusters
 * to pull in atoms that want to be near the other atoms in the cluster, and
 * repell atoms that are far apart. This can potentially make better clusters
 * than a Packer that does not know that information.
 *
 * In the Placer, the flat-placement can help decide where clusters of atoms
 * want to be placed. If this placement is then fed into a Simulated Annealing
 * based Detailed Placement step, this would enable it to converge on a better
 * answer faster.
 */
class APPack : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    /**
     * @brief Run APPack.
     *
     * This will call the Packer and Placer using the options provided by the
     * user for these stages in VPR.
     */
    void legalize(const PartialPlacement& p_placement) final;
};
