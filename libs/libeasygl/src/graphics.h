#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <iostream>
#include <string>
#include <cstdint>
#include <cfloat>  // For FLT_MAX

#include "easygl_constants.h"
#include "graphics_types.h"
#include "Surface.h"

// Set X11 by default, if neither NO_GRAPHICS nor WIN32 are defined
#ifndef NO_GRAPHICS
#ifndef WIN32
#ifndef X11
#define X11
#endif
#endif // !WIN32
#endif // !NO_GRAPHICS

#define PI 3.141592654

/************** ESSENTIAL FUNCTIONS: SET UP AND CLOSE DOWN ******************/

/* This is the main routine for the graphics.  When event_loop is
 * called, it will continue executing until the Proceed button is
 * pressed. It will handle many "events" (mouse clicks, button presses, mouse
 * zooms and pans, etc.) for you, and will pass on other events to callback
 * functions you have passed in for your program to handle.
 * Whenever the graphics need to be redrawn, drawscreen will be called;
 * you must pass in a function pointer to a routine you write that can
 * draw the picture you want.
 * You can also pass in event handlers for user input if you wish.
 * act_on_mouse_button() will be called whenever the user clicks
 * in the graphics area.
 * act_on_keypress() and act_on_mousemove() will be called whenever a
 * keyboard key is pressed or the mouse is moved, respectively, in the
 * graphics area. You can turn keypress input and mouse_move input
 * on or off using the set_mouse_move_input () and set_keypress_input ()
 * functions (default for both: off).
 * act_on_keypress() is passed both the ASCII (char) code of the key_pressed,
 * and an integer "keysym" which is a longer code defined in X11/keysymdef.h
 * that has codes for keys like arrow keys that have no ASCII equivalent.
 * act_on_mousebutton() is passed the coordinates (in user coordinates) of
 * the mouse when the button was pressed, along with a t_event_buttonPressed
 * structure that stores which buttons was pressed and if Shift or Ctrl was
 * held down, etc.
 */
void event_loop(void (*act_on_mousebutton) (float x, float y, t_event_buttonPressed button_info),
        void (*act_on_mousemove) (float x, float y),
        void (*act_on_keypress) (char key_pressed, int keysym),
        void (*drawscreen) ());

/* Opens up the graphics; the window will have the UTF-8 string window_name
 * in its title bar and have the specified background colour.
 */
void init_graphics(const std::string& window_name, int cindex_background);
void init_graphics(const std::string& window_name, const t_color& background);
void change_graphics_background(const t_color& background);	// <Addition/Mod: Charles>

/* Sets world coordinates of the graphics window so that the
 * lower-left corner of the screen has world coordinate (xl, yb) and the
 * top-right corner has world coordinate (xr, yt), or if you prefer you
 * can call the other version with a bounding_box giving the coordinates.
 * Call this function before you call event_loop. Do not call it
 * in your drawscreen () callback function, since it will undo any
 * panning or zooming the user has done. If the aspect ratio (height/width) of
 * the coordinates you specify does not match the screen window aspect ratio,
 * either the visible height or visible width of your world will be increased
 * so that 1-world-unit vertically is the same distance as 1-world-unit
 * horizontally when rendered on the screen.
 */
void set_visible_world(float xl, float yb, float xr, float yt);
void set_visible_world(const t_bound_box& bounds);


/**
 * Returns a rectangle with the bounds of the drawn world -- i.e. what are
 * the edges in your chosen (world) coordinate system that match the edges of
 * the window? Useful for figuring out how zoomed in or out you are.
 * Also useful for getting the currently visible rectangle,
 * to manipulate it, or restore it later, by passing it back
 * to set_visible_world(..)
 */
t_bound_box get_visible_world();

/* Returns a rectangle with the screen coordinates -- i.e. top left is (0,0) --
 * defining the graphics area into which you can draw. This area doesn't include
 * the menu area with buttons or the status message area.
 */
t_bound_box get_visible_screen();


/* Closes the graphics windows, frees all its memory and so on. */
void close_graphics();


/************************* USER INTERACTION FUNCTIONS ******************/

/* Changes the status bar message (at the bottom of the main window) to the
 * UTF-8 string msg.
 */
void update_message(const std::string& msg);

/* Creates a button on the menu bar below the button with text (UTF-8)
 * prev_button_text.  The button will have text (UTF-8) button_text,
 * and when clicked will call function button_func.
 * button_func is a function that accepts a void function as
 * an argument; this argument is set to the drawscreen routine
 * as passed into the event loop.
 */
void create_button(const char *prev_button_text, const char *button_text,
        void (*button_func) (void (*drawscreen) ()));

/* Destroys the button with the given text; i.e. removes it from
 * the display.
 */
void destroy_button(const char *button_text);

/* Control what buttons are active (default:  all enabled) and
 * whether mouse movements and keyboard presses are sent to callback
 * functions (default:  disabled).
 */
void set_mouse_move_input(bool turn_on);
void set_keypress_input(bool turn_on);
void enable_or_disable_button(int ibutton, bool enabled);

/* Gets/Sets the current ZOOM_FACTOR
 */
float get_zoom_factor();
void set_zoom_factor(float new_zoom_factor);

/***************************** DRAWING ROUTINES *********************/

/****
 * The following routines draw anything you want to the biggest part of the
 * screen (the graphics sub-window), or if you have set the mode to PostScript
 * they write to a file to generate the printable file.
 *
 * The calls below split into ones that set "graphics attributes" like colour
 * and line widths, and rendering functions that actually draw text, lines, etc.
 * All the graphics attribute calls are "sticky" -- they affect all subsequent
 * drawing until you change the attribute again.
 ****/

/* Clears the screen. Should normally be the first call in your
 * screen redrawing function.
 */
void clearscreen();

/**
 * Set the current draw colour to the supplied index colour from color_types,
 * the specified (rgba) t_colour, or rgba value. alpha (a) = 0 means transparent,
 * while a = 255 means opaque (the default). Affects all drawing functions,
 * including text. Note: postscript does not support transparency, so
 * the alpha value is ignored in postscript output.
 */
void setcolor(int cindex);
void setcolor(const t_color& new_color);
void setcolor(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);

/**
 * Set the color with a string instead of an enumerated constant.
 * Colour names should be the same as their enum name, but all lower
 * case, and still no spaces. Slower than the other setcolor variations.
 */
void setcolor_by_name(std::string cname);

/* Get the current color */
t_color getcolor();

/* Sets the line style to the specified line_style, SOLID or DASHED
 * capstyle can be BUTT or ROUND, default is BUTT
 */
void setlinestyle(int linestyle, int capstyle = BUTT);

/* Sets the line width in pixels (for screen output) or points (1/72 of an inch)
 * for PostScript output. A value of 0 means thinnest possible line.
 */
void setlinewidth(int linewidth);

/**
 * Sets/gets the font size, in points. 72 points is 1 inch high.
 *
 * Having performance problems with lots of different text attribute combinations?
 * See comments about the font cache in settextattrs(..)
 */
void setfontsize(int pointsize);
int getfontsize();

/*
 * Set/get the rotation of text to be drawn. I recommend setting rotation
 * back to zero once you are done drawing all rotated text, as most
 * text will not be rotated, and setting rotation there may have been omitted.
 *
 * Having performance problems with lots of different text attribute combinations?
 * See comments about the font cache in settextattrs(..)
 */
void settextrotation(int degrees);
int gettextrotation();

/*
 * Set both the point size and rotation of the text in one call.
 * Slightly more effecient then calling setfontsize() and settextrotation()
 * separately, if that makes sense for your program.
 *
 * Also, If you plan on having many (ie > 40) nonzero rotation-pointsize, or
 * zero rotation-pointsize, combinations of text visible at the same time, I
 * recommend you look into changing the size of the font cache. See comments
 * near FONT_CACHE_SIZE_FOR_ROTATED and FONT_CACHE_SIZE_FOR_ZEROS in graphics.c
 */
void settextattrs(int pointsize, int degrees);

/* All the drawing functions below use current coordinate system set (either
 * world (the default) or screen.
 */

/* Draws a line from (x1, y1) to (x2, y2) or p1 to p2 */
void drawline(float x1, float y1, float x2, float y2);
void drawline(const t_point& p1, const t_point& p2);


/* Draws the rectangle with diagonally-opposite corners
 * at (x1, y1) and (x2, y2) or bottomleft and upperright points, or
 * the bound_box rect. All 3 functions are equivalent (use whichever is most
 * convenient), and draw with the current line style, colour and width.
 */
void drawrect(float x1, float y1, float x2, float y2);
void drawrect(const t_point& bottomleft, const t_point& upperright);
void drawrect(const t_bound_box& rect);



/* Draws a filled rectangle with the specified corners. */
void fillrect(float x1, float y1, float x2, float y2);
void fillrect(const t_point& bottomleft, const t_point& upperright);
void fillrect(const t_bound_box& rect);



/* Draws a filled polygon with the specified points as its boundary.
 * The first and last points passed in are connected to close the polygon.
 */
void fillpoly(t_point *points, int npoints);

/* Draw or fill a circular arc or elliptical arc.  Angles in degrees.
 * startang is measured from positive x-axis of Window.
 * A positive angextent means a counterclockwise arc; a negative
 * angextent means clockwise.
 */
void drawarc(float xcen, float ycen, float rad, float startang,
        float angextent);
void fillarc(const t_point& center, float rad, float startang, float angextent);
void fillarc(float xcen, float ycen, float rad, float startang,
        float angextent);

void drawellipticarc(
        float xc, float yc, float radx, float rady, float startang, float angextent);
void drawellipticarc(
        const t_point& center, float radx, float rady, float startang, float angextent);

void fillellipticarc(float xc, float yc, float radx, float rady, float startang, float angextent);
void fillellipticarc(const t_point& center, float radx, float rady, float startang, float angextent);


/*
 * These functions all draw UTF-8 text within some sort of bounding box;
 * they are all very closely related and just give slightly different
 * interfaces so you can choose whichever one is most convenient.
 *
 * The text is drawn centered around the point (xc,yc), text_center, or
 * in the case of drawtext_in, the centre of the bbox parameter.
 *
 * - SPECIFIC PAREMETER SPECIFICATION:
 *
 * If text won't fit in bounds specified, the text isn't drawn.
 * This is useful for avoiding text going everywhere at high zoom levels.
 *
 * boundx and boundy specify a width and height bound, respectively, whereas
 * bounds and bbox specify a box in which the text completely fit, or it won't
 * be drawn. boundx and boundy default to FLT_MAX, meaning always draw the
 * text.
 *
 * If you would like to have these functions ignore a particular bound,
 * specify a huge value. I recommend FLT_MAX or std::numeric_limits<float>::max(),
 * from <cfloat> and <limits> respectively.
 *
 * tolerance, effectively, makes the given bounding box bigger, on
 * all sides by that amount.
 *
 * For debugging purposes, if you would like to see exactly where the
 * bounds of the text are and the center of the text define SHOW_TEXT_BBOX
 * in your build, or uncomment it in drawtext(..) in graphics.c .
 *
 * - NOTES:
 * Finally, it should be noted that bounding is done based on the dimensions of
 * the final bounding rectangle of the actual rendered text, so the content _will_
 * affect the height (and/or width with rotated text), therefore affecting the bounds.
 * If you would like to hide text based on zoomlevel and fontsize directly, use
 * the Level Of Detail functions for this.
 *
 * As said before, these functions take UTF-8 encoded strings, so if you would like
 * to use non-ASCII characters, make sure your font supports them. Also, note that
 * those characters probably won't show up in postscript output, as postscript doesn't
 * understand UTF-8. They will be present in the file, but won't be displayed.
 *
 * Oh, and one more thing, if you would like aligned baselines, see the large comment
 * in drawtext(..) in graphics.c .
 */

void drawtext(float xc, float yc, const std::string& text, float boundx=FLT_MAX, float boundy=FLT_MAX);
float gettextwidth(const std::string& text);	// In the coodirnate system current drawing at // <Addition/Mod: Charles>
void drawtextleftaligned(float xc, float yc, const std::string& text, float boundx = FLT_MAX, float boundy = FLT_MAX);	// <Addition/Mod: Charles>
void drawtextrightaligned(float xc, float yc, const std::string& text, float boundx = FLT_MAX, float boundy = FLT_MAX);	// <Addition/Mod: Charles>
void drawtext(const t_point& text_center, const std::string& text, float boundx=FLT_MAX, float boundy=FLT_MAX);
void drawtext(const t_point& text_center, const std::string& text, const t_bound_box& bounds);
void drawtext(const t_point& text_center, const std::string& text, const t_bound_box& bounds, float tolerance);
void drawtext_in(const t_bound_box& bbox, const std::string& text);
void drawtext_in(const t_bound_box& bbox, const std::string& text, float tolerance);

/**
 * World coordinates = in the scene; will automatically pan and zoom
 * Screen coordinates = for any UI overlays; will be fixed in one spot on the window
 */
void set_coordinate_system(t_coordinate_system coord);

/*************************** ADVANCED FUNCTIONS ***************************/

/* Empties event queue. Can be useful with non-interactive graphics (animation)
 * to make sure things display.
 */
void flushinput();

/* DRAW_NORMAL is the default mode (overwrite, also known as copy_pen).
 * Can use DRAW_XOR for fast rubber-banding.
 */
void set_draw_mode(enum e_draw_mode draw_mode);

/**
 * Change the text on a button. Both strings are UTF-8.
 */
void change_button_text(const char *button_text, const char *new_button_text);

/* Normal users shouldn't have to use draw_message.  Should only be
 * useful if using non-interactive graphics and you want to redraw
 * yourself because of an expose.
 */
void draw_message();

/**
 * @brief Changes what all the drawX functions draws to.
 *
 * By default, drawX will draw straight to the screen. By setting this to
 * draw off screen, all drawX commands will draw to an offscreen pixmap.
 * When you've finished drawing everything to this pixmap, copy it to the
 * screen using copy_off_screen_buffer_to_screen(). Drawing to an offscreen
 * buffer like this (double-buffering) can reduce screen flashing when you
 * pan the graphics, making the drawing seem smoother.
 *
 * @param[in] draw_mode ON_SCREEN || OFF_SCREEN
 */
void set_drawing_buffer(t_draw_to draw_mode);

/**
 * @brief Copies the offscreen buffer to the main screen.
 *
 * Call this function when you want to display the offscreen pixmap to the
 * main screen. This routine does nothing if the set drawing_buffer was ON_SCREEN.
 */
void copy_off_screen_buffer_to_screen();

/**
 * @brief Loads png from file.
 *
 * @return A Surface you can draw to the screen.
 */
Surface load_png_from_file(const char* file_path);

/**
 * @brief Draws a Surface.
 *
 * @param[in] x x coordinate (world or screen) of upper-left corner
 * @param[in] y y coordinate (world or screen) of upper-left corner
 * @param[in] surface Surface loaded from load_png_from_file(const char*)
 */
void draw_surface(const Surface& surface, float x, float y);
void draw_surface(const Surface& surface, t_point upper_left);

/**************** LEVEL OF DETAIL FUNCTIONS   **************************
 *
 * These functions may be convenient for deciding to not draw
 * small details, unless they user is zoomed in past a certain level.
 ***********************************************************************/

/**
 * returns true iff the area of your (world) coordinate system
 * that is currently visible on the screen is below area_threshold.
 */
inline bool LOD_area_test(float area_threshold) {
    return get_visible_world().area() < area_threshold;
}

/**
 * returns true iff the smallest dimension of the visible
 * world is less than dim_threshold.
 */
bool LOD_min_dim_test(float dim_threshold);

/**
 * screen_area_threshold is in (screen pixels)^2. I suggest something around 3
 *
 * Iff the _screen_ area of the rectangle (passed in as world coordinates)
 * is less than screen_area_threshold then this function returns false. When
 * this function returns false it means that bounding box passed in will
 * occupy less than (screen_area_threshold pixels)^2 on the screen.
 */
bool LOD_screen_area_test(t_bound_box test, float screen_area_threshold);


/**
 * returns the screen coordinates (i.e. pixel location) of the world coordinate 'point'
 */
t_point world_to_scrn(const t_point& point);


/**
 * returns the screen coordinates (i.e. pixel locations) of the world coordinate bounding box
 * 'box'
 */
t_bound_box world_to_scrn(const t_bound_box& box);

/**
 * returns the world coordinates of the screen (i.e. pixel) location 'point'
 */
t_point scrn_to_world(const t_point& point);

/**
 * returns the world coordinates of the screen (i.e. pixel) bounding box 'box'
 */
t_bound_box scrn_to_world(const t_bound_box& box);

/***************** POSTSCRIPT PRINTING ROUTINES ******************************/

/* Opens file for postscript commands and initializes it.  All subsequent
 * drawing commands go to this file until close_postscript is called.
 * You can generate postscript output by explicitly calling
 * this routine, and then calling drawscreen. More commonly you'll
 * just click on the "PostScript" button though, and that button
 * calls this routine and drawscreen to generate a postscript file
 * that exactly matches the graphics area display on the screen.
 *
 * Limitation: Postscript output does not support transparency (object will
 * be opaque) or draw_surface of pngs/bitmaps (will not be drawn).
 *
 * Warning: not all UTF-8 filenames will work on Windows (this uses fopen)
 */
int init_postscript(const char *fname); /* Returns 1 if successful */

/* Closes file and directs output to screen again.       */
void close_postscript();

/******************** DEBUGGING FUNCTIONS **********************************/

/* Data structure below is for debugging the easygl API itself; you normally
 * will never use it or the functions below.
 */
/* t_report lets you get a bunch of info about the low-level graphics state.
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


/* For debugging only.  Get window size etc. */
void get_report_structure(t_report*);


/**************** Extra functions available only in WIN32 (and Cygwin). *******/
#if defined(WIN32) || defined(CYGWIN)
/* VB: TODO: I should make any generally useful functions below work in
 * X11 as well, and probably delete anything else.
 */

/* Draw beizer curve. Currently not used, but saving for possible use
 * in the future.
 */
void win32_drawcurve(t_point *points, int npoints);
void win32_fillcurve(t_point *points, int npoints);

/* Error message reporter */
void WIN32_DELETE_ERROR();
#endif // WIN32 or CYGWIN

#endif // GRAPHICS_H
