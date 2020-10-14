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

//sentinel value for indicating that a subtile had not been specified
extern int NO_SUBTILE;

class Region {
  public:
    Region();

    //vtr::Rect get_region_rect();
    int get_xmin();
    int get_xmax();
    int get_ymin();
    int get_ymax();
    void set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax);

    int get_sub_tile();

    void set_sub_tile(int _sub_tile);

    //function to see if two regions intersect anywhere
    bool do_regions_intersect(Region region);

    //returns the coordinates of the intersection of two regions
    Region regions_intersection(Region region);

    //function to check whether a block is locked down to a specific x, y, subtile location
    //can be called by the placer to see if the block is locked down
    bool locked();

  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds; //xmin, ymin, xmax, ymax inclusive
    int sub_tile;                 //users will optionally select a subtile, will select if they want to lock down block to specific location
};

#endif /* REGION_H */
