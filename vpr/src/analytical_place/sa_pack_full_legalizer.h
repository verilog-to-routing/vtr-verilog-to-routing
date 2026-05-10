#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2026
 * @brief   Defines the SAPack full legalizer class and its supporting types.
 */

#include <optional>
#include <vector>
#include "cluster_legalizer.h"
#include "full_legalizer.h"
#include "logic_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"

class DeviceGrid;
struct PartialPlacement;

/**
 * @brief State of a single cluster slot in the SAPack device grid.
 */
struct SAPackCluster {
    /// @brief The legalization cluster occupying this sub-tile slot.
    LegalizationClusterId cluster_id = LegalizationClusterId::INVALID();
    /// @brief Whether this cluster has passed full intra-LB routing legality.
    bool is_finalized = false;
};

/**
 * @brief Per-tile cluster state for the SAPack legalizer.
 *
 * Wraps a 3-D NdMatrix indexed by (layer, x, y), where each entry is a vector
 * of SAPackCluster objects sized to the tile's sub-tile capacity. The sizing
 * and initialization of each root-tile entry is performed in the constructor,
 * keeping that bookkeeping out of the legalizer itself.
 */
class SAPackDeviceGrid {
  public:
    /// @brief Construct and initialize cluster slots from the given device grid.
    explicit SAPackDeviceGrid(const DeviceGrid& device_grid);

    /// @brief Return the cluster slots for the given tile location.
    std::vector<SAPackCluster>& get_clusters(t_physical_tile_loc tile_loc);
    const std::vector<SAPackCluster>& get_clusters(t_physical_tile_loc tile_loc) const;

  private:
    vtr::NdMatrix<std::vector<SAPackCluster>, 3> clusters_;
};

/**
 * @brief SAPack: An experimental full legalizer that greedily places molecules
 *        into tiles based on the partial placement, then iteratively legalizes
 *        the resulting clusters using full intra-LB routing checks.
 */
class SAPack : public FullLegalizer {
  public:
    using FullLegalizer::FullLegalizer;

    void legalize(const PartialPlacement& p_placement) final;

  private:
    /// @brief Per-tile cluster state, initialized at the start of legalize().
    std::optional<SAPackDeviceGrid> sa_pack_grid_;

    /// @brief The cluster legalizer, initialized at the start of legalize().
    std::optional<ClusterLegalizer> cluster_legalizer_;

    /// @brief Candidate cluster types for each primitive model.
    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types_;

    /**
     * @brief Try to place the given molecule into the given tile.
     *
     * Iterates over each sub-tile. Tries to add the molecule to an existing
     * non-finalized cluster, or creates a new cluster if the slot is empty.
     *
     * @return True if the molecule was successfully placed, false otherwise.
     */
    bool try_place_mol_in_tile(PackMoleculeId mol_id, t_physical_tile_loc tile_loc);

    /**
     * @brief Try to place the given molecule into the nearest compatible tile,
     *        starting from tile_loc and expanding outward via BFS.
     *
     * @return True if the molecule was successfully placed, false otherwise.
     */
    bool try_place_mol_in_nearest_tile(PackMoleculeId mol_id, t_physical_tile_loc tile_loc);

    /**
     * @brief Place all molecules from the AP netlist into pin-counting-legal
     *        clusters at their desired tile locations.
     *
     * First tries to place each molecule directly into its desired tile. Any
     * molecules that cannot be placed in their desired tile are then placed in
     * the nearest compatible tile via BFS.
     */
    void place_molecules(const PartialPlacement& p_placement);

    /**
     * @brief Finalize the cluster by performing a full intra-lb route.
     *
     * If the cluster fails to legalize as is, it will reset itself and
     * insert the molecules one at a time at the same logical block type.
     *
     * Returns a list of molecules which were rejected by the cluster during
     * full legalization. If the cluster was fully legal, this list will be
     * empty.
     */
    std::vector<PackMoleculeId> finalize_cluster(SAPackCluster& cluster);

    /**
     * @brief Fully legalize clusters on the grid.
     *
     * Any illegalities will be solved by moving molecules the minimum
     * amount from where they presently are.
     */
    void fully_legalize_placement();
};
