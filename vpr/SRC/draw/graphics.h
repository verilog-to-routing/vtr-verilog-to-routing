#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <iostream>
#include <string>
#include "easygl_constants.h"
using namespace std;

// Set X11 by default, if neither NO_GRAPHICS nor WIN32 are defined
#ifndef NO_GRAPHICS
#ifndef WIN32
	#ifndef X11
	#define X11
	#endif
#endif // !WIN32
#endif // !NO_GRAPHICS

/* Graphics.h
* Originally written by Vaughn Betz (vaughn@eecg.utoronto.ca)
* Win32 port by Paul Leventis (leventi@eecg.utoronto.ca)
* Enhanced version by William Chow (chow@eecg.utoronto.ca)
* Minor updates by Guy Lemieux (lemieux@ece.ubc.ca)
* More updates by Vaughn Betz to make win32 cleaner and more robust.
* More updates and code cleanup by Long Yu Wang (longyu.wang@mail.utoronto.ca)
*/


/******* Constants and enums ******************************************/

/* Data structure below is for debugging.  Lets you get a bunch
 * of info about the low-level graphics state.
 * xmult, ymult: world to pixel coordinate multiplier for screen
 * ps_xmult, ps_ymult: world to pixel coordinate multiplier for postscript
 * xleft, xright, ytop, yleft:  current world coordinates of user-graphics display corners
 * top_width, top_height: size (in pixels) of top-level window
 */
typedef struct {
	float xmult, ymult;
	float ps_xmult, ps_ymult;
	float xleft, xright, ytop, ybot;
	int top_width, top_height;
} t_report;

/************** ESSENTIAL FUNCTIONS ******************/

/* This is the main routine for the graphics.  When event_loop is
* called, it will continue executing until the Proceed button is
* pressed.  
* Whenever the graphics need to be redrawn, drawscreen will be called;
* you must pass in a function pointer to a routine you write that can
* draw the picture you want.
* You can also pass in event handlers for user input if you wish.
* act_on_mouse_button() will be called whenever the user left-clicks
* in the graphics area.
* act_on_keypress() and act_on_mousemove() will be called whenever a 
* keyboard key is pressed or the mouse is moved, respectively, in the 
* graphics area. You can turn keypress input and mouse_move input
* on or off using the set_mouse_move_input () and set_keypress_input ()
* functions (default for both: off).
*/
void event_loop (void (*act_on_mousebutton) (float x, float y, t_event_buttonPressed button_info),
			void (*act_on_mousemove) (float x, float y),
			void (*act_on_keypress) (char key_pressed),
			void (*drawscreen) (void));  

/* Opens up the graphics; the window will have window_name in its
 * title bar and the specified background colour.
 * Known bug:  can't re-open graphics after closing them.
 */
void init_graphics (const char *window_name, int cindex_background);

/* Sets world coordinates of the graphics window so that the 
 * upper-left corner has virtual coordinate (xl, yt) and the
 * bottom-right corner has virtual coordinate (xr, yb). 
 * Call this function before you call event_loop. Do not call it 
 * in your drawscreen () callback function, since it will undo any
 * panning or zooming the user has done.
 */
void init_world (float xl, float yt, float xr, float yb); 

/* Closes the graphics */
void close_graphics (void);

/* Changes the status bar message to msg. */
void update_message (const char *msg);

/* Creates a button on the menu bar below the button with text
 * prev_button_text.  The button will have text button_text,
 * and when clicked will call function button_func.  
 * button_func is a function that accepts a void function as
 * an argument; this argument is set to the drawscreen routine
 * as passed into the event loop. 
 */
void create_button (const char *prev_button_text , const char *button_text, 
			void (*button_func) (void (*drawscreen) (void))); 

/* Destroys the button with the given text; i.e. removes it from
 * the display.
 */
void destroy_button (const char *button_text);

/*************** PostScript Routines *****************/

/* Opens file for postscript commands and initializes it.  All subsequent  
 * drawing commands go to this file until close_postscript is called. 
 * You can generate postscript output by explicitly calling
 * this routine, and then calling drawscreen. More commonly you'll 
 * just click on the "PostScript" button though, and that button 
 * calls this routine and drawscreen to generate a postscript file
 * that exactly matches the graphics area display on the screen.
 */
int init_postscript (const char *fname);   /* Returns 1 if successful */

/* Closes file and directs output to screen again.       */
void close_postscript (void);      


/*************** DRAWING ROUTINES ******************/

/* Clears the screen. Should normally be the first call in your 
 * screen redrawing function.
 */
void clearscreen (void);

/* The following routines draw to SCREEN if disp_type = SCREEN 
 * and to a PostScript file if disp_type = POSTSCRIPT         
 */

/* Set the current draw colour to the supplied colour index from color_types */
void setcolor (int cindex);

/* Set the color with a string instead of an enumerated constant */
void setcolor_by_name (string cname);

/* Get the current color */
int getcolor(void);

/* Sets the line style to the specified line_style */
void setlinestyle (int linestyle); 

/* Sets the line width in pixels (for screen output) or points (1/72 of an inch)
 * for PostScript output. A value of 0 means thinnest possible line.
 */
void setlinewidth (int linewidth);

/* Sets the font size, in points. 72 points is 1 inch high. I allow
 * fonts from 1 to 24 points in size; change MAX_FONT_SIZE if you want
 * bigger fonts still.
 */
void setfontsize (int pointsize);

/* Draws a line from (x1, y1) to (x2, y2) in world coordinates */
void drawline (float x1, float y1, float x2, float y2);

/* Draws a rectangle from (x1, y1) to (x2, y2) in world coordinates, using
 * the current line style, colour and width.
 */
void drawrect (float x1, float y1, float x2, float y2);

/* Draws a filled rectangle with the specified corners, in world coordinates. */
void fillrect (float x1, float y1, float x2, float y2);

/* Draws a filled polygon */
void fillpoly (t_point *points, int npoints); 

/* Draw or fill a circular arc or elliptical arc.  Angles in degrees.  
 * startang is measured from positive x-axis of Window.  
 * A positive angextent means a counterclockwise arc; a negative 
 * angextent means clockwise.
 */
void drawarc (float xcen, float ycen, float rad, float startang,
			  float angextent); 
void fillarc (float xcen, float ycen, float rad, float startang,
			  float angextent);
void drawellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent);
void fillellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent);

/* boundx specifies horizontal bounding box.  If text won't fit in    
 * the space specified by boundx (world coordinates) the text isn't drawn.
 * That avoids text going everywhere for high zoom levels.
 * If you always want the text to display (even if it overwrites lots of
 * stuff at high zoom levels), just specify a huge boundx.
 */
void drawtext (float xc, float yc, const char *text, float boundx);

/* Control what buttons are active (default:  all enabled) and
 * whether mouse movements and keypresses are sent to callback
 * functions (default:  disabled).
 */
void set_mouse_move_input (bool turn_on);
void set_keypress_input (bool turn_on);
void enable_or_disable_button (int ibutton, bool enabled);

/*************** ADVANCED FUNCTIONS *****************/

/* Normal users shouldn't have to use draw_message.  Should only be 
 * useful if using non-interactive graphics and you want to redraw  
 * yourself because of an expose.                                    i
 */
void draw_message (void);     

/* Empties event queue. Can be useful with non-interactive graphics to make
 * sure things display.
 */
void flushinput (void);

/* DRAW_NORMAL is the default mode (overwrite, also known as copy_pen).
 * Can use DRAW_XOR for fast rubber-banding.
 */ 
enum e_draw_mode {DRAW_NORMAL = 0, DRAW_XOR};
void set_draw_mode (enum e_draw_mode draw_mode);

/* Change the text on a button.
 */
void change_button_text(const char *button_text, const char *new_button_text);

/* For debugging only.  Get window size etc. */
void report_structure(t_report*);


/**************** Extra functions available only in WIN32. *******/
#ifdef WIN32
/* VB: TODO: I should make any generally useful functions below work in
 * X11 as well, and probably delete anything else.
 */

/* MW: Draw beizer curve. Currently not used, but saving for possible use
 * in the future.                            
 */
void win32_drawcurve(t_point *points, int npoints);
void win32_fillcurve(t_point *points, int npoints);

#endif // WIN32

#endif // GRAPHICS_H
