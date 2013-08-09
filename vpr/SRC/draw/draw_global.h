/* This file contains declarations of variables shared by all drawing routines.
 *
 * Key structures: 
 *  t_draw_coords - holds coordinates and dimensions for each grid tile and each 
 *					logic block
 */

#ifndef DRAW_GLOBAL_H
#define DRAW_GLOBAL_H

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
} t_draw_coords;
extern t_draw_coords draw_coords;

#endif 
