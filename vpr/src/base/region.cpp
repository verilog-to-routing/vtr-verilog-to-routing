#include "region.h"

Region::Region() {
    sub_tile = NO_SUBTILE;

    //default rect for a region is (999, 999, -1, -1)
    //these values indicate an empty rectangle, they are set as default values to help catch uninitialized use
    region_bounds.set_xmin(999);
    region_bounds.set_ymin(999);
    region_bounds.set_xmax(-1);
    region_bounds.set_ymax(-1);
}

vtr::Rect<int> Region::get_region_rect() const {
    return region_bounds;
}

void Region::set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax) {
    region_bounds.set_xmin(_xmin);
    region_bounds.set_xmax(_xmax);
    region_bounds.set_ymin(_ymin);
    region_bounds.set_ymax(_ymax);
}

int Region::get_sub_tile() const {
    return sub_tile;
}

void Region::set_sub_tile(int _sub_tile) {
    sub_tile = _sub_tile;
}

bool Region::empty() {
    return (region_bounds.xmax() < region_bounds.xmin() || region_bounds.ymax() < region_bounds.ymin());
}

bool Region::is_loc_in_reg(t_pl_loc loc) {
    bool is_loc_in_reg = false;

    vtr::Point<int> loc_coord(loc.x, loc.y);

    //check that loc x and y coordinates are within region bounds
    bool in_rectangle = region_bounds.coincident(loc_coord);

    //if a subtile is specified for the region, the location subtile should match
    if (in_rectangle && sub_tile == loc.sub_tile) {
        is_loc_in_reg = true;
    }

    //if no subtile is specified for the region, it is enough for the location to be in the rectangle
    if (in_rectangle && sub_tile == NO_SUBTILE) {
        is_loc_in_reg = true;
    }

    return is_loc_in_reg;
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

Region intersection(const Region& r1, const Region& r2) {
    Region intersect;
    vtr::Rect<int> r1_rect = r1.get_region_rect();
    vtr::Rect<int> r2_rect = r2.get_region_rect();
    vtr::Rect<int> intersect_rect;

    /*
     * If the subtiles of two regions match (i.e. they both have no subtile specified, or the same subtile specified),
     * the regions are intersected. The resulting intersection region will have a rectangle that reflects their overlap,
     * (it will be empty if there is no overlap), and a subtile that is the same as that of both regions.
     *
     * If one of the two regions has a subtile specified, and the other does not, the regions are intersected.
     * The resulting intersection region will have a rectangle that reflects their overlap (it will be empty if there is
     * no overlap), and a subtile that is the same as the subtile of the region with the specific subtile assigned.
     *
     * If none of the above cases are true (i.e. the regions both have subtiles specified, but the subtiles do not
     * match each other) then the intersection is not performed and a region with an empty rectangle is returned.
     * This is because there can be no overlap in this case since the regions are constrained to two separate subtiles.
     */
    if (r1.get_sub_tile() == r2.get_sub_tile()) {
        intersect.set_sub_tile(r1.get_sub_tile());
        intersect_rect = intersection(r1_rect, r2_rect);
        intersect.set_region_rect(intersect_rect.xmin(), intersect_rect.ymin(), intersect_rect.xmax(), intersect_rect.ymax());

    } else if (r1.get_sub_tile() == NO_SUBTILE && r2.get_sub_tile() != NO_SUBTILE) {
        intersect.set_sub_tile(r2.get_sub_tile());
        intersect_rect = intersection(r1_rect, r2_rect);
        intersect.set_region_rect(intersect_rect.xmin(), intersect_rect.ymin(), intersect_rect.xmax(), intersect_rect.ymax());

    } else if (r1.get_sub_tile() != NO_SUBTILE && r2.get_sub_tile() == NO_SUBTILE) {
        intersect.set_sub_tile(r1.get_sub_tile());
        intersect_rect = intersection(r1_rect, r2_rect);
        intersect.set_region_rect(intersect_rect.xmin(), intersect_rect.ymin(), intersect_rect.xmax(), intersect_rect.ymax());
    }

    return intersect;
}

void print_region(FILE* fp, Region region) {
    fprintf(fp, "\tRegion: \n");
    print_rect(fp, region.get_region_rect());
    fprintf(fp, "\tsubtile: %d\n\n", region.get_sub_tile());
}
