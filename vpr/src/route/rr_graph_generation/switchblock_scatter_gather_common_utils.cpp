
#include "switchblock_scatter_gather_common_utils.h"

#include "globals.h"
#include "vpr_error.h"
#include "rr_node_types.h"
#include "rr_types.h"

/**
 * @brief Counts and summarizes wire types in a routing channel.
 *
 * Iterates through the given array of segment details for a routing channel,
 * grouping segments by wire type (as identified by `type_name()`).
 * For each unique wire type, it records:
 *   - the wire segment length,
 *   - the number of wires of that type,
 *   - the start index of the first wire of that type in the array.
 *
 * @param channel Pointer to an array of `t_chan_seg_details` representing the wires in a channel.
 * @param nodes_per_chan The total number of wire segments (i.e., size of the `channel` array).
 * @return A map (`t_wire_type_sizes`) from wire type name to `WireInfo` containing size and indexing info for each type.
 */
static t_wire_type_sizes count_chan_wire_type_sizes(const t_chan_seg_details* channel, int nodes_per_chan);

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

static t_wire_type_sizes count_chan_wire_type_sizes(const t_chan_seg_details* channel, int nodes_per_chan) {
    int num_wires = 0;
    t_wire_info wire_info;

    vtr::string_view wire_type = channel[0].type_name();
    int length = channel[0].length();
    int start = 0;

    t_wire_type_sizes wire_type_sizes;

    for (int iwire = 0; iwire < nodes_per_chan; iwire++) {
        vtr::string_view new_type = channel[iwire].type_name();
        int new_length = channel[iwire].length();
        int new_start = iwire;
        if (new_type != wire_type) {
            wire_info.set(length, num_wires, start);
            wire_type_sizes[wire_type] = wire_info;
            wire_type = new_type;
            length = new_length;
            start = new_start;
            num_wires = 0;
        }
        num_wires++;
    }

    wire_info.set(length, num_wires, start);
    wire_type_sizes[wire_type] = wire_info;

    return wire_type_sizes;
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

const t_chan_details& index_into_correct_chan(const t_physical_tile_loc& sb_loc,
                                              e_side src_side,
                                              const t_chan_details& chan_details_x,
                                              const t_chan_details& chan_details_y,
                                              t_physical_tile_loc& chan_loc,
                                              e_rr_type& chan_type) {
    chan_type = e_rr_type::CHANX;
    // here we use the VPR convention that a tile 'owns' the channels directly to the right and above it
    switch (src_side) {
        case TOP:
            // this is y-channel belonging to tile above in the same layer
            chan_loc.x = sb_loc.x;
            chan_loc.y = sb_loc.y + 1;
            chan_loc.layer_num = sb_loc.layer_num;
            chan_type = e_rr_type::CHANY;
            return chan_details_y;
            break;
        case RIGHT:
            // this is x-channel belonging to tile to the right in the same layer
            chan_loc.x = sb_loc.x + 1;
            chan_loc.y = sb_loc.y;
            chan_loc.layer_num = sb_loc.layer_num;
            chan_type = e_rr_type::CHANX;
            return chan_details_x;
            break;
        case BOTTOM:
            // this is y-channel on the right of the tile in the same layer
            chan_loc.x = sb_loc.x;
            chan_loc.y = sb_loc.y;
            chan_loc.layer_num = sb_loc.layer_num;
            chan_type = e_rr_type::CHANY;
            return chan_details_y;
            break;
        case LEFT:
            // this is x-channel on top of the tile in the same layer
            chan_loc.x = sb_loc.x;
            chan_loc.y = sb_loc.y;
            chan_loc.layer_num = sb_loc.layer_num;
            chan_type = e_rr_type::CHANX;
            return chan_details_x;
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ARCH, "index_into_correct_chan: unknown side specified: %d\n", src_side);
            break;
    }
    VTR_ASSERT(false);
    return chan_details_x; // Unreachable
}

bool chan_coords_out_of_bounds(const t_physical_tile_loc& loc, e_rr_type chan_type) {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    const int grid_width = grid.width();
    const int grid_height = grid.height();
    const int grid_layers = grid.get_num_layers();

    // the layer that channel is located at must be legal regardless of chan_type
    if (loc.layer_num < 0 || loc.layer_num > grid_layers) {
        return true;
    }

    if (e_rr_type::CHANX == chan_type) {
        // there is no x-channel at x=0
        if (loc.x <= 0 || loc.x >= grid_width - 1 || loc.y < 0 || loc.y >= grid_height - 1) {
            return true;
        }
    } else if (e_rr_type::CHANY == chan_type) {
        // there is no y-channel at y=0
        if (loc.x < 0 || loc.x >= grid_width - 1 || loc.y <= 0 || loc.y >= grid_height - 1) {
            return true;
        }
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH, "chan_coords_out_of_bounds(): illegal channel type %d\n", chan_type);
    }

    return false;
}

std::pair<t_wire_type_sizes, t_wire_type_sizes> count_wire_type_sizes(const t_chan_details& chan_details_x,
                                                                      const t_chan_details& chan_details_y,
                                                                      const t_chan_width& nodes_per_chan) {

    // We assume that x & y channels have the same ratios of wire types. i.e., looking at a single
    // channel is representative of all channels in the FPGA.

    // Count the number of wires in each wire type in the specified channel. Note that this is representative of
    // the wire count for every channel in direction due to the assumption stated above.
    // This will not hold if we
    // 1) support different horizontal and vertical segment distributions
    // 2) support non-uniform channel distributions.

    t_wire_type_sizes wire_type_sizes_x = count_chan_wire_type_sizes(chan_details_x[0][0].data(), nodes_per_chan.x_max);
    t_wire_type_sizes wire_type_sizes_y = count_chan_wire_type_sizes(chan_details_y[0][0].data(), nodes_per_chan.y_max);

    return {wire_type_sizes_x, wire_type_sizes_y};
}