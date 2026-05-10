#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2026
 * @brief   Defines the SAPack full legalizer class.
 */

#include "cluster_legalizer.h"
#include "full_legalizer.h"
#include "logic_types.h"
#include "physical_types.h"
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
    /**
     * @brief Place all molecules from the AP netlist into pin-counting-legal
     *        clusters at their desired tile locations.
     *
     * First tries to place each molecule directly into its desired tile. Any
     * molecules that cannot be placed in their desired tile are then placed in
     * the nearest compatible tile via BFS.
     */
    void place_molecules(const PartialPlacement& p_placement,
                         vtr::NdMatrix<std::vector<LegalizationClusterId>, 3>& tile_clusters,
                         vtr::NdMatrix<std::vector<bool>, 3>& is_cluster_finalized,
                         ClusterLegalizer& cluster_legalizer,
                         const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);
};
