/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */

#include <cstdio>
#include <cstring>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"

static void CheckGrid(void);
static t_type_ptr find_type_col(INP int x);

#ifdef TORO_FABRIC_GRID_OVERRIDE 
//===========================================================================//
#include "TFH_FabricGridHandler.h"

static void alloc_and_load_block_override_ios(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny );
//===========================================================================//
#endif

#ifdef TORO_FABRIC_BLOCK_OVERRIDE 
//===========================================================================//
#include "TFH_FabricBlockHandler.h"

static t_type_ptr alloc_and_load_block_override_type(
		t_type_ptr type, int x, int y);

static void alloc_and_load_block_override_blocks(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny);
static void alloc_and_load_block_override_blocks_invalid(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny);
static void alloc_and_load_block_override_blocks_empty(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny,
		const TFH_GridBlockList_t& gridBlockList);
//===========================================================================//
#endif

static void alloc_and_load_num_instances_type(
		t_grid_tile** L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types);

/* Create and fill FPGA architecture grid.         */
void alloc_and_load_grid(INOUTP int *num_instances_type) {

	int i, j;
	t_type_ptr type;

#ifdef SHOW_ARCH
	FILE *dump;
#endif

	/* To remove this limitation, change ylow etc. in t_rr_node to        *
	 * * be ints instead.  Used shorts to save memory.                      */
	if ((nx > 32766) || (ny > 32766)) {
		vpr_printf(TIO_MESSAGE_ERROR, "nx and ny must be less than 32767, since the router uses shorts (16-bit) to store coordinates.\n");
		vpr_printf(TIO_MESSAGE_ERROR, "nx: %d, ny: %d\n", nx, ny);
		exit(1);
	}

	assert(nx >= 1 && ny >= 1);

	grid = (struct s_grid_tile **) alloc_matrix(0, (nx + 1), 0, (ny + 1),
			sizeof(struct s_grid_tile));

	/* Clear the full grid to have no type (NULL), no capacity, etc */
	for (i = 0; i <= (nx + 1); ++i) {
		for (j = 0; j <= (ny + 1); ++j) {
			memset(&grid[i][j], 0, (sizeof(struct s_grid_tile)));
		}
	}

	/* Nothing goes in the corners. */
	grid[0][0].type = grid[nx + 1][0].type = EMPTY_TYPE;
	grid[0][ny + 1].type = grid[nx + 1][ny + 1].type = EMPTY_TYPE;

	for (i = 1; i <= nx; i++) {
		grid[i][0].type = IO_TYPE;
		grid[i][ny + 1].type = IO_TYPE;

		grid[i][0].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);
		grid[i][ny + 1].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);

		for (j = 0; j < IO_TYPE->capacity; j++) {
			grid[i][0].blocks[j] = EMPTY;
			grid[i][ny + 1].blocks[j] = EMPTY;
		}
	}

	for (i = 1; i <= ny; i++) {
		grid[0][i].type = IO_TYPE;
		grid[nx + 1][i].type = IO_TYPE;

		grid[0][i].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);
		grid[nx + 1][i].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);

		for (j = 0; j < IO_TYPE->capacity; j++) {
			grid[0][i].blocks[j] = EMPTY;
			grid[nx + 1][i].blocks[j] = EMPTY;
		}
	}

	for (i = 1; i <= nx; i++) { /* Interior (LUT) cells */
		type = find_type_col(i);
		for (j = 1; j <= ny; j++) {

			if (grid[i][j].type == EMPTY_TYPE)
				continue;

			if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0)
				continue;

#ifdef TORO_FABRIC_BLOCK_OVERRIDE 
			type = alloc_and_load_block_override_type(type, i, j);
#endif

			if (i + type->width - 1 <= nx && j + type->height - 1 <= ny) {
				for (int i_offset = 0; i_offset < type->width; ++i_offset) {
					for (int j_offset = 0; j_offset < type->height; ++j_offset) {
						grid[i+i_offset][j+j_offset].type = type;
						grid[i+i_offset][j+j_offset].width_offset = i_offset;
						grid[i+i_offset][j+j_offset].height_offset = j_offset;
						grid[i+i_offset][j+j_offset].blocks = (int *) my_malloc(sizeof(int) * type->capacity);
						grid[i+i_offset][j+j_offset].blocks[0] = EMPTY;
						for (int k = 0; k < type->capacity; ++k) {
							grid[i+i_offset][j+j_offset].blocks[k] = EMPTY;
						}
					}
				}
			} else {
				grid[i][j].type = EMPTY_TYPE;
				grid[i][j].blocks = (int *) my_malloc(sizeof(int));
				grid[i][j].blocks[0] = EMPTY;
			}
		}
	}

#ifdef TORO_FABRIC_GRID_OVERRIDE 
	alloc_and_load_block_override_ios(grid, nx, ny);
#endif

#ifdef TORO_FABRIC_BLOCK_OVERRIDE 
	alloc_and_load_block_override_blocks(grid, nx, ny);
#endif

	// And, refresh (ie. reset and update) the "num_instances_type" array
	// (while also forcing any remaining INVALID blocks to EMPTY_TYPE)
	alloc_and_load_num_instances_type(grid, nx, ny,	num_instances_type, num_types);

	CheckGrid();

#ifdef SHOW_ARCH
	/* DEBUG code */
	dump = my_fopen("grid_type_dump.txt", "w", 0);
	for (j = (ny + 1); j >= 0; --j)
	{
		for (i = 0; i <= (nx + 1); ++i)
		{
			fprintf(dump, "%c", grid[i][j].type->name[1]);
		}
		fprintf(dump, "\n");
	}
	fclose(dump);
#endif
}

#ifdef TORO_FABRIC_GRID_OVERRIDE 
//===========================================================================//
static void alloc_and_load_block_override_ios(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny ) {

	const TFH_FabricGridHandler_c& fabricGridHandler = TFH_FabricGridHandler_c::GetInstance();
	if (fabricGridHandler.IsValid()) {

		/* Iterate over the existing grid (IOs and CLBs) to adjust IO ring (as needed) */
		const TGO_Polygon_c& ioPolygon = fabricGridHandler.GetPolygon( );

		/* First pass, iterate grid to force type EMPTY for any locations outside the IO ring */
		for (int x = 0; x <= (vpr_nx + 1); ++x) {
			for (int y = 0; y <= (vpr_ny + 1); ++y) {
				if (ioPolygon.IsWithin(x, y)) 
					continue;
				if (ioPolygon.IsEdge(x, y) && !ioPolygon.IsConvexCorner(x, y)) 
					continue;

				free(grid[x][y].blocks);
				grid[x][y].type = EMPTY_TYPE;
				grid[x][y].blocks = (int *) my_malloc(sizeof(int));
				grid[x][y].blocks[0] = EMPTY;
			}
		}
	
		// Second pass, iterate grid to force type IO for any locations on the IO ring */
		for (int x = 0; x <= (vpr_nx + 1); ++x) {
			for (int y = 0; y <= (vpr_ny + 1); ++y) {
				if (!ioPolygon.IsEdge(x, y) || ioPolygon.IsConvexCorner(x, y))
					continue;

				free(grid[x][y].blocks);
				grid[x][y].type = IO_TYPE;
				grid[x][y].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);
				for (int i = 0; i < IO_TYPE->capacity; ++i) {
					grid[x][y].blocks[i] = EMPTY;
				}
			}
		}
	}
}
//===========================================================================//
#endif

#ifdef TORO_FABRIC_BLOCK_OVERRIDE 
//===========================================================================//
static t_type_ptr alloc_and_load_block_override_type(
		t_type_ptr type, int x, int y) {

	const TFH_FabricBlockHandler_c& fabricBlockHandler = TFH_FabricBlockHandler_c::GetInstance();
	if (fabricBlockHandler.IsValid()) {

		vpr_printf(TIO_MESSAGE_INFO, "Overriding architecture block[%d][%d] based on fabric %s block...\n", x, y, type->name);

		// Search for override type based on given block grid list
		const TFH_GridBlockList_t& gridBlockList = fabricBlockHandler.GetGridBlockList( );
		TFH_GridBlock_c gridBlock(x, y);
		if( gridBlockList.Find(gridBlock, &gridBlock))
		{
			int i = gridBlock.GetTypeIndex( );
			type = &type_descriptors[i];
		}
	}
	return(type);
}

//===========================================================================//
static void alloc_and_load_block_override_blocks(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny ) {

	const TFH_FabricBlockHandler_c& fabricBlockHandler = TFH_FabricBlockHandler_c::GetInstance();
	if (fabricBlockHandler.IsValid()) {

		// Iterate VPR's grid to initially force all blocks to INVALID
		alloc_and_load_block_override_blocks_invalid(vpr_grid, vpr_nx, vpr_ny);

		// Then, iterate valid grid list to force unused blocks to EMPTY
		const TFH_GridBlockList_t& gridBlockList = fabricBlockHandler.GetGridBlockList( );
		alloc_and_load_block_override_blocks_empty(vpr_grid, vpr_nx, vpr_ny,
				gridBlockList);
	}
}

//===========================================================================//
static void alloc_and_load_block_override_blocks_invalid(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny) {

	// Iterate grid array to force all blocks to INVALID
	for (int x = 0; x <= (vpr_nx + 1); ++x) {
		for (int y = 0; y <= (vpr_ny + 1); ++y) {

			if (!vpr_grid[x][y].type)
				continue;

			if (vpr_grid[x][y].type == EMPTY_TYPE)
				continue;

			for (int z = 0; z < vpr_grid[x][y].type->capacity; ++z) {
				vpr_grid[x][y].blocks[z] = INVALID;
			}
		}
	}
}

//===========================================================================//
static void alloc_and_load_block_override_blocks_empty(
		t_grid_tile** vpr_grid, int vpr_nx, int vpr_ny,
		const TFH_GridBlockList_t& gridBlockList) {

	// Iterate optional valid grid list to force selected blocks to EMPTY
	for (size_t i = 0; i < gridBlockList.GetLength( ); ++i) {

		const TFH_GridBlock_c& gridBlock = *gridBlockList[i];
		TFH_BlockType_t blockType = gridBlock.GetBlockType( );
		const TGO_Point_c& gridPoint = gridBlock.GetGridPoint( );

		int x = gridPoint.x;
		int y = gridPoint.y;
		int z = gridPoint.z;

		if (x < 0 || x > (vpr_nx + 1) || y < 0 || y > (vpr_ny + 1))
			continue;

		if (!vpr_grid[x][y].type )
			continue;

		if (z < 0 || z > vpr_grid[x][y].type->capacity - 1)
			continue;

		if (vpr_grid[x][y].type == EMPTY_TYPE)
			continue;

		if (blockType == TFH_BLOCK_INPUT_OUTPUT && vpr_grid[x][y].type != IO_TYPE)
			continue;

		if (blockType != TFH_BLOCK_INPUT_OUTPUT && vpr_grid[x][y].type == IO_TYPE)
			continue;

		int dx = vpr_grid[x][y].type->width;
		int dy = vpr_grid[x][y].type->height;
		for (x = gridPoint.x; x <= min( gridPoint.x + dx - 1, nx + 1 ); ++x) {
			for (y = gridPoint.y ; y <= min( gridPoint.y + dy - 1, ny + 1 ); ++y) {
				vpr_grid[x][y].blocks[z] = EMPTY;
			}
		}
	}
}
//===========================================================================//
#endif

//===========================================================================//
static void alloc_and_load_num_instances_type(
		t_grid_tile** L_grid, int L_nx, int L_ny,
		int* L_num_instances_type, int L_num_types) {

	for (int i = 0; i < L_num_types; ++i) {
		L_num_instances_type[i] = 0;
	}

	for (int x = 0; x <= (L_nx + 1); ++x) {
		for (int y = 0; y <= (L_ny + 1); ++y) {

			if (!L_grid[x][y].type)
				continue;

			bool isValid = false;
			for (int z = 0; z < L_grid[x][y].type->capacity; ++z) {

				if (L_grid[x][y].blocks[z] == INVALID) {
					L_grid[x][y].blocks[z] = EMPTY;
				} else {
					isValid = true;
				}
			}
			if (!isValid) {
				L_grid[x][y].type = EMPTY_TYPE;
				L_grid[x][y].width_offset = 0;
				L_grid[x][y].height_offset = 0;
			}

			if (L_grid[x][y].width_offset > 0 || L_grid[x][y].height_offset > 0) {
				continue;
			}

			if (L_grid[x][y].type == EMPTY_TYPE) {
				++L_num_instances_type[EMPTY_TYPE->index];
				continue;
			}

			for (int z = 0; z < L_grid[x][y].type->capacity; ++z) {

				if (L_grid[x][y].type == IO_TYPE) {
					++L_num_instances_type[IO_TYPE->index];
				} else {
					++L_num_instances_type[L_grid[x][y].type->index];
				}
			}
		}
	}
}

void freeGrid(void) {

	int i, j;
	if (grid == NULL) {
		return;
	}

	for (i = 0; i <= (nx + 1); ++i) {
		for (j = 0; j <= (ny + 1); ++j) {
			free(grid[i][j].blocks);
		}
	}
	free_matrix(grid, 0, nx + 1, 0, sizeof(struct s_grid_tile));
	grid = NULL;
}

static void CheckGrid(void) {

	int i, j;

	/* Check grid is valid */
	for (i = 0; i <= (nx + 1); ++i) {
		for (j = 0; j <= (ny + 1); ++j) {
			if (NULL == grid[i][j].type) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has no type.\n", i, j);
				exit(1);
			}

			if (grid[i][j].usage != 0) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has non-zero usage (%d) before netlist load.\n", i, j, grid[i][j].usage);
				exit(1);
			}

			if ((grid[i][j].width_offset < 0)
					|| (grid[i][j].width_offset >= grid[i][j].type->width)) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has invalid width offset (%d).\n", i, j, grid[i][j].width_offset);
				exit(1);
			}
			if ((grid[i][j].height_offset < 0)
					|| (grid[i][j].height_offset >= grid[i][j].type->height)) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has invalid height offset (%d).\n", i, j, grid[i][j].height_offset);
				exit(1);
			}

			if ((NULL == grid[i][j].blocks)
					&& (grid[i][j].type->capacity > 0)) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has no block list allocated.\n", i, j);
				exit(1);
			}
		}
	}
}

static t_type_ptr find_type_col(INP int x) {

	int i, j;
	int start, repeat;
	float rel;
	boolean match;
	int priority, num_loc;
	t_type_ptr column_type;

	priority = FILL_TYPE->grid_loc_def[0].priority;
	column_type = FILL_TYPE;

	for (i = 0; i < num_types; i++) {
		if (&type_descriptors[i] == IO_TYPE
				|| &type_descriptors[i] == EMPTY_TYPE
				|| &type_descriptors[i] == FILL_TYPE)
			continue;
		num_loc = type_descriptors[i].num_grid_loc_def;
		for (j = 0; j < num_loc; j++) {
			if (priority < type_descriptors[i].grid_loc_def[j].priority) {
				match = FALSE;
				if (type_descriptors[i].grid_loc_def[j].grid_loc_type
						== COL_REPEAT) {
					start = type_descriptors[i].grid_loc_def[j].start_col;
					repeat = type_descriptors[i].grid_loc_def[j].repeat;
					if (start < 0) {
						start += (nx + 1);
					}
					if (x == start) {
						match = TRUE;
					} else if (repeat > 0 && x > start && start > 0) {
						if ((x - start) % repeat == 0) {
							match = TRUE;
						}
					}
				} else if (type_descriptors[i].grid_loc_def[j].grid_loc_type
						== COL_REL) {
					rel = type_descriptors[i].grid_loc_def[j].col_rel;
					if (nint(rel * nx) == x) {
						match = TRUE;
					}
				}
				if (match) {
					priority = type_descriptors[i].grid_loc_def[j].priority;
					column_type = &type_descriptors[i];
				}
			}
		}
	}
	return column_type;
}
