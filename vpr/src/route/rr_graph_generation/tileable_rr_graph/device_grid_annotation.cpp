#include "device_grid_annotation.h"
#include "vtr_log.h"
#include "vpr_utils.h"

DeviceGridAnnotation::DeviceGridAnnotation(const DeviceGrid& grid, const bool perimeter_cb) {
    alloc(grid);
    init(grid, perimeter_cb);
}

void DeviceGridAnnotation::alloc(const DeviceGrid& grid) {
    // Allocate
    chanx_existence_.resize({grid.width(), grid.height()}, false);
    chany_existence_.resize({grid.width(), grid.height()}, false);
}

void DeviceGridAnnotation::init(const DeviceGrid& grid, const bool perimeter_cb) {
    // If shrink is not considered, perimeters are the borderlines
    size_t start_x = 1;
    size_t end_x = grid.width() - 1;
    if (perimeter_cb) {
        start_x = 0;
        end_x = grid.width();
    }
    for (size_t iy = 0; iy < grid.height() - 1; ++iy) {
        for (size_t ix = start_x; ix < end_x; ++ix) {
            chanx_existence_[ix][iy] = !is_empty_type(grid.get_physical_type({(int)ix, (int)iy + 1, 0}));
        }
    }
    size_t start_y = 1;
    size_t end_y = grid.height() - 1;
    if (perimeter_cb) {
        start_y = 0;
        end_y = grid.height();
    }
    for (size_t ix = 0; ix < grid.width() - 1; ++ix) {
        for (size_t iy = start_y; iy < end_y; ++iy) {
            chany_existence_[ix][iy] = !is_empty_type(grid.get_physical_type({(int)ix, (int)iy, 0}));
        }
    }
}

bool DeviceGridAnnotation::is_chanx_exist(const vtr::Point<size_t>& coord) const {
    return chanx_existence_[coord.x()][coord.y()];
}

bool DeviceGridAnnotation::is_chany_exist(const vtr::Point<size_t>& coord) const {
    return chany_existence_[coord.x()][coord.y()];
}

bool DeviceGridAnnotation::is_chanx_start(const vtr::Point<size_t>& coord) const {
    vtr::Point<size_t> neighbor_coord = get_neighbor_coord(coord, LEFT);
    if (neighbor_coord == coord) {
        return true;
    }
    return chanx_existence_[coord.x()][coord.y()] != chanx_existence_[neighbor_coord.x()][neighbor_coord.y()];
}

bool DeviceGridAnnotation::is_chanx_end(const vtr::Point<size_t>& coord) const {
    vtr::Point<size_t> neighbor_coord = get_neighbor_coord(coord, RIGHT);
    if (neighbor_coord == coord) {
        return true;
    }
    return chanx_existence_[coord.x()][coord.y()] != chanx_existence_[neighbor_coord.x()][neighbor_coord.y()];
}

bool DeviceGridAnnotation::is_chany_start(const vtr::Point<size_t>& coord) const {
    vtr::Point<size_t> neighbor_coord = get_neighbor_coord(coord, BOTTOM);
    if (neighbor_coord == coord) {
        return true;
    }
    return chany_existence_[coord.x()][coord.y()] != chany_existence_[neighbor_coord.x()][neighbor_coord.y()];
}

bool DeviceGridAnnotation::is_chany_end(const vtr::Point<size_t>& coord) const {
    vtr::Point<size_t> neighbor_coord = get_neighbor_coord(coord, TOP);
    if (neighbor_coord == coord) {
        return true;
    }
    return chany_existence_[coord.x()][coord.y()] != chany_existence_[neighbor_coord.x()][neighbor_coord.y()];
}

vtr::Point<size_t> DeviceGridAnnotation::get_neighbor_coord(const vtr::Point<size_t>& coord, const e_side& side) const {
    vtr::Point<size_t> neighbor_coord(coord.x(), coord.y());
    if (side == LEFT) {
        if (coord.x() == 0) {
            return coord;
        }
        neighbor_coord.set_x(coord.x() - 1);
    } else if (side == RIGHT) {
        if (coord.x() == chanx_existence_.dim_size(0) - 1) {
            return coord;
        }
        neighbor_coord.set_x(coord.x() + 1);
    } else if (side == TOP) {
        if (coord.y() == chanx_existence_.dim_size(1) - 1) {
            return coord;
        }
        neighbor_coord.set_y(coord.y() + 1);
    } else if (side == BOTTOM) {
        if (coord.y() == 0) {
            return coord;
        }
        neighbor_coord.set_y(coord.y() - 1);
    }
    return neighbor_coord;
}
