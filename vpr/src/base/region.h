#ifndef REGION_H
#define REGION_H

#include "globals.h"

/**
 * @file
 * @brief This file defines the Region class. The Region class stores the data for each constraint region.
 *        This includes the bounds of the region rectangle and the sub_tile bounds. To lock a block down to a specific location,
 *        when defining the region specify xmin = xmax, ymin = ymax, sub_tile_min = sub_tile_max
 *
 */

class Region {
  public:
    vtr::Rect get_region_rect();
    void set_region_rect(int _xmin, int _xmax, int _ymin, int _ymax);

    int get_sub_tile_min();
    int get_sub_tile_max();

    void set_sub_tile_min(int _sub_tile_min);
    void set_sub_tile_max(int _sub_tile_max);

  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds;   //xmin, xmax, ymin, ymax inclusive
    int sub_tile_min, sub_tile_max; //inclusive bounds
};

#endif /* REGION_H */
