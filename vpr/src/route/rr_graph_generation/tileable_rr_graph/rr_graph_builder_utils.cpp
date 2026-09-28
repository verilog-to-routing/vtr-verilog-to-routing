/************************************************************************
 *  This file contains most utilized functions for rr_graph builders 
 ***********************************************************************/
#include <vector>
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_utils.h"

#include "rr_graph_builder_utils.h"

/************************************************************************
 * Correct number of routing channel width to be compatible to 
 * uni-directional routing architecture
 ***********************************************************************/
size_t find_unidir_routing_channel_width(const size_t chan_width) {
    size_t actual_chan_width = chan_width;
    /* Correct the chan_width: it should be an even number */
    if (0 != actual_chan_width % 2) {
        actual_chan_width++; /* increment it to be even */
    }
    VTR_ASSERT(0 == actual_chan_width % 2);

    return actual_chan_width;
}

/************************************************************************
 * Get the class index of a grid pin 
 ***********************************************************************/
int get_grid_pin_class_index(const DeviceGrid& grids,
                             const size_t layer,
                             const size_t x,
                             const size_t y,
                             const int pin_index) {
    /* check */
    t_physical_tile_loc tile_loc(x, y, layer);
    t_physical_tile_type_ptr phy_tile_type = grids.get_physical_type(tile_loc);
    VTR_ASSERT(pin_index < phy_tile_type->num_pins);
    return phy_tile_type->pin_class[pin_index];
}

/* Determine the side of a io grid */
std::vector<e_side> determine_io_grid_pin_side(const vtr::Point<size_t>& device_size,
                                               const vtr::Point<size_t>& grid_coordinate,
                                               const bool perimeter_cb) {
    std::vector<e_side> pin_sides;
    /* TOP side IO of FPGA */
    if (device_size.y() == grid_coordinate.y()) {
        /* Such I/O has only bottom side pins */
        pin_sides.push_back(BOTTOM);
        /* If cbs are allowed around boundary I/Os, add two more sides */
        if (perimeter_cb) {
            pin_sides.push_back(LEFT);
            pin_sides.push_back(RIGHT);
        }
    } else if (device_size.x() == grid_coordinate.x()) { /* RIGHT side IO of FPGA */
        /* Such I/O has only Left side pins */
        pin_sides.push_back(LEFT);
        /* If cbs are allowed around boundary I/Os, add two more sides */
        if (perimeter_cb) {
            pin_sides.push_back(TOP);
            pin_sides.push_back(BOTTOM);
        }
    } else if (0 == grid_coordinate.y()) { /* BOTTOM side IO of FPGA */
        /* Such I/O has only Top side pins */
        pin_sides.push_back(TOP);
        /* If cbs are allowed around boundary I/Os, add two more sides */
        if (perimeter_cb) {
            pin_sides.push_back(LEFT);
            pin_sides.push_back(RIGHT);
        }
    } else if (0 == grid_coordinate.x()) { /* LEFT side IO of FPGA */
        /* Such I/O has only Right side pins */
        pin_sides.push_back(RIGHT);
        /* If cbs are allowed around boundary I/Os, add two more sides */
        if (perimeter_cb) {
            pin_sides.push_back(TOP);
            pin_sides.push_back(BOTTOM);
        }
    } else if ((grid_coordinate.x() < device_size.x()) && (grid_coordinate.y() < device_size.y())) {
        /* I/O grid in the center grid */
        return {TOP, RIGHT, BOTTOM, LEFT};
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "Invalid coordinate (%lu, %lu) for I/O Grid whose size is (%lu, %lu)!\n",
                        grid_coordinate.x(), grid_coordinate.y(),
                        device_size.x(), device_size.y());
    }
    return pin_sides;
}

/* Determine the side of a pin of a grid */
std::vector<e_side> find_grid_pin_sides(const DeviceGrid& grids,
                                        const size_t layer,
                                        const size_t x,
                                        const size_t y,
                                        const size_t pin_id) {
    std::vector<e_side> pin_sides;

    t_physical_tile_loc tile_loc(x, y, layer);
    t_physical_tile_type_ptr phy_tile_type = grids.get_physical_type(tile_loc);
    int width_offset = grids.get_width_offset(tile_loc);
    int height_offset = grids.get_height_offset(tile_loc);
    for (const e_side side : {TOP, RIGHT, BOTTOM, LEFT}) {
        if (true == phy_tile_type->pinloc[width_offset][height_offset][size_t(side)][pin_id]) {
            pin_sides.push_back(side);
        }
    }

    return pin_sides;
}

std::vector<int> get_grid_side_pins(const DeviceGrid& grids,
                                    const size_t layer,
                                    const size_t x,
                                    const size_t y,
                                    const e_pin_type pin_type,
                                    const e_side pin_side,
                                    const int pin_width,
                                    const int pin_height) {
    std::vector<int> pin_list;

    t_physical_tile_type_ptr phy_tile_type = grids.get_physical_type(t_physical_tile_loc(x, y, layer));
    for (int ipin = 0; ipin < phy_tile_type->num_pins; ++ipin) {
        int class_id = phy_tile_type->pin_class[ipin];
        if ((1 == phy_tile_type->pinloc[pin_width][pin_height][pin_side][ipin])
            && (pin_type == phy_tile_type->class_inf[class_id].type)) {
            pin_list.push_back(ipin);
        }
    }
    return pin_list;
}

/************************************************************************
 * Get the number of pins for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides  
 ***********************************************************************/
size_t get_grid_num_pins(const DeviceGrid& grids,
                         const size_t layer,
                         const size_t x,
                         const size_t y,
                         const e_pin_type pin_type,
                         const std::vector<e_side>& io_side) {
    size_t num_pins = 0;

    /* For IO_TYPE sides */
    t_physical_tile_type_ptr phy_tile_type = grids.get_physical_type(t_physical_tile_loc(x, y, layer));
    for (const e_side side : io_side) {
        /* Get pin list */
        for (int width = 0; width < phy_tile_type->width; ++width) {
            for (int height = 0; height < phy_tile_type->height; ++height) {
                std::vector<int> pin_list = get_grid_side_pins(grids, layer, x, y, pin_type, side, width, height);
                num_pins += pin_list.size();
            }
        }
    }

    return num_pins;
}

/************************************************************************
 * Get the number of pins for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides  
 ***********************************************************************/
size_t get_grid_num_classes(const DeviceGrid& grids,
                            const size_t layer,
                            const size_t x,
                            const size_t y,
                            const e_pin_type pin_type) {
    size_t num_classes = 0;

    t_physical_tile_type_ptr phy_tile_type = grids.get_physical_type(t_physical_tile_loc(x, y, layer));
    for (size_t iclass = 0; iclass < phy_tile_type->class_inf.size(); ++iclass) {
        /* Bypass unmatched pin_type */
        if (pin_type != phy_tile_type->class_inf[iclass].type) {
            continue;
        }
        num_classes++;
    }

    return num_classes;
}

/************************************************************************
 * Identify if a X-direction routing channel exist in the fabric
 * This could be entirely possible that a routig channel
 * is in the middle of a multi-width and multi-height grid
 *
 * As the chanx always locates on top of a grid with the same coord
 *
 *     +----------+
 *     |   CHANX  |
 *     |  [x][y]  |
 *     +----------+
 *
 *     +----------+
 *     |   Grid   |   height_offset = height - 1
 *     |  [x][y]  |
 *     +----------+
 *
 *     +----------+
 *     |  Grid    |   height_offset = height - 2
 *     | [x][y-1] |
 *     +----------+
 *  If the CHANX is in the middle of a multi-width and multi-height grid
 *  it should locate at a grid whose height_offset is lower than the its height defined in physical_tile
 *  When height_offset == height - 1, it means that the grid is at the top side of this multi-width and multi-height block
 ***********************************************************************/
bool is_chanx_exist(const DeviceGrid& grids,
                    const size_t layer,
                    const vtr::Point<size_t>& chanx_coord,
                    const bool perimeter_cb,
                    const bool through_channel) {
    size_t chanx_start = 1;
    size_t chanx_end = grids.width() - 2;
    if (perimeter_cb) {
        chanx_start = 0;
        chanx_end = grids.width() - 1;
    }
    if ((chanx_start > chanx_coord.x()) || (chanx_coord.x() > chanx_end)) {
        return false;
    }

    if (chanx_coord.y() > grids.height() - 2) {
        return false;
    }

    if (true == through_channel) {
        return true;
    }

    return (grids.get_height_offset(t_physical_tile_loc(chanx_coord.x(), chanx_coord.y(), layer)) == grids.get_physical_type(t_physical_tile_loc(chanx_coord.x(), chanx_coord.y(), layer))->height - 1);
}

/************************************************************************
 * Identify if a Y-direction routing channel exist in the fabric
 * This could be entirely possible that a routig channel
 * is in the middle of a multi-width and multi-height grid
 *
 * As the chany always locates on right of a grid with the same coord
 *
 * +-----------+  +---------+  +--------+
 * |   Grid    |  |  Grid   |  |  CHANY |
 * | [x-1][y]  |  | [x][y]  |  | [x][y] |
 * +-----------+  +---------+  +--------+
 *  width_offset   width_offset
 *  = width - 2   = width -1
 *  If the CHANY is in the middle of a multi-width and multi-height grid
 *  it should locate at a grid whose width_offset is lower than the its width defined in physical_tile
 *  When height_offset == height - 1, it means that the grid is at the top side of this multi-width and multi-height block
 *
 *  If through channel is allowed, the chany will always exists
 *  unless it falls out of the grid array
 ***********************************************************************/
bool is_chany_exist(const DeviceGrid& grids,
                    const size_t layer,
                    const vtr::Point<size_t>& chany_coord,
                    const bool perimeter_cb,
                    const bool through_channel) {
    size_t chany_start = 1;
    size_t chany_end = grids.height() - 2;
    if (perimeter_cb) {
        chany_start = 0;
        chany_end = grids.height() - 1;
    }
    if (chany_coord.x() > grids.width() - 2) {
        return false;
    }

    if ((chany_start > chany_coord.y()) || (chany_coord.y() > chany_end)) {
        return false;
    }

    if (true == through_channel) {
        return true;
    }

    return (grids.get_width_offset(t_physical_tile_loc(chany_coord.x(), chany_coord.y(), layer)) == grids.get_physical_type(t_physical_tile_loc(chany_coord.x(), chany_coord.y(), layer))->width - 1);
}

/************************************************************************
 * Identify if a X-direction routing channel is at the right side of a 
 * multi-height grid
 *
 *     +-----------------+
 *     |                 |
 *     |                 |  +-------------+
 *     |      Grid       |  |   CHANX     |
 *     |    [x-1][y]     |  |   [x][y]    |
 *     |                 |  +-------------+
 *     |                 |
 *     +-----------------+
 ***********************************************************************/
bool is_chanx_right_to_multi_height_grid(const DeviceGrid& grids,
                                         const size_t layer,
                                         const vtr::Point<size_t>& chanx_coord,
                                         const bool perimeter_cb,
                                         const bool through_channel) {
    size_t start_x = 1;
    if (perimeter_cb) {
        start_x = 0;
    } else {
        VTR_ASSERT(0 < chanx_coord.x());
    }
    if (start_x == chanx_coord.x()) {
        /* This is already the LEFT side of FPGA fabric,
         * it is the same results as chanx is right to a multi-height grid
         */
        return true;
    }

    if (false == through_channel) {
        /* We check the left neighbor of chanx, if it does not exist, the chanx is left to a multi-height grid */
        vtr::Point<size_t> left_chanx_coord(chanx_coord.x() - 1, chanx_coord.y());
        if (false == is_chanx_exist(grids, layer, left_chanx_coord, perimeter_cb)) {
            return true;
        }
    }

    return false;
}

/************************************************************************
 * Identify if a X-direction routing channel is at the left side of a 
 * multi-height grid
 *
 *                            +-----------------+
 *                            |                 |
 *        +---------------+   |                 | 
 *        |    CHANX      |   |      Grid       | 
 *        |    [x][y]     |   |    [x+1][y]     | 
 *        +---------------+   |                 |
 *                            |                 |
 *                            +-----------------+
 ***********************************************************************/
bool is_chanx_left_to_multi_height_grid(const DeviceGrid& grids,
                                        const size_t layer,
                                        const vtr::Point<size_t>& chanx_coord,
                                        const bool perimeter_cb,
                                        const bool through_channel) {
    VTR_ASSERT(chanx_coord.x() <= grids.width() - 1);
    size_t end_x = grids.width() - 2;
    if (perimeter_cb) {
        end_x = grids.width() - 1;
    }

    if (end_x == chanx_coord.x()) {
        /* This is already the RIGHT side of FPGA fabric,
         * it is the same results as chanx is right to a multi-height grid
         */
        return true;
    }

    if (false == through_channel) {
        /* We check the right neighbor of chanx, if it does not exist, the chanx is left to a multi-height grid */
        vtr::Point<size_t> right_chanx_coord(chanx_coord.x() + 1, chanx_coord.y());
        if (false == is_chanx_exist(grids, layer, right_chanx_coord, perimeter_cb)) {
            return true;
        }
    }

    return false;
}

/************************************************************************
 * Identify if a Y-direction routing channel is at the top side of a 
 * multi-width grid
 * 
 *          +--------+
 *          | CHANY  |
 *          | [x][y] | 
 *          +--------+
 *
 *     +-----------------+
 *     |                 |
 *     |                 | 
 *     |      Grid       | 
 *     |    [x-1][y]     | 
 *     |                 | 
 *     |                 |
 *     +-----------------+
 ***********************************************************************/
bool is_chany_top_to_multi_width_grid(const DeviceGrid& grids,
                                      const size_t layer,
                                      const vtr::Point<size_t>& chany_coord,
                                      const bool perimeter_cb,
                                      const bool through_channel) {
    size_t start_y = 1;
    if (perimeter_cb) {
        start_y = 0;
    } else {
        VTR_ASSERT(0 < chany_coord.y());
    }
    if (start_y == chany_coord.y()) {
        /* This is already the BOTTOM side of FPGA fabric,
         * it is the same results as chany is at the top of a multi-width grid
         */
        return true;
    }

    if (false == through_channel) {
        /* We check the bottom neighbor of chany, if it does not exist, the chany is top to a multi-height grid */
        vtr::Point<size_t> bottom_chany_coord(chany_coord.x(), chany_coord.y() - 1);
        if (false == is_chany_exist(grids, layer, bottom_chany_coord, perimeter_cb)) {
            return true;
        }
    }

    return false;
}

/************************************************************************
 * Identify if a Y-direction routing channel is at the bottom side of a 
 * multi-width grid
 * 
 *     +-----------------+
 *     |                 |
 *     |                 | 
 *     |      Grid       | 
 *     |    [x][y+1]     | 
 *     |                 | 
 *     |                 |
 *     +-----------------+
 *          +--------+
 *          | CHANY  |
 *          | [x][y] | 
 *          +--------+
 *
 ***********************************************************************/
bool is_chany_bottom_to_multi_width_grid(const DeviceGrid& grids,
                                         const size_t layer,
                                         const vtr::Point<size_t>& chany_coord,
                                         const bool perimeter_cb,
                                         const bool through_channel) {
    VTR_ASSERT(chany_coord.y() <= grids.height() - 1);
    size_t end_y = grids.height() - 2;
    if (perimeter_cb) {
        end_y = grids.height() - 1;
    }

    if (end_y == chany_coord.y()) {
        /* This is already the TOP side of FPGA fabric,
         * it is the same results as chany is at the bottom of a multi-width grid
         */
        return true;
    }

    if (false == through_channel) {
        /* We check the top neighbor of chany, if it does not exist, the chany is left to a multi-height grid */
        vtr::Point<size_t> top_chany_coord(chany_coord.x(), chany_coord.y() + 1);
        if (false == is_chany_exist(grids, layer, top_chany_coord, perimeter_cb)) {
            return true;
        }
    }

    return false;
}

int find_parallel_seg_index(const int abs_index,
                            const t_unified_to_parallel_seg_index& index_map,
                            const e_parallel_axis parallel_axis) {
    int index = -1;
    auto itr_pair = index_map.equal_range(abs_index);

    for (auto itr = itr_pair.first; itr != itr_pair.second; ++itr) {
        if (itr->second.second == parallel_axis) {
            index = itr->second.first;
        }
    }

    return index;
}
