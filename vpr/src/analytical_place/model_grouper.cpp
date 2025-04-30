/**
 * @file
 * @author  Alex Singer
 * @date    March 2025
 * @brief   Implementation of a model grouper class which groups models together
 *          which must be legalized together in a flat placement.
 */

#include "model_grouper.h"
#include <queue>
#include <unordered_set>
#include <vector>
#include "cad_types.h"
#include "logic_types.h"
#include "prepack.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_vector.h"

/**
 * @brief Recursive helper function which gets the models in the given pattern
 *        block.
 *
 *  @param pattern_block
 *      The pattern block to get the models of.
 *  @param models
 *      A set of the models found so far.
 *  @param block_visited
 *      A vector of flags for each pattern block to signify which blocks have
 *      been visited.
 */
static void get_pattern_models_recurr(t_pack_pattern_block* pattern_block,
                                      std::unordered_set<LogicalModelId>& models,
                                      std::vector<bool>& block_visited) {
    // If the pattern block is invalid or this block has been visited, return.
    if (pattern_block == nullptr || block_visited[pattern_block->block_id]) {
        return;
    }

    // Mark this block as visited and insert its model into the models vector.
    block_visited[pattern_block->block_id] = true;
    models.insert(pattern_block->pb_type->model_id);

    // Go through this block's connections and get their pattern models.
    t_pack_pattern_connections* connection = pattern_block->connections;
    while (connection != nullptr) {
        get_pattern_models_recurr(connection->from_block, models, block_visited);
        get_pattern_models_recurr(connection->to_block, models, block_visited);
        connection = connection->next;
    }
}

/**
 * @brief Entry point into the recursive function above. Gets the models in
 *        the given pack pattern.
 */
static std::unordered_set<LogicalModelId> get_pattern_models(const t_pack_patterns& pack_pattern) {
    std::unordered_set<LogicalModelId> models_in_pattern;

    // Initialize the visited flags for each block to false.
    std::vector<bool> block_visited(pack_pattern.num_blocks, false);
    // Begin the recursion with the root block.
    get_pattern_models_recurr(pack_pattern.root_block, models_in_pattern, block_visited);

    return models_in_pattern;
}

ModelGrouper::ModelGrouper(const Prepacker& prepacker,
                           const LogicalModels& models,
                           int log_verbosity) {
    /**
     * Group the models together based on their pack patterns. If model A and
     * model B form a pattern, and model B and model C form a pattern, then
     * models A, B, and C are in a group together.
     *
     * An efficient way to find this is to represent this problem as a graph,
     * where each node is a model and each edge is a relationship where a model
     * is in a pack pattern with another model. We can then perform BFS to find
     * the connected sub-graphs which will be the groups.
     */

    // Create an adjacency list for the edges. An edge is formed where two
    // models share a pack pattern together.
    size_t num_models = models.all_models().size();
    vtr::vector<LogicalModelId, std::unordered_set<LogicalModelId>> adj_list(num_models);
    for (const t_pack_patterns& pack_pattern : prepacker.get_all_pack_patterns()) {
        // Get the models within this pattern.
        auto models_in_pattern = get_pattern_models(pack_pattern);
        VTR_ASSERT_SAFE(!models_in_pattern.empty());

        // Debug print the models within the pattern.
        if (log_verbosity >= 20) {
            VTR_LOG("Pattern: %s\n\t", pack_pattern.name);
            for (LogicalModelId model_id : models_in_pattern) {
                VTR_LOG("%s ", models.model_name(model_id).c_str());
            }
            VTR_LOG("\n");
        }

        // Connect each of the models to the first model in the pattern. Since
        // we only care if there exist a path from each model to another, we do
        // not need to connect the models in a clique.
        LogicalModelId first_model_idx = *models_in_pattern.begin();
        for (LogicalModelId model_idx : models_in_pattern) {
            adj_list[model_idx].insert(first_model_idx);
            adj_list[first_model_idx].insert(model_idx);
        }
    }

    // Perform BFS to group the models.
    VTR_LOGV(log_verbosity >= 20,
             "Finding model groups...\n");
    std::queue<LogicalModelId> node_queue;
    model_group_id_.resize(num_models, ModelGroupId::INVALID());
    for (LogicalModelId model_id : models.all_models()) {
        // If this model is already in a group, skip it.
        if (model_group_id_[model_id].is_valid()) {
            VTR_LOGV(log_verbosity >= 20,
                     "\t(%zu -> %zu)\n", model_id, model_group_id_[model_id]);
            continue;
        }

        ModelGroupId group_id = ModelGroupId(group_ids_.size());
        // Put the model in this group and push to the queue.
        model_group_id_[model_id] = group_id;
        node_queue.push(model_id);

        while (!node_queue.empty()) {
            // Pop a node from the queue, and explore its neighbors.
            LogicalModelId node_model_id = node_queue.front();
            node_queue.pop();
            for (LogicalModelId neighbor_model_id : adj_list[node_model_id]) {
                // If this neighbor is already in this group, skip it.
                if (model_group_id_[neighbor_model_id].is_valid()) {
                    VTR_ASSERT_SAFE(model_group_id_[neighbor_model_id] == group_id);
                    continue;
                }
                // Put the neighbor in this group and push it to the queue.
                model_group_id_[neighbor_model_id] = group_id;
                node_queue.push(neighbor_model_id);
            }
        }

        VTR_LOGV(log_verbosity >= 20,
                 "\t(%zu -> %zu)\n", model_id, model_group_id_[model_id]);
        group_ids_.push_back(group_id);
    }

    // Create a lookup between each group and the models it contains.
    groups_.resize(groups().size());
    for (LogicalModelId model_id : models.all_models()) {
        groups_[model_group_id_[model_id]].push_back(model_id);
    }

    // Debug printing for each group.
    if (log_verbosity >= 20) {
        for (ModelGroupId group_id : groups()) {
            const std::vector<LogicalModelId>& group = groups_[group_id];
            VTR_LOG("Group %zu:\n", group_id);
            VTR_LOG("\tSize = %zu\n", group.size());
            VTR_LOG("\tContained models:\n");
            for (LogicalModelId model_id : group) {
                VTR_LOG("\t\t%s\n", models.model_name(model_id).c_str());
            }
        }
    }
}
