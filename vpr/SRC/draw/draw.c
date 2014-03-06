/*********************************** Top-level Summary *************************************
 This is VPR's main graphics application program. The program interacts with graphics.c, 
 which provides an API for displaying graphics on both X11 and Win32. The most important
 subroutine in this file is drawscreen(), which is a callback function that X11 or Win32
 will call whenever the screen needs to be updated. Then, drawscreen() will decide what 
 drawing subroutines to call depending on whether PLACEMENT or ROUTING is shown on screen 
 and whether any of the menu buttons has been triggered. As a note, looks into draw_global.c
 for understanding the data structures associated with drawing.
 
 Authors: Vaughn Betz, Long Yu (Mike) Wang
 Last updated: August 2013
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
using namespace std;

#include <assert.h>

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "graphics.h"
#include "path_delay.h"
#include "draw.h"
#include "read_xml_arch_file.h"
#include "util.h"
#include "draw_types.h"
#include "draw_global.h"
#include "intra_logic_block.h"

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
			  * track CPU runtime.														   */
#include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
	   * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
	   * which means tracking CPU time will not be the same as the actual wall clock time. *
	   * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#include <sys/time.h>
#endif

#ifdef DEBUG
#include "rr_graph.h"
#endif

/****************************** Define Macros *******************************/

#define DEFAULT_RR_NODE_COLOR BLACK
//#define TIME_DRAWSCREEN /* Enable if want to track runtime for drawscreen() */

/************************** File Scope Variables ****************************/

/* Draw_state is globally accessible through global accesssor function 
 * get_draw_state_vars().
 */
t_draw_state *draw_state;

/* Call global accessor function get_draw_coords_vars() to retrieve 
 * global variables for coordinates information. 
 */
t_draw_coords *draw_coords;

/********************** Subroutines local to this module ********************/

static void toggle_nets(void (*drawscreen)(void));
static void toggle_rr(void (*drawscreen)(void));
static void toggle_congestion(void (*drawscreen)(void));
static void highlight_crit_path(void (*drawscreen_ptr)(void));

static void drawscreen(void);
static void redraw_screen(void);
static void drawplace(void);
static void drawnets(void);
static void drawroute(enum e_draw_net_type draw_net_type);
static void draw_congestion(void);

static void highlight_blocks(float x, float y, t_event_buttonPressed button_info);
static void get_block_center(int bnum, float *x, float *y);
static void deselect_all(void);

static void draw_rr(void);
static void draw_rr_edges(int from_node);
static void draw_rr_pin(int inode, enum color_types color);
static void draw_rr_chanx(int inode, int itrack, enum color_types color);
static void draw_rr_chany(int inode, int itrack, enum color_types color);
static t_draw_bbox draw_get_rr_chan_bbox(int inode);
static void draw_get_rr_pin_coords(int inode, int iside, int width_offset, 
								   int height_offset, float *xcen, float *ycen);
static void draw_pin_to_chan_edge(int pin_node, int chan_node);
static void draw_x(float x, float y, float size);
static void draw_pin_to_pin(int opin, int ipin);
static void draw_rr_switch(float from_x, float from_y, float to_x, float to_y,
						   boolean buffered);
static void draw_chany_to_chany_edge(int from_node, int from_track, int to_node,
									 int to_track, short switch_type);
static void draw_chanx_to_chanx_edge(int from_node, int from_track, int to_node,
									 int to_track, short switch_type);
static void draw_chanx_to_chany_edge(int chanx_node, int chanx_track, int chany_node, 
									 int chany_track, enum e_edge_dir edge_dir,
									 short switch_type);
static void draw_triangle_along_line(float xend, float yend, float x1, float x2,
									 float y1, float y2);
static int get_track_num(int inode, int **chanx_track, int **chany_track);
static bool draw_if_net_highlighted (int inet);
static void draw_highlight_fan_in_fan_out(int hit_node);
static void highlight_nets(char *message, int hit_node);
static int draw_check_rr_node_hit (float click_x, float click_y);
static void highlight_rr_nodes(float x, float y);
static void draw_highlight_blocks_color(t_type_ptr type, int bnum);
static void draw_reset_blk_color(int i);

/********************** Subroutine definitions ******************************/

bool is_inode_an_interposer_wire(int inode)
{
	int ix = rr_node[inode].xlow;
	int itrack = rr_node[inode].ptc_num;
	if( rr_node[inode].type==CHANY && itrack >= chan_width.y_list[ix] )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* This Funtion draws an interposer wire
 * The interposer wires are shifted up in the channel to show the effect that they are not
 * part of the channel tracks.
 * Also, the interposer wires are drawn in RED so that it's clear where cuts are happening
*/
void draw_shifted_line(int inode)
{
	t_draw_bbox bound_box = draw_get_rr_chan_bbox(inode);
	int ix = rr_node[inode].xlow;
	int savecolor = getcolor();
	int bottom_shift_y = 0; 
	int top_shift_y = 0;
	if(is_inode_an_interposer_wire(inode))
	{
		setcolor(RED);
		bottom_shift_y = draw_coords->tile_width + chan_width.y_list[ix] + 0.25*chan_width.y_list[ix];
		top_shift_y = chan_width.y_list[ix]   + chan_width.y_list[ix] - 0.25*chan_width.y_list[ix];
	}
	drawline(bound_box.xleft, bound_box.ybottom+bottom_shift_y, bound_box.xright, bound_box.ytop+top_shift_y);
	setcolor(savecolor);
}

void init_graphics_state(boolean show_graphics_val, int gr_automode_val,
		enum e_route_type route_type) 
{	
	/* Call accessor functions to retrieve global variables. */
	draw_state = get_draw_state_vars();

	/* Sets the static show_graphics and gr_automode variables to the    *
	 * desired values.  They control if graphics are enabled and, if so, *
	 * how often the user is prompted for input.                         */

	draw_state->show_graphics = show_graphics_val;
	draw_state->gr_automode = gr_automode_val;
	draw_state->draw_route_type = route_type;
}

void update_screen(int priority, char *msg, enum pic_type pic_on_screen_val,
		boolean crit_path_button_enabled) {

	/* Updates the screen if the user has requested graphics.  The priority  *
	 * value controls whether or not the Proceed button must be clicked to   *
	 * continue.  Saves the pic_on_screen_val to allow pan and zoom redraws. */

	if (!draw_state->show_graphics) /* Graphics turned off */
		return;

	/* If it's the type of picture displayed has changed, set up the proper  *
	 * buttons.                                                              */
	if (draw_state->pic_on_screen != pic_on_screen_val) {
		if (pic_on_screen_val == PLACEMENT && draw_state->pic_on_screen == NO_PICTURE) {
			create_button("Window", "Toggle Nets", toggle_nets);
			create_button("Toggle Nets", "Blk Internal", toggle_blk_internal);
		} 
		else if (pic_on_screen_val == ROUTING && draw_state->pic_on_screen == PLACEMENT) {
			create_button("Blk Internal", "Toggle RR", toggle_rr);
			create_button("Toggle RR", "Congestion", toggle_congestion);

			if (crit_path_button_enabled) {
				create_button("Congestion", "Crit. Path", highlight_crit_path);
			}
		} 
		else if (pic_on_screen_val == PLACEMENT && draw_state->pic_on_screen == ROUTING) {
			destroy_button("Toggle RR");
			destroy_button("Congestion");

			if (crit_path_button_enabled) {
				destroy_button("Crit. Path");
			}
		} 
		else if (pic_on_screen_val == ROUTING
				&& draw_state->pic_on_screen == NO_PICTURE) {
			create_button("Window", "Toggle Nets", toggle_nets);
			create_button("Toggle Nets", "Blk Internal", toggle_blk_internal);
			create_button("Blk Internal", "Toggle RR", toggle_rr);
			create_button("Toggle RR", "Congestion", toggle_congestion);

			if (crit_path_button_enabled) {
				create_button("Congestion", "Crit. Path", highlight_crit_path);
			}
		}
	}
	/* Save the main message. */

	my_strncpy(draw_state->default_message, msg, BUFSIZE);

	draw_state->pic_on_screen = pic_on_screen_val;
	update_message(msg);
	drawscreen();
	if (priority >= draw_state->gr_automode) {
		event_loop(highlight_blocks, NULL, NULL, drawscreen);
	} else {
		flushinput();
	}
}

static void drawscreen() {

#ifdef TIME_DRAWSCREEN
	/* This can be used to test how long it takes for the redrawing routing to finish   *
	 * updating the screen for a given input which would cause the screen to be redrawn.*/

#ifdef WIN32
	clock_t drawscreen_begin,drawscreen_end;
	drawscreen_begin = clock();

#else /* For X11. The clock() function in time.h does not output correct time difference *
       * in Linux, so use gettimeofday() in sys/time.h for accurate runtime tracking. */
	struct timeval begin;
	gettimeofday(&begin,NULL);  /* get start time */

	unsigned long begin_time;
	begin_time = begin.tv_sec * 1000000 + begin.tv_usec;
#endif
#endif

	/* This is the screen redrawing routine that event_loop assumes exists.  *
	 * It erases whatever is on screen, then calls redraw_screen to redraw   *
	 * it.                                                                   */

	clearscreen();
	redraw_screen();

#ifdef TIME_DRAWSCREEN

#ifdef WIN32
	drawscreen_end = clock();

	#ifdef CLOCKS_PER_SEC /* This macro has to do with the version of compiler being used */
		printf("Drawscreen took %f seconds.\n", (float)(drawscreen_end - drawscreen_begin) / CLOCKS_PER_SEC);
	#else
		printf("Drawscreen took %f seconds.\n", (float)(drawscreen_end - drawscreen_begin) / CLK_PER_SEC);
	#endif

#else /* X11 */
	struct timeval end;
	gettimeofday(&end,NULL);  /* get end time */

	unsigned long end_time;
	end_time = end.tv_sec * 1000000 + end.tv_usec;

	unsigned long time_diff_microsec;
	time_diff_microsec = end_time - begin_time;

	printf("Drawscreen took %ld microseconds\n", time_diff_microsec);
#endif /* WIN32 */
#endif /* TIME_DRAWSCREEN */
}

static void redraw_screen() {

	/* The screen redrawing routine called by drawscreen and           *
	 * highlight_blocks.  Call this routine instead of drawscreen if   *
	 * you know you don't need to erase the current graphics, and want *
	 * to avoid a screen "flash".                                      */

	setfontsize(14); 
	if (draw_state->pic_on_screen == PLACEMENT) {
		drawplace();
		if (draw_state->show_nets) {
			drawnets();
		}
		
		if (draw_state->show_blk_internal) {
			draw_internal_draw_subblk();
		}
	} else { /* ROUTING on screen */
		drawplace();

		if (draw_state->show_nets) {
			drawroute(ALL_NETS);
		} else {
			draw_rr();
		}

		if (draw_state->show_blk_internal) {
			draw_internal_draw_subblk();
		}

		if (draw_state->show_congestion != DRAW_NO_CONGEST) {
			draw_congestion();
		}
	}
}

static void toggle_nets(void (*drawscreen_ptr)(void)) {

	/* Enables/disables drawing of nets when a the user clicks on a button.    *
	 * Also disables drawing of routing resources.  See graphics.c for details *
	 * of how buttons work.                                                    */

	draw_state->show_nets = (draw_state->show_nets == FALSE) ? TRUE : FALSE;
	draw_state->draw_rr_toggle = DRAW_NO_RR;
	draw_state->show_congestion = DRAW_NO_CONGEST;

	update_message(draw_state->default_message);
	drawscreen_ptr();
}

static void toggle_rr(void (*drawscreen_ptr)(void)) {

	/* Cycles through the options for viewing the routing resources available   *
	 * in an FPGA.  If a routing isn't on screen, the routing graph hasn't been *
	 * built, and this routine doesn't switch the view. Otherwise, this routine *
	 * switches to the routing resource view.  Clicking on the toggle cycles    *
	 * through the options:  DRAW_NO_RR, DRAW_ALL_RR, DRAW_ALL_BUT_BUFFERS_RR,  *
	 * DRAW_NODES_AND_SBOX_RR, and DRAW_NODES_RR.                               */

	draw_state->draw_rr_toggle = (enum e_draw_rr_toggle) (((int)draw_state->draw_rr_toggle + 1) 
														  % ((int)DRAW_RR_TOGGLE_MAX));
	draw_state->show_nets = FALSE;
	draw_state->show_congestion = DRAW_NO_CONGEST;

	update_message(draw_state->default_message);
	drawscreen_ptr();
}

static void toggle_congestion(void (*drawscreen_ptr)(void)) {

	/* Turns the congestion display on and off.   */
	char msg[BUFSIZE];
	int inode, num_congested;

	draw_state->show_nets = FALSE;
	draw_state->draw_rr_toggle = DRAW_NO_RR;
	draw_state->show_congestion = (enum e_draw_congestion) (((int)draw_state->show_congestion + 1) 
														  % ((int)DRAW_CONGEST_MAX));

	if (draw_state->show_congestion == DRAW_NO_CONGEST) {
		update_message(draw_state->default_message);
	} else {
		num_congested = 0;
		for (inode = 0; inode < num_rr_nodes; inode++) {
			if (rr_node[inode].occ > rr_node[inode].capacity) {
				num_congested++;
			}
		}

		sprintf(msg, "%d routing resources are overused.", num_congested);
		update_message(msg);
	}

	drawscreen_ptr();
}

static void highlight_crit_path(void (*drawscreen_ptr)(void)) {

	/* Highlights all the blocks and nets on the critical path. */

	t_linked_int *critical_path_head, *critical_path_node;
	int inode, iblk, inet, num_nets_seen;
	static int nets_to_highlight = 1;
	char msg[BUFSIZE];

	if (nets_to_highlight == 0) { /* Clear the display of all highlighting. */
		nets_to_highlight = 1;
		deselect_all();
		update_message(draw_state->default_message);
		drawscreen_ptr();
		return;
	}

	critical_path_head = allocate_and_load_critical_path();
	critical_path_node = critical_path_head;
	num_nets_seen = 0;

	while (critical_path_node != NULL) {
		inode = critical_path_node->data;
		get_tnode_block_and_output_net(inode, &iblk, &inet);

		if (num_nets_seen == nets_to_highlight) { /* Last block */
			draw_state->block_color[iblk] = MAGENTA;
		} else if (num_nets_seen == nets_to_highlight - 1) { /* 2nd last block */
			draw_state->block_color[iblk] = YELLOW;
		} else if (num_nets_seen < nets_to_highlight) { /* Earlier block */
			draw_state->block_color[iblk] = DARKGREEN;
		}

		if (inet != OPEN) {
			num_nets_seen++;

			if (num_nets_seen < nets_to_highlight) { /* First nets. */
				draw_state->net_color[inet] = DARKGREEN;
			} else if (num_nets_seen == nets_to_highlight) {
				draw_state->net_color[inet] = CYAN; /* Last (new) net. */
			}
		}

		critical_path_node = critical_path_node->next;
	}

	if (nets_to_highlight == num_nets_seen) {
		nets_to_highlight = 0;
		sprintf(msg, "All %d nets on the critical path highlighted.",
				num_nets_seen);
	} else {
		sprintf(msg, "First %d nets on the critical path highlighted.",
				nets_to_highlight);
		nets_to_highlight++;
	}

	free_int_list(&critical_path_head);

	update_message(msg);
	drawscreen_ptr();
}


void alloc_draw_structs(void) {
	/* Call accessor functions to retrieve global variables. */
	draw_coords = get_draw_coords_vars();

	/* Allocate the structures needed to draw the placement and routing.  Set *
	 * up the default colors for blocks and nets.                             */

	draw_coords->tile_x = (float *) my_malloc((nx + 2) * sizeof(float));
	draw_coords->tile_y = (float *) my_malloc((ny + 2) * sizeof(float));

	/* For sub-block drawings inside clbs */
	draw_internal_alloc_blk();

	draw_state->net_color = (enum color_types *) my_malloc(
								g_clbs_nlist.net.size() * sizeof(enum color_types));

	draw_state->block_color = (enum color_types *) my_malloc(
								 num_blocks * sizeof(enum color_types));

	/* Space is allocated for draw_rr_node but not initialized because we do *
	 * not yet know information about the routing resources.				  */
	draw_state->draw_rr_node = (t_draw_rr_node *) my_malloc(
									num_rr_nodes * sizeof(t_draw_rr_node));

	deselect_all(); /* Set initial colors */
}

void free_draw_structs(void) {

	/* Free everything allocated by alloc_draw_structs. Called after close_graphics() *
	 * in vpr_api.c.
	 *
	 * For safety, set all the array pointers to NULL in case any data
	 * structure gets freed twice.													 */

	if(draw_coords != NULL) {
		free(draw_coords->tile_x);  
		draw_coords->tile_x = NULL;
		free(draw_coords->tile_y);  
		draw_coords->tile_y = NULL;		
	}

	if(draw_state != NULL) {
		free(draw_state->net_color);  	
		draw_state->net_color = NULL;
		free(draw_state->block_color);  
		draw_state->block_color = NULL;

		free(draw_state->draw_rr_node);	
		draw_state->draw_rr_node = NULL;
	}
}

void init_draw_coords(float width_val) {

	/* Load the arrays containing the left and bottom coordinates of the clbs   *
	 * forming the FPGA.  tile_width_val sets the width and height of a drawn    *
	 * clb.                                                                     */

	int i;
	int j;

	if (!draw_state->show_graphics)
		return; /* -nodisp was selected. */

	/* Each time routing is on screen, need to reallocate the color of each *
	 * rr_node, as the number of rr_nodes may change.						*/
	if (num_rr_nodes != 0) {
		draw_state->draw_rr_node = (t_draw_rr_node *) my_realloc(draw_state->draw_rr_node,
										(num_rr_nodes) * sizeof(t_draw_rr_node));
		for (i = 0; i < num_rr_nodes; i++) {
			draw_state->draw_rr_node[i].color = DEFAULT_RR_NODE_COLOR;
			draw_state->draw_rr_node[i].node_highlighted = false;
		}
	}

	draw_coords->tile_width = width_val;
	draw_coords->pin_size = 0.3;
	for (i = 0; i < num_types; ++i) {
		draw_coords->pin_size = min(draw_coords->pin_size,
				(draw_coords->tile_width / (4.0F * type_descriptors[i].num_pins)));
	}

	j = 0;
	for (i = 0; i < (nx + 1); i++) {
		draw_coords->tile_x[i] = (i * draw_coords->tile_width) + j;
		j += chan_width.y_list[i] + 1; /* N wires need N+1 units of space */

		// to show the interposer wires, we need more space between the blocks
		#ifdef INTERPOSER_BASED_ARCHITECTURE
		j += (chan_width.y_list[i] + 1);
		#endif
	}
	draw_coords->tile_x[nx + 1] = ((nx + 1) * draw_coords->tile_width) + j;

	j = 0;
	for (i = 0; i < (ny + 1); ++i) {
		draw_coords->tile_y[i] = (i * draw_coords->tile_width) + j;
		j += chan_width.x_list[i] + 1;

		// to show the interposer wires, we need more space between the blocks
		#ifdef INTERPOSER_BASED_ARCHITECTURE
		j += (chan_width.x_list[i] + 1);
		#endif
	}
	draw_coords->tile_y[ny + 1] = ((ny + 1) * draw_coords->tile_width) + j;

	/* Load coordinates of sub-blocks inside the clbs */
	draw_internal_init_blk();

	init_world(0.0, draw_coords->tile_y[ny + 1] + draw_coords->tile_width, 
			   draw_coords->tile_x[nx + 1] + draw_coords->tile_width, 0.0);
}

static void drawplace(void) {

	/* Draws the blocks placed on the proper clbs.  Occupied blocks are darker colours *
	 * while empty ones are lighter colours and have a dashed border.      */

	float sub_tile_step;
	float x1, y1, x2, y2;
	int i, j, k, bnum;
	int num_sub_tiles;
	int height;

	setlinewidth(0);

	for (i = 0; i <= (nx + 1); i++) {
		for (j = 0; j <= (ny + 1); j++) {
			/* Only the first block of a group should control drawing */
			if (grid[i][j].width_offset > 0 || grid[i][j].height_offset > 0) 
				continue;

			num_sub_tiles = grid[i][j].type->capacity;
			sub_tile_step = draw_coords->tile_width / num_sub_tiles;
			height = grid[i][j].type->height;

			/* Don't draw if tile capacity is zero. This includes corners. */
			if (num_sub_tiles == 0)
				continue;

			for (k = 0; k < num_sub_tiles; ++k) {
				/* Graphics will look unusual for multiple height and capacity */
				assert(height == 1 || num_sub_tiles == 1);
				/* Get coords of current sub_tile */
				if ((j < 1) || (j > ny)) { /* top and bottom fringes */
					/* Only for the top and bottom fringes (ie. io blocks place on top 
					 * and bottom edges of the chip), sub_tiles are placed next to each 
					 * other horizontally, because it is easier to visualize the 
					 * connection from each individual io block to other parts of the 
					 * chip when toggle_nets is selected.
					 */
					x1 = draw_coords->tile_x[i] + (k * sub_tile_step);
					y1 = draw_coords->tile_y[j];
					x2 = x1 + sub_tile_step;
					y2 = y1 + draw_coords->tile_width;
				}
				else {
					/* Sub_tiles are stacked on top of each other vertically. */
					x1 = draw_coords->tile_x[i];
					y1 = draw_coords->tile_y[j] + (k * sub_tile_step);
					x2 = x1 + draw_coords->tile_width;
					y2 = draw_coords->tile_y[j + height - 1] + ((k + 1) * sub_tile_step);
				}

				/* Look at the tile at start of large block */
				bnum = grid[i][j].blocks[k];

				/* Fill background for the clb. Do not fill if "show_blk_internal" 
				 * is toggled. 
				 */
				if (bnum != EMPTY && bnum != INVALID) {
					setcolor(draw_state->block_color[bnum]);
					fillrect(x1, y1, x2, y2);
				} else {
					/* colour empty blocks a particular colour depending on type  */
					if (grid[i][j].type->index < 3) {
						setcolor(WHITE);
					} else if (grid[i][j].type->index < 3 + MAX_BLOCK_COLOURS) {
						setcolor(BISQUE + grid[i][j].type->index - 3);
					} else {
						setcolor(BISQUE + MAX_BLOCK_COLOURS - 1);
					}
					fillrect(x1, y1, x2, y2);
				}

				setcolor(BLACK);

				setlinestyle((EMPTY == bnum) ? DASHED : SOLID);
				drawrect(x1, y1, x2, y2);

				/* Draw text if the space has parts of the netlist */
				if (bnum != EMPTY && bnum != INVALID) {
					drawtext((x1 + x2) / 2.0, (y1 + y2) / 2.0, block[bnum].name,
							sub_tile_step);
				}

				/* Draw text for block type so that user knows what block */
				if (grid[i][j].width_offset == 0 && grid[i][j].height_offset == 0) {
					if (i > 0 && i <= nx && j > 0 && j <= ny) {
						drawtext((x1 + x2) / 2.0, y1 + (draw_coords->tile_width / 4.0),
								grid[i][j].type->name, sub_tile_step);
					}
				}
			}
		}
	}
}

static void drawnets(void) {

	/* This routine draws the nets on the placement.  The nets have not *
	 * yet been routed, so we just draw a chain showing a possible path *
	 * for each net.  This gives some idea of future congestion.        */

	unsigned ipin, inet;
	int b1, b2;
	float x1, y1, x2, y2;

	setlinestyle(SOLID);
	setlinewidth(0);

	/* Draw the net as a star from the source to each sink. Draw from centers of *
	 * blocks (or sub blocks in the case of IOs).                                */

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global)
			continue; /* Don't draw global nets. */

		setcolor(draw_state->net_color[inet]);
		b1 = g_clbs_nlist.net[inet].pins[0].block; /* The DRIVER */
		get_block_center(b1, &x1, &y1);

		for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
			b2 = g_clbs_nlist.net[inet].pins[ipin].block;
			get_block_center(b2, &x2, &y2);
			drawline(x1, y1, x2, y2);

			/* Uncomment to draw a chain instead of a star. */
			/*      x1 = x2;  */
			/*      y1 = y2;  */
		}
	}
}

static void get_block_center(int bnum, float *x, float *y) {

	/* This routine finds the center of block bnum in the current placement, *
	 * and returns it in *x and *y.  This is used in routine shownets.       */

	int i, j, k;
	float sub_tile_step;

	i = block[bnum].x;
	j = block[bnum].y;
	k = block[bnum].z;

	sub_tile_step = draw_coords->tile_width / block[bnum].type->capacity;

	if ((j < 1) || (j > ny)) { /* Top and bottom fringe */
		/* For the top and bottom fringes (ie. io blocks place on top 
		 * and bottom edges of the chip), sub_tiles are placed next to each 
		 * other horizontally. Therefore, for the center of each io 
		 * sub_block, the x-coordinate varies but the y-coordinate is half
		 * tile_width above the bottom of the tile.
		 */
		*x = draw_coords->tile_x[i] + (sub_tile_step * (k + 0.5));
		*y = draw_coords->tile_y[j] + (draw_coords->tile_width / 2.0);
	} else {
		/* For everything other than the top and bottom io blocks, sub_tiles
		 * (if any) are stacked vertically. Therefore, y-coordinate changes
		 * for each io sub_block but x-coordinate is half way across the 
		 * block.
		 */
		*x = draw_coords->tile_x[i] + (draw_coords->tile_width / 2.0);
		*y = draw_coords->tile_y[j] + (sub_tile_step * (k + 0.5));
	}
}

static void draw_congestion(void) {

	/* Draws all the overused routing resources (i.e. congestion) in RED.   */

	int inode, itrack;

	setlinewidth(2);

	for (inode = 0; inode < num_rr_nodes; inode++) {
		if (rr_node[inode].occ > 0) {
			switch (rr_node[inode].type) {
			case CHANX:
				itrack = rr_node[inode].ptc_num;
				if (draw_state->show_congestion == DRAW_CONGESTED &&
					rr_node[inode].occ > rr_node[inode].capacity) {
					draw_rr_chanx(inode, itrack, RED);
				}
				else if (draw_state->show_congestion == DRAW_CONGESTED_AND_USED) {
					if (rr_node[inode].occ > rr_node[inode].capacity)
						draw_rr_chanx(inode, itrack, RED);
					else
						draw_rr_chanx(inode, itrack, BLUE);
				}
				break;

			case CHANY:
				itrack = rr_node[inode].ptc_num;
				if (draw_state->show_congestion == DRAW_CONGESTED &&
					rr_node[inode].occ > rr_node[inode].capacity) {
					draw_rr_chany(inode, itrack, RED);
				}
				else if (draw_state->show_congestion == DRAW_CONGESTED_AND_USED) {
					if (rr_node[inode].occ > rr_node[inode].capacity)
						draw_rr_chany(inode, itrack, RED);
					else
						draw_rr_chany(inode, itrack, BLUE);
				}
				break;

			case IPIN:
			case OPIN:
				if (draw_state->show_congestion == DRAW_CONGESTED &&
					rr_node[inode].occ > rr_node[inode].capacity) {
					draw_rr_pin(inode, RED);
				}
				else if (draw_state->show_congestion == DRAW_CONGESTED_AND_USED) {
					if (rr_node[inode].occ > rr_node[inode].capacity)
						draw_rr_pin(inode, RED);
					else
						draw_rr_pin(inode, BLUE);
				}
				break;
			default:
				break;
			}
		}
	}
}


void draw_rr(void) {

	/* Draws the routing resources that exist in the FPGA, if the user wants *
	 * them drawn.                                                           */

	int inode, itrack;

	if (draw_state->draw_rr_toggle == DRAW_NO_RR) {
		setlinewidth(3);
		drawroute(HIGHLIGHTED);
		setlinewidth(0);
		return;
	}

	setlinestyle(SOLID);

	for (inode = 0; inode < num_rr_nodes; inode++) {
		if (!draw_state->draw_rr_node[inode].node_highlighted) 
		{
			/* If not highlighted node, assign color based on type. */
			switch (rr_node[inode].type) {
			case CHANX:
			case CHANY:
				draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
				break;
			case OPIN:
				draw_state->draw_rr_node[inode].color = PINK;
				break;
			case IPIN:
				draw_state->draw_rr_node[inode].color = LIGHTSKYBLUE;
				break;
			default:
				break;
			}
		}

		/* Now call drawing routines to draw the node. */
		switch (rr_node[inode].type) {

		case SOURCE:
		case SINK:
			break; /* Don't draw. */

		case CHANX:
			itrack = rr_node[inode].ptc_num;
			draw_rr_chanx(inode, itrack, draw_state->draw_rr_node[inode].color);
			draw_rr_edges(inode);
			break;

		case CHANY:
			itrack = rr_node[inode].ptc_num;
			draw_rr_chany(inode, itrack, draw_state->draw_rr_node[inode].color);
			draw_rr_edges(inode);
			break;

		case IPIN:
			draw_rr_pin(inode, draw_state->draw_rr_node[inode].color);
			break;

		case OPIN:
			draw_rr_pin(inode, draw_state->draw_rr_node[inode].color);
			draw_rr_edges(inode);
			break;

		default:
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
					"in draw_rr: Unexpected rr_node type: %d.\n", rr_node[inode].type);
		}
	}

	drawroute(HIGHLIGHTED);
}


static void draw_rr_chanx(int inode, int itrack, enum color_types color) {

	/* Draws an x-directed channel segment.                       */

	enum {
		BUFFSIZE = 80
	};
	t_draw_bbox bound_box;
	float wire_start_y1, wire_start_y2; 
	int k; 
	char str[BUFFSIZE];

	// For CHANX, bound_box.ybottom is same as bound_box.ytop
	bound_box = draw_get_rr_chan_bbox(inode);

	setcolor(color);
	if (color != DEFAULT_RR_NODE_COLOR) {
		// If wire is highlighted, then draw with thicker linewidth.
		setlinewidth(3);
		drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);
		setlinewidth(0);
	}
	else
		drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);

	wire_start_y1 = bound_box.ybottom - 0.25;
	wire_start_y2 = bound_box.ytop + 0.25;

	if (rr_node[inode].direction == INC_DIRECTION) {
		setlinewidth(2);
		setcolor(YELLOW);
		/* Draw a line at start of wire to indicate mux */
		drawline(bound_box.xleft, wire_start_y1, bound_box.xleft, wire_start_y2); 

		/* Mux balence numbers */
		setcolor(BLACK);
		sprintf(str, "%d", rr_node[inode].fan_in);
		drawtext(bound_box.xleft, bound_box.ybottom, str, 5);

		setcolor(BLACK);
		setlinewidth(0);
		draw_triangle_along_line(bound_box.xright - 0.15, bound_box.ytop, bound_box.xleft, 
								 bound_box.xright, bound_box.ybottom, bound_box.ytop);

		setcolor(LIGHTGREY);
		/* TODO: this looks odd, why does it ignore final block? does this mean nothing 
		 * appears with L=1 ? 
		 */
		for (k = rr_node[inode].xlow; k < rr_node[inode].xhigh; k++) {
			bound_box.xright = draw_coords->tile_x[k] + draw_coords->tile_width;
			draw_triangle_along_line(bound_box.xright - 0.15, bound_box.ytop, bound_box.xleft, 
									 bound_box.xright, bound_box.ybottom, bound_box.ytop);
			bound_box.xright = draw_coords->tile_x[k + 1];
			draw_triangle_along_line(bound_box.xright + 0.15, bound_box.ytop, bound_box.xleft, 
									 bound_box.xright, bound_box.ybottom, bound_box.ytop);
		}
		setcolor(color);
	} else if (rr_node[inode].direction == DEC_DIRECTION) {
		setlinewidth(2);
		setcolor(YELLOW);
		drawline(bound_box.xright, wire_start_y1, bound_box.xright, wire_start_y2);

		/* Mux balance numbers */
		setcolor(BLACK);
		sprintf(str, "%d", rr_node[inode].fan_in);
		drawtext(bound_box.xright, bound_box.ybottom, str, 5);

		setlinewidth(0);
		draw_triangle_along_line(bound_box.xleft + 0.15, bound_box.ybottom, bound_box.xright, 
								 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
		setcolor(LIGHTGREY);
		for (k = rr_node[inode].xhigh; k > rr_node[inode].xlow; k--) {
			bound_box.xleft = draw_coords->tile_x[k];
			draw_triangle_along_line(bound_box.xleft + 0.15, bound_box.ybottom, bound_box.xright, 
									 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
			bound_box.xleft = draw_coords->tile_x[k - 1] + draw_coords->tile_width;
			draw_triangle_along_line(bound_box.xleft - 0.15, bound_box.ybottom, bound_box.xright, 
									 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
		}
		setcolor(color);
	}
}

static void draw_rr_chany(int inode, int itrack, enum color_types color) {

	/* Draws a y-directed channel segment.                       */
	enum {
		BUFFSIZE = 80
	};
	t_draw_bbox bound_box;
	float wire_start_x1, wire_start_x2; 
	int k; 
	char str[BUFFSIZE];

	// Get the coordinates of the channel wire segment.
	// For CHANY, bound_box.xleft is equal to bound_box.xright.
	bound_box = draw_get_rr_chan_bbox(inode);

	setcolor(color);
	if (color != DEFAULT_RR_NODE_COLOR) 
	{
		// If wire is highlighted, then draw with thicker linewidth.
		setlinewidth(3);

		#ifdef INTERPOSER_BASED_ARCHITECTURE
		if(is_inode_an_interposer_wire(inode))
		{
			draw_shifted_line(inode);
		}
		else
		{
			drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);
		}
		#else
		drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);
		#endif		
		
		setlinewidth(0);
	}
	else
	{
		#ifdef INTERPOSER_BASED_ARCHITECTURE
		if(is_inode_an_interposer_wire(inode))
		{
			draw_shifted_line(inode);
		}
		else
		{
			drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);
		}
		#else
		drawline(bound_box.xleft, bound_box.ybottom, bound_box.xright, bound_box.ytop);
		#endif
	}

	wire_start_x1 = bound_box.xleft - 0.25;
	wire_start_x2 = bound_box.xright + 0.25;
	
	if (rr_node[inode].direction == INC_DIRECTION) {
		setlinewidth(2);
		setcolor(YELLOW);
		drawline(wire_start_x1, bound_box.ybottom, wire_start_x2, bound_box.ybottom);

		setcolor(BLACK);
		sprintf(str, "%d", rr_node[inode].fan_in);
		drawtext(bound_box.xleft, bound_box.ybottom, str, 5);
		setcolor(BLACK);

		setlinewidth(0);
		draw_triangle_along_line(bound_box.xright, bound_box.ytop - 0.15, bound_box.xleft, 
								 bound_box.xright, bound_box.ybottom, bound_box.ytop);
		setcolor(LIGHTGREY);
		for (k = rr_node[inode].ylow; k < rr_node[inode].yhigh; k++) {
			bound_box.ytop = draw_coords->tile_y[k] + draw_coords->tile_width;
			draw_triangle_along_line(bound_box.xright, bound_box.ytop - 0.15, bound_box.xleft, 
									 bound_box.xright, bound_box.ybottom, bound_box.ytop);
			bound_box.ytop = draw_coords->tile_y[k + 1];
			draw_triangle_along_line(bound_box.xright, bound_box.ytop + 0.15, bound_box.xleft, 
									 bound_box.xright, bound_box.ybottom, bound_box.ytop);
		}
		setcolor(color);
	} else if (rr_node[inode].direction == DEC_DIRECTION) {
		setlinewidth(2);
		setcolor(YELLOW);
		drawline(wire_start_x1, bound_box.ytop, wire_start_x2, bound_box.ytop);

		setcolor(BLACK);
		sprintf(str, "%d", rr_node[inode].fan_in);
		drawtext(bound_box.xleft, bound_box.ytop, str, 5);
		setcolor(BLACK);

		setlinewidth(0);
		draw_triangle_along_line(bound_box.xleft, bound_box.ybottom + 0.15, bound_box.xright, 
								 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
		setcolor(LIGHTGREY);
		for (k = rr_node[inode].yhigh; k > rr_node[inode].ylow; k--) {
			bound_box.ybottom = draw_coords->tile_y[k];
			draw_triangle_along_line(bound_box.xleft, bound_box.ybottom + 0.15, bound_box.xright, 
									 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
			bound_box.ybottom = draw_coords->tile_y[k - 1] + draw_coords->tile_width;
			draw_triangle_along_line(bound_box.xleft, bound_box.ybottom - 0.15, bound_box.xright, 
									 bound_box.xleft, bound_box.ytop, bound_box.ybottom);
		}
		setcolor(color);
	}
}

static void draw_rr_edges(int inode) {

	/* Draws all the edges that the user wants shown between inode and what it *
	 * connects to.  inode is assumed to be a CHANX, CHANY, or OPIN.           */

	t_rr_type from_type, to_type;
	int iedge, to_node, from_ptc_num, to_ptc_num;
	short switch_type;

	from_type = rr_node[inode].type;

	if ((draw_state->draw_rr_toggle == DRAW_NODES_RR)
			|| (draw_state->draw_rr_toggle == DRAW_NODES_AND_SBOX_RR && from_type == OPIN)) {
		return; /* Nothing to draw. */
	}

	from_ptc_num = rr_node[inode].ptc_num;

	for (iedge = 0; iedge < rr_node[inode].num_edges; iedge++) {
		to_node = rr_node[inode].edges[iedge];
		to_type = rr_node[to_node].type;
		to_ptc_num = rr_node[to_node].ptc_num;

		switch (from_type) {

		case OPIN:
			switch (to_type) {
			case CHANX:
			case CHANY:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					// If OPIN was clicked on, set color to fan-out
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					// If CHANX or CHANY got clicked, set color to fan-in
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(PINK);
				draw_pin_to_chan_edge(inode, to_node);
				break;
			case IPIN:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(MEDIUMPURPLE);
				draw_pin_to_pin(inode, to_node);
				break;
			default:
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
						inode, from_type, to_node, to_type);
				break;
			}
			break;

		case CHANX: /* from_type */
			switch (to_type) {
			case IPIN:
				if (draw_state->draw_rr_toggle == DRAW_NODES_AND_SBOX_RR) {
					break;
				}

				if (draw_state->draw_rr_node[to_node].node_highlighted &&
					draw_state->draw_rr_node[inode].color == DEFAULT_RR_NODE_COLOR) {
					// If the IPIN is clicked on, draw connection to all the CHANX
					// wire segments fanning into the pin. If a CHANX wire is clicked
					// on, draw only the connection between that wire and the IPIN, with
					// the pin fanning out from the wire.
					break;
				}

				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(LIGHTSKYBLUE);
				draw_pin_to_chan_edge(to_node, inode);
				break;

			case CHANX:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(DARKGREEN);
				switch_type = rr_node[inode].switches[iedge];
				draw_chanx_to_chanx_edge(inode, from_ptc_num, to_node,
						to_ptc_num, switch_type);
				break;

			case CHANY:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(DARKGREEN);
				switch_type = rr_node[inode].switches[iedge];
				draw_chanx_to_chany_edge(inode, from_ptc_num, to_node,
						to_ptc_num, FROM_X_TO_Y, switch_type);
				break;

			default:
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
						inode, from_type, to_node, to_type);
				break;
			}
			break;

		case CHANY: /* from_type */
			switch (to_type) {
			case IPIN:
				if (draw_state->draw_rr_toggle == DRAW_NODES_AND_SBOX_RR) {
					break;
				}

				if (draw_state->draw_rr_node[to_node].node_highlighted &&
					draw_state->draw_rr_node[inode].color == DEFAULT_RR_NODE_COLOR) {
					// If the IPIN is clicked on, draw connection to all the CHANY
					// wire segments fanning into the pin. If a CHANY wire is clicked
					// on, draw only the connection between that wire and the IPIN, with
					// the pin fanning out from the wire.
					break;
				}

				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(LIGHTSKYBLUE);
				draw_pin_to_chan_edge(to_node, inode);
				break;

			case CHANX:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(DARKGREEN);
				switch_type = rr_node[inode].switches[iedge];
				draw_chanx_to_chany_edge(to_node, to_ptc_num, inode,
						from_ptc_num, FROM_Y_TO_X, switch_type);
				break;

			case CHANY:
				if (draw_state->draw_rr_node[inode].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[to_node].color);
				} else if (draw_state->draw_rr_node[to_node].color == MAGENTA) {
					setcolor(draw_state->draw_rr_node[inode].color);
				} else
					setcolor(DARKGREEN);
				switch_type = rr_node[inode].switches[iedge];
				draw_chany_to_chany_edge(inode, from_ptc_num, to_node,
						to_ptc_num, switch_type);
				break;

			default:
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
						inode, from_type, to_node, to_type);
				break;
			}
			break;

		default: /* from_type */
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
					"draw_rr_edges called with node %d of type %d.\n", 
					inode, from_type);
			break;
		}
	} /* End of for each edge loop */
}

static void draw_x(float x, float y, float size) {

	/* Draws an X centered at (x,y).  The width and height of the X are each    *
	 * 2 * size.                                                                */

	drawline(x - size, y + size, x + size, y - size);
	drawline(x - size, y - size, x + size, y + size);
}


static void draw_chanx_to_chany_edge(int chanx_node, int chanx_track,
		int chany_node, int chany_track, enum e_edge_dir edge_dir,
		short switch_type) {

	/* Draws an edge (SBOX connection) between an x-directed channel and a    *
	 * y-directed channel.                                                    */

	float x1, y1, x2, y2;
	t_draw_bbox chanx_bbox, chany_bbox;
	int chanx_xlow, chany_x, chany_ylow, chanx_y;

	/* Get the coordinates of the CHANX and CHANY segments. */
	chanx_bbox = draw_get_rr_chan_bbox(chanx_node);
	chany_bbox = draw_get_rr_chan_bbox(chany_node);

	/* (x1,y1): point on CHANX segment, (x2,y2): point on CHANY segment. */

	y1 = chanx_bbox.ybottom; 
	x2 = chany_bbox.xleft;

	chanx_xlow = rr_node[chanx_node].xlow;
	chanx_y = rr_node[chanx_node].ylow;
	chany_x = rr_node[chany_node].xlow;
	chany_ylow = rr_node[chany_node].ylow;

	if (chanx_xlow <= chany_x) { /* Can draw connection going right */
		/* Connection not at end of the CHANX segment. */
		x1 = draw_coords->tile_x[chany_x] + draw_coords->tile_width;
		
		if (rr_node[chanx_node].direction != BI_DIRECTION) {
			if (edge_dir == FROM_X_TO_Y) {
				if ((chanx_track % 2) == 1) { /* If dec wire, then going left */
					x1 = draw_coords->tile_x[chany_x + 1];
				}
			}
		}
		
	} else { /* Must draw connection going left. */
		x1 = chanx_bbox.xleft;
	}

	if (chany_ylow <= chanx_y) { /* Can draw connection going up. */
		/* Connection not at end of the CHANY segment. */
		y2 = draw_coords->tile_y[chanx_y] + draw_coords->tile_width;
		
		if (rr_node[chany_node].direction != BI_DIRECTION) {
			if (edge_dir == FROM_Y_TO_X) {
				if ((chany_track % 2) == 1) { /* If dec wire, then going down */
					y2 = draw_coords->tile_y[chanx_y + 1];
				}
			}
		}
		
	} else { /* Must draw connection going down. */
		y2 = chany_bbox.ybottom;
	}

#ifdef INTERPOSER_BASED_ARCHITECTURE
	// handle CHANY connections to interposer wires
	// connect to the bottom of an interposer wire
	if(chany_track >= chan_width.y_list[chany_x])
	{
		y2 = draw_coords->tile_y[chanx_y] + draw_coords->tile_width + chan_width.y_list[chany_x] + 0.25*chan_width.y_list[chany_x];
	}
#endif 

	drawline(x1, y1, x2, y2);

	if (draw_state->draw_rr_toggle != DRAW_ALL_RR) {
		if (draw_state->draw_rr_node[chanx_node].node_highlighted) {
			float xend, yend;
			xend = x2 + (x1 - x2) / 10.;
			yend = y2 + (y1 - y2) / 10.;
			// Draw arrow showing direction
			draw_triangle_along_line(xend, yend, x1, x2, y1, y2);
		}
		return;
	}

	if (edge_dir == FROM_X_TO_Y)
		draw_rr_switch(x1, y1, x2, y2, switch_inf[switch_type].buffered);
	else
		draw_rr_switch(x2, y2, x1, y1, switch_inf[switch_type].buffered);
}


static void draw_chanx_to_chanx_edge(int from_node, int from_track, int to_node,
		int to_track, short switch_type) {

	/* Draws a connection between two x-channel segments.  Passing in the track *
	 * numbers allows this routine to be used for both rr_graph and routing     *
	 * drawing.                                                                 */

	float x1, x2, y1, y2;
	t_draw_bbox from_chan, to_chan;
	int from_xlow, to_xlow, from_xhigh, to_xhigh;
	
	// Get the coordinates of the channel wires.
	from_chan = draw_get_rr_chan_bbox(from_node);
	to_chan = draw_get_rr_chan_bbox(to_node);

	/* (x1, y1) point on from_node, (x2, y2) point on to_node. */

	y1 = from_chan.ybottom;
	y2 = to_chan.ybottom;

	from_xlow = rr_node[from_node].xlow;
	from_xhigh = rr_node[from_node].xhigh;
	to_xlow = rr_node[to_node].xlow;
	to_xhigh = rr_node[to_node].xhigh;

	if (to_xhigh < from_xlow) { /* From right to left */
		/* UDSD Note by WMF: could never happen for INC wires, unless U-turn. For DEC 
		 * wires this handles well */
		x1 = from_chan.xleft;
		x2 = to_chan.xright;
	} else if (to_xlow > from_xhigh) { /* From left to right */
		/* UDSD Note by WMF: could never happen for DEC wires, unless U-turn. For INC 
		 * wires this handles well */
		x1 = from_chan.xright;
		x2 = to_chan.xleft;
	}

	/* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
	 * make sure the drawing is symmetric in the from rr and to rr so the edges *
	 * will be drawn on top of each other for bidirectional connections.        */

	else {
		if (rr_node[to_node].direction != BI_DIRECTION) {
			/* must connect to to_node's wire beginning at x2 */
			if (to_track % 2 == 0) { /* INC wire starts at leftmost edge */
				assert(from_xlow < to_xlow);
				x2 = to_chan.xleft;
				/* since no U-turns from_track must be INC as well */
				x1 = draw_coords->tile_x[to_xlow - 1] + draw_coords->tile_width;
			} else { /* DEC wire starts at rightmost edge */
				assert(from_xhigh > to_xhigh);
				x2 = to_chan.xright;
				x1 = draw_coords->tile_x[to_xhigh + 1];
			}
		} else {
			if (to_xlow < from_xlow) { /* Draw from left edge of one to other */
				x1 = from_chan.xleft;
				x2 = draw_coords->tile_x[from_xlow - 1] + draw_coords->tile_width;
			} else if (from_xlow < to_xlow) {
				x1 = draw_coords->tile_x[to_xlow - 1] + draw_coords->tile_width;
				x2 = to_chan.xleft;
			} /* The following then is executed when from_xlow == to_xlow */
			else if (to_xhigh > from_xhigh) { /* Draw from right edge of one to other */
				x1 = from_chan.xright;
				x2 = draw_coords->tile_x[from_xhigh + 1];
			} else if (from_xhigh > to_xhigh) {
				x1 = draw_coords->tile_x[to_xhigh + 1];
				x2 = to_chan.xright;
			} else { /* Complete overlap: start and end both align. Draw outside the sbox */
				x1 = from_chan.xleft;
				x2 = from_chan.xleft + draw_coords->tile_width;
			}
		}
	}
	
	drawline(x1, y1, x2, y2);

	if (draw_state->draw_rr_toggle == DRAW_ALL_RR)
		draw_rr_switch(x1, y1, x2, y2, switch_inf[switch_type].buffered);
	else if (draw_state->draw_rr_node[from_node].node_highlighted) {
		float xend, yend;
		xend = x2 + (x1 - x2) / 10.;
		yend = y2 + (y1 - y2) / 10.;
		// Draw arrow showing direction
		draw_triangle_along_line(xend, yend, x1, x2, y1, y2);
	}
}


static void draw_chany_to_chany_edge(int from_node, int from_track, int to_node,
		int to_track, short switch_type) {

	/* Draws a connection between two y-channel segments.  Passing in the track *
	 * numbers allows this routine to be used for both rr_graph and routing     *
	 * drawing.                                                                 */

	float x1, x2, y1, y2;
	t_draw_bbox from_chan, to_chan;
	int from_ylow, to_ylow, from_yhigh, to_yhigh, from_x, to_x;

	// Get the coordinates of the channel wires.
	from_chan = draw_get_rr_chan_bbox(from_node);
	to_chan = draw_get_rr_chan_bbox(to_node);

	from_x = rr_node[from_node].xlow;
	to_x = rr_node[to_node].xlow;
	from_ylow = rr_node[from_node].ylow;
	from_yhigh = rr_node[from_node].yhigh;
	to_ylow = rr_node[to_node].ylow;
	to_yhigh = rr_node[to_node].yhigh;

	/* (x1, y1) point on from_node, (x2, y2) point on to_node. */

	x1 = from_chan.xleft;
	x2 = to_chan.xleft;

	if (to_yhigh < from_ylow) { /* From upper to lower */
		y1 = from_chan.ybottom;
		y2 = to_chan.ytop;
	} else if (to_ylow > from_yhigh) { /* From lower to upper */
		y1 = from_chan.ytop;
		y2 = to_chan.ybottom;
	}

	/* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
	 * make sure the drawing is symmetric in the from rr and to rr so the edges *
	 * will be drawn on top of each other for bidirectional connections.        */

	/* UDSD Modification by WMF Begin */
	else {
		if (rr_node[to_node].direction != BI_DIRECTION) {
			if (to_track % 2 == 0) { /* INC wire starts at bottom edge */
			
				// for interposer-based architectures, the interposer node will have
				// the same y_low and y_high as the cutline y-coordinate
				// so, for connections from CHANY wire just below the cut to the interposer node,
				// from_ylow can be equal to to_ylow
				#ifndef INTERPOSER_BASED_ARCHITECTURE
				assert(from_ylow < to_ylow);
				#endif
				
				y2 = to_chan.ybottom;
				/* since no U-turns from_track must be INC as well */
				y1 = draw_coords->tile_y[to_ylow - 1] + draw_coords->tile_width;
			} else { /* DEC wire starts at top edge */
			
				// for interposer-based architectures, the interposer node is marked as CHANY
				// so, it is possible to have from_yhigh equal to to_yhigh
				#ifndef INTERPOSER_BASED_ARCHITECTURE
				if (!(from_yhigh > to_yhigh)) {
					vpr_printf_info("from_yhigh (%d) !> to_yhigh (%d).\n", 
							from_yhigh, to_yhigh);
					vpr_printf_info("from is (%d, %d) to (%d, %d) track %d.\n",
							rr_node[from_node].xhigh, rr_node[from_node].yhigh,
							rr_node[from_node].xlow, rr_node[from_node].ylow,
							rr_node[from_node].ptc_num);
					vpr_printf_info("to is (%d, %d) to (%d, %d) track %d.\n",
							rr_node[to_node].xhigh, rr_node[to_node].yhigh,
							rr_node[to_node].xlow, rr_node[to_node].ylow,
							rr_node[to_node].ptc_num);
					exit(1);
				}
				#endif
				
				y2 = to_chan.ytop;
				y1 = draw_coords->tile_y[to_yhigh + 1];
			}
		} else {
			if (to_ylow < from_ylow) { /* Draw from bottom edge of one to other. */
				y1 = from_chan.ybottom;
				y2 = draw_coords->tile_y[from_ylow - 1] + draw_coords->tile_width;
			} else if (from_ylow < to_ylow) {
				y1 = draw_coords->tile_y[to_ylow - 1] + draw_coords->tile_width;
				y2 = to_chan.ybottom;
			} else if (to_yhigh > from_yhigh) { /* Draw from top edge of one to other. */
				y1 = from_chan.ytop;
				y2 = draw_coords->tile_y[from_yhigh + 1];
			} else if (from_yhigh > to_yhigh) {
				y1 = draw_coords->tile_y[to_yhigh + 1];
				y2 = to_chan.ytop;
			} else { /* Complete overlap: start and end both align. Draw outside the sbox */
				y1 = from_chan.ybottom;
				y2 = from_chan.ybottom + draw_coords->tile_width;
			}
		}
	}

#ifdef INTERPOSER_BASED_ARCHITECTURE
	//handle CHANX connections to and from interposer wires
	if(is_inode_an_interposer_wire(from_node))
	{
		if(rr_node[from_node].direction == DEC_DIRECTION)
		{
			y1 = draw_coords->tile_y[from_ylow] + draw_coords->tile_width + chan_width.y_list[from_x] + 0.25*chan_width.y_list[from_x];
		}
		else if(rr_node[from_node].direction == INC_DIRECTION)
		{
			y1 = draw_coords->tile_y[from_ylow] + draw_coords->tile_width + 2*chan_width.y_list[from_x] - 0.25*chan_width.y_list[from_x];
		}
	}
	if(is_inode_an_interposer_wire(to_node))
	{
		if(rr_node[to_node].direction == INC_DIRECTION)
		{
			y2 = draw_coords->tile_y[to_ylow] + draw_coords->tile_width + chan_width.y_list[to_x] + 0.25*chan_width.y_list[to_x];
		}
		else if(rr_node[to_node].direction == DEC_DIRECTION)
		{
			y2 = draw_coords->tile_y[to_ylow] + draw_coords->tile_width + 2*chan_width.y_list[to_x] - 0.25*chan_width.y_list[to_x];
		}
	}
#endif

	/* UDSD Modification by WMF End */
	drawline(x1, y1, x2, y2);

	if (draw_state->draw_rr_toggle == DRAW_ALL_RR)
		draw_rr_switch(x1, y1, x2, y2, switch_inf[switch_type].buffered);
	else if (draw_state->draw_rr_node[from_node].node_highlighted) {
		float xend, yend;
		xend = x2 + (x1 - x2) / 10.;
		yend = y2 + (y1 - y2) / 10.;
		// Draw arrow showing direction
		draw_triangle_along_line(xend, yend, x1, x2, y1, y2);
	}
}


/* This function computes and returns the boundary coordinates of a channel
 * wire segment. This can be used for drawing a wire or determining if a 
 * wire has been clicked on by the user. 
 * TODO: Fix this for global routing, currently for detailed only. 
 */
static t_draw_bbox draw_get_rr_chan_bbox (int inode) {
	t_draw_bbox bound_box;

	switch (rr_node[inode].type) {
		case CHANX:
			bound_box.xleft = draw_coords->tile_x[rr_node[inode].xlow];
	        bound_box.xright = draw_coords->tile_x[rr_node[inode].xhigh] 
						        + draw_coords->tile_width;
			bound_box.ybottom = draw_coords->tile_y[rr_node[inode].ylow] 
								+ draw_coords->tile_width + (1. + rr_node[inode].ptc_num);
			bound_box.ytop = draw_coords->tile_y[rr_node[inode].ylow] 
								+ draw_coords->tile_width + (1. + rr_node[inode].ptc_num);
			break;
		case CHANY:
			bound_box.xleft = draw_coords->tile_x[rr_node[inode].xlow] 
								+ draw_coords->tile_width + (1. + rr_node[inode].ptc_num);
			bound_box.xright = draw_coords->tile_x[rr_node[inode].xlow] 
								+ draw_coords->tile_width + (1. + rr_node[inode].ptc_num);
			bound_box.ybottom = draw_coords->tile_y[rr_node[inode].ylow];
			bound_box.ytop = draw_coords->tile_y[rr_node[inode].yhigh] 
								+ draw_coords->tile_width;
			break;
		default:
			bound_box.xleft = -1;
			bound_box.xright = -1;
			bound_box.ybottom = -1;
			bound_box.ytop = -1;
			break;
	}

	return bound_box;
}


static void draw_rr_switch(float from_x, float from_y, float to_x, float to_y,
		boolean buffered) {

	/* Draws a buffer (triangle) or pass transistor (circle) on the edge        *
	 * connecting from to to, depending on the status of buffered.  The drawing *
	 * is closest to the from_node, since it reflects the switch type of from.  */

	const float switch_rad = 0.15;
	float magnitude, xcen, ycen, xdelta, ydelta, xbaseline, ybaseline;
	float xunit, yunit;
	t_point poly[3];

	xcen = from_x + (to_x - from_x) / 10.;
	ycen = from_y + (to_y - from_y) / 10.;

	if (!buffered) { /* Draw a circle for a pass transistor */
		drawarc(xcen, ycen, switch_rad, 0., 360.);
	} else { /* Buffer */
		xdelta = to_x - from_x;
		ydelta = to_y - from_y;
		magnitude = sqrt(xdelta * xdelta + ydelta * ydelta);
		xunit = xdelta / magnitude;
		yunit = ydelta / magnitude;
		poly[0].x = xcen + xunit * switch_rad;
		poly[0].y = ycen + yunit * switch_rad;
		xbaseline = xcen - xunit * switch_rad;
		ybaseline = ycen - yunit * switch_rad;

		/* Recall: perpendicular vector to the unit vector along the switch (xv, yv) *
		 * is (yv, -xv).                                                             */

		poly[1].x = xbaseline + yunit * switch_rad;
		poly[1].y = ybaseline - xunit * switch_rad;
		poly[2].x = xbaseline - yunit * switch_rad;
		poly[2].y = ybaseline + xunit * switch_rad;
		fillpoly(poly, 3);
	}
}

static void draw_rr_pin(int inode, enum color_types color) {

	/* Draws an IPIN or OPIN rr_node.  Note that the pin can appear on more    *
	 * than one side of a clb.  Also note that this routine can change the     *
	 * current color to BLACK.                                                 */

	int ipin, i, j, iside;
	float xcen, ycen;
	char str[BUFSIZE];
	t_type_ptr type;

	i = rr_node[inode].xlow;
	j = rr_node[inode].ylow;
	ipin = rr_node[inode].ptc_num;
	type = grid[i][j].type;
	int width_offset = grid[i][j].width_offset;
	int height_offset = grid[i][j].height_offset;

	setcolor(color);

	/* TODO: This is where we can hide fringe physical pins and also identify globals (hide, color, show) */
	for (iside = 0; iside < 4; iside++) {
		if (type->pinloc[grid[i][j].width_offset][grid[i][j].height_offset][iside][ipin]) { /* Pin exists on this side. */
			draw_get_rr_pin_coords(inode, iside, width_offset, height_offset, &xcen, &ycen);
			fillrect(xcen - draw_coords->pin_size, ycen - draw_coords->pin_size, 
					 xcen + draw_coords->pin_size, ycen + draw_coords->pin_size);
			sprintf(str, "%d", ipin);
			setcolor(BLACK);
			drawtext(xcen, ycen, str, 2 * draw_coords->pin_size);
			setcolor(color);
		}
	}
}

static void draw_get_rr_pin_coords(int inode, int iside, 
		int width_offset, int height_offset, 
		float *xcen, float *ycen) {

	/* Returns the coordinates at which the center of this pin should be drawn. *
	 * inode gives the node number, and iside gives the side of the clb or pad  *
	 * the physical pin is on.                                                  */

	int i, j, k, ipin, pins_per_sub_tile;
	float offset, xc, yc, step;
	t_type_ptr type;

	i = rr_node[inode].xlow + width_offset;
	j = rr_node[inode].ylow + height_offset;

	xc = draw_coords->tile_x[i];
	yc = draw_coords->tile_y[j];

	ipin = rr_node[inode].ptc_num;
	type = grid[i][j].type;
	pins_per_sub_tile = grid[i][j].type->num_pins / grid[i][j].type->capacity;
	k = ipin / pins_per_sub_tile;

	/* Since pins numbers go across all sub_tiles in a block in order
	 * we can treat as a block box for this step */

	/* For each sub_tile we need and extra padding space */
	step = (float) (draw_coords->tile_width) / (float) (type->num_pins + type->capacity);
	offset = (ipin + k + 1) * step;

	switch (iside) {
	case LEFT:
		yc += offset;
		break;

	case RIGHT:
		xc += draw_coords->tile_width;
		yc += offset;
		break;

	case BOTTOM:
		xc += offset;
		break;

	case TOP:
		xc += offset;
		yc += draw_coords->tile_width;
		break;

	default:
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"in draw_get_rr_pin_coords: Unexpected iside %d.\n", iside);
		break;
	}

	*xcen = xc;
	*ycen = yc;
}


static void drawroute(enum e_draw_net_type draw_net_type) {

	/* Draws the nets in the positions fixed by the router.  If draw_net_type is *
	 * ALL_NETS, draw all the nets.  If it is HIGHLIGHTED, draw only the nets    *
	 * that are not coloured black (useful for drawing over the rr_graph).       */

	/* Next free track in each channel segment if routing is GLOBAL */

	static int **chanx_track = NULL; /* [1..nx][0..ny] */
	static int **chany_track = NULL; /* [0..nx][1..ny] */

	unsigned int inet;
	int i, j, inode, prev_node, prev_track, itrack;
	short switch_type;
	struct s_trace *tptr;
	t_rr_type rr_type, prev_type;

	if (draw_state->draw_route_type == GLOBAL) {
		/* Allocate some temporary storage if it's not already available. */
		if (chanx_track == NULL) {
			chanx_track = (int **) alloc_matrix(1, nx, 0, ny, sizeof(int));
		}

		if (chany_track == NULL) {
			chany_track = (int **) alloc_matrix(0, nx, 1, ny, sizeof(int));
		}

		for (i = 1; i <= nx; i++)
			for (j = 0; j <= ny; j++)
				chanx_track[i][j] = (-1);

		for (i = 0; i <= nx; i++)
			for (j = 1; j <= ny; j++)
				chany_track[i][j] = (-1);
	}

	setlinestyle(SOLID);

	/* Now draw each net, one by one.      */

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global) /* Don't draw global nets. */
			continue;

		if (trace_head[inet] == NULL) /* No routing.  Skip.  (Allows me to draw */
			continue; /* partially complete routes).            */

		if (draw_net_type == HIGHLIGHTED && draw_state->net_color[inet] == BLACK)
			continue;

		tptr = trace_head[inet]; /* SOURCE to start */
		inode = tptr->index;
		rr_type = rr_node[inode].type;

		for (;;) {
			prev_node = inode;
			prev_type = rr_type;
			switch_type = tptr->iswitch;
			tptr = tptr->next;
			inode = tptr->index;
			rr_type = rr_node[inode].type;

			if (draw_if_net_highlighted(inet)) {
				/* If a net has been highlighted, highlight the whole net in *
				 * the same color.											 */
				draw_state->draw_rr_node[inode].color = draw_state->net_color[inet];
				draw_state->draw_rr_node[inode].node_highlighted = true;
			}
			else {
				/* If not highlighted, draw the node in default color. */
				draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
			}

			switch (rr_type) {

			case OPIN:
				draw_rr_pin(inode, draw_state->draw_rr_node[inode].color);
				break;

			case IPIN:
				draw_rr_pin(inode, draw_state->draw_rr_node[inode].color);
				if(rr_node[prev_node].type == OPIN) {
					draw_pin_to_pin(prev_node, inode);
				} else {
					prev_track = get_track_num(prev_node, chanx_track, chany_track);
					draw_pin_to_chan_edge(inode, prev_node);
				}
				break;

			case CHANX:
				if (draw_state->draw_route_type == GLOBAL)
					chanx_track[rr_node[inode].xlow][rr_node[inode].ylow]++;

				itrack = get_track_num(inode, chanx_track, chany_track);
				draw_rr_chanx(inode, itrack, draw_state->draw_rr_node[inode].color);

				switch (prev_type) {

				case CHANX:
					prev_track = get_track_num(prev_node, chanx_track,
							chany_track);
					draw_chanx_to_chanx_edge(prev_node, prev_track, inode,
							itrack, switch_type);
					break;

				case CHANY:
					prev_track = get_track_num(prev_node, chanx_track,
							chany_track);
					draw_chanx_to_chany_edge(inode, itrack, prev_node,
							prev_track, FROM_Y_TO_X, switch_type);
					break;

				case OPIN:
					draw_pin_to_chan_edge(prev_node, inode);
					break;

				default:
					vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
							"in drawroute: Unexpected connection from an rr_node of type %d to one of type %d.\n",
							prev_type, rr_type);
				}

				break;

			case CHANY:
				if (draw_state->draw_route_type == GLOBAL)
					chany_track[rr_node[inode].xlow][rr_node[inode].ylow]++;

				itrack = get_track_num(inode, chanx_track, chany_track);
				draw_rr_chany(inode, itrack, draw_state->draw_rr_node[inode].color);

				switch (prev_type) {

				case CHANX:
					prev_track = get_track_num(prev_node, chanx_track,
							chany_track);
					draw_chanx_to_chany_edge(prev_node, prev_track, inode,
							itrack, FROM_X_TO_Y, switch_type);
					break;

				case CHANY:
					prev_track = get_track_num(prev_node, chanx_track,
							chany_track);
					draw_chany_to_chany_edge(prev_node, prev_track, inode,
							itrack, switch_type);
					break;

				case OPIN:
					draw_pin_to_chan_edge(prev_node, inode);

					break;

				default:
					vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
							"in drawroute: Unexpected connection from an rr_node of type %d to one of type %d.\n",
							prev_type, rr_type);
				}

				break;

			default:
				break;

			}

			if (rr_type == SINK) { /* Skip the next segment */
				tptr = tptr->next;
				if (tptr == NULL)
					break;
				inode = tptr->index;
				rr_type = rr_node[inode].type;
			}

		} /* End loop over traceback. */
	} /* End for (each net) */
}


static int get_track_num(int inode, int **chanx_track, int **chany_track) {

	/* Returns the track number of this routing resource node.   */

	int i, j;
	t_rr_type rr_type;

	if (draw_state->draw_route_type == DETAILED)
		return (rr_node[inode].ptc_num);

	/* GLOBAL route stuff below. */

	rr_type = rr_node[inode].type;
	i = rr_node[inode].xlow; /* NB: Global rr graphs must have only unit */
	j = rr_node[inode].ylow; /* length channel segments.                 */

	switch (rr_type) {
	case CHANX:
		return (chanx_track[i][j]);

	case CHANY:
		return (chany_track[i][j]);

	default:
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"in get_track_num: Unexpected node type %d for node %d.\n", rr_type, inode);
		return OPEN;
	}
}


/* This helper function determines whether a net has been highlighted. The highlighting *
 * could be caused by the user clicking on a routing resource, highlight critical path  *
 * toggled, or fan-in/fan-out of a highlighted node.									*/
static bool draw_if_net_highlighted (int inet) {
	bool highlighted = false;

	if (draw_state->net_color[inet] == MAGENTA || draw_state->net_color[inet] == RED 
		|| draw_state->net_color[inet] == BLUE || draw_state->net_color[inet] == DARKGREEN 
		|| draw_state->net_color[inet] == CYAN)
		highlighted = true;

	return highlighted;
}


/* If an rr_node has been clicked on, it will be highlighted in MAGENTA.
 * If so, and toggle nets is selected, highlight the whole net in that colour.
 */
static void highlight_nets(char *message, int hit_node) {
	unsigned int inet;
	struct s_trace *tptr;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		for (tptr = trace_head[inet]; tptr != NULL; tptr = tptr->next) {
			if (draw_state->draw_rr_node[tptr->index].color == MAGENTA) {
				draw_state->net_color[inet] = draw_state->draw_rr_node[tptr->index].color;
				if (tptr->index == hit_node) {
					sprintf(message, "%s  ||  Net: %d (%s)", message, inet,
							g_clbs_nlist.net[inet].name);
				}
			}
			else if (draw_state->draw_rr_node[tptr->index].color == WHITE) {
				// If node is de-selected.
				draw_state->net_color[inet] = BLACK;
				break;
			}
		}
	}
	update_message(message);
}


/* If an rr_node has been clicked on, it will be either highlighted in MAGENTA,
 * or de-highlighted in WHITE. If highlighted, and toggle_rr is selected, highlight 
 * fan_in into the node in blue and fan_out from the node in red. If de-highlighted,
 * de-highlight its fan_in and fan_out.
 */
static void draw_highlight_fan_in_fan_out(int hit_node) {
	int iedge, inode;

	/* Highlight the fanout nodes in red. */
	for (iedge = 0; iedge < rr_node[hit_node].num_edges; iedge++) {
		int fanout_node = rr_node[hit_node].edges[iedge];

		if (draw_state->draw_rr_node[hit_node].color == MAGENTA) {
			// If node is highlighted, highlight its fanout
			draw_state->draw_rr_node[fanout_node].color = RED;
			draw_state->draw_rr_node[fanout_node].node_highlighted = true;
		}
		else if (draw_state->draw_rr_node[hit_node].color == WHITE) {
			// If node is de-highlighted, de-highlight its fanout
			draw_state->draw_rr_node[fanout_node].color = DEFAULT_RR_NODE_COLOR;
			draw_state->draw_rr_node[fanout_node].node_highlighted = false;
		}
	}

	/* Highlight the nodes that can fanin to this node in blue. */
	for (inode = 0; inode < num_rr_nodes; inode++) {
		for (iedge = 0; iedge < rr_node[inode].num_edges; iedge++) {
			int fanout_node = rr_node[inode].edges[iedge];
			if (fanout_node == hit_node) { 
				if (draw_state->draw_rr_node[hit_node].color == MAGENTA) {
					// If hit_node is highlighted, highlight its fanin
					draw_state->draw_rr_node[inode].color = BLUE;  
					draw_state->draw_rr_node[inode].node_highlighted = true;
				}
				else if (draw_state->draw_rr_node[hit_node].color == WHITE) {
					// If hit_node is de-highlighted, de-highlight its fanin
					draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
					draw_state->draw_rr_node[inode].node_highlighted = false;
				}
			}
		}
	}
}


/* This is a helper function for highlight_rr_nodes(). It determines whether 
 * a routing resource has been clicked on by computing a bounding box for that 
 *  and checking if the mouse click hit inside its bounding box.
 */
static int draw_check_rr_node_hit (float click_x, float click_y) {
	int inode;
	int hit_node = OPEN;
	t_draw_bbox bound_box;

	for (inode = 0; inode < num_rr_nodes; inode++) {
		switch (rr_node[inode].type) {
			case IPIN:
			case OPIN:		
			{
				int i = rr_node[inode].xlow;
				int j = rr_node[inode].ylow;
				t_type_ptr type = grid[i][j].type;
				int width_offset = grid[i][j].width_offset;
				int height_offset = grid[i][j].height_offset;
				int ipin = rr_node[inode].ptc_num;
				float xcen, ycen;
				
				int iside;
				for (iside = 0; iside < 4; iside++) {
					// If pin exists on this side of the block, then get pin coordinates
					if (type->pinloc[width_offset][height_offset][iside][ipin]) {
						draw_get_rr_pin_coords(inode, iside, width_offset, height_offset, 
											   &xcen, &ycen);

						// Now check if we clicked on this pin
						if (click_x >= xcen - draw_coords->pin_size &&
							click_x <= xcen + draw_coords->pin_size &&
							click_y >= ycen - draw_coords->pin_size && 
							click_y <= ycen + draw_coords->pin_size) {
							hit_node = inode;
							return hit_node;
						}
					}
				}
				break;
			}
			case CHANX:
			case CHANY:
			{
				bound_box = draw_get_rr_chan_bbox(inode);

				// Check if we clicked on this wire, with 30%
				// tolerance outside its boundary
				const float tolerance = 0.3;
				if (click_x >= bound_box.xleft - tolerance &&
					click_x <= bound_box.xright + tolerance &&
					click_y >= bound_box.ybottom - tolerance && 
					click_y <= bound_box.ytop + tolerance) {
					hit_node = inode;
					return hit_node;
				}
				break;
			}
			default:
				break;
		}
	}
	return hit_node;
}


/* This routine is called when the routing resource graph is shown, and someone 
 * clicks outside a block. That click might represent a click on a wire -- we call
 * this routine to determine which wire (if any) was clicked on.  If a wire was
 * clicked upon, we highlight it in Magenta, and its fanout in red. 
 */
static void highlight_rr_nodes(float x, float y) {

	int hit_node = OPEN;  // i.e. -1, no node selected.
	char message[250] = "";

	if (draw_state->draw_rr_toggle == DRAW_NO_RR && !draw_state->show_nets) {
		update_message(draw_state->default_message);
		drawscreen();
		return;
	}

	// Check which rr_node (if any) was clicked on.
	hit_node = draw_check_rr_node_hit (x, y);

	if (hit_node != OPEN) {
		int xlow = rr_node[hit_node].xlow;
		int xhigh = rr_node[hit_node].xhigh;
		int ylow = rr_node[hit_node].ylow;
		int yhigh = rr_node[hit_node].yhigh;
		int ptc_num = rr_node[hit_node].ptc_num;

		if (draw_state->draw_rr_node[hit_node].color != MAGENTA) {
			/* If the node hasn't been clicked on before, highlight it
			 * in magenta.
			 */
			draw_state->draw_rr_node[hit_node].color = MAGENTA;
			draw_state->draw_rr_node[hit_node].node_highlighted = true;

			sprintf(message, "Selected node #%d: %s (%d,%d) -> (%d,%d) track: %d, %d edges, occ: %d, capacity: %d",
				    hit_node, rr_node[hit_node].rr_get_type_string(),
				    xlow, ylow, xhigh, yhigh, ptc_num, rr_node[hit_node].num_edges, 
				    rr_node[hit_node].occ, rr_node[hit_node].capacity);

		}
		else {
			/* Using white color to represent de-highlighting (or 
			 * de-selecting) of node.
			 */
			draw_state->draw_rr_node[hit_node].color = WHITE;
			draw_state->draw_rr_node[hit_node].node_highlighted = false;
		}

#ifdef DEBUG
		print_rr_node(stdout, rr_node, hit_node);
#endif
		if (draw_state->draw_rr_toggle != DRAW_NO_RR) 
			// If rr_graph is shown, highlight the fan-in/fan-outs for
			// this node.
			draw_highlight_fan_in_fan_out(hit_node);
   }

	if (hit_node == OPEN) {
		update_message(draw_state->default_message);
		drawscreen();
		return;
	}

	if (draw_state->show_nets) {
		highlight_nets(message, hit_node);
	} else
		update_message(message);

	drawscreen();
}


static void highlight_blocks(float x, float y, t_event_buttonPressed button_info) {

	/* This routine is called when the user clicks in the graphics area. *
	 * It determines if a clb was clicked on.  If one was, it is         *
	 * highlighted in green, it's fanin nets and clbs are highlighted in *
	 * blue and it's fanout is highlighted in red.  If no clb was        *
	 * clicked on (user clicked on white space) any old highlighting is  *
	 * removed.  Note that even though global nets are not drawn, their  *
	 * fanins and fanouts are highlighted when you click on a block      *
	 * attached to them.                                                 */

	int i, j, k, hit, bnum;
	float io_step;
	t_type_ptr type;
	char msg[BUFSIZE];

	/* Control + mouse click to select multiple nets. */
	if (!button_info.ctrl_pressed)
		deselect_all();

	hit = i = j = k = 0;

	for (i = 0; i <= (nx + 1) && !hit; i++) {
		if (x <= draw_coords->tile_x[i] + draw_coords->tile_width) {
			if (x >= draw_coords->tile_x[i]) {
				for (j = 0; j <= (ny + 1) && !hit; j++) {
					if (grid[i][j].width_offset != 0 || grid[i][j].height_offset != 0)
						continue;
					type = grid[i][j].type;
					if (y <= draw_coords->tile_y[j + type->height - 1] 
						+ draw_coords->tile_width) 
					{
						if (y >= draw_coords->tile_y[j])
							hit = 1;
					}
				}

			}
		}
	}
	i--;
	j--;

	if (!hit) {
		highlight_rr_nodes(x, y);
		/* update_message(draw_state->default_message);
		 drawscreen(); */
		return;
	}
	type = grid[i][j].type;
	hit = 0;

	if (EMPTY_TYPE == type) {
		update_message(draw_state->default_message);
		drawscreen();
		return;
	}

	/* The user selected the clb at location (i,j). */
	io_step = draw_coords->tile_width / type->capacity;

	if ((i < 1) || (i > nx)) /* Vertical columns of IOs */
		k = (int) ((y - draw_coords->tile_y[j]) / io_step);
	else
		k = (int) ((x - draw_coords->tile_x[i]) / io_step);

	assert(k < type->capacity);
	if (grid[i][j].blocks[k] == EMPTY) {
		update_message(draw_state->default_message);
		drawscreen();
		return;
	}
	bnum = grid[i][j].blocks[k];

	/* Highlight block and fan-in/fan-outs. */
	draw_highlight_blocks_color(type, bnum);

	sprintf(msg, "Block %d (%s) at (%d, %d) selected.", bnum, block[bnum].name,
			i, j);
	update_message(msg);
	drawscreen(); /* Need to erase screen. */
}


static void draw_highlight_blocks_color(t_type_ptr type, int bnum) {
	int k, netnum, fanblk, iclass;
	unsigned ipin;

	for (k = 0; k < type->num_pins; k++) { /* Each pin on a CLB */
		netnum = block[bnum].nets[k];

		if (netnum == OPEN)
			continue;

		iclass = type->pin_class[k];

		if (type->class_inf[iclass].type == DRIVER) { /* Fanout */
			if (draw_state->block_color[bnum] == GREEN) {
				/* If block already highlighted, de-highlight the fanout. */
				draw_state->net_color[netnum] = BLACK;
				for (ipin = 1; ipin < g_clbs_nlist.net[netnum].pins.size(); ipin++) {
					fanblk = g_clbs_nlist.net[netnum].pins[ipin].block;
					draw_reset_blk_color(fanblk);
				}
			}
			else {
				/* Highlight the fanout */
				draw_state->net_color[netnum] = RED;
				for (ipin = 1; ipin < g_clbs_nlist.net[netnum].pins.size(); ipin++) {
					fanblk = g_clbs_nlist.net[netnum].pins[ipin].block;
					draw_state->block_color[fanblk] = RED;
				}
			}
		} 
		else { /* This net is fanin to the block. */
			if (draw_state->block_color[bnum] == GREEN) {
				/* If block already highlighted, de-highlight the fanin. */
				draw_state->net_color[netnum] = BLACK;
				fanblk = g_clbs_nlist.net[netnum].pins[0].block; /* DRIVER to net */
				draw_reset_blk_color(fanblk);
			}
			else {
				/* Highlight the fanin */
				draw_state->net_color[netnum] = BLUE;
				fanblk = g_clbs_nlist.net[netnum].pins[0].block; /* DRIVER to net */
				draw_state->block_color[fanblk] = BLUE;
			}
		}
	}

	if (draw_state->block_color[bnum] == GREEN) { 
		/* If block already highlighted, de-highlight the selected block. */
		draw_reset_blk_color(bnum);
	}
	else { 
		/* Highlight the selected block. */
		draw_state->block_color[bnum] = GREEN; 
	}
}


static void deselect_all(void) {
	/* Sets the color of all clbs, nets and rr_nodes to the default.  */

	int i;

	/* Create some colour highlighting */
	for (i = 0; i < num_blocks; i++) {
		draw_reset_blk_color(i);
	}

	for (i = 0; i < (int) g_clbs_nlist.net.size(); i++)
		draw_state->net_color[i] = BLACK;

	for (i = 0; i < num_rr_nodes; i++) {
		draw_state->draw_rr_node[i].color = DEFAULT_RR_NODE_COLOR;
		draw_state->draw_rr_node[i].node_highlighted = false;
	}
}


static void draw_reset_blk_color(int i) {
	if (block[i].type->index < 3) {
			draw_state->block_color[i] = LIGHTGREY;
	} else if (block[i].type->index < 3 + MAX_BLOCK_COLOURS) {
			draw_state->block_color[i] = (enum color_types) (BISQUE + MAX_BLOCK_COLOURS 
															+ block[i].type->index - 3);
	} else {
			draw_state->block_color[i] = (enum color_types) (BISQUE + 2 * MAX_BLOCK_COLOURS 
															- 1);
	}
}


static void draw_triangle_along_line(float xend, float yend, float x1, float x2,
		float y1, float y2) {
	float switch_rad = 0.15;
	float xdelta, ydelta;
	float magnitude;
	float xunit, yunit;
	float xbaseline, ybaseline;
	t_point poly[3];

	xdelta = x2 - x1;
	ydelta = y2 - y1;
	magnitude = sqrt(xdelta * xdelta + ydelta * ydelta);
	xunit = xdelta / magnitude;
	yunit = ydelta / magnitude;

	poly[0].x = xend + xunit * switch_rad;
	poly[0].y = yend + yunit * switch_rad;
	xbaseline = xend - xunit * switch_rad;
	ybaseline = yend - yunit * switch_rad;
	poly[1].x = xbaseline + yunit * switch_rad;
	poly[1].y = ybaseline - xunit * switch_rad;
	poly[2].x = xbaseline - yunit * switch_rad;
	poly[2].y = ybaseline + xunit * switch_rad;

	fillpoly(poly, 3);
}

static void draw_pin_to_chan_edge(int pin_node, int chan_node) {

	/* This routine draws an edge from the pin_node to the chan_node (CHANX or   *
	 * CHANY).  The connection is made to the nearest end of the track instead   *
	 * of perpundicular to the track to symbolize a single-drive connection.     *
	 * If mark_conn is TRUE, draw a box where the pin connects to the track      *
	 * (useful for drawing  the rr graph)                                        */

	/* TODO: Fix this for global routing, currently for detailed only */

	t_rr_type chan_type;
	int grid_x, grid_y, pin_num, chan_xlow, chan_ylow;
	float x1, x2, y1, y2;
	int start, end, i;
	t_draw_bbox chan_bbox;
	float xend, yend;
	float draw_pin_off;
	enum e_direction direction;
	enum e_side iside;
	t_type_ptr type;

	direction = rr_node[chan_node].direction;
	grid_x = rr_node[pin_node].xlow;
	grid_y = rr_node[pin_node].ylow;
	pin_num = rr_node[pin_node].ptc_num;
	chan_type = rr_node[chan_node].type;
	type = grid[grid_x][grid_y].type;

	/* large block begins at primary tile (offset == 0) */
	int width_offset = grid[grid_x][grid_y].width_offset;
	int height_offset = grid[grid_x][grid_y].height_offset;
	grid_x = grid_x - width_offset;
	grid_y = grid_y - height_offset;

	int width = grid[grid_x][grid_y].type->width;
	int height = grid[grid_x][grid_y].type->height;
	chan_ylow = rr_node[chan_node].ylow;
	chan_xlow = rr_node[chan_node].xlow;

	start = -1;
	end = -1;

	switch (chan_type) {

	case CHANX:
		start = rr_node[chan_node].xlow;
		end = rr_node[chan_node].xhigh;
		if (is_opin(pin_num, type)) {
			if (direction == INC_DIRECTION) {
				end = rr_node[chan_node].xlow;
			} else if (direction == DEC_DIRECTION) {
				start = rr_node[chan_node].xhigh;
			}
		}

		start = max(start, grid_x);
		end = min(end, grid_x); /* Width is 1 always */
		assert(end >= start);
		/* Make sure we are nearby */

		if ((grid_y + height - 1) == chan_ylow) {
			iside = TOP;
			width_offset = width - 1;
			height_offset = height - 1;
			draw_pin_off = draw_coords->pin_size;
		} else {
			assert((grid_y - 1) == chan_ylow);

			iside = BOTTOM;
			width_offset = 0;
			height_offset = 0;
			draw_pin_off = -draw_coords->pin_size;
		}
		assert(grid[grid_x][grid_y].type->pinloc[width_offset][height_offset][iside][pin_num]);

		draw_get_rr_pin_coords(pin_node, iside, width_offset, height_offset, &x1, &y1);
		chan_bbox = draw_get_rr_chan_bbox(chan_node);

		y1 += draw_pin_off;
		y2 = chan_bbox.ybottom;
		x2 = x1;
		if (is_opin(pin_num, type)) {
			if (direction == INC_DIRECTION) {
				x2 = chan_bbox.xleft;
			} else if (direction == DEC_DIRECTION) {
				x2 = chan_bbox.xright;
			}
		}
		break;

	case CHANY:
		start = rr_node[chan_node].ylow;
		end = rr_node[chan_node].yhigh;
		if (is_opin(pin_num, type)) {
			if (direction == INC_DIRECTION) {
				end = rr_node[chan_node].ylow;
			} else if (direction == DEC_DIRECTION) {
				start = rr_node[chan_node].yhigh;
			}
		}

		start = max(start, grid_y);
		end = min(end, (grid_y + height - 1)); /* Width is 1 always */
		assert(end >= start);
		/* Make sure we are nearby */

		if ((grid_x) == chan_xlow) {
			iside = RIGHT;
			draw_pin_off = draw_coords->pin_size;
		} else {
			assert((grid_x - 1) == chan_xlow);
			iside = LEFT;
			draw_pin_off = -draw_coords->pin_size;
		}
		for (i = start; i <= end; i++) {
			height_offset = i - grid_y;
			assert(height_offset >= 0 && height_offset < type->height);
			/* Once we find the location, break out, this will leave ioff pointing
			 * to the correct offset.  If an offset is not found, the assertion after
			 * this will fail.  With the correct routing graph, the assertion will not
			 * be triggered.  This also takes care of connecting a wire once to multiple
			 * physical pins on the same side. */
			if (grid[grid_x][grid_y].type->pinloc[width_offset][height_offset][iside][pin_num]) {
				break;
			}
		}
		assert(grid[grid_x][grid_y].type->pinloc[width_offset][height_offset][iside][pin_num]);

		draw_get_rr_pin_coords(pin_node, iside, width_offset, height_offset, &x1, &y1);
		chan_bbox = draw_get_rr_chan_bbox(chan_node);
		
		x1 += draw_pin_off;
		x2 = chan_bbox.xleft;
		y2 = y1;
		if (is_opin(pin_num, type)) {
			if (direction == INC_DIRECTION) {
				y2 = chan_bbox.ybottom;
			} else if (direction == DEC_DIRECTION) {
				y2 = chan_bbox.ytop;
			}
		}
		break;

	default:
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"in draw_pin_to_chan_edge: Invalid channel node %d.\n", chan_node);
		x1 = x2 = y1 = y2 = OPEN; //Prevents compiler error of variable uninitialized.  
	}

	drawline(x1, y1, x2, y2);
	if (direction == BI_DIRECTION || !is_opin(pin_num, type)) {
		draw_x(x2, y2, 0.7 * draw_coords->pin_size);
	} else {
		xend = x2 + (x1 - x2) / 10.;
		yend = y2 + (y1 - y2) / 10.;
		draw_triangle_along_line(xend, yend, x1, x2, y1, y2);
	}
}


static void draw_pin_to_pin(int opin_node, int ipin_node) {
	
	/* This routine draws an edge from the opin rr node to the ipin rr node */
	int opin_grid_x, opin_grid_y, opin_pin_num;
	int ipin_grid_x, ipin_grid_y, ipin_pin_num;
	int width_offset, height_offset;
	boolean found;
	float x1, x2, y1, y2;
	float xend, yend;
	enum e_side iside, pin_side;
	t_type_ptr type;

	assert(rr_node[opin_node].type == OPIN);
	assert(rr_node[ipin_node].type == IPIN);
	iside = (enum e_side)0;
	x1 = y1 = x2 = y2 = 0;
	width_offset = 0;
	height_offset = 0;
	pin_side = TOP;

	/* get opin coordinate */
	opin_grid_x = rr_node[opin_node].xlow;
	opin_grid_y = rr_node[opin_node].ylow;
	opin_grid_x = opin_grid_x - grid[opin_grid_x][opin_grid_y].width_offset;
	opin_grid_y = opin_grid_y - grid[opin_grid_x][opin_grid_y].height_offset;

	opin_pin_num = rr_node[opin_node].ptc_num;
	type = grid[opin_grid_x][opin_grid_y].type;
	
	found = FALSE;
	for (int width = 0; width < type->width && !found; ++width) {
		for (int height = 0; height < type->height && !found; ++height) {
			for (iside = (enum e_side)0; iside < 4 && !found; iside = (enum e_side)(iside + 1)) {
				/* Find first location of pin */
				if (1 == type->pinloc[width][height][iside][opin_pin_num]) {
					width_offset = width;
					height_offset = height;
					pin_side = iside;
					found = TRUE;
				}
			}
		}
	}
	assert(found);
	draw_get_rr_pin_coords(opin_node, pin_side, width_offset, height_offset, &x1, &y1);

	/* get ipin coordinate */
	ipin_grid_x = rr_node[ipin_node].xlow;
	ipin_grid_y = rr_node[ipin_node].ylow;
	ipin_grid_x = ipin_grid_x - grid[ipin_grid_x][ipin_grid_y].width_offset;
	ipin_grid_y = ipin_grid_y - grid[ipin_grid_x][ipin_grid_y].height_offset;

	ipin_pin_num = rr_node[ipin_node].ptc_num;
	type = grid[ipin_grid_x][ipin_grid_y].type;
	
	found = FALSE;
	for (int width = 0; width < type->width && !found; ++width) {
		for (int height = 0; height < type->height && !found; ++height) {
			for (iside = (enum e_side)0; iside < 4 && !found; iside = (enum e_side)(iside + 1)) {
				/* Find first location of pin */
				if (1 == type->pinloc[width][height][iside][ipin_pin_num]) {
					width_offset = width;
					height_offset = height;
					pin_side = iside;
					found = TRUE;
				}
			}
		}
	}
	assert(found);
	draw_get_rr_pin_coords(ipin_node, pin_side, width_offset, height_offset, &x2, &y2);

	drawline(x1, y1, x2, y2);	
	xend = x2 + (x1 - x2) / 10.;
	yend = y2 + (y1 - y2) / 10.;
	draw_triangle_along_line(xend, yend, x1, x2, y1, y2);
}

