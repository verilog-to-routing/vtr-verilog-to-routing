#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <iostream>
#include <string>
#include "easygl_constants.h"
#include <cstdint>

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
* More updates and c++ integration - Matthew J.P. Walker (matthewjp.walker@mail.utoronto.ca)
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


/**
 * A point datatype.
 */
struct t_point {
	float x; 
	float y;

	void set(float x, float y);
	void set(const t_point& src);

	/**
	 * beahves like a 2 argument plusequals.
	 */
	void offset(float x, float y);
	
	/**
	 * These add the given point to this point in a 
	 * componentwise fashion, ie x = x + rhs.x
	 *
	 * Naturally, {+,-} don't modify and {+,-}= do.
	 */
	t_point operator+ (const t_point& rhs) const;
	t_point operator- (const t_point& rhs) const;
	t_point operator* (float rhs) const;
	t_point& operator+= (const t_point& rhs);
	t_point& operator-= (const t_point& rhs);
	t_point& operator*= (float rhs);

	/**
	 * Assign that point to this one - copy the components
	 */
	t_point& operator= (const t_point& src);

	t_point();
	t_point(const t_point& src);
	t_point(float x, float y);

};

t_point operator*(float lhs, const t_point& rhs);

/**
 * represents a rectangle, used as a bounding box.
 */
struct t_bound_box {

	/**
	 * These return their respective edge/point's location
	 */
	const float& left() const;
	const float& right() const;
	const float& bottom() const;
	const float& top() const;
	float& left();
	float& right();
	float& bottom();
	float& top();	

	const t_point& bottom_left() const;
	const t_point& top_right() const;
	t_point& bottom_left();
	t_point& top_right();

	/**
	 * calculate and return the center
	 */
	float get_xcenter() const;
	float get_ycenter() const;
	t_point get_center() const;

	/**
	 * calculate and return the width/height
	 */
	float get_width() const;
	float get_height() const;

	/**
	 * These behave like the plusequal operator
	 * They add their x and y values to all corners
	 */
	void offset(const t_point& make_relative_to);
	void offset(float by_x, float by_y);

	bool intersects(const t_point& test_pt) const;
	bool intersects(float x, float y) const;

	float area() const;

	/**
	 * These add the given point to this bbox - they
	 * offset each corner by this point. Usful for calculating
	 * the location of a box in a higher scope, or for moving
	 * it around as part of a calculation
	 *
	 * Naturally, the {+,-} don't modify and the {+,-}= do.
	 */
	t_bound_box operator+ (const t_point& rhs) const;
	t_bound_box operator- (const t_point& rhs) const;
	t_bound_box& operator+= (const t_point& rhs);
	t_bound_box& operator-= (const t_point& rhs);

	/**
	 * Assign that box to this one - copy it's left, right, bottom, and top.
	 */
	t_bound_box& operator= (const t_bound_box& src);

	t_bound_box();
	t_bound_box(const t_bound_box& src);
	t_bound_box(float left, float bottom, float right, float top);
	t_bound_box(const t_point& bottomleft, const t_point& topright);
	t_bound_box(const t_point& bottomleft, float width, float height);
private:
	t_point bottomleft;
	t_point topright;
};

struct t_color {
	uint_fast8_t red;
	uint_fast8_t green;
	uint_fast8_t blue;

	t_color(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue);
	t_color(const t_color& src);
	t_color();
	bool operator== (const t_color& rhs) const;
	bool operator!= (const t_color& rhs) const;

	unsigned long as_rgb_int() const;

	// (temporary?)
	t_color(color_types src);
	color_types operator=(color_types color_enum);
	bool operator== (color_types rhs) const;
	bool operator!= (color_types rhs) const;
	// bool operator> (color_types rhs) const;
	// bool operator< (color_types rhs) const;
};

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

/**
 * Set the current draw colour to the supplied colour index from color_types
 * or the specified rgb colour or triplet
 */
void setcolor (int cindex);
void setcolor (const t_color& new_color);
void setcolor (uint_fast8_t r, uint_fast8_t g, uint_fast8_t b);

/* Set the color with a string instead of an enumerated constant */
void setcolor_by_name (std::string cname);

/* Get the current color */
t_color getcolor(void);

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

/*
 * Set the rotation of text to be drawn. I recommend setting rotation
 * back to zero once you are done drawing all rotated text, as most
 * text will not be rotated, and setting rotation there may have been omitted.
 */
void settextrotation (float degrees);
float gettextrotation();

/*
 * Set both the point size and rotation of the text in one call.
 * This should be more effecient then calling setfontsize() and settextrotation()
 * sepearatly, if that makes sense for your program.
 */
void settextattrs (int pointsize, float degrees);

/* Draws a line from (x1, y1) to (x2, y2) in world coordinates */
void drawline (const t_point& p1, const t_point& p2);
void drawline (float x1, float y1, float x2, float y2);

/* Draws a rectangle from (x1, y1) to (x2, y2) in world coordinates, using
 * the current line style, colour and width.
 */
void drawrect (const t_bound_box& rect);
void drawrect (const t_point& bottomleft, const t_point& upperright);
void drawrect (float x1, float y1, float x2, float y2);

/* Draws a filled rectangle with the specified corners, in world coordinates. */
void fillrect (const t_bound_box& rect);
void fillrect (const t_point& bottomleft, const t_point& upperright);
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
void fillarc (const t_point& center, float rad, float startang, float angextent);
void fillarc (float xcen, float ycen, float rad, float startang,
			  float angextent);
void drawellipticarc (
	const t_point& center, float radx, float rady, float startang, float angextent);
void drawellipticarc (
	float xc, float yc, float radx, float rady, float startang, float angextent);

void fillellipticarc (t_point center, float radx, float rady, float startang, float angextent);
void fillellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent);

/* 
 * These functions all draw text within some sort of bounding box.
 * The text is drawn centred around the point (xc,yc), text_center, or
 * in the case of drawtext_in, the centre of the bbox parameter. 
 *
 * boundx and boundy specify the width and height bound, respectively, wheras
 * bounds and bbox specifiy a box in which the text must fit inside completely.
 *
 * If you would like to have these functions ignore a particular bound,
 * specify a huge value. I recommend FLT_MAX or std::numeric_limits<float>::max()
 * 
 * If text won't fit in bounds specified, the text isn't drawn.
 * Useful for avoiding text going everywhere at high zoom levels.
 *
 * tolerance, effectively, makes the given bounding box bigger, on
 * all sides by that amount.
 */
void drawtext_in (const t_bound_box& bbox, const char* text);
void drawtext_in (const t_bound_box& bbox, const char* text, float tolerance);
void drawtext (const t_point& text_center, const char* text, const t_bound_box& bounds);
void drawtext (
	const t_point& text_center, const char* text, const t_bound_box& bounds, float tolerance);
void drawtext (const t_point& text_center, const char* text, float boundx, float boundy);
void drawtext (float xc, float yc, const char* text, float boundx, float boundy);

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

/************************************
 * Level Of Detail Functions
 *
 * These functions may be convenient for deciding to not draw
 * small details, unless they user is zoomed in past a certain level.
 ************************************/

/**
 * Returns a rectangle with the bounds of the drawn world.
 */
t_bound_box get_visible_world();

/**
 * returns true iff the _world_ area of the screen is
 * below a area_threshold.
 */
inline bool LOD_area_test(float area_threshold) {
	return get_visible_world().area() < area_threshold;
}

/**
 * returns true iff the smallest dimension of the visible
 * world is less than dim_threshold.
 */
inline bool LOD_min_dim_test(float dim_threshold) {
	t_bound_box vis_world = get_visible_world();
	return
		(
			(vis_world.get_height() < vis_world.get_width()) ?
			vis_world.get_height() : vis_world.get_width()
		) < dim_threshold;
}

/**
 * screen_area_threshold is in (screen pixels)^2. I suggest something around 3
 *
 * Iff the _screen_ area of the rectangle, specified throught the various means,
 * is less than screen_area_threshold then these functions return false.
 */
bool LOD_screen_area_test(t_bound_box test, float screen_area_threshold);

inline bool LOD_screen_area_test(float width, float height, float screen_area_threshold) {
	return LOD_screen_area_test(t_bound_box(0,0,width,height),screen_area_threshold);
}

inline bool LOD_screen_area_test_square(float width, float screen_area_threshold) {
	return LOD_screen_area_test(width,width,screen_area_threshold);
}

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
