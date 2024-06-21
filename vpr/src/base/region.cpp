
#include "region.h"

#include <algorithm>
#include <limits>

RegionRectCoord::RegionRectCoord(const vtr::Rect<int>& rect, int layer_num_min, int layer_num_max)
    : rect_(rect)
    , layer_range_({layer_num_min, layer_num_max}) {}

RegionRectCoord::RegionRectCoord(const vtr::Rect<int>& rect, int layer_num)
    : RegionRectCoord(rect, layer_num, layer_num) {}

RegionRectCoord::RegionRectCoord(int x_min, int y_min, int x_max, int y_max, int layer_num_min, int layer_num_max)
    : rect_({x_min, y_min, x_max, y_max})
    , layer_range_({layer_num_min, layer_num_max}) {}

RegionRectCoord::RegionRectCoord(int x_min, int y_min, int x_max, int y_max, int layer_num)
    : RegionRectCoord(x_min, y_min, x_max, y_max, layer_num, layer_num) {}

void RegionRectCoord::set_layer_range(std::pair<int, int> layer_range) {
    layer_range_ = layer_range;
}

void RegionRectCoord::set_rect(const vtr::Rect<int>& rect) {
    rect_ = rect;
}

Region::Region()
    : region_bounds_({std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                      std::numeric_limits<int>::min(), std::numeric_limits<int>::min()},
                      -1, -1)    //these values indicate an empty rectangle
    , sub_tile_(NO_SUBTILE) {}

const RegionRectCoord& Region::get_region_bounds() const {
    return region_bounds_;
}

void Region::set_region_bounds(const RegionRectCoord& rect_coord) {
    region_bounds_ = rect_coord;
}

int Region::get_sub_tile() const {
    return sub_tile_;
}

void Region::set_sub_tile(int sub_tile) {
    sub_tile_ = sub_tile;
}

bool Region::empty() const {
    const vtr::Rect<int>& rect = region_bounds_.get_rect();
    const auto [layer_begin, layer_end] = region_bounds_.get_layer_range();

    return (rect.xmax() < rect.xmax() || rect.ymax() < rect.ymin() ||
            layer_begin < 0 || layer_end < 0 ||
            layer_end < layer_begin);
}

bool Region::is_loc_in_reg(t_pl_loc loc) const {
    const int loc_layer_num = loc.layer;
    const auto [layer_begin, layer_end] = region_bounds_.get_layer_range();
    const auto& rect = region_bounds_.get_rect();

    if (loc_layer_num > layer_end || loc_layer_num < layer_begin) {
        return false;
    }

    vtr::Point<int> loc_coord(loc.x, loc.y);

    //check that loc x and y coordinates are within region bounds
    bool in_rectangle = rect.coincident(loc_coord);

    //if a subtile is specified for the region, the location subtile should match
    if (in_rectangle && sub_tile_ == loc.sub_tile) {
        return true;
    }

    //if no subtile is specified for the region, it is enough for the location to be in the rectangle
    if (in_rectangle && sub_tile_ == NO_SUBTILE) {
        return true;
    }

    return false;
}

Region intersection(const Region& r1, const Region& r2) {
    const vtr::Rect<int>& r1_rect = r1.get_region_bounds().get_rect();
    const vtr::Rect<int>& r2_rect = r2.get_region_bounds().get_rect();

    auto [r1_layer_begin, r1_layer_end] = r1.get_region_bounds().get_layer_range();
    auto [r2_layer_begin, r2_layer_end] = r2.get_region_bounds().get_layer_range();

    auto [intersect_layer_begin, intersect_layer_end] = std::make_pair(std::max(r1_layer_begin, r2_layer_begin),
                                                                                std::min(r1_layer_end, r2_layer_end));

    if (intersect_layer_begin > intersect_layer_end || intersect_layer_begin < 0 || intersect_layer_end < 0) {
        return {};
    }

    Region intersect;
    RegionRectCoord region_bounds;
    region_bounds.set_layer_range({intersect_layer_begin, intersect_layer_end});
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
        vtr::Rect<int> intersect_rect = intersection(r1_rect, r2_rect);
        region_bounds.set_rect(intersect_rect);
        intersect.set_region_bounds(region_bounds);
        return intersect;
    } else if (r1.get_sub_tile() == NO_SUBTILE && r2.get_sub_tile() != NO_SUBTILE) {
        intersect.set_sub_tile(r2.get_sub_tile());
        vtr::Rect<int> intersect_rect = intersection(r1_rect, r2_rect);
        region_bounds.set_rect(intersect_rect);
        intersect.set_region_bounds(region_bounds);
        return intersect;
    } else if (r1.get_sub_tile() != NO_SUBTILE && r2.get_sub_tile() == NO_SUBTILE) {
        intersect.set_sub_tile(r1.get_sub_tile());
        vtr::Rect<int> intersect_rect = intersection(r1_rect, r2_rect);
        region_bounds.set_rect(intersect_rect);
        intersect.set_region_bounds(region_bounds);
        return intersect;
    }

    // subtile are not compatible
    return {};
}

void print_region(FILE* fp, const Region& region) {
    const RegionRectCoord& region_bounds = region.get_region_bounds();
    auto [layer_begin, layer_end] = region_bounds.get_layer_range();
    fprintf(fp, "\tRegion: \n");
    fprintf(fp, "\tlayer range: [%d, %d]\n", layer_begin, layer_end);
    print_rect(fp, region_bounds.get_rect());
    fprintf(fp, "\tsubtile: %d\n\n", region.get_sub_tile());
}
