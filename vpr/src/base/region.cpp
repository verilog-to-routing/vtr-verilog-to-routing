#include "region.h"

/// @brief sentinel value for indicating that a subtile has not been specified
constexpr int NO_SUBTILE = -1;

Region::Region() {
    sub_tile = NO_SUBTILE;

    //default rect for a region is (-1, -1, -1, -1)
    //these values indicate an empty rectangle, they are set as default values to help catch uninitialized use
    region_bounds.set_xmin(-1);
    region_bounds.set_ymin(-1);
    region_bounds.set_xmax(-1);
    region_bounds.set_ymax(-1);
}

vtr::Rect<int> Region::get_region_rect() {
    return region_bounds;
}

void Region::set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax) {
    region_bounds.set_xmin(_xmin);
    region_bounds.set_xmax(_xmax);
    region_bounds.set_ymin(_ymin);
    region_bounds.set_ymax(_ymax);
}

int Region::get_sub_tile() {
    return sub_tile;
}

void Region::set_sub_tile(int _sub_tile) {
    sub_tile = _sub_tile;
}

bool Region::locked() {
    return region_bounds.xmin() == region_bounds.xmax() && region_bounds.ymin() == region_bounds.ymax() && sub_tile != NO_SUBTILE;
}

bool Region::empty() {
    return region_bounds.empty();
}

bool do_regions_intersect(Region r1, Region r2) {
    bool intersect = true;

    vtr::Rect<int> r1_rect = r1.get_region_rect();
    vtr::Rect<int> r2_rect = r2.get_region_rect();
    vtr::Rect<int> intersect_rect;

    intersect_rect = intersection(r1_rect, r2_rect);

    /**
     * if the intersection rectangle is empty or the subtile of the two regions does not match,
     * the regions do not intersect
     */
    if (intersect_rect.empty() || r1.get_sub_tile() != r2.get_sub_tile()) {
        return intersect = false;
    }

    return intersect;
}

Region intersection(Region r1, Region r2) {
    Region intersect;

    /**
     * If the subtiles of the two regions don't match, there is no intersection.
     * If they do match, the intersection function if used to get the overlap of the two regions' rectangles.
     * If there is no overlap, an empty rectangle will be returned.
     */
    if (r1.get_sub_tile() == r2.get_sub_tile()) {
        intersect.set_sub_tile(r1.get_sub_tile());
        vtr::Rect<int> r1_rect = r1.get_region_rect();
        vtr::Rect<int> r2_rect = r2.get_region_rect();
        vtr::Rect<int> intersect_rect;

        intersect_rect = intersection(r1_rect, r2_rect);

        intersect.set_region_rect(intersect_rect.xmin(), intersect_rect.ymin(), intersect_rect.xmax(), intersect_rect.ymax());
    }

    return intersect;
}
