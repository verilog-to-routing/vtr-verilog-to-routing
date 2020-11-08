#ifndef REGION_H
#define REGION_H

#include <vtr_geometry.h>

/**
 * @file
 * @brief This file defines the Region class. The Region class stores the data for each constraint region.
 *
 * This includes the x and y bounds of the region rectangle and its sub_tile. To lock a block down to a specific location,
 * when defining the region specify xmin = xmax, ymin = ymax, and specify a subtile value.
 *
 */

class Region {
  public:
    /**
     * @brief Constructor for the Region class, sets member variables to invalid values
     */
    Region();

    /**
     * @brief Accessor for the region's rectangle
     */
    vtr::Rect<int> get_region_rect();

    /**
     * @brief Mutator for the region's rectangle
     */
    void set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax);

    /**
     * @brief Accessor for the region's subtile
     */
    int get_sub_tile();

    /**
     * @brief Mutator for the region's subtile
     */
    void set_sub_tile(int _sub_tile);

    /**
     * @brief Return whether the region is empty, based on whether the region rectangle is empty
     */
    bool empty();

    /**
     * @brief Checks whether a block is locked down to a specific x, y, subtile location
     */
    bool locked();

  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds; ///< xmin, ymin, xmax, ymax inclusive
    int sub_tile;                 ///< users will optionally select a subtile
};

/**
 * @brief Returns whether two regions intersect
 *
 * Intersection is the area of overlap between the rectangles of two regions and the subtile value of the intersecting rectangle,
 * given that both regions have matching subtile values, or no subtiles assigned to them
 * The overlap is inclusive of the x and y boundaries of the rectangles
 *
 *   @param r1  One of the regions to check for intersection
 *   @param r2  One of the regions to check for intersection
 */
bool do_regions_intersect(Region r1, Region r2);

/**
 * @brief Returns the intersection of two regions
 *
 *   @param r1  One of the regions to intersect
 *   @param r2  One of the regions to intersect
 *
 */
Region intersection(Region r1, Region r2);

#endif /* REGION_H */
