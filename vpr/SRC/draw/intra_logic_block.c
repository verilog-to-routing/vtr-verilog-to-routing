/* This file holds subroutines responsible for drawing inside clustered logic blocks.
 * The four main subroutines defined here are draw_internal_alloc_blk(), 
 * draw_internal_init_blk(), draw_internal_draw_subblk(), and toggle_blk_internal().
 * When VPR graphics initially sets up, draw_internal_alloc_blk() will be called from 
 * draw.c to allocate space for the structures needed for internal blks drawing.
 * Before any drawing, draw_internal_init_blk() will pre-compute a bounding box
 * for each sub-block in the pb_graph of every physical block type. When the menu button 
 * "Blk Internal" is pressed, toggle_blk_internal() will enable internal blocks drawing.
 * Then, with each subsequent click on the button, toggle_blk_internal() will propel one 
 * more level of pbs to be drawn. Draw_internal_draw_subblk() will be called whenever 
 * new blocks need to be drawn, and this function is responsible for drawing sub-blocks 
 * from the pre-computed bounding box values.
 * 
 * Author: Long Yu (Mike) Wang 
 * Date: August 2013
 */

#include <cstdio>
#include <algorithm>
#include <assert.h>
#include <string.h>
using namespace std;

#include "intra_logic_block.h"
#include "globals.h"
#include "vpr_utils.h"
#include "draw_global.h"
#include "graphics.h"


/****************************** File scope variables *************************************/

/* The maximum number of sub-block levels among all physical block types in the FPGA. 
 * Used as a flag to toggle internal drawing off. */
int max_sub_blk_lvl = 0;

/************************* Subroutines local to this file. *******************************/

static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
									  float parent_width, float parent_height);
static void draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
									  int num_pb_types, int type_index, int num_pb, int pb_index, 
									  float parent_width, float parent_height, 
									  OUTP float *blk_width, OUTP float *blk_height);
static int draw_internal_find_max_lvl(t_pb_type pb_type);
static void draw_internal_pb(int blk_id, t_pb *pb, float start_x, float start_y,
							 float end_x, float end_y);
static bool is_top_lvl_block_highlighted(int blk_id);


/************************* Subroutine definitions begin *********************************/

void toggle_blk_internal(void (*drawscreen_ptr)(void)) {
	t_draw_state *draw_state;

	/* Call accessor function to retrieve global variables. */
	draw_state = get_draw_state_vars();

	/* Increment the depth of sub-blocks to be shown */
	draw_state->show_blk_internal++;
	/* If depth exceeds maximum sub-block level in pb_graph, then
	 * disable internals drawing
	 */
	if (draw_state->show_blk_internal > max_sub_blk_lvl)
		draw_state->show_blk_internal = 0;
	
	drawscreen_ptr();
}


void draw_internal_alloc_blk() {
	t_draw_coords *draw_coords;
	int i;
	t_pb_graph_node *pb_graph_head;
	
	/* Call accessor function to retrieve global variables. */
	draw_coords = get_draw_coords_vars();
	
	/* Create a vector holding coordinate information for each type of physical logic
	 * block.
	 */
	draw_coords->blk_info.resize(num_types);

	for (i = 0; i < num_types; ++i) {
		/* Empty block has no sub_blocks */
		if (&type_descriptors[i] == EMPTY_TYPE)
			continue;
		
		pb_graph_head = type_descriptors[i].pb_graph_head;
		
		/* Create an vector with size equal to the total number of pins for each type 
		 * of physical logic block, in order to uniquely identify each sub-block in 
		 * the pb_graph of that type.
		 */
		draw_coords->blk_info.at(i).subblk_array.resize(pb_graph_head->total_pb_pins);
	}
}


void draw_internal_init_blk() {
	t_draw_coords *draw_coords;
	int i, type_descriptor_index;
	t_pb_graph_node *pb_graph_head_node;
	float blk_width, blk_height;
	int num_sub_tiles;

	/* Call accessor function to retrieve global variables. */
	draw_coords = get_draw_coords_vars();

	for (i = 0; i < num_types; ++i) {
		/* Empty block has no sub_blocks */
		if (&type_descriptors[i] == EMPTY_TYPE)
			continue;

		pb_graph_head_node = type_descriptors[i].pb_graph_head;
		type_descriptor_index = type_descriptors[i].index;

		num_sub_tiles = type_descriptors[i].capacity;
		/* Calculate width and height of each top-level physical logic 
		 * block type. 
		 */
		blk_width = type_descriptors[i].width * draw_coords->tile_width
					/num_sub_tiles;
		blk_height = type_descriptors[i].height * draw_coords->tile_width
					/num_sub_tiles;

		draw_internal_load_coords(type_descriptor_index, pb_graph_head_node, 
								  blk_width, blk_height);

		/* Determine the max number of sub_block levels in the FPGA */
		max_sub_blk_lvl = max(draw_internal_find_max_lvl(*type_descriptors[i].pb_type),
							   max_sub_blk_lvl);
	}
}


void draw_internal_draw_subblk() {
	t_draw_coords *draw_coords;
	int i, j, k;
	int num_sub_tiles, blk_height, bnum;
	float sub_tile_offset;
	float start_x, start_y, end_x, end_y;

	/* Call accessor function to retrieve global variables. */
	draw_coords = get_draw_coords_vars();

	for (i = 0; i <= (nx + 1); i++) {
		for (j = 0; j <= (ny + 1); j++) {
			/* Only the first block of a group should control drawing */
			if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0) 
				continue;

			/* Don't draw if tile is empty. This includes corners. */
			if (grid[i][j].type == EMPTY_TYPE)
				continue;
			
			num_sub_tiles = grid[i][j].type->capacity;
			sub_tile_offset = draw_coords->tile_width / num_sub_tiles;
			blk_height = grid[i][j].type->height;

			for (k = 0; k < num_sub_tiles; ++k) {
				/* Don't draw if block is empty. */
				if (grid[i][j].blocks[k] == EMPTY || grid[i][j].blocks[k] == INVALID)
					continue;

				/* Get block ID */
				bnum = grid[i][j].blocks[k];
				/* Safety check, that physical blocks exists in the CLB */
				if (block[bnum].pb == NULL)
					continue;

				if ((j < 1) || (j > ny)) {	
					start_x = draw_coords->tile_x[i] + (k * sub_tile_offset);
					start_y = draw_coords->tile_y[j];
					end_x = start_x + sub_tile_offset;
					end_y = start_y + draw_coords->tile_width;

					draw_internal_pb(bnum, block[bnum].pb, start_x, start_y, end_x, end_y);
				}
				else {
					start_x = draw_coords->tile_x[i];
					start_y = draw_coords->tile_y[j] + (k * sub_tile_offset);
					end_x = start_x + draw_coords->tile_width;
					end_y = draw_coords->tile_y[j + blk_height - 1] + ((k + 1) * sub_tile_offset);
					
					draw_internal_pb(bnum, block[bnum].pb, start_x, start_y, end_x, end_y);
				}
			}
		}
	}
}


/* This function traverses through the pb_graph of a certain physical block type and 
 * finds the maximum sub-block levels for that type.
 */
static int draw_internal_find_max_lvl(t_pb_type pb_type) {
	int i, j;
	t_mode mode;
	int max_levels = 0;

	/* If no modes, we have reached the end of pb_graph */
	if (pb_type.num_modes == 0)
		return (pb_type.depth);

	for (i = 0; i < pb_type.num_modes; ++i) {
		mode = pb_type.modes[i];

		for (j = 0; j < mode.num_pb_type_children; ++j) {
			max_levels = max(draw_internal_find_max_lvl(mode.pb_type_children[j]), max_levels);
		}
	}

	return max_levels;
}


/* Helper function for initializing bounding box values for each sub-block. This function
 * traverses through the pb_graph for a descriptor_type (given by type_descrip_index), and
 * calls helper function to compute bounding box values.
 */
static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
									  float parent_width, float parent_height) {
	int i, j, k;
	t_pb_type *pb_type;
	int num_modes, num_children, num_pb;
	t_mode mode;
	float blk_width = 0.;
	float blk_height = 0.;

	/* Get information about the pb_type */
	pb_type = pb_graph_node->pb_type;
	num_modes = pb_type->num_modes;

	/* If no modes, we have reached the end of pb_graph */
	if (num_modes == 0)
		return;

	for (i = 0; i < num_modes; ++i) {
		mode = pb_type->modes[i];
		num_children = mode.num_pb_type_children;

		for (j = 0; j < num_children; ++j) {
			/* Find the number of instances for each child pb_type. */
			num_pb = mode.pb_type_children[j].num_pb;
			
			for (k = 0; k < num_pb; ++k) { 
				/* Compute bound box for block. Don't call if pb_type is root-level pb. */
				draw_internal_calc_coords(type_descrip_index, 
										  &pb_graph_node->child_pb_graph_nodes[i][j][k], 
										  num_children, j, num_pb, k,
										  parent_width, parent_height, 
										  &blk_width, &blk_height);		

				/* Traverse to next level in the pb_graph */
				draw_internal_load_coords(type_descrip_index, 
										  &pb_graph_node->child_pb_graph_nodes[i][j][k], 
										  blk_width, blk_height);
			}
		}
	}
	return;
}


/* Helper function which computes bounding box values for a sub-block. The coordinates 
 * are relative to the left and bottom corner of the parent block.
 */
static void 
draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
						  int num_pb_types, int type_index, int num_pb, int pb_index, 
						  float parent_width, float parent_height, 
						  OUTP float *blk_width, OUTP float *blk_height) 
{
	t_draw_coords *draw_coords;
	int node_id;
	float drawing_width, drawing_height;
	float sub_tile_x, sub_tile_y;
	float width, height;
	float start_x, start_y, end_x, end_y;

	/* Call accessor function to retrieve global variables. */
	draw_coords = get_draw_coords_vars();

	/* Draw all child-level blocks in 90% of the space inside their parent block. */
	drawing_width = parent_width * 0.9;
	drawing_height = parent_height * 0.9;

	/* The left and bottom corner (inside the parent block) of the space to draw 
	 * child blocks. 
	 */
	sub_tile_x = parent_width * 0.05;
	sub_tile_y = parent_height * 0.05;

	/* Divide drawing_width by the number of child types. */
	width = drawing_width/num_pb_types;
	/* Divide drawing_height by the number of instances of the pb_type. */
	height = drawing_height/num_pb;

	/* The starting point to draw the physical block. */
	start_x = width * type_index + sub_tile_x;
	start_y = height * pb_index + sub_tile_y;

	/* Leave some space between different pb_types. */
	width *= 0.9;
	/* Leave some space between different instances of the same type. */
	height *= 0.95;

	/* Endpoint for drawing the pb_type */
	end_x = start_x + width;
	end_y = start_y + height;

	/* Get unique id for the pb_node */
	node_id = get_unique_pb_graph_node_id(pb_graph_node);

	/* Load coordinates into globally declared structure */
	draw_coords->blk_info.at(type_descrip_index).subblk_array.at(node_id).xleft = start_x;
	draw_coords->blk_info.at(type_descrip_index).subblk_array.at(node_id).xright = end_x;
	draw_coords->blk_info.at(type_descrip_index).subblk_array.at(node_id).ybottom = start_y;
	draw_coords->blk_info.at(type_descrip_index).subblk_array.at(node_id).ytop = end_y;
	
	*blk_width = width;
	*blk_height = height;

	return;
}

/* Helper subroutine to draw all sub-blocks. This function traverses through the pb_graph 
 * which a netlist block can map to, and draws each sub-block inside its parent block. With
 * each click on the "Blk Internal" button, a new level is shown. 
 */
static void draw_internal_pb(int blk_id, t_pb *pb, float start_x, float start_y, 
							 float end_x, float end_y) 
{
	t_draw_coords *draw_coords;
	t_draw_state *draw_state;
	t_pb_graph_node *pb_graph_node, *pb_child_node;
	t_pb_type *pb_type, *pb_child_type;
	t_mode mode;
	int i, j, num_children, num_pb;
	int type_index, node_id;
	float x1, x2, y1, y2;

	/* Call accessor function to retrieve global variables. */
	draw_coords = get_draw_coords_vars();
	draw_state = get_draw_state_vars();

	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* If the sub-block level (depth) is equal or greater than the number 
	 * of sub-block levels to be shown, don't draw.
	 */
	if (pb_type->depth >= draw_state->show_blk_internal)
		return;

	/* Set the color in the block to white, so the child blocks can be drawn
	 * in grey. If the block is a top-level physical block, only set block
	 * color to white if the block has not been highlighted.
	 */
	if (pb_type->depth == 0 && is_top_lvl_block_highlighted(blk_id)) 
		;
	else {
		setcolor(WHITE);
		fillrect(start_x, start_y, end_x, end_y); 
		setcolor(BLACK);
		setlinestyle(SOLID);
		drawrect(start_x, start_y, end_x, end_y);
	}

	/* Draw text for block pb_type */
	drawtext((start_x + end_x) / 2.0, end_y - (end_y - start_y) / 20.0,
			 pb_type->name, min((end_x - start_x), (end_y - start_y)));

	/* Get the mode of operation */
	mode = pb_type->modes[pb->mode];
	num_children = mode.num_pb_type_children;

	for (i = 0; i < num_children; ++i) {
		num_pb = mode.pb_type_children[i].num_pb;
			
		for (j = 0; j < num_pb; ++j) {	
			/* Iterate through each child_pb. */

			/* Point to child block */
			pb_child_node = pb->child_pbs[i][j].pb_graph_node;
			pb_child_type = pb_child_node->pb_type;

			/* type_index indicates what type of block. */
			type_index = block[blk_id].type->index;
			/* get unique ID for child block. */
			node_id = get_unique_pb_graph_node_id(pb_child_node);
			
			/* Set coordinates to draw sub_block */
			x1 = start_x + draw_coords->blk_info.at(type_index).subblk_array.at(node_id).xleft;
			x2 = start_x + draw_coords->blk_info.at(type_index).subblk_array.at(node_id).xright;
			y1 = start_y + draw_coords->blk_info.at(type_index).subblk_array.at(node_id).ybottom;
			y2 = start_y + draw_coords->blk_info.at(type_index).subblk_array.at(node_id).ytop;
			
			if (pb->child_pbs[i][j].name != NULL) {
				/* If child block is used, draw it in default background and
				 * solid border.
				 */
				setlinestyle(SOLID);
				
				/* Set default background color based on the top-level physical block type */
				if (type_index < 3) {
					setcolor(LIGHTGREY);
				} else if (type_index < 3 + MAX_BLOCK_COLOURS) {
					setcolor(BISQUE + MAX_BLOCK_COLOURS + type_index - 3);
				} else {
					setcolor(BISQUE + 2 * MAX_BLOCK_COLOURS - 1);
				}
			}
			else {
				/* If child block is not used, draw empty block (ie. white 
				 * background with dashed border). 
				 */
				setlinestyle(DASHED);
				setcolor(WHITE);
			}
			
			fillrect(x1, y1, x2, y2); 
			setcolor(BLACK);
			drawrect(x1, y1, x2, y2);

			/* Display names for sub_blocks */
			if (pb->child_pbs[i][j].name != NULL) {
				/* If child block is used, label it by its pb_type, followed
				 * by its name in brackets.
				 */
				int type_len, name_len, tot_len;
				char *blk_tag;

				type_len = strlen(pb_child_type->name);
				name_len = strlen(pb->child_pbs[i][j].name);
				tot_len = type_len + name_len;
				blk_tag = (char *)my_malloc((tot_len + 5) * sizeof(char));

				sprintf (blk_tag, "%s(%s)", pb_child_type->name, pb->child_pbs[i][j].name);

				drawtext((x1 + x2) / 2.0, (y1 + y2) / 2.0,
					 blk_tag, min((x2 - x1)/2, (y2 - y1)));

				free(blk_tag);
			}
			else {
				/* If child block is not used, label it only by its pb_type
				 */
				drawtext((x1 + x2) / 2.0, (y1 + y2) / 2.0,
					 pb_child_type->name, min((x2 - x1)/2, (y2 - y1)));
			}

			/* If no more child physical blocks to draw, return */
			if (pb_child_type->num_modes == 0 || pb->child_pbs == NULL 
				|| pb->child_pbs[i] == NULL || pb->child_pbs[i][j].name == NULL)
				continue;

			/* Traverse to next-level in pb_graph */
			draw_internal_pb(blk_id, &pb->child_pbs[i][j], x1, y1, x2, y2);
		}
	}
	
}


/* This function checks whether a top-level clb has been highlighted. It does 
 * so by checking whether the color in this block is default color.
 */
static bool is_top_lvl_block_highlighted(int blk_id) {
	t_draw_state *draw_state;

	/* Call accessor function to retrieve global variables. */
	draw_state = get_draw_state_vars();
	
	if (block[blk_id].type->index < 3) {
		if (draw_state->block_color[blk_id] == LIGHTGREY)
			return false;
	} else if (block[blk_id].type->index < 3 + MAX_BLOCK_COLOURS) {
		if (draw_state->block_color[blk_id] == (BISQUE + MAX_BLOCK_COLOURS 
												+ block[blk_id].type->index - 3))
			return false;
	} else {
		if (draw_state->block_color[blk_id] == (BISQUE + 2 * MAX_BLOCK_COLOURS - 1))
			return false;
	}

	return true;
}