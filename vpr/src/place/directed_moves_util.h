#ifndef VPR_DIRECTED_MOVES_UTIL_H
#define VPR_DIRECTED_MOVES_UTIL_H

#include "globals.h"
#include "timing_place.h"
#include "move_generator.h"

void calculate_centroid_loc(ClusterBlockId b_from, bool timing_weights, t_pl_loc& centroid, const PlacerCriticalities* criticalities);

#endif
