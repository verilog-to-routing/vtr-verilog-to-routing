#ifndef REGION_H
#define REGION_H

#include <vtr_geometry.h>

/**
 * @file
 * @brief This file defines the Region class. The Region class stores the data for each constraint region.
 *        This includes the x and y bounds of the region rectangle and its sub_tile. To lock a block down to a specific location,
 *        when defining the region specify xmin = xmax, ymin = ymax, and specify a subtile value.
 *
 */

class Region {
  public:
    Region();

    vtr::Rect<int> get_region_rect();

    void set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax);

    int get_sub_tile();

    void set_sub_tile(int _sub_tile);

    //function to see if region is empty (checks if region rectangle is empty)
    bool empty();

    /**function to see if two regions intersect anywhere
     * Intersection is the area of overlap between the rectangles of two regions,
     * given that both regions have matching subtile values, or no subtiles assigned to them
     * The overlap is inclusive of the x and y boundaries of the rectangles
     */
    bool do_regions_intersect(Region region);

    //returns the coordinates of the rectangular intersection of two regions
    Region regions_intersection(Region region);

    //function to check whether a block is locked down to a specific x, y, subtile location
    //can be called by the placer to see if the block is locked down
    bool locked();

  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds; //xmin, ymin, xmax, ymax inclusive
    int sub_tile;                 //users will optionally select a subtile
};

#endif /* REGION_H */
