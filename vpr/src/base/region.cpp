#include "region.h"

//sentinel value for indicating that a subtile has not been specified
constexpr int NO_SUBTILE = -1;

Region::Region() {
    sub_tile = NO_SUBTILE;

    //default rect for a region is (-1, -1, -1, -1)
    region_bounds.set_xmin(-1);
    region_bounds.set_ymin(-1);
    region_bounds.set_xmax(-1);
    region_bounds.set_ymax(-1);
}

vtr::Rect<int> Region::get_region_rect() {
    return region_bounds;
}

int Region::get_xmin() {
    return region_bounds.xmin();
}

int Region::get_xmax() {
    return region_bounds.xmax();
}

int Region::get_ymin() {
    return region_bounds.ymin();
}

int Region::get_ymax() {
    return region_bounds.ymax();
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

bool Region::do_regions_intersect(Region region) {
    bool intersect = true;

    vtr::Rect<int> region_rect = region.get_region_rect();
    vtr::Rect<int> intersect_rect;

    intersect_rect = intersection(region_bounds, region_rect);

    /**if the intersection rectangle is empty or the subtile of the two regions does not match,
     * the regions do not intersect
     */
    if (intersect_rect.empty() || sub_tile != region.get_sub_tile()) {
        return intersect = false;
    }

    return intersect;
}

Region Region::regions_intersection(Region region) {
    Region intersect;

    //if the subtiles do not match, there is no intersection
    //so, the control will go straight to returning intersect, which will just be a region with an empty rectangle
    //if the subtiles do match, then there will be an intersection as long as the rectangles overlap
    //and the intersection will be found by the code in the if statement
    //if they do not overlap, the intersection rectangle will still return an empty rectangle
    if (sub_tile == region.get_sub_tile()) {
        intersect.set_sub_tile(sub_tile);
        vtr::Rect<int> region_rect = region.get_region_rect();
        vtr::Rect<int> intersect_rect;

        intersect_rect = intersection(region_bounds, region_rect);

        intersect.set_region_rect(intersect_rect.xmin(), intersect_rect.ymin(), intersect_rect.xmax(), intersect_rect.ymax());
    }

    return intersect;
}

bool Region::locked() {
    bool locked = false;

    if (region_bounds.xmin() != region_bounds.xmax()) {
        return locked;
    }

    if (region_bounds.ymin() != region_bounds.ymax()) {
        return locked;
    }

    if (sub_tile == NO_SUBTILE) {
        return locked;
    }

    return locked = true;
}

bool Region::empty() {
    return region_bounds.empty();
}
