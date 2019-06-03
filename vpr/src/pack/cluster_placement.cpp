/*
 * Given a group of atom blocks and a partially-packed complex block, find placement for group of atom blocks in complex block
 * To use, keep "cluster_placement_stats" data structure throughout packing
 * cluster_placement_stats undergoes these major states:
 * Initialization - performed once at beginning of packing
 * Reset          - reset state in between packing of clusters
 * In flight      - Speculatively place
 * Finalized      - Commit or revert placements
 * Freed          - performed once at end of packing
 *
 * Author: Jason Luu
 * March 12, 2012
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_memory.h"

#include "read_xml_arch_file.h"
#include "vpr_types.h"
#include "globals.h"
#include "atom_netlist.h"
#include "vpr_utils.h"
#include "hash.h"
#include "cluster_placement.h"

/****************************************/
/*Local Function Declaration			*/
/****************************************/
static void load_cluster_placement_stats_for_pb_graph_node(t_cluster_placement_stats* cluster_placement_stats,
                                                           t_pb_graph_node* pb_graph_node);
static void requeue_primitive(t_cluster_placement_stats* cluster_placement_stats,
                              t_cluster_placement_primitive* cluster_placement_primitive);
static void update_primitive_cost_or_status(const t_pb_graph_node* pb_graph_node,
                                            const float incremental_cost,
                                            const bool valid);
static float try_place_molecule(const t_pack_molecule* molecule,
                                t_pb_graph_node* root,
                                t_pb_graph_node** primitives_list);
static bool expand_forced_pack_molecule_placement(const t_pack_molecule* molecule,
                                                  const t_pack_pattern_block* pack_pattern_block,
                                                  t_pb_graph_node** primitives_list,
                                                  float* cost);
static t_pb_graph_pin* expand_pack_molecule_pin_edge(const int pattern_id,
                                                     const t_pb_graph_pin* cur_pin,
                                                     const bool forward);
static void flush_intermediate_queues(t_cluster_placement_stats* cluster_placement_stats);

/****************************************/
/*Function Definitions					*/
/****************************************/

/**
 * [0..num_pb_types-1] array of cluster placement stats, one for each device_ctx.block_types
 */
t_cluster_placement_stats* alloc_and_load_cluster_placement_stats() {
    t_cluster_placement_stats* cluster_placement_stats_list;
    int i;

    auto& device_ctx = g_vpr_ctx.device();

    cluster_placement_stats_list = (t_cluster_placement_stats*)vtr::calloc(device_ctx.num_block_types,
                                                                           sizeof(t_cluster_placement_stats));
    for (i = 0; i < device_ctx.num_block_types; i++) {
        if (device_ctx.EMPTY_TYPE != &device_ctx.block_types[i]) {
            cluster_placement_stats_list[i].valid_primitives = (t_cluster_placement_primitive**)vtr::calloc(
                get_max_primitives_in_pb_type(device_ctx.block_types[i].pb_type) + 1,
                sizeof(t_cluster_placement_primitive*)); /* too much memory allocated but shouldn't be a problem */
            cluster_placement_stats_list[i].curr_molecule = nullptr;
            load_cluster_placement_stats_for_pb_graph_node(&cluster_placement_stats_list[i],
                                                           device_ctx.block_types[i].pb_graph_head);
        }
    }
    return cluster_placement_stats_list;
}

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
 */
bool get_next_primitive_list(t_cluster_placement_stats* cluster_placement_stats,
                             const t_pack_molecule* molecule,
                             t_pb_graph_node** primitives_list) {
    t_cluster_placement_primitive *cur, *next, *best, *before_best, *prev;
    int i;
    float cost, lowest_cost;
    best = nullptr;
    before_best = nullptr;

    if (cluster_placement_stats->curr_molecule != molecule) {
        /* New block, requeue tried primitives and in-flight primitives */
        flush_intermediate_queues(cluster_placement_stats);

        cluster_placement_stats->curr_molecule = molecule;
    } else {
        /* Hack! Same failed molecule may re-enter if upper stream functions suck,
         * I'm going to make the molecule selector more intelligent.
         * TODO: Remove later
         */
        if (cluster_placement_stats->in_flight != nullptr) {
            /* Hack end */

            /* old block, put root primitive currently inflight to tried queue	*/
            cur = cluster_placement_stats->in_flight;
            next = cur->next_primitive;
            cur->next_primitive = cluster_placement_stats->tried;
            cluster_placement_stats->tried = cur;
            /* should have only one block in flight at any point in time */
            VTR_ASSERT(next == nullptr);
            cluster_placement_stats->in_flight = nullptr;
        }
    }

    /* find next set of blocks
     * 1. Remove invalid blocks to invalid queue
     * 2. Find lowest cost array of primitives that implements blocks
     * 3. When found, move current blocks to in-flight, return lowest cost array of primitives
     * 4. Return NULL if not found
     */
    lowest_cost = HUGE_POSITIVE_FLOAT;
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        if (cluster_placement_stats->valid_primitives[i]->next_primitive == nullptr) {
            continue; /* no more primitives of this type available */
        }
        if (primitive_type_feasible(molecule->atom_block_ids[molecule->root],
                                    cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type)) {
            prev = cluster_placement_stats->valid_primitives[i];
            cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
            while (cur) {
                /* remove invalid nodes lazily when encountered */
                while (cur && cur->valid == false) {
                    prev->next_primitive = cur->next_primitive;
                    cur->next_primitive = cluster_placement_stats->invalid;
                    cluster_placement_stats->invalid = cur;
                    cur = prev->next_primitive;
                }
                if (cur == nullptr) {
                    break;
                }
                /* try place molecule at root location cur */
                cost = try_place_molecule(molecule, cur->pb_graph_node, primitives_list);
                // if the cost is lower than the best, or is equal to the best but this
                // primitive is more available in the cluster mark it as the best primitive
                if (cost < lowest_cost || (best && cost == lowest_cost && cur->pb_graph_node->total_primitive_count > best->pb_graph_node->total_primitive_count)) {
                    lowest_cost = cost;
                    best = cur;
                    before_best = prev;
                }
                prev = cur;
                cur = cur->next_primitive;
            }
        }
    }
    if (best == nullptr) {
        /* failed to find a placement */
        for (i = 0; i < molecule->num_blocks; i++) {
            primitives_list[i] = nullptr;
        }
    } else {
        /* populate primitive list with best */
        cost = try_place_molecule(molecule, best->pb_graph_node, primitives_list);
        VTR_ASSERT(cost == lowest_cost);

        /* take out best node and put it in flight */
        cluster_placement_stats->in_flight = best;
        before_best->next_primitive = best->next_primitive;
        best->next_primitive = nullptr;
    }

    if (best == nullptr) {
        return false;
    }
    return true;
}

/**
 * Resets one cluster placement stats by clearing incremental costs and returning all primitives to valid queue
 */
void reset_cluster_placement_stats(t_cluster_placement_stats* cluster_placement_stats) {
    t_cluster_placement_primitive *cur, *next;
    int i;

    /* Requeue primitives */
    flush_intermediate_queues(cluster_placement_stats);
    cur = cluster_placement_stats->invalid;
    while (cur != nullptr) {
        next = cur->next_primitive;
        requeue_primitive(cluster_placement_stats, cur);
        cur = next;
    }
    cur = cluster_placement_stats->invalid = nullptr;
    /* reset flags and cost */
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        VTR_ASSERT(cluster_placement_stats->valid_primitives[i] != nullptr
                   && cluster_placement_stats->valid_primitives[i]->next_primitive != nullptr);
        cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
        while (cur != nullptr) {
            cur->incremental_cost = 0;
            cur->valid = true;
            cur = cur->next_primitive;
        }
    }
    cluster_placement_stats->curr_molecule = nullptr;
    cluster_placement_stats->has_long_chain = false;
}

/**
 * Free linked lists found in cluster_placement_stats_list
 */
void free_cluster_placement_stats(t_cluster_placement_stats* cluster_placement_stats_list) {
    t_cluster_placement_primitive *cur, *next;
    int i, j;
    auto& device_ctx = g_vpr_ctx.device();

    for (i = 0; i < device_ctx.num_block_types; i++) {
        cur = cluster_placement_stats_list[i].tried;
        while (cur != nullptr) {
            next = cur->next_primitive;
            free(cur);
            cur = next;
        }
        cur = cluster_placement_stats_list[i].in_flight;
        while (cur != nullptr) {
            next = cur->next_primitive;
            free(cur);
            cur = next;
        }
        cur = cluster_placement_stats_list[i].invalid;
        while (cur != nullptr) {
            next = cur->next_primitive;
            free(cur);
            cur = next;
        }
        for (j = 0; j < cluster_placement_stats_list[i].num_pb_types; j++) {
            cur = cluster_placement_stats_list[i].valid_primitives[j]->next_primitive;
            while (cur != nullptr) {
                next = cur->next_primitive;
                free(cur);
                cur = next;
            }
            free(cluster_placement_stats_list[i].valid_primitives[j]);
        }
        free(cluster_placement_stats_list[i].valid_primitives);
    }
    free(cluster_placement_stats_list);
}

/**
 * Put primitive back on queue of valid primitives
 *  Note that valid status is not changed because if the primitive is not valid, it will get properly collected later
 */
static void requeue_primitive(t_cluster_placement_stats* cluster_placement_stats,
                              t_cluster_placement_primitive* cluster_placement_primitive) {
    int i;
    int null_index;
    bool success;
    null_index = OPEN;

    success = false;
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        if (cluster_placement_stats->valid_primitives[i]->next_primitive == nullptr) {
            null_index = i;
            continue;
        }
        if (cluster_placement_primitive->pb_graph_node->pb_type
            == cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type) {
            success = true;
            cluster_placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
            cluster_placement_stats->valid_primitives[i]->next_primitive = cluster_placement_primitive;
        }
    }
    if (success == false) {
        VTR_ASSERT(null_index != OPEN);
        cluster_placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[null_index]->next_primitive;
        cluster_placement_stats->valid_primitives[null_index]->next_primitive = cluster_placement_primitive;
    }
}

/**
 * Add any primitives found in pb_graph_nodes to cluster_placement_stats
 * Adds backward link from pb_graph_node to cluster_placement_primitive
 */
static void load_cluster_placement_stats_for_pb_graph_node(t_cluster_placement_stats* cluster_placement_stats,
                                                           t_pb_graph_node* pb_graph_node) {
    int i, j, k;
    t_cluster_placement_primitive* placement_primitive;
    const t_pb_type* pb_type = pb_graph_node->pb_type;
    bool success;
    if (pb_type->modes == nullptr) {
        placement_primitive = (t_cluster_placement_primitive*)vtr::calloc(1,
                                                                          sizeof(t_cluster_placement_primitive));
        placement_primitive->pb_graph_node = pb_graph_node;
        placement_primitive->valid = true;
        pb_graph_node->cluster_placement_primitive = placement_primitive;
        placement_primitive->base_cost = compute_primitive_base_cost(pb_graph_node);
        success = false;
        i = 0;
        while (success == false) {
            if (cluster_placement_stats->valid_primitives[i] == nullptr
                || cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type
                       == pb_graph_node->pb_type) {
                if (cluster_placement_stats->valid_primitives[i] == nullptr) {
                    cluster_placement_stats->valid_primitives[i] = (t_cluster_placement_primitive*)vtr::calloc(1,
                                                                                                               sizeof(t_cluster_placement_primitive)); /* head of linked list is empty, makes it easier to remove nodes later */
                    cluster_placement_stats->num_pb_types++;
                }
                success = true;
                placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
                cluster_placement_stats->valid_primitives[i]->next_primitive = placement_primitive;
            }
            i++;
        }
    } else {
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

/**
 * Commit primitive, invalidate primitives blocked by mode assignment and update costs for primitives in same cluster as current
 * Costing is done to try to pack blocks closer to existing primitives
 *  actual value based on closest common ancestor to committed placement, the farther the ancestor, the less reduction in cost there is
 * Side effects: All cluster_placement_primitives may be invalidated/costed in this algorithm
 *               Al intermediate queues are requeued
 */
void commit_primitive(t_cluster_placement_stats* cluster_placement_stats,
                      const t_pb_graph_node* primitive) {
    t_pb_graph_node *pb_graph_node, *skip;
    float incr_cost;
    int i, j, k;
    int valid_mode;
    t_cluster_placement_primitive* cur;

    /* Clear out intermediate queues */
    flush_intermediate_queues(cluster_placement_stats);

    /* commit primitive as used, invalidate it */
    cur = primitive->cluster_placement_primitive;
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
                        update_primitive_cost_or_status(&pb_graph_node->child_pb_graph_nodes[i][j][k],
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
void set_mode_cluster_placement_stats(const t_pb_graph_node* pb_graph_node, int mode) {
    int i, j, k;
    for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        if (i != mode) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    update_primitive_cost_or_status(&pb_graph_node->child_pb_graph_nodes[i][j][k], 0, false);
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
static void update_primitive_cost_or_status(const t_pb_graph_node* pb_graph_node,
                                            const float incremental_cost,
                                            const bool valid) {
    int i, j, k;
    t_cluster_placement_primitive* placement_primitive;
    if (pb_graph_node->is_primitive()) {
        /* is primitive */
        placement_primitive = (t_cluster_placement_primitive*)pb_graph_node->cluster_placement_primitive;
        if (valid) {
            placement_primitive->incremental_cost += incremental_cost;
        } else {
            placement_primitive->valid = false;
        }
    } else {
        for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                    update_primitive_cost_or_status(&pb_graph_node->child_pb_graph_nodes[i][j][k],
                                                    incremental_cost, valid);
                }
            }
        }
    }
}

/**
 * Try place molecule at root location, populate primitives list with locations of placement if successful
 */
static float try_place_molecule(const t_pack_molecule* molecule,
                                t_pb_graph_node* root,
                                t_pb_graph_node** primitives_list) {
    int list_size, i;
    float cost = HUGE_POSITIVE_FLOAT;
    list_size = get_array_size_of_molecule(molecule);

    if (primitive_type_feasible(molecule->atom_block_ids[molecule->root],
                                root->pb_type)) {
        if (root->cluster_placement_primitive->valid == true) {
            for (i = 0; i < list_size; i++) {
                primitives_list[i] = nullptr;
            }
            cost = root->cluster_placement_primitive->base_cost
                   + root->cluster_placement_primitive->incremental_cost;
            primitives_list[molecule->root] = root;
            if (molecule->type == MOLECULE_FORCED_PACK) {
                if (!expand_forced_pack_molecule_placement(molecule,
                                                           molecule->pack_pattern->root_block, primitives_list,
                                                           &cost)) {
                    return HUGE_POSITIVE_FLOAT;
                }
            }
            for (i = 0; i < list_size; i++) {
                VTR_ASSERT((primitives_list[i] == nullptr) == (!molecule->atom_block_ids[i]));
                for (int j = 0; j < list_size; j++) {
                    if (i != j) {
                        if (primitives_list[i] != nullptr && primitives_list[i] == primitives_list[j]) {
                            return HUGE_POSITIVE_FLOAT;
                        }
                    }
                }
            }
        }
    }
    return cost;
}

/**
 * Expand molecule at pb_graph_node
 * Assumes molecule and pack pattern connections have fan-out 1
 */
static bool expand_forced_pack_molecule_placement(const t_pack_molecule* molecule,
                                                  const t_pack_pattern_block* pack_pattern_block,
                                                  t_pb_graph_node** primitives_list,
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
        if (primitives_list[next_block->block_id] == nullptr && molecule->atom_block_ids[next_block->block_id]) {
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
                if (molecule->atom_block_ids[next_block->block_id] && primitives_list[next_block->block_id] == nullptr) {
                    if (next_primitive->cluster_placement_primitive->valid == true && primitive_type_feasible(molecule->atom_block_ids[next_block->block_id], next_primitive->pb_type)) {
                        primitives_list[next_block->block_id] = next_primitive;
                        *cost += next_primitive->cluster_placement_primitive->base_cost + next_primitive->cluster_placement_primitive->incremental_cost;
                        if (!expand_forced_pack_molecule_placement(molecule, next_block, primitives_list, cost)) {
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

static void flush_intermediate_queues(t_cluster_placement_stats* cluster_placement_stats) {
    t_cluster_placement_primitive *cur, *next;
    cur = cluster_placement_stats->tried;
    while (cur != nullptr) {
        next = cur->next_primitive;
        requeue_primitive(cluster_placement_stats, cur);
        cur = next;
    }
    cluster_placement_stats->tried = nullptr;

    cur = cluster_placement_stats->in_flight;
    if (cur != nullptr) {
        next = cur->next_primitive;
        requeue_primitive(cluster_placement_stats, cur);
        /* should have at most one block in flight at any point in time */
        VTR_ASSERT(next == nullptr);
    }
    cluster_placement_stats->in_flight = nullptr;
}

/* Determine max index + 1 of molecule */
int get_array_size_of_molecule(const t_pack_molecule* molecule) {
    if (molecule->type == MOLECULE_FORCED_PACK) {
        return molecule->pack_pattern->num_blocks;
    } else {
        return molecule->num_blocks;
    }
}

/* Given atom block, determines if a free primitive exists for it */
bool exists_free_primitive_for_atom_block(t_cluster_placement_stats* cluster_placement_stats,
                                          const AtomBlockId blk_id) {
    int i;
    t_cluster_placement_primitive *cur, *prev;

    /* might have a primitive in flight that's still valid */
    if (cluster_placement_stats->in_flight) {
        if (primitive_type_feasible(blk_id,
                                    cluster_placement_stats->in_flight->pb_graph_node->pb_type)) {
            return true;
        }
    }

    /* Look through list of available primitives to see if any valid */
    for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
        if (cluster_placement_stats->valid_primitives[i]->next_primitive == nullptr) {
            continue; /* no more primitives of this type available */
        }
        if (primitive_type_feasible(blk_id,
                                    cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type)) {
            prev = cluster_placement_stats->valid_primitives[i];
            cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
            while (cur) {
                /* remove invalid nodes lazily when encountered */
                while (cur && cur->valid == false) {
                    prev->next_primitive = cur->next_primitive;
                    cur->next_primitive = cluster_placement_stats->invalid;
                    cluster_placement_stats->invalid = cur;
                    cur = prev->next_primitive;
                }
                if (cur == nullptr) {
                    break;
                }
                return true;
            }
        }
    }

    return false;
}

void reset_tried_but_unused_cluster_placements(t_cluster_placement_stats* cluster_placement_stats) {
    flush_intermediate_queues(cluster_placement_stats);
}
