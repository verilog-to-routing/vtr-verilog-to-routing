/*
 * Given a group of atom blocks and a partially-packed complex block, find placement for group of atom blocks in complex block
 * To use, keep "cluster_placement_stats" data structure throughout packing
 * cluster_placement_stats undergoes these major states:
 * Initialization - performed once at beginning of packing
 * Reset          - reset state in between packing of clusters
 * In flight      - Speculatively place
 *
 *
 * Finalized      - Commit or revert placements
 * Freed          - performed once at end of packing
 *
 * Author: Jason Luu
 * March 12, 2012
 *
 * Refactored by Mohamed Elgammal
 * November 17, 2022
 */

#include "cluster_placement.h"
#include "hash.h"
#include "physical_types.h"
#include "prepack.h"
#include "vpr_utils.h"
#include "vtr_assert.h"

/****************************************/
/*Local Function Declaration			*/
/****************************************/

static void load_cluster_placement_stats_for_pb_graph_node(t_intra_cluster_placement_stats* cluster_placement_stats,
                                                           t_pb_graph_node* pb_graph_node);

static void reset_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats);

static void set_mode_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats,
                                             const t_pb_graph_node* pb_graph_node,
                                             int mode);

static void update_primitive_cost_or_status(t_intra_cluster_placement_stats* cluster_placement_stats,
                                            const t_pb_graph_node* pb_graph_node,
                                            float incremental_cost,
                                            bool valid);

static bool expand_forced_pack_molecule_placement(t_intra_cluster_placement_stats* cluster_placement_stats,
                                                  PackMoleculeId molecule_id,
                                                  const t_pack_pattern_block* pack_pattern_block,
                                                  std::vector<t_pb_graph_node*>& primitives_list,
                                                  const Prepacker& prepacker,
                                                  float* cost);

static t_pb_graph_pin* expand_pack_molecule_pin_edge(int pattern_id,
                                                     const t_pb_graph_pin* cur_pin,
                                                     bool forward);

/****************************************/
/*Function Definitions					*/
/****************************************/

void t_intra_cluster_placement_stats::move_inflight_to_tried() {
    tried.insert(*in_flight.begin());
    in_flight.clear();
}

void t_intra_cluster_placement_stats::invalidate_primitive_and_increment_iterator(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it) {
    invalid.insert(*it);
    valid_primitives[pb_type_index].erase(it++);
}

void t_intra_cluster_placement_stats::move_primitive_to_inflight(int pb_type_index, std::unordered_multimap<int, t_cluster_placement_primitive*>::iterator& it) {
    in_flight.insert(*it);
    valid_primitives[pb_type_index].erase(it);
}

/**
 * @brief Put primitive back on the correct location of valid primitives vector based on the primitive pb type
 *
 * @note that valid status is not changed because if the primitive is not valid, it will get properly collected later
 */
void t_intra_cluster_placement_stats::insert_primitive_in_valid_primitives(std::pair<int, t_cluster_placement_primitive*> cluster_placement_primitive) {
    int i;
    bool success = false;
    int null_index = UNDEFINED;
    t_cluster_placement_primitive* input_cluster_placement_primitive = cluster_placement_primitive.second;

    for (i = 0; i < num_pb_types && !success; i++) {
        if (valid_primitives[i].empty()) {
            null_index = i;
            continue;
        }
        t_cluster_placement_primitive* cur_cluster_placement_primitive = valid_primitives[i].begin()->second;
        if (input_cluster_placement_primitive->pb_graph_node->pb_type
            == cur_cluster_placement_primitive->pb_graph_node->pb_type) {
            success = true;
            valid_primitives[i].insert(cluster_placement_primitive);
        }
    }
    if (!success) {
        VTR_ASSERT(null_index != UNDEFINED);
        valid_primitives[null_index].insert(cluster_placement_primitive);
    }
}

void t_intra_cluster_placement_stats::flush_queue(std::unordered_multimap<int, t_cluster_placement_primitive*>& queue) {
    for (auto& it : queue) {
        insert_primitive_in_valid_primitives(it);
    }
    queue.clear();
}

void t_intra_cluster_placement_stats::flush_intermediate_queues() {
    flush_queue(in_flight);
    flush_queue(tried);
}

void t_intra_cluster_placement_stats::flush_invalid_queue() {
    flush_queue(invalid);
}

bool t_intra_cluster_placement_stats::in_flight_empty() {
    return in_flight.empty();
}

t_pb_type* t_intra_cluster_placement_stats::in_flight_type() {
    return in_flight.begin()->second->pb_graph_node->pb_type;
}

void t_intra_cluster_placement_stats::free_primitives() {
    for (auto& primitive : tried)
        delete primitive.second;

    for (auto& primitive : in_flight)
        delete primitive.second;

    for (auto& primitive : invalid)
        delete primitive.second;

    for (int j = 0; j < num_pb_types; j++) {
        for (auto& primitive : valid_primitives[j]) {
            delete primitive.second;
        }
    }
}

t_intra_cluster_placement_stats* alloc_and_load_cluster_placement_stats(t_logical_block_type_ptr cluster_type,
                                                                        int cluster_mode) {
    t_intra_cluster_placement_stats* cluster_placement_stats = new t_intra_cluster_placement_stats;
    *cluster_placement_stats = t_intra_cluster_placement_stats();
    // TODO: This initialization may be able to be made more efficient.
    //       The reset and setting the mode can be done while loading the placement
    //       stats.
    if (!is_empty_type(cluster_type)) {
        cluster_placement_stats->curr_molecule = PackMoleculeId::INVALID();
        load_cluster_placement_stats_for_pb_graph_node(cluster_placement_stats,
                                                       cluster_type->pb_graph_head);
    }
    reset_cluster_placement_stats(cluster_placement_stats);
    set_mode_cluster_placement_stats(cluster_placement_stats,
                                     cluster_type->pb_graph_head,
                                     cluster_mode);
    return cluster_placement_stats;
}

void free_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats) {
    if (cluster_placement_stats != nullptr) {
        cluster_placement_stats->free_primitives();
        delete cluster_placement_stats;
    }
}

float try_place_molecule(t_intra_cluster_placement_stats* cluster_placement_stats,
                                PackMoleculeId molecule_id,
                                t_pb_graph_node* root,
                                std::vector<t_pb_graph_node*>& primitives_list,
                                const Prepacker& prepacker) {
    float cost = std::numeric_limits<float>::max();
    const t_pack_molecule& molecule = prepacker.get_molecule(molecule_id);
    size_t list_size = molecule.atom_block_ids.size();

    if (primitive_type_feasible(molecule.atom_block_ids[molecule.root],
                                root->pb_type)) {
        t_cluster_placement_primitive* root_placement_primitive = cluster_placement_stats->get_pb_graph_node_placement_primitive(root);
        if (root_placement_primitive->valid) {
            for (size_t i = 0; i < list_size; i++) {
                primitives_list[i] = nullptr;
            }
            cost = root_placement_primitive->base_cost
                   + root_placement_primitive->incremental_cost;
            primitives_list[molecule.root] = root;
            if (molecule.type == e_pack_pattern_molecule_type::MOLECULE_FORCED_PACK) {
                if (!expand_forced_pack_molecule_placement(cluster_placement_stats,
                                                           molecule_id,
                                                           molecule.pack_pattern->root_block,
                                                           primitives_list,
                                                           prepacker,
                                                           &cost)) {
                    return std::numeric_limits<float>::max();
                }
            }
            for (size_t i = 0; i < list_size; i++) {
                VTR_ASSERT((primitives_list[i] == nullptr) == (!molecule.atom_block_ids[i]));
                for (size_t j = 0; j < list_size; j++) {
                    if (i != j) {
                        if (primitives_list[i] != nullptr && primitives_list[i] == primitives_list[j]) {
                            return std::numeric_limits<float>::max();
                        }
                    }
                }
            }
        }
    }
    return cost;
}

LazyPopUniquePriorityQueue<t_pb_graph_node*, std::tuple<float,int,int>> build_primitive_candidate_queue(t_intra_cluster_placement_stats* cluster_placement_stats,
                                                                                                        PackMoleculeId molecule_id,
                                                                                                        std::vector<t_pb_graph_node*>& primitives_list,
                                                                                                        const Prepacker& prepacker,
                                                                                                        int force_site) {
    LazyPopUniquePriorityQueue<t_pb_graph_node*, std::tuple<float,int,int>> primitives_alive;
    if (cluster_placement_stats->curr_molecule != molecule_id) {
        // New block, requeue tried primitives and in-flight primitives
        cluster_placement_stats->flush_intermediate_queues();
        cluster_placement_stats->curr_molecule = molecule_id;
    } else {
        // Hack! Same failed molecule may re-enter if upper stream functions suck,
        // I'm going to make the molecule selector more intelligent.
        // TODO: Remove later
        if (!cluster_placement_stats->in_flight_empty()) {
            // Hack end
            // old block, put root primitive currently inflight to tried queue
            cluster_placement_stats->move_inflight_to_tried();
        }
    }

    // Build a priority queue of feasible primitive roots for this molecule.
    // Behavior:
    //   1. Lazily remove invalid primitives from valid_primitives.
    //   2. For each primitive type that is compatible with the molecule root:
    //        - Attempt placement using try_place_molecule().
    //        - If feasible, insert the root into the priority queue.
    //   3. The priority key is:
    //        (-cost, total_primitive_count, -encounter_order)
    //      which ensures:
    //        - Lower placement cost is preferred.
    //        - Higher total_primitive_count breaks ties.
    //        - Earlier encountered primitives break remaining ties.
    //   4. If force_site >= 0:
    //        - Only the primitive matching flat_site_index == force_site
    //          is considered.
    //        - The returned priority queue contains at most one element.
    //        - If the forced site is infeasible, the returned priority queue is empty.
    // The returned priority queue may be empty if no feasible primitive exists.

    // Initialize variables
    float cost = std::numeric_limits<float>::max();
    int encounter_order = 0;

    // Iterate over each primitive block type in the current cluster_placement_stats
    for (int i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        if (!cluster_placement_stats->valid_primitives[i].empty()) {
            t_cluster_placement_primitive* cur_cluster_placement_primitive = cluster_placement_stats->valid_primitives[i].begin()->second;
            const t_pack_molecule& molecule = prepacker.get_molecule(molecule_id);
            if (primitive_type_feasible(molecule.atom_block_ids[molecule.root], cur_cluster_placement_primitive->pb_graph_node->pb_type)) {
                // Iterate over the unordered_multimap of the valid primitives of a specific pb primitive type
                for (auto it = cluster_placement_stats->valid_primitives[i].begin(); it != cluster_placement_stats->valid_primitives[i].end(); /*loop increment is done inside the loop*/) {
                    // Lazily remove invalid primitives
                    if (!it->second->valid) {
                        cluster_placement_stats->invalidate_primitive_and_increment_iterator(i, it); //iterator is incremented here
                        continue;
                    }

                    // Check for force site match, if applicable
                    if (force_site > -1) {
                        // Check that the forced site index is within the available range
                        int max_site = it->second->pb_graph_node->total_primitive_count - 1;
                        if (force_site > max_site) {
                            VTR_LOG("The specified primitive site (%d) is out of range (max %d)\n",
                                    force_site, max_site);
                            break;
                        }
                        if (force_site == it->second->pb_graph_node->flat_site_index) {
                            cost = try_place_molecule(cluster_placement_stats,
                                                      molecule_id,
                                                      it->second->pb_graph_node,
                                                      primitives_list,
                                                      prepacker);
                            primitives_alive.clear();
                            if (cost < std::numeric_limits<float>::max()) {
                                int total_primitive_count = it->second->pb_graph_node->total_primitive_count;
                                primitives_alive.push(it->second->pb_graph_node,
                                                      std::make_tuple(-cost, total_primitive_count, -encounter_order));
                            }
                            return primitives_alive;
                        } else {
                            ++it;
                            continue;
                        }
                    }

                    // Try place molecule at current root location
                    cost = try_place_molecule(cluster_placement_stats,
                                              molecule_id,
                                              it->second->pb_graph_node,
                                              primitives_list,
                                              prepacker);

                    if (cost < std::numeric_limits<float>::max()) {
                        int total_primitive_count = it->second->pb_graph_node->total_primitive_count;
                        primitives_alive.push(it->second->pb_graph_node,
                                              std::make_tuple(-cost, total_primitive_count, -encounter_order));
                        ++encounter_order;
                    }
                    ++it;
                }
            }
        }
    }

    return primitives_alive;
}

bool move_root_node_to_inflight(t_intra_cluster_placement_stats* cluster_placement_stats,
                                t_pb_graph_node* target_root) {
    for (int pb_type_idx = 0; pb_type_idx < cluster_placement_stats->num_pb_types; ++pb_type_idx) {
        auto& valid_primitives = cluster_placement_stats->valid_primitives[pb_type_idx];
        for (auto it = valid_primitives.begin(); it != valid_primitives.end(); /* increment handled inside */) {
            t_cluster_placement_primitive* placement_primitive = it->second;

            // Lazily remove invalid primitives
            if (!placement_primitive->valid) {
                cluster_placement_stats->invalidate_primitive_and_increment_iterator(pb_type_idx, it);
                continue;
            }

            // Found the requested root primitive
            if (placement_primitive->pb_graph_node == target_root) {
                cluster_placement_stats->move_primitive_to_inflight(pb_type_idx, it);
                return true;
            }
            ++it;
        }
    }

    // Root not found among valid primitives
    return false;
}

bool try_start_root_placement(t_intra_cluster_placement_stats* cluster_placement_stats,
                              PackMoleculeId molecule_id,
                              t_pb_graph_node* root,
                              std::vector<t_pb_graph_node*>& primitives_list,
                              const Prepacker& prepacker) {
    t_cluster_placement_primitive* root_prim =
        cluster_placement_stats->get_pb_graph_node_placement_primitive(root);

    // Skip stale entries
    if (!root_prim->valid) {
        return false;
    }

    // Move valid to inflight
    if (!move_root_node_to_inflight(cluster_placement_stats, root)) {
        return false;
    }

    // Re-run try_place_molecule() to populate primitives_list for the
    // actual placement attempt. This is required even though the root
    // was scored earlier when building the candidate queue, since the
    // earlier call does not persist primitives_list state.
    float cost = try_place_molecule(cluster_placement_stats,
                                    molecule_id,
                                    root,
                                    primitives_list,
                                    prepacker);

    VTR_ASSERT_MSG(cost != std::numeric_limits<float>::max(),
                   "The cost of the molecule placement cannot be infinite after validity check.");

    return true;
}

/**
 * Resets one cluster placement stats by clearing incremental costs and returning all primitives to valid queue
 */
static void reset_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats) {
    int i;

    /* Requeue primitives */
    cluster_placement_stats->flush_intermediate_queues();
    cluster_placement_stats->flush_invalid_queue();

    /* reset flags and cost */
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        for (auto& primitive : cluster_placement_stats->valid_primitives[i]) {
            primitive.second->incremental_cost = 0;
            primitive.second->valid = true;
        }
    }
    cluster_placement_stats->curr_molecule = PackMoleculeId::INVALID();
    cluster_placement_stats->has_long_chain = false;
}

/**
 * Add any primitives found in pb_graph_nodes to cluster_placement_stats
 * Adds backward link from pb_graph_node to cluster_placement_primitive
 */
static void load_cluster_placement_stats_for_pb_graph_node(t_intra_cluster_placement_stats* cluster_placement_stats,
                                                           t_pb_graph_node* pb_graph_node) {
    int i, j, k;
    t_cluster_placement_primitive* placement_primitive;
    const t_pb_type* pb_type = pb_graph_node->pb_type;
    //if pb_graph_node is primitive
    if (pb_type->modes == nullptr) {
        placement_primitive = new t_cluster_placement_primitive();
        placement_primitive->pb_graph_node = pb_graph_node;
        placement_primitive->valid = true;
        cluster_placement_stats->set_pb_graph_node_placement_primitive(pb_graph_node, placement_primitive);
        placement_primitive->base_cost = compute_primitive_base_cost(pb_graph_node);

        bool success = false;
        /**
         * Insert the cluster_placement_primitive in the corresponding valid_primitives location based on its pb_type
         *
         *  - Iterate over the elements of valid_prmitives
         *  - There should be an element with index 0 (otherwise this element will not exist in the vector) --> find it
         *  - Check the pb_type of this element with the pb_type of pb_graph_node
         *      - if matched --> insert the primitive
         */
        for (auto& type_primitives : cluster_placement_stats->valid_primitives) {
            auto first_elem = type_primitives.find(0);
            if (first_elem != type_primitives.end() && first_elem->second->pb_graph_node->pb_type == pb_graph_node->pb_type) {
                type_primitives.insert({type_primitives.size(), placement_primitive});
                success = true;
                break;
            }
        }

        /**
         * If the loop is done and we didn't find a match, push an empty map into valid_primitives for this new type
         * and insert the placement primitive into the new map with index 0
         */
        if (!success) {
            cluster_placement_stats->valid_primitives.emplace_back();
            cluster_placement_stats->valid_primitives[cluster_placement_stats->valid_primitives.size() - 1].insert({0, placement_primitive});
            cluster_placement_stats->num_pb_types++;
        }

    } else { // not a primitive, recursively call the function for all its children
        for (i = 0; i < pb_type->num_modes; i++) {
            for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb;
                     k++) {
                    load_cluster_placement_stats_for_pb_graph_node(cluster_placement_stats,
                                                                   &pb_graph_node->child_pb_graph_nodes[i][j][k]);
                }
            }
        }
    }
}

void commit_primitive(t_intra_cluster_placement_stats* cluster_placement_stats,
                      const t_pb_graph_node* primitive) {
    t_pb_graph_node *pb_graph_node, *skip;
    float incr_cost;
    int i, j, k;
    int valid_mode;
    t_cluster_placement_primitive* cur;

    /* Clear out intermediate queues */
    cluster_placement_stats->flush_intermediate_queues();

    /* commit primitive as used, invalidate it */
    cur = cluster_placement_stats->get_pb_graph_node_placement_primitive(primitive);
    VTR_ASSERT(cur->valid == true);

    cur->valid = false;
    incr_cost = -0.01; /* cost of using a node drops as its neighbours are used, this drop should be small compared to scarcity values */

    pb_graph_node = cur->pb_graph_node;
    /* walk up pb_graph_node and update primitives of children */
    while (!pb_graph_node->is_root()) {
        skip = pb_graph_node; /* do not traverse stuff that's already traversed */
        valid_mode = pb_graph_node->pb_type->parent_mode->index;
        pb_graph_node = pb_graph_node->parent_pb_graph_node;
        for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    if (&pb_graph_node->child_pb_graph_nodes[i][j][k] != skip) {
                        update_primitive_cost_or_status(cluster_placement_stats,
                                                        &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                                        incr_cost, (bool)(i == valid_mode));
                    }
                }
            }
        }
        incr_cost /= 10; /* blocks whose ancestor is further away in tree should be affected less than blocks closer in tree */
    }
}

/**
 * Set mode of cluster
 */
static void set_mode_cluster_placement_stats(t_intra_cluster_placement_stats* cluster_placement_stats,
                                             const t_pb_graph_node* pb_graph_node,
                                             int mode) {
    int i, j, k;
    for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        if (i != mode) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    update_primitive_cost_or_status(cluster_placement_stats,
                                                    &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                                    0,
                                                    false);
                }
            }
        }
    }
}

/**
 * For sibling primitives of pb_graph node, decrease cost
 * For modes invalidated by pb_graph_node, invalidate primitive
 * int distance is the distance of current pb_graph_node from original
 */
static void update_primitive_cost_or_status(t_intra_cluster_placement_stats* cluster_placement_stats,
                                            const t_pb_graph_node* pb_graph_node,
                                            const float incremental_cost,
                                            const bool valid) {
    if (pb_graph_node->is_primitive()) {
        /* is primitive */
        t_cluster_placement_primitive* placement_primitive = cluster_placement_stats->get_pb_graph_node_placement_primitive(pb_graph_node);
        if (valid) {
            placement_primitive->incremental_cost += incremental_cost;
        } else {
            placement_primitive->valid = false;
        }
    } else {
        for (int i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            for (int j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (int k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    update_primitive_cost_or_status(cluster_placement_stats,
                                                    &pb_graph_node->child_pb_graph_nodes[i][j][k],
                                                    incremental_cost, valid);
                }
            }
        }
    }
}

/**
 * Expand molecule at pb_graph_node
 * Assumes molecule and pack pattern connections have fan-out 1
 */
static bool expand_forced_pack_molecule_placement(t_intra_cluster_placement_stats* cluster_placement_stats,
                                                  PackMoleculeId molecule_id,
                                                  const t_pack_pattern_block* pack_pattern_block,
                                                  std::vector<t_pb_graph_node*>& primitives_list,
                                                  const Prepacker& prepacker,
                                                  float* cost) {
    t_pb_graph_node* pb_graph_node = primitives_list[pack_pattern_block->block_id];
    t_pb_graph_node* next_primitive;
    t_pack_pattern_connections* cur;
    t_pb_graph_pin *cur_pin, *next_pin;
    t_pack_pattern_block* next_block;

    cur = pack_pattern_block->connections;
    while (cur) {
        if (cur->from_block == pack_pattern_block) {
            next_block = cur->to_block;
        } else {
            next_block = cur->from_block;
        }
        const t_pack_molecule& molecule = prepacker.get_molecule(molecule_id);
        if (primitives_list[next_block->block_id] == nullptr && molecule.atom_block_ids[next_block->block_id]) {
            /* first time visiting location */

            /* find next primitive based on pattern connections, expand next primitive if not visited */
            if (cur->from_block == pack_pattern_block) {
                /* forward expand to find next block */
                int from_pin, from_port;
                from_pin = cur->from_pin->pin_number;
                from_port = cur->from_pin->port->port_index_by_type;
                cur_pin = &pb_graph_node->output_pins[from_port][from_pin];
                next_pin = expand_pack_molecule_pin_edge(pack_pattern_block->pattern_index, cur_pin, true);
            } else {
                /* backward expand to find next block */
                VTR_ASSERT(cur->to_block == pack_pattern_block);
                int to_pin, to_port;
                to_pin = cur->to_pin->pin_number;
                to_port = cur->to_pin->port->port_index_by_type;

                if (cur->from_pin->port->is_clock) {
                    cur_pin = &pb_graph_node->clock_pins[to_port][to_pin];
                } else {
                    cur_pin = &pb_graph_node->input_pins[to_port][to_pin];
                }
                next_pin = expand_pack_molecule_pin_edge(pack_pattern_block->pattern_index, cur_pin, false);
            }
            /* found next primitive */
            if (next_pin != nullptr) {
                next_primitive = next_pin->parent_node;
                /* Check for legality of placement, if legal, expand from legal placement, if not, return false */
                if (molecule.atom_block_ids[next_block->block_id] && primitives_list[next_block->block_id] == nullptr) {
                    t_cluster_placement_primitive* next_placement_primitive = cluster_placement_stats->get_pb_graph_node_placement_primitive(next_primitive);
                    if (next_placement_primitive->valid && primitive_type_feasible(molecule.atom_block_ids[next_block->block_id], next_primitive->pb_type)) {
                        primitives_list[next_block->block_id] = next_primitive;
                        *cost += next_placement_primitive->base_cost + next_placement_primitive->incremental_cost;
                        if (!expand_forced_pack_molecule_placement(cluster_placement_stats, molecule_id, next_block, primitives_list, prepacker, cost)) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            } else {
                return false;
            }
        }
        cur = cur->next;
    }

    return true;
}

/**
 * Find next primitive pb_graph_pin
 */
static t_pb_graph_pin* expand_pack_molecule_pin_edge(const int pattern_id,
                                                     const t_pb_graph_pin* cur_pin,
                                                     const bool forward) {
    int i, j, k;
    t_pb_graph_pin *temp_pin, *dest_pin;
    temp_pin = nullptr;
    dest_pin = nullptr;
    if (forward) {
        for (i = 0; i < cur_pin->num_output_edges; i++) {
            /* one fanout assumption */
            if (cur_pin->output_edges[i]->infer_pattern) {
                for (k = 0; k < cur_pin->output_edges[i]->num_output_pins;
                     k++) {
                    if (cur_pin->output_edges[i]->output_pins[k]->parent_node->pb_type->num_modes
                        == 0) {
                        temp_pin = cur_pin->output_edges[i]->output_pins[k];
                    } else {
                        temp_pin = expand_pack_molecule_pin_edge(pattern_id,
                                                                 cur_pin->output_edges[i]->output_pins[k],
                                                                 forward);
                    }
                }
                if (temp_pin != nullptr) {
                    VTR_ASSERT(dest_pin == nullptr || dest_pin == temp_pin);
                    dest_pin = temp_pin;
                }
            } else {
                for (j = 0; j < cur_pin->output_edges[i]->num_pack_patterns;
                     j++) {
                    if (cur_pin->output_edges[i]->pack_pattern_indices[j]
                        == pattern_id) {
                        for (k = 0;
                             k < cur_pin->output_edges[i]->num_output_pins;
                             k++) {
                            if (cur_pin->output_edges[i]->output_pins[k]->parent_node->pb_type->num_modes
                                == 0) {
                                temp_pin = cur_pin->output_edges[i]->output_pins[k];
                            } else {
                                temp_pin = expand_pack_molecule_pin_edge(pattern_id,
                                                                         cur_pin->output_edges[i]->output_pins[k],
                                                                         forward);
                            }
                        }
                        if (temp_pin != nullptr) {
                            VTR_ASSERT(dest_pin == nullptr || dest_pin == temp_pin);
                            dest_pin = temp_pin;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < cur_pin->num_input_edges; i++) {
            /* one fanout assumption */
            if (cur_pin->input_edges[i]->infer_pattern) {
                for (k = 0; k < cur_pin->input_edges[i]->num_input_pins; k++) {
                    if (cur_pin->input_edges[i]->input_pins[k]->parent_node->pb_type->num_modes
                        == 0) {
                        temp_pin = cur_pin->input_edges[i]->input_pins[k];
                    } else {
                        temp_pin = expand_pack_molecule_pin_edge(pattern_id,
                                                                 cur_pin->input_edges[i]->input_pins[k],
                                                                 forward);
                    }
                }
                if (temp_pin != nullptr) {
                    VTR_ASSERT(dest_pin == nullptr || dest_pin == temp_pin);
                    dest_pin = temp_pin;
                }
            } else {
                for (j = 0; j < cur_pin->input_edges[i]->num_pack_patterns;
                     j++) {
                    if (cur_pin->input_edges[i]->pack_pattern_indices[j]
                        == pattern_id) {
                        for (k = 0; k < cur_pin->input_edges[i]->num_input_pins;
                             k++) {
                            if (cur_pin->input_edges[i]->input_pins[k]->parent_node->pb_type->num_modes
                                == 0) {
                                temp_pin = cur_pin->input_edges[i]->input_pins[k];
                            } else {
                                temp_pin = expand_pack_molecule_pin_edge(pattern_id,
                                                                         cur_pin->input_edges[i]->input_pins[k],
                                                                         forward);
                            }
                        }
                        if (temp_pin != nullptr) {
                            VTR_ASSERT(dest_pin == nullptr || dest_pin == temp_pin);
                            dest_pin = temp_pin;
                        }
                    }
                }
            }
        }
    }
    return dest_pin;
}

/* Given atom block, determines if a free primitive exists for it */
bool exists_free_primitive_for_atom_block(t_intra_cluster_placement_stats* cluster_placement_stats,
                                          const AtomBlockId blk_id) {
    int i;

    /* might have a primitive in flight that's still valid */
    if (!cluster_placement_stats->in_flight_empty()) {
        if (primitive_type_feasible(blk_id,
                                    cluster_placement_stats->in_flight_type())) {
            return true;
        }
    }

    /* Look through list of available primitives to see if any valid */
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        //for (auto& primitive : cluster_placement_stats->valid_primitives[i]) {
        if (!cluster_placement_stats->valid_primitives[i].empty() && primitive_type_feasible(blk_id, cluster_placement_stats->valid_primitives[i].begin()->second->pb_graph_node->pb_type)) {
            for (auto it = cluster_placement_stats->valid_primitives[i].begin(); it != cluster_placement_stats->valid_primitives[i].end();) {
                if (it->second->valid)
                    return true;
                else {
                    cluster_placement_stats->invalidate_primitive_and_increment_iterator(i, it);
                }
            }
        }
    }

    return false;
}

void reset_tried_but_unused_cluster_placements(t_intra_cluster_placement_stats* cluster_placement_stats) {
    cluster_placement_stats->flush_intermediate_queues();
}
