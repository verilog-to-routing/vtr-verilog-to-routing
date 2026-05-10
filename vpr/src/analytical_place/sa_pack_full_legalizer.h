#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2026
 * @brief   Defines the SAPack full legalizer class.
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

struct PartialPlacement;

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
    /// @brief Clusters at each tile location indexed as [layer][x][y][sub_tile].
    vtr::NdMatrix<std::vector<LegalizationClusterId>, 3> tile_clusters_;

    /// @brief Tracks which sub-tile slots have been fully legalized.
    vtr::NdMatrix<std::vector<bool>, 3> is_cluster_finalized_;

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
};
