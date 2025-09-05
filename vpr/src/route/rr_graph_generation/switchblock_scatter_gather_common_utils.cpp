
#include "switchblock_scatter_gather_common_utils.h"

#include "vpr_error.h"

/**
 * @brief Check whether specified coordinate is located at the device grid corner and a switch block exists there
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param x x-coordinate of the location
 *   @param y y-coordinate of the location
 *   @param layer layer-coordinate of the location
 *
 * @return true if the specified coordinate represents a corner location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_corner_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer);

/**
 * @brief check whether specified coordinate is located at one of the perimeter device grid locations and a switch block exists there
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param x x-coordinate of the location
 *   @param y y-coordinate of the location
 *   @param layer layer-coordinate of the location
 *
 * @return true if the specified coordinate represents a perimeter location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_perimeter_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer);

/**
 * @brief check whether specified coordinate is located at core of the device grid (not perimeter) and a switch block exists there
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param x x-coordinate of the location
 *   @param y y-coordinate of the location
 *   @param layer layer-coordinate of the location
 *
 * @return true if the specified coordinate represents a core location within the device grid and a switch block exists there, false otherwise.
 */
static bool is_core_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer);


/**
 * @brief check whether specified coordinate is located in the architecture-specified regions that the switchblock should be applied to
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param x x-coordinate of the location
 *   @param y y-coordinate of the location
 *   @param sb switchblock information specified in the architecture file
 *
 * @return true if the specified coordinate falls into the architecture-specified location for this switchblock, false otherwise.
 */
static bool match_sb_xy(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer, const t_specified_loc& sb);


static bool is_corner_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer) {
    if (!inter_cluster_rr[layer]) {
        return false;
    }

    bool is_corner = false;
    if ((x == 0 && y == 0) || (x == 0 && y == int(grid.height()) - 2) || //-2 for no perim channels
        (x == int(grid.width()) - 2 && y == 0) ||                        //-2 for no perim channels
        (x == int(grid.width()) - 2 && y == int(grid.height()) - 2)) {   //-2 for no perim channels
            is_corner = true;
        }
    return is_corner;
}

static bool is_perimeter_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer) {
    if (!inter_cluster_rr[layer]) {
        return false;
    }

    bool is_perimeter = false;
    if (x == 0 || x == int(grid.width()) - 2 || y == 0 || y == int(grid.height()) - 2) {
        is_perimeter = true;
    }
    return is_perimeter;
}

static bool is_core_sb(const DeviceGrid& grid, const std::vector<bool>& inter_cluster_rr, int x, int y, int layer) {
    if (!inter_cluster_rr[layer]) {
        return false;
    }

    bool is_core = !is_perimeter_sb(grid, inter_cluster_rr, x, y, layer);
    return is_core;
}

static bool match_sb_xy(const DeviceGrid& grid,
                        const std::vector<bool>& inter_cluster_rr,
                        int x,
                        int y,
                        int layer,
                        const t_specified_loc& specified_loc) {
    if (!inter_cluster_rr[layer]) {
        return false;
    }

    // if one of sb_x and sb_y is defined, we either know the exact location (x,y) or the exact x location (will apply it to all rows)
    // or the exact y location (will apply it to all columns)
    if (specified_loc.x != -1 || specified_loc.y != -1) {
        if (x == specified_loc.x && y == specified_loc.y) {
            return true;
        }

        if (x == specified_loc.x && specified_loc.y == -1) {
            return true;
        }

        if (specified_loc.x == -1 && y == specified_loc.y) {
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
    int x_reg_step = (sb.reg_x.repeat != 0) ? (x - sb.reg_x.start) / sb.reg_x.repeat : sb.reg_x.start;
    int y_reg_step = (sb.reg_y.repeat != 0) ? (y - sb.reg_y.start) / sb.reg_y.repeat : sb.reg_y.start;

    // step must be non-negative
    x_reg_step = std::max(0, x_reg_step);
    y_reg_step = std::max(0, y_reg_step);

    int reg_startx = sb.reg_x.start + (x_reg_step * sb.reg_x.repeat);
    int reg_endx = sb.reg_x.end + (x_reg_step * sb.reg_x.repeat);
    reg_endx = std::min(reg_endx, int(grid.width() - 1));

    int reg_starty = sb.reg_y.start + (y_reg_step * sb.reg_y.repeat);
    int reg_endy = sb.reg_y.end + (y_reg_step * sb.reg_y.repeat);
    reg_endy = std::min(reg_endy, int(grid.height() - 1));

    // check x coordinate
    if (x >= reg_startx && x <= reg_endx) { //should fall into the region
        // we also should respect the incrx
        // if incrx is not equal to 1, all locations within this region are *NOT* valid
        if ((x + reg_startx) % sb.reg_x.incr == 0) {
            // valid x coordinate, check for y value
            if (y >= reg_starty && y <= reg_endy) {
                // check for incry, similar as incrx
                if ((y + reg_starty) % sb.reg_y.incr == 0) {
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
                 int x,
                 int y,
                 int layer,
                 e_sb_location sb_location,
                 const t_specified_loc& specified_loc/*=t_specified_loc()*/) {
    bool sb_not_here = true;

    switch (sb_location) {
        case e_sb_location::E_EVERYWHERE:
            sb_not_here = false;
            break;

        case e_sb_location::E_PERIMETER:
            if (is_perimeter_sb(grid, inter_cluster_rr, x, y, layer)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_CORNER:
            if (is_corner_sb(grid, inter_cluster_rr, x, y, layer)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_CORE:
            if (is_core_sb(grid, inter_cluster_rr, x, y, layer)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_FRINGE:
            if (is_perimeter_sb(grid, inter_cluster_rr, x, y, layer) && !is_corner_sb(grid, inter_cluster_rr, x, y, layer)) {
                sb_not_here = false;
            }
            break;

        case e_sb_location::E_XY_SPECIFIED:
            if (match_sb_xy(grid, inter_cluster_rr, x, y, layer, specified_loc)) {
                sb_not_here = false;
            }
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "sb_not_here: unrecognized location enum: %d\n", sb_location);
            break;
    }

    return sb_not_here;
}