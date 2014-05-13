#include "vpr_types.h"

bool placement_region_is_empty(s_placement_region *p);
bool placement_region_is_universe(s_placement_region *p);
void placement_region_init_universe(s_placement_region *p);
s_placement_region placement_region_intersection(s_placement_region *a, s_placement_region *b);
