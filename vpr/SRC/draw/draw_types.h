/* This file contains declarations of structures and types shared by all drawing 
 * routines.
 *
 * Key structures: 
 *  t_draw_coords - holds coordinates and dimensions for each grid tile and each 
 *					logic block
 *  t_draw_state - holds variables that control drawing modes based on user input
 *				   (eg. clicking on the menu buttons)
 *               - holds state variables that control drawing and highlighting of
 *				   architectural elements on the FPGA chip
 *                 
 * Author: Long Yu (Mike) Wang
 * Date: August 20, 2013
 */

#ifndef DRAW_TYPES_H
#define DRAW_TYPES_H

#include <vector>
#include "util.h"
#include "graphics.h"
#include "vpr_types.h"
using namespace std;

/* Draw rr_graph from less detailed to more detailed
 * in order to speed up drawing when toggle_rr is clicked
 * on for the first time.
 */
enum e_draw_rr_toggle {
	DRAW_NO_RR = 0,
	DRAW_NODES_RR,
	DRAW_NODES_AND_SBOX_RR,
	DRAW_ALL_BUT_BUFFERS_RR,
	DRAW_ALL_RR,
	DRAW_RR_TOGGLE_MAX
};

enum e_draw_congestion {
	DRAW_NO_CONGEST = 0,
	DRAW_CONGESTED,
	DRAW_CONGESTED_AND_USED,
	DRAW_CONGEST_MAX
};

enum e_draw_net_type {
	ALL_NETS, HIGHLIGHTED
};

/* Chanx to chany or vice versa? */
enum e_edge_dir {
	FROM_X_TO_Y, FROM_Y_TO_X
};

/* Structure which stores state information of a rr_node. Used
 * for controling the drawing each rr_node when ROUTING is on screen.
 * color: Color of the rr_node
 * node_highlighted: Whether the node is highlighted. Useful for 
 *					 highlighting routing resources on rr_graph
 */
typedef struct {
	color_types color;
	bool node_highlighted;
} t_draw_rr_node;

/* Structure used to store state variables that control drawing and 
 * highlighting.
 * pic_on_screen: What to draw on the screen (PLACEMENT, ROUTING, or 
 *				  NO_PICTURE).
 * show_nets: Whether to show nets at placement and routing.
 * show_congestion: Controls if congestion is shown, when ROUTING is
 *					on screen.
 * draw_rr_toggle: Controls drawing of routing resources on screen,
 *				   if pic_on_screen is ROUTING.
 * show_blk_internal: If 0, no internal drawing is shown. Otherwise,
 *					  indicates how many levels of sub-pbs to be drawn
 * show_graphics: Whether graphics is enabled.
 * gr_automode: How often is user input required. (0: each t, 
 *				1: each place, 2: never)
 * draw_route_type: GLOBAL or DETAILED
 * default_message: default screen message on screen
 * net_color: color in which each net should be drawn. 
 *			  [0..g_clbs_nlist.net.size()-1]
 * block_color: color in which each block should be drawn.
 *			    [0..num_blocks-1]
 * draw_rr_node: stores the state information of each routing resource.  
 *				 Used to control drawing each routing resource when 
 *				 ROUTING is on screen.
 *				 [0..num_rr_nodes-1]
 */
struct t_draw_state {
	pic_type pic_on_screen;
	boolean show_nets;
	e_draw_congestion show_congestion;
	e_draw_rr_toggle draw_rr_toggle;
	int show_blk_internal;
	boolean show_graphics;
	int gr_automode;
	e_route_type draw_route_type;
	char default_message[BUFSIZE];
	color_types *net_color, *block_color;
	t_draw_rr_node *draw_rr_node;
};

/* Stores the minimum and maximum coordinates for a drawing 
 * object with a bouding box.
 */
struct t_draw_bbox {
	float xleft;
	float xright;
	float ybottom;
	float ytop;
};

/* For each cluster type, this structure stores drawing 
 * information for all sub-blocks inside. This includes
 * the bounding box for drawing each sub-block. */
struct t_draw_pb_type_info {
	vector<t_draw_bbox> subblk_array;
};

/* Structure used to store coordinates and dimensions for 
 * grid tiles and logic blocks in the FPGA. 
 * tile_x and tile_y: together form two axes that make a
 * COORDINATE SYSTEM for grid_tiles, which goes from 
 * (tile_x[0],tile_y[0]) at the lower left corner of the FPGA 
 * to (tile_x[nx+1]+tile_width, tile_y[ny+1]+tile_width) in 
 * the upper right corner.       
 * tile_width: Width (and height) of a grid_tile.
 *			   Set when init_draw_coords is called.
 * pin_size: The half-width or half-height of a pin.
 *			 Set when init_draw_coords is called.
 */
struct t_draw_coords {
	float *tile_x, *tile_y;
	float tile_width, pin_size;
	vector<t_draw_pb_type_info> blk_info;
};

#endif