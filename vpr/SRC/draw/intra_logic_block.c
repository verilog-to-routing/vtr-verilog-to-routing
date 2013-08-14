#include <cstdio>
#include <algorithm>

#include <assert.h>

#include "intra_logic_block.h"
#include "globals.h"
#include "draw_global.h"

/* Subroutines local to this file. */
static int draw_internal_find_max_lvl(t_pb_type pb_type);
static void draw_internal_load_coords(t_pb_type pb_type, float parent_width, float parent_height);
static void draw_internal_calc_coords(t_pb_type pb_type, int parent_num_modes, int mode_index, 
									   int num_sibling, int pb_index, float parent_width, 
									   float parent_height, float *blk_width, float *blk_height);
static void draw_internal_alloc_pb_type(t_pb_type pb_type);


/* Subroutine definitions begin. */
void draw_internal_alloc_blk() {
	int i, max_subblk_level;
	size_t v_index; 
	
	draw_coords.subblk_info = new vector<t_draw_pb_info>;

	max_subblk_level = 0;
	for (i = 0; i < num_types; ++i) {
		/* Empty block has no sub_blocks */
		if (strcmp(type_descriptors[i].name, "<EMPTY>") == 0)
			continue;
		
		/* Determine the max number of sub_block levels in the FPGA */
		max_subblk_level = max(draw_internal_find_max_lvl(*type_descriptors[i].pb_type),
							   max_subblk_level);
	}

	/* size() member function in std::vector returns unsigned integral type */
	for (v_index = 0; v_index < draw_coords.subblk_info->size(); ++v_index) {
		/* Subblk_levels start counting at level 0. Therefore, need to add 1 to 
		 * max_subblk_level.
		 */
		draw_coords.subblk_info->at(v_index).hierarchy = new vector<t_draw_pb_hierarchy>(max_subblk_level);
	}
}


void draw_internal_init_blk() {
	int i;
	t_pb_type pb_type;
	float blk_width, blk_height;

	for (i = 0; i < num_types; ++i) {
		pb_type = *type_descriptors[i].pb_type;

		/* Calculate width and height of each physical logic block type. */
		blk_width = type_descriptors[i].width * draw_coords.tile_width;
		blk_height = type_descriptors[i].height * draw_coords.tile_width;

		draw_internal_load_coords(pb_type, blk_width, blk_height);
	}
}


void draw_internal_draw_subblk() {
	int i, j, k;
	int num_sub_tiles;
	float sub_tile_step;
	int height;
	float x1, y1, x2, y2;

	for (i = 0; i <= (nx + 1); i++) {
		for (j = 0; j <= (ny + 1); j++) {
			/* Only the first block of a group should control drawing */
			if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0) 
				continue;

			num_sub_tiles = grid[i][j].type->capacity;
			sub_tile_step = draw_coords.tile_width / num_sub_tiles;
			height = grid[i][j].type->height;

			/* Don't draw if tile capacity is zero. This includes corners. */
			if (num_sub_tiles == 0)
				continue;

			for (k = 0; k < num_sub_tiles; ++k) {
				/* Get coords of current sub_tile */
				if ((j < 1) || (j > ny)) { /* top and bottom fringes */
					/* Only for the top and bottom fringes (ie. io blocks place on top 
					 * and bottom edges of the chip), sub_tiles are placed next to each 
					 * other horizontally, because it is easier to visualize the 
					 * connection from each individual io block to other parts of the 
					 * chip when toggle_nets is selected.
					 */
					x1 = draw_coords.tile_x[i] + (k * sub_tile_step);
					y1 = draw_coords.tile_y[j];
					x2 = x1 + sub_tile_step;
					y2 = y1 + draw_coords.tile_width;

				}
				else {
					/* Sub_tiles are stacked on top of each other vertically. */
					x1 = draw_coords.tile_x[i];
					y1 = draw_coords.tile_y[j] + (k * sub_tile_step);
					x2 = x1 + draw_coords.tile_width;
					y2 = y1 + sub_tile_step;
				}
			}
		}
	}
}

			
static int draw_internal_find_max_lvl(t_pb_type pb_type) {
	int i, j;
	t_mode mode;
	int max_levels = 0;

	/* Add new pb_type into draw_coords.blk_info if pb_type
	 * is not a root-level pb
	 */
	if (pb_type.depth != 0)
		draw_internal_alloc_pb_type(pb_type);

	/* If no modes, we have reached the end of pb_graph */
	if (pb_type.num_modes == 0)
		return (pb_type.depth);

	for (i = 0; i < pb_type.num_modes; ++i) {
		mode = pb_type.modes[i];

		if (mode.num_pb_type_children == 0)
			continue;

		for (j = 0; j < mode.num_pb_type_children; ++j) {
			max_levels = max(draw_internal_find_max_lvl(mode.pb_type_children[j]), max_levels);
		}
	}

	return max_levels;
}


static void draw_internal_alloc_pb_type(t_pb_type pb_type) {
	size_t i;
	t_draw_pb_info new_pb_type;

	for (i = 0; i < draw_coords.subblk_info->size(); ++i) {
		/* Check if the pb_type already exists in blk_info */
		if (pb_type.name == draw_coords.subblk_info->at(i).blk_type)
			return;
	}

	new_pb_type.blk_type = pb_type.name;
	new_pb_type.hierarchy = NULL;
	draw_coords.subblk_info->push_back(new_pb_type);
}


static void draw_internal_load_coords(t_pb_type pb_type, float parent_width, float parent_height) {
	int i, j;
	int num_modes;
	t_mode mode;
	int num_children;
	t_pb_type pb_type_child;
	float blk_width = 0.;
	float blk_height = 0.;

	num_modes = pb_type.num_modes;

	/* If no modes, we have reached the end of pb_graph */
	if (num_modes == 0)
		return;

	for (i = 0; i < num_modes; ++i) {
		mode = pb_type.modes[i];
		num_children = mode.num_pb_type_children;

		if (mode.num_pb_type_children == 0)
			continue;

		for (j = 0; j < num_children; ++j) {
			pb_type_child = mode.pb_type_children[j];

			/* Compute bound box for sub_block. Don't call if pb_type is root-level pb. */
			if (pb_type.depth != 0)
				draw_internal_calc_coords(pb_type, num_modes, i, num_children, j, parent_width,
										  parent_height, &blk_width, &blk_height);
			
			/* Traverse to next level in the pb_graph */
			draw_internal_load_coords(pb_type_child, blk_width, blk_height);
		}
	}
}


static void 
draw_internal_calc_coords(t_pb_type pb_type, int parent_num_modes, int mode_index, 
						  int num_sibling, int pb_index, float parent_width, float parent_height, 
						  float *blk_width, float *blk_height) 
{
	size_t i;
	int hierarchy;
	float drawing_width, drawing_height;
	float sub_tile_x, sub_tile_y;
	float width_offset, height_offset;
	float start_x, start_y, end_x, end_y;

	/* Draw all child-level blocks in 90% of the space inside their parent block. */
	drawing_width = parent_width * 0.9;
	drawing_height = parent_height * 0.9;

	/* The left and bottom corner (inside the parent block) of the space to draw 
	 * child blocks. 
	 */
	sub_tile_x = parent_width * 0.05;
	sub_tile_y = parent_height * 0.05;

	/* Divide drawing_width by the number of child types. */
	width_offset = drawing_width/num_sibling;
	/* Divide drawing_height by the number of modes that the parent contains. */
	height_offset = drawing_height/parent_num_modes;

	/* The starting point to draw the pb_type */
	start_x = width_offset * mode_index + sub_tile_x;
	start_y = height_offset * pb_index + sub_tile_y;

	/* Leave some space between different pb_types. */
	width_offset *= 0.95;
	/* Leave some space between different modes. */
	height_offset *= 0.95;

	/* Endpoint for drawing the pb_type */
	end_x = start_x + width_offset;
	end_y = start_y + height_offset;

	for (i = 0; i < draw_coords.subblk_info->size(); ++i) {
		/* Check if the pb_type already exists in blk_info */
		if (pb_type.name == draw_coords.subblk_info->at(i).blk_type) {
			hierarchy = pb_type.depth;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_bbox.xleft = start_x;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_bbox.xright = end_x;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_bbox.ybottom = start_y;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_bbox.ytop = end_y;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_width = width_offset;
			draw_coords.subblk_info->at(i).hierarchy->at(hierarchy).blk_height = height_offset;
		}
	}

	*blk_width = width_offset;
	*blk_height = height_offset/pb_type.num_pb;

	return;
}


