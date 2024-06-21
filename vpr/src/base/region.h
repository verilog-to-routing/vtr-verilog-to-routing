#ifndef REGION_H
#define REGION_H

#include "vtr_geometry.h"

#include "vpr_types.h"

/**
 * @class RegionRectCoord
 * @brief This class stores coordinates of a cubic floorplan region.
 */
struct RegionRectCoord {
    RegionRectCoord() = default;

    RegionRectCoord(const vtr::Rect<int>& rect, int layer_num_max, int layer_num_min);

    RegionRectCoord(const vtr::Rect<int>& rect, int layer_num);

    RegionRectCoord(int x_min, int y_min, int x_max, int y_max, int layer_num_min, int layer_num_max);

    RegionRectCoord(int x_min, int y_min, int x_max, int y_max, int layer_num);

    /// @brief Convert to a vtr::Rect
    const vtr::Rect<int>& get_rect() const {
        return rect_;
    }

    void set_rect(const vtr::Rect<int>& rect);

    vtr::Rect<int>& get_mutable_rect() {
        return rect_;
    }

    void set_layer_range(std::pair<int, int> layer_range);

    std::pair<int, int> get_layer_range() const {
        return layer_range_;
    }

    std::pair<int, int>& get_mutable_layer_range() {
        return layer_range_;
    }

    /// @brief Equality operator
    bool operator==(const RegionRectCoord& rhs) const {
        return (rect_ == rhs.rect_) &&
               (layer_range_.first == rhs.layer_range_.first) &&
               (layer_range_.second == rhs.layer_range_.second);
    }

  private:
    /**
     * Represents a rectangle in the x-y plane.
     * This rectangle is the projection of the cube represented by this class
     * onto the x-y plane.
     */
    vtr::Rect<int> rect_;

    /**
     * Represents the range of layers spanned by the projected rectangle (rect_).
     * layer_range_.first --> the lowest layer
     * layer_range.second --> the higher layer
     * This range is inclusive, meaning that the lowest and highest layers are
     * part of the floorplan region.
     */
    std::pair<int, int> layer_range_;
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
    const RegionRectCoord& get_region_bounds() const;

    /**
     * @brief Mutator for the region's rectangle
     */
    void set_region_bounds(const RegionRectCoord& rect_coord);

    /**
     * @brief Accessor for the region's subtile
     */
    int get_sub_tile() const;

    /**
     * @brief Mutator for the region's subtile
     */
    void set_sub_tile(int sub_tile);

    /**
     * @brief Return whether the region is empty (i. e. the region bounds rectangle
     * covers no area)
     */
    bool empty() const;

    /**
     * @brief Check if the location is in the region (at a valid x, y, subtile location within the region bounds, inclusive)
     * If the region has no subtile specified, then the location subtile does not have to match. If it does, the location
     * and region subtile must match. The location provided is assumed to be valid.
     *
     *   @param loc     The location to be checked
     */
    bool is_loc_in_reg(t_pl_loc loc) const;

    bool operator==(const Region& reg) const {
        return (reg.get_region_bounds() == region_bounds_ &&
                reg.get_sub_tile() == sub_tile_);
    }

  private:
    RegionRectCoord region_bounds_;
    int sub_tile_;                 ///< users will optionally select a subtile
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
//bool do_regions_intersect(Region r1, Region r2);

/**
 * @brief Returns the intersection of two regions
 *
 *   @param r1  One of the regions to intersect
 *   @param r2  One of the regions to intersect
 *
 */
Region intersection(const Region& r1, const Region& r2);

///@brief Used to print data from a Region
void print_region(FILE* fp, const Region& region);

namespace std {
template<>
struct hash<Region> {
    std::size_t operator()(const Region& reg) const noexcept {
        const RegionRectCoord& reg_bounds = reg.get_region_bounds();
        const vtr::Rect<int> rect = reg_bounds.get_rect();
        auto [layer_begin, layer_end] = reg_bounds.get_layer_range();

        std::size_t seed = std::hash<int>{}(rect.xmin());
        vtr::hash_combine(seed, rect.ymin());
        vtr::hash_combine(seed, rect.xmax());
        vtr::hash_combine(seed, rect.ymax());
        vtr::hash_combine(seed, layer_begin);
        vtr::hash_combine(seed, layer_end);
        vtr::hash_combine(seed, reg.get_sub_tile());
        return seed;
    }
};
} // namespace std

#endif /* REGION_H */
