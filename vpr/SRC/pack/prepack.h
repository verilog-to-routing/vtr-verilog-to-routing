/* 
 Prepacking: Group together technology-mapped netlist blocks before packing.  This gives hints to the packer on what groups of blocks to keep together during packing.
 Primary use 1) "Forced" packs (eg LUT+FF pair)
 2) Carry-chains
 */

#ifndef PREPACK_H
#define PREPACK_H
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "arch_types.h"
#include "vpr_types.h"

t_pack_patterns *alloc_and_load_pack_patterns(int *num_packing_patterns);
void free_list_of_pack_patterns(t_pack_patterns *list_of_pack_patterns, const int num_packing_patterns);

t_pack_molecule *alloc_and_load_pack_molecules(
		t_pack_patterns *list_of_pack_patterns,
        std::unordered_multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
		const int num_packing_patterns);

#endif
