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
static float compute_primitive_cost(INP t_pb_graph_node *primitive);
static void requeue_primitive(INOUTP t_cluster_placement_stats *cluster_placement_stats, t_cluster_placement_primitive *cluster_placement_primitive);


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
		load_cluster_placement_stats_for_pb_graph_node(&cluster_placement_stats_list[i], type_descriptors[i].pb_graph_head);
	}
	return cluster_placement_stats_list;
}

/**
 * Resets one cluster placement stats by clearing incremental costs and returning all primitives to valid queue
 */
void reset_cluster_placement_stats(INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	t_cluster_placement_primitive *cur, *next;
	int i;
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
	for(i = 0; i < cluster_placement_stats->num_pb_types; i++) {
		assert(cluster_placement_stats->valid_primitives[i] != NULL);
		cur = cluster_placement_stats->valid_primitives[i];
		while(cur != NULL) {
			cur->incremental_cost = 0;
			cur = cur->next_primitive;
		}
	}
	cluster_placement_stats->curr_logical_block = NULL;
}

/**
 * Finalize in-flight primitives, either accept or reject
 */
void commit_in_flight_primitives(INOUTP t_cluster_placement_stats *cluster_placement_stats) {
	/* jedit do at home */
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
			cur = cluster_placement_stats_list[i].valid_primitives[j];
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
		}
	}
	if(success == FALSE) {
		assert(null_index != OPEN);
		cluster_placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i];
		cluster_placement_stats->valid_primitives[i] = cluster_placement_primitive;
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
		pb_graph_node->cluster_placement_primitive = placement_primitive;
		placement_primitive->base_cost = compute_primitive_cost(pb_graph_node);
		success = FALSE;
		i = 0;
		while(success == FALSE) {
			if(cluster_placement_stats->valid_primitives[i] == NULL || 
				cluster_placement_stats->valid_primitives[i]->pb_graph_node->pb_type == pb_graph_node->pb_type) {
				if(cluster_placement_stats->valid_primitives[i] == NULL) {
					cluster_placement_stats->num_pb_types++;
				}
				success = TRUE;
				placement_primitive->next_primitive = cluster_placement_stats->valid_primitives[i];
				cluster_placement_stats->valid_primitives[i] = placement_primitive;
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

/* Determine cost for using primitive, should use primitives of low cost before selecting primitives of high cost
   For now, assume primitives that have a lot of pins are scarcer than those without so use primitives with less pins before those with more
*/
static float compute_primitive_cost(INP t_pb_graph_node *primitive) {
	return (primitive->pb_type->num_input_pins + primitive->pb_type->num_output_pins + primitive->pb_type->num_clock_pins);
}

