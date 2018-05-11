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
#include <fstream>
#include <queue>
#include <stack>
#include <map>
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
#include "pb_graph_walker.h"
#include "molecule_dfa.h"

/*
 * Local Data Types
 */

struct t_pack_pattern_connection {
    t_pb_graph_pin* from_pin;
    t_pb_graph_pin* to_pin;
    std::string pattern_name;
};

struct Match {
    struct Edge {
        Edge(AtomPinId from, AtomPinId to, int edge_id, int sink_idx)
            : from_pin(from)
            , to_pin(to)
            , pattern_edge_id(edge_id)
            , pattern_edge_sink(sink_idx) {}

        AtomPinId from_pin;
        AtomPinId to_pin;
        int pattern_edge_id = OPEN; //Pattern edge which matched these pins
        int pattern_edge_sink = OPEN; //Matching sink number within pattern edge
    };

    //Evaluates true when the match is non-empty
    operator bool() {
        return netlist_edges.size() > 0;
    }

    std::vector<Edge> netlist_edges;
};

/*****************************************/
/*Local Function Declaration			 */
/*****************************************/
static int add_pattern_name_to_hash(t_hash **nhash,
		const char *pattern_name, int *ncount);
static void discover_pattern_names_in_pb_graph_node(
		t_pb_graph_node *pb_graph_node, t_hash **nhash,
		int *ncount, int depth);
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
static void print_pack_pattern(FILE* fp, 
                               const t_pack_patterns& pack_pattern);
static void print_pack_pattern_block(FILE* fp, 
                               const t_pack_pattern_block* pack_pattern_block,
                               int depth);
static void print_pack_pattern_connection(FILE* fp, 
                               const t_pack_pattern_connections* conn,
                               int depth);
static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block(const AtomBlockId blk_id);
static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(const AtomBlockId blk_id, t_pb_graph_node *curr_pb_graph_node, float *cost);
static AtomBlockId find_new_root_atom_for_chain(const AtomBlockId blk_id, const t_pack_patterns *list_of_pack_pattern, 
        const std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules);

static void init_pack_patterns();
static std::map<std::string,std::vector<t_pack_pattern_connection>> collect_type_pack_pattern_connections(t_type_ptr type);
static t_pack_pattern build_pack_pattern(std::string pattern_name, const std::vector<t_pack_pattern_connection>& connections);
static std::vector<t_netlist_pack_pattern> abstract_pack_patterns(const std::vector<t_pack_pattern>& arch_pack_patterns);
static t_netlist_pack_pattern abstract_pack_pattern(const t_pack_pattern& arch_pack_pattern);

static std::vector<Match> collect_pattern_matches_in_netlist(const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static std::set<AtomBlockId> collect_internal_blocks_in_match(const Match& match, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);

static Match match_largest(AtomBlockId blk, const t_netlist_pack_pattern& pattern, const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);
static bool match_largest_recur(Match& match, const AtomBlockId blk, 
                                int pattern_node_id, const t_netlist_pack_pattern& pattern,
                                const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist);
static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist);
static AtomBlockId find_parent_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static bool matches_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static bool matches_pattern_node(const AtomBlockId blk, const t_netlist_pack_pattern_node& pattern_node, const AtomNetlist& netlist);
static bool is_wildcard_node(const t_netlist_pack_pattern_node& pattern_node);
static bool is_wildcard_pin(const t_netlist_pack_pattern_pin& pattern_pin);
static bool matches_pattern_pin(const AtomPinId from_pin, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist);
static void print_match(const Match& match, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist);
static void write_netlist_pack_pattern_dot(std::ostream& os, const t_netlist_pack_pattern& netlist_pattern);
static void write_arch_pack_pattern_dot(std::ostream& os, const t_pack_pattern& arch_pattern);

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

    init_pack_patterns();

	/* alloc and initialize array of packing patterns based on architecture complex blocks */
	nhash = alloc_hash_table();
	ncount = 0;
	for (i = 0; i < device_ctx.num_block_types; i++) {
        //vtr::printf("Looking for pack patterns within %s\n", device_ctx.block_types[i].name);
		discover_pattern_names_in_pb_graph_node(
				device_ctx.block_types[i].pb_graph_head, nhash, &ncount, 1);
	}

	list_of_packing_patterns = alloc_and_init_pattern_list_from_hash(ncount, nhash);

	/* load packing patterns by traversing the edges to find edges belonging to pattern */
	for (i = 0; i < ncount; i++) {
		for (j = 0; j < device_ctx.num_block_types; j++) {
            //vtr::printf("Creating pack pattern %d for %s\n", i, device_ctx.block_types[j].name);
			expansion_edge = find_expansion_edge_of_pattern(i, device_ctx.block_types[j].pb_graph_head);
			if (expansion_edge == nullptr) {
				continue;
			}
			L_num_blocks = 0;
			list_of_packing_patterns[i].base_cost = 0;
			backward_expand_pack_pattern_from_edge(expansion_edge,
					list_of_packing_patterns, i, nullptr, nullptr, &L_num_blocks);
			list_of_packing_patterns[i].num_blocks = L_num_blocks;

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
		int *ncount, int depth) {
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
									pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_names[m],
									ncount);
					if (pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices = (int*)
								vtr::malloc( pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
                    //vtr::printf("%sFound pack pattern '%s' (index %d) to input pin %s[%d].%s[%d]\n", 
                                //vtr::indent(depth, "  ").c_str(),
                                //pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_names[m],
                                //index,
                                //pb_graph_node->pb_type->name,
                                //pb_graph_node->placement_index,
                                //pb_graph_node->input_pins[i][j].port->name,
                                //pb_graph_node->input_pins[i][j].pin_number
                                //);
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
									pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_names[m],
									ncount);
					if (pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices = (int*)
								vtr::malloc( pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
                    //vtr::printf("%sFound pack pattern '%s' (index %d) from output pin %s[%d].%s[%d]\n", 
                                //vtr::indent(depth, "  ").c_str(),
                                //pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_names[m],
                                //index,
                                //pb_graph_node->pb_type->name,
                                //pb_graph_node->placement_index,
                                //pb_graph_node->output_pins[i][j].port->name,
                                //pb_graph_node->output_pins[i][j].pin_number
                                //);
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
									pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_names[m],
									ncount);
					if (pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices == nullptr) {
						pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices = (int*)
								vtr::malloc( pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns * sizeof(int));
					}
					pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices[m] = index;
                    //vtr::printf("%sFound pack pattern '%s' (index %d) to clock pin %s[%d].%s[%d]\n", 
                                //vtr::indent(depth, "  ").c_str(),
                                //pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_names[m],
                                //index,
                                //pb_graph_node->pb_type->name,
                                //pb_graph_node->placement_index,
                                //pb_graph_node->clock_pins[i][j].port->name,
                                //pb_graph_node->clock_pins[i][j].pin_number
                                //);
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
						&pb_graph_node->child_pb_graph_nodes[i][j][k], nhash,
						ncount, depth + 1);
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
        //vtr::printf("Allocating Pack Pattern %d: %s\n", curr_pattern->index, curr_pattern->name);

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
			for (k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->input_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if (pb_graph_node->input_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
                        //vtr::printf("Found expansion edge on input pin %s.%s[%d] edge %d pattern %d\n",
                                    //pb_graph_node->pb_type->name,
                                    //pb_graph_node->input_pins[i][j].port->name,
                                    //pb_graph_node->input_pins[i][j].pin_number,
                                    //k,
                                    //pattern_index);
						return pb_graph_node->input_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_output_ports; i++) {
		for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
			for (k = 0; k < pb_graph_node->output_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->output_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if (pb_graph_node->output_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
                        //vtr::printf("Found expansion edge on output pin %s.%s[%d] edge %d pattern %d\n",
                                    //pb_graph_node->pb_type->name,
                                    //pb_graph_node->output_pins[i][j].port->name,
                                    //pb_graph_node->output_pins[i][j].pin_number,
                                    //k,
                                    //pattern_index);
						return pb_graph_node->output_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
		for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
			for (k = 0; k < pb_graph_node->clock_pins[i][j].num_output_edges; k++) {
				for (m = 0; m < pb_graph_node->clock_pins[i][j].output_edges[k]->num_pack_patterns; m++) {
					if (pb_graph_node->clock_pins[i][j].output_edges[k]->pack_pattern_indices[m] == pattern_index) {
                        //vtr::printf("Found expansion edge on clock pin %s.%s[%d] edge %d pattern %d\n",
                                    //pb_graph_node->pb_type->name,
                                    //pb_graph_node->clock_pins[i][j].port->name,
                                    //pb_graph_node->clock_pins[i][j].pin_number,
                                    //k,
                                    //pattern_index);
						return pb_graph_node->clock_pins[i][j].output_edges[k];
					}
				}
			}
		}
	}

	for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
		for (j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
			for (k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
				edge = find_expansion_edge_of_pattern(pattern_index, &pb_graph_node->child_pb_graph_nodes[i][j][k]);
				if (edge != nullptr) {
					return edge;
				}
			}
		}
	}
	return nullptr;
}

/** 
 * Find if receiver of edge is in the same pattern, if yes, add to pattern
 *  Convention: Connections are made on backward expansion only (to make future 
 *              multi-fanout support easier) so this function will not update connections
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
	for (i = 0;	!found && i < expansion_edge->num_pack_patterns; i++) {
		if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
			found = true;
		}
	}
	if (!found) {
		return;
	}

    //vtr::printf("Forward expanding pattern %d edge (%s)\n",
                //curr_pattern_index,
                //describe_pb_graph_edge_hierarchy(expansion_edge).c_str());
	found = false;
	for (i = 0; i < expansion_edge->num_output_pins; i++) {
		if (expansion_edge->output_pins[i]->parent_node->pb_type->num_modes == 0) {
			destination_pb_graph_node = expansion_edge->output_pins[i]->parent_node;
			VTR_ASSERT(found == false);
			/* Check assumption that each forced net has only one fan-out */
			/* This is the destination node */
			found = true;

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			if (destination_pb_graph_node->temp_scratch_pad == nullptr
					|| ((t_pack_pattern_block*) destination_pb_graph_node->temp_scratch_pad)->pattern_index != curr_pattern_index) {
				destination_block = (t_pack_pattern_block*)vtr::calloc(1, sizeof(t_pack_pattern_block));
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(destination_pb_graph_node);
				destination_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
				destination_pb_graph_node->temp_scratch_pad =
						(void *) destination_block;
				destination_block->pattern_index = curr_pattern_index;
				destination_block->pb_type = destination_pb_graph_node->pb_type;
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
			if (((t_pack_pattern_block*) destination_pb_graph_node->temp_scratch_pad)->pattern_index
							== curr_pattern_index) {
				if(make_root_of_chain == true) {
					list_of_packing_patterns[curr_pattern_index].chain_root_pin = expansion_edge->output_pins[i];
					list_of_packing_patterns[curr_pattern_index].root_block = destination_block;
				}
			}
		} else {
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
					}
				}
			}
		}
	}

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
	for (i = 0;	!found && i < expansion_edge->num_pack_patterns; i++) {
		if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
			found = true;
		}
	}
	if (!found) {
		return;
	}

    //vtr::printf("Backward expanding pattern %d edge (%s)\n",
                //curr_pattern_index,
                //describe_pb_graph_edge_hierarchy(expansion_edge).c_str());
	found = false;
	for (i = 0; i < expansion_edge->num_input_pins; i++) {
		if (expansion_edge->input_pins[i]->parent_node->pb_type->num_modes == 0) {
			source_pb_graph_node = expansion_edge->input_pins[i]->parent_node;
			VTR_ASSERT(found == false);
			/* Check assumption that each forced net has only one fan-out */
			/* This is the source node for destination */
			found = true;

            //vtr::printf("\tBackward expanding pattern %d edge input pin '%s' (no modes)\n",
                        //curr_pattern_index,
                        //describe_pb_graph_pin_hierarchy(expansion_edge->input_pins[i]).c_str());

			/* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
			source_block = (t_pack_pattern_block*) source_pb_graph_node->temp_scratch_pad;
			if (source_block == nullptr
                || source_block->pattern_index != curr_pattern_index) {

				source_block = (t_pack_pattern_block *)vtr::calloc(1, sizeof(t_pack_pattern_block));
				source_block->block_id = *L_num_blocks;
				(*L_num_blocks)++;
				list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(source_pb_graph_node);
				source_pb_graph_node->temp_scratch_pad = (void *) source_block;
				source_block->pattern_index = curr_pattern_index;
				source_block->pb_type = source_pb_graph_node->pb_type;

                //vtr::printf("\tAdding pattern %d source block %s[%d]\n",
                            //curr_pattern_index,
                            //source_pb_graph_node->pb_type->name,
                            //source_pb_graph_node->placement_index);

				if (list_of_packing_patterns[curr_pattern_index].root_block == nullptr) {
					list_of_packing_patterns[curr_pattern_index].root_block = source_block;
                    //vtr::printf("\t\tAdded as pattern %d root block\n", curr_pattern_index);
				}

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
				VTR_ASSERT( ((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index);

				source_block = (t_pack_pattern_block*) source_pb_graph_node->temp_scratch_pad;

                //Create the connection
				pack_pattern_connection = (t_pack_pattern_connections *)vtr::calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;

                //Add the new connection to the source block
				pack_pattern_connection->next = source_block->connections;
				source_block->connections = pack_pattern_connection;

                //Create a duplicate connection to store with the sink block
				pack_pattern_connection = (t_pack_pattern_connections *)vtr::calloc(1, sizeof(t_pack_pattern_connections));
				pack_pattern_connection->from_block = source_block;
				pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
				pack_pattern_connection->to_block = destination_block;
				pack_pattern_connection->to_pin = destination_pin;

                //Add the duplicate connection to the destination block
				pack_pattern_connection->next = destination_block->connections;
				destination_block->connections = pack_pattern_connection;

                //vtr::printf("\tAdding connection %s -> %s\n",
                            //describe_pb_graph_pin_hierarchy(pack_pattern_connection->from_pin).c_str(),
                            //describe_pb_graph_pin_hierarchy(pack_pattern_connection->to_pin).c_str());

				if (source_block == destination_block) {
					vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, 
							"Invalid packing pattern defined. Source and destination block are the same (%s).\n",
							source_block->pb_type->name);
				}
			}
		} else {
			if(expansion_edge->input_pins[i]->num_input_edges == 0) {
				if(expansion_edge->input_pins[i]->parent_node->pb_type->parent_mode == nullptr) {
					/* This pack pattern extends to CLB input pin, thus it extends across multiple logic blocks, treat as a chain */
					list_of_packing_patterns[curr_pattern_index].is_chain = true;
					forward_expand_pack_pattern_from_edge(
									expansion_edge,
									list_of_packing_patterns,
									curr_pattern_index, L_num_blocks, true);
				}
			} else {
				for (j = 0; j < expansion_edge->input_pins[i]->num_input_edges; j++) {
					if (expansion_edge->input_pins[i]->input_edges[j]->infer_pattern == true) {
						backward_expand_pack_pattern_from_edge(
								expansion_edge->input_pins[i]->input_edges[j],
								list_of_packing_patterns, curr_pattern_index,
								destination_pin, destination_block, L_num_blocks);
					} else {
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
			cur_molecule->num_ext_inputs = atom_ctx.nlist.block_input_pins(blk_id).size();
			cur_molecule->chain_pattern = nullptr;
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
		molecule->num_ext_inputs = 0;

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
			molecule->num_ext_inputs--; /* This block is revisited, implies net is entirely internal to molecule, reduce count */
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

		molecule->num_ext_inputs += atom_ctx.nlist.block_input_pins(blk_id).size();
		
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
        print_pack_pattern(fp, list_of_pack_patterns[i]);
        fprintf(fp, "\n");
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

static void print_pack_pattern(FILE* fp, 
                               const t_pack_patterns& pack_pattern) {

    VTR_ASSERT(pack_pattern.root_block);
    fprintf(fp, "pack_pattern_index: %d block_count: %d name: '%s' root: '%s' is_chain: %d",
            pack_pattern.index,
            pack_pattern.num_blocks,
            pack_pattern.name,
            pack_pattern.root_block->pb_type->name,
            pack_pattern.is_chain);
    if (pack_pattern.chain_root_pin) {
        fprintf(fp, " chain_root_pin: '%s'",
                describe_pb_graph_pin_hierarchy(pack_pattern.chain_root_pin).c_str());
    } else {
        fprintf(fp, " chain_root_pin: none");
    }
    fprintf(fp, "\n");

    print_pack_pattern_block(fp, pack_pattern.root_block, 1);
}

static void print_pack_pattern_block(FILE* fp, 
                               const t_pack_pattern_block* pack_pattern_block,
                               int depth) {
    fprintf(fp, "%sblock: %s",
            vtr::indent(depth, "  ").c_str(),
            describe_pb_type_hierarchy(pack_pattern_block->pb_type).c_str());
    if (!pack_pattern_block->connections) {
        fprintf(fp, " (no outgoing pack patterns)");
    }
    fprintf(fp, "\n");

    for (t_pack_pattern_connections* conn = pack_pattern_block->connections; conn != nullptr; conn = conn->next) {
        if (conn->from_block != pack_pattern_block) continue; //Only print connections where current is source
        print_pack_pattern_connection(fp, conn, depth + 1);
        print_pack_pattern_block(fp, conn->to_block, depth + 2);
    }
}

static void print_pack_pattern_connection(FILE* fp, 
                               const t_pack_pattern_connections* conn,
                               int depth) {
    fprintf(fp, "%sconn: %s -> %s\n",
            vtr::indent(depth, "  ").c_str(),
            describe_pb_graph_pin_hierarchy(conn->from_pin).c_str(),
            describe_pb_graph_pin_hierarchy(conn->to_pin).c_str());
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

//Walks the PB graph collecting any pack patterns found on edges
class PbGraphPackPatternCollector : public PbGraphWalker {
    public:
        void on_edge(t_pb_graph_edge* edge) override {
            if (edge->num_pack_patterns < 1) return;

            for (int ipin = 0; ipin < edge->num_input_pins; ++ipin) {
                for (int opin = 0; opin < edge->num_output_pins; ++opin) {
                    for (int ipattern = 0; ipattern < edge->num_pack_patterns; ++ipattern) {
                        t_pack_pattern_connection conn;
                        conn.from_pin = edge->input_pins[ipin];
                        conn.to_pin = edge->output_pins[opin];
                        conn.pattern_name = edge->pack_pattern_names[ipattern];
                        
                        pack_pattern_connections[conn.pattern_name].push_back(conn);
                    }
                }
            }
        }

        std::map<std::string,std::vector<t_pack_pattern_connection>> pack_pattern_connections;
};

static void init_pack_patterns() {
    std::vector<t_pack_pattern> arch_pack_patterns;

    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& netlist = atom_ctx.nlist;

    //For every complex block
    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        //Collect the pack pattern connections defined in the architecture
        const auto& pack_pattern_connections = collect_type_pack_pattern_connections(&device_ctx.block_types[itype]);

        vtr::printf("TYPE: %s\n", device_ctx.block_types[itype].name);

        //Convert each set of connections to an architecture pack pattern
        for (auto kv : pack_pattern_connections) {
            t_pack_pattern pack_pattern = build_pack_pattern(kv.first, kv.second);

            arch_pack_patterns.push_back(pack_pattern);


            //Debug
            std::ofstream ofs("arch_pack_pattern." + pack_pattern.name + ".echo");
            write_arch_pack_pattern_dot(ofs, pack_pattern);
        }
    }

    //Covert the architectural pack patterns to netlist pack patterns
    std::vector<t_netlist_pack_pattern> netlist_pack_patterns = abstract_pack_patterns(arch_pack_patterns);

    //Debug
    for (const auto& pack_pattern : netlist_pack_patterns) {
        std::ofstream ofs("netlist_pack_pattern." + pack_pattern.name + ".echo");
        write_netlist_pack_pattern_dot(ofs, pack_pattern);
    }

    for (const auto& pack_pattern : netlist_pack_patterns) {
        auto matches = collect_pattern_matches_in_netlist(pack_pattern, netlist);
    }
}

static std::map<std::string,std::vector<t_pack_pattern_connection>> collect_type_pack_pattern_connections(t_type_ptr type) {
    PbGraphPackPatternCollector collector;

    collector.walk(type->pb_graph_head);

    return collector.pack_pattern_connections;
}

static std::vector<t_netlist_pack_pattern> abstract_pack_patterns(const std::vector<t_pack_pattern>& arch_pack_patterns) {
    std::vector<t_netlist_pack_pattern> netlist_pack_patterns;

    for (auto arch_pattern : arch_pack_patterns) {
        auto netlist_pattern = abstract_pack_pattern(arch_pattern);
        netlist_pack_patterns.push_back(netlist_pattern);
    }

    return netlist_pack_patterns;
}

static t_pack_pattern build_pack_pattern(std::string pattern_name, const std::vector<t_pack_pattern_connection>& connections) {
    t_pack_pattern pack_pattern;
    pack_pattern.name = pattern_name;

    for (auto conn : connections) {
        vtr::printf("pack_pattern '%s' conn: %s -> %s\n", 
                    conn.pattern_name.c_str(),
                    describe_pb_graph_pin_hierarchy(conn.from_pin).c_str(),
                    describe_pb_graph_pin_hierarchy(conn.to_pin).c_str());
    }

    //Create the nodes
    std::map<t_pb_graph_pin*, int> pb_graph_pin_to_pattern_node_id;
    for (auto conn : connections) {
        VTR_ASSERT(conn.pattern_name == pattern_name);

        t_pb_graph_pin* from_pin = conn.from_pin;
        t_pb_graph_pin* to_pin = conn.to_pin;

        if (from_pin) {
            //t_pb_graph_node* from_node = from_pin->parent_node;
            if (!pb_graph_pin_to_pattern_node_id.count(from_pin)) {
                //Create
                int pattern_node_id = pack_pattern.nodes.size();
                pack_pattern.nodes.emplace_back(from_pin);

                //Save ID
                pb_graph_pin_to_pattern_node_id[from_pin] = pattern_node_id;
            }
        }

        if (to_pin) {
            //t_pb_graph_node* to_node = to_pin->parent_node;
            if (!pb_graph_pin_to_pattern_node_id.count(to_pin)) {
                //Create
                int pattern_node_id = pack_pattern.nodes.size();
                pack_pattern.nodes.emplace_back(to_pin);

                //Save ID
                pb_graph_pin_to_pattern_node_id[to_pin] = pattern_node_id;
            }
        }
    }

    //Create the edges
    std::map<std::pair<t_pb_graph_pin*,t_pb_graph_pin*>, int> conn_to_pattern_edge;
    for (auto conn : connections) {
        auto key = std::make_pair(conn.from_pin, conn.to_pin);
        if (!conn_to_pattern_edge.count(key)) {
            //Find connected nodes
            VTR_ASSERT(pb_graph_pin_to_pattern_node_id.count(conn.from_pin));
            VTR_ASSERT(pb_graph_pin_to_pattern_node_id.count(conn.to_pin));
            int from_node_id = pb_graph_pin_to_pattern_node_id[conn.from_pin];
            int to_node_id = pb_graph_pin_to_pattern_node_id[conn.to_pin];

            //Create edge
            int edge_id = pack_pattern.edges.size();
            pack_pattern.edges.emplace_back(); 

            pack_pattern.edges[edge_id].from_node_id = from_node_id;
            pack_pattern.edges[edge_id].to_node_id = to_node_id;

            //Save ID
            conn_to_pattern_edge[key] = edge_id;


            //Add edge references to connected nodes
            pack_pattern.nodes[from_node_id].out_edge_ids.push_back(edge_id);
            pack_pattern.nodes[to_node_id].in_edge_ids.push_back(edge_id);
        }
    }

    //Record roots
    for (size_t inode = 0; inode < pack_pattern.nodes.size(); ++inode) {
        if (pack_pattern.nodes[inode].in_edge_ids.empty()) {
            pack_pattern.root_node_ids.push_back(inode);
        }
    }

    return pack_pattern;
}


static t_netlist_pack_pattern abstract_pack_pattern(const t_pack_pattern& arch_pattern) {
    t_netlist_pack_pattern netlist_pattern;
    netlist_pattern.name = arch_pattern.name;

    struct EdgeInfo {
        EdgeInfo(int from_node, int from_edge, int via_edge, int curr_node)
            : from_arch_node(from_node)
            , from_arch_edge(from_edge)
            , via_arch_edge(via_edge)
            , curr_arch_node(curr_node) {}

        int from_arch_node = OPEN; //Last primitive node we came from
        int from_arch_edge = OPEN; //Last primitive edge we came from
        int via_arch_edge = OPEN; //Edge to current node
        int curr_arch_node = OPEN; //Current node

    };


    //Breadth-First Traversal of arch pack pattern graph, 
    //to record which primitives are reachable from each other.
    //
    //The resulting edges are abstracted from the 
    //pb_graph hierarchy and contain only primitives (no intermediate
    //hiearchy)
    //
    //Note that to get the full set of possible edges we perform a BFT
    //from each primitive/top-level node
    std::vector<EdgeInfo> arch_abstract_edges;
    for (int root_node : arch_pattern.root_node_ids) {
        std::set<int> visited;
        std::queue<EdgeInfo> q;

        vtr::printf("BFT from root %d\n", root_node);

        q.emplace(OPEN, OPEN, OPEN, root_node); //Starting at root

        while (!q.empty()) {
            EdgeInfo info = q.front();
            q.pop();

            int curr_node = info.curr_arch_node;

            if (visited.count(curr_node)) continue; //Don't revisit to avoid loops
            visited.insert(curr_node);
            
            bool is_primitive = arch_pattern.nodes[curr_node].pb_graph_pin->parent_node->is_primitive();
            bool is_top = arch_pattern.nodes[curr_node].pb_graph_pin->parent_node->is_top_level();
            vtr::printf(" Visiting %d via %d (is_prim: %d)\n", curr_node, info.via_arch_edge, is_primitive);

            if ((is_primitive || is_top) && info.from_arch_edge != OPEN && info.via_arch_edge != OPEN) {
                //Record the primitive-to-primitive edge used to reach this node
                arch_abstract_edges.push_back(info);
                vtr::printf("  Recording Abstract Edge from node %d (via edge %d) -> node %d (via edge %d)\n", info.from_arch_node, info.from_arch_edge, curr_node, info.via_arch_edge);
            }

            //Expand out edges
            for (auto out_edge : arch_pattern.nodes[curr_node].out_edge_ids) {

                int next_node = arch_pattern.edges[out_edge].to_node_id;

                if (is_primitive || curr_node == root_node) {
                    //Expanding from a primitive, use the current node as from
                    q.emplace(curr_node, out_edge, out_edge, next_node);
                } else {
                    //Expanding from a non-primitive, re-use the previous primtiive as from
                    q.emplace(info.from_arch_node, info.from_arch_edge, out_edge, next_node);
                }
            }
        }
    }


    //
    //Build the netlist pattern from the edges collected above
    //
    std::map<t_pb_graph_pin*,int> pb_graph_pin_to_netlist_pattern_node;
    std::map<t_pb_graph_node*,int> pb_graph_node_to_netlist_pattern_node;

    std::map<t_pb_graph_pin*,int> pb_graph_pin_to_netlist_pattern_edge;

    for (auto arch_abstract_edge : arch_abstract_edges) {

        //Find or create from node
        int netlist_from_node = OPEN;
        t_pb_graph_pin* from_pin = arch_pattern.nodes[arch_abstract_edge.from_arch_node].pb_graph_pin;
        if (pb_graph_pin_to_netlist_pattern_node.count(from_pin)) {
            //Existing
            netlist_from_node = pb_graph_pin_to_netlist_pattern_node[from_pin];
        } else if (pb_graph_node_to_netlist_pattern_node.count(from_pin->parent_node)) {
            //Existing
            netlist_from_node = pb_graph_node_to_netlist_pattern_node[from_pin->parent_node];
        } else if (from_pin->parent_node->is_top_level()) {
            //Create
            netlist_from_node = netlist_pattern.create_node(true);

            //Default initialization

            //Save Id
            pb_graph_pin_to_netlist_pattern_node[from_pin] = netlist_from_node;
        } else {
            //Create
            netlist_from_node = netlist_pattern.create_node(false);

            //Initialize
            netlist_pattern.nodes[netlist_from_node].model_type = from_pin->parent_node->pb_type->model;

            //Save Id
            pb_graph_node_to_netlist_pattern_node[from_pin->parent_node] = netlist_from_node;
        }
        VTR_ASSERT(netlist_from_node != OPEN);

        //Find or create to node
        int netlist_to_node = OPEN;
        t_pb_graph_pin* to_pin = arch_pattern.nodes[arch_abstract_edge.curr_arch_node].pb_graph_pin;
        if (pb_graph_pin_to_netlist_pattern_node.count(to_pin)) {
            //Existing
            netlist_to_node = pb_graph_pin_to_netlist_pattern_node[to_pin];
        } else if (pb_graph_node_to_netlist_pattern_node.count(to_pin->parent_node)) {
            //Existing
            netlist_to_node = pb_graph_node_to_netlist_pattern_node[to_pin->parent_node];
        } else if (to_pin->parent_node->is_top_level()) {
            //Create
            netlist_to_node = netlist_pattern.create_node(true);

            //Default initialization

            //Save Id
            pb_graph_pin_to_netlist_pattern_node[to_pin] = netlist_to_node;
        } else {
            //Create
            netlist_to_node = netlist_pattern.create_node(false);

            //Initialize
            netlist_pattern.nodes[netlist_to_node].model_type = to_pin->parent_node->pb_type->model;

            //Save Id
            pb_graph_node_to_netlist_pattern_node[to_pin->parent_node] = netlist_to_node;
        }
        VTR_ASSERT(netlist_to_node != OPEN);

        //Create edge

        int netlist_edge = OPEN;
        if (pb_graph_pin_to_netlist_pattern_edge.count(from_pin)) {
            //Existing
            netlist_edge = pb_graph_pin_to_netlist_pattern_edge[from_pin];
        } else {
            //create
            netlist_edge = netlist_pattern.create_edge();

            //Initialize
            netlist_pattern.edges[netlist_edge].from_pin.node_id = netlist_from_node;
            if (!pb_graph_pin_to_netlist_pattern_node.count(from_pin)) {
                //Only initialze these fields for non source/sink
                netlist_pattern.edges[netlist_edge].from_pin.model = from_pin->parent_node->pb_type->model;
                netlist_pattern.edges[netlist_edge].from_pin.model_port = from_pin->port->model_port;
                netlist_pattern.edges[netlist_edge].from_pin.port_pin = from_pin->pin_number;
            }

            //Update node ref
            netlist_pattern.nodes[netlist_from_node].out_edge_ids.push_back(netlist_edge);

            //Save Id
            pb_graph_pin_to_netlist_pattern_edge[from_pin] = netlist_edge;
        }

        //Add sink
        t_netlist_pack_pattern_pin sink_pin;
        sink_pin.node_id = netlist_to_node;
        if (!pb_graph_pin_to_netlist_pattern_node.count(to_pin)) {
            sink_pin.model = to_pin->parent_node->pb_type->model;
            sink_pin.model_port = to_pin->port->model_port;
            sink_pin.port_pin = to_pin->pin_number;
        }

        netlist_pattern.edges[netlist_edge].to_pins.push_back(sink_pin);

        //Update node refs
        netlist_pattern.nodes[netlist_to_node].in_edge_ids.push_back(netlist_edge);
    }

    //Record the root
    for (size_t inode = 0; inode < netlist_pattern.nodes.size(); ++inode) {
        if (netlist_pattern.nodes[inode].in_edge_ids.empty()) {
            if (netlist_pattern.root_node != OPEN) {
                VPR_THROW(VPR_ERROR_ARCH, 
                        "Pack pattern '%s' has multiple roots (a link is probably missing causing the pattern to be disconnected)",
                        netlist_pattern.name.c_str());
            }
            VTR_ASSERT(netlist_pattern.root_node == OPEN);
            netlist_pattern.root_node = inode;
        }
    }
    VTR_ASSERT(netlist_pattern.root_node != OPEN);

    return netlist_pattern;
}

static std::vector<Match> collect_pattern_matches_in_netlist(const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    std::vector<Match> matches;

    std::set<AtomBlockId> covered_blks;

    for (auto blk : netlist.blocks()) {
        if (covered_blks.count(blk)) continue; //Already in molecule
        if (!matches_pattern_root(blk, pattern, netlist)) continue; //Not a valid root

        //Initially the current block is the only candidate
        std::stack<AtomBlockId> root_candidates;
        root_candidates.push(blk);

        //Collect any root candidates upstream of the current blk
        while (root_candidates.top()) {
            AtomBlockId next_root_candidate = find_parent_pattern_root(root_candidates.top(), pattern, netlist); 

            if (!next_root_candidate) break; //No more potential roots
            if (covered_blks.count(next_root_candidate)) break; //Already used

            root_candidates.push(next_root_candidate);

            VTR_ASSERT_SAFE(matches_pattern_root(next_root_candidate, pattern, netlist));
        }


        //Try each candidate root from the most upstream downward
        // This ensures we get the largest match from the most upstream root connected to the
        // original blk
        Match match;
        while (!match && !root_candidates.empty()) {
            match = match_largest(root_candidates.top(), pattern, covered_blks, netlist);
            root_candidates.pop();
        }

        if (match) {
            //Save the match
            matches.push_back(match);

            //Record the blocks covered so blocks don't end up in multiple matches
            auto match_blocks = collect_internal_blocks_in_match(match, pattern, netlist);
            covered_blks.insert(match_blocks.begin(), match_blocks.end());

            print_match(match, pattern, netlist);
        }
    }

    return matches;
}

static std::set<AtomBlockId> collect_internal_blocks_in_match(const Match& match, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    std::set<AtomBlockId> blocks;

    for (auto match_edge : match.netlist_edges) {
        int pattern_from_node = pattern.edges[match_edge.pattern_edge_id].from_pin.node_id;
        int pattern_to_node = pattern.edges[match_edge.pattern_edge_id].to_pins[match_edge.pattern_edge_sink].node_id;

        if (pattern.nodes[pattern_from_node].is_internal()) {
            blocks.insert(netlist.pin_block(match_edge.from_pin));
        }

        if (pattern.nodes[pattern_to_node].is_internal()) {
            blocks.insert(netlist.pin_block(match_edge.to_pin));
        }
    }

    return blocks;
}

static Match match_largest(AtomBlockId blk, const t_netlist_pack_pattern& pattern, const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {
    Match match;
    if (match_largest_recur(match, blk, pattern.root_node, pattern, excluded_blocks, netlist)) {
        return match; 
    }
    return Match(); //No match, return empty
}

static bool match_largest_recur(Match& match, const AtomBlockId blk, 
                                int pattern_node_id, const t_netlist_pack_pattern& pattern,
                                const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {

    if (excluded_blocks.count(blk) && pattern.nodes[pattern_node_id].is_internal()) {
        //Block is already part of another match, and is matching an internal node of the pattern
        return false;
    }

    if (!matches_pattern_node(blk, pattern.nodes[pattern_node_id], netlist)) {
        return false; 
    }

    for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) {
        const auto& pattern_edge = pattern.edges[pattern_edge_id];
        const auto& from_pattern_pin = pattern_edge.from_pin;

        AtomPinId from_pin = find_matching_pin(netlist.block_output_pins(blk), from_pattern_pin, excluded_blocks, netlist);
        if (!from_pin) {
            //No matching driver pin
            if (from_pattern_pin.required) {
                //Required: give-up
                return false;
            } else {
                //Optional: try next edge
                continue;
            }
        }

        AtomNetId net = netlist.pin_net(from_pin);
        auto net_sinks = netlist.net_sinks(net);

        for (size_t isink = 0; isink < pattern_edge.to_pins.size(); ++isink) {
            const auto& to_pattern_pin = pattern_edge.to_pins[isink];
            const auto& to_pattern_node = pattern.nodes[to_pattern_pin.node_id];

            AtomPinId to_pin = find_matching_pin(net_sinks, to_pattern_pin, excluded_blocks, netlist);
            if (!to_pin || (excluded_blocks.count(netlist.pin_block(to_pin)) && to_pattern_node.is_internal())) {
                //No valid to_pin (either doesn't match pattern, or is on an excluded block)

                if (to_pattern_pin.required) {
                    //Required: give-up
                    return false;
                } else {
                    //Optional: try next pattern pin
                    continue;
                }
            }

            //Valid match between netlist from_pin/to_pin and from_pattern_pin/to_pattern_pin
            //Add it to the match
            match.netlist_edges.emplace_back(from_pin, to_pin, pattern_edge_id, isink);

            //Collect any downstream matches recursively
            AtomBlockId to_blk = netlist.pin_block(to_pin);
            bool subtree_matched = match_largest_recur(match, to_blk, to_pattern_pin.node_id, pattern, excluded_blocks, netlist);

            if (!subtree_matched) {
                return false;
            }
        }
    }

    return true;
}

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const AtomNetlist& netlist) {
    return find_matching_pin(pin_range, pattern_pin, {}, netlist);
}

static AtomPinId find_matching_pin(const AtomNetlist::pin_range pin_range, const t_netlist_pack_pattern_pin& pattern_pin,
                                   const std::set<AtomBlockId>& excluded_blocks, const AtomNetlist& netlist) {

    for (AtomPinId pin : pin_range) {
        if (matches_pattern_pin(pin, pattern_pin, netlist)) {

            if (excluded_blocks.count(netlist.pin_block(pin))) {
                continue;
            } else {
                return pin;
            }
        }
    }

    return AtomPinId::INVALID(); //No match
}

//Returns a parent block of blk, if it is also a valid root for pattern
static AtomBlockId find_parent_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    int pattern_node_id = pattern.root_node;

    VTR_ASSERT_SAFE(matches_pattern_root(blk, pattern, netlist));

    //Find an upstream block which is also a valid root
    for (auto pins : {netlist.block_input_pins(blk), netlist.block_clock_pins(blk)}) { //Current blocks inputs
        for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) { //Root out edges
            for (const auto& pattern_pin : pattern.edges[pattern_edge_id].to_pins) { //Edge pins

                //Do the inputs of the current block match the output edges of the root pattern?
                AtomPinId to_pin = find_matching_pin(pins, pattern_pin, netlist);
                if (!to_pin) continue;

                AtomNetId in_net = netlist.pin_net(to_pin);
                AtomBlockId from_blk = netlist.net_driver_block(in_net);

                if (!matches_pattern_root(from_blk, pattern, netlist)) continue;

                return from_blk;
            }
        }
    }

    return AtomBlockId::INVALID();
}

//Returns true if matches the pattern root node (and it's out-going edges)
static bool matches_pattern_root(const AtomBlockId blk, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    int pattern_node_id = pattern.root_node;

    if (!matches_pattern_node(blk, pattern.nodes[pattern_node_id], netlist)) {
        return false; 
    }

    for (int pattern_edge_id : pattern.nodes[pattern_node_id].out_edge_ids) {
        const auto& pattern_edge = pattern.edges[pattern_edge_id];

        AtomPinId from_pin = find_matching_pin(netlist.block_output_pins(blk), pattern_edge.from_pin, netlist);
        if (!from_pin) {
            //No matching driver pin
            return false;
        }

        AtomNetId net = netlist.pin_net(from_pin);
        auto net_sinks = netlist.net_sinks(net);

        for (size_t isink = 0; isink < pattern_edge.to_pins.size(); ++isink) {
            const auto& to_pattern_pin = pattern_edge.to_pins[isink];

            AtomPinId to_pin = find_matching_pin(net_sinks, to_pattern_pin, netlist);
            if (!to_pin) {

                if (to_pattern_pin.required) {
                    //Required: give-up
                    return false;
                } else {
                    //Optional: try next pattern pin
                    continue;
                }
            }
        }
    }
    return true;
}

static bool matches_pattern_node(const AtomBlockId blk, const t_netlist_pack_pattern_node& pattern_node, const AtomNetlist& netlist) {
    if (is_wildcard_node(pattern_node)) {
        return true; //Wildcard
    }
    return netlist.block_model(blk) == pattern_node.model_type;
}

static bool is_wildcard_node(const t_netlist_pack_pattern_node& pattern_node) {
    return pattern_node.model_type == nullptr;
}

static bool is_wildcard_pin(const t_netlist_pack_pattern_pin& pattern_pin) {
    return (pattern_pin.model == nullptr
            && pattern_pin.model_port == nullptr
            && pattern_pin.port_pin == OPEN);
}

static bool matches_pattern_pin(const AtomPinId from_pin, const t_netlist_pack_pattern_pin& pattern_pin, const AtomNetlist& netlist) {

    if (is_wildcard_pin(pattern_pin)) {
        return true; //Wildcard
    }

    if (netlist.block_model(netlist.pin_block(from_pin)) != pattern_pin.model
        || netlist.port_model(netlist.pin_port(from_pin)) != pattern_pin.model_port
        || netlist.pin_port_bit(from_pin) != (BitIndex) pattern_pin.port_pin) {
        return false;
    }

    return true;
}

static void print_match(const Match& match, const t_netlist_pack_pattern& pattern, const AtomNetlist& netlist) {
    vtr::printf("Netlist Match to %s:\n", pattern.name.c_str());
    for (auto& edge : match.netlist_edges) {
        int pattern_from_node_id = pattern.edges[edge.pattern_edge_id].from_pin.node_id;
        int pattern_to_node_id = pattern.edges[edge.pattern_edge_id].to_pins[edge.pattern_edge_sink].node_id;
        vtr::printf("  %s -> %s (%s%s -> %s%s) (pattern edge #%d.%d)\n", 
                netlist.pin_name(edge.from_pin).c_str(),
                netlist.pin_name(edge.to_pin).c_str(),
                netlist.block_model(netlist.pin_block(edge.from_pin))->name,
                (pattern.nodes[pattern_from_node_id].is_external()) ? "*" : "",
                netlist.block_model(netlist.pin_block(edge.to_pin))->name,
                (pattern.nodes[pattern_to_node_id].is_external()) ? "*" : "",
                edge.pattern_edge_id,
                edge.pattern_edge_sink);
    }
    for (auto blk : collect_internal_blocks_in_match(match, pattern, netlist)) {
        vtr::printf("\tInternal match block: %s (%zu)\n", netlist.block_name(blk).c_str(), size_t(blk));

    }
}

static void write_arch_pack_pattern_dot(std::ostream& os, const t_pack_pattern& arch_pattern) {
    os << "#Dot file of architecture pack pattern graph\n";
    os << "digraph " << arch_pattern.name << " {\n";

    for (size_t inode = 0; inode < arch_pattern.nodes.size(); ++inode) {
        os << "N" << inode;
        
        os << " [";

        os << "label=\"" << describe_pb_graph_pin_hierarchy(arch_pattern.nodes[inode].pb_graph_pin) << " (#" << inode << ")\"";

        os << "];\n";
    }

    for (size_t iedge = 0; iedge < arch_pattern.edges.size(); ++iedge) {
        os << "N" << arch_pattern.edges[iedge].from_node_id << " -> N" << arch_pattern.edges[iedge].to_node_id;
        os << " [";

        os << "label=\"" ;
        os << "(#" << iedge << ")";
        os << "\"";

        os << "];\n";
    }

    os << "}\n";
}

static void write_netlist_pack_pattern_dot(std::ostream& os, const t_netlist_pack_pattern& netlist_pattern) {
    os << "#Dot file of netlist pack pattern graph\n";
    os << "digraph " << netlist_pattern.name << " {\n";

    //Nodes
    for (size_t inode = 0; inode < netlist_pattern.nodes.size(); ++inode) {
        os << "N" << inode;
        
        os << " [";

        os << "label=\"";
        if (netlist_pattern.nodes[inode].model_type) {
            os << netlist_pattern.nodes[inode].model_type->name;
        } else {
            os << "*";
        }
        os << " (#" << inode << ")";
        os << "\"";

        if (netlist_pattern.nodes[inode].is_external()) {
            os << " shape=invhouse";
        }

        os << "];\n";
    }

    //Edges
    for (size_t iedge = 0; iedge < netlist_pattern.edges.size(); ++iedge) {
        const auto& edge = netlist_pattern.edges[iedge];

        const auto& from_pin = edge.from_pin;
        for (size_t isink = 0; isink < edge.to_pins.size(); ++isink) {
            const auto& to_pin = edge.to_pins[isink];

            os << "N" << edge.from_pin.node_id << " -> N" << edge.to_pins[isink].node_id;
            os << " [";

            os << "label=\"" ;

            os << "(#" << iedge << "." << isink << ")\\n";
            if (is_wildcard_pin(from_pin)) {
                os << "*";
            } else {
                os << from_pin.model_port->name << "[" << from_pin.port_pin << "]";
            }
            os << "\\n -> ";
            if (is_wildcard_pin(to_pin)) {
                os << "*";
            } else {
                os << to_pin.model_port->name << "[" << to_pin.port_pin << "]";
            }
            os << "\"";

            if (to_pin.required) {
                os << " style=solid";
            } else {
                os << " style=dashed";
            }

            os << "];\n";
        }
    }

    os << "}\n";
}
