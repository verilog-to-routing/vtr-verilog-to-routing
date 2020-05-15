/*
 * Prepacking: Group together technology-mapped netlist blocks before packing.  This gives hints to the packer on what groups of blocks to keep together during packing.
 * Primary use 1) "Forced" packs (eg LUT+FF pair)
 * 2) Carry-chains
 */

#ifndef PREPACK_H
#define PREPACK_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "arch_types.h"
#include "vpr_types.h"

std::vector<t_pack_patterns> alloc_and_load_pack_patterns();
void free_list_of_pack_patterns(std::vector<t_pack_patterns>& list_of_pack_patterns);
void free_pack_pattern(t_pack_patterns* pack_pattern);

t_pack_molecule* alloc_and_load_pack_molecules(t_pack_patterns* list_of_pack_patterns,
                                               std::multimap<AtomBlockId, t_pack_molecule*>& atom_molecules,
                                               std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                                               const int num_packing_patterns);

void free_pack_molecules(t_pack_molecule* list_of_pack_molecules);

#endif
