/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <regex>
using namespace std;

#include "vtr_assert.h"
#include "vtr_matrix.h"
#include "vtr_math.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "SetupGrid.h"
#include "expr_eval.h"

static bool grid_satisfies_instance_counts(const DeviceGrid& grid, std::map<t_type_ptr,size_t> instance_counts);
static DeviceGrid create_device_grid(const t_grid_def& grid_def, size_t width, size_t height);

static void CheckGrid(const DeviceGrid& grid);

static void set_grid_block_type(int priority, const t_type_descriptor* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<int>& grid_priorities);

static bool is_integer(std::string val);

//Create a device grid which satisfies the minimum block counts
//  If a set of fixed grid layouts are specified, the smallest satisfying grid is picked
//  If an auto grid layouts are specified, the smallest dynamicly sized grid is picked
DeviceGrid create_smallest_device_grid(std::vector<t_grid_def> grid_layouts, std::map<t_type_ptr,size_t> minimum_instance_counts) {
    VTR_ASSERT(grid_layouts.size() > 0);

    if (grid_layouts[0].grid_type == GridDefType::AUTO) {
        //Automatic grid layout, find the smallest height/width
        VTR_ASSERT_MSG(grid_layouts.size() == 1, "Only one grid definitions is valid if using an auto grid layout");

        const auto& grid_def = grid_layouts[0];
        VTR_ASSERT(grid_def.aspect_ratio >= 0.);

        int width = 3;
        int height = 3;
        while (true) {

            //Increase the size
            if (grid_def.aspect_ratio >= 1.) {
                height++;
                width = vtr::nint(height / grid_def.aspect_ratio);
            } else {
                width++;
                height = vtr::nint(width / grid_def.aspect_ratio);
            }

            //Build the device
            auto grid = create_device_grid(grid_def, width, height);

            //Check if it satisfies the block counts
            if (grid_satisfies_instance_counts(grid, minimum_instance_counts)) {
                return grid;
            }
        }

    } else {
        VTR_ASSERT(grid_layouts[0].grid_type == GridDefType::FIXED);
        //Fixed grid layouts, find the smallest of the fixed layouts

        //Sort the grid layouts from smallest to largest
        auto area_cmp = [](const t_grid_def& lhs, const t_grid_def& rhs) {
            int lhs_area = lhs.width * lhs.height;
            int rhs_area = rhs.width * rhs.height;

            return lhs_area < rhs_area;
        };
        std::stable_sort(grid_layouts.begin(), grid_layouts.end(), area_cmp);

        //Try all the fixed devices in order from smallest to largest
        for (const auto& grid_def : grid_layouts) {

            //Build the grid
            auto grid = create_device_grid(grid_def, grid_def.width, grid_def.height);        

            if (grid_satisfies_instance_counts(grid, minimum_instance_counts)) {
                return grid;
            }
        }
    }

    VPR_THROW(VPR_ERROR_OTHER, "Faild to find device which satisifies resource requirements");
    return DeviceGrid(); //Unreachable
}

static bool grid_satisfies_instance_counts(const DeviceGrid& grid, std::map<t_type_ptr,size_t> instance_counts) {
    for (auto kv : instance_counts) {
        t_type_ptr type;
        size_t min_count;
        std::tie(type, min_count) = kv;

        size_t inst_cnt = grid.num_instances(type);

        if (inst_cnt < min_count) {
            return false;
        }
    }
    return true;
}

//Build the specified device grid 
static DeviceGrid create_device_grid(const t_grid_def& grid_def, size_t width, size_t height) {
    if (grid_def.grid_type == GridDefType::FIXED) {

        if (grid_def.width != int(width) || grid_def.height != int(height)) {
            VPR_THROW(VPR_ERROR_OTHER, 
                    "Requested grid size (%dx%d) does not match fixed device size (%dx%d)",
                    width, height, grid_def.width, grid_def.height);
        }
    }

    auto& device_ctx = g_vpr_ctx.device();

    //Track the current priority for each grid location
    // Note that we initialize it to the lowest (i.e. most negative) possible value, so
    // any user-specified priority will override the default empty grid
    auto grid_priorities = vtr::Matrix<int>({width, height}, std::numeric_limits<int>::lowest());

    auto grid = vtr::Matrix<t_grid_tile>({width, height});

    //Initialize the device to all empty blocks
    auto empty_type = find_block_type_by_name(EMPTY_BLOCK_NAME, device_ctx.block_types, device_ctx.num_block_types);
    VTR_ASSERT(empty_type != nullptr);
    for (size_t x = 0; x < width; ++x) {
        for (size_t y = 0; y < height; ++y) {
            set_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                    empty_type, x, y, grid, grid_priorities);
        }
    }

    //Sort the grid specifications by priority
    // Note that we us a stable sort to respect the architecture file order in the ambiguous case
    // of specifications with equal priorities
    auto priority_order = [](const t_grid_loc_def& lhs, const t_grid_loc_def& rhs) {
        return lhs.priority < rhs.priority;
    };
    auto grid_loc_defs = grid_def.loc_defs;
    std::stable_sort(grid_loc_defs.begin(), grid_loc_defs.end(), priority_order);

    //Process the gird location specifications from lowest to highest priority,
    //this ensure higher priority specifications override lower priority specifications
    for (const auto& grid_loc_def : grid_loc_defs) {
        //Fill in the block types according to the specification

        t_type_descriptor* type = find_block_type_by_name(grid_loc_def.block_type, device_ctx.block_types, device_ctx.num_block_types);

        if (!type) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Failed to find block type '%s' for grid location specification",
                    grid_loc_def.block_type.c_str());
        }

        t_formula_data vars;
        vars.set_var_value("W", width);
        vars.set_var_value("H", height);
        vars.set_var_value("w", type->width);
        vars.set_var_value("h", type->height);

        //vtr::printf("Applying grid_loc_def for '%s' priority %d\n", type->name, grid_loc_def.priority);

        //Load the x specification
        auto& xspec = grid_loc_def.x;

        VTR_ASSERT_MSG(!xspec.start_expr.empty(), "x start position must be specified");
        VTR_ASSERT_MSG(!xspec.end_expr.empty(), "x end position must be specified");
        VTR_ASSERT_MSG(!xspec.incr_expr.empty(), "x increment must be specified");
        VTR_ASSERT_MSG(!xspec.repeat_expr.empty(), "x repeat must be specified");

        size_t startx = parse_formula(xspec.start_expr, vars);
        size_t endx = parse_formula(xspec.end_expr, vars);
        size_t incrx = parse_formula(xspec.incr_expr, vars);
        size_t repeatx = parse_formula(xspec.repeat_expr, vars);

        //Load the y specification
        auto& yspec = grid_loc_def.y;

        VTR_ASSERT_MSG(!yspec.start_expr.empty(), "y start position must be specified");
        VTR_ASSERT_MSG(!yspec.end_expr.empty(), "y end position must be specified");
        VTR_ASSERT_MSG(!yspec.incr_expr.empty(), "y increment must be specified");
        VTR_ASSERT_MSG(!yspec.repeat_expr.empty(), "y repeat must be specified");

        size_t starty = parse_formula(yspec.start_expr, vars);
        size_t endy = parse_formula(yspec.end_expr, vars);
        size_t incry = parse_formula(yspec.incr_expr, vars);
        size_t repeaty = parse_formula(yspec.repeat_expr, vars);

        //Warn if start and end fall outside the device dimensions
        // Unless it is explicitly specified as an integer, this avoids spurious warnings during the early 
        // stages of packing when the device may be very small and some larger columns may not yet exist.
        // Note that this suppress only warnings for explicit integers and will still warn about possibly 
        // incorrect formula
        if (startx > width - 1 && !is_integer(xspec.start_expr)) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification startx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                    type->name, xspec.start_expr.c_str(), startx, 0, width - 1);
        }

        if (endx > width - 1 && !is_integer(xspec.end_expr)) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification endx (%s = %d) falls outside device horizontal range [%d,%d]\n",
                    type->name, xspec.end_expr.c_str(), endx, 0, width - 1);
        }

        if (starty > height - 1 && !is_integer(yspec.start_expr)) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification starty (%s = %d) falls outside device vertical range [%d,%d]\n",
                    type->name, yspec.start_expr.c_str(), starty, 0, height - 1);
        }

        if (endy > height - 1 && !is_integer(yspec.end_expr)) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification endy (%s = %d) falls outside device vertical range [%d,%d]\n",
                    type->name, yspec.end_expr.c_str(), endy, 0, height - 1);
        }

        //The end must fall after (or equal) to the start
        if (endx < startx) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification endx (%s = %d) can not come before startx (%s = %d) for block type '%s'",
                    xspec.end_expr.c_str(), endx, xspec.start_expr.c_str(), startx, type->name);
        }

        if (endy < starty) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification endy (%s = %d) can not come before starty (%s = %d) for block type '%s'",
                    yspec.end_expr.c_str(), endy, yspec.start_expr.c_str(), starty, type->name);
        }

        //The minimum increment is the block dimension
        VTR_ASSERT(type->width > 0);
        if (incrx < size_t(type->width)) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification x increment for block type '%s' must be at least block width (%d) to avoid overlapping instances (was %s = %d)",
                    type->name, type->width, xspec.incr_expr.c_str(), incrx);
        }

        VTR_ASSERT(type->height > 0);
        if (incry < size_t(type->height)) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification y increment for block type '%s' must be at least block height (%d) to avoid overlapping instances (was %s = %d)",
                    type->name, type->height, yspec.incr_expr.c_str(), incry);
        }

        //The minimum repeat is the region dimension
        size_t region_width = endx - startx + 1; //+1 since start/end are both inclusive
        if (repeatx < region_width) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification x repeat for block type '%s' must be at least the region width (%d) to avoid overlapping instances (was %s = %d)",
                    type->name, region_width, xspec.repeat_expr.c_str(), repeatx);
        }

        size_t region_height = endy - starty + 1; //+1 since start/end are both inclusive
        if (repeaty < region_height) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification y repeat for block type '%s' must be at least the region height (%d) to avoid overlapping instances (was %s = %d)",
                    type->name, region_height, xspec.repeat_expr.c_str(), repeaty);
        }

        size_t x_end = 0;
        for (size_t kx = 0; x_end < width; ++kx) {
            size_t x_start = startx + kx * repeatx;
            x_end = endx + kx * repeatx;

            size_t y_end = 0;
            for (size_t ky = 0; y_end < height; ++ky) {
                size_t y_start = starty + ky * repeaty;
                y_end = endy + ky * repeaty;


                size_t x_max = std::min(x_end, width-1);
                size_t y_max = std::min(y_end, height-1);

                for(size_t x = x_start; x + (type->width - 1) <= x_max; x += incrx) {
                    for(size_t y = y_start; y + (type->height - 1) <= y_max; y += incry) {
                        set_grid_block_type(grid_loc_def.priority, type, x, y, grid, grid_priorities);
                    }
                }
            }
        }
    }

    auto device_grid = DeviceGrid(grid_def.name, grid);

	CheckGrid(device_grid);

    return device_grid;
}

static void set_grid_block_type(int priority, const t_type_descriptor* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<int>& grid_priorities) {

    struct TypeLocation {
        TypeLocation(size_t x_val, size_t y_val, const t_type_descriptor* type_val, int priority_val)
            : x(x_val), y(y_val), type(type_val), priority(priority_val) {}
        size_t x;
        size_t y;
        const t_type_descriptor* type;
        int priority;

        bool operator<(const TypeLocation& rhs) const {
            return x < rhs.x || y < rhs.y || type < rhs.type;
        }
   };

    //Collect locations effected by this block
    std::set<TypeLocation> target_locations;
    for(size_t x = x_root; x < x_root + type->width; ++x) {
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            target_locations.insert(TypeLocation(x, y, grid[x][y].type, grid_priorities[x][y]));
        }
    }

    //Record the highest priority of all effected locations
    auto iter = target_locations.begin();
    TypeLocation max_priority_type_loc = *iter;
    for(; iter != target_locations.end(); ++iter) {
        if (iter->priority > max_priority_type_loc.priority) {
            max_priority_type_loc = *iter;
        }
    }

    if (priority < max_priority_type_loc.priority) {
        //Lower priority, do not override
        return;
    }

    if (priority == max_priority_type_loc.priority) {
        //Ambiguous case where current grid block and new specification have equal priority
        //
        //We arbitrarily decide to take the 'last applied' wins approach, and warn the user 
        //about the potential ambiguity
        vtr::printf_warning(__FILE__, __LINE__,
                "Ambiguous block type specification at grid location (%zu,%zu)."
                " Existing block type '%s' at (%zu,%zu) has the same priority (%d) as new overlapping type '%s'."
                " The last specification will apply.\n",
                x_root, y_root, 
                max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
                priority, type->name);
    }



    //Mark all the grid tiles 'covered' by this block with the appropriate type
    //and width/height offsets
    std::set<TypeLocation> root_blocks_to_rip_up;
    auto& device_ctx = g_vpr_ctx.device();
    for (size_t x = x_root; x < x_root + type->width; ++x) {
        VTR_ASSERT(x < grid.end_index(0));

        size_t x_offset = x - x_root;
        for (size_t y = y_root; y < y_root + type->height; ++y) {

            VTR_ASSERT(y < grid.end_index(1));
            size_t y_offset = y - y_root;

            auto& grid_tile = grid[x][y];
            VTR_ASSERT(grid_priorities[x][y] <= priority);


            if (   grid_tile.type != nullptr 
                && grid_tile.type != device_ctx.EMPTY_TYPE) {
                //We are overriding a non-empty block, we need to be careful
                //to ensure we remove any blocks which will be invalidated when we 
                //overwrite part of their locations

                size_t orig_root_x = x - grid[x][y].width_offset;
                size_t orig_root_y = y - grid[x][y].height_offset;

                root_blocks_to_rip_up.insert(TypeLocation(orig_root_x, orig_root_y, grid[x][y].type, grid_priorities[x][y]));
            }

            grid[x][y].type = type;
            grid[x][y].width_offset = x_offset;
            grid[x][y].height_offset = y_offset;

            grid_priorities[x][y] = priority;
        }
    }

    //Rip-up any invalidated blocks
    for (auto invalidated_root : root_blocks_to_rip_up) {

        VTR_ASSERT(grid[invalidated_root.x][invalidated_root.y].width_offset == 0);
        VTR_ASSERT(grid[invalidated_root.x][invalidated_root.y].height_offset == 0);

        //Mark all the grid locations used by this root block as empty
        for (size_t x = invalidated_root.x; x < invalidated_root.x + invalidated_root.type->width; ++x) {
            int x_offset = x - invalidated_root.x;
            for (size_t y = invalidated_root.y; y < invalidated_root.y + invalidated_root.type->height; ++y) {
                int y_offset = y - invalidated_root.y;

                if (   grid[x][y].type == invalidated_root.type 
                    && grid[x][y].width_offset == x_offset
                    && grid[x][y].height_offset == y_offset) {
                    //This is a left-over invalidated block, mark as empty
                    // Note: that we explicitly check the type and offsets, since the original block
                    //       may have been completely overwritten, and we don't want to change anything
                    //       in that case
                    VTR_ASSERT(device_ctx.EMPTY_TYPE->width == 1);
                    VTR_ASSERT(device_ctx.EMPTY_TYPE->height == 1);

                    grid[x][y].type = device_ctx.EMPTY_TYPE;
                    grid[x][y].width_offset = 0;
                    grid[x][y].height_offset = 0;

                    grid_priorities[x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
}

static void CheckGrid(const DeviceGrid& grid) {

	int i, j;

    auto& device_ctx = g_vpr_ctx.device();

	/* Check grid is valid */
	for (i = 0; i <= (device_ctx.nx + 1); ++i) {
		for (j = 0; j <= (device_ctx.ny + 1); ++j) {
            auto type = grid[i][j].type;
			if (NULL == type) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has no type.\n", i, j);
			}

			if ((grid[i][j].width_offset < 0)
					|| (grid[i][j].width_offset >= type->width)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has invalid width offset (%d).\n", i, j, grid[i][j].width_offset);
			}
			if ((grid[i][j].height_offset < 0)
					|| (grid[i][j].height_offset >= type->height)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has invalid height offset (%d).\n", i, j, grid[i][j].height_offset);
			}

            //Verify that type and width/height offsets are correct (e.g. for dimension > 1 blocks)
            if (grid[i][j].width_offset == 0 && grid[i][j].height_offset == 0) {
                //From the root block check that all other blocks are correct
                for (int x = i; x < i + type->width; ++x) {
                    int x_offset = x - i;
                    for (int y = j; y < j + type->height; ++y) {
                        int y_offset = y - j;

                        auto& tile = grid[x][y];
                        if (tile.type != type) {
                            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
                                    "Grid Location (%d,%d) should have type '%s' (based on root location) but has type '%s'\n", 
                                    i, j, type->name, tile.type->name);
                        }

                        if (tile.width_offset != x_offset) {
                            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
                                    "Grid Location (%d,%d) of type '%s' should have width offset '%d' (based on root location) but has '%d'\n", 
                                    i, j, type->name, x_offset, tile.width_offset);
                        }

                        if (tile.height_offset != y_offset) {
                            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
                                    "Grid Location (%d,%d)  of type '%s' should have height offset '%d' (based on root location) but has '%d'\n", 
                                    i, j, type->name, y_offset, tile.height_offset);
                        }
                    }
                }
            }
		}
	}
}

static bool is_integer(std::string val) {
    auto regex = std::regex("\\s*\\d+\\s*"); 
    
    return std::regex_match(val, regex);
}
