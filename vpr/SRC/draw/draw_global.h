/* This file contains declarations of variables shared by all drawing routines.
 *
 * Key structures: 
 *  t_draw_coords - holds coordinates and dimensions for each grid tile and each 
 *					logic block
 */

#ifndef DRAW_GLOBAL_H
#define DRAW_GLOBAL_H

#include <vector>
#include <string>
using namespace std;

/* Stores the minimum and maximum coordinates for a channel wire. The function
 * draw_get_rr_chan_bbox() to compute the coordinates for a particular node.
 */
typedef struct {
	float xleft;
	float xright;
	float ybottom;
	float ytop;
} t_draw_bbox;

typedef struct {
	float blk_width;
	float blk_height;
	t_draw_bbox blk_bbox;
} t_draw_pb_hierarchy;

typedef struct {
	string blk_type;
	vector<t_draw_pb_hierarchy> *hierarchy;
} t_draw_pb_info;

/* Structure used to store coordinates and dimensions for 
 * grid tiles and logic blocks in the FPGA. 
 * tile_x: x-coordinate of the left and bottom corner
 *         of each grid_tile. (tile_x[0..nx+1])
 * tile_y: y-coordinate of the left and bottom corner
 *		   of each grid_tile. (tile_y[0..nx+1])
 * tile_width: Width (and height) of a grid_tile.
 *			   Set when init_draw_coords is called.
 * pin_size: The half-width or half-height of a pin.
 *			 Set when init_draw_coords is called.
 */
typedef struct {
	float *tile_x, *tile_y;
	float tile_width, pin_size;
	vector<t_draw_pb_info> *subblk_info;
} t_draw_coords;
extern t_draw_coords draw_coords;

#endif 
