
#include "switchblock_scatter_gather_common_utils.h"

#include "vpr_error.h"

/**
 * @brief Check whether specified coordinate is located at the device grid corner and a switch block exists there
 * @return true if the specified coordinate represents a corner location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_corner_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc);

/**
 * @brief check whether specified coordinate is located at one of the perimeter device grid locations and a switch block exists there *
 * @return true if the specified coordinate represents a perimeter location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_perimeter_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc);

/**
 * @brief check whether specified coordinate is located at core of the device grid (not perimeter) and a switch block exists there
 * @return true if the specified coordinate represents a core location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_core_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc);

/**
 * @brief check whether specified coordinate is located in the architecture-specified regions that the switchblock should be applied to
 * @return true if the specified coordinate falls into the architecture-specified location for this switchblock, false otherwise.
 */
static bool match_sb_xy(const DeviceGrid& grid, const t_physical_tile_loc& loc, const t_specified_loc& sb);


static bool is_corner_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc) {
    bool is_corner = false;
    if ((loc.x == 0 && loc.y == 0) || (loc.x == 0 && loc.y == int(grid.height()) - 2) || //-2 for no perim channels
        (loc.x == int(grid.width()) - 2 && loc.y == 0) ||                                //-2 for no perim channels
        (loc.x == int(grid.width()) - 2 && loc.y == int(grid.height()) - 2)) {           //-2 for no perim channels
            is_corner = true;
        }
    return is_corner;
}

static bool is_perimeter_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc) {
    bool is_perimeter = false;
    if (loc.x == 0 || loc.x == int(grid.width()) - 2 || loc.y == 0 || loc.y == int(grid.height()) - 2) {
        is_perimeter = true;
    }
    return is_perimeter;
}

static bool is_core_sb(const DeviceGrid& grid, const t_physical_tile_loc& loc) {
    bool is_core = !is_perimeter_sb(grid, loc);
    return is_core;
}

static bool match_sb_xy(const DeviceGrid& grid,
                        const t_physical_tile_loc& loc,
                        const t_specified_loc& specified_loc) {
    // if one of sb_x and sb_y is defined, we either know the exact location (x,y) or the exact x location (will apply it to all rows)
    // or the exact y location (will apply it to all columns)
    if (specified_loc.x != -1 || specified_loc.y != -1) {
        if (loc.x == specified_loc.x && loc.y == specified_loc.y) {
            return true;
        }

        if (loc.x == specified_loc.x && specified_loc.y == -1) {
            return true;
        }

        if (specified_loc.x == -1 && loc.y == specified_loc.y) {
            return true;
        }
    }

    // if both sb_x and sb_y is not defined, we have a region that we should apply this SB pattern to, we just need to check
    // whether the location passed into this function falls within this region or not
    // calculate the appropriate region based on the repeatx/repeaty and current location.
    // This is to determine whether the given location is part of the current SB specified region with regular expression or not
    // After region calculation, the current SB will apply to this location if:
    // 1) the given (x,y) location falls into the calculated region
    // *AND*
    // 2) incrx/incry are respected within the region, this means all locations within the calculated region do
    //    not necessarily crosspond to the current SB. If incrx/incry is equal to 1, then all locations within the
    //    calculated region are valid.

    // calculate the region
    int x_reg_step = (specified_loc.reg_x.repeat != 0) ? (loc.x - specified_loc.reg_x.start) / specified_loc.reg_x.repeat : specified_loc.reg_x.start;
    int y_reg_step = (specified_loc.reg_y.repeat != 0) ? (loc.y - specified_loc.reg_y.start) / specified_loc.reg_y.repeat : specified_loc.reg_y.start;

    // step must be non-negative
    x_reg_step = std::max(0, x_reg_step);
    y_reg_step = std::max(0, y_reg_step);

    int reg_startx = specified_loc.reg_x.start + (x_reg_step * specified_loc.reg_x.repeat);
    int reg_endx = specified_loc.reg_x.end + (x_reg_step * specified_loc.reg_x.repeat);
    reg_endx = std::min(reg_endx, int(grid.width() - 1));

    int reg_starty = specified_loc.reg_y.start + (y_reg_step * specified_loc.reg_y.repeat);
    int reg_endy = specified_loc.reg_y.end + (y_reg_step * specified_loc.reg_y.repeat);
    reg_endy = std::min(reg_endy, int(grid.height() - 1));

    // check x coordinate
    if (loc.x >= reg_startx && loc.x <= reg_endx) { //should fall into the region
        // we also should respect the incrx
        // if incrx is not equal to 1, all locations within this region are *NOT* valid
        if ((loc.x + reg_startx) % specified_loc.reg_x.incr == 0) {
            // valid x coordinate, check for y value
            if (loc.y >= reg_starty && loc.y <= reg_endy) {
                // check for incry, similar as incrx
                if ((loc.y + reg_starty) % specified_loc.reg_y.incr == 0) {
                    // both x and y are valid
                    return true;
                }
            }
        }
    }

    // if reach here, we don't have sb in this location
    return false;
}


bool sb_not_here(const DeviceGrid& grid,
                 const std::vector<bool>& inter_cluster_rr,
                 const t_physical_tile_loc& loc,
                 e_sb_location sb_location,
                 const t_specified_loc& specified_loc/*=t_specified_loc()*/) {
    if (!inter_cluster_rr[loc.layer_num]) {
        return true;
    }

    bool sb_not_here = true;
    switch (sb_location) {
        case e_sb_location::E_EVERYWHERE:
            sb_not_here = false;
            break;

        case e_sb_location::E_PERIMETER:
            if (is_perimeter_sb(grid, loc)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_CORNER:
            if (is_corner_sb(grid, loc)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_CORE:
            if (is_core_sb(grid, loc)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_FRINGE:
            if (is_perimeter_sb(grid, loc) && !is_corner_sb(grid, loc)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_XY_SPECIFIED:
            if (match_sb_xy(grid, loc, specified_loc)) {
                sb_not_here = false;
            }
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "sb_not_here: unrecognized location enum: %d\n", sb_location);
            break;
    }

    return sb_not_here;
}