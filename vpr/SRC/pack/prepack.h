/* 
 Prepacking: Group together technology-mapped netlist blocks before packing.  This gives hints to the packer on what groups of blocks to keep together during packing.
 Primary use 1) "Forced" packs (eg LUT+FF pair)
 2) Carry-chains
 */

#ifndef PREPACK_H
#define PREPACK_H
#include "arch_types.h"
#include "util.h"

t_pack_patterns *alloc_and_load_pack_patterns(OUTP int *num_packing_patterns);
void free_list_of_pack_patterns(INP t_pack_patterns *list_of_pack_patterns, INP int num_packing_patterns);

t_pack_molecule *alloc_and_load_pack_molecules(
		INP t_pack_patterns *list_of_pack_patterns,
		INP int num_packing_patterns, OUTP int *num_pack_molecule);

#endif
