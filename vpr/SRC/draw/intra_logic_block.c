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
 * Date: August 2013, May 2014
 *
 * Author: Matthew J.P. Walker
 * Date: May 2014
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
#include "draw.h"

/************************* Subroutines local to this file. *******************************/

static void draw_internal_load_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
									  float parent_width, float parent_height);
static void draw_internal_calc_coords(int type_descrip_index, t_pb_graph_node *pb_graph_node, 
									  int num_pb_types, int type_index, int num_pb, int pb_index, 
									  float parent_width, float parent_height, 
									  OUTP float *blk_width, OUTP float *blk_height);
static int draw_internal_find_max_lvl(t_pb_type pb_type);
static void draw_internal_pb(const t_block* const clb, t_pb* pb, float start_x, float start_y,
							 float end_x, float end_y);
static bool is_top_lvl_block_highlighted(const t_block& clb);

static float calc_text_xbound(float start_x, float start_y, float end_x, float end_y, char* const text);

static void draw_logical_connections_of(t_pb* pb, t_block* clb);

void draw_one_logical_connection(
	const t_net_pin& src_pin,  const t_logical_block& src_lblk, const t_draw_bbox& src_abs_bbox,
	const t_net_pin& sink_pin, const t_logical_block& sink_lblk, const t_draw_bbox& sink_abs_bbox);

/************************* Subroutine definitions begin *********************************/

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
	/* Call accessor function to retrieve global variables. */
	t_draw_coords* draw_coords = get_draw_coords_vars();
	t_draw_state* draw_state = get_draw_state_vars();

	int i, type_descriptor_index;
	t_pb_graph_node *pb_graph_head_node;
	float blk_width, blk_height;
	int num_sub_tiles;

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
		draw_state->max_sub_blk_lvl = max(draw_internal_find_max_lvl(*type_descriptors[i].pb_type),
							   draw_state->max_sub_blk_lvl);
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
					// draw blocks on the left and right edges sideways (ie. the i/o blocks there)
					start_x = draw_coords->tile_x[i] + (k * sub_tile_offset);
					start_y = draw_coords->tile_y[j];
					end_x = start_x + sub_tile_offset;
					end_y = start_y + draw_coords->tile_width;

					draw_internal_pb(&block[bnum], block[bnum].pb, start_x, start_y, end_x, end_y);
				}
				else {
					start_x = draw_coords->tile_x[i];
					start_y = draw_coords->tile_y[j] + (k * sub_tile_offset);
					end_x = start_x + draw_coords->tile_width;
					end_y = draw_coords->tile_y[j + blk_height - 1] + ((k + 1) * sub_tile_offset);
					
					draw_internal_pb(&block[bnum], block[bnum].pb, start_x, start_y, end_x, end_y);
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
	float parent_drawing_width, parent_drawing_height;
	float sub_tile_x, sub_tile_y;
	float child_width, child_height;
	// get the bbox for this pb type
	t_draw_bbox& pb_bbox = get_draw_coords_vars()->blk_info.at(type_descrip_index).get_pb_bbox(*pb_graph_node);

	const float FRACTION_PARENT_PADDING_X = 0.01;

	const float FRACTION_PARENT_HEIGHT = 0.90;
	const float FRACTION_PARENT_PADDING_BOTTOM = 0.01;

	const float FRACTION_CHILD_MARGIN_X = 0.025;
	const float FRACTION_CHILD_MARGIN_Y = 0.04;

	/* Draw all child-level blocks in just most of the space inside their parent block. */
	parent_drawing_width = parent_width * (1 - FRACTION_PARENT_PADDING_X*2);
	parent_drawing_height = parent_height * FRACTION_PARENT_HEIGHT;

	/* The left and bottom corner (inside the parent block) of the space to draw 
	 * child blocks. 
	 */
	sub_tile_x = parent_width * FRACTION_PARENT_PADDING_X;
	sub_tile_y = parent_height * FRACTION_PARENT_PADDING_BOTTOM;

	/* Divide parent_drawing_width by the number of child types. */
	child_width = parent_drawing_width/num_pb_types;
	/* Divide parent_drawing_height by the number of instances of the pb_type. */
	child_height = parent_drawing_height/num_pb;

	/* The starting point to draw the physical block. */
	pb_bbox.left()   = child_width * type_index + sub_tile_x + FRACTION_CHILD_MARGIN_X * child_width;
	pb_bbox.bottom() = child_height * pb_index  + sub_tile_y + FRACTION_CHILD_MARGIN_Y * child_height;

	/* Leave some space between different pb_types. */
	child_width *= 1 - FRACTION_CHILD_MARGIN_X*2;
	/* Leave some space between different instances of the same type. */
	child_height *= 1 - FRACTION_CHILD_MARGIN_Y*2;

	/* Endpoint for drawing the pb_type */
	pb_bbox.right() = pb_bbox.left()   + child_width;
	pb_bbox.top()   = pb_bbox.bottom() + child_height;

	*blk_width = child_width;
	*blk_height = child_height;

	return;
}

/* Helper subroutine to draw all sub-blocks. This function traverses through the pb_graph 
 * which a netlist block can map to, and draws each sub-block inside its parent block. With
 * each click on the "Blk Internal" button, a new level is shown. 
 */
static void draw_internal_pb(const t_block* const clb, t_pb* pb, float start_x, float start_y, 
							 float end_x, float end_y) 
{
	t_draw_coords *draw_coords = get_draw_coords_vars();
	t_draw_state *draw_state = get_draw_state_vars();

	t_pb_type *pb_type = pb->pb_graph_node->pb_type;

	/* If the sub-block level (depth) is equal or greater than the number 
	 * of sub-block levels to be shown, don't draw.
	 */
	if (pb_type->depth >= draw_state->show_blk_internal)
		return;

	/* Set the color in the block to white, so the child blocks can be drawn
	 * in grey. If the block is a top-level physical block, only set block
	 * color to white if the block has not been highlighted.
	 */
	if (pb_type->depth == 0 && is_top_lvl_block_highlighted(*clb)) 
		;
	else if (!get_selected_sub_block_info().is_selected(pb->pb_graph_node, clb)) {

		setcolor(WHITE);
		fillrect(start_x, start_y, end_x, end_y); 
		setcolor(BLACK);
		setlinestyle(SOLID);
		drawrect(start_x, start_y, end_x, end_y);
	}

	/* Draw text for block pb_type (the text at the top, eg. "clb" ) */
	setfontsize(16); // note: calc_text_xbound(...) assumes this is 16
	drawtext((start_x + end_x) / 2.0, end_y - (end_y - start_y) / 15.0,
			// here, I am tricking the function into using a slightly different weighting because
			// the text is very close to the top of the block.
	         pb_type->name, calc_text_xbound(start_x, start_y / 5.0f, end_x, end_y / 5.0f, pb_type->name));


	int num_child_types = pb->get_num_child_types();
	for (int i = 0; i < num_child_types; ++i) {
			
		int num_pb = pb->get_num_children_of_type(i);
		for (int j = 0; j < num_pb; ++j) {	
			/* Iterate through each child_pb */
			t_pb* child_pb = &pb->child_pbs[i][j];

			/* Point to child block */
			t_pb_type* pb_child_type = child_pb->pb_graph_node->pb_type;
			
			t_draw_bbox& child_bbox = draw_coords->get_pb_bbox(*clb, *child_pb->pb_graph_node);
			
			/* Set coordinates to draw sub_block */
			float x1 = start_x + child_bbox.left();
			float x2 = start_x + child_bbox.right();
			float y1 = start_y + child_bbox.bottom();
			float y2 = start_y + child_bbox.top();
			
			if (child_pb->name != NULL) {
				/* If child block is used, draw it in default background and
				 * solid border.
				 */
				setlinestyle(SOLID);
				
				/* type_index indicates what type of block. */
				const int type_index = clb->type->index;

				t_selected_sub_block_info& sel_sub_info = get_selected_sub_block_info();

				/* Set default background color based on the top-level physical block type */
				if (sel_sub_info.is_selected(child_pb->pb_graph_node, clb)) {
					setcolor(SELECTED_COLOR);
				} else if (sel_sub_info.is_sink_of_selected(child_pb->pb_graph_node, clb)) {
					setcolor(DRIVES_IT_COLOR);
				} else if (sel_sub_info.is_source_of_selected(child_pb->pb_graph_node, clb)) {
					setcolor(DRIVEN_BY_IT_COLOR);
				} else if (type_index < 3) {
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
			setfontsize(16);
			if (child_pb->name != NULL) {
				/* If child block is used, label it by its pb_type, followed
				 * by its name in brackets.
				 */
				int type_len, name_len, tot_len;
				char *blk_tag;

				type_len = strlen(pb_child_type->name);
				name_len = strlen(child_pb->name);
				tot_len = type_len + name_len;
				blk_tag = (char *)my_malloc((tot_len + 8) * sizeof(char));

				sprintf (blk_tag, "%s(%s)", pb_child_type->name, child_pb->name);

				drawtext((x1 + x2) / 2.0, (y1 + y2) / 2.0,
					 blk_tag, calc_text_xbound(x1, y1, x2, y2, blk_tag));
				// min((x2 - x1) * 0.9f , (y2 - y1) * strlen(blk_tag) * 0.45f)

				free(blk_tag);
			}
			else {
				/* If child block is not used, label it only by its pb_type
				 */
				drawtext((x1 + x2) / 2.0, (y1 + y2) / 2.0,
					 pb_child_type->name, calc_text_xbound(x1, y1, x2, y2, pb_child_type->name));
				// min((x2 - x1) * 0.9f, (y2 - y1) * strlen(pb_child_type->name) * 0.45f)
			}

			/* If no more child physical blocks to draw, return */
			if (pb_child_type->num_modes == 0 || pb->child_pbs == NULL 
				|| pb->child_pbs[i] == NULL || child_pb->name == NULL)
				continue;

			/* Traverse to next-level in pb_graph */
			draw_internal_pb(clb, child_pb, x1, y1, x2, y2);
		}
	}
}

void draw_logical_connections() {
	const t_selected_sub_block_info& sel_sub_info = get_selected_sub_block_info();
	t_draw_state* draw_state = get_draw_state_vars();
	// t_draw_coords *draw_coords = get_draw_coords_vars();

	if (draw_state->show_nets == DRAW_LOGICAL_CONNECTIONS) {
		draw_logical_connections_of(NULL,NULL);
	}

	if (sel_sub_info.has_selection()) {

		t_pb* pb = sel_sub_info.get_selected_pb();
		t_block* clb = sel_sub_info.get_containing_block();

		draw_logical_connections_of(pb,clb);
	}
}

static void draw_logical_connections_of(t_pb* pb, t_block* clb) {
	
	if (pb && clb && pb->child_pbs != 0) {
		int num_child_types = pb->get_num_child_types();
		for (int i = 0; i < num_child_types; ++i) {
				
			int num_pb = pb->get_num_children_of_type(i);
			for (int j = 0; j < num_pb; ++j) {	

				draw_logical_connections_of(&pb->child_pbs[i][j], clb);

			}
		}
		return;
	}

	// iterate over all the atom nets
	for (vector<t_vnet>::iterator net = g_atoms_nlist.net.begin(); net != g_atoms_nlist.net.end(); ++net){

		int logical_block_id = net->pins.at(0).block;
		t_logical_block* src_lblk = &logical_block[logical_block_id];
		t_pb_graph_node* src_pb_gnode = src_lblk->pb->pb_graph_node;

		// get the abs bbox of of the driver pb
		const t_draw_bbox& src_bbox = get_draw_coords_vars()->get_absolute_pb_bbox(src_lblk->clb_index, src_pb_gnode);

		// iterate over the sinks
		for (std::vector<t_net_pin>::iterator pin = net->pins.begin() + 1;
			pin != net->pins.end(); ++pin) {

			t_logical_block* sink_lblk = &logical_block[pin->block];
			t_pb_graph_node* sink_pb_gnode = sink_lblk->pb->pb_graph_node;

			if (pb == NULL || clb == NULL) {
				setcolor(BLACK);
			} else {
				if (src_pb_gnode == pb->pb_graph_node && clb == &block[src_lblk->clb_index]) {
					setcolor(DRIVES_IT_COLOR);
				} else if (sink_pb_gnode == pb->pb_graph_node && clb == &block[sink_lblk->clb_index]) {
					setcolor(DRIVEN_BY_IT_COLOR);
				} else {
					continue; // not showing all, and not the sperified block, so skip
				}
			}

			// get the abs. bbox of the sink pb
			const t_draw_bbox& sink_bbox = get_draw_coords_vars()->get_absolute_pb_bbox(sink_lblk->clb_index, sink_pb_gnode);

			draw_one_logical_connection(net->pins.at(0), *src_lblk, src_bbox, *pin, *sink_lblk, sink_bbox);		
		}
	}
}

void find_pin_index_at_model_scope(
	const t_net_pin& the_pin, const t_logical_block& lblk, const bool search_inputs,
	OUTP int* pin_index, OUTP int* total_pins) {

	t_model_ports* port = NULL;
	if (search_inputs) {
		port = lblk.model->inputs;
	} else { // searching outputs
		port = lblk.model->outputs;
	}

	*pin_index = -1;
	int i = 0;
	// Note, that all iterations must go through - no early exits -
	// as we are also trying to count the total number of pins

	// iterate over the ports.
	while(port != NULL) {
		if(search_inputs ? port->is_clock == FALSE : true) {
			int iport = port->index;
			// iterate over the pins on that port
			for (int ipin = 0; ipin < port->size; ipin++) {
				// get the net that connects here.
				int inet = OPEN;
				if (search_inputs) {
					inet = lblk.input_nets[iport][ipin];
				} else {
					inet = lblk.output_nets[iport][ipin];
				}
				if(inet != OPEN) {
					t_vnet& net = g_atoms_nlist.net.at(inet);
					for (auto pin = net.pins.begin(); pin != net.pins.end(); ++pin) {

						if (pin->block == the_pin.block
							&& ipin == the_pin.block_pin
							&& iport == the_pin.block_port) {
							// we've found our pin
							*pin_index = i;
						}
						
						i = i + 1;
					}
				}
				i = i + 1;
			}
		}
		port = port->next;
	}
	*total_pins = i;
	// if (*pin_index < 0) {
	// 	puts("didn't find!!!!");
	// }
	// puts("--- NEXT! ---");
}

void draw_one_logical_connection(
	const t_net_pin& src_pin,  const t_logical_block& src_lblk, const t_draw_bbox& src_abs_bbox,
	const t_net_pin& sink_pin, const t_logical_block& sink_lblk, const t_draw_bbox& sink_abs_bbox) {

	const float FRACTION_USABLE_WIDTH = 0.3;

	float src_width =  src_abs_bbox.get_width();
	float sink_width = sink_abs_bbox.get_width();

	float src_usable_width =  src_width  * FRACTION_USABLE_WIDTH;
	float sink_usable_width = sink_width * FRACTION_USABLE_WIDTH;

	float src_x_offset = src_abs_bbox.left() + src_width * (1 - FRACTION_USABLE_WIDTH)/2;
	float sink_x_offset = sink_abs_bbox.left() + sink_width * (1 - FRACTION_USABLE_WIDTH)/2;

	int src_pin_index, sink_pin_index, src_pin_total, sink_pin_total;

	find_pin_index_at_model_scope(src_pin,  src_lblk, false,  &src_pin_index, &src_pin_total );
	find_pin_index_at_model_scope(sink_pin, sink_lblk, true, &sink_pin_index, &sink_pin_total);

	const t_point src_start =  {
		src_x_offset + src_usable_width * src_pin_index / ((float)src_pin_total),
		src_abs_bbox.get_ycenter()
	};
	const t_point sink_start = {
		sink_x_offset + sink_usable_width * sink_pin_index / ((float)sink_pin_total),
		sink_abs_bbox.get_ycenter()
	};

	// draw a link connecting the center of the pbs.
	drawline(src_start.x, src_start.y,
		sink_start.x, sink_start.y);

	if (src_lblk.clb_index == sink_lblk.clb_index) {
		// if they are in the same clb, put one arrow in the center
		float center_x = (src_start.x + sink_start.x) / 2;
		float center_y = (src_start.y + sink_start.y) / 2;

		draw_triangle_along_line(
			center_x, center_y,
			src_start.x, sink_start.x,
			src_start.y, sink_start.y
		);
	} else {
		// if they are not, put 2 near each end
		draw_triangle_along_line(
			3,
			src_start.x, sink_start.x,
			src_start.y, sink_start.y
		);
		draw_triangle_along_line(
			-3,
			src_start.x, sink_start.x,
			src_start.y, sink_start.y
		);
	}

}

/* This function checks whether a top-level clb has been highlighted. It does 
 * so by checking whether the color in this block is default color.
 */
static bool is_top_lvl_block_highlighted(const t_block& clb) {
	t_draw_state *draw_state;
	ptrdiff_t blk_id = &clb - block;

	/* Call accessor function to retrieve global variables. */
	draw_state = get_draw_state_vars();
	
	if (clb.type->index < 3) {
		if (draw_state->block_color[blk_id] == LIGHTGREY)
			return false;
	} else if (clb.type->index < 3 + MAX_BLOCK_COLOURS) {
		if (draw_state->block_color[blk_id] == (BISQUE + MAX_BLOCK_COLOURS 
												+ clb.type->index - 3))
			return false;
	} else {
		if (draw_state->block_color[blk_id] == (BISQUE + 2 * MAX_BLOCK_COLOURS - 1))
			return false;
	}

	return true;
}

t_pb* highlight_sub_block_helper(
	t_draw_pb_type_info* blk_info, t_pb* pb, float rel_x, float rel_y, int max_depth);

int highlight_sub_block(int blocknum, float rel_x, float rel_y) {
	t_draw_state* draw_state = get_draw_state_vars();
	t_draw_coords *draw_coords = get_draw_coords_vars();

	int max_depth = draw_state->show_blk_internal;

	int type_index = block[blocknum].type->index;
	t_draw_pb_type_info* blk_info = &draw_coords->blk_info.at(type_index);
	
	t_pb* new_selected_sub_block = 
		highlight_sub_block_helper(blk_info, block[blocknum].pb, rel_x, rel_y, max_depth);
	if (new_selected_sub_block == NULL) {
		get_selected_sub_block_info().clear();
		return 1;
	} else {
		get_selected_sub_block_info().set(new_selected_sub_block, &block[blocknum]);
		return 0;
	}
}

t_pb* highlight_sub_block_helper(
	t_draw_pb_type_info* blk_info, t_pb* pb, float rel_x, float rel_y, int max_depth)
{
	
	t_pb* child_pb;
	t_pb_graph_node *pb_graph_node, *pb_child_node;
	t_pb_type *pb_type;
	t_mode mode;
	int i, j, num_children, num_pb;

	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	// check to see if we are past the displayed level,
	// if pb has children,
	// and if pb is dud
	if (pb_type->depth + 1 > max_depth
	 || pb->child_pbs == NULL
	 || pb_type->num_modes == 0) {
		return NULL;
	}

	// Get the mode of operation
	mode = pb_type->modes[pb->mode];
	num_children = mode.num_pb_type_children;

	// Iterate through each type of child pb, and each pb of that type.
	for (i = 0; i < num_children; ++i) {
		num_pb = mode.pb_type_children[i].num_pb;

		for (j = 0; j < num_pb; ++j) {

			if (pb->child_pbs[i] == NULL) {
				continue;
			}

			child_pb = &pb->child_pbs[i][j];
			pb_child_node = child_pb->pb_graph_node;

			// get the bbox for this child
			const t_draw_bbox& bbox = blk_info->get_pb_bbox(*pb_child_node);

			// If child block is being used, check if it intersects
			if (child_pb->name != NULL &&
				bbox.left() < rel_x && rel_x < bbox.right() &&
				bbox.bottom() < rel_y && rel_y < bbox.top()) {

				// check farther down the graph, see if we can find
				// something more specific.
				t_pb* subtree_result =
					highlight_sub_block_helper(
						blk_info, child_pb, rel_x - bbox.left(), rel_y - bbox.bottom(), max_depth);
				if (subtree_result != NULL) {
					// we found something more specific.
					return subtree_result;
				} else {
					// couldn't find something more specific, return parent
					return child_pb;
				}
			}
		}
	}
	return NULL;
}

float calc_text_xbound(float start_x, float start_y, float end_x, float end_y, char* const text) {
	// limit the text display length to just outside of the bounding box,
	// and limit it by the the height of the bounding box, with an adjustment
	// for the length of the text, so effectively by the height of the text.
	// note that this is calibrated for a 16pt font
	return min((end_x - start_x) * 1.1f, (end_y - start_y) * strlen(text) * 0.5f);
}

/*
 * Begin definition of t_selected_sub_block_info functions.
 */

void t_selected_sub_block_info::set(t_pb* new_selected_sub_block, t_block* new_containing_block) {
	selected_pb = new_selected_sub_block;
	selected_pb_gnode = (selected_pb == NULL) ? NULL : selected_pb->pb_graph_node;
	containing_block = new_containing_block;
	sinks.clear();
	sources.clear();


	if (has_selection()) {
		for (int i = 0; i < num_logical_blocks; ++i) {
			const t_logical_block* lblk = &logical_block[i];
			// find the logical block that corrisponds to this pb.
			if (&block[lblk->clb_index] == get_containing_block()
				&& lblk->pb->pb_graph_node == get_selected_pb_gnode()) {

				t_model* model = lblk->model;

				t_model_ports* model_ports = model->inputs;
				// iterate over the input ports
				while(model_ports != NULL) {
					if(model_ports->is_clock == FALSE) {
						int iport = model_ports->index;
						// iterate over the pins on that port
						for (int ipin = 0; ipin < model_ports->size; ipin++) {
							// get the net that connects here.
							int inet = lblk->input_nets[iport][ipin];
							if(inet != OPEN) {
								t_vnet& net = g_atoms_nlist.net.at(inet);

								// put the driver in the sources list
								t_net_pin& pin = net.pins.at(0);
								t_logical_block* src_lblk = &logical_block[pin.block];

								sources.insert(std::make_pair(
									src_lblk->pb->pb_graph_node, &block[src_lblk->clb_index])
								);
							}
						}
					}
					model_ports = model_ports->next;
				}

				model_ports = model->outputs;
				while(model_ports != NULL) {
					int iport = model_ports->index;
					for (int ipin = 0; ipin < model_ports->size; ipin++) {
						int inet = lblk->output_nets[iport][ipin];
						if(inet != OPEN) {
							t_vnet& net = g_atoms_nlist.net.at(inet);

							// put the all sinks in the sink list
							for (auto pin = net.pins.begin() + 1; pin != net.pins.end(); ++pin) {
								t_logical_block* sink_lblk = &logical_block[pin->block];

								sinks.insert(std::make_pair(
									sink_lblk->pb->pb_graph_node, &block[sink_lblk->clb_index])
								);
							}
						}
					}
					model_ports = model_ports->next;
				}

				break;
			}
		}
	}
}

t_selected_sub_block_info::t_selected_sub_block_info() {
	clear();
}

void t_selected_sub_block_info::clear() {
	set(NULL, NULL);
}

t_pb* t_selected_sub_block_info::get_selected_pb() const { return selected_pb; }

t_pb_graph_node* t_selected_sub_block_info::get_selected_pb_gnode() const { return selected_pb_gnode; }

t_block* t_selected_sub_block_info::get_containing_block() const { return containing_block; }

bool t_selected_sub_block_info::has_selection() const {
	return get_selected_pb_gnode() != NULL && get_containing_block() != NULL; 
}

bool t_selected_sub_block_info::is_selected(const t_pb_graph_node* test, const t_block* test_block) const {
	return get_selected_pb_gnode() == test && get_containing_block() == test_block;
}

bool t_selected_sub_block_info::is_sink_of_selected(const t_pb_graph_node* test, const t_block* test_block) const {
	return sinks.find(std::make_pair(test,test_block)) != sinks.end();
}

bool t_selected_sub_block_info::is_source_of_selected(const t_pb_graph_node* test, const t_block* test_block) const {
	return sources.find(std::make_pair(test,test_block)) != sources.end();
}

t_selected_sub_block_info& get_selected_sub_block_info() {
	// used to keep track of the selected sub-block.
	static t_selected_sub_block_info selected_sub_block_info;

	return selected_sub_block_info;
}
