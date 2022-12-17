#ifndef RR_TYPES_H
#define RR_TYPES_H
#include <vector>
#include "vtr_ndmatrix.h"

/* AA: This structure stores the track connections for each physical pin. Note that num_pins refers to the # of logical pins for a tile and 
 * we use the relative x and y location (0...width and 0...height of the tile) and the side of that unit tile to locate the physical pin. 
 * If pinloc[ipin][iwidth][iheight][side]==1 it exists there... 
 * The alloc_and_load_pin_to_track_map loads up the tracks that connect to each of the *PHYSICAL* pins. Thus, the last dimension of the matrix
 * goes from [0...Fc-1] where Fc is the actual Fc value for that pin. 
 *
 * The matrix should be accessed as follows as a result after allocation in rr_graph.cpp: alloc_pin_to_track_lookup (used by unidir and bidir)
 * [0..device_ctx.physical_tile_types.size()-1][0..num_pins-1][0..width][0..height][0..3][0..Fc-1] */

typedef std::vector<vtr::NdMatrix<std::vector<int>, 4>> t_pin_to_track_lookup;

/* AA: t_pin_to_track_lookup is alloacted first and is then converted to t_track_to_pin lookup by simply redefining the accessing order. 
 * As a result, the matrix should be accessed as follow as a result after allocation in rr_graph.cpp: alloc_track_to_pin_lookup (used by unidir and bidir)
 * [0..device_ctx.physical_tile_types.size()-1][0..max_chan_width-1][0..width][0..height][0..3] 
 * 
 * Note that when we modell different channels based on position not axis, we can't use this anymore and need to have a lookup for each grid location. */

typedef std::vector<vtr::NdMatrix<std::vector<int>, 4>> t_track_to_pin_lookup;

#endif
