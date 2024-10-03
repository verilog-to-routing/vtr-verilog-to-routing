/*
 * Find placement for group of atom blocks in complex block
 * Author: Jason Luu
 */

#ifndef CLUSTER_PLACEMENT_H
#define CLUSTER_PLACEMENT_H

#include <vector>
#include <unordered_map>
#include "physical_types.h"
#include "vpr_types.h"

/**
 * @brief Stats keeper for placement within the cluster during packing
 *
 * Contains data structure of placement locations based on status of primitive
 */
class t_intra_cluster_placement_stats {
  public:
    int num_pb_types;                     ///<num primitive pb_types inside complex block
    bool has_long_chain;                  ///<specifies if this cluster has a molecule placed in it that belongs to a long chain (a chain that spans more than one cluster)
    const t_pack_molecule* curr_molecule; ///<current molecule being considered for packing

    // Vector of size num_pb_types [0.. num_pb_types-1]. Each element is an unordered_map of the cluster_placement_primitives that are of this pb_type
    // Each cluster_placement_primitive is associated with and index (key of the map) for easier lookup, insertion and deletion.
    std::vector<std::unordered_map<int, t_cluster_placement_primitive*>> valid_primitives;

  public:
    // Moves primitives that are inflight to the tried map
    void move_inflight_to_tried();

    /**
     * @brief Move the primitive at (it) to inflight and increment the current iterator.
     *
     * Because the element at (it) is deleted from valid_primitives, (it) is incremented to keep it valid and pointing at the next element.
     *
     * @param pb_type_index: is the index of this pb_type in valid_primitives vector
     * @param it: is the iterator pointing at the element that needs to be moved to inflight
     */
    void move_primitive_to_inflight(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it);

    /**
     * @brief Move the primitive at (it) to invalid and increment the current iterator
     *
     * Because the element at (it) is deleted from valid_primitives, (it) is incremented to keep it valid and pointing at the next element.
     *
     * @param  pb_type_index: is the index of this pb_type in valid_primitives vector
     * @param it: is the iterator pointing at the element that needs to be moved to invalid
     */
    void invalidate_primitive_and_increment_iterator(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it);

    /**
     * @brief Add a primitive in its correct location in valid_primitives vector based on its pb_type
     *
     * @param cluster_placement_primitive: a pair of the cluster_placement_primtive and its corresponding index(for reference in pb_graph_node)
     */
    void insert_primitive_in_valid_primitives(std::pair<int, t_cluster_placement_primitive*> cluster_placement_primitive);

    /**
     * @brief Move all the primitives from (in_flight and tried) maps to valid primitives and clear (in_flight and tried)
     */
    void flush_intermediate_queues();

    /**
     * @brief Move all the primitives from invalid to valid_primitives and clear the invalid map
     */
    void flush_invalid_queue();

    /**
     * @brief Return true if the in_flight map is empty (no primitive is in_flight currently)
     */
    bool in_flight_empty();

    /**
     * @brief Return the type of the first element of the primitives currently being considered
     */
    t_pb_type* in_flight_type();

    /**
     * @brief Set the placement primitive of the given pb_graph_node.
     *
     * The cluster placement primitive contains information about the molecules
     * which currently occupy the pb_graph_node.
     */
    inline void set_pb_graph_node_placement_primitive(const t_pb_graph_node* pb_graph_node,
                                                      t_cluster_placement_primitive* placement_primitive) {
        VTR_ASSERT_SAFE(pb_graph_node != nullptr);
        VTR_ASSERT_SAFE(placement_primitive != nullptr);
        pb_graph_node_placement_primitive[pb_graph_node] = placement_primitive;
    }

    /**
     * @brief Get the placement primitive of the given pb_graph_node.
     *
     * Assumes that the pb_graph_node was set previously.
     */
    inline t_cluster_placement_primitive* get_pb_graph_node_placement_primitive(const t_pb_graph_node* pb_graph_node) {
        VTR_ASSERT_SAFE(pb_graph_node != nullptr);
        VTR_ASSERT(pb_graph_node_placement_primitive.count(pb_graph_node) != 0);
        return pb_graph_node_placement_primitive[pb_graph_node];
    }

    /**
     * @brief free the dynamically allocated memory for primitives
     */
    void free_primitives();

  private:
    std::unordered_multimap<int, t_cluster_placement_primitive*> in_flight; ///<ptrs to primitives currently being considered to pack into
    std::unordered_multimap<int, t_cluster_placement_primitive*> tried;     ///<ptrs to primitives that are already tried but current logic block unable to pack to
    std::unordered_multimap<int, t_cluster_placement_primitive*> invalid;   ///<ptrs to primitives that are invalid (already occupied by another primitive in this cluster)

    /// @brief A mapping between pb_graph_nodes and the cluster placement primitive.
    ///
    /// TODO: This could be stored more efficiently if each t_pb_graph_node had a
    ///       unique ID per cluster.
    std::unordered_map<const t_pb_graph_node*, t_cluster_placement_primitive*> pb_graph_node_placement_primitive;

    /**
     * @brief iterate over elements of a queue and move its elements to valid_primitives
     *
     * @param queue the unordered_multimap to work on (e.g. in_flight, tried, or invalid)
     */
    void flush_queue(std::unordered_multimap<int, t_cluster_placement_primitive*>& queue);
};

/**
 * @brief Allocates and loads the cluster placement stats for a cluster of the
 *        given cluster type and mode.
 *
 * The pointer returned by this method must be freed.
 */
t_intra_cluster_placement_stats* alloc_and_load_cluster_placement_stats(t_logical_block_type_ptr cluster_type,
                                                                  int cluster_mode);

/**
 * @brief Frees the cluster placement stats of a cluster.
 */
void free_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats);

/**
 * get next list of primitives for list of atom blocks
 *
 * primitives is the list of ptrs to primitives that matches with the list of atom block, assumes memory is preallocated
 *   - if this is a new block, requeue tried primitives and return a in-flight primitive list to try
 *   - if this is an old block, put root primitive to tried queue, requeue rest of primitives. try another set of primitives
 *
 * return true if can find next primitive, false otherwise
 *
 * cluster_placement_stats - ptr to the current cluster_placement_stats of open complex block
 * molecule - molecule to pack into open complex block
 * primitives_list - a list of primitives indexed to match atom_block_ids of molecule.
 *                   Expects an allocated array of primitives ptrs as inputs.
 *                   This function loads the array with the lowest cost primitives that implement molecule
 * force_site - optional user-specified primitive site on which to place the molecule; if a force_site
 *              argument is provided, the function either selects the specified site or reports failure.
 *              If the force_site argument is set to its default value (-1), vpr selects an available site.
 */
bool get_next_primitive_list(
    t_intra_cluster_placement_stats* cluster_placement_stats,
    const t_pack_molecule* molecule,
    t_pb_graph_node** primitives_list,
    int force_site = -1);

/**
 * @brief Commit primitive, invalidate primitives blocked by mode assignment and update costs for primitives in same cluster as current
 * Costing is done to try to pack blocks closer to existing primitives
 *  actual value based on closest common ancestor to committed placement, the farther the ancestor, the less reduction in cost there is
 * Side effects: All cluster_placement_primitives may be invalidated/costed in this algorithm
 *               Al intermediate queues are requeued
 */
void commit_primitive(t_intra_cluster_placement_stats* cluster_placement_stats,
                      const t_pb_graph_node* primitive);

/**
 * @brief Determine max index + 1 of molecule
 */
int get_array_size_of_molecule(const t_pack_molecule* molecule);

/**
 * @brief Given atom block, determines if a free primitive exists for it,
 */
bool exists_free_primitive_for_atom_block(
    t_intra_cluster_placement_stats* cluster_placement_stats,
    const AtomBlockId blk_id);

void reset_tried_but_unused_cluster_placements(
    t_intra_cluster_placement_stats* cluster_placement_stats);

#endif
