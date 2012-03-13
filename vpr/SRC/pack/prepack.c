/* 
Prepacking: Group together technology-mapped netlist blocks before packing.  This gives hints to the packer on what groups of blocks to keep together during packing.
Primary purpose 1) "Forced" packs (eg LUT+FF pair)
                2) Carry-chains


Duties: Find pack patterns in architecture, find pack patterns in netlist.

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
#include "hash.h"
#include "prepack.h"


static int add_pattern_name_to_hash(INOUTP struct s_hash **nhash, INP char *pattern_name, INOUTP int *ncount);
static void discover_pattern_names_in_pb_graph_node(INOUTP t_pb_graph_node *pb_graph_node, INOUTP struct s_hash **nhash, INOUTP int *ncount);
static t_pack_patterns *alloc_and_init_pattern_list_from_hash(INP int ncount, INOUTP struct s_hash **nhash);
static t_pb_graph_edge * find_expansion_edge_of_pattern(INP int pattern_index, INP t_pb_graph_node *pb_graph_node);
static void forward_expand_pack_pattern_from_edge(INP t_pb_graph_edge *expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index);
static void backward_expand_pack_pattern_from_edge(INP t_pb_graph_edge* expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index,
												  INP t_pb_graph_pin *destination_pin, INP t_pack_pattern_block *destination_block);

/**
 * Find all packing patterns in architecture 
 * [0..num_packing_patterns-1]
 *
 * Limitations: Currently assumes that forced pack nets must be single-fanout, more complicated structures should probably be handled either downstream (general packing) or upstream (in tech mapping)
 *              If this limitation is too constraining, code is designed so that this limitation can be removed
*/
t_pack_patterns *alloc_and_load_pack_patterns(OUTP int *num_packing_patterns) {
	int i, j, ncount;
	struct s_hash **nhash;
	t_pack_patterns *list_of_packing_patterns;
	t_pb_graph_edge *expansion_edge;

	/* alloc and initialize array of packing patterns based on architecture complex blocks */
	nhash = alloc_hash_table();
	ncount = 0;
	for(i = 0; i < num_types; i++) {
		discover_pattern_names_in_pb_graph_node(type_descriptors[i].pb_graph_head, nhash, &ncount);
	}

	list_of_packing_patterns = alloc_and_init_pattern_list_from_hash(ncount, nhash);
	
	/* load packing patterns by traversing the edges to find edges belonging to pattern */
	for(i = 0; i < ncount; i++) {
		for(j = 0; j < num_types; j++) {
			expansion_edge = find_expansion_edge_of_pattern(i, type_descriptors[j].pb_graph_head);
			if(expansion_edge == NULL) {
				continue;
			}
			backward_expand_pack_pattern_from_edge(expansion_edge, list_of_packing_patterns, i, NULL, NULL);
			break;
		}
	}	
	

	free_hash_table(nhash);

	*num_packing_patterns = ncount;
	return list_of_packing_patterns;
}



/**
 * Adds pack pattern name to hashtable of pack pattern names.
 */
static int add_pattern_name_to_hash(INOUTP struct s_hash **nhash, INP char *pattern_name, INOUTP int *ncount) {
	struct s_hash *hash_value;

	hash_value = insert_in_hash_table(nhash, pattern_name, *ncount);
	if(hash_value->count == 1) {
		assert(*ncount == hash_value->index);
		(*ncount)++;
	}
	return hash_value->index;
}

/**
 * Locate all pattern names 
 * Side-effect: set all pb_graph_node temp_scratch_pad field to NULL
 */
static void discover_pattern_names_in_pb_graph_node(INOUTP t_pb_graph_node *pb_graph_node, INOUTP struct s_hash **nhash, INOUTP int *ncount) {
	int i, j, k, m;
	int index;
	/* Iterate over all edges to discover if an edge in current physical block belongs to a pattern 
	   If edge does, then record the name of the pattern in a hash table
	*/

	if(pb_graph_node == NULL) {
		return;
	}

	pb_graph_node->temp_scratch_pad = NULL;
	
	for(i = 0; i < pb_graph_node->num_input_ports; i++) {
		for(j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					index = add_pattern_name_to_hash(nhash, pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if(pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices == NULL) {
						pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices = my_malloc(pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->num_output_ports; i++) {
		for(j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->output_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					index = add_pattern_name_to_hash(nhash, pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if(pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices == NULL) {
						pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices = my_malloc(pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for(j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->clock_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					index = add_pattern_name_to_hash(nhash, pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if(pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices == NULL) {
						pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices = my_malloc(pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for(j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for(k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				discover_pattern_names_in_pb_graph_node(&pb_graph_node->child_pb_graph_nodes[i][j][k], nhash, ncount);
			}
		}
	}
}

/**
 * Allocates memory for models and loads the name of the packing pattern so that it can be identified and loaded with
 * more complete information later
 */
static t_pack_patterns *alloc_and_init_pattern_list_from_hash(INP int ncount, INOUTP struct s_hash **nhash) {
	t_pack_patterns *nlist;
	struct s_hash_iterator hash_iter;
	struct s_hash *curr_pattern;
	
	nlist = my_calloc(ncount, sizeof(t_pack_patterns));

	hash_iter = start_hash_table_iterator();
	curr_pattern = get_next_hash(nhash, &hash_iter);	
	while(curr_pattern != NULL) {
		assert(nlist[curr_pattern->index].name == NULL);
		nlist[curr_pattern->index].name = my_strdup(curr_pattern->name);
		nlist[curr_pattern->index].root_block = NULL;
		nlist[curr_pattern->index].index = curr_pattern->index;		
		curr_pattern = get_next_hash(nhash, &hash_iter);
	}
	return nlist;
}


/**
 * Locate first edge that belongs to pattern index 
 */
static t_pb_graph_edge * find_expansion_edge_of_pattern(INP int pattern_index,
														INP t_pb_graph_node *pb_graph_node) {
	int i, j, k, m;
	t_pb_graph_edge * edge;
	/* Iterate over all edges to discover if an edge in current physical block belongs to a pattern 
	   If edge does, then return that edge
	*/

	if(pb_graph_node == NULL) {
		return NULL;
	}
	
	for(i = 0; i < pb_graph_node->num_input_ports; i++) {
		for(j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if(pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return pb_graph_node->input_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->num_output_ports; i++) {
		for(j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->output_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if(pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return pb_graph_node->output_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for(j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			for(k = 0; k < pb_graph_node->clock_pins[i][j].num_output_edges; k++) {
				for(m = 0; m < pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if(pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return pb_graph_node->clock_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for(i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for(j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for(k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				edge = find_expansion_edge_of_pattern(pattern_index, &pb_graph_node->child_pb_graph_nodes[i][j][k]);
				if(edge != NULL) {
					return edge;
				}
			}
		}
	}
	return NULL;
}

/** 
 * Find if receiver of edge is in the same pattern, if yes, add to pattern
 *  Convention: Connections are made on backward expansion only (to make future multi-fanout support easier) so this function will not update connections
 */
static void forward_expand_pack_pattern_from_edge(INP t_pb_graph_edge* expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index) {
  	int i, j, k;
	int iport, ipin, iedge;
	boolean found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *destination_block;
	t_pb_graph_node *destination_pb_graph_node;
	
	found = FALSE;

	for(i = 0; i < expansion_edge->num_output_pins; i++) {
		if(expansion_edge->output_pins[i]->parent_node->pb_type->num_modes == 0) {
			destination_pb_graph_node = expansion_edge->output_pins[i]->parent_node;
			assert(found == FALSE); /* Check assumption that each forced net has only one fan-out */
			/* This is the destination node */
			found = TRUE;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			if(destination_pb_graph_node->temp_scratch_pad == NULL || ((t_pack_pattern_block*)destination_pb_graph_node->temp_scratch_pad)->pattern_index != curr_pattern_index) {
				destination_block = my_calloc(1, sizeof(t_pack_pattern_block));
				destination_pb_graph_node->temp_scratch_pad = (void *)destination_block;
				destination_block->pattern_index = curr_pattern_index;
				destination_block->pb_type = destination_pb_graph_node->pb_type;
				for(iport = 0; iport < destination_pb_graph_node->num_input_ports; iport++) {
					for(ipin = 0; ipin < destination_pb_graph_node->num_input_pins[iport]; ipin++) {
						for(iedge = 0; iedge < destination_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(destination_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	&destination_pb_graph_node->input_pins[iport][ipin], 
																	destination_block);
						}
					}
				}
				for(iport = 0; iport < destination_pb_graph_node->num_output_ports; iport++) {
					for(ipin = 0; ipin < destination_pb_graph_node->num_output_pins[iport]; ipin++) {
						for(iedge = 0; iedge < destination_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(destination_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index);
						}
					}
				}
				for(iport = 0; iport < destination_pb_graph_node->num_clock_ports; iport++) {
					for(ipin = 0; ipin < destination_pb_graph_node->num_clock_pins[iport]; ipin++) {
						for(iedge = 0; iedge < destination_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(destination_pb_graph_node->clock_pins[iport][ipin].input_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	&destination_pb_graph_node->clock_pins[iport][ipin], 
																	destination_block);
						}
					}
				}
			}
		} else {
			for(j = 0; j < expansion_edge->output_pins[i]->num_output_edges; j++) {
				for(k = 0; k < expansion_edge->output_pins[i]->output_edges[j]->num_pack_patterns; k++) {
					if(expansion_edge->output_pins[i]->output_edges[j]->pack_pattern_indices[k] == curr_pattern_index || 
					   expansion_edge->output_pins[i]->output_edges[j]->infer_pattern == TRUE) {
						assert(found == FALSE); /* Check assumption that each forced net has only one fan-out */
						found = TRUE;
						forward_expand_pack_pattern_from_edge(expansion_edge->output_pins[i]->output_edges[j], list_of_packing_patterns, curr_pattern_index);
					}
				}
			}
		}
	}

}

/** 
 * Find if driver of edge is in the same pattern, if yes, add to pattern
 *  Convention: Connections are made on backward expansion only (to make future multi-fanout support easier) so this function must update both source and destination blocks
 */
static void backward_expand_pack_pattern_from_edge(INP t_pb_graph_edge* expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index,
												  INP t_pb_graph_pin *destination_pin, INP t_pack_pattern_block *destination_block) {
	int i, j, k;
	int iport, ipin, iedge;
	boolean found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *source_block;
	t_pb_graph_node *source_pb_graph_node;
	t_pack_pattern_connections *pack_pattern_connection;
	
	found = FALSE;

	for(i = 0; i < expansion_edge->num_input_pins; i++) {
		if(expansion_edge->input_pins[i]->parent_node->pb_type->num_modes == 0) {
			source_pb_graph_node = expansion_edge->input_pins[i]->parent_node;
			assert(found == FALSE); /* Check assumption that each forced net has only one fan-out */
			/* This is the source node for destination */
			found = TRUE;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			if(source_pb_graph_node->temp_scratch_pad == NULL || ((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index != curr_pattern_index) {
				source_block = my_calloc(1, sizeof(t_pack_pattern_block));
				source_pb_graph_node->temp_scratch_pad = (void *)source_block;
				source_block->pattern_index = curr_pattern_index;
				source_block->pb_type = source_pb_graph_node->pb_type;
				for(iport = 0; iport < source_pb_graph_node->num_input_ports; iport++) {
					for(ipin = 0; ipin < source_pb_graph_node->num_input_pins[iport]; ipin++) {
						for(iedge = 0; iedge < source_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(source_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	&source_pb_graph_node->input_pins[iport][ipin], 
																	source_block);
						}
					}
				}
				for(iport = 0; iport < source_pb_graph_node->num_output_ports; iport++) {
					for(ipin = 0; ipin < source_pb_graph_node->num_output_pins[iport]; ipin++) {
						for(iedge = 0; iedge < source_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(source_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index);
						}
					}
				}
				for(iport = 0; iport < source_pb_graph_node->num_clock_ports; iport++) {
					for(ipin = 0; ipin < source_pb_graph_node->num_clock_pins[iport]; ipin++) {
						for(iedge = 0; iedge < source_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(source_pb_graph_node->clock_pins[iport][ipin].input_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	&source_pb_graph_node->clock_pins[iport][ipin], 
																	source_block);
						}
					}
				}
			}
			if(destination_pin != NULL) {
				assert(((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index != curr_pattern_index);
				source_block = (t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad;
				pack_pattern_connection = my_calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;
				pack_pattern_connection->next = source_block->connections;
				source_block->connections = pack_pattern_connection;

				pack_pattern_connection = my_calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;
				pack_pattern_connection->next = destination_block->connections;
				destination_block->connections = pack_pattern_connection;
			}

		} else {
			for(j = 0; j < expansion_edge->input_pins[i]->num_input_edges; j++) {
				for(k = 0; k < expansion_edge->input_pins[i]->input_edges[j]->num_pack_patterns; k++) {
					if(expansion_edge->input_pins[i]->input_edges[j]->pack_pattern_indices[k] == curr_pattern_index || 
					   expansion_edge->input_pins[i]->input_edges[j]->infer_pattern == TRUE) {
						assert(found == FALSE); /* Check assumption that each forced net has only one fan-out */
						found = TRUE;
						backward_expand_pack_pattern_from_edge(expansion_edge->input_pins[i]->input_edges[j], list_of_packing_patterns, curr_pattern_index, destination_pin, destination_block);
					}
				}
			}
		}
	}
}



