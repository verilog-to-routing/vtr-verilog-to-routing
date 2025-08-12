#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    August 2025
 * @brief   Declaration of a class that manages the unrelated clustering mode
 *          used within APPack.
 */

#include <string>
#include <vector>
#include "physical_types.h"
#include "vtr_assert.h"

/**
 * @brief Manager class for unrelated clustering in APPack.
 *
 * After searching for candidates by connectivity and timing, the user may
 * turn on unrelated clustering, which attempt to pack into the cluster molecules
 * which are not attracted to the molecules within the current cluster (according
 * to the standard packing heuristics). APPack uses flat placement information
 * to decide which unrelated molecules to try.
 *
 * APPack will search for unrelated molecules in the tile which contains
 * the flat location of the cluster.  It will then look farther out, tile
 * by tile. The distance that it will search up to is the max unrelated tile
 * distance.
 *
 * Unrelated clustering occurs after all other candidate selection methods
 * have failed. This attempts to cluster in molecules that are not attracted
 * (using the packer's heuristics) to the molecules within a given cluster.
 * The max unrelated clustering attempts sets how many times we will attempt unrelated
 * clustering between failures of unrelated clustering. If a molecule used
 * for unrelated clustering failed to cluster it will not be attempted
 * again for that cluster (note: if it succeeds, the number of attempts get
 * reset).
 * NOTE: A similar option exists in the candidate selector class. This was
 *       duplicated since it is very likely that APPack would need a
 *       different value for this option than the non-APPack flow.
 */
class APPackUnrelatedClusteringManager {

    /**
     * @brief The default distance each logical block will search for unrelated
     *        clusters. Setting this to 0 would only allow APPack to search
     *        within the cluster's tile. Setting this to a higher number would
     *        allow APPack to search farther away; but may bring in molecules
     *        which do not "want" to be in the cluster.
     */
    static constexpr float default_max_unrelated_tile_distance_ = 0.0f;

    /**
     * @brief the default number of unrelated clustering attempts each cluster
     *        type will perform.
     */
    static constexpr int default_max_unrelated_clustering_attempts_ = 1;

  public:
    /**
     * @brief When the packing is particularly difficult, and it has failed enough
     *        times, APPack will perform high-effort unrelated clustering. This will
     *        increase the search distance to the size of the device and increase
     *        the number of attempts to the following amount.
     */
    static constexpr int high_effort_max_unrelated_clustering_attempts_ = 12;

  public:
    APPackUnrelatedClusteringManager() = default;

    /**
     * @brief Initializer for the manager class. The search distance and max
     *        number of attempts are selected here.
     *
     *  @param unrelated_clustering_args
     *      An array of strings representing the user-defined unrelated clustering
     *      args. This allows the user to select default unrelated clustering
     *      behavior.
     *  @param logical_block_types
     *      A vector of all logical block types in the architecture.
     */
    void init(const std::vector<std::string>& unrelated_clustering_args,
              const std::vector<t_logical_block_type>& logical_block_types);

    /**
     * @brief Get the max search distance for the given logical block type.
     */
    inline float get_max_unrelated_tile_dist(const t_logical_block_type& logical_block_ty) const {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackUnrelatedClusteringManager has not been initialized "
                            ", cannot call this method.");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < max_unrelated_tile_distance_.size(),
                            "Logical block type does not have a max unrelated tile distance.");

        return max_unrelated_tile_distance_[logical_block_ty.index];
    }

    /**
     * @brief Get the max unrelated clustering attempts for the given logical block type.
     */
    inline int get_max_unrelated_clustering_attempts(const t_logical_block_type& logical_block_ty) const {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackUnrelatedClusteringManager has not been initialized "
                            ", cannot call this method.");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < max_unrelated_clustering_attempts_.size(),
                            "Logical block type does not have a max unrelated clustering attempts.");

        return max_unrelated_clustering_attempts_[logical_block_ty.index];
    }

    /**
     * @brief Set the max search distance for the given logical block type to the given value.
     */
    inline void set_max_unrelated_tile_dist(const t_logical_block_type& logical_block_ty,
                                            float new_max_unrelated_tile_dist) {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackUnrelatedClusteringManager has not been initialized "
                            ", cannot call this method.");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < max_unrelated_tile_distance_.size(),
                            "Logical block type does not have a max unrelated tile distance.");

        max_unrelated_tile_distance_[logical_block_ty.index] = new_max_unrelated_tile_dist;
    }

    /**
     * @brief Set the max unrelated clustering attempts for the given logical block type
     *        to the given value.
     */
    inline void set_max_unrelated_clustering_attempts(const t_logical_block_type& logical_block_ty,
                                                      int new_max_unrelated_clustering_attempts) {
        VTR_ASSERT_SAFE_MSG(is_initialized_,
                            "APPackUnrelatedClusteringManager has not been initialized "
                            ", cannot call this method.");
        VTR_ASSERT_SAFE_MSG((size_t)logical_block_ty.index < max_unrelated_clustering_attempts_.size(),
                            "Logical block type does not have a max unrelated clustering attempts.");


        max_unrelated_clustering_attempts_[logical_block_ty.index] = new_max_unrelated_clustering_attempts;
    }

  private:
    /// @brief A flag which shows if the data within this class has been initialized
    ///        or not.
    bool is_initialized_ = false;

    /// @brief The max unrelated tile distance of all logical blocks in the architecture.
    ///             [block_type_index] -> unrelated_tile_distance
    std::vector<float> max_unrelated_tile_distance_;

    /// @brief The max unrelated clustering attempts of all logical blocks in the architecture.
    ///             [block_type_index] -> max_unrelated_attempts
    std::vector<int> max_unrelated_clustering_attempts_;
};
