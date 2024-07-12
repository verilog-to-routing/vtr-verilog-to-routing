
#include "region.h"

#include <algorithm>
#include <limits>

Region::Region(const vtr::Rect<int>& rect, int layer_num_min, int layer_num_max)
    : rect_(rect)
    , layer_range_({layer_num_min, layer_num_max})
    , sub_tile_(NO_SUBTILE) {}

Region::Region(const vtr::Rect<int>& rect, int layer_num)
    : Region(rect, layer_num, layer_num) {}

Region::Region(int x_min, int y_min, int x_max, int y_max, int layer_num_min, int layer_num_max)
    : rect_({x_min, y_min, x_max, y_max})
    , layer_range_({layer_num_min, layer_num_max})
    , sub_tile_(NO_SUBTILE) {}

Region::Region(int x_min, int y_min, int x_max, int y_max, int layer_num)
    : Region(x_min, y_min, x_max, y_max, layer_num, layer_num) {}

void Region::set_layer_range(std::pair<int, int> layer_range) {
    layer_range_ = layer_range;
}

void Region::set_rect(const vtr::Rect<int>& rect) {
    rect_ = rect;
}

Region::Region()
    : rect_({std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
             std::numeric_limits<int>::min(), std::numeric_limits<int>::min()})
    , layer_range_(0, 0)
    , sub_tile_(NO_SUBTILE) {}

int Region::get_sub_tile() const {
    return sub_tile_;
}

void Region::set_sub_tile(int sub_tile) {
    sub_tile_ = sub_tile;
}

bool Region::empty() const {
    const auto [layer_low, layer_high] = layer_range_;
    return (rect_.xmax() < rect_.xmin() || rect_.ymax() < rect_.ymin() ||
            layer_high < layer_low);
}

bool Region::is_loc_in_reg(t_pl_loc loc) const {
    const int loc_layer_num = loc.layer;
    const auto [layer_low, layer_high] = layer_range_;

    // the location is falls outside the layer range
    if (loc_layer_num > layer_high || loc_layer_num < layer_low) {
        return false;
    }

    const vtr::Point<int> loc_coord(loc.x, loc.y);

    //check that loc x and y coordinates are within region bounds
    bool in_rectangle = rect_.coincident(loc_coord);

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
    const vtr::Rect<int>& r1_rect = r1.get_rect();
    const vtr::Rect<int>& r2_rect = r2.get_rect();

    auto [r1_layer_low, r1_layer_high] = r1.get_layer_range();
    auto [r2_layer_low, r2_layer_high] = r2.get_layer_range();

    auto [intersect_layer_begin, intersect_layer_end] = std::make_pair(std::max(r1_layer_low, r2_layer_low),
                                                                                std::min(r1_layer_high, r2_layer_high));

    // check that the give layer range start from a lower layer and end at a higher or the same layer
    // negative layer means that the given Region object is an empty region
    if (intersect_layer_begin > intersect_layer_end || intersect_layer_begin < 0 || intersect_layer_end < 0) {
        return {};
    }

    Region intersect;
    intersect.set_layer_range({intersect_layer_begin, intersect_layer_end});
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
        intersect.set_rect(intersect_rect);
        return intersect;
    } else if (r1.get_sub_tile() == NO_SUBTILE && r2.get_sub_tile() != NO_SUBTILE) {
        intersect.set_sub_tile(r2.get_sub_tile());
        vtr::Rect<int> intersect_rect = intersection(r1_rect, r2_rect);
        intersect.set_rect(intersect_rect);
        return intersect;
    } else if (r1.get_sub_tile() != NO_SUBTILE && r2.get_sub_tile() == NO_SUBTILE) {
        intersect.set_sub_tile(r1.get_sub_tile());
        vtr::Rect<int> intersect_rect = intersection(r1_rect, r2_rect);
        intersect.set_rect(intersect_rect);
        return intersect;
    }

    // subtiles are not compatible
    return {};
}

void print_region(FILE* fp, const Region& region) {
    const auto [layer_begin, layer_end] = region.get_layer_range();
    fprintf(fp, "\tRegion: \n");
    fprintf(fp, "\tlayer range: [%d, %d]\n", layer_begin, layer_end);
    print_rect(fp, region.get_rect());
    fprintf(fp, "\tsubtile: %d\n\n", region.get_sub_tile());
}
