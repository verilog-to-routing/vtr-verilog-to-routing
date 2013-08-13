#include <cstdio>
#include <algorithm>

#include <assert.h>

#include "intra_logic_block.h"
#include "globals.h"
#include "draw_global.h"

/* Subroutines local to this file. */
static int draw_internal_find_max_lvl(t_pb_type pb_type);
static void draw_internal_load_coords(t_pb_type pb_type, float width);
static float draw_internal_calc_coords(t_pb_type pb_type, float width);
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


//void draw_internal_init_blk() {
//	int i, j, k;
//	t_pb_type pb_type;
//	t_mode mode;
//	float blk_width, blk_height;
//
//	for (i = 0; i < num_types; ++i) {
//		pb_type = *type_descriptors[i].pb_type;
//		
//		blk_width = type_descriptors[i].width * draw_coords.tile_width;
//		blk_height = type_descriptors[i].height * draw_coords.tile_width;
//
//		for (j = 0; j < pb_type.num_modes; ++j) {
//			mode = pb_type.modes[j];
//
//			if (mode.num_pb_type_children == 0)
//				continue;
//
//			for (k = 0; k < mode.num_pb_type_children; ++k) {
//				draw_internal_load_coords(mode.pb_type_children[k], draw_coords.tile_width);
//			}
//		}
//	}
//}
//
//
//void draw_internal_draw_subblk() {
//	int i, j, k;
//	int num_sub_tiles;
//	float sub_tile_step;
//	int height;
//	float x1, y1, x2, y2;
//
//	for (i = 0; i <= (nx + 1); i++) {
//		for (j = 0; j <= (ny + 1); j++) {
//			/* Only the first block of a group should control drawing */
//			if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0) 
//				continue;
//
//			num_sub_tiles = grid[i][j].type->capacity;
//			sub_tile_step = draw_coords.tile_width / num_sub_tiles;
//			height = grid[i][j].type->height;
//
//			/* Don't draw if tile capacity is zero. This includes corners. */
//			if (num_sub_tiles == 0)
//				continue;
//
//			for (k = 0; k < num_sub_tiles; ++k) {
//				/* Get coords of current sub_tile */
//				if ((j < 1) || (j > ny)) { /* top and bottom fringes */
//					/* Only for the top and bottom fringes (ie. io blocks place on top 
//					 * and bottom edges of the chip), sub_tiles are placed next to each 
//					 * other horizontally, because it is easier to visualize the 
//					 * connection from each individual io block to other parts of the 
//					 * chip when toggle_nets is selected.
//					 */
//					x1 = draw_coords.tile_x[i] + (k * sub_tile_step);
//					y1 = draw_coords.tile_y[j];
//					x2 = x1 + sub_tile_step;
//					y2 = y1 + draw_coords.tile_width;
//
//				}
//				else {
//					/* Sub_tiles are stacked on top of each other vertically. */
//					x1 = draw_coords.tile_x[i];
//					y1 = draw_coords.tile_y[j] + (k * sub_tile_step);
//					x2 = x1 + draw_coords.tile_width;
//					y2 = y1 + sub_tile_step;
//				}
//			}
//		}
//	}
//}

			
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


static void draw_internal_load_coords(t_pb_type pb_type, float width) {
	int i, j;
	t_mode mode;
	float sub_blk_width;
	
	if (pb_type.depth != 0)
		sub_blk_width = draw_internal_calc_coords(pb_type, width);

	/* If no modes, we have reached the end of pb_graph */
	if (pb_type.num_modes == 0)
		return;

	for (i = 0; i < pb_type.num_modes; ++i) {
		mode = pb_type.modes[i];

		if (mode.num_pb_type_children == 0)
			continue;

		for (j = 0; j < mode.num_pb_type_children; ++j) {
			draw_internal_load_coords(mode.pb_type_children[j], width);
		}
	}
}


static float draw_internal_calc_coords(t_pb_type pb_type, float width) {

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