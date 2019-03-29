/*
 * Prepacking: Group together technology-mapped netlist blocks before packing.
 * This gives hints to the packer on what groups of blocks to keep together during packing.
 * Primary purpose:
 *    1) "Forced" packs (eg LUT+FF pair)
 *    2) Carry-chains
 * Duties: Find pack patterns in architecture, find pack patterns in netlist.
 *
 * Author: Jason Luu
 * March 12, 2012
 */

#include <cstdio>
#include <cstring>
#include <map>
#include <queue>
using namespace std;

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "read_xml_arch_file.h"
#include "globals.h"
#include "atom_netlist.h"
#include "hash.h"
#include "prepack.h"
#include "vpr_utils.h"
#include "echo_files.h"

/*****************************************/
/*Local Function Declaration			 */
/*****************************************/
static int add_pattern_name_to_hash(t_hash **nhash,
		const char *pattern_name, int *ncount);
static void discover_pattern_names_in_pb_graph_node(
		t_pb_graph_node *pb_graph_node, t_hash **nhash,
		int *ncount);
static void forward_infer_pattern(t_pb_graph_pin *pb_graph_pin);
static void backward_infer_pattern(t_pb_graph_pin *pb_graph_pin);
static t_pack_patterns *alloc_and_init_pattern_list_from_hash(const int ncount,
		t_hash **nhash);
static t_pb_graph_edge * find_expansion_edge_of_pattern(const int pattern_index,
		const t_pb_graph_node *pb_graph_node);
static void forward_expand_pack_pattern_from_edge(
		const t_pb_graph_edge *expansion_edge,
		t_pack_patterns *list_of_packing_patterns,
		const int curr_pattern_index, int *L_num_blocks, const bool make_root_of_chain);
static void backward_expand_pack_pattern_from_edge(
		const t_pb_graph_edge* expansion_edge,
		t_pack_patterns *list_of_packing_patterns,
		const int curr_pattern_index, t_pb_graph_pin *destination_pin,
		t_pack_pattern_block *destination_block, int *L_num_blocks);
static int compare_pack_pattern(const t_pack_patterns *pattern_a, const t_pack_patterns *pattern_b);
static void free_pack_pattern(t_pack_pattern_block *pattern_block, t_pack_pattern_block **pattern_block_list);
static t_pack_molecule *try_create_molecule(
		t_pack_patterns *list_of_pack_patterns,
        std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
        const int pack_pattern_index,
		AtomBlockId blk_id);
static bool try_expand_molecule(t_pack_molecule *molecule,
        const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
		const AtomBlockId blk_id,
		const t_pack_pattern_block *current_pattern_block);
static void print_pack_molecules(const char *fname,
		const t_pack_patterns *list_of_pack_patterns, const int num_pack_patterns,
		const t_pack_molecule *list_of_molecules);
static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block(const AtomBlockId blk_id);
static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(const AtomBlockId blk_id, t_pb_graph_node *curr_pb_graph_node, float *cost);
static AtomBlockId find_new_root_atom_for_chain(const AtomBlockId blk_id, const t_pack_patterns *list_of_pack_pattern,
        const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules);

static std::vector<t_pb_graph_pin *> find_end_of_path(t_pb_graph_pin* input_pin, int pattern_index);

static void expand_search(t_pb_graph_pin * input_pin, std::queue<t_pb_graph_pin *>& pins_queue, int pattern_index);

/*****************************************/
/*Function Definitions					 */
/*****************************************/

/**
 * Find all packing patterns in architecture
 * [0..num_packing_patterns-1]
 *
 * Limitations: Currently assumes that forced pack nets must be single-fanout
 * as this covers all the reasonable architectures we wanted.
 * More complicated structures should probably be handled either downstream
 * (general packing) or upstream (in tech mapping).
 * If this limitation is too constraining, code is designed so that this limitation can be removed.
 */
t_pack_patterns *alloc_and_load_pack_patterns(int *num_packing_patterns) {
	int i, j, ncount, k;
	int L_num_blocks;
	t_hash **nhash;
	t_pack_patterns *list_of_packing_patterns;
	t_pb_graph_edge *expansion_edge;
    auto& device_ctx = g_vpr_ctx.device();

	/* alloc and initialize array of packing patterns based on architecture complex blocks */
	nhash = alloc_hash_table();
	ncount = 0;
	for (i = 0; i < device_ctx.num_block_types; i++) {
		discover_pattern_names_in_pb_graph_node(
				device_ctx.block_types[i].pb_graph_head, nhash, &ncount);
	}

	list_of_packing_patterns = alloc_and_init_pattern_list_from_hash(ncount, nhash);

	/* load packing patterns by traversing the edges to find edges belonging to pattern */
	for (i = 0; i < ncount; i++) {
		for (j = 0; j < device_ctx.num_block_types; j++) {
			expansion_edge = find_expansion_edge_of_pattern(i, device_ctx.block_types[j].pb_graph_head);
			if (expansion_edge == nullptr) {
				continue;
			}

			L_num_blocks = 0;
			list_of_packing_patterns[i].base_cost = 0;
			backward_expand_pack_pattern_from_edge(expansion_edge,
					list_of_packing_patterns, i, nullptr, nullptr, &L_num_blocks);
			list_of_packing_patterns[i].num_blocks = L_num_blocks;

            if (list_of_packing_patterns[i].is_chain) {
                auto output_nodes = find_end_of_path(expansion_edge->input_pins[0], i);
            }

			/* Default settings: A section of a netlist must match all blocks in a pack
             * pattern before it can be made a molecule except for carry-chains.
             * For carry-chains, since carry-chains are typically quite flexible in terms
             * of size, it is optional whether or not an atom in a netlist matches any
             * particular block inside the chain */
			list_of_packing_patterns[i].is_block_optional = (bool*) vtr::malloc(L_num_blocks * sizeof(bool));
			for(k = 0; k < L_num_blocks; k++) {
				list_of_packing_patterns[i].is_block_optional[k] = false;
				if(list_of_packing_patterns[i].is_chain && list_of_packing_patterns[i].root_block->block_id != k) {
					list_of_packing_patterns[i].is_block_optional[k] = true;
				}
			}
			break;
		}
	}

    //Sanity check, every pattern should have a root block
    for(i = 0; i < ncount; ++i) {
        if(list_of_packing_patterns[i].root_block == nullptr) {
            VPR_THROW(VPR_ERROR_ARCH, "Failed to find root block for pack pattern %s", list_of_packing_patterns[i].name);
        }
    }

	free_hash_table(nhash);

	*num_packing_patterns = ncount;


	return list_of_packing_patterns;
}

/**
 * Adds pack pattern name to hashtable of pack pattern names.
 */
static int add_pattern_name_to_hash(t_hash **nhash,
		const char *pattern_name, int *ncount) {
	t_hash *hash_value;

	hash_value = insert_in_hash_table(nhash, pattern_name, *ncount);
	if (hash_value->count == 1) {
		VTR_ASSERT(*ncount == hash_value->index);
		(*ncount)++;
	}
	return hash_value->index;
}

/**
 * Locate all pattern names
 * Side-effect: set all pb_graph_node temp_scratch_pad field to NULL
 *				For cases where a pattern inference is "obvious", mark it as obvious.
 */
static void discover_pattern_names_in_pb_graph_node(
		t_pb_graph_node *pb_graph_node, t_hash **nhash,
		int *ncount) {
	int i, j, k, m;
	int index;
	bool hasPattern;
	/* Iterate over all edges to discover if an edge in current physical block belongs to a pattern
	 If edge does, then record the name of the pattern in a hash table
	 */

	if (pb_graph_node == nullptr) {
		return;
	}

	pb_graph_node->temp_scratch_pad = nullptr;

	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
			hasPattern = false;
			for (k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					hasPattern = true;
					index = add_pattern_name_to_hash(nhash,
                                pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if (pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices =
                            (int*) vtr::malloc(pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns
                                                    * sizeof(int));
					}
					pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
			if (hasPattern == true) {
				forward_infer_pattern(&pb_graph_node->input_pins[i][j]);
				backward_infer_pattern(&pb_graph_node->input_pins[i][j]);
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			hasPattern = false;
			for (k = 0; k < pb_graph_node->output_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					hasPattern = true;
					index = add_pattern_name_to_hash(nhash,
							    pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if (pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices =
                            (int*) vtr::malloc(pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns
                                                * sizeof(int));
					}
					pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
			if (hasPattern == true) {
				forward_infer_pattern(&pb_graph_node->output_pins[i][j]);
				backward_infer_pattern(&pb_graph_node->output_pins[i][j]);
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			hasPattern = false;
			for (k = 0; k < pb_graph_node->clock_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					hasPattern = true;
					index = add_pattern_name_to_hash(nhash,
					            pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_names[m], ncount);
					if (pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices =
                            (int*) vtr::malloc(pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns
                                                * sizeof(int));
					}
					pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
				}
			}
			if (hasPattern == true) {
				forward_infer_pattern(&pb_graph_node->clock_pins[i][j]);
				backward_infer_pattern(&pb_graph_node->clock_pins[i][j]);
			}
		}
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				discover_pattern_names_in_pb_graph_node(
				    &pb_graph_node->child_pb_graph_nodes[i][j][k], nhash, ncount);
			}
		}
	}
}

/**
 * In obvious cases where a pattern edge has only one path to go, set that path to be inferred
 */
static void forward_infer_pattern(t_pb_graph_pin *pb_graph_pin) {
	if (pb_graph_pin->num_output_edges == 1 && pb_graph_pin->output_edges[0]->num_pack_patterns == 0 && pb_graph_pin->output_edges[0]->infer_pattern == false) {
		pb_graph_pin->output_edges[0]->infer_pattern = true;
		if (pb_graph_pin->output_edges[0]->num_output_pins == 1) {
			forward_infer_pattern(pb_graph_pin->output_edges[0]->output_pins[0]);
		}
	}
}
static void backward_infer_pattern(t_pb_graph_pin *pb_graph_pin) {
	if (pb_graph_pin->num_input_edges == 1 && pb_graph_pin->input_edges[0]->num_pack_patterns == 0 && pb_graph_pin->input_edges[0]->infer_pattern == false) {
		pb_graph_pin->input_edges[0]->infer_pattern = true;
		if (pb_graph_pin->input_edges[0]->num_input_pins == 1) {
			backward_infer_pattern(pb_graph_pin->input_edges[0]->input_pins[0]);
		}
	}
}

/**
 * Allocates memory for models and loads the name of the packing pattern
 * so that it can be identified and loaded with more complete information later
 */
static t_pack_patterns *alloc_and_init_pattern_list_from_hash(const int ncount,
		t_hash **nhash) {
	t_pack_patterns *nlist;
	t_hash_iterator hash_iter;
	t_hash *curr_pattern;

	nlist = (t_pack_patterns*)vtr::calloc(ncount, sizeof(t_pack_patterns));

	hash_iter = start_hash_table_iterator();
	curr_pattern = get_next_hash(nhash, &hash_iter);
	while (curr_pattern != nullptr) {
		VTR_ASSERT(nlist[curr_pattern->index].name == nullptr);
		nlist[curr_pattern->index].name = vtr::strdup(curr_pattern->name);
		nlist[curr_pattern->index].root_block = nullptr;
		nlist[curr_pattern->index].is_chain = false;
		nlist[curr_pattern->index].index = curr_pattern->index;
		curr_pattern = get_next_hash(nhash, &hash_iter);
	}
	return nlist;
}

void free_list_of_pack_patterns(t_pack_patterns *list_of_pack_patterns, const int num_packing_patterns) {
	int i, j, num_pack_pattern_blocks;
	t_pack_pattern_block **pattern_block_list;
	if (list_of_pack_patterns != nullptr) {
		for (i = 0; i < num_packing_patterns; i++) {
			num_pack_pattern_blocks = list_of_pack_patterns[i].num_blocks;
			pattern_block_list = (t_pack_pattern_block **)vtr::calloc(num_pack_pattern_blocks, sizeof(t_pack_pattern_block *));
			free(list_of_pack_patterns[i].name);
			free(list_of_pack_patterns[i].is_block_optional);
			free_pack_pattern(list_of_pack_patterns[i].root_block, pattern_block_list);
			for (j = 0; j < num_pack_pattern_blocks; j++) {
				free(pattern_block_list[j]);
			}
			free(pattern_block_list);
		}
		free(list_of_pack_patterns);
	}
}

/**
 * Locate first edge that belongs to pattern index
 */
static t_pb_graph_edge * find_expansion_edge_of_pattern(const int pattern_index,
		const t_pb_graph_node *pb_graph_node) {
	int i, j, k, m;
	t_pb_graph_edge * edge;
	/* Iterate over all edges to discover if an edge in current physical block belongs to a pattern
	 If edge does, then return that edge
	 */

	if (pb_graph_node == nullptr) {
		return nullptr;
	}

	for (i = 0; i < pb_graph_node->num_input_ports; i++) {
		for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            auto& input_pin = pb_graph_node->input_pins[i][j];
			for (k = 0; k < input_pin.num_output_edges; k++) {
				for (m = 0; m < input_pin.output_edges[k]->num_pack_patterns; m++) {
					if (input_pin.output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return input_pin.output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            auto& output_pin = pb_graph_node->output_pins[i][j];
			for (k = 0; k < output_pin.num_output_edges; k++) {
				for (m = 0; m < output_pin.output_edges[k]->num_pack_patterns; m++) {
					if (output_pin.output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return output_pin.output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            auto& clock_pin = pb_graph_node->clock_pins[i][j];
			for (k = 0; k < clock_pin.num_output_edges; k++) {
				for (m = 0; m < clock_pin.output_edges[k]->num_pack_patterns; m++) {
					if (clock_pin.output_edges[k]->pack_pattern_indices[m] == pattern_index) {
						return clock_pin.output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        auto& pb_mode = pb_graph_node->pb_type->modes[i];
		for (j = 0; j < pb_mode.num_pb_type_children; j++) {
			for (k = 0; k < pb_mode.pb_type_children[j].num_pb; k++) {
				edge = find_expansion_edge_of_pattern(
                       pattern_index, &pb_graph_node->child_pb_graph_nodes[i][j][k]);
				if (edge != nullptr) {
					return edge;
				}
			}
		}
	}
	return nullptr;
}

/**
 *  This function expands forward from the given expansion_edge. If a primtive is found that 
 *  belongs to the pack pattern we are searching for, create a pack pattern block of using
 *  this primtive to be added later to the pack pattern when creating the pack pattern 
 *  connections in the backward_expand_pack_pattern_from_edge function.
 *
 *  expansion_edge: starting edge to expand forward from 
 *  list_of_packing_patterns: list of packing patterns in the architecture
 *  curr_pattern_index: current packing pattern that we buildng
 *  L_num_blocks: number of primitives found to belong to this pattern so far
 *  make_root_of_chain: flag indicating that the given expansion_edge is connected
 *                      to a primitive that is the root of this packing pattern
 *
 *  Convention: Pack pattern block connections are made on backward expansion only (to make 
 *              future multi-fanout support easier) so this function will not update connections
 */
static void forward_expand_pack_pattern_from_edge(
		const t_pb_graph_edge* expansion_edge,
		t_pack_patterns *list_of_packing_patterns,
		const int curr_pattern_index, int *L_num_blocks, bool make_root_of_chain) {
	int i, j, k;
	int iport, ipin, iedge;
	bool found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *destination_block = nullptr;
	t_pb_graph_node *destination_pb_graph_node = nullptr;

	found = expansion_edge->infer_pattern;
    // if the pack pattern shouldn't be infered check if the expansion 
    // edge is annotated with the current pack pattern we are expanding
	for (i = 0;	!found && i < expansion_edge->num_pack_patterns; i++) {
		if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
			found = true;
		}
	}
    // if this edge isn't annoted with the current pack pattern
    // no need to explore it
	if (!found) {
		return;
	}

	found = false;
    // iterate over the expansion edge output pins
	for (i = 0; i < expansion_edge->num_output_pins; i++) {
        // check if expansion_edge parent node is a primitive (i.e num_nodes = 0)
		if (expansion_edge->output_pins[i]->parent_node->pb_type->num_modes == 0) {
			destination_pb_graph_node = expansion_edge->output_pins[i]->parent_node;
			VTR_ASSERT(found == false);
			/* Check assumption that each forced net has only one fan-out */
			/* This is the destination node */
			found = true;

            // the temp_scratch_pad points to the last primitive from this pb_graph_node that was added to a packing pattern. 
            const auto& destination_pb_temp = (t_pack_pattern_block*) destination_pb_graph_node->temp_scratch_pad;
            // if this pb_graph_node (primitive) is not added to the packing pattern already, add it and expand all its edges
			if (destination_pb_temp == nullptr || destination_pb_temp->pattern_index != curr_pattern_index) {
                // a primitive that belongs to this pack pattern is found: 1) create a new pattern block,
                // 2) assign an id to this pattern block, 3) increment the number of found blocks belonging to this 
                // pattern and 4) expand all its edges to find the other primitives that belong to this pattern
				destination_block = (t_pack_pattern_block*)vtr::calloc(1, sizeof(t_pack_pattern_block));
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(destination_pb_graph_node);
				destination_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
				destination_pb_graph_node->temp_scratch_pad = (void *) destination_block;
				destination_block->pattern_index = curr_pattern_index;
				destination_block->pb_type = destination_pb_graph_node->pb_type;

                // explore the inputs to this primitive 
				for (iport = 0; iport < destination_pb_graph_node->num_input_ports; iport++) {
					for (ipin = 0; ipin < destination_pb_graph_node->num_input_pins[iport]; ipin++) {
						for (iedge = 0; iedge < destination_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(
									destination_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index,
									&destination_pb_graph_node->input_pins[iport][ipin],
									destination_block, L_num_blocks);
						}
					}
				}

                // explore the outputs of this primtive
				for (iport = 0; iport < destination_pb_graph_node->num_output_ports; iport++) {
					for (ipin = 0; ipin < destination_pb_graph_node->num_output_pins[iport]; ipin++) {
						for (iedge = 0; iedge < destination_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(
									destination_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index, L_num_blocks, false);
						}
					}
				}

                // explore the clock pins of this primtivie
				for (iport = 0; iport < destination_pb_graph_node->num_clock_ports; iport++) {
					for (ipin = 0; ipin < destination_pb_graph_node->num_clock_pins[iport]; ipin++) {
						for (iedge = 0; iedge < destination_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(
									destination_pb_graph_node->clock_pins[iport][ipin].input_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index,
									&destination_pb_graph_node->clock_pins[iport][ipin],
									destination_block, L_num_blocks);
						}
					}
				}
			}

            // if this pb_graph_node (primitive) should be added to the pack pattern blocks
			if (((t_pack_pattern_block*) destination_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index) {
                // if this pb_graph_node is known to be the root of the chain, update the root block and root pin
				if(make_root_of_chain == true) {
					list_of_packing_patterns[curr_pattern_index].chain_root_pin = expansion_edge->output_pins[i];
					list_of_packing_patterns[curr_pattern_index].root_block = destination_block;
				}
			}

        // the expansion_edge parent node is not a primitive
		} else {
            // continue expanding forward
			for (j = 0; j < expansion_edge->output_pins[i]->num_output_edges; j++) {
				if (expansion_edge->output_pins[i]->output_edges[j]->infer_pattern == true) {
					forward_expand_pack_pattern_from_edge(
							expansion_edge->output_pins[i]->output_edges[j],
							list_of_packing_patterns, curr_pattern_index,
							L_num_blocks, make_root_of_chain);
				} else {
					for (k = 0; k < expansion_edge->output_pins[i]->output_edges[j]->num_pack_patterns; k++) {
						if (expansion_edge->output_pins[i]->output_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
							if (found == true) {
								/* Check assumption that each forced net has only one fan-out */
								vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
										"Invalid packing pattern defined.  Multi-fanout nets not supported when specifying pack patterns.\n"
										"Problem on %s[%d].%s[%d] for pattern %s\n",
										expansion_edge->output_pins[i]->parent_node->pb_type->name,
										expansion_edge->output_pins[i]->parent_node->placement_index,
										expansion_edge->output_pins[i]->port->name,
										expansion_edge->output_pins[i]->pin_number,
                                        list_of_packing_patterns[curr_pattern_index].name);
							}
							found = true;
							forward_expand_pack_pattern_from_edge(
									expansion_edge->output_pins[i]->output_edges[j],
									list_of_packing_patterns,
									curr_pattern_index, L_num_blocks, make_root_of_chain);
						} 
					}  // End for pack paterns of output edge 
				}
			} // End for number of output edges
		}
	} // End for output pins of expansion edge

}

/**
 * Find if driver of edge is in the same pattern, if yes, add to pattern
 *  Convention: Connections are made on backward expansion only (to make future multi-
 *               fanout support easier) so this function must update both source and
 *               destination blocks
 */
static void backward_expand_pack_pattern_from_edge(
		const t_pb_graph_edge* expansion_edge,
		t_pack_patterns *list_of_packing_patterns,
		const int curr_pattern_index, t_pb_graph_pin *destination_pin,
		t_pack_pattern_block *destination_block, int *L_num_blocks) {
	int i, j, k;
	int iport, ipin, iedge;
	bool found; /* Error checking, ensure only one fan-out for each pattern net */
	t_pack_pattern_block *source_block = nullptr;
	t_pb_graph_node *source_pb_graph_node = nullptr;
	t_pack_pattern_connections *pack_pattern_connection = nullptr;

	found = expansion_edge->infer_pattern;
    // if the pack pattern shouldn't be infered check if the expansion 
    // edge is annotated with the current pack pattern we are expanding
	for (i = 0;	!found && i < expansion_edge->num_pack_patterns; i++) {
		if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
			found = true;
		}
	}

    // if this edge isn't annoted with the current pack pattern
    // no need to explore it
	if (!found) {
		return;
	}

	found = false;
    // iterate over all the drivers of this edge
	for (i = 0; i < expansion_edge->num_input_pins; i++) {
        // check if the expansion_edge parent node is a primitive (i.e num_modes == 0)
		if (expansion_edge->input_pins[i]->parent_node->pb_type->num_modes == 0) {
			source_pb_graph_node = expansion_edge->input_pins[i]->parent_node;
			VTR_ASSERT(found == false);
			/* Check assumption that each forced net has only one fan-out */
			/* This is the source node for destination */
			found = true;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			source_block = (t_pack_pattern_block*) source_pb_graph_node->temp_scratch_pad;
			if (source_block == nullptr || source_block->pattern_index != curr_pattern_index) {
				source_block = (t_pack_pattern_block *)vtr::calloc(1, sizeof(t_pack_pattern_block));
				source_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(source_pb_graph_node);
				source_pb_graph_node->temp_scratch_pad = (void *) source_block;
				source_block->pattern_index = curr_pattern_index;
				source_block->pb_type = source_pb_graph_node->pb_type;

				if (list_of_packing_patterns[curr_pattern_index].root_block == nullptr) {
					list_of_packing_patterns[curr_pattern_index].root_block = source_block;
				}

                // explore the inputs of this primitive
				for (iport = 0; iport < source_pb_graph_node->num_input_ports; iport++) {
					for (ipin = 0; ipin < source_pb_graph_node->num_input_pins[iport]; ipin++) {
						for (iedge = 0; iedge < source_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(
									source_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index,
									&source_pb_graph_node->input_pins[iport][ipin],
									source_block, L_num_blocks);
						}
					}
				}

                // explore the outputs of this primitive
				for (iport = 0; iport < source_pb_graph_node->num_output_ports; iport++) {
					for (ipin = 0; ipin < source_pb_graph_node->num_output_pins[iport]; ipin++) {
						for (iedge = 0; iedge < source_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
							forward_expand_pack_pattern_from_edge(
									source_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index, L_num_blocks, false);
						}
					}
				}

                // explore the clock pins of thie primitive
				for (iport = 0; iport < source_pb_graph_node->num_clock_ports; iport++) {
					for (ipin = 0; ipin < source_pb_graph_node->num_clock_pins[iport]; ipin++) {
						for (iedge = 0; iedge < source_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
							backward_expand_pack_pattern_from_edge(
									source_pb_graph_node->clock_pins[iport][ipin].input_edges[iedge],
									list_of_packing_patterns,
									curr_pattern_index,
									&source_pb_graph_node->clock_pins[iport][ipin],
									source_block, L_num_blocks);
						}
					}
				}
			}

			if (destination_pin != nullptr) {
				VTR_ASSERT(((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index);
				source_block = (t_pack_pattern_block*) source_pb_graph_node->temp_scratch_pad;
				pack_pattern_connection = (t_pack_pattern_connections *) vtr::calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;
				pack_pattern_connection->next = source_block->connections;
				source_block->connections = pack_pattern_connection;

				pack_pattern_connection = (t_pack_pattern_connections *)vtr::calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;
				pack_pattern_connection->next = destination_block->connections;
				destination_block->connections = pack_pattern_connection;

				if (source_block == destination_block) {
					vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
							"Invalid packing pattern defined. Source and destination block are the same (%s).\n",
							source_block->pb_type->name);
				}
			}

        // expansion edge parent is not a primitive
		} else {
            // check if this input pin of the expansion edge has no driving pin
			if(expansion_edge->input_pins[i]->num_input_edges == 0) {
                // check if this input pin of the expansion edge belongs to a root block (i.e doesn't have a parent block)
				if(expansion_edge->input_pins[i]->parent_node->pb_type->parent_mode == nullptr) {
					// This pack pattern extends to CLB (root pb block) input pin, 
                    // thus it extends across multiple logic blocks, treat as a chain 
					list_of_packing_patterns[curr_pattern_index].is_chain = true;
                    // since this input pin has not driving nets, expand in the forward direction instead
					forward_expand_pack_pattern_from_edge(
									expansion_edge,
									list_of_packing_patterns,
									curr_pattern_index, L_num_blocks, true);
				}
            // this input pin of the expansion edge has a driving pin
			} else {
                // iterate over all the driving edges of this input pin
				for (j = 0; j < expansion_edge->input_pins[i]->num_input_edges; j++) {
                    // if pattern should be infered for this edge continue the expansion backwards
					if (expansion_edge->input_pins[i]->input_edges[j]->infer_pattern == true) {
						backward_expand_pack_pattern_from_edge(
								expansion_edge->input_pins[i]->input_edges[j],
								list_of_packing_patterns, curr_pattern_index,
								destination_pin, destination_block, L_num_blocks);
                    // if pattern shouldn't be infered
					} else {
                        // check if this input pin edge is annotated with the current pattern
						for (k = 0; k < expansion_edge->input_pins[i]->input_edges[j]->num_pack_patterns; k++) {
							if (expansion_edge->input_pins[i]->input_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
								VTR_ASSERT(found == false);
								/* Check assumption that each forced net has only one fan-out */
								found = true;
								backward_expand_pack_pattern_from_edge(
										expansion_edge->input_pins[i]->input_edges[j],
										list_of_packing_patterns,
										curr_pattern_index, destination_pin,
										destination_block, L_num_blocks);
							}
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
 * 3.  Chained molecules are molecules that follow a carry-chain style pattern,
 *     ie. a single linear chain that can be split across multiple complex blocks
 */
t_pack_molecule *alloc_and_load_pack_molecules(
		t_pack_patterns *list_of_pack_patterns,
        std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
        std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
		const int num_packing_patterns) {
	int i, j, best_pattern;
	t_pack_molecule *list_of_molecules_head;
	t_pack_molecule *cur_molecule;
	bool *is_used;
    auto& atom_ctx = g_vpr_ctx.atom();

	is_used = (bool*)vtr::calloc(num_packing_patterns, sizeof(bool));

	cur_molecule = list_of_molecules_head = nullptr;

	/* Find forced pack patterns
	 * Simplifying assumptions: Each atom can map to at most one molecule,
     *                          use first-fit mapping based on priority of pattern
	 * TODO: Need to investigate better mapping strategies than first-fit
     */
	for (i = 0; i < num_packing_patterns; i++) {
		best_pattern = 0;
		for(j = 1; j < num_packing_patterns; j++) {
			if(is_used[best_pattern]) {
				best_pattern = j;
			} else if (is_used[j] == false && compare_pack_pattern(&list_of_pack_patterns[j], &list_of_pack_patterns[best_pattern]) == 1) {
				best_pattern = j;
			}
		}
		VTR_ASSERT(is_used[best_pattern] == false);
		is_used[best_pattern] = true;

        auto blocks = atom_ctx.nlist.blocks();
        for(auto blk_iter = blocks.begin(); blk_iter != blocks.end(); ++blk_iter) {
            auto blk_id = *blk_iter;

            cur_molecule = try_create_molecule(list_of_pack_patterns, atom_molecules, best_pattern, blk_id);
            if (cur_molecule != nullptr) {
                cur_molecule->next = list_of_molecules_head;
                /* In the event of multiple molecules with the same atom block pattern,
                 * bias to use the molecule with less costly physical resources first */
                /* TODO: Need to normalize magical number 100 */
                cur_molecule->base_gain = cur_molecule->num_blocks - (cur_molecule->pack_pattern->base_cost / 100);
                list_of_molecules_head = cur_molecule;

                //Note: atom_molecules is an (ordered) multimap so the last molecule
                //      inserted for a given blk_id will be the last valid element
                //      in the equal_range
                auto rng = atom_molecules.equal_range(blk_id); //The range of molecules matching this block
                bool range_empty = (rng.first == rng.second);
                bool cur_was_last_inserted = false;
                if (!range_empty) {
                    auto last_valid_iter = --rng.second; //Iterator to last element (only valid if range is not empty)
                    cur_was_last_inserted = (last_valid_iter->second == cur_molecule);
                }
                if(range_empty || !cur_was_last_inserted) {
                    /* molecule did not cover current atom (possibly because molecule created is
                     * part of a long chain that extends past multiple logic blocks), try again */
                    --blk_iter;
                }
            }
		}
	}
	free(is_used);

	/* List all atom blocks as a molecule for blocks that do not belong to any molecules.
	 This allows the packer to be consistent as it now packs molecules only instead of atoms and molecules

	 If a block belongs to a molecule, then carrying the single atoms around can make the packing problem
	 more difficult because now it needs to consider splitting molecules.
	 */
    for(auto blk_id : atom_ctx.nlist.blocks()) {

        expected_lowest_cost_pb_gnode[blk_id] = get_expected_lowest_cost_primitive_for_atom_block(blk_id);

        auto rng = atom_molecules.equal_range(blk_id);
        bool rng_empty = (rng.first == rng.second);
		if (rng_empty) {
			cur_molecule = new t_pack_molecule;
			cur_molecule->valid = true;
			cur_molecule->type = MOLECULE_SINGLE_ATOM;
			cur_molecule->num_blocks = 1;
			cur_molecule->root = 0;
			cur_molecule->pack_pattern = nullptr;

            cur_molecule->atom_block_ids = {blk_id};

			cur_molecule->next = list_of_molecules_head;
			cur_molecule->base_gain = 1;
			list_of_molecules_head = cur_molecule;

            atom_molecules.insert({blk_id, cur_molecule});
		}
	}

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS)) {
		print_pack_molecules(getEchoFileName(E_ECHO_PRE_PACKING_MOLECULES_AND_PATTERNS),
				list_of_pack_patterns, num_packing_patterns,
				list_of_molecules_head);
	}

	return list_of_molecules_head;
}


static void free_pack_pattern(t_pack_pattern_block *pattern_block, t_pack_pattern_block **pattern_block_list) {
	t_pack_pattern_connections *connection, *next;
	if (pattern_block == nullptr || pattern_block->block_id == OPEN) {
		/* already traversed, return */
		return;
	}
	pattern_block_list[pattern_block->block_id] = pattern_block;
	pattern_block->block_id = OPEN;
	connection = pattern_block->connections;
	while (connection) {
		free_pack_pattern(connection->from_block, pattern_block_list);
		free_pack_pattern(connection->to_block, pattern_block_list);
		next = connection->next;
		free(connection);
		connection = next;
	}
}

/**
 * Given a pattern and an atom block to serve as the root block, determine if
 * the candidate atom block serving as the root node matches the pattern.
 * If yes, return the molecule with this atom block as the root, if not, return NULL
 *
 * Limitations: Currently assumes that forced pack nets must be single-fanout as
 *              this covers all the reasonable architectures we wanted. More complicated
 *              structures should probably be handled either downstream (general packing)
 *              or upstream (in tech mapping).
 *              If this limitation is too constraining, code is designed so that this limitation can be removed
 *
 * Side Effect: If successful, link atom to molecule
 */
static t_pack_molecule *try_create_molecule(
		t_pack_patterns *list_of_pack_patterns,
        std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
        const int pack_pattern_index,
		AtomBlockId blk_id) {
	int i;
	t_pack_molecule *molecule;

	bool failed = false;

	{
		molecule = new t_pack_molecule;
		molecule->valid = true;
		molecule->type = MOLECULE_FORCED_PACK;
		molecule->pack_pattern = &list_of_pack_patterns[pack_pattern_index];
		if (molecule->pack_pattern == nullptr) {failed = true; goto end_prolog;}

        molecule->atom_block_ids = std::vector<AtomBlockId>(molecule->pack_pattern->num_blocks); //Initializes invalid

		molecule->num_blocks = list_of_pack_patterns[pack_pattern_index].num_blocks;
		if (molecule->num_blocks == 0) {failed = true; goto end_prolog;}

		if (list_of_pack_patterns[pack_pattern_index].root_block == nullptr) {failed = true; goto end_prolog;}
		molecule->root = list_of_pack_patterns[pack_pattern_index].root_block->block_id;

		if(list_of_pack_patterns[pack_pattern_index].is_chain == true) {
			/* A chain pattern extends beyond a single logic block so we must find
             * the blk_id that matches with the portion of a chain for this particular logic block */
			blk_id = find_new_root_atom_for_chain(blk_id, &list_of_pack_patterns[pack_pattern_index], atom_molecules);
		}
	}

	end_prolog:

	if (!failed && blk_id && try_expand_molecule(molecule, atom_molecules, blk_id,
			molecule->pack_pattern->root_block) == true) {
		/* Success! commit module */
		for (i = 0; i < molecule->pack_pattern->num_blocks; i++) {
            auto blk_id2 = molecule->atom_block_ids[i];
			if(!blk_id2) {
				VTR_ASSERT(list_of_pack_patterns[pack_pattern_index].is_block_optional[i] == true);
				continue;
			}

            atom_molecules.insert({blk_id2, molecule});
		}
	} else {
		failed = true;
	}

	if (failed == true) {
		/* Does not match pattern, free molecule */
		delete molecule;
		molecule = nullptr;
	}
	return molecule;
}

/**
 * Determine if atom block can match with the pattern to form a molecule
 * return true if it matches, return false otherwise
 */
static bool try_expand_molecule(t_pack_molecule *molecule,
        const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
		const AtomBlockId blk_id,
		const t_pack_pattern_block *current_pattern_block) {
	int ipin;
	bool success;
	bool is_optional;
	bool *is_block_optional;
	t_pack_pattern_connections *cur_pack_pattern_connection;

    auto& atom_ctx = g_vpr_ctx.atom();

	is_block_optional = molecule->pack_pattern->is_block_optional;
	is_optional = is_block_optional[current_pattern_block->block_id];

    /* If the block in the pattern has already been visited, then there is no need to revisit it */
	if (molecule->atom_block_ids[current_pattern_block->block_id]) {
		if (molecule->atom_block_ids[current_pattern_block->block_id] != blk_id) {
			/* Mismatch between the visited block and the current block implies
             * that the current netlist structure does not match the expected pattern,
             * return whether or not this matters */
			return is_optional;
		} else {
			return true;
		}
	}

	/* This node has never been visited */
	/* Simplifying assumption: An atom can only map to one molecule */
    auto rng = atom_molecules.equal_range(blk_id);
    bool rng_empty = rng.first == rng.second;
	if(!rng_empty) {
		/* This block is already in a molecule, return whether or not this matters */
		return is_optional;
	}

	if (primitive_type_feasible(blk_id, current_pattern_block->pb_type)) {

		success = true;
		/* If the primitive types match, store it, expand it and explore neighbouring nodes */


        /* store that this node has been visited */
		molecule->atom_block_ids[current_pattern_block->block_id] = blk_id;

		cur_pack_pattern_connection = current_pattern_block->connections;
		while (cur_pack_pattern_connection != nullptr && success == true) {
			if (cur_pack_pattern_connection->from_block == current_pattern_block) {
				/* find net corresponding to pattern */
                AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, cur_pack_pattern_connection->from_pin->port->model_port);
                if(!port_id) {
                    //No matching port, we may be at the end
                    success = is_block_optional[cur_pack_pattern_connection->to_block->block_id];
                } else {
                    ipin = cur_pack_pattern_connection->from_pin->pin_number;
                    AtomNetId net_id = atom_ctx.nlist.port_net(port_id, ipin);

                    /* Check if net is valid */
                    if (!net_id || atom_ctx.nlist.net_sinks(net_id).size() != 1) { /* One fanout assumption */
                        success = is_block_optional[cur_pack_pattern_connection->to_block->block_id];
                    } else {
                        auto net_sinks = atom_ctx.nlist.net_sinks(net_id);
                        VTR_ASSERT(net_sinks.size() == 1);

                        auto sink_pin_id = *net_sinks.begin();
                        auto sink_blk_id = atom_ctx.nlist.pin_block(sink_pin_id);

                        success = try_expand_molecule(molecule, atom_molecules, sink_blk_id,
                                    cur_pack_pattern_connection->to_block);
                    }
                }
			} else {
				VTR_ASSERT(cur_pack_pattern_connection->to_block == current_pattern_block);
				/* find net corresponding to pattern */

                auto port_id = atom_ctx.nlist.find_atom_port(blk_id, cur_pack_pattern_connection->to_pin->port->model_port);
                VTR_ASSERT(port_id);
				ipin = cur_pack_pattern_connection->to_pin->pin_number;

                AtomNetId net_id;
				if (cur_pack_pattern_connection->to_pin->port->model_port->is_clock) {
                    VTR_ASSERT(ipin == 1); //TODO: support multi-clock primitives
					net_id = atom_ctx.nlist.port_net(port_id, ipin);
				} else {
					net_id = atom_ctx.nlist.port_net(port_id, ipin);
				}
				/* Check if net is valid */
				if (!net_id || atom_ctx.nlist.net_sinks(net_id).size() != 1) { /* One fanout assumption */
					success = is_block_optional[cur_pack_pattern_connection->from_block->block_id];
				} else {
                    auto driver_blk_id = atom_ctx.nlist.net_driver_block(net_id);
					success = try_expand_molecule(molecule, atom_molecules, driver_blk_id,
                                cur_pack_pattern_connection->from_block);
				}
			}
			cur_pack_pattern_connection = cur_pack_pattern_connection->next;
		}
	} else {
		success = is_optional;
	}

	return success;
}

static void print_pack_molecules(const char *fname,
		const t_pack_patterns *list_of_pack_patterns, const int num_pack_patterns,
		const t_pack_molecule *list_of_molecules) {
	int i;
	FILE *fp;
	const t_pack_molecule *list_of_molecules_current;
    auto& atom_ctx = g_vpr_ctx.atom();

	fp = std::fopen(fname, "w");
	fprintf(fp, "# of pack patterns %d\n", num_pack_patterns);

	for (i = 0; i < num_pack_patterns; i++) {
        VTR_ASSERT(list_of_pack_patterns[i].root_block);
		fprintf(fp, "pack pattern index %d block count %d name %s root %s\n",
				list_of_pack_patterns[i].index,
				list_of_pack_patterns[i].num_blocks,
				list_of_pack_patterns[i].name,
				list_of_pack_patterns[i].root_block->pb_type->name);
	}

	list_of_molecules_current = list_of_molecules;
	while (list_of_molecules_current != nullptr) {
		if (list_of_molecules_current->type == MOLECULE_SINGLE_ATOM) {
			fprintf(fp, "\nmolecule type: atom\n");
			fprintf(fp, "\tpattern index %d: atom block %s\n", i,
					atom_ctx.nlist.block_name(list_of_molecules_current->atom_block_ids[0]).c_str());
		} else if (list_of_molecules_current->type == MOLECULE_FORCED_PACK) {
			fprintf(fp, "\nmolecule type: %s\n",
					list_of_molecules_current->pack_pattern->name);
			for (i = 0; i < list_of_molecules_current->pack_pattern->num_blocks;
					i++) {
				if(!list_of_molecules_current->atom_block_ids[i]) {
					fprintf(fp, "\tpattern index %d: empty \n",	i);
				} else {
					fprintf(fp, "\tpattern index %d: atom block %s",
						i,
						atom_ctx.nlist.block_name(list_of_molecules_current->atom_block_ids[i]).c_str());
					if(list_of_molecules_current->pack_pattern->root_block->block_id == i) {
						fprintf(fp, " root node\n");
					} else {
						fprintf(fp, "\n");
					}
				}
			}
		} else {
			VTR_ASSERT(0);
		}
		list_of_molecules_current = list_of_molecules_current->next;
	}

	fclose(fp);
}

/* Search through all primitives and return the lowest cost primitive that fits this atom block */
static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block(const AtomBlockId blk_id) {
	int i;
	float cost, best_cost;
	t_pb_graph_node *current, *best;
    auto& device_ctx = g_vpr_ctx.device();

	best_cost = UNDEFINED;
	best = nullptr;
	current = nullptr;
	for(i = 0; i < device_ctx.num_block_types; i++) {
		cost = UNDEFINED;
		current = get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(blk_id, device_ctx.block_types[i].pb_graph_head, &cost);
		if(cost != UNDEFINED) {
			if(best_cost == UNDEFINED || best_cost > cost) {
				best_cost = cost;
				best = current;
			}
		}
	}

    if(!best) {
        auto& atom_ctx = g_vpr_ctx.atom();
        VPR_THROW(VPR_ERROR_PACK, "Failed to find any location to pack primitive of type '%s' in architecture",
                  atom_ctx.nlist.block_model(blk_id)->name);
    }

	return best;
}

static t_pb_graph_node *get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(const AtomBlockId blk_id, t_pb_graph_node *curr_pb_graph_node, float *cost) {
	t_pb_graph_node *best, *cur;
	float cur_cost, best_cost;
	int i, j;

	best = nullptr;
	best_cost = UNDEFINED;
	if(curr_pb_graph_node == nullptr) {
		return nullptr;
	}

	if(curr_pb_graph_node->pb_type->blif_model != nullptr) {
		if(primitive_type_feasible(blk_id, curr_pb_graph_node->pb_type)) {
			cur_cost = compute_primitive_base_cost(curr_pb_graph_node);
			if(best_cost == UNDEFINED || best_cost > cur_cost) {
				best_cost = cur_cost;
				best = curr_pb_graph_node;
			}
		}
	} else {
		for(i = 0; i < curr_pb_graph_node->pb_type->num_modes; i++) {
			for(j = 0; j < curr_pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
				*cost = UNDEFINED;
				cur = get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(blk_id, &curr_pb_graph_node->child_pb_graph_nodes[i][j][0], cost);
				if(cur != nullptr) {
					if(best == nullptr || best_cost > *cost) {
						best = cur;
						best_cost = *cost;
					}
				}
			}
		}
	}

	*cost = best_cost;
	return best;
}


/* Determine which of two pack pattern should take priority */
static int compare_pack_pattern(const t_pack_patterns *pattern_a, const t_pack_patterns *pattern_b) {
	float base_gain_a, base_gain_b, diff;

	/* Bigger patterns should take higher priority than smaller patterns because they are harder to fit */
	if (pattern_a->num_blocks > pattern_b->num_blocks) {
		return 1;
	} else if (pattern_a->num_blocks < pattern_b->num_blocks) {
		return -1;
	}

	base_gain_a = pattern_a->base_cost;
	base_gain_b = pattern_b->base_cost;
	diff = base_gain_a - base_gain_b;

	/* Less costly patterns should be used before more costly patterns */
	if (diff < 0) {
		return 1;
	}
	if (diff > 0) {
		return -1;
	}
	return 0;
}

/* A chain can extend across multiple atom blocks.  Must segment the chain to fit in an atom
 * block by identifying the actual atom that forms the root of the new chain.
 * Returns AtomBlockId::INVALID() if this block_index doesn't match up with any chain
 *
 * Assumes that the root of a chain is the primitive that starts the chain or is driven from outside the logic block
 * block_index: index of current atom
 * list_of_pack_pattern: ptr to current chain pattern
 */
static AtomBlockId find_new_root_atom_for_chain(const AtomBlockId blk_id, const t_pack_patterns *list_of_pack_pattern,
        const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules) {
    AtomBlockId new_root_blk_id;
	t_pb_graph_pin *root_ipin;
	t_pb_graph_node *root_pb_graph_node;
	t_model_ports *model_port;

    auto& atom_ctx = g_vpr_ctx.atom();

	VTR_ASSERT(list_of_pack_pattern->is_chain == true);
	root_ipin = list_of_pack_pattern->chain_root_pin;
	root_pb_graph_node = root_ipin->parent_node;

	if(primitive_type_feasible(blk_id, root_pb_graph_node->pb_type) == false) {
		return AtomBlockId::INVALID();
	}

	/* Assign driver furthest up the chain that matches the root node and is unassigned to a molecule as the root */
	model_port = root_ipin->port->model_port;

    AtomPortId port_id = atom_ctx.nlist.find_atom_port(blk_id, model_port);
    if(!port_id) {
        //There is no port with the chain connection on this block, it must be the furthest
        //up the chain, so return it as root
        return blk_id;
    }

	AtomNetId driving_net_id = atom_ctx.nlist.port_net(port_id, root_ipin->pin_number);
	if(!driving_net_id) {
        //There is no net associated with the chain connection on this block, it must be the furthest
        //up the chain, so return it as root
		return blk_id;
	}

    auto driver_pin_id = atom_ctx.nlist.net_driver(driving_net_id);
	AtomBlockId driver_blk_id = atom_ctx.nlist.pin_block(driver_pin_id);

    auto rng = atom_molecules.equal_range(driver_blk_id);
    bool rng_empty = (rng.first == rng.second);
	if(!rng_empty) {
		/* Driver is used/invalid, so current block is the furthest up the chain, return it */
		return blk_id;
	}

	new_root_blk_id = find_new_root_atom_for_chain(driver_blk_id, list_of_pack_pattern, atom_molecules);
	if(!new_root_blk_id) {
		return blk_id;
	} else {
		return new_root_blk_id;
	}
}

/**
 * This function takes an input pin to a root (has no parent block) pb_graph_node
 * an returns a vector of all the output pins that are reachable from this input
 * pin and have the same packing pattern
 */

std::vector<t_pb_graph_pin *> find_end_of_path(t_pb_graph_pin* input_pin, int pattern_index) {

    VTR_LOG("Trying to find end of path starting at: %s\n", input_pin->pin_to_string().c_str());

    // Enforce some constraints on the function

    // 1) the start of the path should be at the input of the root block
    VTR_ASSERT(input_pin->is_root_block_pin());

    // 2) this pin is an input pin to the root block
    VTR_ASSERT(input_pin->num_input_edges == 0);

    // create a queue of pin pointers for the breadth first search
    std::queue<t_pb_graph_pin *> pins_queue;

    // add the input pin to the queue
    pins_queue.push(input_pin);

    // found reachable output pins
    std::vector<t_pb_graph_pin *> reachable_pins;

    // do breadth first search till all connected
    // pins are explored
    while(!pins_queue.empty()) {

       // get the first pin in the queue
       auto current_pin = pins_queue.front();

       // remove pin from queue
       pins_queue.pop();

       // expand search from current pin
       expand_search(current_pin, pins_queue, pattern_index);

       // if this is an output pin of a root block
       // add to reachable output pins
       if (current_pin->is_root_block_pin()
           && current_pin->num_output_edges == 0) {
           reachable_pins.push_back(current_pin);
       }
    }

    VTR_LOG("Found the following output pins:\n");
    // some checks on the output of the search algorithm
    for(const auto& pin_ptr: reachable_pins) {
        VTR_LOG("%s\n", pin_ptr->pin_to_string().c_str());
        // the found pins should belong to the root block
        VTR_ASSERT(pin_ptr->is_root_block_pin());
        // and should be an output pin
        VTR_ASSERT(pin_ptr->num_output_edges == 0);
    }

    VTR_LOG("\n\n");

    return reachable_pins;
}



void expand_search(t_pb_graph_pin * input_pin, std::queue<t_pb_graph_pin *>& pins_queue, int pattern_index) {

    // If not a primitive input pin
    // ----------------------------

    // iterate over all output edges at this pin
    for(int iedge = 0; iedge < input_pin->num_output_edges; iedge++) {

        const auto& pin_edge = input_pin->output_edges[iedge];
        // if we cannot infer the pattern of this edge and its pattern
        // doesn't match the pattern_index, ignore this edge
        if (pin_edge->infer_pattern == false
            && pin_edge->annotated_with_pattern(pattern_index) == false) {
            continue;
        }

        // this edge either matched the pack pattern or its pack pattern could be infered

        // iterate over all the pins of that edge and add them to the pins_queue
        for (int ipin = 0; ipin < pin_edge->num_output_pins; ipin++) {
             pins_queue.push(pin_edge->output_pins[ipin]);
        }

    } // End for pin edges


    // If a primitive input pin
    // ------------------------

    // if this is an input pin to a primitive, it won't have output edges
    // so the previuos for loop won't be entered
    if (input_pin->is_primitive_pin() && input_pin->num_output_edges == 0) {
        // iterate over the output ports of the primitive
        const auto& pin_pb_graph_node = input_pin->parent_node;
        for (int iport = 0; iport < pin_pb_graph_node->num_output_ports; iport++) {
             // iterate over the pins of each port
             const auto& port_pins = pin_pb_graph_node->num_output_pins[iport];
             for(int ipin = 0; ipin < port_pins; ipin++) {
                 // add primtive output pins to pins_queue to be explored
                 pins_queue.push(&pin_pb_graph_node->output_pins[iport][ipin]);
             }
        }
    }

    // If this is a root block output pin
    // ----------------------------------

    // No expansion will happen in this case
}
