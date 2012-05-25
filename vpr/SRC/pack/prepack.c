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
#include "vpr_utils.h"
#include "ReadOptions.h"

/*****************************************/
/*Local Function Declaration			 */
/*****************************************/
static int add_pattern_name_to_hash(INOUTP struct s_hash **nhash, INP char *pattern_name, INOUTP int *ncount);
static void discover_pattern_names_in_pb_graph_node(INOUTP t_pb_graph_node *pb_graph_node, INOUTP struct s_hash **nhash, INOUTP int *ncount);
static t_pack_patterns *alloc_and_init_pattern_list_from_hash(INP int ncount, INOUTP struct s_hash **nhash);
static t_pb_graph_edge * find_expansion_edge_of_pattern(INP int pattern_index, INP t_pb_graph_node *pb_graph_node);
static void forward_expand_pack_pattern_from_edge(INP t_pb_graph_edge *expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index, INP int *L_num_blocks);
static void backward_expand_pack_pattern_from_edge(INP t_pb_graph_edge* expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index,
												  INP t_pb_graph_pin *destination_pin, INP t_pack_pattern_block *destination_block, INP int *L_num_blocks);
static t_pack_molecule *try_create_molecule(INP t_pack_patterns *list_of_pack_patterns, INP int pack_pattern_index, INP int block_index);
static boolean try_expand_molecule(INOUTP t_pack_molecule *molecule, INP int logical_block_index, INP t_pack_pattern_block *current_pattern_block);
static void print_pack_molecules(INP char *fname, INP t_pack_patterns *list_of_pack_patterns, INP int num_pack_patterns, INP t_pack_molecule *list_of_molecules);

/*****************************************/
/*Function Definitions					 */
/*****************************************/

/**
 * Find all packing patterns in architecture 
 * [0..num_packing_patterns-1]
 *
 * Limitations: Currently assumes that forced pack nets must be single-fanout as this covers all the reasonable architectures we wanted.
				More complicated structures should probably be handled either downstream (general packing) or upstream (in tech mapping)
 *              If this limitation is too constraining, code is designed so that this limitation can be removed
*/
t_pack_patterns *alloc_and_load_pack_patterns(OUTP int *num_packing_patterns) {
	int i, j, ncount;
	int L_num_blocks;
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
			L_num_blocks = 0;
			list_of_packing_patterns[i].base_cost = 0;
			backward_expand_pack_pattern_from_edge(expansion_edge, list_of_packing_patterns, i, NULL, NULL, &L_num_blocks);
			list_of_packing_patterns[i].num_blocks = L_num_blocks;
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
static void forward_expand_pack_pattern_from_edge(INP t_pb_graph_edge* expansion_edge, INOUTP t_pack_patterns *list_of_packing_patterns, INP int curr_pattern_index, INP int *L_num_blocks) {
  	int i, j, k;
	int iport, ipin, iedge;
	boolean found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *destination_block = NULL;
	t_pb_graph_node *destination_pb_graph_node = NULL;
	
	found = FALSE;

	for(i = 0; i < expansion_edge->num_output_pins; i++) {
		if(expansion_edge->output_pins[i]->parent_node->pb_type->num_modes == 0) {
			destination_pb_graph_node = expansion_edge->output_pins[i]->parent_node;
			assert(found == FALSE);
/* Check assumption that each forced net has only one fan-out */
/* This is the destination node */
found = TRUE;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			if(destination_pb_graph_node->temp_scratch_pad == NULL || ((t_pack_pattern_block*)destination_pb_graph_node->temp_scratch_pad)->pattern_index != curr_pattern_index) {
				destination_block = my_calloc(1, sizeof(t_pack_pattern_block));
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(destination_pb_graph_node);
				destination_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
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
																	destination_block,
																	L_num_blocks);
						}
					}
				}
				for(iport = 0; iport < destination_pb_graph_node->num_output_ports; iport++) {
					for(ipin = 0; ipin < destination_pb_graph_node->num_output_pins[iport]; ipin++) {
						for(iedge = 0; iedge < destination_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(destination_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	L_num_blocks);
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
																	destination_block,
																	L_num_blocks);
						}
					}
				}
			}
		} else {
			for(j = 0; j < expansion_edge->output_pins[i]->num_output_edges; j++) {
				if(expansion_edge->output_pins[i]->output_edges[j]->infer_pattern == TRUE) {
					forward_expand_pack_pattern_from_edge(expansion_edge->output_pins[i]->output_edges[j], list_of_packing_patterns, curr_pattern_index, L_num_blocks);
				} else {
					for(k = 0; k < expansion_edge->output_pins[i]->output_edges[j]->num_pack_patterns; k++) {
						if(expansion_edge->output_pins[i]->output_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
							assert(found == FALSE);
/* Check assumption that each forced net has only one fan-out */
found = TRUE;
							forward_expand_pack_pattern_from_edge(expansion_edge->output_pins[i]->output_edges[j], list_of_packing_patterns, curr_pattern_index, L_num_blocks);
						}
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
												  INP t_pb_graph_pin *destination_pin, INP t_pack_pattern_block *destination_block, INP int *L_num_blocks) {
	int i, j, k;
	int iport, ipin, iedge;
	boolean found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *source_block = NULL;
	t_pb_graph_node *source_pb_graph_node = NULL;
	t_pack_pattern_connections *pack_pattern_connection = NULL;
	
	found = FALSE;
	for(i = 0; i < expansion_edge->num_input_pins; i++) {
		if(expansion_edge->input_pins[i]->parent_node->pb_type->num_modes == 0) {
			source_pb_graph_node = expansion_edge->input_pins[i]->parent_node;
			assert(found == FALSE);
/* Check assumption that each forced net has only one fan-out */
/* This is the source node for destination */
found = TRUE;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			source_block = (t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad;
			if(source_block == NULL || source_block->pattern_index != curr_pattern_index) {
				source_block = my_calloc(1, sizeof(t_pack_pattern_block));
				source_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(source_pb_graph_node);
				source_pb_graph_node->temp_scratch_pad = (void *)source_block;
				source_block->pattern_index = curr_pattern_index;
				source_block->pb_type = source_pb_graph_node->pb_type;

				if(list_of_packing_patterns[curr_pattern_index].root_block == NULL) {
					list_of_packing_patterns[curr_pattern_index].root_block = source_block;
				}

				for(iport = 0; iport < source_pb_graph_node->num_input_ports; iport++) {
					for(ipin = 0; ipin < source_pb_graph_node->num_input_pins[iport]; ipin++) {
						for(iedge = 0; iedge < source_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(source_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	&source_pb_graph_node->input_pins[iport][ipin], 
																	source_block,
																	L_num_blocks);
						}
					}
				}
				for(iport = 0; iport < source_pb_graph_node->num_output_ports; iport++) {
					for(ipin = 0; ipin < source_pb_graph_node->num_output_pins[iport]; ipin++) {
						for(iedge = 0; iedge < source_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(source_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
																	list_of_packing_patterns,
																	curr_pattern_index,
																	L_num_blocks);
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
																	source_block,
																	L_num_blocks);
						}
					}
				}
			}
			if(destination_pin != NULL) {
				assert(
		((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index);
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

				if(source_block == destination_block) {
					printf(ERRTAG "Invalid packing pattern defined.  Source and destination block are the same (%s)\n", source_block->pb_type->name);
				}
			}
		} else {
			for(j = 0; j < expansion_edge->input_pins[i]->num_input_edges; j++) {
				if(expansion_edge->input_pins[i]->input_edges[j]->infer_pattern == TRUE) {
					backward_expand_pack_pattern_from_edge(expansion_edge->input_pins[i]->input_edges[j], list_of_packing_patterns, curr_pattern_index, destination_pin, destination_block, L_num_blocks);
				} else {
					for(k = 0; k < expansion_edge->input_pins[i]->input_edges[j]->num_pack_patterns; k++) {
						if(expansion_edge->input_pins[i]->input_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
							assert(found == FALSE);
/* Check assumption that each forced net has only one fan-out */
found = TRUE;
							backward_expand_pack_pattern_from_edge(expansion_edge->input_pins[i]->input_edges[j], list_of_packing_patterns, curr_pattern_index, destination_pin, destination_block, L_num_blocks);
						}
					}
				}
			}
		}
	}
}


/**
 * Pre-pack atoms in netlist to molecules
 * 1.  Single atoms are by definition a molecule.
 * 2.  Forced pack molecules are groupings of atoms that matches a t_pack_pattern definition.
 * 3.  Chained molecules are molecules that follow a carry-chain style pattern: ie. a single linear chain that can be split across multiple complex blocks
 */
t_pack_molecule *alloc_and_load_pack_molecules(INP t_pack_patterns *list_of_pack_patterns, INP int num_packing_patterns, OUTP int *num_pack_molecule) {
	int i, j;
	t_pack_molecule *list_of_molecules_head;
	t_pack_molecule *cur_molecule;
	int L_num_blocks;

	cur_molecule = list_of_molecules_head = NULL;

	/* Find forced pack patterns */
	/* TODO: Need to properly empirically investigate the right base cost function values (gain variable) and do some normalization */
	for(i = 0; i < num_packing_patterns; i++) {
		for(j = 0; j < num_logical_blocks; j++) {
			cur_molecule = try_create_molecule(list_of_pack_patterns, i, j);
			if(cur_molecule != NULL) {
				cur_molecule->next = list_of_molecules_head;
				/* In the event of multiple molecules with the same logical block pattern, bias to use the molecule with less costly physical resources first */
				/* TODO: Need to normalize magical number 100 */
				cur_molecule->base_gain = cur_molecule->num_blocks - (cur_molecule->pack_pattern->base_cost / 100); 
				list_of_molecules_head = cur_molecule;
			}
		}
	}

	/* jedit TODO: Find chain patterns */


		/* List all logical blocks as a molecule for blocks that do not belong to any molecules.
		   This allows the packer to be consistent as it now packs molecules only instead of atoms and molecules

		   If a block belongs to a molecule, then carrying the single atoms around can make the packing problem
		   more difficult because now it needs to consider splitting molecules.

		   TODO: Need to handle following corner case, if I have a LUT -> FF -> multiplier, LUT + FF is one molecule and FF + multiplier is another,
		   need to ensure that this is packable.  Solution is probably to allow for single atom molecules until something a particular packing forces
		   another molecule to get broken
		*/
	for(i = 0; i < num_logical_blocks; i++) {
		if(logical_block[i].packed_molecules == NULL) {
			cur_molecule = (t_pack_molecule*)my_calloc(1, sizeof(t_pack_molecule));
			cur_molecule->valid = TRUE;
			cur_molecule->type = MOLECULE_SINGLE_ATOM;
			cur_molecule->num_blocks = 1;
			cur_molecule->root = 0;
			cur_molecule->num_ext_inputs = logical_block[i].used_input_pins;
			cur_molecule->chain_pattern = NULL;
			cur_molecule->pack_pattern = NULL;
			cur_molecule->logical_block_ptrs = my_malloc(1 * sizeof(t_logical_block*));
			cur_molecule->logical_block_ptrs[0] = &logical_block[i];
			cur_molecule->next = list_of_molecules_head;
			cur_molecule->base_gain = 1;
			list_of_molecules_head = cur_molecule;

			logical_block[i].packed_molecules = my_calloc(1, sizeof(struct s_linked_vptr));
			logical_block[i].packed_molecules->data_vptr = (void*)cur_molecule;
		}
	}

	/* Reduce incentive to use logical blocks as standalone blocks when molecules already exist */
	for(i = 0; i < num_logical_blocks; i++) {
		cur_molecule = (t_pack_molecule*)logical_block[i].packed_molecules->data_vptr;
		L_num_blocks = 1;
		while(cur_molecule != NULL) {
			if(cur_molecule->num_blocks > 1) {
				L_num_blocks = cur_molecule->num_blocks;
				break;
			}
			cur_molecule = cur_molecule->next;
		}
		if(L_num_blocks > 1) {
			cur_molecule = (t_pack_molecule*)logical_block[i].packed_molecules->data_vptr;
			while(cur_molecule != NULL) {
				if(cur_molecule->num_blocks == 1) {
					cur_molecule->base_gain /= L_num_blocks;
				}
				cur_molecule = cur_molecule->next;
			}
		}
	}


	if (GetEchoOption()){
		print_pack_molecules("prepack_molecules_and_patterns.echo", list_of_pack_patterns, num_packing_patterns, list_of_molecules_head);
	}

	return list_of_molecules_head;
}

/**
 * Given a pattern and a logical block to serve as the root block, determine if the candidate logical block serving as the root node matches the pattern
 * If yes, return the molecule with this logical block as the root, if not, return NULL
 * Limitations: Currently assumes that forced pack nets must be single-fanout as this covers all the reasonable architectures we wanted
				More complicated structures should probably be handled either downstream (general packing) or upstream (in tech mapping)
 *              If this limitation is too constraining, code is designed so that this limitation can be removed
 * Side Effect: If successful, link atom to molecule
 */
static t_pack_molecule *try_create_molecule(INP t_pack_patterns *list_of_pack_patterns, INP int pack_pattern_index, INP int block_index) {
	int i;
	t_pack_molecule *molecule;
	struct s_linked_vptr *molecule_linked_list;
	
	molecule = my_calloc(1, sizeof(t_pack_molecule));
	molecule->valid = TRUE;
	molecule->type = MOLECULE_FORCED_PACK;
	molecule->pack_pattern = &list_of_pack_patterns[pack_pattern_index];
	molecule->logical_block_ptrs = my_calloc(molecule->pack_pattern->num_blocks, sizeof(t_logical_block *));
	molecule->num_blocks = list_of_pack_patterns[pack_pattern_index].num_blocks;
	molecule->root = list_of_pack_patterns[pack_pattern_index].root_block->block_id;
	molecule->num_ext_inputs = 0;

	if(try_expand_molecule(molecule, block_index, molecule->pack_pattern->root_block) == TRUE) {
		/* Success! commit module */
		for(i = 0; i < molecule->pack_pattern->num_blocks; i++) {
			assert(molecule->logical_block_ptrs[i] != NULL);
molecule_linked_list = my_calloc(1, sizeof(struct s_linked_vptr));
			molecule_linked_list->data_vptr = (void *)molecule;
			molecule_linked_list->next = molecule->logical_block_ptrs[i]->packed_molecules;
			molecule->logical_block_ptrs[i]->packed_molecules = molecule_linked_list;
		}
	} else {
		/* Does not match pattern, free molecule */
		free(molecule->logical_block_ptrs);
		free(molecule);
		molecule = NULL;
	}
	
	return molecule;
}

/**
 * Determine if logical block can match with the pattern to form a molecule
 * return TRUE if it matches, return FALSE otherwise
 */
static boolean try_expand_molecule(INOUTP t_pack_molecule *molecule, INP int logical_block_index, INP t_pack_pattern_block *current_pattern_block) {
	int iport, ipin, inet;
	boolean success = FALSE;
	t_pack_pattern_connections *cur_pack_pattern_connection;
	
	/* If the block in the pattern has already been visited, then there is no need to revisit it */
	if(molecule->logical_block_ptrs[current_pattern_block->block_id] != NULL) {
		if(molecule->logical_block_ptrs[current_pattern_block->block_id] != &logical_block[logical_block_index]) {
			/* Mismatch between the visited block and the current block implies that the current netlist structure does not match the expected pattern, return fail */
			return FALSE;
		} else {
			return TRUE;
		}
	}

	/* This node has never been visited, expand it and explore neighbouring nodes */
	molecule->logical_block_ptrs[current_pattern_block->block_id] = &logical_block[logical_block_index]; /* store that this node has been visited */
	molecule->num_ext_inputs += logical_block[logical_block_index].used_input_pins;
	if(primitive_type_feasible(logical_block_index, current_pattern_block->pb_type)) {
		success = TRUE;
		/* If the primitive types match, check for connections */
		cur_pack_pattern_connection = current_pattern_block->connections;
		while(cur_pack_pattern_connection != NULL && success == TRUE) {
			if(cur_pack_pattern_connection->from_block == current_pattern_block) {
				/* find net corresponding to pattern */
				iport = cur_pack_pattern_connection->to_pin->port->model_port->index;
				ipin = cur_pack_pattern_connection->to_pin->pin_number;
				inet = logical_block[logical_block_index].output_nets[iport][ipin];
				
				/* Check if net is valid */
				if(vpack_net[inet].num_sinks != 1) { /* One fanout assumption */
					success = FALSE;
				} else {
					success = try_expand_molecule(molecule, vpack_net[inet].node_block[1], cur_pack_pattern_connection->to_block);
				}
			} else {
				assert(cur_pack_pattern_connection->to_block == current_pattern_block);
molecule->num_ext_inputs--; /* input to logical block is internal to molecule */
/* find net corresponding to pattern */
iport = cur_pack_pattern_connection->from_pin->port->model_port->index;
ipin = cur_pack_pattern_connection->from_pin->pin_number;
if(cur_pack_pattern_connection->to_pin->port->model_port->is_clock) {
	inet = logical_block[logical_block_index].clock_net;
} else {
	inet = logical_block[logical_block_index].input_nets[iport][ipin];
}
/* Check if net is valid */
if(vpack_net[inet].num_sinks != 1) { /* One fanout assumption */
	success = FALSE;
} else {
	success = try_expand_molecule(molecule, vpack_net[inet].node_block[0], cur_pack_pattern_connection->from_block);
}
}
cur_pack_pattern_connection = cur_pack_pattern_connection->next;
}
} else {
success = FALSE;
}

return success;
}

static void print_pack_molecules(INP char *fname, INP t_pack_patterns *list_of_pack_patterns, INP int num_pack_patterns, INP t_pack_molecule *list_of_molecules) {
int i;
FILE *fp;
t_pack_molecule *list_of_molecules_current;

fp = my_fopen(fname, "w", 0);

for(i = 0; i < num_pack_patterns; i++) {
fprintf(fp, "# of pack patterns %d\n", num_pack_patterns);
fprintf(fp, "pack pattern index %d block count %d name %s root %s\n", list_of_pack_patterns[i].index, list_of_pack_patterns[i].num_blocks,
list_of_pack_patterns[i].name, list_of_pack_patterns[i].root_block->pb_type->name);
}

list_of_molecules_current = list_of_molecules;
while(list_of_molecules_current != NULL) {
if(list_of_molecules_current->type == MOLECULE_SINGLE_ATOM) {
fprintf(fp, "\nmolecule type: atom\n");
fprintf(fp, "\tpattern index %d: logical block [%d] name %s\n", i,
	list_of_molecules_current->logical_block_ptrs[0]->index, list_of_molecules_current->logical_block_ptrs[0]->name);
} else if (list_of_molecules_current->type == MOLECULE_FORCED_PACK) {
fprintf(fp, "\nmolecule type: %s\n", list_of_molecules_current->pack_pattern->name);
for(i = 0; i < list_of_molecules_current->pack_pattern->num_blocks; i++) {
fprintf(fp, "\tpattern index %d: logical block [%d] name %s\n", i,
		list_of_molecules_current->logical_block_ptrs[i]->index, list_of_molecules_current->logical_block_ptrs[i]->name);
}
} else {
assert(list_of_molecules_current->type == MOLECULE_CHAIN);
fprintf(fp, "\nmolecule type: %s\n", list_of_molecules_current->chain_pattern->name);
}
list_of_molecules_current = list_of_molecules_current->next;
}

fclose(fp);
}

