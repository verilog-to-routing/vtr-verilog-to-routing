#ifndef REGION_H
#define REGION_H

#include <vtr_geometry.h>
#include "vpr_types.h"

/**
 * @file
 * @brief This file defines the Region class. The Region class stores the data for each constraint region.
 *
 * This includes the x and y bounds of the region rectangle and its sub_tile. To lock a block down to a specific location,
 * when defining the region specify xmin = xmax, ymin = ymax, and specify a subtile value.
 *
 */

/// @brief sentinel value for indicating that a subtile has not been specified
constexpr int NO_SUBTILE = -1;

class Region {
  public:
    /**
     * @brief Constructor for the Region class, sets member variables to invalid values
     */
    Region();

    /**
     * @brief Accessor for the region's rectangle
     */
    vtr::Rect<int> get_region_rect() const;

    /**
     * @brief Mutator for the region's rectangle
     */
    void set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax);

    /**
     * @brief Accessor for the region's subtile
     */
    int get_sub_tile() const;

    /**
     * @brief Mutator for the region's subtile
     */
    void set_sub_tile(int _sub_tile);

    /**
     * @brief Return whether the region is empty (i. e. the region bounds rectangle
     * covers no area)
     */
    bool empty();

    /**
     * @brief Check if the location is in the region (at a valid x, y, subtile location within the region bounds, inclusive)
     * If the region has no subtile specified, then the location subtile does not have to match. If it does, the location
     * and region subtile must match. The location provided is assumed to be valid.
     *
     *   @param loc     The location to be checked
     */
    bool is_loc_in_reg(t_pl_loc loc);

    bool operator==(const Region& reg) const {
        return (reg.get_region_rect() == this->get_region_rect() && reg.get_sub_tile() == this->get_sub_tile());
    }

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
Region intersection(const Region& r1, const Region& r2);

///@brief Used to print data from a Region
void print_region(FILE* fp, Region region);

namespace std {
template<>
struct hash<Region> {
    std::size_t operator()(const Region& reg) const noexcept {
        vtr::Rect<int> rect = reg.get_region_rect();
        std::size_t seed = std::hash<int>{}(rect.xmin());
        vtr::hash_combine(seed, rect.ymin());
        vtr::hash_combine(seed, rect.xmax());
        vtr::hash_combine(seed, rect.ymax());
        vtr::hash_combine(seed, reg.get_sub_tile());
        return seed;
    }
};
} // namespace std

#endif /* REGION_H */
