/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */

#include <cstdio>
#include <cstring>
#include <algorithm>
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

static void CheckGrid(void);

static void set_grid_block_type(int priority, const t_type_descriptor* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<t_grid_blocks>& grid_blocks, vtr::Matrix<int>& grid_priorities);

static void alloc_and_load_num_instances_type(
		vtr::Matrix<t_grid_tile>& L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types);

/* Create and fill FPGA architecture grid.         */
void alloc_and_load_grid(std::vector<t_grid_def> grid_layouts, int *num_instances_type) {
    //TODO: handle properly
    VTR_ASSERT(grid_layouts.size() == 1);
    auto grid_loc_defs = grid_layouts[0].loc_defs;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	/* To remove this limitation, change ylow etc. in t_rr_node to        *
	 * * be ints instead.  Used shorts to save memory.                      */
	if ((device_ctx.nx > 32766) || (device_ctx.ny > 32766)) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
			"Device width and height must be less than 32767, since the router uses shorts (16-bit) to store coordinates.\n"
			"device_ctx.nx: %d, device_ctx.ny: %d\n", device_ctx.nx, device_ctx.ny);
	}

    //Size and allocate the device
	VTR_ASSERT(device_ctx.nx >= 1 && device_ctx.ny >= 1);
    size_t W = device_ctx.nx + 2;
    size_t H = device_ctx.ny + 2;

	auto grid = vtr::Matrix<t_grid_tile>({W, H});

    //Ensure ther is space in the reverse grid to block look-up
    place_ctx.grid_blocks.resize({W, H});


    //Track the current priority for each grid location
    // Note that we initialize it to the lowest (i.e. most negative) possible value, so
    // any user-specified priority will override the default empty grid
    auto grid_priorities = vtr::Matrix<int>({W, H}, std::numeric_limits<int>::lowest());

    //Initialize the device to all empty blocks
    for (size_t x = 0; x < W; ++x) {
        for (size_t y = 0; y < H; ++y) {
            set_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                    device_ctx.EMPTY_TYPE, x, y, grid, place_ctx.grid_blocks, grid_priorities);
        }
    }

    //Sort the grid specifications by priority
    // Note that we us a stable sort to respect the architecture file order in the ambiguous case
    // of specifications with equal priorities
    auto priority_order = [](const t_grid_loc_def& lhs, const t_grid_loc_def& rhs) {
        return lhs.priority < rhs.priority;
    };
    std::stable_sort(grid_loc_defs.begin(), grid_loc_defs.end(), priority_order);

    t_formula_data vars;
    vars.set_var_value("W", W);
    vars.set_var_value("H", H);

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

        //vtr::printf("Applying grid_loc_def for '%s' priority %d\n", type->name, grid_loc_def.priority);

        //Load the x specification
        auto& xspec = grid_loc_def.x;

        VTR_ASSERT_MSG(!xspec.start_expr.empty(), "x start position must be specified");
        VTR_ASSERT_MSG(!xspec.end_expr.empty(), "x end position must be specified");

        size_t startx = parse_formula(xspec.start_expr, vars);
        size_t endx = parse_formula(xspec.end_expr, vars);

        size_t incrx = type->width; //Default to block width
        if (!xspec.incr_expr.empty()) {
            //Use expression if specified
            incrx = parse_formula(xspec.incr_expr, vars);
        }

        size_t repeatx = W; //Default to no repeats
        if (!xspec.repeat_expr.empty()) {
            //Use expression if specified
            repeatx = parse_formula(xspec.repeat_expr, vars);
        }

        //Load the y specification
        auto& yspec = grid_loc_def.y;

        VTR_ASSERT_MSG(!yspec.start_expr.empty(), "y start position must be specified");
        VTR_ASSERT_MSG(!yspec.end_expr.empty(), "y end position must be specified");

        size_t starty = parse_formula(yspec.start_expr, vars);
        size_t endy = parse_formula(yspec.end_expr, vars);

        size_t incry = type->height; //Default to block width
        if (!yspec.incr_expr.empty()) {
            //Use expression if specified
            incry = parse_formula(yspec.incr_expr, vars);
        }

        size_t repeaty = H; //Default to no repeats
        if (!yspec.repeat_expr.empty()) {
            //Use expression if specified
            repeaty = parse_formula(yspec.repeat_expr, vars);
        }

        VTR_ASSERT(repeatx > 0);
        VTR_ASSERT(repeaty > 0);

        //Warn if start and end fall outside the device dimensions
        if (startx > W - 1) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification startx (%d) falls outside device horizontal range [%d,%d]\n",
                    type->name, startx, 0, W - 1);
        }

        if (endx > W - 1) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification endx (%d) falls outside device horizontal range [%d,%d]\n",
                    type->name, endx, 0, W - 1);
        }

        if (starty > H - 1) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification starty (%d) falls outside device vertical range [%d,%d]\n",
                    type->name, starty, 0, H - 1);
        }

        if (endy > H - 1) {
            vtr::printf_warning(__FILE__, __LINE__,
                    "Block type '%s' grid location specification endy (%d) falls outside device vertical range [%d,%d]\n",
                    type->name, endy, 0, H - 1);
        }

        //The end must fall after (or equal) to the start
        if (endx < startx) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification endx (%d) can not come before startx (%d) for block type '%s'",
                    endx, startx, type->name);
        }

        if (endy < starty) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification endy (%d) can not come before starty (%d) for block type '%s'",
                    endy, starty, type->name);
        }

        //The minimum increment is the block dimension
        VTR_ASSERT(type->width > 0);
        if (incrx < size_t(type->width)) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification x increment for block type '%s' must be at least block width (%d) to avoid overlapping instances (was %d)",
                    type->name, type->width, incrx);
        }

        VTR_ASSERT(type->height > 0);
        if (incry < size_t(type->height)) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification y increment for block type '%s' must be at least block height (%d) to avoid overlapping instances (was %d)",
                    type->name, type->height, incry);
        }

        //The minimum repeat is the region dimension
        size_t region_width = endx - startx + 1; //+1 since start/end are both inclusive
        if (repeatx < region_width) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification x repeat for block type '%s' must be at least the region width (%d) to avoid overlapping instances (was %d)",
                    type->name, region_width, repeatx);
        }

        size_t region_height = endy - starty + 1; //+1 since start/end are both inclusive
        if (repeaty < region_height) {
            VPR_THROW(VPR_ERROR_ARCH, 
                    "Grid location specification y repeat for block type '%s' must be at least the region height (%d) to avoid overlapping instances (was %d)",
                    type->name, region_height, repeaty);
        }

        size_t x_end = 0;
        for (size_t kx = 0; x_end < W; ++kx) {
            size_t x_start = startx + kx * repeatx;
            x_end = endx + kx * repeatx;

            size_t y_end = 0;
            for (size_t ky = 0; y_end < H; ++ky) {
                size_t y_start = starty + ky * repeaty;
                y_end = endy + ky * repeaty;


                size_t x_max = std::min(x_end, W-1);
                size_t y_max = std::min(y_end, H-1);

                for(size_t x = x_start; x + (type->width - 1) <= x_max; x += incrx) {
                    for(size_t y = y_start; y + (type->height - 1) <= y_max; y += incry) {
                        set_grid_block_type(grid_loc_def.priority, type, x, y, grid, place_ctx.grid_blocks, grid_priorities);
                    }
                }
            }
        }
    }

    device_ctx.grid = DeviceGrid(grid);

    VTR_ASSERT(device_ctx.grid.nx() == device_ctx.nx);
    VTR_ASSERT(device_ctx.grid.ny() == device_ctx.ny);

	// And, refresh (ie. reset and update) the "num_instances_type" array
	// (while also forcing any remaining INVALID_BLOCK blocks to device_ctx.EMPTY_TYPE)
	alloc_and_load_num_instances_type(grid, device_ctx.nx, device_ctx.ny, num_instances_type, device_ctx.num_block_types);

	CheckGrid();


#if 0
    for (int x = 0; x < device_ctx.grid.width(); ++x) {
        for (int y = 0; y < device_ctx.grid.height(); ++y) {
            auto type = device_ctx.grid[x][y].type;
            if (type->width != 1 || type->height != 1) {
                vtr::printf_info("[%d][%d] %s (offset %d,%d)\n", x, y, device_ctx.grid[x][y].type->name, device_ctx.grid[x][y].width_offset, device_ctx.grid[x][y].height_offset);
            } else {
                vtr::printf_info("[%d][%d] %s\n", x, y, device_ctx.grid[x][y].type->name, device_ctx.grid[x][y].width_offset, device_ctx.grid[x][y].height_offset);
            }
		}
	}
#endif

#ifdef SHOW_ARCH
	/* debug code */
	FILE* dump = vtr::fopen("grid_type_dump.txt", "w", 0);
	for (int j = (device_ctx.ny + 1); j >= 0; --j)
	{
		for (int i = 0; i <= (device_ctx.nx + 1); ++i)
		{
			fprintf(dump, "%c", device_ctx.grid[i][j].type->name[1]);
		}
		fprintf(dump, "\n");
	}
	fclose(dump);
#endif
}

static void set_grid_block_type(int priority, const t_type_descriptor* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<t_grid_blocks>& grid_blocks, vtr::Matrix<int>& grid_priorities) {

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

            grid_blocks[x][y].blocks.resize(type->capacity, EMPTY_BLOCK);

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

                    grid_blocks[x][y].blocks.resize(type->capacity, EMPTY_BLOCK);

                    grid_priorities[x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
}

//===========================================================================//
static void alloc_and_load_num_instances_type(
		vtr::Matrix<t_grid_tile>& L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types) {

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	for (int i = 0; i < L_num_types; ++i) {
		L_num_instances_type[i] = 0;
	}

	for (int x = 0; x <= (L_nx + 1); ++x) {
		for (int y = 0; y <= (L_ny + 1); ++y) {

			if (!L_grid[x][y].type)
				continue;

			bool isValid = false;
			for (int z = 0; z < L_grid[x][y].type->capacity; ++z) {

				if (place_ctx.grid_blocks[x][y].blocks[z] == INVALID_BLOCK) {
					place_ctx.grid_blocks[x][y].blocks[z] = EMPTY_BLOCK;
				} else {
					isValid = true;
				}
			}
			if (!isValid) {
				L_grid[x][y].type = device_ctx.EMPTY_TYPE;
				L_grid[x][y].width_offset = 0;
				L_grid[x][y].height_offset = 0;
			}

			if (L_grid[x][y].width_offset > 0 || L_grid[x][y].height_offset > 0) {
				continue;
			}

			if (L_grid[x][y].type == device_ctx.EMPTY_TYPE) {
				++L_num_instances_type[device_ctx.EMPTY_TYPE->index];
				continue;
			}

			for (int z = 0; z < L_grid[x][y].type->capacity; ++z) {

				if (L_grid[x][y].type == device_ctx.IO_TYPE) {
					++L_num_instances_type[device_ctx.IO_TYPE->index];
				} else {
					++L_num_instances_type[L_grid[x][y].type->index];
				}
			}
		}
	}
}

void freeGrid(void) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

	device_ctx.grid.clear();
}

static void CheckGrid(void) {

	int i, j;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

	/* Check grid is valid */
	for (i = 0; i <= (device_ctx.nx + 1); ++i) {
		for (j = 0; j <= (device_ctx.ny + 1); ++j) {
            auto type = device_ctx.grid[i][j].type;
			if (NULL == type) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has no type.\n", i, j);
			}

			if (place_ctx.grid_blocks[i][j].usage != 0) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "place_ctx.grid[%d][%d] has non-zero usage (%d) before netlist load.\n", i, j, place_ctx.grid_blocks[i][j].usage);
			}

			if ((device_ctx.grid[i][j].width_offset < 0)
					|| (device_ctx.grid[i][j].width_offset >= type->width)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has invalid width offset (%d).\n", i, j, device_ctx.grid[i][j].width_offset);
			}
			if ((device_ctx.grid[i][j].height_offset < 0)
					|| (device_ctx.grid[i][j].height_offset >= type->height)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "Grid Location (%d,%d) has invalid height offset (%d).\n", i, j, device_ctx.grid[i][j].height_offset);
			}

            //Verify that type and width/height offsets are correct (e.g. for dimension > 1 blocks)
            if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                //From the root block check that all other blocks are correct
                for (int x = i; x < i + type->width; ++x) {
                    int x_offset = x - i;
                    for (int y = j; y < j + type->height; ++y) {
                        int y_offset = y - j;

                        auto& tile = device_ctx.grid[x][y];
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



			if (place_ctx.grid_blocks[i][j].blocks.empty() && (type->capacity > 0)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "place_ctx.grid[%d][%d] has no block list allocated.\n", i, j);
			}
		}
	}
}
