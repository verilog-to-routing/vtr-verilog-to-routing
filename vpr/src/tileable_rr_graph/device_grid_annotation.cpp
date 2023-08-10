#include "device_grid_annotation.h"

/* Begin namespace openfpga */
namespace openfpga {

DeviceGridAnnotation::DeviceGridAnnotation(const DeviceGrid& grid, const bool& shrink) {
    alloc(grid);
    init(grid, shrink);
}

void DeviceGridAnnotation::alloc(const DeviceGrid& grid) {
    /* Allocate */
    borderline_types_.resize({grid.width(), grid.height()});
}

void DeviceGridAnnotation::init(const DeviceGrid& grid, const bool& shrink) {
    /* If shrink is not considered, perimeters are the borderlines */
    for (size_t ix = 0; ix < grid.width(); ++ix) {
        for (size_t iy = 0; iy < grid.height(); ++iy) {
            for (e_side side : SIDES) {
                borderline_types_[ix][iy][side] = is_empty_zone(grid, vtr::Point<size_t>(ix, iy), side, shrink);
            }
        }
    }
}

bool DeviceGridAnnotation::is_empty_zone(const DeviceGrid& grid, const vtr::Point<size_t>& coord, const e_side& side, const bool& shrink) const {
    bool empty_zone = true;
    /* With the given side, expand on the direction */
    if (side == TOP) {
        if (iy == grid.height() - 1) {
            return empty_zone;
        }
        if (shrink) {
            for (iy = coord.y() + 1; iy < grid.height(); ++iy) {
                if (!is_empty_type(grid.get_physical_type({coord.x(), iy, 0}))) {
                    empty_zone = false;
                    break;
                }
            }
        }
    } else if (side == RIGHT) {
        if (ix == grid.width() - 1) {
            return empty_zone;
        }
        if (shrink) {
            for (ix = coord.x() + 1; ix < grid.height(); ++ix) {
                if (!is_empty_type(grid.get_physical_type({ix, coord.y(), 0}))) {
                    empty_zone = false;
                    break;
                }
            }
        }
    } else if (side == BOTTOM) {
        if (iy == 0) {
            return empty_zone;
        }
        if (shrink) {
            for (iy = coord.y() - 1; iy >= 0; --iy) {
                if (!is_empty_type(grid.get_physical_type({coord.x(), iy, 0}))) {
                    empty_zone = false;
                    break;
                }
            }
        }
    } else if (side == LEFT) {
        if (ix == 0) {
            return empty_zone;
        }
        if (shrink) {
            for (ix = coord.x() - 1; ix >= 0; --ix) {
                if (!is_empty_type(grid.get_physical_type({ix, coord.y(), 0}))) {
                    empty_zone = false;
                    break;
                }
            }
        }
    }

    return empty_zone;
}

bool DeviceGridAnnotation::is_chanx_exist(const vtr::Point<size_t>& coord) const {
    if (coord.y() == borderline_types_.dim_size(2) - 1) {
        return false;
    }
    return !borderline_types[coord.x()][coord.y()][TOP] || !borderline_types[coord.x()][coord.y() + 1][BOTTOM];
}

bool DeviceGridAnnotation::is_chany_exist(const vtr::Point<size_t>& coord) const {
    if (coord.x() == borderline_types_.dim_size(1) - 1) {
        return false;
    }
    return !borderline_types[coord.x()][coord.y()][RIGHT] || !borderline_types[coord.x() + 1][coord.y()][LEFT];
}

bool DeviceGridAnnotation::borderline(const vtr::Point<size_t>& coord, const e_side& side) const {
    /* Check boundary */
    if (coord.x() >= borderline_types_.dim_size(1) || coord.y() >= borderline_types_.dim_size(2)) {
        VTR_LOG_ERROR("Given coordinate (%lu, %lu) is out of device grid size (%lu, %lu)!\n",
                      coord.x(), coord.y(), borderline_types_.dim_size(1), borderline_types_.dim_size(2));
        exit(1);
    }
    if (side == NUM_SIDES) {
        VTR_LOG_ERROR("Invalid side. Expect [TOP|RIGHT|BOTTOM|LEFT]!\n");
        exit(1);
    }
    return borderline_types_[coord.x()][coord.y()][side];
}

} /* End namespace openfpga*/

#endif
