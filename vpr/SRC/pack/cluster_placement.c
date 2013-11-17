/* 
 Given a group of logical blocks and a partially-packed complex block, find placement for group of logical blocks in complex block
 To use, keep "cluster_placement_stats" data structure throughout packing
 cluster_placement_stats undergoes these major states:
 Initialization - performed once at beginning of packing
 Reset          - reset state in between packing of clusters
 In flight      - Speculatively place
 Finalized      - Commit or revert placements
 Freed          - performed once at end of packing

 Author: Jason Luu
 March 12, 2012
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "read_xml_arch_file.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "hash.h"
#include "cluster_placement.h"

/****************************************/
/*Local Function Declaration			*/
/****************************************/
static void load_cluster_placement_stats_for_pb_graph_node(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		INOUTP t_pb_graph_node *pb_graph_node);
static void requeue_primitive(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		t_cluster_placement_primitive *cluster_placement_primitive);
static void update_primitive_cost_or_status(INP t_pb_graph_node *pb_graph_node,
		INP float incremental_cost, INP boolean valid);
static float try_place_molecule(INP t_pack_molecule *molecule,
		INP t_pb_graph_node *root, INOUTP t_pb_graph_node **primitives_list, INP int clb_index);
static boolean expand_forced_pack_molecule_placement(
		INP t_pack_molecule *molecule,
		INP t_pack_pattern_block *pack_pattern_block,
		INOUTP t_pb_graph_node **primitives_list, INOUTP float *cost);
static t_pb_graph_pin *expand_pack_molecule_pin_edge(INP int pattern_id,
		INP t_pb_graph_pin *cur_pin, INP boolean forward);
static void flush_intermediate_queues(
		INOUTP t_cluster_placement_stats *cluster_placement_stats);
static boolean root_passes_early_filter(INP t_pb_graph_node *root, INP t_pack_molecule *molecule, INP int clb_index);

/****************************************/
/*Function Definitions					*/
/****************************************/

/**
 * [0..num_pb_types-1] array of cluster placement stats, one for each type_descriptors 
 */
t_cluster_placement_stats *alloc_and_load_cluster_placement_stats(void) {
	t_cluster_placement_stats *cluster_placement_stats_list;
	int i;

	cluster_placement_stats_list = (t_cluster_placement_stats *) my_calloc(num_types,
			sizeof(t_cluster_placement_stats));
	for (i = 0; i < num_types; i++) {
		if (EMPTY_TYPE != &type_descriptors[i]) {
			cluster_placement_stats_list[i].valid_primitives = (t_cluster_placement_primitive **) my_calloc(
					get_max_primitives_in_pb_type(type_descriptors[i].pb_type)
 							+ 1, sizeof(t_cluster_placement_primitive*)); /* too much memory allocated but shouldn't be a problem */
			cluster_placement_stats_list[i].curr_molecule = NULL;
			load_cluster_placement_stats_for_pb_graph_node(
					&cluster_placement_stats_list[i],
					type_descriptors[i].pb_graph_head);
		}
	}
	return cluster_placement_stats_list;
}

/**
 * get next list of primitives for list of logical blocks
 * primitives is the list of ptrs to primitives that matches with the list of logical_blocks (by index), assumes memory is preallocated
 *   - if this is a new block, requeue tried primitives and return a in-flight primitive list to try
 *   - if this is an old block, put root primitive to tried queue, requeue rest of primitives. try another set of primitives
 *
 * return TRUE if can find next primitive, FALSE otherwise
 * 
 * cluster_placement_stats - ptr to the current cluster_placement_stats of open complex block
 * molecule - molecule to pack into open complex block
 * primitives_list - a list of primitives indexed to match logical_block_ptrs of molecule.  Expects an allocated array of primitives ptrs as inputs.  This function loads the array with the lowest cost primitives that implement molecule
 */
boolean get_next_primitive_list(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		INP t_pack_molecule *molecule, INOUTP t_pb_graph_node **primitives_list, INP int clb_index) {
	t_cluster_placement_primitive *cur, *next, *best, *before_best, *prev;
	int i;
	float cost, lowest_cost;
	best = NULL;
	before_best = NULL;

	if (cluster_placement_stats->curr_molecule != molecule) {
		/* New block, requeue tried primitives and in-flight primitives */
		flush_intermediate_queues(cluster_placement_stats);

		cluster_placement_stats->curr_molecule = molecule;
	} else {
		/* Hack! Same failed molecule may re-enter if upper stream functions suck, I'm going to make the molecule selector more intelligent, TODO: Remove later */
		if (cluster_placement_stats->in_flight != NULL) {
			/* Hack end */

			/* old block, put root primitive currently inflight to tried queue	*/
			cur = cluster_placement_stats->in_flight;
			next = cur->next_primitive;
			cur->next_primitive = cluster_placement_stats->tried;
			cluster_placement_stats->tried = cur;
			/* should have only one block in flight at any point in time */
			assert(next == NULL);
			cluster_placement_stats->in_flight = NULL;
		}
	}

	/* find next set of blocks 
	 1. Remove invalid blocks to invalid queue
	 2. Find lowest cost array of primitives that implements blocks
	 3. When found, move current blocks to in-flight, return lowest cost array of primitives
	 4. Return NULL if not found
	 */
	lowest_cost = HUGE_POSITIVE_FLOAT;
	for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		if (cluster_placement_stats->valid_primitives[i]->next_primitive == NULL) {
			continue; /* no more primitives of this type available */
		}
		if (primitive_type_feasible(
				molecule->logical_block_ptrs[molecule->root]->index,
				cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type)) {
			prev = cluster_placement_stats->valid_primitives[i];
			cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
			while (cur) {
				/* remove invalid nodes lazily when encountered */
				while (cur && cur->valid == FALSE) {
					prev->next_primitive = cur->next_primitive;
					cur->next_primitive = cluster_placement_stats->invalid;
					cluster_placement_stats->invalid = cur;
					cur = prev->next_primitive;
				}
				if (cur == NULL) {
					break;
				}
				/* try place molecule at root location cur */
				cost = try_place_molecule(molecule, cur->pb_graph_node,
						primitives_list, clb_index);
				if (cost < lowest_cost) {
					lowest_cost = cost;
					best = cur;
					before_best = prev;
				}
				prev = cur;
				cur = cur->next_primitive;
			}
		}
	}
	if (best == NULL) {
		/* failed to find a placement */
		for (i = 0; i < molecule->num_blocks; i++) {
			primitives_list[i] = NULL;
		}
	} else {
		/* populate primitive list with best */
		cost = try_place_molecule(molecule, best->pb_graph_node, primitives_list, clb_index);
		assert(cost == lowest_cost);

		/* take out best node and put it in flight */
		cluster_placement_stats->in_flight = best;
		before_best->next_primitive = best->next_primitive;
		best->next_primitive = NULL;
	}

	if (best == NULL) {
		return FALSE;
	}
	return TRUE;
}

/**
 * Resets one cluster placement stats by clearing incremental costs and returning all primitives to valid queue
 */
void reset_cluster_placement_stats(
		INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	t_cluster_placement_primitive *cur, *next;
	int i;

	/* Requeue primitives */
	flush_intermediate_queues(cluster_placement_stats);
	cur = cluster_placement_stats->invalid;
	while (cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		cur = next;
	}
	cur = cluster_placement_stats->invalid = NULL;
	/* reset flags and cost */
	for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		assert(
				cluster_placement_stats->valid_primitives[i] != NULL && cluster_placement_stats->valid_primitives[i]->next_primitive != NULL);
		cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
		while (cur != NULL) {
			cur->incremental_cost = 0;
			cur->valid = TRUE;
			cur = cur->next_primitive;
		}
	}
	cluster_placement_stats->curr_molecule = NULL;
}

/** 
 * Free linked lists found in cluster_placement_stats_list 
 */
void free_cluster_placement_stats(
		INOUTP t_cluster_placement_stats *cluster_placement_stats_list) {
	t_cluster_placement_primitive *cur, *next;
	int i, j;
	for (i = 0; i < num_types; i++) {
		cur = cluster_placement_stats_list[i].tried;
		while (cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		cur = cluster_placement_stats_list[i].in_flight;
		while (cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		cur = cluster_placement_stats_list[i].invalid;
		while (cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		for (j = 0; j < cluster_placement_stats_list[i].num_pb_types; j++) {
			cur =
					cluster_placement_stats_list[i].valid_primitives[j]->next_primitive;
			while (cur != NULL) {
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
static void requeue_primitive(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		t_cluster_placement_primitive *cluster_placement_primitive) {
	int i;
	int null_index;
	boolean success;
	null_index = OPEN;

	success = FALSE;
	for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		if (cluster_placement_stats->valid_primitives[i]->next_primitive == NULL) {
			null_index = i;
			continue;
		}
		if (cluster_placement_primitive->pb_graph_node->pb_type
				== cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type) {
			success = TRUE;
			cluster_placement_primitive->next_primitive =
					cluster_placement_stats->valid_primitives[i]->next_primitive;
			cluster_placement_stats->valid_primitives[i]->next_primitive =
					cluster_placement_primitive;
		}
	}
	if (success == FALSE) {
		assert(null_index != OPEN);
		cluster_placement_primitive->next_primitive =
				cluster_placement_stats->valid_primitives[null_index]->next_primitive;
		cluster_placement_stats->valid_primitives[null_index]->next_primitive =
				cluster_placement_primitive;
	}
}

/** 
 * Add any primitives found in pb_graph_nodes to cluster_placement_stats
 * Adds backward link from pb_graph_node to cluster_placement_primitive
 */
static void load_cluster_placement_stats_for_pb_graph_node(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;
	t_cluster_placement_primitive *placement_primitive;
	const t_pb_type *pb_type = pb_graph_node->pb_type;
	boolean success;
	if (pb_type->modes == 0) {
		placement_primitive = (t_cluster_placement_primitive *) my_calloc(1,
				sizeof(t_cluster_placement_primitive));
		placement_primitive->pb_graph_node = pb_graph_node;
		placement_primitive->valid = TRUE;
		pb_graph_node->cluster_placement_primitive = placement_primitive;
		placement_primitive->base_cost = compute_primitive_base_cost(
				pb_graph_node);
		success = FALSE;
		i = 0;
		while (success == FALSE) {
			if (cluster_placement_stats->valid_primitives[i] == NULL
					|| cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type
							== pb_graph_node->pb_type) {
				if (cluster_placement_stats->valid_primitives[i] == NULL) {
					cluster_placement_stats->valid_primitives[i] = (t_cluster_placement_primitive *) my_calloc(1,
							sizeof(t_cluster_placement_primitive)); /* head of linked list is empty, makes it easier to remove nodes later */
					cluster_placement_stats->num_pb_types++;
				}
				success = TRUE;
				placement_primitive->next_primitive =
						cluster_placement_stats->valid_primitives[i]->next_primitive;
				cluster_placement_stats->valid_primitives[i]->next_primitive =
						placement_primitive;
			}
			i++;
		}
	} else {
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					load_cluster_placement_stats_for_pb_graph_node(
							cluster_placement_stats,
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
void commit_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats,
		INP t_pb_graph_node *primitive) {
	t_pb_graph_node *pb_graph_node, *skip;
	float incr_cost;
	int i, j, k;
	int valid_mode;
	t_cluster_placement_primitive *cur;

	/* Clear out intermediate queues */
	flush_intermediate_queues(cluster_placement_stats);

	/* commit primitive as used, invalidate it */
	cur = primitive->cluster_placement_primitive;
	assert(cur->valid == TRUE);

	cur->valid = FALSE;
	incr_cost = -0.01; /* cost of using a node drops as its neighbours are used, this drop should be small compared to scarcity values */

	pb_graph_node = cur->pb_graph_node;
	/* walk up pb_graph_node and update primitives of children */
	while (pb_graph_node->parent_pb_graph_node != NULL) {
		skip = pb_graph_node; /* do not traverse stuff that's already traversed */
		valid_mode = pb_graph_node->pb_type->parent_mode->index;
		pb_graph_node = pb_graph_node->parent_pb_graph_node;
		for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k
								< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					if (&pb_graph_node->child_pb_graph_nodes[i][j][k] != skip) {
						update_primitive_cost_or_status(
								&pb_graph_node->child_pb_graph_nodes[i][j][k],
								incr_cost, (boolean)(i == valid_mode));
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
void set_mode_cluster_placement_stats(INP t_pb_graph_node *pb_graph_node,
		int mode) {
	int i, j, k;
	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		if (i != mode) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k
								< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					update_primitive_cost_or_status(
							&pb_graph_node->child_pb_graph_nodes[i][j][k], 0,
							FALSE);
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
static void update_primitive_cost_or_status(INP t_pb_graph_node *pb_graph_node,
		INP float incremental_cost, INP boolean valid) {
	int i, j, k;
	t_cluster_placement_primitive *placement_primitive;
	if (pb_graph_node->pb_type->num_modes == 0) {
		/* is primitive */
		placement_primitive =
				(t_cluster_placement_primitive*) pb_graph_node->cluster_placement_primitive;
		if (valid) {
			placement_primitive->incremental_cost += incremental_cost;
		} else {
			placement_primitive->valid = FALSE;
		}
	} else {
		for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for (j = 0;
					j < pb_graph_node->pb_type->modes[i].num_pb_type_children;
					j++) {
				for (k = 0;
						k
								< pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb;
						k++) {
					update_primitive_cost_or_status(
							&pb_graph_node->child_pb_graph_nodes[i][j][k],
							incremental_cost, valid);
				}
			}
		}
	}
}

/**
 * Try place molecule at root location, populate primitives list with locations of placement if successful
 */
static float try_place_molecule(INP t_pack_molecule *molecule,
		INP t_pb_graph_node *root, INOUTP t_pb_graph_node **primitives_list, INP int clb_index) {
	int list_size, i;
	float cost = HUGE_POSITIVE_FLOAT;
	list_size = get_array_size_of_molecule(molecule);

	if (primitive_type_feasible(
			molecule->logical_block_ptrs[molecule->root]->index,
			root->pb_type)) {
		if (root->cluster_placement_primitive->valid == TRUE) {
			if(root_passes_early_filter(root, molecule, clb_index)) {
				for (i = 0; i < list_size; i++) {
					primitives_list[i] = NULL;
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
					assert(
							(primitives_list[i] == NULL) == (molecule->logical_block_ptrs[i] == NULL));
					for (int j = 0; j < list_size; j++) {
						if(i != j) {
							if(primitives_list[i] != NULL && primitives_list[i] == primitives_list[j]) {
								return HUGE_POSITIVE_FLOAT;
							}
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
static boolean expand_forced_pack_molecule_placement(
		INP t_pack_molecule *molecule,
		INP t_pack_pattern_block *pack_pattern_block,
		INOUTP t_pb_graph_node **primitives_list, INOUTP float *cost) {
	t_pb_graph_node *pb_graph_node =
			primitives_list[pack_pattern_block->block_id];
	t_pb_graph_node *next_primitive;
	t_pack_pattern_connections *cur;
	t_pb_graph_pin *cur_pin, *next_pin;
	t_pack_pattern_block *next_block;

	cur = pack_pattern_block->connections;
	while (cur) {
		if (cur->from_block == pack_pattern_block) {
			next_block = cur->to_block;
		} else {
			next_block = cur->from_block;
		}
		if (primitives_list[next_block->block_id] == NULL && molecule->logical_block_ptrs[next_block->block_id] != NULL) {
			/* first time visiting location */

			/* find next primitive based on pattern connections, expand next primitive if not visited */
			if (cur->from_block == pack_pattern_block) {
				/* forward expand to find next block */
				int from_pin, from_port;
				from_pin = cur->from_pin->pin_number;
				from_port = cur->from_pin->port->port_index_by_type;
				cur_pin = &pb_graph_node->output_pins[from_port][from_pin];
				next_pin = expand_pack_molecule_pin_edge(
						pack_pattern_block->pattern_index, cur_pin, TRUE);
			} else {
				/* backward expand to find next block */
				assert(cur->to_block == pack_pattern_block);
				int to_pin, to_port;
				to_pin = cur->to_pin->pin_number;
				to_port = cur->to_pin->port->port_index_by_type;
				
				if (cur->from_pin->port->is_clock) {
					cur_pin = &pb_graph_node->clock_pins[to_port][to_pin];
				} else {
					cur_pin = &pb_graph_node->input_pins[to_port][to_pin];
				}
				next_pin = expand_pack_molecule_pin_edge(
						pack_pattern_block->pattern_index, cur_pin, FALSE);
			}
			/* found next primitive */
			if (next_pin != NULL) {
				next_primitive = next_pin->parent_node;
				/* Check for legality of placement, if legal, expand from legal placement, if not, return FALSE */
				if (molecule->logical_block_ptrs[next_block->block_id] != NULL
						&& primitives_list[next_block->block_id] == NULL) {
					if (next_primitive->cluster_placement_primitive->valid
							== TRUE
							&& primitive_type_feasible(
									molecule->logical_block_ptrs[next_block->block_id]->index,
									next_primitive->pb_type)) {
						primitives_list[next_block->block_id] = next_primitive;
						*cost +=
								next_primitive->cluster_placement_primitive->base_cost
										+ next_primitive->cluster_placement_primitive->incremental_cost;
						if (!expand_forced_pack_molecule_placement(molecule,
								next_block, primitives_list, cost)) {
							return FALSE;
						}
					} else {
						return FALSE;
					}
				}
			} else {
				return FALSE;
			}
		}
		cur = cur->next;
	}

	return TRUE;
}

/**
 * Find next primitive pb_graph_pin
 */
static t_pb_graph_pin *expand_pack_molecule_pin_edge(INP int pattern_id,
		INP t_pb_graph_pin *cur_pin, INP boolean forward) {
	int i, j, k;
	t_pb_graph_pin *temp_pin, *dest_pin;
	temp_pin = NULL;
	dest_pin = NULL;
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
				if (temp_pin != NULL) {
					assert(dest_pin == NULL || dest_pin == temp_pin);
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
								temp_pin =
										cur_pin->output_edges[i]->output_pins[k];
							} else {
								temp_pin =
										expand_pack_molecule_pin_edge(
												pattern_id,
												cur_pin->output_edges[i]->output_pins[k],
												forward);
							}
						}
						if (temp_pin != NULL) {
							assert(dest_pin == NULL || dest_pin == temp_pin);
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
				if (temp_pin != NULL) {
					assert(dest_pin == NULL || dest_pin == temp_pin);
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
								temp_pin =
										cur_pin->input_edges[i]->input_pins[k];
							} else {
								temp_pin = expand_pack_molecule_pin_edge(
										pattern_id,
										cur_pin->input_edges[i]->input_pins[k],
										forward);
							}
						}
						if (temp_pin != NULL) {
							assert(dest_pin == NULL || dest_pin == temp_pin);
							dest_pin = temp_pin;
						}
					}
				}
			}
		}
	}
	return dest_pin;
}

static void flush_intermediate_queues(
		INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	t_cluster_placement_primitive *cur, *next;
	cur = cluster_placement_stats->tried;
	while (cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		cur = next;
	}
	cluster_placement_stats->tried = NULL;

	cur = cluster_placement_stats->in_flight;
	if (cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		/* should have at most one block in flight at any point in time */
		assert(next == NULL);
	}
	cluster_placement_stats->in_flight = NULL;
}

/* Determine max index + 1 of molecule */
int get_array_size_of_molecule(t_pack_molecule *molecule) {
	if (molecule->type == MOLECULE_FORCED_PACK) {
		return molecule->pack_pattern->num_blocks;
	} else {
		return molecule->num_blocks;
	}
}

/* Given logical block, determines if a free primitive exists for it */
boolean exists_free_primitive_for_logical_block(
		INOUTP t_cluster_placement_stats *cluster_placement_stats,
		INP int ilogical_block) {
	int i;
	t_cluster_placement_primitive *cur, *prev;

	/* might have a primitive in flight that's still valid */
	if (cluster_placement_stats->in_flight) {
		if (primitive_type_feasible(ilogical_block,
				cluster_placement_stats->in_flight->pb_graph_node->pb_type)) {
			return TRUE;
		}
	}

	/* Look through list of available primitives to see if any valid */
	for (i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		if (cluster_placement_stats->valid_primitives[i]->next_primitive == NULL) {
			continue; /* no more primitives of this type available */
		}
		if (primitive_type_feasible(ilogical_block,
				cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type)) {
			prev = cluster_placement_stats->valid_primitives[i];
			cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
			while (cur) {
				/* remove invalid nodes lazily when encountered */
				while (cur && cur->valid == FALSE) {
					prev->next_primitive = cur->next_primitive;
					cur->next_primitive = cluster_placement_stats->invalid;
					cluster_placement_stats->invalid = cur;
					cur = prev->next_primitive;
				}
				if (cur == NULL) {
					break;
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}


void reset_tried_but_unused_cluster_placements(
		INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	flush_intermediate_queues(cluster_placement_stats);
}


/* Quick, additional filter to see if root is feasible for molecule 

   Limitation: This code can absorb a single atom by a "forced connection".  A forced connection is one where there is no interconnect flexibility connecting
               two primitives so if one primitive is used, then the other must also be used.

			   TODO: jluu - Many ways to make this either more efficient or more robust.
							1.  For forced connections, I can get the packer to try forced connections first thus avoid trying out other locations that 
							    I know are bad thus saving runtime and potentially improving robustness because the placement cost function is not always 100%.
							2.  I need to extend this so that molecules can be pulled in instead of just atoms.
*/
static boolean root_passes_early_filter(INP t_pb_graph_node *root, INP t_pack_molecule *molecule, INP int clb_index) {
	int i, j;
	boolean feasible;
	t_logical_block *root_block;
	t_model_ports *model_port;
	int inet;
	int isink;
	t_pb_graph_pin *sink_pb_graph_pin;

	feasible = TRUE;
	root_block = molecule->logical_block_ptrs[molecule->root];
	for(i = 0; feasible && i < root->num_output_ports; i++) {
		for(j = 0; feasible && j < root->num_output_pins[i]; j++) {
			if(root->output_pins[i][j].is_forced_connection) {
				model_port = root->output_pins[i][j].port->model_port;
				inet = root_block->output_nets[model_port->index][j];
				if(inet != OPEN) {
					/* This output pin has a dedicated connection to one output, make sure that molecule works */
					if(molecule->type == MOLECULE_SINGLE_ATOM) {
						feasible = FALSE; /* There is only one case where an atom can fit in here, so by default, feasibility is false unless proven otherwise */
						if(g_atoms_nlist.net[inet].num_sinks() == 1) {
							isink = g_atoms_nlist.net[inet].pins[1].block;
							if(logical_block[isink].clb_index == clb_index) {
								sink_pb_graph_pin = &root->output_pins[i][j];
								while(sink_pb_graph_pin->num_output_edges != 0) {
									assert(sink_pb_graph_pin->num_output_edges == 1);
									assert(sink_pb_graph_pin->output_edges[0]->num_output_pins == 1);
									sink_pb_graph_pin = sink_pb_graph_pin->output_edges[0]->output_pins[0];
								}
								if(sink_pb_graph_pin->parent_node == logical_block[isink].pb->pb_graph_node) {
									/* There is a logical block mapped to the physical position that pulls in the atom in question */
									feasible = TRUE;
								}
							}
						}
					}
				}
			}
		}
	}
	return feasible;
}

