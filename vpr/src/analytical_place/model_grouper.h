/**
 * @file
 * @author  Alex Singer
 * @date    March 2025
 * @brief   Declaration of a model grouper class which groups together models
 *          that must be legalized together in a flat placement.
 */

#pragma once

#include <vector>
#include "vtr_assert.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

// Forward declarations.
class Prepacker;
struct t_model;

/// @brief Tag for the ModelGroupId
struct model_group_id_tag;

/// @brief A unique ID of a group of models created by the ModelGrouper class.
typedef vtr::StrongId<model_group_id_tag, size_t> ModelGroupId;

/**
 * @brief A manager class for grouping together models that must be legalized
 *        together in a flat placement due to how they form molecules with each
 *        other.
 *
 * When performing legalization of a flat placement, it is desirable to split
 * the problem into independent legalization problems. We cannot place all of
 * the blocks of different model types independently since some blocks are made
 * of multiple different types of models. We wish to find the minimum number of
 * models that we need to legalize at the same time.
 *
 * This class groups models together based on the pack patterns that they can
 * form in the prepacker. If model A and model B can form a pack pattern, and
 * model B and model C can form a pack pattern, then models A, B, and C form a
 * group and must be legalized together.
 *
 * This class also manages what models each group contains and the group of each
 * model, where the user can use IDs to get relavent information.
 */
class ModelGrouper {
  public:
    // Iterator for the model group IDs
    typedef typename vtr::vector_map<ModelGroupId, ModelGroupId>::const_iterator group_iterator;

    // Range for the model group IDs
    typedef typename vtr::Range<group_iterator> group_range;

  public:
    ModelGrouper() = delete;

    /**
     * @brief Constructor for the model grouper class. Groups are formed here.
     *
     *  @param prepacker
     *      The prepacker used to create molecules in the flat placement. This
     *      provides the pack patterns for forming the groups.
     *  @param user_models
     *      Linked list of user-provided models.
     *  @param library_models
     *      Linked list of library models.
     *  @param log_verbosity
     *      The verbosity of log messages in the grouper class.
     */
    ModelGrouper(const Prepacker& prepacker,
                 t_model* user_models,
                 t_model* library_models,
                 int log_verbosity);

    /**
     * @brief Returns a list of all valid group IDs.
     */
    inline group_range groups() const {
        return vtr::make_range(group_ids_.begin(), group_ids_.end());
    }

    /**
     * @brief Gets the group ID of the given model.
     */
    inline ModelGroupId get_model_group_id(int model_index) const {
        VTR_ASSERT_SAFE_MSG(model_index < (int)model_group_id_.size(),
                            "Model index outside of range for model_group_id_");
        ModelGroupId group_id = model_group_id_[model_index];
        VTR_ASSERT_SAFE_MSG(group_id.is_valid(),
                            "Model is not in a group");
        return group_id;
    }

    /**
     * @brief Gets the models in the given group.
     */
    inline const std::vector<int>& get_models_in_group(ModelGroupId group_id) const {
        VTR_ASSERT_SAFE_MSG(group_id.is_valid(),
                            "Invalid group id");
        VTR_ASSERT_SAFE_MSG(groups_[group_id].size() != 0,
                            "Group is empty");
        return groups_[group_id];
    }

  private:
    /// @brief List of all group IDs.
    vtr::vector_map<ModelGroupId, ModelGroupId> group_ids_;

    /// @brief A lookup between models and the group ID that contains them.
    std::vector<ModelGroupId> model_group_id_;

    /// @brief A lookup between each group ID and the models in that group.
    vtr::vector<ModelGroupId, std::vector<int>> groups_;
};
