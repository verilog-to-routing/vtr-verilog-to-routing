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
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "read_xml_arch_file.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "hash.h"
#include "cluster_placement.h"

/*****************************************/
/*Local Function Declaration
/*****************************************/
static void load_cluster_placement_stats_for_pb_graph_node(INOUTP t_cluster_placement_stats *cluster_placement_stats, INOUTP t_pb_graph_node *pb_graph_node);
static float compute_primitive_base_cost(INP t_pb_graph_node *primitive);
static void requeue_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, t_cluster_placement_primitive *cluster_placement_primitive);
static void commit_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, t_cluster_placement_primitive *cur);
static void update_primitive_cost_or_status(INP t_pb_graph_node *pb_graph_node, INP int incremental_cost, INP boolean valid);

/*****************************************/
/*Function Definitions
/*****************************************/

/**
 * [0..num_pb_types-1] array of cluster placement stats, one for each type_descriptors 
 */
t_cluster_placement_stats *alloc_and_load_cluster_placement_stats() {
	t_cluster_placement_stats *cluster_placement_stats_list;
	int i;
	
	cluster_placement_stats_list = my_calloc(num_types, sizeof(t_cluster_placement_stats));
	for(i = 0; i < num_types; i++) {
		cluster_placement_stats_list[i].valid_primitives = my_calloc(get_max_primitives_in_pb_type(type_descriptors[i].pb_type) + 1, sizeof(t_cluster_placement_primitive*)); /* too much memory allocated but shouldn't be a problem */
		cluster_placement_stats_list[i].curr_logical_blocks = NULL;
		load_cluster_placement_stats_for_pb_graph_node(&cluster_placement_stats_list[i], type_descriptors[i].pb_graph_head);
	}
	return cluster_placement_stats_list;
}

/**
 * get next list of primitives for list of logical blocks
 * primitives is the list of ptrs to primitives that matches with the list of logical_blocks (by index), assumes memory is preallocated
 *   - if this is a new block, requeue tried primitives and return a in-flight primitive list to try
 *   - if this is an old block, try another set of primitives
 * jedit corner case that I'm going to need to handle later - for multi-block packing, putting entire list on "tried" may not be a good idea because there might be overlap (eg. map A->B->A->B to physical structure A->B->A->B->A->B, if map to first 4 then fail, will not try last 4 since middle guys put to tried queue)
 *     THIS CORNER CASE IS NOT HANDLED, must consider in future, for the most part, I expect this to show up in poorly designed architectures and probably shouldn't be an issue, but best to handle just to be safe
 */
void get_next_primitive_list(INOUTP t_cluster_placement_stats *cluster_placement_stats, INP int* logical_block_list, INP int logical_block_list_size, INOUTP t_pb_graph_node **primitives_list) {
	t_cluster_placement_primitive *cur, *next;
	int i;
	if(cluster_placement_stats->curr_logical_blocks != logical_block_list) {
		/* New block, requeue tried primitives and in-flight primitives */
		cur = cluster_placement_stats->tried;
		while(cur != NULL) {
			next = cur->next_primitive;
			requeue_primitive(cluster_placement_stats, cur);
			cur = next;
		}
		cur = cluster_placement_stats->in_flight;
		while(cur != NULL) {
			next = cur->next_primitive;
			requeue_primitive(cluster_placement_stats, cur);
			cur = next;
		}
		cluster_placement_stats->curr_logical_blocks = logical_block_list;
	} else {
		/* old block, put any primitives currently inflight to tried queue */
		cur = cluster_placement_stats->in_flight;
		i = 0;
		while(cur != NULL) {
			next = cur->next_primitive;
			cur->next_primitive = cluster_placement_stats->tried;
			cluster_placement_stats->tried = cur;
			i++;
		}
		assert(i == logical_block_list_size);
	}

	/* find next set of blocks 
	     1. Remove invalid blocks to invalid queue
		 2. Find lowest cost array of primitives that implements blocks
		 3. When found, move current blocks to in-flight, return lowest cost array of primitives
	*/
}

/**
 * Resets one cluster placement stats by clearing incremental costs and returning all primitives to valid queue
 */
void reset_cluster_placement_stats(INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	t_cluster_placement_primitive *cur, *next;
	int i;
	/* Requeue primitives */
	cur = cluster_placement_stats->tried;
	while(cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		cur = next;
	}
	cur = cluster_placement_stats->in_flight;
	while(cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		cur = next;
	}
	cur = cluster_placement_stats->invalid;
	while(cur != NULL) {
		next = cur->next_primitive;
		requeue_primitive(cluster_placement_stats, cur);
		cur = next;
	}
	/* reset flags and cost */
	for(i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		assert(cluster_placement_stats->valid_primitives[i] != NULL && cluster_placement_stats->valid_primitives[i]->next_primitive != NULL);
		cur = cluster_placement_stats->valid_primitives[i]->next_primitive;
		while(cur != NULL) {
			cur->incremental_cost = 0;
			cur->valid = TRUE;
			cur = cur->next_primitive;
		}
	}
	cluster_placement_stats->curr_logical_blocks = NULL;
}

/**
 * Finalize in-flight primitives, either accept or reject
 * If accept, put in-flight primitives to invalid queue, update costs
 * If reject, place in-flight primitives to tried queue
 */
void finalize_in_flight_primitives(INOUTP t_cluster_placement_stats *cluster_placement_stats, INP boolean accept) {
	t_cluster_placement_primitive *cur, *next;
	cur = cluster_placement_stats->in_flight;
	if(accept) {
		while(cur != NULL) {
			next = cur->next_primitive;
			commit_primitive(cluster_placement_stats, cur);
			cur = next;
		}
	} else {
		while(cur != NULL) {
			next = cur->next_primitive;
			cur->next_primitive = cluster_placement_stats->tried;
			cluster_placement_stats->tried = cur;
			cur = next;
		}
	}
}


/** 
 * Free linked lists found in cluster_placement_stats_list 
 */
void free_cluster_placement_stats(INOUTP t_cluster_placement_stats *cluster_placement_stats_list) {
	t_cluster_placement_primitive *cur, *next;
	int i, j;
	for(i = 0; i < num_types; i++) {
		cur = cluster_placement_stats_list[i].tried;
		while(cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		cur = cluster_placement_stats_list[i].in_flight;
		while(cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		cur = cluster_placement_stats_list[i].invalid;
		while(cur != NULL) {
			next = cur->next_primitive;
			free(cur);
			cur = next;
		}
		for(j = 0; j < cluster_placement_stats_list[i].num_pb_types; j++) {
			cur = cluster_placement_stats_list[i].valid_primitives[j]->next_primitive;
			while(cur != NULL) {
				next = cur->next_primitive;
				free(cur);
				cur = next;
			}
		}
	}
	free(cluster_placement_stats_list);
}

/**
 * Put primitive back on queue of valid primitives
 */
static void requeue_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, t_cluster_placement_primitive *cluster_placement_primitive) {
	int i;
	int null_index;
	boolean success;
	null_index = OPEN;

	success = FALSE;
	for(i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		if(cluster_placement_stats->valid_primitives[i]->next_primitive == NULL) {
			null_index = i;
			continue;
		}
		if(cluster_placement_primitive->pb_graph_node->pb_type == cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type) {
			success = TRUE;
			cluster_placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
			cluster_placement_stats->valid_primitives[i]->next_primitive = cluster_placement_primitive;
			cluster_placement_primitive->valid = TRUE;
		}
	}
	if(success == FALSE) {
		assert(null_index != OPEN);
		cluster_placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
		cluster_placement_stats->valid_primitives[i]->next_primitive = cluster_placement_primitive;
	}
}

/** 
 * Add any primitives found in pb_graph_nodes to cluster_placement_stats
 * Adds backward link from pb_graph_node to cluster_placement_primitive
 */
static void load_cluster_placement_stats_for_pb_graph_node(INOUTP t_cluster_placement_stats *cluster_placement_stats, INOUTP t_pb_graph_node *pb_graph_node) {
	int i, j, k;
	t_cluster_placement_primitive *placement_primitive;
	const t_pb_type *pb_type = pb_graph_node->pb_type;
	boolean success;
	if (pb_type->modes == 0) {
		placement_primitive = my_calloc(1, sizeof(t_cluster_placement_primitive));
		placement_primitive->pb_graph_node = pb_graph_node;
		placement_primitive->valid = TRUE;
		pb_graph_node->cluster_placement_primitive = placement_primitive;
		placement_primitive->base_cost = compute_primitive_base_cost(pb_graph_node);
		success = FALSE;
		i = 0;
		while(success == FALSE) {
			if(cluster_placement_stats->valid_primitives[i] == NULL || 
				cluster_placement_stats->valid_primitives[i]->next_primitive->pb_graph_node->pb_type == pb_graph_node->pb_type) {
				if(cluster_placement_stats->valid_primitives[i] == NULL) {
					cluster_placement_stats->valid_primitives[i]= my_calloc(1, sizeof(t_cluster_placement_primitive)); /* head of linked list is empty, makes it easier to remove nodes later */
					cluster_placement_stats->num_pb_types++;
				}
				success = TRUE;
				placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i]->next_primitive;
				cluster_placement_stats->valid_primitives[i]->next_primitive = placement_primitive;
			}
			i++;
		}
	} else {
		for(i = 0; i < pb_type->num_modes; i++) {
			for(j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				for(k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
					load_cluster_placement_stats_for_pb_graph_node(cluster_placement_stats, &pb_graph_node->child_pb_graph_nodes[i][j][k]);
				}
			}
		}
	}
}

/**
 * Determine cost for using primitive, should use primitives of low cost before selecting primitives of high cost
   For now, assume primitives that have a lot of pins are scarcer than those without so use primitives with less pins before those with more
*/
static float compute_primitive_base_cost(INP t_pb_graph_node *primitive) {
	return (primitive->pb_type->num_input_pins + primitive->pb_type->num_output_pins + primitive->pb_type->num_clock_pins);
}

/**
 * Commit primitive, invalidate primitives blocked by mode assignment and update costs for primitives in same cluster as current
 * Costing is done to try to pack blocks closer to existing primitives
 *  actual value based on closest common ancestor to committed placement, the farther the ancestor, the less reduction in cost there is
 */
static void commit_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, t_cluster_placement_primitive *cur) {
	t_pb_graph_node *pb_graph_node, *skip;
	float incr_cost;
	int i, j, k;
	int valid_mode;
	
	cur->valid = FALSE;
	cur->next_primitive = cluster_placement_stats->invalid;
	cluster_placement_stats->invalid = cur;
	
	incr_cost = -0.01; /* cost of using a node drops as its neighbours are used, this drop should be small compared to scarcity values */

	pb_graph_node = cur->pb_graph_node;
	/* walk up pb_graph_node and update primitives of children */
	while(pb_graph_node->parent_pb_graph_node != NULL) {
		skip = pb_graph_node; /* do not traverse stuff that's already traversed */
		valid_mode = pb_graph_node->pb_type->parent_mode->index;
		pb_graph_node = pb_graph_node->parent_pb_graph_node;
		for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for(j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
				for(k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[k].num_pb; k++) {
					if(&pb_graph_node->child_pb_graph_nodes[i][j][k] != skip) {
						update_primitive_cost_or_status(&pb_graph_node->child_pb_graph_nodes[i][j][k], incr_cost, (i == valid_mode));
					}
				}
			}
		}
		incr_cost /= 10; /* blocks whose ancestor is further away in tree should be affected less than blocks closer in tree */
	}
}

/**
 * For sibling primitives of pb_graph node, decrease cost
 * For modes invalidated by pb_graph_node, invalidate primitive
 * int distance is the distance of current pb_graph_node from original
 */
static void update_primitive_cost_or_status(INP t_pb_graph_node *pb_graph_node, INP int incremental_cost, INP boolean valid) {
	int i, j, k;
	t_cluster_placement_primitive *placement_primitive;
	if(pb_graph_node->pb_type->num_modes == 0) {
		/* is primitive */
		if(valid) {
			placement_primitive = (t_cluster_placement_primitive*) pb_graph_node->cluster_placement_primitive;
			placement_primitive->incremental_cost += incremental_cost;
		} else {
			placement_primitive->valid = FALSE;
		}
	} else {
		for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
			for(j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
				for(k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[k].num_pb; k++) {
					update_primitive_cost_or_status(&pb_graph_node->child_pb_graph_nodes[i][j][k], incremental_cost, valid);
				}
			}
		}
	}
}



