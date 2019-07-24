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
#include <unordered_set>
#include <queue>
#include <utility>
using namespace std;

#include "vtr_util.h"
#include "vtr_assert.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "read_xml_arch_file.h"
#include "globals.h"
#include "atom_netlist.h"
#include "prepack.h"
#include "vpr_utils.h"
#include "echo_files.h"

/*****************************************/
/*Local Function Declaration			 */
/*****************************************/
static void discover_pattern_names_in_pb_graph_node(t_pb_graph_node* pb_graph_node,
                                                    std::unordered_map<std::string, int>& pattern_names);

static void forward_infer_pattern(t_pb_graph_pin* pb_graph_pin);

static void backward_infer_pattern(t_pb_graph_pin* pb_graph_pin);

static t_pack_patterns* alloc_and_init_pattern_list_from_hash(std::unordered_map<std::string, int> pattern_names);

static t_pb_graph_edge* find_expansion_edge_of_pattern(const int pattern_index,
                                                       const t_pb_graph_node* pb_graph_node);

static void forward_expand_pack_pattern_from_edge(const t_pb_graph_edge* expansion_edge,
                                                  t_pack_patterns* list_of_packing_patterns,
                                                  const int curr_pattern_index,
                                                  int* L_num_blocks,
                                                  const bool make_root_of_chain);

static void backward_expand_pack_pattern_from_edge(const t_pb_graph_edge* expansion_edge,
                                                   t_pack_patterns* list_of_packing_patterns,
                                                   const int curr_pattern_index,
                                                   t_pb_graph_pin* destination_pin,
                                                   t_pack_pattern_block* destination_block,
                                                   int* L_num_blocks);

static int compare_pack_pattern(const t_pack_patterns* pattern_a, const t_pack_patterns* pattern_b);

static void free_pack_pattern(t_pack_pattern_block* pattern_block, t_pack_pattern_block** pattern_block_list);

static t_pack_molecule* try_create_molecule(t_pack_patterns* list_of_pack_patterns,
                                            std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                            const int pack_pattern_index,
                                            AtomBlockId blk_id);

static bool try_expand_molecule(t_pack_molecule* molecule,
                                const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                const AtomBlockId blk_id);

static void print_pack_molecules(const char* fname,
                                 const t_pack_patterns* list_of_pack_patterns,
                                 const int num_pack_patterns,
                                 const t_pack_molecule* list_of_molecules);

static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block(const AtomBlockId blk_id);

static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(const AtomBlockId blk_id, t_pb_graph_node* curr_pb_graph_node, float* cost);

static AtomBlockId find_new_root_atom_for_chain(const AtomBlockId blk_id, const t_pack_patterns* list_of_pack_pattern, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

static std::vector<t_pb_graph_pin*> find_end_of_path(t_pb_graph_pin* input_pin, int pattern_index);

static void expand_search(const t_pb_graph_pin* input_pin, std::queue<t_pb_graph_pin*>& pins_queue, const int pattern_index);

static void find_all_equivalent_chains(t_pack_patterns* chain_pattern, const t_pb_graph_node* root_block);

static void update_chain_root_pins(t_pack_patterns* chain_pattern,
                                   const std::vector<t_pb_graph_pin*>& chain_input_pins);

static t_pb_graph_pin* get_connected_primitive_input_pin(const t_pb_graph_pin* input_pin, const int pack_pattern);

static t_pb_graph_pin* get_connected_primitive_output_pin(const t_pb_graph_pin* output_pin, const int pack_pattern);

static void get_all_connected_primitive_pins(const t_pb_graph_pin* cluster_input_pin, std::vector<t_pb_graph_pin*>& connected_primitive_pins);

static void init_molecule_chain_info(const AtomBlockId blk_id, t_pack_molecule* molecule, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

static AtomBlockId get_sink_block(const AtomBlockId block_id, const t_model_ports* model_port, const BitIndex pin_number);

static AtomBlockId get_driving_block(const AtomBlockId block_id, const t_model_ports* model_port, const BitIndex pin_number);

static void print_chain_starting_points(t_pack_patterns* chain_pattern);

static t_pb_graph_pin* find_chain_exit_pin(t_pb_graph_pin* input_pin, int pattern_index);

static t_pack_pattern_block* get_atom_pattern_block(const t_pack_molecule* molecule, const int block_id);

static bool chain_input_is_reachable(const t_pack_molecule* molecule,
                                     const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

static bool check_second_level_chain(const t_pack_molecule* molecule,
                                     const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

static t_pb_graph_node* get_driver_pb_graph_node(const t_pack_molecule* prev_molecule, const AtomBlockId driver_block);

static int get_forced_chain_id(t_pack_molecule* molecule, const t_pack_molecule* prev_molecule, const AtomBlockId driver_block_id);

static AtomBlockId get_adder_driver_block(const AtomBlockId block_id, const t_pack_patterns* pack_pattern, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules);

static bool molecule_is_hierarchical(const t_pack_molecule* molecule);

static bool valid_second_level_placement(const AtomBlockId first_level_block, const AtomBlockId block_id, const t_pack_molecule* molecule);

static AtomBlockId is_second_level_block(const t_pack_pattern_block* pattern_block, const t_pack_molecule* molecule);

static bool check_alm_input_limitation(t_pack_molecule* molecule);

static void get_block_input_nets(const AtomBlockId block_id, std::unordered_set<AtomNetId>& nets);

static int get_pb_placement_index(t_pack_pattern_block* pattern_block, std::string pb_name);

static void modify_molecule(t_pack_molecule* molecule, t_pack_pattern_block* pattern_block);

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
t_pack_patterns* alloc_and_load_pack_patterns(int* num_packing_patterns) {
    int L_num_blocks;
    t_pack_patterns* list_of_packing_patterns;
    t_pb_graph_edge* expansion_edge;
    auto& device_ctx = g_vpr_ctx.device();

    /* alloc and initialize array of packing patterns based on architecture complex blocks */
    std::unordered_map<std::string, int> pattern_names;
    for (int i = 0; i < device_ctx.num_block_types; i++) {
        discover_pattern_names_in_pb_graph_node(device_ctx.block_types[i].pb_graph_head, pattern_names);
    }

    list_of_packing_patterns = alloc_and_init_pattern_list_from_hash(pattern_names);

    /* load packing patterns by traversing the edges to find edges belonging to pattern */
    for (size_t i = 0; i < pattern_names.size(); i++) {
        for (int j = 0; j < device_ctx.num_block_types; j++) {
            // find an edge that belongs to this pattern
            expansion_edge = find_expansion_edge_of_pattern(i, device_ctx.block_types[j].pb_graph_head);
            if (!expansion_edge) {
                continue;
            }

            L_num_blocks = 0;
            list_of_packing_patterns[i].base_cost = 0;
            // use the found expansion edge to build the pack pattern
            backward_expand_pack_pattern_from_edge(expansion_edge,
                                                   list_of_packing_patterns, i, nullptr, nullptr, &L_num_blocks);
            list_of_packing_patterns[i].num_blocks = L_num_blocks;

            /* Default settings: A section of a netlist must match all blocks in a pack
             * pattern before it can be made a molecule except for carry-chains.
             * For carry-chains, since carry-chains are typically quite flexible in terms
             * of size, it is optional whether or not an atom in a netlist matches any
             * particular block inside the chain */
            list_of_packing_patterns[i].is_block_optional = (bool*)vtr::malloc(L_num_blocks * sizeof(bool));
            for (int k = 0; k < L_num_blocks; k++) {
                list_of_packing_patterns[i].is_block_optional[k] = false;
                if (list_of_packing_patterns[i].is_chain && list_of_packing_patterns[i].root_block->block_id != k) {
                    list_of_packing_patterns[i].is_block_optional[k] = true;
                }
            }

            // if this is a chain pattern (extends between complex blocks), check if there
            // are multiple equivalent chains with different starting and ending points
            if (list_of_packing_patterns[i].is_chain) {
                find_all_equivalent_chains(&list_of_packing_patterns[i], device_ctx.block_types[j].pb_graph_head);
                print_chain_starting_points(&list_of_packing_patterns[i]);
            }

            // if pack pattern i is found to belong to block j, go to next pack pattern
            break;
        }
    }

    //Sanity check, every pattern should have a root block
    for (size_t i = 0; i < pattern_names.size(); ++i) {
        if (list_of_packing_patterns[i].root_block == nullptr) {
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "Failed to find root block for pack pattern %s", list_of_packing_patterns[i].name);
        }
    }

    *num_packing_patterns = pattern_names.size();

    return list_of_packing_patterns;
}

/**
 * Locate all pattern names
 * Side-effect: set all pb_graph_node temp_scratch_pad field to NULL
 *				For cases where a pattern inference is "obvious", mark it as obvious.
 */
static void discover_pattern_names_in_pb_graph_node(t_pb_graph_node* pb_graph_node,
                                                    std::unordered_map<std::string, int>& pattern_names) {
    /* Iterate over all edges to discover if an edge in current physical block belongs to a pattern
     * If edge does, then record the name of the pattern in a hash table */

    if (pb_graph_node == nullptr) {
        return;
    }

    pb_graph_node->temp_scratch_pad = nullptr;

    for (int i = 0; i < pb_graph_node->num_input_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            bool hasPattern = false;
            for (int k = 0; k < pb_graph_node->input_pins[i][j].num_output_edges; k++) {
                auto output_edge = pb_graph_node->input_pins[i][j].output_edges[k];
                for (int m = 0; m < output_edge->num_pack_patterns; m++) {
                    hasPattern = true;
                    // insert the found pattern name to the hash table. If this pattern is inserted
                    // for the first time, then its index is the current size of the hash table
                    // otherwise the insert function will return an iterator of the previously
                    // inserted element with the index given to that pattern
                    std::string pattern_name(output_edge->pack_pattern_names[m]);
                    int index = (pattern_names.insert({pattern_name, pattern_names.size()}).first)->second;
                    if (!output_edge->pack_pattern_indices) {
                        output_edge->pack_pattern_indices = (int*)vtr::malloc(output_edge->num_pack_patterns * sizeof(int));
                    }
                    output_edge->pack_pattern_indices[m] = index;
                    // if this output edges belongs to a pack pattern. Expand forward starting from
                    // all its output pins to check if you need to infer pattern for direct connections
                    for (int ipin = 0; ipin < output_edge->num_output_pins; ipin++) {
                        forward_infer_pattern(output_edge->output_pins[ipin]);
                    }
                }
            }
            // if the output edge to this pin is annotated with a pack pattern
            // trace the inputs to this pin and mark them to infer pattern
            // if they are direct connections (num_input_edges == 1)
            if (hasPattern) {
                backward_infer_pattern(&pb_graph_node->input_pins[i][j]);
            }
        }
    }

    for (int i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            bool hasPattern = false;
            for (int k = 0; k < pb_graph_node->output_pins[i][j].num_output_edges; k++) {
                auto output_edge = pb_graph_node->output_pins[i][j].output_edges[k];
                for (int m = 0; m < output_edge->num_pack_patterns; m++) {
                    hasPattern = true;
                    // insert the found pattern name to the hash table. If this pattern is inserted
                    // for the first time, then its index is the current size of the hash table
                    // otherwise the insert function will return an iterator of the previously
                    // inserted element with the index given to that pattern
                    std::string pattern_name(output_edge->pack_pattern_names[m]);
                    int index = (pattern_names.insert({pattern_name, pattern_names.size()}).first)->second;
                    if (!output_edge->pack_pattern_indices) {
                        output_edge->pack_pattern_indices = (int*)vtr::malloc(output_edge->num_pack_patterns * sizeof(int));
                    }
                    output_edge->pack_pattern_indices[m] = index;
                    // if this output edges belongs to a pack pattern. Expand forward starting from
                    // all its output pins to check if you need to infer pattern for direct connections
                    for (int ipin = 0; ipin < output_edge->num_output_pins; ipin++) {
                        forward_infer_pattern(output_edge->output_pins[ipin]);
                    }
                }
            }
            // if the output edge to this pin is annotated with a pack pattern
            // trace the inputs to this pin and mark them to infer pattern
            // if they are direct connections (num_input_edges == 1)
            if (hasPattern) {
                backward_infer_pattern(&pb_graph_node->output_pins[i][j]);
            }
        }
    }

    for (int i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (int j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            bool hasPattern = false;
            for (int k = 0; k < pb_graph_node->clock_pins[i][j].num_output_edges; k++) {
                auto& output_edge = pb_graph_node->clock_pins[i][j].output_edges[k];
                for (int m = 0; m < output_edge->num_pack_patterns; m++) {
                    hasPattern = true;
                    // insert the found pattern name to the hash table. If this pattern is inserted
                    // for the first time, then its index is the current size of the hash table
                    // otherwise the insert function will return an iterator of the previously
                    // inserted element with the index given to that pattern
                    std::string pattern_name(output_edge->pack_pattern_names[m]);
                    int index = (pattern_names.insert({pattern_name, pattern_names.size()}).first)->second;
                    if (output_edge->pack_pattern_indices == nullptr) {
                        output_edge->pack_pattern_indices = (int*)vtr::malloc(output_edge->num_pack_patterns * sizeof(int));
                    }
                    output_edge->pack_pattern_indices[m] = index;
                    // if this output edges belongs to a pack pattern. Expand forward starting from
                    // all its output pins to check if you need to infer pattern for direct connections
                    for (int ipin = 0; ipin < output_edge->num_output_pins; ipin++) {
                        forward_infer_pattern(output_edge->output_pins[ipin]);
                    }
                }
            }
            // if the output edge to this pin is annotated with a pack pattern
            // trace the inputs to this pin and mark them to infer pattern
            // if they are direct connections (num_input_edges == 1)
            if (hasPattern) {
                backward_infer_pattern(&pb_graph_node->clock_pins[i][j]);
            }
        }
    }

    for (int i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
        for (int j = 0; j < pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
            for (int k = 0; k < pb_graph_node->pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                discover_pattern_names_in_pb_graph_node(&pb_graph_node->child_pb_graph_nodes[i][j][k], pattern_names);
            }
        }
    }
}

/**
 * In obvious cases where a pattern edge has only one path to go, set that path to be inferred
 */
static void forward_infer_pattern(t_pb_graph_pin* pb_graph_pin) {
    if (pb_graph_pin->num_output_edges == 1 && pb_graph_pin->output_edges[0]->num_pack_patterns == 0 && pb_graph_pin->output_edges[0]->infer_pattern == false) {
        pb_graph_pin->output_edges[0]->infer_pattern = true;
        if (pb_graph_pin->output_edges[0]->num_output_pins == 1) {
            forward_infer_pattern(pb_graph_pin->output_edges[0]->output_pins[0]);
        }
    }
}
static void backward_infer_pattern(t_pb_graph_pin* pb_graph_pin) {
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
static t_pack_patterns* alloc_and_init_pattern_list_from_hash(std::unordered_map<std::string, int> pattern_names) {
    t_pack_patterns* nlist = new t_pack_patterns[pattern_names.size()];

    for (const auto& curr_pattern : pattern_names) {
        VTR_ASSERT(nlist[curr_pattern.second].name == nullptr);
        nlist[curr_pattern.second].name = vtr::strdup(curr_pattern.first.c_str());
        nlist[curr_pattern.second].root_block = nullptr;
        nlist[curr_pattern.second].is_chain = false;
        nlist[curr_pattern.second].index = curr_pattern.second;
    }

    return nlist;
}

void free_list_of_pack_patterns(t_pack_patterns* list_of_pack_patterns, const int num_packing_patterns) {
    int i, j, num_pack_pattern_blocks;
    t_pack_pattern_block** pattern_block_list;
    if (list_of_pack_patterns != nullptr) {
        for (i = 0; i < num_packing_patterns; i++) {
            num_pack_pattern_blocks = list_of_pack_patterns[i].num_blocks;
            pattern_block_list = (t_pack_pattern_block**)vtr::calloc(num_pack_pattern_blocks, sizeof(t_pack_pattern_block*));
            free(list_of_pack_patterns[i].name);
            free(list_of_pack_patterns[i].is_block_optional);
            free_pack_pattern(list_of_pack_patterns[i].root_block, pattern_block_list);
            for (j = 0; j < num_pack_pattern_blocks; j++) {
                free(pattern_block_list[j]);
            }
            free(pattern_block_list);
        }
        delete[] list_of_pack_patterns;
    }
}

/**
 * Locate first edge that belongs to pattern index
 */
static t_pb_graph_edge* find_expansion_edge_of_pattern(const int pattern_index,
                                                       const t_pb_graph_node* pb_graph_node) {
    int i, j, k, m;
    t_pb_graph_edge* edge;
    /* Iterate over all edges to discover if an edge in current physical block belongs to a pattern
     * If edge does, then return that edge
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
 *  This function expands forward from the given expansion_edge. If a primitive is found that
 *  belongs to the pack pattern we are searching for, create a pack pattern block of using
 *  this primitive to be added later to the pack pattern when creating the pack pattern
 *  connections in the backward_expand_pack_pattern_from_edge function.
 *
 *  expansion_edge: starting edge to expand forward from
 *  list_of_packing_patterns: list of packing patterns in the architecture
 *  curr_pattern_index: current packing pattern that we are building
 *  L_num_blocks: number of primitives found to belong to this pattern so far
 *  make_root_of_chain: flag indicating that the given expansion_edge is connected
 *                      to a primitive that is the root of this packing pattern
 *
 *  Convention: Pack pattern block connections are made on backward expansion only (to make
 *              future multi-fanout support easier) so this function will not update connections
 */
static void forward_expand_pack_pattern_from_edge(const t_pb_graph_edge* expansion_edge,
                                                  t_pack_patterns* list_of_packing_patterns,
                                                  const int curr_pattern_index,
                                                  int* L_num_blocks,
                                                  bool make_root_of_chain) {
    int i, j, k;
    int iport, ipin, iedge;
    bool found; /* Error checking, ensure only one fan-out for each pattern net */
    t_pack_pattern_block* destination_block = nullptr;
    t_pb_graph_node* destination_pb_graph_node = nullptr;

    found = expansion_edge->infer_pattern;
    // if the pack pattern shouldn't be inferred check if the expansion
    // edge is annotated with the current pack pattern we are expanding
    for (i = 0; !found && i < expansion_edge->num_pack_patterns; i++) {
        if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
            found = true;
        }
    }
    // if this edge isn't annotated with the current pack pattern
    // no need to explore it
    if (!found) {
        return;
    }

    found = false;
    // iterate over the expansion edge output pins
    for (i = 0; i < expansion_edge->num_output_pins; i++) {
        // check if expansion_edge parent node is a primitive (i.e num_nodes = 0)
        if (expansion_edge->output_pins[i]->is_primitive_pin()) {
            destination_pb_graph_node = expansion_edge->output_pins[i]->parent_node;
            VTR_ASSERT(found == false);
            /* Check assumption that each forced net has only one fan-out */
            /* This is the destination node */
            found = true;

            // the temp_scratch_pad points to the last primitive from this pb_graph_node that was added to a packing pattern.
            const auto& destination_pb_temp = (t_pack_pattern_block*)destination_pb_graph_node->temp_scratch_pad;
            // if this pb_graph_node (primitive) is not added to the packing pattern already, add it and expand all its edges
            if (destination_pb_temp == nullptr || destination_pb_temp->pattern_index != curr_pattern_index) {
                // a primitive that belongs to this pack pattern is found: 1) create a new pattern block,
                // 2) assign an id to this pattern block, 3) increment the number of found blocks belonging to this
                // pattern and 4) expand all its edges to find the other primitives that belong to this pattern
                destination_block = (t_pack_pattern_block*)vtr::calloc(1, sizeof(t_pack_pattern_block));
                list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(destination_pb_graph_node);
                destination_block->block_id = *L_num_blocks;
                (*L_num_blocks)++;
                destination_pb_graph_node->temp_scratch_pad = (void*)destination_block;
                destination_block->pattern_index = curr_pattern_index;
                destination_block->pb_type = destination_pb_graph_node->pb_type;

                // explore the inputs to this primitive
                for (iport = 0; iport < destination_pb_graph_node->num_input_ports; iport++) {
                    for (ipin = 0; ipin < destination_pb_graph_node->num_input_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < destination_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
                            backward_expand_pack_pattern_from_edge(destination_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
                                                                   list_of_packing_patterns,
                                                                   curr_pattern_index,
                                                                   &destination_pb_graph_node->input_pins[iport][ipin],
                                                                   destination_block, L_num_blocks);
                        }
                    }
                }

                // explore the outputs of this primitive
                for (iport = 0; iport < destination_pb_graph_node->num_output_ports; iport++) {
                    for (ipin = 0; ipin < destination_pb_graph_node->num_output_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < destination_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
                            forward_expand_pack_pattern_from_edge(destination_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
                                                                  list_of_packing_patterns,
                                                                  curr_pattern_index, L_num_blocks, false);
                        }
                    }
                }

                // explore the clock pins of this primitive
                for (iport = 0; iport < destination_pb_graph_node->num_clock_ports; iport++) {
                    for (ipin = 0; ipin < destination_pb_graph_node->num_clock_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < destination_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
                            backward_expand_pack_pattern_from_edge(destination_pb_graph_node->clock_pins[iport][ipin].input_edges[iedge],
                                                                   list_of_packing_patterns,
                                                                   curr_pattern_index,
                                                                   &destination_pb_graph_node->clock_pins[iport][ipin],
                                                                   destination_block, L_num_blocks);
                        }
                    }
                }
            }

            // if this pb_graph_node (primitive) should be added to the pack pattern blocks
            if (((t_pack_pattern_block*)destination_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index) {
                // if this pb_graph_node is known to be the root of the chain, update the root block and root pin
                if (make_root_of_chain && destination_block) {
                    list_of_packing_patterns[curr_pattern_index].chain_root_pins = {{expansion_edge->output_pins[i]}};
                    list_of_packing_patterns[curr_pattern_index].root_block = destination_block;
                }
            }

            // the expansion_edge parent node is not a primitive
        } else {
            // continue expanding forward
            for (j = 0; j < expansion_edge->output_pins[i]->num_output_edges; j++) {
                if (expansion_edge->output_pins[i]->output_edges[j]->infer_pattern == true) {
                    forward_expand_pack_pattern_from_edge(expansion_edge->output_pins[i]->output_edges[j],
                                                          list_of_packing_patterns,
                                                          curr_pattern_index,
                                                          L_num_blocks,
                                                          make_root_of_chain);
                } else {
                    for (k = 0; k < expansion_edge->output_pins[i]->output_edges[j]->num_pack_patterns; k++) {
                        if (expansion_edge->output_pins[i]->output_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
                            if (found == true) {
                                /* Check assumption that each forced net has only one fan-out */
                                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                                "Invalid packing pattern defined.  Multi-fanout nets not supported when specifying pack patterns.\n"
                                                "Problem on %s[%d].%s[%d] for pattern %s\n",
                                                expansion_edge->output_pins[i]->parent_node->pb_type->name,
                                                expansion_edge->output_pins[i]->parent_node->placement_index,
                                                expansion_edge->output_pins[i]->port->name,
                                                expansion_edge->output_pins[i]->pin_number,
                                                list_of_packing_patterns[curr_pattern_index].name);
                            }
                            found = true;
                            forward_expand_pack_pattern_from_edge(expansion_edge->output_pins[i]->output_edges[j],
                                                                  list_of_packing_patterns,
                                                                  curr_pattern_index,
                                                                  L_num_blocks,
                                                                  make_root_of_chain);
                        }
                    } // End for pack patterns of output edge
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
static void backward_expand_pack_pattern_from_edge(const t_pb_graph_edge* expansion_edge,
                                                   t_pack_patterns* list_of_packing_patterns,
                                                   const int curr_pattern_index,
                                                   t_pb_graph_pin* destination_pin,
                                                   t_pack_pattern_block* destination_block,
                                                   int* L_num_blocks) {
    int i, j, k;
    int iport, ipin, iedge;
    bool found; /* Error checking, ensure only one fan-out for each pattern net */
    t_pack_pattern_block* source_block = nullptr;
    t_pb_graph_node* source_pb_graph_node = nullptr;
    t_pack_pattern_connections* pack_pattern_connection = nullptr;

    found = expansion_edge->infer_pattern;
    // if the pack pattern shouldn't be inferred check if the expansion
    // edge is annotated with the current pack pattern we are expanding
    for (i = 0; !found && i < expansion_edge->num_pack_patterns; i++) {
        if (expansion_edge->pack_pattern_indices[i] == curr_pattern_index) {
            found = true;
        }
    }

    // if this edge isn't annotated with the current pack pattern
    // no need to explore it
    if (!found) {
        return;
    }

    found = false;
    // iterate over all the drivers of this edge
    for (i = 0; i < expansion_edge->num_input_pins; i++) {
        // check if the expansion_edge parent node is a primitive
        if (expansion_edge->input_pins[i]->is_primitive_pin()) {
            source_pb_graph_node = expansion_edge->input_pins[i]->parent_node;
            VTR_ASSERT(found == false);
            /* Check assumption that each forced net has only one fan-out */
            /* This is the source node for destination */
            found = true;

            /* If this pb_graph_node is part not of the current pattern index, put it in and expand all its edges */
            source_block = (t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad;
            if (source_block == nullptr || source_block->pattern_index != curr_pattern_index) {
                source_block = (t_pack_pattern_block*)vtr::calloc(1, sizeof(t_pack_pattern_block));
                source_block->block_id = *L_num_blocks;
                (*L_num_blocks)++;
                list_of_packing_patterns[curr_pattern_index].base_cost += compute_primitive_base_cost(source_pb_graph_node);
                source_pb_graph_node->temp_scratch_pad = (void*)source_block;
                source_block->pattern_index = curr_pattern_index;
                source_block->pb_type = source_pb_graph_node->pb_type;

                if (list_of_packing_patterns[curr_pattern_index].root_block == nullptr) {
                    list_of_packing_patterns[curr_pattern_index].root_block = source_block;
                }

                // explore the inputs of this primitive
                for (iport = 0; iport < source_pb_graph_node->num_input_ports; iport++) {
                    for (ipin = 0; ipin < source_pb_graph_node->num_input_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < source_pb_graph_node->input_pins[iport][ipin].num_input_edges; iedge++) {
                            backward_expand_pack_pattern_from_edge(source_pb_graph_node->input_pins[iport][ipin].input_edges[iedge],
                                                                   list_of_packing_patterns,
                                                                   curr_pattern_index,
                                                                   &source_pb_graph_node->input_pins[iport][ipin],
                                                                   source_block,
                                                                   L_num_blocks);
                        }
                    }
                }

                // explore the outputs of this primitive
                for (iport = 0; iport < source_pb_graph_node->num_output_ports; iport++) {
                    for (ipin = 0; ipin < source_pb_graph_node->num_output_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < source_pb_graph_node->output_pins[iport][ipin].num_output_edges; iedge++) {
                            forward_expand_pack_pattern_from_edge(source_pb_graph_node->output_pins[iport][ipin].output_edges[iedge],
                                                                  list_of_packing_patterns,
                                                                  curr_pattern_index,
                                                                  L_num_blocks,
                                                                  false);
                        }
                    }
                }

                // explore the clock pins of this primitive
                for (iport = 0; iport < source_pb_graph_node->num_clock_ports; iport++) {
                    for (ipin = 0; ipin < source_pb_graph_node->num_clock_pins[iport]; ipin++) {
                        for (iedge = 0; iedge < source_pb_graph_node->clock_pins[iport][ipin].num_input_edges; iedge++) {
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

            if (destination_pin != nullptr) {
                VTR_ASSERT(((t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad)->pattern_index == curr_pattern_index);
                source_block = (t_pack_pattern_block*)source_pb_graph_node->temp_scratch_pad;
                pack_pattern_connection = (t_pack_pattern_connections*)vtr::calloc(1, sizeof(t_pack_pattern_connections));
                pack_pattern_connection->from_block = source_block;
                pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
                pack_pattern_connection->to_block = destination_block;
                pack_pattern_connection->to_pin = destination_pin;
                pack_pattern_connection->next = source_block->connections;
                source_block->connections = pack_pattern_connection;

                pack_pattern_connection = (t_pack_pattern_connections*)vtr::calloc(1, sizeof(t_pack_pattern_connections));
                pack_pattern_connection->from_block = source_block;
                pack_pattern_connection->from_pin = expansion_edge->input_pins[i];
                pack_pattern_connection->to_block = destination_block;
                pack_pattern_connection->to_pin = destination_pin;
                pack_pattern_connection->next = destination_block->connections;
                destination_block->connections = pack_pattern_connection;

                if (source_block == destination_block) {
                    VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                    "Invalid packing pattern defined. Source and destination block are the same (%s).\n",
                                    source_block->pb_type->name);
                }
            }

            // expansion edge parent is not a primitive
        } else {
            // check if this input pin of the expansion edge has no driving pin
            if (expansion_edge->input_pins[i]->num_input_edges == 0) {
                // check if this input pin of the expansion edge belongs to a root block (i.e doesn't have a parent block)
                if (expansion_edge->input_pins[i]->parent_node->pb_type->parent_mode == nullptr) {
                    // This pack pattern extends to CLB (root pb block) input pin,
                    // thus it extends across multiple logic blocks, treat as a chain
                    list_of_packing_patterns[curr_pattern_index].is_chain = true;
                    // since this input pin has not driving nets, expand in the forward direction instead
                    forward_expand_pack_pattern_from_edge(expansion_edge,
                                                          list_of_packing_patterns,
                                                          curr_pattern_index,
                                                          L_num_blocks,
                                                          true);
                }
                // this input pin of the expansion edge has a driving pin
            } else {
                // iterate over all the driving edges of this input pin
                for (j = 0; j < expansion_edge->input_pins[i]->num_input_edges; j++) {
                    // if pattern should be inferred for this edge continue the expansion backwards
                    if (expansion_edge->input_pins[i]->input_edges[j]->infer_pattern == true) {
                        backward_expand_pack_pattern_from_edge(expansion_edge->input_pins[i]->input_edges[j],
                                                               list_of_packing_patterns,
                                                               curr_pattern_index,
                                                               destination_pin,
                                                               destination_block,
                                                               L_num_blocks);
                        // if pattern shouldn't be inferred
                    } else {
                        // check if this input pin edge is annotated with the current pattern
                        for (k = 0; k < expansion_edge->input_pins[i]->input_edges[j]->num_pack_patterns; k++) {
                            if (expansion_edge->input_pins[i]->input_edges[j]->pack_pattern_indices[k] == curr_pattern_index) {
                                VTR_ASSERT(found == false);
                                /* Check assumption that each forced net has only one fan-out */
                                found = true;
                                backward_expand_pack_pattern_from_edge(expansion_edge->input_pins[i]->input_edges[j],
                                                                       list_of_packing_patterns,
                                                                       curr_pattern_index,
                                                                       destination_pin,
                                                                       destination_block,
                                                                       L_num_blocks);
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
t_pack_molecule* alloc_and_load_pack_molecules(t_pack_patterns* list_of_pack_patterns,
                                               std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                               std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                                               const int num_packing_patterns) {
    int i, j, best_pattern;
    t_pack_molecule* list_of_molecules_head;
    t_pack_molecule* cur_molecule;
    bool* is_used;
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
        for (j = 1; j < num_packing_patterns; j++) {
            if (is_used[best_pattern]) {
                best_pattern = j;
            } else if (is_used[j] == false && compare_pack_pattern(&list_of_pack_patterns[j], &list_of_pack_patterns[best_pattern]) == 1) {
                best_pattern = j;
            }
        }
        VTR_ASSERT(is_used[best_pattern] == false);
        is_used[best_pattern] = true;

        auto blocks = atom_ctx.nlist.blocks();
        for (auto blk_iter = blocks.begin(); blk_iter != blocks.end(); ++blk_iter) {
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
                if (range_empty || !cur_was_last_inserted) {
                    /* molecule did not cover current atom (possibly because molecule created is
                     * part of a long chain that extends past multiple logic blocks), try again */
                    --blk_iter;
                }
            }
        }
    }
    free(is_used);

    /* List all atom blocks as a molecule for blocks that do not belong to any molecules.
     * This allows the packer to be consistent as it now packs molecules only instead of atoms and molecules
     *
     * If a block belongs to a molecule, then carrying the single atoms around can make the packing problem
     * more difficult because now it needs to consider splitting molecules.
     */
    for (auto blk_id : atom_ctx.nlist.blocks()) {
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

static void free_pack_pattern(t_pack_pattern_block* pattern_block, t_pack_pattern_block** pattern_block_list) {
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
static t_pack_molecule* try_create_molecule(t_pack_patterns* list_of_pack_patterns,
                                            std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                            const int pack_pattern_index,
                                            AtomBlockId blk_id) {
    t_pack_molecule* molecule;

    auto pack_pattern = &list_of_pack_patterns[pack_pattern_index];

    // Check pack pattern validity
    if (pack_pattern == nullptr || pack_pattern->num_blocks == 0 || pack_pattern->root_block == nullptr) {
        return nullptr;
    }

    // If a chain pattern extends beyond a single logic block, we must find
    // the furthest blk_id up the chain that is not mapped to a molecule yet.
    if (pack_pattern->is_chain) {
        blk_id = find_new_root_atom_for_chain(blk_id, pack_pattern, atom_molecules);
        if (!blk_id) return nullptr;
    }

    molecule = new t_pack_molecule;
    molecule->valid = true;
    molecule->type = MOLECULE_FORCED_PACK;
    molecule->pack_pattern = pack_pattern;
    molecule->atom_block_ids = std::vector<AtomBlockId>(pack_pattern->num_blocks); //Initializes invalid
    molecule->num_blocks = pack_pattern->num_blocks;
    molecule->root = pack_pattern->root_block->block_id;

    if (try_expand_molecule(molecule, atom_molecules, blk_id)) {
        // Success! commit molecule

        // update chain info for chain molecules
        if (molecule->pack_pattern->is_chain) {
            init_molecule_chain_info(blk_id, molecule, atom_molecules);
        }

        // update the atom_molcules with the atoms that are mapped to this molecule
        for (int i = 0; i < molecule->pack_pattern->num_blocks; i++) {
            auto blk_id2 = molecule->atom_block_ids[i];
            if (!blk_id2) {
                VTR_ASSERT(molecule->pack_pattern->is_block_optional[i]);
                continue;
            }

            atom_molecules.insert({blk_id2, molecule});
        }
    } else {
        // Failed to create molecule
        delete molecule;
        return nullptr;
    }

    return molecule;
}

/**
 * Determine if an atom block can match with the pattern to from a molecule.
 *
 * This function takes a molecule that represents a packing pattern. It also
 * takes a (netlist) atom block represented by blk_id which matches the
 * root primitive of this packing pattern. Using this atom block and the structure
 * of the packing pattern, this function tries to fill all the available positions
 * in the packing pattern. If all the non-optional primitive positions in the
 * pattern are filled return true, return false otherwise.
 *      molecule       : the molecule we are trying to expand
 *      atom_molecules : map of atom block ids that are assigned a molecule and a pointer to this molecule
 *      blk_id         : chosen to be the root of this molecule and the code is expanding from
 */
static bool try_expand_molecule(t_pack_molecule* molecule,
                                const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                const AtomBlockId blk_id) {
    bool has_second_level = false;
    bool found_second_level = false;
    const bool hierarchical_molecule = molecule_is_hierarchical(molecule);
    const t_model_ports* cin_port_model = nullptr;
    if (molecule->is_chain()) {
        cin_port_model = molecule->pack_pattern->chain_root_pins[0][0]->port->model_port;
    }
    // root block of the pack pattern, which is the starting point of this pattern
    const auto pattern_root_block = molecule->pack_pattern->root_block;
    // bool array indicating whether a position in a pack pattern is optional or should
    // be filled with an atom for legality
    const auto is_block_optional = molecule->pack_pattern->is_block_optional;

    // create a queue of pattern block and atom block id suggested for this block
    std::queue<std::pair<t_pack_pattern_block*, AtomBlockId>> pattern_block_queue;
    // initialize the queue with the pattern root block and the matching atom block
    pattern_block_queue.push(std::make_pair(pattern_root_block, blk_id));

    // do breadth first search by walking through the pack pattern structure along
    // with the atom netlist structure
    while (!pattern_block_queue.empty()) {
        // get the front pattern block, atom block id pair from the queue
        const auto pattern_block_atom_pair = pattern_block_queue.front();
        const auto pattern_block = pattern_block_atom_pair.first;
        const auto block_id = pattern_block_atom_pair.second;

        // remove the front of the queue
        pattern_block_queue.pop();

        // get the atom block id of the atom occupying this primitive position in this molecule
        auto molecule_atom_block_id = molecule->atom_block_ids[pattern_block->block_id];

        // if this primitive position in this molecule is already visited go to
        // the next node in the queue
        // NOTE: Another condition might need to be added for completeness. That is
        // the block found in this position in the molecule should match the block
        // found in the netlist. Meaning (molecule_atom_block_id == block id).
        // This condition is not added to be able handle the case where inside a pack
        // pattern there is a connection between a primitive and an adder input. Since
        // the packer doesn't know that adder inputs are logically equivalent. If the
        // packer becomes aware of this, this condition could be added.
        if (molecule_atom_block_id) {
            continue;
        }

        if (!block_id || !primitive_type_feasible(block_id, pattern_block->pb_type) || (molecule_atom_block_id && molecule_atom_block_id != block_id) || atom_molecules.find(block_id) != atom_molecules.end()) {
            // Stopping conditions, if:
            // 1) this is an invalid atom block (nothing)
            // 2) this atom block cannot fit in this primitive type
            // 3) this primitive is occupied by another block
            // 4) this atom block is already used by another molecule
            // then if the molecule cannot be formed without placing an atom
            // at that primitive position, then creating this molecule has failed
            // otherwise go to the next atom block and its corresponding pattern block
            if (!is_block_optional[pattern_block->block_id]) {
                return false;
            }
            continue;
        }


        if (hierarchical_molecule && !found_second_level) {
            auto first_level_block = is_second_level_block(pattern_block, molecule);
            if (first_level_block) {
                if (valid_second_level_placement(first_level_block, block_id, molecule))
                    found_second_level = true;
                else
                    continue;
            }
        }

        // before commiting this atom check if this a second level addition. If so, make sure that if the flag is unset
        // this this is only driven by a dummy adder and set the flag. If the flag is unset and it's not driven by a dummy
        // adder ignore it.
        // set this node in the molecule as visited

        molecule->atom_block_ids[pattern_block->block_id] = block_id;

        // starting from the first connections, add all the connections of this block to the queue
        auto block_connection = pattern_block->connections;

        while (block_connection != nullptr) {
            // this block is the driver of this connection
            if (block_connection->from_block == pattern_block) {
                // find the block this connection is driving and add it to the queue
                auto port_model = block_connection->from_pin->port->model_port;
                auto ipin = block_connection->from_pin->pin_number;
                auto sink_blk_id = get_sink_block(block_id, port_model, ipin);
                // add this sink block id with its corresponding pattern block to the queue
                pattern_block_queue.push(std::make_pair(block_connection->to_block, sink_blk_id));
                // this block is being driven by this connection
            } else if (block_connection->to_block == pattern_block) {
                // find the block that is driving this connection and it to the queue
                auto port_model = block_connection->to_pin->port->model_port;
                auto ipin = block_connection->to_pin->pin_number;
                auto driver_blk_id = get_driving_block(block_id, port_model, ipin);
                if (molecule->is_chain() && port_model != cin_port_model &&
                    block_connection->to_pin->parent_node->pb_type == block_connection->from_pin->parent_node->pb_type) {
                    has_second_level = true;
                }
                // add this driver block id with its corresponding pattern block to the queue
                // only if it's driving the cin port. To avoid adding blocks by tracking the adder
                // inputs port which will result in a molecule that cannot be placed
                if (molecule->is_chain() && (port_model == cin_port_model || block_connection->from_block->pb_type->model != molecule->pack_pattern->chain_root_pins[0][0]->parent_node->pb_type->model)) {
                    pattern_block_queue.push(std::make_pair(block_connection->from_block, driver_blk_id));
                }
            }

            // this block should be either driving or driven by the connection
            VTR_ASSERT(block_connection->from_block == pattern_block || block_connection->to_block == pattern_block);
            // go to the next connection of this pattern block
            block_connection = block_connection->next;
        }
    }

    if (!has_second_level && hierarchical_molecule) {
        return false;
    }

    // if this molecule is being fed by another molecule check that the root
    // block is reachable from the other molecule
    // if all non-optional positions in the pack pattern have atoms
    // mapped to them, then this molecule is valid
    if (molecule->is_chain()) {
        if (chain_input_is_reachable(molecule, atom_molecules) && check_second_level_chain(molecule, atom_molecules))
            return check_alm_input_limitation(molecule);
        else
            return false;
    }

    return true;
}

static void print_nets(std::unordered_set<AtomNetId>& nets, int alm_placement_index, int alut_placement_index) {
    VTR_LOG("Placement index: %d->%d (%d)\n", alm_placement_index, alut_placement_index, nets.size());
    for (const auto net : nets) {
       VTR_LOG("%d %s\n", net, g_vpr_ctx.atom().nlist.net_name(net).c_str());
    }
    VTR_LOG("\n");
}

// get the number of ALM inputs feeding the LUTs. The assumption is
// ALMs with 4-LUT has 6 inputs feeding LUTs, however, ALMs with 3-LUTs
// has 8 inputs feeding LUTs. This is a very specific assumption targeting
// the architectures in this study
static size_t get_alm_inputs_feeding_luts(t_pack_molecule* molecule) {

    auto pattern_block = molecule->pack_pattern->root_block;
    auto cin_pin = molecule->pack_pattern->chain_root_pins[0][0];
    auto cin_port = cin_pin->port;
    auto cin_port_model = cin_port->model_port;

    auto connection = pattern_block->connections;

    while (connection) {
       if (connection->to_block == pattern_block && connection->to_pin->port->model_port != cin_port_model) {
           auto lut_pb_type = connection->from_pin->parent_node->pb_type;
           auto lut_input_pins = lut_pb_type->num_input_pins;
           return (lut_input_pins == 3)? 8 : 6;
       }
       connection = connection->next;
    }

    VTR_ASSERT(false);
    return 0;

}

static bool check_alm_input_limitation(t_pack_molecule* molecule) {

    std::string pattern_name(molecule->pack_pattern->name);
    if (pattern_name.find("lut_chain") != 0) return true;

    auto& atom_ctx = g_vpr_ctx.atom();

    auto block_id = molecule->atom_block_ids[molecule->root];
    auto pattern_block = molecule->pack_pattern->root_block;

    const auto ALM_INPUTS = get_alm_inputs_feeding_luts(molecule);

    auto cin_pin = molecule->pack_pattern->chain_root_pins[0][0];
    auto cin_port = cin_pin->port;
    auto cin_port_model = cin_port->model_port;

    std::string alm_name = "fle";
    if (get_pb_placement_index(pattern_block, alm_name) == -1)
        alm_name = "fle1";
    std::string alut_name = "ble5";
    auto alm_placement_index = get_pb_placement_index(pattern_block, alm_name);
    auto alut_placement_index = get_pb_placement_index(pattern_block, alut_name);

    std::unordered_set<AtomNetId> alm_nets;
    std::unordered_set<AtomNetId> alut_nets;

    while(true) {
       auto connection = pattern_block->connections;
       // get the unique net ids feeding the adders
       VTR_LOG("\n%s\n", atom_ctx.nlist.block_name(molecule->atom_block_ids[pattern_block->block_id]).c_str());
       while (connection) {
           if (connection->to_block == pattern_block && connection->to_pin->port->model_port != cin_port_model) {
               auto& lut_id = molecule->atom_block_ids[connection->from_block->block_id];
               if (lut_id) {
                   get_block_input_nets(lut_id, alm_nets);
                   get_block_input_nets(lut_id, alut_nets);
                   VTR_LOG("LUT %s (%zu)\n", atom_ctx.nlist.block_name(lut_id).c_str(), atom_ctx.nlist.block_input_pins(lut_id).size());
                   if (atom_ctx.nlist.block_input_pins(lut_id).empty()) {
                       alm_nets.insert((AtomNetId) 0);
                       alut_nets.insert((AtomNetId) 0);
                   }
               } else {
                   auto port_id = atom_ctx.nlist.find_atom_port(block_id, connection->to_pin->port->model_port);
                   if (port_id) {
                       auto net_id = atom_ctx.nlist.port_net(port_id, connection->to_pin->pin_number);
                       if (net_id) {
                           alm_nets.insert(net_id);
                           alut_nets.insert(net_id);
                       }
                   }
               }
           }
           connection = connection->next;
       }

       if (alm_nets.empty()) break;

       print_nets(alm_nets, alm_placement_index, alut_placement_index);
       // go to the next pattern block if it is still in the same ALM
       connection = pattern_block->connections;
       bool found_to_block = false;
       while (connection) {
           if (connection->from_block == pattern_block && connection->to_pin->port->model_port == cin_port_model) {
               auto alm_new_placement_index = get_pb_placement_index(connection->to_block, alm_name);
               auto alut_new_placement_index = get_pb_placement_index(connection->to_block, alut_name);
               if (alm_placement_index != alm_new_placement_index) {
                   if (alm_nets.size() > ALM_INPUTS || alut_nets.size() > 4) {
                       print_nets(alm_nets, alm_placement_index, alut_placement_index);
                       modify_molecule(molecule, pattern_block);
                       return check_alm_input_limitation(molecule);
                   }
                   alut_nets.clear();
                   alm_nets.clear();
                   alm_placement_index = alm_new_placement_index;
                   alut_placement_index = alut_new_placement_index;
               } else if (alut_placement_index != alut_new_placement_index) {
                   if (alut_nets.size() > 4) {
                       print_nets(alm_nets, alm_placement_index, alut_placement_index);
                       modify_molecule(molecule, pattern_block);
                       return check_alm_input_limitation(molecule);
                   }
                   alut_nets.clear();
                   alut_placement_index = alut_new_placement_index;
               }
               pattern_block = connection->to_block;
               block_id = molecule->atom_block_ids[pattern_block->block_id];
               found_to_block = true;
               break;
           }
           connection = connection->next;
       }

       if (!found_to_block) {
           if (alm_nets.size() > ALM_INPUTS || alut_nets.size() > 4) {
               print_nets(alm_nets, alm_placement_index, alut_placement_index);
               modify_molecule(molecule, pattern_block);
               return check_alm_input_limitation(molecule);
           }
           break;
       }
    }

    return true;
}

static bool valid_second_level_placement(const AtomBlockId first_level_block, const AtomBlockId block_id, const t_pack_molecule* molecule) {

    const auto& atom_ctx = g_vpr_ctx.atom();
    // check that this block is being placed in the second level of the chain
    auto cin_pin = molecule->pack_pattern->chain_root_pins[0][0];
    auto cin_port_model = cin_pin->port->model_port;

    auto first_level_driver = first_level_block;
    auto second_level_driver = block_id;
    do {
        first_level_driver = atom_ctx.nlist.find_atom_pin_driver(first_level_driver, cin_port_model, cin_pin->pin_number);
        second_level_driver = atom_ctx.nlist.find_atom_pin_driver(second_level_driver, cin_port_model, cin_pin->pin_number);

        if (first_level_driver && !second_level_driver) return true;
        if (!first_level_driver && second_level_driver) return false;
    } while(first_level_driver || second_level_driver);

    // check the number of atoms connected to cin of this molecule and not yet in a molecule
    // is less than or equal to the same number of first level atom directly above this one
    return true;
}

static AtomBlockId is_second_level_block(const t_pack_pattern_block* pattern_block, const t_pack_molecule* molecule) {
    auto cin_pin = molecule->pack_pattern->chain_root_pins[0][0];
    auto cin_port_model = cin_pin->port->model_port;
    auto connection = pattern_block->connections;
    while(connection) {
        // if this block is being driven by this connection and the port being driven is not cin port
        if (connection->to_block == pattern_block &&
            connection->to_pin->port->model_port != cin_port_model) {
            return molecule->atom_block_ids[connection->from_block->block_id];
        }
        connection = connection->next;
    }

    return AtomBlockId::INVALID();
}

static void modify_molecule(t_pack_molecule* molecule, t_pack_pattern_block* pattern_block) {
    auto cin_pin = molecule->pack_pattern->chain_root_pins[0][0];
    auto cin_port = cin_pin->port;
    auto cin_port_model = cin_port->model_port;

    auto connection = pattern_block->connections;
    bool node_removed = false;

    while (true) {
        connection = pattern_block->connections;
        // get the unique net ids feeding the adders
        while (connection) {
           if (connection->to_block == pattern_block && connection->to_pin->port->model_port != cin_port_model) {
               auto& lut_id = molecule->atom_block_ids[connection->from_block->block_id];
               if (lut_id && g_vpr_ctx.atom().nlist.block_input_pins(lut_id).size() > 1) {
                   molecule->atom_block_ids[connection->from_block->block_id] = AtomBlockId::INVALID();
                   node_removed = true;
                   break;
               }
           }
           connection = connection->next;
        }
        if (node_removed) return;

        connection = pattern_block->connections;
        bool found_from_block = false;
        while (connection) {
            if (connection->to_block == pattern_block && connection->to_pin->port->model_port == cin_port_model) {
                pattern_block = connection->from_block;
                found_from_block = true;
                break;
            }
            connection = connection->next;
        }
        VTR_ASSERT(found_from_block);
    }
}

static void get_block_input_nets(const AtomBlockId block_id, std::unordered_set<AtomNetId>& nets) {
    const auto& atom_ctx = g_vpr_ctx.atom();
    for (const auto& pin_id : atom_ctx.nlist.block_input_pins(block_id)) {
         nets.insert(atom_ctx.nlist.pin_net(pin_id));
     }
}

static int get_pb_placement_index(t_pack_pattern_block* pattern_block, std::string pb_name) {

    auto connection = pattern_block->connections;

    while(connection) {
        if (connection->to_block == pattern_block) break;
        connection = connection->next;
    }

    VTR_ASSERT(connection);

    auto input_pin = connection->to_pin;
    auto parent_node = input_pin->parent_node;

    std::string parent_name(parent_node->pb_type->name);

    while (parent_node && parent_name != pb_name) {
        parent_node = parent_node->parent_pb_graph_node;
        if (!parent_node) break;
        std::string name(parent_node->pb_type->name);
        parent_name = name;
    }

    //VTR_ASSERT(parent_node);
    if (!parent_node)
        return -1;
    return parent_node->placement_index;
}

/**
 * Find the atom block in the netlist driven by this pin of the input atom block
 * If doesn't exist return AtomBlockId::INVALID()
 * Limitation: The block should be driving only one sink block
 *      block_id   : id of the atom block that is driving the net connected to the sink block
 *      model_port : the model of the port driving the net
 *      pin_number : the pin_number of the pin driving the net (pin index within the port)
 */
static AtomBlockId get_sink_block(const AtomBlockId block_id, const t_model_ports* model_port, const BitIndex pin_number) {
    auto& atom_ctx = g_vpr_ctx.atom();

    auto port_id = atom_ctx.nlist.find_atom_port(block_id, model_port);

    if (port_id) {
        auto net_id = atom_ctx.nlist.port_net(port_id, pin_number);
        if (net_id && atom_ctx.nlist.net_sinks(net_id).size() == 1) { /* Single fanout assumption */
            auto net_sinks = atom_ctx.nlist.net_sinks(net_id);
            auto sink_pin_id = *(net_sinks.begin());
            return atom_ctx.nlist.pin_block(sink_pin_id);
        }
    }

    return AtomBlockId::INVALID();
}

/**
 * Find the atom block in the netlist driving this pin of the input atom block
 * If doesn't exist return AtomBlockId::INVALID()
 * Limitation: This driving block should be driving only the input block
 *      block_id   : id of the atom block that is connected to a net driven by the driving block
 *      model_port : the model of the port driven by the net
 *      pin_number : the pin_number of the pin driven by the net (pin index within the port)
 */
static AtomBlockId get_driving_block(const AtomBlockId block_id, const t_model_ports* model_port, const BitIndex pin_number) {
    if (model_port->is_clock) {
        VTR_ASSERT(pin_number == 1); //TODO: support multi-clock primitives
    }

    auto& atom_ctx = g_vpr_ctx.atom();

    auto port_id = atom_ctx.nlist.find_atom_port(block_id, model_port);

    if (port_id) {
        auto net_id = atom_ctx.nlist.port_net(port_id, pin_number);
        if (net_id && atom_ctx.nlist.net_sinks(net_id).size() == 1) { /* Single fanout assumption */
            return atom_ctx.nlist.net_driver_block(net_id);
        }
    }

    return AtomBlockId::INVALID();
}

/**
 * This function checks the second level chain either starts at the same position
 * or after the first level. Since the root block of the molecule is the first adder
 * in the first level chain, having a second chain that starts earlier will not allow
 * the driver of it's cin to reach it.
 */
static bool check_second_level_chain(const t_pack_molecule* molecule,
                                     const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    const auto cout_pin_model = molecule->pack_pattern->chain_exit_pins[0]->port->model_port;
    const auto root_block = molecule->pack_pattern->root_block;
    auto connection = root_block->connections;
    // get the id of the very first atom in the second level of chain
    AtomBlockId second_root;

    // iterate over all the connections of the root block
    while (connection) {
        // if this connection is driven by sumout port of the root block then, the block
        // connected to this pin is the second root
        if (connection->from_block == root_block && connection->from_pin->port->model_port != cout_pin_model) {
            VTR_ASSERT(connection->from_pin->parent_node->pb_type == connection->to_pin->parent_node->pb_type);
            second_root = molecule->atom_block_ids[connection->to_block->block_id];
            break;
        }
        connection = connection->next;
    }

    // if there is no atom placed in the position of the second root
    // then this case will not happen for this molecule
    if (!second_root) return true;

    // get the driver of this atom

    const auto& chain_root_pins = molecule->pack_pattern->chain_root_pins;
    // get the model of the cin port of the adder primitive
    const auto cin_port_model = chain_root_pins[0][0]->port->model_port;
    // get the pin number of the cin pin within the cin port
    const auto cin_pin_number = chain_root_pins[0][0]->pin_number;
    // get the atom block driving the root block of this molecule
    const auto driver_block = g_vpr_ctx.atom().nlist.find_atom_pin_driver(second_root, cin_port_model, cin_pin_number);

    auto driver_molecule_it = atom_molecules.find(driver_block);
    // if the driver block is not in molecule yet
    // then the block is driven by a constant net
    // which will not cause a problem in routing
    if (driver_molecule_it == atom_molecules.end())
        return false;

    auto driver_molecule = driver_molecule_it->second;

    if (driver_molecule->type != MOLECULE_FORCED_PACK)
        return false;

    return true;
}

/**
 * This function checks if the cin of the root block is driven by another atom
 * or not and returns true it is not driven by another atom or is driven but
 * this atom could still reach the root block through the direct connect
 */
static bool chain_input_is_reachable(const t_pack_molecule* molecule,
                                     const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    const auto& chain_root_pins = molecule->pack_pattern->chain_root_pins;
    // assume that if the molecule can start in multiple locations
    // it will always be reachable from the previous molecule
    if (chain_root_pins.size() > 1)
        return true;

    // id of the root block of this molecule
    const auto root_block = molecule->atom_block_ids[molecule->root];
    // get the model of the cin port of the adder primitive
    const auto cin_port_model = chain_root_pins[0][0]->port->model_port;
    // get the pin number of the cin pin within the cin port
    const auto cin_pin_number = chain_root_pins[0][0]->pin_number;
    // get the atom block driving the root block of this molecule
    const auto driver_block = g_vpr_ctx.atom().nlist.find_atom_pin_driver(root_block, cin_port_model, cin_pin_number);

    auto driver_molecule_it = atom_molecules.find(driver_block);
    // if the driver block is not in molecule yet
    // then the block is driven by a constant net
    if (driver_molecule_it == atom_molecules.end())
        return true;

    auto driver_molecule = driver_molecule_it->second;

    if (driver_molecule->type != MOLECULE_FORCED_PACK)
        return true;

    t_pb_graph_node* driver_pb_graph_node = get_driver_pb_graph_node(driver_molecule, driver_block);

    // get the model of the cout port of the adder primitive
    const auto cout_port_model = molecule->pack_pattern->chain_exit_pins[0]->port->model_port;

    for (int iport = 0; iport < driver_pb_graph_node->num_output_ports; iport++) {
        for (int ipin = 0; ipin < driver_pb_graph_node->num_output_pins[iport]; ipin++) {
            const auto& pin = driver_pb_graph_node->output_pins[iport][ipin];
            if (pin.port->model_port == cout_port_model) {
                if (&pin == molecule->pack_pattern->chain_exit_pins[0])
                    return true;
                else
                    return false;
            }
        }
    }

    return false;
}

/**
 * This function finds the atom driving the root block of a molecule
 * and find the pb_graph_node associated with this block
 */
static t_pb_graph_node* get_driver_pb_graph_node(const t_pack_molecule* driver_molecule, const AtomBlockId driver_block) {
    auto it = std::find(driver_molecule->atom_block_ids.begin(), driver_molecule->atom_block_ids.end(), driver_block);
    VTR_ASSERT(it != driver_molecule->atom_block_ids.end());

    auto driver_pattern_block_id = std::distance(driver_molecule->atom_block_ids.begin(), it);
    auto driver_pattern_block = get_atom_pattern_block(driver_molecule, driver_pattern_block_id);

    auto block_connection = driver_pattern_block->connections;
    while (block_connection) {
        if (block_connection->to_block == driver_pattern_block) {
            return block_connection->to_pin->parent_node;
        }
        block_connection = block_connection->next;
    }

    VTR_ASSERT(false);
    return nullptr;
}

/**
 * get the pattern block that matches the input block id in this molecule
 */
static t_pack_pattern_block* get_atom_pattern_block(const t_pack_molecule* molecule, const int block_id) {
    const auto root_block = molecule->pack_pattern->root_block;

    std::vector<bool> visited_blocks(molecule->num_blocks);

    std::queue<t_pack_pattern_block*> pattern_block_queue;
    pattern_block_queue.push(root_block);

    // do breadth first search to find the block that matches block_id
    while (!pattern_block_queue.empty()) {
        auto pattern_block = pattern_block_queue.front();
        pattern_block_queue.pop();

        // ignore if a nullptr or is already visited
        if (!pattern_block || visited_blocks[pattern_block->block_id])
            continue;

        if (pattern_block->block_id == block_id)
            return pattern_block;

        visited_blocks[pattern_block->block_id] = true;

        auto block_connections = pattern_block->connections;

        // add all the blocks in the list of connections to the queue
        while (block_connections) {
            pattern_block_queue.push(block_connections->from_block);
            pattern_block_queue.push(block_connections->to_block);
            block_connections = block_connections->next;
        }
    }

    // this block is in this molecule
    // so it should be found
    VTR_ASSERT(false);
    return nullptr;
}

static void print_pack_molecules(const char* fname,
                                 const t_pack_patterns* list_of_pack_patterns,
                                 const int num_pack_patterns,
                                 const t_pack_molecule* list_of_molecules) {
    int i;
    FILE* fp;
    const t_pack_molecule* list_of_molecules_current;
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
                if (!list_of_molecules_current->atom_block_ids[i]) {
                    fprintf(fp, "\tpattern index %d: empty \n", i);
                } else {
                    fprintf(fp, "\tpattern index %d: atom block %s",
                            i,
                            atom_ctx.nlist.block_name(list_of_molecules_current->atom_block_ids[i]).c_str());
                    if (list_of_molecules_current->pack_pattern->root_block->block_id == i) {
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
    for (i = 0; i < device_ctx.num_block_types; i++) {
        cost = UNDEFINED;
        current = get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(blk_id, device_ctx.block_types[i].pb_graph_head, &cost);
        if (cost != UNDEFINED) {
            if (best_cost == UNDEFINED || best_cost > cost) {
                best_cost = cost;
                best = current;
            }
        }
    }

    if (!best) {
        auto& atom_ctx = g_vpr_ctx.atom();
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Failed to find any location to pack primitive of type '%s' in architecture",
                        atom_ctx.nlist.block_model(blk_id)->name);
    }

    return best;
}

static t_pb_graph_node* get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(const AtomBlockId blk_id, t_pb_graph_node* curr_pb_graph_node, float* cost) {
    t_pb_graph_node *best, *cur;
    float cur_cost, best_cost;
    int i, j;

    best = nullptr;
    best_cost = UNDEFINED;
    if (curr_pb_graph_node == nullptr) {
        return nullptr;
    }

    if (curr_pb_graph_node->pb_type->blif_model != nullptr) {
        if (primitive_type_feasible(blk_id, curr_pb_graph_node->pb_type)) {
            cur_cost = compute_primitive_base_cost(curr_pb_graph_node);
            if (best_cost == UNDEFINED || best_cost > cur_cost) {
                best_cost = cur_cost;
                best = curr_pb_graph_node;
            }
        }
    } else {
        for (i = 0; i < curr_pb_graph_node->pb_type->num_modes; i++) {
            for (j = 0; j < curr_pb_graph_node->pb_type->modes[i].num_pb_type_children; j++) {
                *cost = UNDEFINED;
                cur = get_expected_lowest_cost_primitive_for_atom_block_in_pb_graph_node(blk_id, &curr_pb_graph_node->child_pb_graph_nodes[i][j][0], cost);
                if (cur != nullptr) {
                    if (best == nullptr || best_cost > *cost) {
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
static int compare_pack_pattern(const t_pack_patterns* pattern_a, const t_pack_patterns* pattern_b) {
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
static AtomBlockId find_new_root_atom_for_chain(const AtomBlockId blk_id, const t_pack_patterns* list_of_pack_pattern, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    AtomBlockId new_root_blk_id;
    t_pb_graph_pin* root_ipin;
    t_pb_graph_node* root_pb_graph_node;

    VTR_ASSERT(list_of_pack_pattern->is_chain);
    VTR_ASSERT(list_of_pack_pattern->chain_root_pins.size());

    root_ipin = list_of_pack_pattern->chain_root_pins[0][0];
    root_pb_graph_node = root_ipin->parent_node;

    if (!primitive_type_feasible(blk_id, root_pb_graph_node->pb_type)) {
        return AtomBlockId::INVALID();
    }

    // find the block id of the atom block driving the input of this block
    AtomBlockId driver_blk_id = get_adder_driver_block(blk_id, list_of_pack_pattern, atom_molecules);

    // if there is no driver block for this net
    // then it is the furthest up the chain
    if (!driver_blk_id) {
        return blk_id;
    }

    // didn't find furthest atom up the chain, keep searching further up the chain
    new_root_blk_id = find_new_root_atom_for_chain(driver_blk_id, list_of_pack_pattern, atom_molecules);

    if (!new_root_blk_id) {
        return blk_id;
    } else {
        return new_root_blk_id;
    }
}

static AtomBlockId get_adder_driver_block(const AtomBlockId block_id, const t_pack_patterns* pack_pattern, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    const auto& atom_ctx = g_vpr_ctx.atom();

    const auto cin_pin = pack_pattern->chain_root_pins[0][0];
    const auto cin_model = cin_pin->port->model_port;
    const auto block_pb_graph_node = cin_pin->parent_node;
    const auto block_pb_type = block_pb_graph_node->pb_type;

    // checked all ports except for cin and no port is driven by an adder // check the cin port then
    // Steps for finding the driving adder of this adder atom
    // 1) Check if the adder is driven by another adder from the cin port.
    //      a) If this driver adder is a dummy adder that starts the adder chain (adder with two inputs connected to ground and cin unconnected),
    //         check the input ports of block_id first to be able to go back to the first addition in the adder tree.
    //      b) If this driver adder is not a dummy adder, return it.
    // find the driver of this atom from the cin port
    auto driver_id = atom_ctx.nlist.find_atom_pin_driver(block_id, cin_model, cin_pin->pin_number);
    AtomBlockId dummy_adder = AtomBlockId::INVALID();
    // if this atom is driven by another adder from the cin port and this adder is not in a molecule yet
    if (driver_id && primitive_type_feasible(driver_id, block_pb_type) && atom_molecules.find(driver_id) == atom_molecules.end()) {
        // if this driver adder is not the very first adder in the chain or the first adder but is driven by a ground connection
        if (atom_ctx.nlist.find_atom_pin_driver(driver_id, cin_model, cin_pin->pin_number))
            return driver_id;
        else
            dummy_adder = driver_id;
    }

    // Either this adder is not driven by another adder from the cin port or is driven by a dummy adder, therefore we need to
    // check ports a and b of the adder first to reach the top of the adder tree.
    for (int iport = 0; iport < block_pb_graph_node->num_input_ports; iport++) {
        for (int ipin = 0; ipin < block_pb_graph_node->num_input_pins[iport]; ipin++) {
            const auto& pin = block_pb_graph_node->input_pins[iport][ipin];
            if (pin.port->model_port != cin_model) {
                auto input_driver_id = atom_ctx.nlist.find_atom_pin_driver(block_id, pin.port->model_port, pin.pin_number);
                if (input_driver_id && primitive_type_feasible(input_driver_id, block_pb_type) && atom_molecules.find(input_driver_id) == atom_molecules.end()) {
                    return input_driver_id;
                }
            }
        }
    }

    // If this adder is driven by a dummy adder and no other adder is driving this one
    // from a and b ports then the dummy adder is the very firs
    return dummy_adder;
}

/**
 * This function takes an input pin to a root (has no parent block) pb_graph_node
 * an returns a vector of all the output pins that are reachable from this input
 * pin and have the same packing pattern
 */

static std::vector<t_pb_graph_pin*> find_end_of_path(t_pb_graph_pin* input_pin, int pattern_index) {
    // Enforce some constraints on the function

    // 1) the start of the path should be at the input of the root block
    VTR_ASSERT(input_pin->is_root_block_pin());

    // 2) this pin is an input pin to the root block
    VTR_ASSERT(input_pin->num_input_edges == 0);

    // create a queue of pin pointers for the breadth first search
    std::queue<t_pb_graph_pin*> pins_queue;

    // add the input pin to the queue
    pins_queue.push(input_pin);

    // found reachable output pins
    std::vector<t_pb_graph_pin*> reachable_pins;

    // do breadth first search till all connected
    // pins are explored
    while (!pins_queue.empty()) {
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

    return reachable_pins;
}

static void expand_search(const t_pb_graph_pin* input_pin, std::queue<t_pb_graph_pin*>& pins_queue, const int pattern_index) {
    // If not a primitive input pin (has output edges)
    // -----------------------------------------------

    // iterate over all output edges at this pin
    for (int iedge = 0; iedge < input_pin->num_output_edges; iedge++) {
        const auto& pin_edge = input_pin->output_edges[iedge];
        // if this edge is not anotated with this pattern and its pattern cannot be inferred, ignore it.
        if (!pin_edge->annotated_with_pattern(pattern_index) && !pin_edge->infer_pattern) {
            continue;
        }

        // this edge either matched the pack pattern or its pack pattern could be inferred
        // iterate over all the pins of that edge and add them to the pins_queue
        for (int ipin = 0; ipin < pin_edge->num_output_pins; ipin++) {
            pins_queue.push(pin_edge->output_pins[ipin]);
        }

    } // End for pin edges

    // If a primitive input pin
    // ------------------------

    // if this is an input pin to a primitive, it won't have
    // output edges so the previous for loop won't be entered
    if (input_pin->is_primitive_pin() && input_pin->num_output_edges == 0) {
        // iterate over the output ports of the primitive
        const auto& pin_pb_graph_node = input_pin->parent_node;
        for (int iport = 0; iport < pin_pb_graph_node->num_output_ports; iport++) {
            // iterate over the pins of each port
            const auto& port_pins = pin_pb_graph_node->num_output_pins[iport];
            for (int ipin = 0; ipin < port_pins; ipin++) {
                // add primitive output pins to pins_queue to be explored
                pins_queue.push(&pin_pb_graph_node->output_pins[iport][ipin]);
            }
        }
    }

    // If this is a root block output pin
    // ----------------------------------

    // No expansion will happen in this case
}

/**
 *  This function takes a chain pack pattern and a root pb_block
 *  containing this pattern. Then searches for all the input pins of this
 *  pb_block that are annotated with this pattern. The function then
 *  identifies whether those inputs represent different starting point for
 *  this pattern or are all required for building this pattern.
 *
 *  If this inputs represent different starting point for this pattern, it
 *  means that in this pb_block there exist multiple chains that are exactly
 *  the same. For example an architecture that has two separate adder chains
 *  behaving exactly the same but are totally separate from each other.
 *
 *                        cin[0] cin[1]
 *                       ---|------|---
 *                       | ---    --- |
 *                       | | |    | |<|---- Full Adder
 *                       | ---    --- |
 *   Pb_block ---------> |  |      |<-|---- Second Adder chain
 *                       |  .      .  |
 *                       |  .      .  |
 *                       |  |<-----|--|---- First Adder chain
 *                       | ---    --- |
 *                       | | |    | | |
 *                       | ---    --- |
 *                       ---|------|---
 *                       cout[0] cout[1]
 *
 *  In this case, the chain_root_pin array of the pack pattern is updated
 *  with all the pin that represent a starting point for this pattern.
 */
static void find_all_equivalent_chains(t_pack_patterns* chain_pattern, const t_pb_graph_node* root_block) {
    // this vector will be updated with all root_block input
    // pins that are annotated with this chain pattern
    std::vector<t_pb_graph_pin*> chain_input_pins;

    // iterate over all the input pins of the root_block and populate
    // the chain_input_pins vector
    for (int iports = 0; iports < root_block->num_input_ports; iports++) {
        for (int ipins = 0; ipins < root_block->num_input_pins[iports]; ipins++) {
            auto& input_pin = root_block->input_pins[iports][ipins];
            for (int iedge = 0; iedge < input_pin.num_output_edges; iedge++) {
                if (input_pin.output_edges[iedge]->belongs_to_pattern(chain_pattern->index)) {
                    chain_input_pins.push_back(&input_pin);
                }
            }
        }
    }

    // if this chain has only one cluster input, then
    // there is no need to proceed with the search
    if (chain_input_pins.size() == 1) {
        update_chain_root_pins(chain_pattern, chain_input_pins);
        chain_pattern->chain_exit_pins.push_back(find_chain_exit_pin(chain_input_pins[0], chain_pattern->index));
        return;
    }

    // find the root block output pins reachable when starting from the chain_input_pins
    // found before following the edges that are annotated with the given pack_pattern
    std::vector<std::vector<t_pb_graph_pin*>> reachable_pins;

    for (const auto& pin_ptr : chain_input_pins) {
        auto reachable_output_pins = find_end_of_path(pin_ptr, chain_pattern->index);
        // find the chain exit pin of this chain input pin
        auto chain_exit_pin = find_chain_exit_pin(pin_ptr, chain_pattern->index);
        // sort the reachable output pins to compare them later using set_intersection
        std::stable_sort(reachable_output_pins.begin(), reachable_output_pins.end());
        reachable_pins.push_back(reachable_output_pins);
        // update the chain exit pins array
        chain_pattern->chain_exit_pins.push_back(chain_exit_pin);
    }

    // Search for intersections between reachable pins. Intersection
    // between reachable indicates that found chain_input_pins
    // represent a single chain pattern and not multiple similar
    // chain patterns with multiple starting locations.
    std::vector<t_pb_graph_pin*> intersection;
    for (size_t i = 0; i < reachable_pins.size() - 1; i++) {
        for (size_t j = 1; j < reachable_pins.size(); j++) {
            std::set_intersection(reachable_pins[i].begin(), reachable_pins[i].end(),
                                  reachable_pins[j].begin(), reachable_pins[j].end(),
                                  std::back_inserter(intersection));
            if (intersection.size()) break;
        }
        if (intersection.size()) break;
    }

    // if there are no intersections between the reachable pins,
    // this each input pin represents a separate chain of type
    // chain_pattern. Else, they are all representing the same
    // chain.
    if (intersection.empty()) {
        // update the chain_root_pin array of the chain_pattern
        // with all the possible starting points of the chain.
        update_chain_root_pins(chain_pattern, chain_input_pins);
    }
}

/**
 *  This function takes a chain pack pattern and a vector of pin
 *  pointers that represent the root pb block input pins that can connect
 *  a chain to the previous pb block. The function uses the pin pointers
 *  to find the primitive input pin connected to them and updates
 *  the chain_root_pin array with this those pointers
 *  Side Effect: Updates the chain_root_pins array of the input chain_pattern
 */
static void update_chain_root_pins(t_pack_patterns* chain_pattern,
                                   const std::vector<t_pb_graph_pin*>& chain_input_pins) {
    std::vector<std::vector<t_pb_graph_pin*>> primitive_input_pins;

    for (const auto pin_ptr : chain_input_pins) {
        std::vector<t_pb_graph_pin*> connected_primitive_pins;
        get_all_connected_primitive_pins(pin_ptr, connected_primitive_pins);
        primitive_input_pins.push_back(connected_primitive_pins);
    }

    chain_pattern->chain_root_pins = primitive_input_pins;
}

/**
 *  This function takes a pin as an input an does a depth first search on all the output edges
 *  of this pin till it finds all the primitive input pins connected to this pin. For example,
 *  if the input pin given to this function is the Cin pin of the cluster. This pin will return
 *  the Cin pin of all the adder primitives connected to this pin. Which is for typical architectures
 *  will be only one pin connected to the very first adder in the cluster.
 */
static void get_all_connected_primitive_pins(const t_pb_graph_pin* cluster_input_pin, std::vector<t_pb_graph_pin*>& connected_primitive_pins) {
    for (int iedge = 0; iedge < cluster_input_pin->num_output_edges; iedge++) {
        const auto& output_edge = cluster_input_pin->output_edges[iedge];
        for (int ipin = 0; ipin < output_edge->num_output_pins; ipin++) {
            if (output_edge->output_pins[ipin]->is_primitive_pin()) {
                connected_primitive_pins.push_back(output_edge->output_pins[ipin]);
            } else {
                get_all_connected_primitive_pins(output_edge->output_pins[ipin], connected_primitive_pins);
            }
        }
    }

    VTR_ASSERT(connected_primitive_pins.size());
}

/**
 *  Find the next primitive input pin connected to the given cluster_input_pin.
 *  Following edges that are annotated with pack_pattern index
 */
static t_pb_graph_pin* get_connected_primitive_input_pin(const t_pb_graph_pin* cluster_input_pin, const int pack_pattern) {
    for (int iedge = 0; iedge < cluster_input_pin->num_output_edges; iedge++) {
        const auto& output_edge = cluster_input_pin->output_edges[iedge];
        // if edge is annotated with pack pattern or its pack pattern could be inferred
        if (output_edge->annotated_with_pattern(pack_pattern) || output_edge->infer_pattern) {
            for (int ipin = 0; ipin < output_edge->num_output_pins; ipin++) {
                if (output_edge->output_pins[ipin]->is_primitive_pin()) {
                    return output_edge->output_pins[ipin];
                }
                return get_connected_primitive_input_pin(output_edge->output_pins[ipin], pack_pattern);
            }
        }
    }

    // primitive input pin should always
    // be found when using this function
    VTR_ASSERT(false);
    return nullptr;
}

/**
 *  Find the previous primitive output pin connected to the given cluster_output_pin.
 *  Following edges that are annotated with pack_pattern index
 */
static t_pb_graph_pin* get_connected_primitive_output_pin(const t_pb_graph_pin* cluster_output_pin, const int pack_pattern) {
    for (int iedge = 0; iedge < cluster_output_pin->num_input_edges; iedge++) {
        const auto& input_edge = cluster_output_pin->input_edges[iedge];
        // if edge is annotated with pack pattern or its pack pattern could be inferred
        if (input_edge->annotated_with_pattern(pack_pattern) || input_edge->infer_pattern) {
            for (int ipin = 0; ipin < input_edge->num_input_pins; ipin++) {
                if (input_edge->input_pins[ipin]->is_primitive_pin()) {
                    return input_edge->input_pins[ipin];
                }
                return get_connected_primitive_output_pin(input_edge->input_pins[ipin], pack_pattern);
            }
        }
    }

    // primitive output pin should always
    // be found when using this function
    VTR_ASSERT(false);
    return nullptr;
}

/**
 * This function initializes the chain info data structure of the molecule.
 * If this is the furthest molecule up the chain, the chain_info data
 * structure is created. Otherwise, the input pack_molecule is set to
 * point to the same chain_info of the molecule feeding it
 *
 * Limitation: This function assumes that the molecules of a chain are
 * created and fed to this function in order. Meaning the first molecule
 * fed to the function should be the furthest molecule up the chain.
 * The second one should should be the molecule directly after that one
 * and so on.
 */
static void init_molecule_chain_info(const AtomBlockId blk_id, t_pack_molecule* molecule, const std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules) {
    // the input molecule to this function should have a pack
    // pattern assigned to it and the input block should be valid
    VTR_ASSERT(molecule->pack_pattern && blk_id);

    auto& atom_ctx = g_vpr_ctx.atom();

    auto root_ipin = molecule->pack_pattern->chain_root_pins[0][0];
    auto model_pin = root_ipin->port->model_port;
    auto pin_bit = root_ipin->pin_number;

    // find the atom driving the chain input pin of this atom
    auto driver_atom_id = atom_ctx.nlist.find_atom_pin_driver(blk_id, model_pin, pin_bit);

    // find the molecule this driver atom is mapped to
    auto itr = atom_molecules.find(driver_atom_id);

    // if this is the first molecule to be created for this chain
    // initialize the chain info data structure. This is the case
    // if either there is no driver to the block input pin or
    // if the driver is not part of a molecule
    if (!driver_atom_id || itr == atom_molecules.end()) {
        // allocate chain info
        molecule->chain_info = std::make_shared<t_chain_info>();
        molecule->chain_info->chain_id = 0;
        // this is not the first molecule to be created for this chain
    } else {
        // molecule driving blk_id
        auto prev_molecule = itr->second;
        // molecule should have chain_info associated with it
        VTR_ASSERT(prev_molecule && prev_molecule->chain_info);
        // this molecule is now known to belong to a long chain
        prev_molecule->chain_info->is_long_chain = true;
        // this new molecule should share the same chain_info
        molecule->chain_info = prev_molecule->chain_info;
        // if the two molecules are of different types
        if (prev_molecule->pack_pattern->chain_root_pins.size() < molecule->pack_pattern->chain_root_pins.size()) {
            molecule->chain_info->chain_id = get_forced_chain_id(molecule, prev_molecule, driver_atom_id);
        }
    }
}

static int get_forced_chain_id(t_pack_molecule* molecule, const t_pack_molecule* prev_molecule, const AtomBlockId driver_block_id) {
    t_pb_graph_node* driver_pb_graph_node = get_driver_pb_graph_node(prev_molecule, driver_block_id);

    VTR_ASSERT(driver_pb_graph_node);

    const auto& chain_exit_pins = molecule->pack_pattern->chain_exit_pins;
    // get the model of the cout port of the adder primitive
    const auto cout_port_model = chain_exit_pins[0]->port->model_port;

    for (int iport = 0; iport < driver_pb_graph_node->num_output_ports; iport++) {
        for (int ipin = 0; ipin < driver_pb_graph_node->num_output_pins[iport]; ipin++) {
            const auto& pin = driver_pb_graph_node->output_pins[iport][ipin];
            if (pin.port->model_port == cout_port_model) {
                for (size_t chain_id = 0; chain_id < chain_exit_pins.size(); chain_id++) {
                    // architecture specific hack
                    if (pin.parent_node->placement_index == chain_exit_pins[chain_id]->parent_node->placement_index)
                        return chain_id;
                }
            }
        }
    }

    VTR_ASSERT(false);
    return -1;
}

/**
 * This function prints all the starting points of the carry chains in the architecture
 */
static void print_chain_starting_points(t_pack_patterns* chain_pattern) {
    const auto& chain_root_pins = chain_pattern->chain_root_pins;

    VTR_LOGV(chain_root_pins.size() > 1, "\nThere are %zu independent chains for chain pattern \"%s\":\n",
             chain_pattern->chain_root_pins.size(), chain_pattern->name);
    VTR_LOGV(chain_root_pins.size() == 1, "\nThere is one chain in this architecture called \"%s\" with the following starting points:\n", chain_pattern->name);

    size_t chainId = 0;
    for (const auto& chain : chain_root_pins) {
        VTR_LOGV(chain_root_pins.size() > 1 && chain.size() > 1, "\n There are %zu starting points for chain id #%zu:\n", chain.size(), chainId++);
        VTR_LOGV(chain_root_pins.size() > 1 && chain.size() == 1, "\n There is 1 starting point for chain id #%zu:\n", chainId++);
        for (const auto& pin_ptr : chain) {
            VTR_LOG("\t%s\n", pin_ptr->to_string().c_str());
        }
    }

    VTR_LOG("\n");
}

/**
 * This function takes the input pin starting a chain (Cin of the root block) and finds the
 * the Cout pin of the last adder primitve of the chain.
 */
static t_pb_graph_pin* find_chain_exit_pin(t_pb_graph_pin* input_pin, int pattern_index) {
    VTR_ASSERT(input_pin->num_output_edges == 1);
    VTR_ASSERT(input_pin->output_edges[0]->annotated_with_pattern(pattern_index));

    auto first_cin_pin = get_connected_primitive_input_pin(input_pin, pattern_index);
    // pointer to the port model of the cin port of the adder primitive
    const auto cin_port_model = first_cin_pin->port->model_port;

    // create a queue of pin pointers for the breadth first search
    std::queue<t_pb_graph_pin*> pins_queue;

    // add the input pin to the queue
    pins_queue.push(first_cin_pin);

    // do breadth first search till all
    // connected pins are explored
    while (!pins_queue.empty()) {
        // get the first pin in the queue
        auto current_pin = pins_queue.front();

        // remove pin from queue
        pins_queue.pop();

        // if this is a primitive input pin and it's not a cin port, ignore pin
        // since we are only searching along the path of the chain ports
        if (current_pin->is_primitive_pin()
            && current_pin->port->type == IN_PORT
            && current_pin->port->model_port != cin_port_model) {
            continue;
        }

        // expand search from current pin
        expand_search(current_pin, pins_queue, pattern_index);

        // if this is an output pin of a root block then its connected
        // to the last cout of the chain. Return the connected primtive pin.
        if (current_pin->is_root_block_pin()
            && current_pin->num_output_edges == 0) {
            return get_connected_primitive_output_pin(current_pin, pattern_index);
        }
    }

    // Exit chain pin should be found
    VTR_ASSERT(false);
    return nullptr;
}

/**
 * This function returns true is this molecule is a packed molecule
 * that has hierarchical structure. For example, an adder that is feeding
 * another adder throught the sumout port.
 */
static bool molecule_is_hierarchical(const t_pack_molecule* molecule) {
    // assume that only chained molecules can be hierarchical
    if (!molecule->is_chain())
        return false;

    const auto cout_pin_model = molecule->pack_pattern->chain_exit_pins[0]->port->model_port;
    const auto root_block = molecule->pack_pattern->root_block;
    auto connection = root_block->connections;

    while (connection) {
        if (connection->from_block == root_block && connection->from_pin->port->model_port != cout_pin_model && connection->from_pin->parent_node->pb_type == connection->to_pin->parent_node->pb_type)
            return true;
        connection = connection->next;
    }

    return false;
}
