#ifndef REGION_H
#define REGION_H

#include <vtr_geometry.h>
#include "vpr_types.h"

/**
 * @brief This class stores the data for each constraint region on a layer
 * @param xmin The minimum x coordinate of the region
 * @param ymin The minimum y coordinate of the region
 * @param xmax The maximum x coordinate of the region
 * @param ymax The maximum y coordinate of the region
 * @param layer_num The layer number of the region
 */
struct RegionRectCoord {
    RegionRectCoord() = default;
    RegionRectCoord(int _xmin, int _ymin, int _xmax, int _ymax, int _layer_num)
        : xmin(_xmin)
        , ymin(_ymin)
        , xmax(_xmax)
        , ymax(_ymax)
        , layer_num(_layer_num) {}

    RegionRectCoord(const vtr::Rect<int>& rect, int _layer_num)
        : xmin(rect.xmin())
        , ymin(rect.ymin())
        , xmax(rect.xmax())
        , ymax(rect.ymax())
        , layer_num(_layer_num) {}

    int xmin;
    int ymin;
    int xmax;
    int ymax;
    int layer_num;

    /// @brief Convert to a vtr::Rect
    vtr::Rect<int> get_rect() const {
        return vtr::Rect<int>(xmin, ymin, xmax, ymax);
    }

    /// @brief Equality operator
    bool operator==(const RegionRectCoord& rhs) const {
        vtr::Rect<int> lhs_rect(xmin, ymin, xmax, ymax);
        vtr::Rect<int> rhs_rect(rhs.xmin, rhs.ymin, rhs.xmax, rhs.ymax);
        return lhs_rect == rhs_rect
               && layer_num == rhs.layer_num;
    }
};

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
    RegionRectCoord get_region_rect() const;

    /**
     * @brief Mutator for the region's rectangle
     */
    void set_region_rect(const RegionRectCoord& rect_coord);

    /**
     * @brief Accessor for the region's layer number
     */
    int get_layer_num() const;

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
        return (reg.get_region_rect() == this->get_region_rect()
                && reg.get_sub_tile() == this->get_sub_tile()
                && reg.layer_num == this->layer_num);
    }

  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds; ///< xmin, ymin, xmax, ymax inclusive
    int layer_num;                ///< layer number of the region
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
        const auto region_coord = reg.get_region_rect();
        std::size_t seed = std::hash<int>{}(region_coord.xmin);
        vtr::hash_combine(seed, region_coord.ymin);
        vtr::hash_combine(seed, region_coord.xmax);
        vtr::hash_combine(seed, region_coord.ymax);
        vtr::hash_combine(seed, region_coord.layer_num);
        vtr::hash_combine(seed, reg.get_sub_tile());
        return seed;
    }
};
} // namespace std

#endif /* REGION_H */
