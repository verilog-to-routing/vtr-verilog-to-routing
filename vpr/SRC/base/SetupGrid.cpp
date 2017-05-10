/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block device_ctx.grid for VPR.

 */

#include <cstdio>
#include <cstring>
using namespace std;

#include "vtr_assert.h"
#include "vtr_matrix.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"

static void CheckGrid(void);
static t_type_ptr find_type_col(const int x);

static void alloc_and_load_num_instances_type(
		t_grid_tile** L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types);

/* Create and fill FPGA architecture grid.         */
void alloc_and_load_grid(int *num_instances_type) {

#ifdef SHOW_ARCH
	FILE *dump;
#endif

    auto& device_ctx = g_vpr_ctx.mutable_device();

	/* To remove this limitation, change ylow etc. in t_rr_node to        *
	 * * be ints instead.  Used shorts to save memory.                      */
	if ((device_ctx.nx > 32766) || (device_ctx.ny > 32766)) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
			"Device width and height must be less than 32767, since the router uses shorts (16-bit) to store coordinates.\n"
			"device_ctx.nx: %d, device_ctx.ny: %d\n", device_ctx.nx, device_ctx.ny);
	}

	VTR_ASSERT(device_ctx.nx >= 1 && device_ctx.ny >= 1);

	device_ctx.grid = vtr::alloc_matrix<t_grid_tile>(0, (device_ctx.nx + 1), 0, (device_ctx.ny + 1));

	/* Clear the full device_ctx.grid to have no type (NULL), no capacity, etc */
	for (int x = 0; x <= (device_ctx.nx + 1); ++x) {
		for (int y = 0; y <= (device_ctx.ny + 1); ++y) {
			memset(&device_ctx.grid[x][y], 0, (sizeof(t_grid_tile)));
		}
	}

	for (int x = 0; x <= device_ctx.nx + 1; ++x) {
		for (int y = 0; y <= device_ctx.ny + 1; ++y) {

			t_type_ptr type = 0;
			if ((x == 0 && y == 0) || (x == 0 && y == device_ctx.ny + 1) || (x == device_ctx.nx + 1 && y == 0) || (x == device_ctx.nx + 1 && y == device_ctx.ny + 1)) {

				// Assume corners are empty type (by default)
				type = device_ctx.EMPTY_TYPE;

			} else if (x == 0 || y == 0 || x == device_ctx.nx + 1 || y == device_ctx.ny + 1) {

				// Assume edges are IO type (by default)
				type = device_ctx.IO_TYPE;

			} else {

				if (device_ctx.grid[x][y].width_offset > 0 || device_ctx.grid[x][y].height_offset > 0)
					continue;

				// Assume core are not empty and not IO types (by default)
				type = find_type_col(x);
			}

			if (x + type->width - 1 <= device_ctx.nx && y + type->height - 1 <= device_ctx.ny) {
				for (int x_offset = 0; x_offset < type->width; ++x_offset) {
					for (int y_offset = 0; y_offset < type->height; ++y_offset) {
						device_ctx.grid[x+x_offset][y+y_offset].type = type;
						device_ctx.grid[x+x_offset][y+y_offset].width_offset = x_offset;
						device_ctx.grid[x+x_offset][y+y_offset].height_offset = y_offset;
						device_ctx.grid[x+x_offset][y+y_offset].blocks = (int *) vtr::malloc(sizeof(int) * max(1,type->capacity));
						for (int i = 0; i < max(1,type->capacity); ++i) {
							device_ctx.grid[x+x_offset][y+y_offset].blocks[i] = EMPTY_BLOCK;
						}
					}
				}
			} else if (type == device_ctx.IO_TYPE ) {
				device_ctx.grid[x][y].type = type;
				device_ctx.grid[x][y].blocks = (int *) vtr::malloc(sizeof(int) * max(1,type->capacity));
				for (int i = 0; i < max(1,type->capacity); ++i) {
					device_ctx.grid[x][y].blocks[i] = EMPTY_BLOCK;
				}
			} else {
				device_ctx.grid[x][y].type = device_ctx.EMPTY_TYPE;
				device_ctx.grid[x][y].blocks = (int *) vtr::malloc(sizeof(int));
				device_ctx.grid[x][y].blocks[0] = EMPTY_BLOCK;
			}
		}
	}

	// And, refresh (ie. reset and update) the "num_instances_type" array
	// (while also forcing any remaining INVALID_BLOCK blocks to device_ctx.EMPTY_TYPE)
	alloc_and_load_num_instances_type(device_ctx.grid, device_ctx.nx, device_ctx.ny,	num_instances_type, device_ctx.num_block_types);

	CheckGrid();

#if 0
	for (int y = 0; y <= device_ctx.ny + 1; ++y) {
		for (int x = 0; x <= device_ctx.nx + 1; ++x) {
			vtr::printf_info("[%d][%d] %s\n", x, y, device_ctx.grid[x][y].type->name);
		}
	}
#endif

#ifdef SHOW_ARCH
	/* debug code */
	dump = my_fopen("grid_type_dump.txt", "w", 0);
	for (j = (device_ctx.ny + 1); j >= 0; --j)
	{
		for (i = 0; i <= (device_ctx.nx + 1); ++i)
		{
			fprintf(dump, "%c", device_ctx.grid[i][j].type->name[1]);
		}
		fprintf(dump, "\n");
	}
	fclose(dump);
#endif
}

//===========================================================================//
static void alloc_and_load_num_instances_type(
		t_grid_tile** L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types) {

    auto& device_ctx = g_vpr_ctx.device();

	for (int i = 0; i < L_num_types; ++i) {
		L_num_instances_type[i] = 0;
	}

	for (int x = 0; x <= (L_nx + 1); ++x) {
		for (int y = 0; y <= (L_ny + 1); ++y) {

			if (!L_grid[x][y].type)
				continue;

			bool isValid = false;
			for (int z = 0; z < L_grid[x][y].type->capacity; ++z) {

				if (L_grid[x][y].blocks[z] == INVALID_BLOCK) {
					L_grid[x][y].blocks[z] = EMPTY_BLOCK;
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

	int i, j;
	if (device_ctx.grid == NULL) {
		return;
	}

	for (i = 0; i <= (device_ctx.nx + 1); ++i) {
		for (j = 0; j <= (device_ctx.ny + 1); ++j) {
			free(device_ctx.grid[i][j].blocks);
		}
	}
    vtr::free_matrix(device_ctx.grid, 0, device_ctx.nx + 1, 0);
	device_ctx.grid = NULL;
}

static void CheckGrid(void) {

	int i, j;

    auto& device_ctx = g_vpr_ctx.device();

	/* Check grid is valid */
	for (i = 0; i <= (device_ctx.nx + 1); ++i) {
		for (j = 0; j <= (device_ctx.ny + 1); ++j) {
			if (NULL == device_ctx.grid[i][j].type) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "device_ctx.grid[%d][%d] has no type.\n", i, j);
			}

			if (device_ctx.grid[i][j].usage != 0) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "device_ctx.grid[%d][%d] has non-zero usage (%d) before netlist load.\n", i, j, device_ctx.grid[i][j].usage);
			}

			if ((device_ctx.grid[i][j].width_offset < 0)
					|| (device_ctx.grid[i][j].width_offset >= device_ctx.grid[i][j].type->width)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "device_ctx.grid[%d][%d] has invalid width offset (%d).\n", i, j, device_ctx.grid[i][j].width_offset);
			}
			if ((device_ctx.grid[i][j].height_offset < 0)
					|| (device_ctx.grid[i][j].height_offset >= device_ctx.grid[i][j].type->height)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "device_ctx.grid[%d][%d] has invalid height offset (%d).\n", i, j, device_ctx.grid[i][j].height_offset);
			}

			if ((NULL == device_ctx.grid[i][j].blocks)
					&& (device_ctx.grid[i][j].type->capacity > 0)) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "device_ctx.grid[%d][%d] has no block list allocated.\n", i, j);
			}
		}
	}
}

static t_type_ptr find_type_col(const int x) {

	int i, j;
	int start, repeat;
	float rel;
	bool match;
	int priority, num_loc;
	t_type_ptr column_type;

    auto& device_ctx = g_vpr_ctx.device();

	priority = device_ctx.FILL_TYPE->grid_loc_def[0].priority;
	column_type = device_ctx.FILL_TYPE;

	for (i = 0; i < device_ctx.num_block_types; i++) {
		if (&device_ctx.block_types[i] == device_ctx.IO_TYPE
				|| &device_ctx.block_types[i] == device_ctx.EMPTY_TYPE
				|| &device_ctx.block_types[i] == device_ctx.FILL_TYPE)
			continue;
		num_loc = device_ctx.block_types[i].num_grid_loc_def;
		for (j = 0; j < num_loc; j++) {
			if (priority < device_ctx.block_types[i].grid_loc_def[j].priority) {
				match = false;
				if (device_ctx.block_types[i].grid_loc_def[j].grid_loc_type
						== COL_REPEAT) {
					start = device_ctx.block_types[i].grid_loc_def[j].start_col;
					repeat = device_ctx.block_types[i].grid_loc_def[j].repeat;
					if (start < 0) {
						start += (device_ctx.nx + 1);
					}
					if (x == start) {
						match = true;
					} else if (repeat > 0 && x > start && start > 0) {
						if ((x - start) % repeat == 0) {
							match = true;
						}
					}
				} else if (device_ctx.block_types[i].grid_loc_def[j].grid_loc_type
						== COL_REL) {
					rel = device_ctx.block_types[i].grid_loc_def[j].col_rel;
					if (vtr::nint(rel * device_ctx.nx) == x) {
						match = true;
					}
				}
				if (match) {
					priority = device_ctx.block_types[i].grid_loc_def[j].priority;
					column_type = &device_ctx.block_types[i];
				}
			}
		}
	}
	return column_type;
}
