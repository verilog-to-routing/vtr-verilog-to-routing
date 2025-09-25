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

    /// @brief Update drawing data structure for current placement
    void update_drawing_data_structures();

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
 * Reconstructs (packs and places) an input flat placement with minimal
 * disturbance. The flat placement may be read from a ``.fplace`` file or
 * taken from Global Placement (GP). In both cases the input is expected to be
 * near-legal.
 *
 * Before packing, molecules are sorted so that long carry chain molecules are
 * priorizited. For molecules in the same priority group, the number of external pins
 * is used as a tie-breaker. It then groups the molecules according to the tiles
 * determined from their flat placement.
 *
 * The packing consists of three passes:
 * 1) Self clustering: For each tile, form as few clusters as possible from
 *    molecules targeting that tile, and does not create more clusters than the
 *    tile can accommodate. Try clustering with the faster SKIP_INTRA_LB_ROUTE
 *    legality strategy (after each molecule is added) first; if the resulting
 *    cluster turns out to be unroutable, packing it is retried with the FULL
 *    legality checking strategy. Molecules that still cannot be clustered
 *    (incompatible with the tile or with the newly formed clusters) are passed
 *    to the neighbor pass.
 *
 * 2) Neighbor clustering: For each unclustered molecule, inspect clusters in
 *    the 8 neighboring tiles within the same layer. Tiles are processed in
 *    ascending order of average molecules-per-cluster. The unclustered molecule is
 *    added to an existing cluster if compatible (and all 8 are checked); no new
 *    clusters are created in this pass.
 *
 * 3) Orphan-window clustering: Remaining “orphan” molecules are clustered by
 *    repeated BFS expansions centered at seeds chosen by highest external input
 *    pin count. From each seed’s assigned location, try to cluster orphan molecules
 *    within a Manhattan distance ≤ radius. The default radius is 8 (empirically
 *    minimizes cluster count without inflating displacement on Titan benchmarks).
 *    If the resulting clusters do not fit the device, the pass is retried with
 *    radius 16, and finally with a radius spanning the whole device. Larger radii
 *    reduce cluster count but can increase displacement.
 *
 * After cluster creation, each cluster is placed by the initial placer at the
 * grid location nearest to the centroid of its atoms.
 *
 * TODO: Refer to the FPT 2025 Triple-AP paper if accepted.
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
    /// TODO: It might make sense to store this as a 3D NDMatrix of arrays where we can
    ///       index into the [layer][x][y][subtile] and get the cluster ID at that location.
    ///       It will be faster than using an unordered map and likely more space efficient.
    std::unordered_map<t_pl_loc, LegalizationClusterId> loc_to_cluster_id_placed;

    /// @brief Mapping from a molecule id to its desired physical tile location.
    vtr::vector<PackMoleculeId, t_physical_tile_loc> mol_desired_physical_tile_loc;

    /// @brief Mapping from legalization cluster ids to subtile locations. Using
    ///        unordered_map instead of vtr::vector since LegalizationClusterIds
    ///        can have significant gaps as you create a new ID for each cluster
    ///        you attempt to create.
    std::unordered_map<LegalizationClusterId, t_pl_loc> cluster_locs;

    /// @brief 3D NDMatrix of legalization cluster ids. Stores the cluster ids at
    ///        that tile location and can be accessed in the format of [layer][x][y].
    ///        This is stored to be used in the neighbor pass.
    vtr::NdMatrix<std::unordered_set<LegalizationClusterId>, 3> tile_clusters_matrix;

    /**
     * @brief Helper method to sort and group molecules by desired tile location.
     *        It first sorts by being in a long carry chain, then by external input
     *        pin count.
     * @return Mapping from tile location to sorted vector of molecules that
     *         want to be in that tile.
     */
    std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>
    sort_and_group_blocks_by_tile(const PartialPlacement& p_placement);

    /**
     * @brief Helper method to create clusters at a given tile location using
     *        given vector of molecules.
     *
     * Iterates over each subtile in the same order each time, hence trying to
     * create the fewest clusters in that tile. It also checks the compatibility
     * of the molecules with the tile before creating a cluster. Stores the cluster
     * ids' to check their legality or clean afterwards if needed.
     *
     *  @param tile_loc                        The physical tile location that clusters aimed to be created.
     *  @param tile_type                       The physical type of the tile that clusters aimed to be created.
     *  @param tile_molecules                  A vector of molecule ids aimed to be placed in that tile.
     *  @param cluster_legalizer               The cluster legalizer which is used to create and grow clusters.
     *  @param primitive_candidate_block_types A list of candidate block types for the given molecule to create a cluster.
     *  @return The set of LegalizationClusterIds created in that tile.
     */
    std::unordered_set<LegalizationClusterId>
    cluster_molecules_in_tile(const t_physical_tile_loc& tile_loc,
                              const t_physical_tile_type_ptr& tile_type,
                              const std::vector<PackMoleculeId>& tile_molecules,
                              ClusterLegalizer& cluster_legalizer,
                              const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

    /**
     * @brief Helper method to perform self clustering pass.
     *
     * Iterates over each tile and first tries to create the fewest clusters
     * in that tile with SKIP_INTRA_LB_ROUTE strategy. If the resulting
     * cluster is found to be unroutable when fully checked, retry adding the
     * molecules with the FULL strategy before going to next tile.
     *
     *  @param cluster_legalizer               The cluster legalizer which is used to create and grow clusters. The result of
     *                                         this pass is an updated cluster_legalizer.
     *  @param device_grid                     The device grid used to get physical tile types.
     *  @param primitive_candidate_block_types A list of candidate block types for the given molecule to create a cluster.
     *  @param tile_blocks                     The list of molecules to pack in each non-empty tile.
     */
    void self_clustering(ClusterLegalizer& cluster_legalizer,
                         const DeviceGrid& device_grid,
                         const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                         std::unordered_map<t_physical_tile_loc, std::vector<PackMoleculeId>>& tile_blocks);

    /**
     * @brief Helper method to perform neighbor clustering.
     *
     * For each unclustered molecule, examines clusters in the 8 neighboring tiles.
     * Neighbor tiles are processed in order of increasing average molecule density.
     * The molecule is then added to an existing cluster if compatible. No new
     * clusters are created in this pass.
     *
     *  @return The set of molecule ids clustered in that pass.
     */
    std::unordered_set<PackMoleculeId>
    neighbor_clustering(ClusterLegalizer& cluster_legalizer,
                        const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

    /**
     * @brief Helper method to perform orphan window clustering.
     *
     * Iteratively selects a seed orphan molecule (highest external input pins).
     * From the seed’s assigned location, expands within the given Manhattan
     * search radius in a BFS manner to find nearby orphan molecules. Compatible
     * neighbors are added to the seed’s cluster in order of proximity. After
     * reaching the search radius, the process repeats with new seeds until all
     * orphan molecules are clustered.
     *
     *  @param cluster_legalizer               The cluster legalizer which is used to create and grow clusters.
     *  @param primitive_candidate_block_types A list of candidate block types for the given molecule to create a cluster.
     *  @param search_radius                   The search radius that determines the allowed max distance from the seed
     *                                         molecule to candidate molecules.
     *  @return The set of LegalizationClusterIds created in that pass.
     */
    std::unordered_set<LegalizationClusterId>
    orphan_window_clustering(ClusterLegalizer& cluster_legalizer,
                             const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                             int search_radius);
    /**
     * @brief Helper method to report the clustering summary.
     */
    void report_clustering_summary(ClusterLegalizer& cluster_legalizer,
                                   std::unordered_set<PackMoleculeId>& neighbor_pass_molecules,
                                   std::unordered_set<LegalizationClusterId>& orphan_window_clusters);

    /**
     * @brief Helper method to create clusters with self, neighbor, and orphan window clustering.
     *
     *  @param cluster_legalizer The cluster legalizer which is used to create and grow clusters. Keeps track of
     *                           the clusters created and molecules clustered while checking cluster legality.
     *  @param p_placement       The partial placement used to guide where each molecule should be placed.
     */
    void create_clusters(ClusterLegalizer& cluster_legalizer,
                         const PartialPlacement& p_placement);

    /**
     * @brief Helper method to perform initial placement on clusters created.
     *
     * Each cluster is placed by the initial placer at the grid location nearest
     * to the centroid of its atoms.
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
