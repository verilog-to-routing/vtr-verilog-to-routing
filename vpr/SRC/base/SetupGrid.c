/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"

static void CheckGrid(void);
static t_type_ptr find_type_col(INP int x);

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

	for (i = 0; i < num_types; i++) {
		num_instances_type[i] = 0;
	}

	/* Nothing goes in the corners. */
	grid[0][0].type = grid[nx + 1][0].type = EMPTY_TYPE;
	grid[0][ny + 1].type = grid[nx + 1][ny + 1].type = EMPTY_TYPE;
	num_instances_type[EMPTY_TYPE->index] = 4;

	for (i = 1; i <= nx; i++) {
		grid[i][0].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);
		grid[i][0].type = IO_TYPE;

		grid[i][ny + 1].blocks = (int *) my_malloc(
				sizeof(int) * IO_TYPE->capacity);
		grid[i][ny + 1].type = IO_TYPE;

		for (j = 0; j < IO_TYPE->capacity; j++) {
			grid[i][0].blocks[j] = EMPTY;
			grid[i][ny + 1].blocks[j] = EMPTY;
		}
	}

	for (i = 1; i <= ny; i++) {
		grid[0][i].blocks = (int *) my_malloc(sizeof(int) * IO_TYPE->capacity);
		grid[0][i].type = IO_TYPE;

		grid[nx + 1][i].blocks = (int *) my_malloc(
				sizeof(int) * IO_TYPE->capacity);
		grid[nx + 1][i].type = IO_TYPE;
		for (j = 0; j < IO_TYPE->capacity; j++) {
			grid[0][i].blocks[j] = EMPTY;
			grid[nx + 1][i].blocks[j] = EMPTY;
		}
	}

	num_instances_type[IO_TYPE->index] = 2 * IO_TYPE->capacity * (nx + ny);

	for (i = 1; i <= nx; i++) { /* Interior (LUT) cells */
		type = find_type_col(i);
		for (j = 1; j <= ny; j++) {
			grid[i][j].type = type;
			grid[i][j].offset = (j - 1) % type->height;
			if (j + grid[i][j].type->height - 1 - grid[i][j].offset > ny) {
				grid[i][j].type = EMPTY_TYPE;
				grid[i][j].offset = 0;
			}

			if (type->capacity > 1) {
				vpr_printf(TIO_MESSAGE_ERROR, "in FillArch(), expected core blocks to have capacity <= 1 but (%d, %d) has type '%s' and capacity %d.\n", 
						i, j, grid[i][j].type->name, grid[i][j].type->capacity);
				exit(1);
			}

			grid[i][j].blocks = (int *) my_malloc(sizeof(int));
			grid[i][j].blocks[0] = EMPTY;
			if (grid[i][j].offset == 0) {
				num_instances_type[grid[i][j].type->index]++;
			}
		}
	}

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

void freeGrid() {
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

static void CheckGrid() {
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

			if ((grid[i][j].offset < 0)
					|| (grid[i][j].offset >= grid[i][j].type->height)) {
				vpr_printf(TIO_MESSAGE_ERROR, "grid[%d][%d] has invalid offset (%d).\n", i, j, grid[i][j].offset);
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
