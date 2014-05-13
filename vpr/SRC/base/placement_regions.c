#include "placement_regions.h"

#include <limits>

bool placement_region_is_empty(s_placement_region *p) {
  return (p->xlow == -1 && p->xhigh == -1 && p->ylow == -1 && p->yhigh == -1);
}

bool placement_region_is_universe(s_placement_region *p) {
  return (p->xlow == 0 && p->xhigh == std::numeric_limits<int>::max() && p->ylow == 0 && p->yhigh == std::numeric_limits<int>::max());
}

void placement_region_init_universe(s_placement_region *p) {
  p->xlow = 0;
  p->xhigh = std::numeric_limits<int>::max();
  p->ylow = 0;
  p->yhigh = std::numeric_limits<int>::max();
}


void placement_region_init_empty(s_placement_region *p) {
  p->xlow = -1;
  p->xhigh = -1;
  p->ylow = -1;
  p->yhigh = -1;
}

struct s_placement_region placement_region_intersection(s_placement_region *a, s_placement_region *b) {
  struct s_placement_region result;
  placement_region_init_empty(&result);

  /* Check that the two rectangles overlap. */

  /* One is to the left (or right) of the other. */
  if (a->xhigh < b->xlow || b->xhigh < a->xlow) return result;

  /* One is on above (or below) the other. */
  if (a->yhigh < b->ylow || b->yhigh < a->ylow) return result;

  result.xlow = std::max(a->xlow, b->xlow);
  result.xhigh = std::min(a->xhigh, b->xhigh);
  result.ylow = std::max(a->ylow, b->ylow);
  result.yhigh = std::min(a->yhigh, b->yhigh);

  return result;
}
