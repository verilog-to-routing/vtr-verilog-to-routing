#ifndef BUILD_SWITCHBLOCKS_H
#define BUILD_SWITCHBLOCKS_H

#include <map>
#include <vector>
#include "util.h"
#include "vpr_types.h"


/* allocate and build switch block permutation map */
t_sb_connection_map * alloc_and_load_switchblock_permutations( INP t_chan_details * chan_details_x, 
				INP t_chan_details * chan_details_y, INP int nx, INP int ny, 
				INP std::vector<t_switchblock_inf> switchblocks, 
				INP s_chan_width *nodes_per_chan, INP enum e_directionality directionality);

/* deallocates switch block connections sparse array */
void free_switchblock_permutations(INOUTP t_sb_connection_map *sb_conns, int nx, int ny);

#endif
