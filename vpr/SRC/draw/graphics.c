/*                                                                         *
* Easygl Version 3.0                                                       *
* Written by Vaughn Betz at the University of Toronto, Department of       *
* Electrical and Computer Engineering, with additions by Paul Leventis     *
* and William Chow of Altera, Guy Lemieux of the University of Brish       *
* Columbia, Long Yu (Mike) Wang of the University of Toronto, and          *
* Matthew J.P. Walker of the University of Toronto                         *
* All rights reserved by U of T, etc.                                      *
*                                                                          *
* You may freely use this graphics interface for non-commercial purposes   *
* as long as you leave the author info above in it.                        *
*                                                                          *
* Revision History:                                                        *
*                                                                          *
* V3.0 May 2014 - June 2014 (Matthew J.P. Walker)                          *
* - continued integration with c++                                         *
* - added ybounding and convenience functions to text drawing              *
* - added get_visible_world() - returns a rectangle describing the bounds  *
*   of the visible world.                                                  *
* - Panning and zooming will drop events instead of drawing all of them.   *
*   Much more usable on large drawings now.                                *
* - Added support for aribtrary RGB color triplets to X11, windows and ps. *
* - Modernized X11 font handling and text drawing by converting it to Xft. *
* - Added text rotation to X11, windows and postscript.                    *
* - Changed font caching, removing font size limit.                        *
* - Added unicode support in all text, via UTF-8.                          *
*                                                                          *
* V2.0.2 May 2013 - June 2013 (Mike Wang)                                  *
* - In Win32, removed "Window" operation with right mouse click to align   *
*   with X11.                                                              *
* - Fixed a bug in Win32 where when the "Window" button is clicked, if the *
*   application window gets minimized then returned back up, the screen    *
*   background would not be redrawn before new rubber band gets drawn.     *
* - Grouped file scope variables into structures, factored out functions   *
*   that were too long, and formatted code spacings etc.                   *
* - Cleaned up Win32 code by removing the inefficient saving and updating  *
*   of graphics contexts.                                                  *
* - Added zooming with respect to cursor location.                         *
* - Added panning with middle mouse button (or mousewheel) click and drag. *
* - Ran experiment to test the impact of rect_off_screen() on drawscreen() *
*   runtime. Found rect_off_screen() to be effective in speeding up screen *
*   redraw.                                                                *
* - Added zooming using mousewheel.                                        *
*                                                                          *
* V2.0.1 Sept. 2012 (Vaughn Betz)                                          *
* - Fixed a bug in Win32 where postscript output would make the graphics   *
*   crash when you redrew.                                                 *
* - Made a cleaner makefile to simplify platform selection.                *
* - Commented and reorganized some of the code.  Started cleaning up some  *
*   of the win32 code.  Win32 looks inefficient; it is saving and updating *
*   graphics contexts all the time even though we know when the context is *
*   valid vs. not-valid.                                                   *
*   TODO: make win32 work more like X11 (minimize gc updates).             *
*                                                                          *
* V2.0:  Nov. 21, 2011 (Vaughn Betz)                                       *
* - Updated example code, and some cleanup and bug fixes to win32 code.    *
* - Removed some win32 code that had no X11 equivalent or wasn't well      *
*   documented.                                                            *
* - Used const char * where appropriate to get rid of g++ warnings.        *
* - Made interface to things like xor drawing more consistent, and added   *
*   to example program.                                                    *
* - Made a simpler (easygl.cpp) interface to the graphics library for      *
*   use by undergraduate students.                                         *
*                                                                          *
* V1.06 : July 23, 2003 : (Guy Lemieux)                                    *
* - added some typecasts to cleanly compile with g++ and MS c++ tools      *
* - if WIN32 not defined, it defines X11 automatically                     *
* - fixed X11 compilation; WIN32 broke some things                         *
*                                                                          *
* V1.05 : July 26, 2001 : (William)									       *
* - changed keyboard detect function to accept an int (virtual key)        *
*                                                                          *
* V1.04 : June 29, 2001 : (William)									       *
* - added drawcurve(), fillcurve() using Bezier curves                     *
*   (support WIN32 screen / ps)											   *
* - added pt on object capability : using a memory buffer to draw an       *
*   graphics objects, then query if a point fall on the object (bear the   *
*   object's colour) : object_start(), object_end(), pt_on_object()        *
* - added drawellipticarc(), fillellipticarc()                             *
* - added findfontsize() to help find a pointsize of a given height        *
* - extended t_report to keep xleft, xright, ytop, ybot                    *
* - added update_window() to set the window bb                             *
*                                                                          *
* V1.03 : June 18, 2001 : (William)									       *
* - added change_button_text()											   *
*                                                                          *
* V1.02 : June 13, 2001 : (William)									       *
* - extension to mouse click function : can tell if ctrl/shift keys are	   *
*   pressed															  	   *
*                                                                          *
* V1.01 : June 1, 2001 : (William)									       *
* - add tooltip support												  	   *
*                                                                          *
* V1.0 : May 14, 2001 : (William)									       *
* - fixed a problem with line styles, initial release on the internet  	   *
*                                                                          *
* March 27, 2001 : (William)		                                       *
* - added setcolor_by_colorref to make more colors available (in Win32)	   *
*                                                                          *
* February 16, 2001 : (William)                                            *
* - added quick zoom using right mouse clicks							   *
*                                                                          *
* February 11, 2001 : (William)                                            *
* - can define cleanup(), passed in when calling init_graphics(), and	   *
*   called when shutting down							                   *
*                                                                          *
* February 1, 2001 : (William)                                             *
* - fix xor mode redraw problem			                                   *
*                                                                          *
* September 19, 2000 : (William)                                           *
* - can define mouse_move callback function                                *
* - can add separators in between buttons                                  *
*                                                                          *
* September 8, 2000 : (William)                                            *
* - added result_structure(),                                              *
* - can define background color in init_graphics                           *
*                                                                          *
* August 10, 2000 : (William Chow, choww@eecg.utoronto.ca)                 *
* - Finished all Win32 support functions                                   *
* - use XOR mode for window zooming box                                    *
* - added double buffering feature                                         *
*                                                                          *
* January 12, 1999: (Paul)                                                 *
* - Fixed a bunch of stuff with the Win32 support (memory leaks, etc)      *
* - Made the clipping function using the update rectangle for Win32        *
*                                                                          *
* January 9, 1999:  (Paul Leventis, leventi@eecg.utoronto.ca)              *
* - Added Win32 support.  Should work under Windows98/95/NT 4.0/NT 5.0.    *
* - Added a check to deselect_all to determine whether the screen needs to *
* be updated or not.  Should elminate flicker from mouse clicks            *
* - Added win32_invalidate_screen() call to graphics.c that in turn calls  *
*   update_screen, so this function was made non-static and added to the   *
*   header file.  This is due to differences in the structure of Win32     *
*   windowing apps.                                                        *
* - Win32 needs clipping (though done automatically, could be faster)      *
*                                                                          *
* Sept. 19, 1997:  Incorporated Zoom Fit code of Haneef Mohammed at        *
* Cypress.  Makes it easy to zoom to a full view of the graphics.          *
*                                                                          *
* Sept. 11, 1997:  Added the create_and destroy_button interface to        *
* make it easy to add and destroy buttons from user code.  Removed the     *
* bnum parameter to the button functions, since it wasn't really needed.   *
*                                                                          *
* June 28, 1997:  Added filled arc drawing primitive.  Minor modifications *
* to PostScript driver to make the PostScript output slightly smaller.     *
*                                                                          *
* April 15, 1997:  Added code to init_graphics so it waits for a window    *
* to be exposed before returning.  This ensures that users of non-         *
* interactive graphics can never draw to a window before it is available.  *
*                                                                          *
* Feb. 24, 1997:  Added code so the package will allocate  a private       *
* colormap if the default colormap doesn't have enough free colours.       *
*                                                                          *
* June 28, 1996:  Converted all internal functions in graphics.c to have   *
* internal (static) linkage to avoid any conflicts with user routines in   *
* the rest of the program.                                                 *
*                                                                          *
* June 12, 1996:  Added setfontsize and setlinewidth attributes.  Added    *
* pre-clipping of objects for speed (and compactness of PS output) when    *
* graphics are zoomed in.  Rewrote PostScript engine to shrink the output  *
* and make it easier to read.  Made drawscreen a callback function passed  *
* in rather than a global.  Graphics attribute calls are more efficient -- *
* they check if they have to change anything before doing it.              * 
*                                                                          *
* October 27, 1995:  Added the message area, a callback function for       *
* interacting with user button clicks, and implemented a workaround for a  *
* Sun X Server bug that misdisplays extremely highly zoomed graphics.      *
*                                                                          *
* Jan. 13, 1995:  Modified to incorporate PostScript Support.              */

/**************************** Top-level summary ******************************
This graphics package provides API for client program that wishes to implement 
a graphical user interface. The client program will first call init_graphics()
for initial setup, which includes opening up an application window, setting up
the specified window_name and background colour, creating sublevel windows, 
etc. Then the program will call a few more setup functions such as 
init_world(), which sets up world coordinates, create_button(), which sets up
menu buttons, and update_message(), which updates status bar message. After 
all the setup, the program will call the main routine for the graphics,
event_loop(), which will take control of the program until the "Proceed" 
button is pressed. Event_loop() directly communicates with X11/Win32 to 
receive event notifications, and it either calls other event-handling 
functions in the graphics package or callbacks passed as function pointers 
from the client program. The most important callback function which the 
client should provide is drawscreen(). Whenever the graphics need to be 
redrawn, drawscreen() will be called. There are a number drawing routines 
which the client can use to update the graphic contexts (ie. setcolor, 
setfontsize, setlinewidth, and setlinestyle) and draw simple shapes as
desired. When the program is done executing the graphics, it should call
close_graphics() to release all drawing structures and close the graphics.*/


/**********************************
 * Common Preprocessor Directives *
 **********************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sys/timeb.h>
#include "graphics.h"
using namespace std;

#ifndef NO_GRAPHICS  // Strip everything out and just put in stubs if NO_GRAPHICS defined


#if defined(X11) || defined(WIN32)

// VERBOSE is very helpful for developing event handling features. Outputs
// useful information when user interacts with the graphic interface.
// Uncomment the line below to turn on VERBOSE.
// #define VERBOSE

#define MWIDTH			104 /* Width of menu window */
#define MENU_FONT_SIZE  11  /* Font for menus and dialog boxes. */
#define T_AREA_HEIGHT	24  /* Height of text window */

/**
 * Maximum sizes for the font cache. The "ZEROS" one is for fonts of zero rotation, and
 * the "ROTATED" one is for fonts with other rotation. If you plan on rotating text to
 * many rotations, say random ones, and displaying them all at the same time, I suggest
 * something large for "ROTATED". The value of "ZEROS" is probably fine.
 *
 * What these values really mean to you is: because the text drawing is really lazy about
 * loading fonts, the "ZEROS" should be at least how many font sizes you expect to be visible
 * at a given time, and the "ROTATED" is for how many other different angle-fontsize combinations
 * you expect to be visible at a given time. If you program remains within these limits, there
 * will be no slow frames, except the first one where you change many visible combinations.
 * For after this frame, the font cache has filled up with the fonts that are visible
 *
 * A quick experiment gave these results:
 *    10 fonts =>     540 KiB
 *   100 fonts =>   1.7   MiB
 *  1000 fonts =>  13.5   Mib
 * 10000 fonts => 132.4   Mib
 */
#define FONT_CACHE_SIZE_FOR_ZEROS 40
#define FONT_CACHE_SIZE_FOR_ROTATED 40

#define PI				3.141592654

#define BUTTON_TEXT_LEN	100
#define BUFSIZE			1000

#define ZOOM_FACTOR  1.6667 /* For zooming on the graphics */
#endif

#ifdef X11

/*********************************************
 * X-Windows Specific Preprocessor Directives *
 *********************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

/* Uncomment the line below if your X11 header files don't define XPointer */
/* typedef char *XPointer;                                                 */

#endif /* X11 Preprocessor Directives */

#ifdef WIN32

/*************************************************************
 * Microsoft Windows (WIN32) Specific Preprocessor Directives *
 *************************************************************/

#pragma warning(disable : 4996)   // Turn off annoying warnings about strcmp.

#ifndef UNICODE
 #define UNICODE // force windows api into unicode (usually UTF-16) mode.
#endif

#include <windows.h>
#include <WindowsX.h>

/* Using large values of pixel limits in order to preserve the slope of diagonal 
 * lines when their endpoints are clipped (one at a time) to these pixel limits.
 * However, if these values are too large, then Windows will not draw the lines
 * when zoomed way in. I have increased MAXPIXEL and MINPIXEL to these values so
 * the diagonal lines do not change slope when zoomed way in. Also, I have tested 
 * to make sure these values are safe. Adding one more digit to these values 
 * may cause problems when zoomed way in. 
 * As a last note, specifying these values does not affect line clipping in X11. 
 * The most probable reason is that X11 does line clipping automatically.
 * MW, June 2013.
 */
#define MAXPIXEL 21474836
#define MINPIXEL -21474836

/* Windows work in degrees, where as X11 and PostScript both work in radians.
 * Thus, a conversion is needed for Windows. 
 */
#define DEGTORAD(x) ((x)/180.*PI)
#endif /* Win32 preprocessor Directives */

#ifdef X11

/*************************************************************
 * X11 Structure Definitions                                 *
 *************************************************************/

/* Structure used to store X Windows state variables.
 * display: Structure containing information needed to
 *			communicate with Xlib.
 * screen_num: Value the Xlib server uses to identify every
 *			   connected screen.
 * current_font: the currently selected font.
 * toplevel, menu, textarea: Toplevel window and 2 child windows.
 * gc_normal, gc_xor, current_gc: Graphic contexts for drawing
 *				in the graphics area. (gc_normal for overwrite
 *				drawing, gc_xor for rubber band drawing, and
 *				current_gc is the current gc used)
 * gc_menus: Graphic context for drawing in the status message
 *			 and menu area
 * xft_menutextcolor: the XFT colour used for drawing button and menu text.
 * xft_currentcolor: the XFT colour used for drawing normal text. Is kept in
 *                   sync with gl_state.foreground_color
 */
struct t_x11_state {
   Display *display;
   int screen_num;
   Window toplevel, menu, textarea;
   GC gc_normal, gc_xor, gc_menus, current_gc;
   XftDraw *toplevel_draw, *menu_draw, *textarea_draw;
   XftColor xft_menutextcolor;
   XftColor xft_currentcolor;
   XVisualInfo visual_info;
   Colormap colormap_to_use;
};

typedef XftFont* font_ptr;

#endif

#ifdef WIN32

/*************************************************************
 * WIN32 Structure Definitions                               *
 *************************************************************/

/* Flag used for the "window" button. Before the user presses the button, the
 * "window" operation is in WINDOW_DEACTIVATED state. After user presses the
 * button, the operation proceeds to WAITING_FOR_FIRST_CORNER_POINT state and
 * waits for the user to click the first point in the graphics area as the first
 * corner for rubber band drawing. After user clicks the first point, the operation
 * proceeds to WAITING_FOR_SECOND_CORNER_POINT and waits for the user to click on
 * the second point to define the rectangular region enclosed by the rubber band.
 * Then the application window will zoom in to the region defined and the "window"
 * operation goes back to WINDOW_DEACTIVATED.
 */
typedef enum {
	WINDOW_DEACTIVATED = 0,
	WAITING_FOR_FIRST_CORNER_POINT,
	WAITING_FOR_SECOND_CORNER_POINT
} t_window_button_state;


/* Structure used to store Win32 state variables.
 * InEventLoop: Whether in event_loop(); used to indicate if part of the application window need
 *              to be redrawn when system makes request by sending WM_PAINT message in the
 *				WIN32_GraphicsWND callback function.
 * windowAdjustFlag: Flag used for the "Window" button operation. This variable has 3 states
 *					 which are defined above.
 * adjustButton: Holds the index of the "Window" button in the array of buttons created
 * adjustRect: Used for the "Window" button operation. Holds the boundary coordinates (in screen
 *			   pixels) of the region enclosed by the rubber band.
 * hMainWnd, hGraphicsWnd, hButtonsWnd, hStatusWnd: Handles to the top level window and
 *				3 subwindows.
 * hGraphicsDC: Handle to the graphics device context.
 * hGraphicsPen, hGraphicsBrush, hGrayBrush, hGraphicsFont: Handles to Windows GDI objects used
 *				for drawing. (hGraphicsPen for drawing lines, hGraphicsBrush for filling shapes,
 *				and hGrayBrush for filling the background of the status message and menu areas)
 */
struct t_win32_state {
	bool InEventLoop;
	t_window_button_state windowAdjustFlag;
	int adjustButton;
	RECT adjustRect;
	HWND hMainWnd, hGraphicsWnd, hButtonsWnd, hStatusWnd;
	HDC hGraphicsDC;
	HPEN hGraphicsPen;
	HBRUSH hGraphicsBrush, hGrayBrush;
	HFONT hGraphicsFont;
};

typedef LOGFONT* font_ptr;

#endif

/*************************************************************
 * Common Structure Definitions                              *
 *************************************************************/

/* Used to define where the output of drawscreen (graphics primitives in the user-controlled
 * area) currently goes to: the screen or a postscript file.
 */
typedef enum {
	SCREEN = 0,
	POSTSCRIPT = 1
} t_display_type;


/* Indicates if this button displays text, a polygon or is just a separator.
 */
typedef enum {
	BUTTON_TEXT = 0,
	BUTTON_POLY,
	BUTTON_SEPARATOR
} t_button_type;


/* Structure used to define the buttons on the right hand side of the main window (menu region).
 * width, height: button size, in pixels.
 * xleft, ytop: coordinates, in pixels, of the top-left corner of the button relative to its
 *    containing (menu) window.
 * fcn: a callback function that is called when the button is pressed. This function takes one 
 *    argument, a function pointer to the routine that can draw the graphics area (user routine).
 * win, hwnd:  X11 and Win32 data pointer to the window, respectively.
 * button_type: indicates if this button displays text, a polygon or is just a separator.
 * text: the text to display if this is a text button.
 * poly: the polygon (up to 3 points right now) to display if this is a polygon button
 * is_pressed: has the button been pressed, and is currently executing its callback?
 * is_enabled: can you press this button right now? Visually will look "pushed in" when
 *      not enabled, and won't respond to clicks.
 */
typedef struct {
	int width;
	int height;
	int xleft;
	int ytop;
	void(*fcn) (void(*drawscreen) (void));
#ifdef X11
	Window win;
	XftDraw* draw;
#else
	HWND hwnd;
#endif
	t_button_type type;
	char text[BUTTON_TEXT_LEN];
	int poly[3][2];
	bool ispressed;
	bool enabled;
} t_button;


/* Structure used to store all the buttons created in the menu region.
 * button: array of pointers to all the buttons created [0..num_buttons-1]
 * num_buttons: number of menu buttons created
 */
typedef struct {
	t_button *button;
	int num_buttons;
} t_button_state;

/**
 * This is a class that caches fonts. It separates out unrotated and rotated fonts,
 * for the reason that the unrotated ones are used for bounds estimation anyway, as
 * well as that they are generally convenient to have around. For each group, it uses
 * a queue to keep track of the order of creation, and when a new font would put a
 * cache over capacity, it deletes the oldest one first. The maps are to allow fast
 * lookup of fonts from their size and rotation.
 *
 * This cache is also not very complicated, and it could be, if there were a good reason.
 *
 * Also note that no references retrieved from this cache should be retained, as
 * another part of the program may cause the creation of another font(s) that will
 * push yours out. Instead a font should be retrieved every time it is needed.
 */
class FontCache {
public:
	FontCache()
		: order_zeros()
		, descriptor2font_zeros()
		, order_rotated()
		, descriptor2font_rotated()
	{ }

	/**
	 * Retrieve a font from this cache, or create it if it doesn't already exist,
	 * pushing out, and deleting a font if the new font wont fit in its cache.
	 * Gets a font from one of two caches depending on whether or not it is rotated.
	 */
	font_ptr get_font_info(size_t pointsize, float degrees);

	/**
	 * Clear out this cache so that it contains no fonts. This deletes all fonts,
	 * as well, so make sure any fonts that were retrieved before cannot be referenced.
	 */
	void clear();
private:
	typedef std::pair<size_t,float> font_descriptor;
	struct fontdesc_hasher {
		inline std::size_t operator()(const font_descriptor& v) const {
			std::hash<size_t> sizet_hasher;
			std::hash<float> float_hasher;
			return sizet_hasher(v.first) ^ float_hasher(v.second);
		}
	};
	FontCache(const FontCache&);
	FontCache& operator=(const FontCache&);

	// this function actually does all the work of get_font_info, but only
	// looks/creates in the given map and queue.
	template<class queue_type, class map_type>
	static font_ptr get_font_info(
		size_t pointsize, float degrees,
		queue_type& orderqueue, map_type& descr2font_map, size_t max_size);

	// unrotated fonts (zero rotation)
	std::deque<font_descriptor> order_zeros;
	std::unordered_map<font_descriptor, font_ptr, fontdesc_hasher> descriptor2font_zeros;
	// rotated fonts
	std::deque<font_descriptor> order_rotated;
	std::unordered_map<font_descriptor, font_ptr, fontdesc_hasher> descriptor2font_rotated;
};

/* Structure used to store overall graphics state variables.
 * initialized:  true if the graphics window & state have been
 *      created and initialized, false otherwise.
 * disp_type: Selects SCREEN or POSTSCRIPT
 * background_color: colour of the window (or page for PS) background colour
 * foreground_color: current color in the graphics context
 * currentlinestyle: current linestyle in the graphics context
 * currentlinewidth: current linewidth in the graphics context
 * currentfontsize: current font size in the graphics context
 * currentfontrotation: current text rotation angle, in degrees
 * current_draw_mode: select DRAW_NORMAL (for overwrite) or DRAW_XOR (for rubber-banding)
 * ps: for PostScript output
 * ProceedPressed: whether the Proceed button has been pressed
 * statusMessage: user message to display
 * font_info: a font cache
 * get_keypress_input: whether keypresses are sent back to callback functions
 * get_mouse_move_input: whether mouse movements are sent back to callback functions
 *
 * Need to initialize graphics_loaded to false, since checking it is
 * how we avoid multiple construction or destruction of the graphics
 * window. Initializing display type and background color index is not
 * necessary since they are set in init_graphics(), but doing so
 * just for safety.
 */
struct t_gl_state {
   bool initialized;
   t_display_type disp_type;
   t_color background_color;
   t_color foreground_color;
   int currentlinestyle;
   int currentlinewidth;
   int currentfontsize;
   float currentfontrotation;
   e_draw_mode current_draw_mode;
   FILE *ps;
   bool ProceedPressed;
   char statusMessage[BUFSIZE];
   FontCache font_info;
   bool get_keypress_input, get_mouse_move_input;
	t_gl_state()
		: initialized(false)
		, disp_type(SCREEN)
		, background_color(0xFF, 0xFF, 0xCC)
	{ }
};


/* Structure used to store coordinate information used for
 * graphic transformations.
 * display_width and display_height: entire screen size
 * top_width and top_height: application window size in pixels
 * init_xleft, init_xright, init_ytop, and init_ybot: initial world	coordinates
 *							(for use in zoom_fit() to restore initial coordinates)
 * xleft, xright, ytop, and ybot: boundaries of the graphic child window in world
 * 									coordinates
 * ps_left, ps_right, ps_top, and ps_bot: figure boundaries for PostScript output,
 *											in PostScript coordinates
 * ps_xmult and ps_ymult: world to PostScript transformation factors (number of
 *							PostScript coordinates per world coordinate)
 * wtos_xmult and wtos_ymult: world to screen transformation factors (number of
 *								screen pixels per world coordinate)
 * stow_xmult and stow_ymult: screen to world transformation factors (number of
 *								world coordinates per screen pixel)
 */
typedef struct {
	int display_width, display_height;
	int top_width, top_height;
	float init_xleft, init_xright, init_ytop, init_ybot;
	float xleft, xright, ytop, ybot;
	float ps_left, ps_right, ps_top, ps_bot;
	float ps_xmult, ps_ymult;
	float wtos_xmult, wtos_ymult;
	float stow_xmult, stow_ymult;
} t_transform_coordinates;


/* Structure used to store state variables used for panning.
 * previous_x and previous_y: (in window (pixel) coordinates,) last location of
 * 								the cursor before new motion
 * panning_enabled: whether panning is activated or de-activated
 */
typedef struct {
	int previous_x, previous_y;
	bool panning_enabled;
} t_panning_state;

/*********************************************************************
 *        File scope variables.              *
 *********************************************************************/

static t_gl_state gl_state;

// Stores all the menu buttons created. Initialize the button pointer 
// and num_buttons for safety.
static t_button_state button_state = {NULL, 0};

// Contains all coordinates useful for graphics transformation
static t_transform_coordinates trans_coord;

// Initialize panning_enabled to false, so panning is not activated
static t_panning_state pan_state = {0, 0, false};

// Predefined colours
static const vector<t_color> predef_colors = {
	t_color(0xFF, 0xFF, 0xFF),  // "white"
	t_color(0x00, 0x00, 0x00),  // "black"
	t_color(0x8C, 0x8C, 0x8C),  // "grey55"
	t_color(0xBF, 0xBF, 0xBF),  // "grey75"
	t_color(0xFF, 0x00, 0x00),  // "red"
	t_color(0xFF, 0xA5, 0x00),  // "orange"
	t_color(0xFF, 0xFF, 0x00),  // "yellow"
	t_color(0x00, 0xFF, 0x00),  // "green"
	t_color(0x00, 0xFF, 0xFF),  // "cyan"
	t_color(0x00, 0x00, 0xFF),  // "blue"
	t_color(0xA0, 0x20, 0xF0),  // "purple"
	t_color(0xFF, 0xC0, 0xCB),  // "pink"
	t_color(0xFF, 0xB6, 0xC1),  // "lightpink"
	t_color(0x00, 0x64, 0x00),  // "darkgreen"
	t_color(0xFF, 0x00, 0xFF),  // "magenta"
	t_color(0xFF, 0xE4, 0xC4),  // "bisque"
	t_color(0x87, 0xCE, 0xFA),  // "lightskyblue"
	t_color(0xD8, 0xBF, 0xD8),  // "thistle"
	t_color(0xDD, 0xA0, 0xDD),  // "plum"
	t_color(0xF0, 0xE6, 0x8C),  // "khaki"
	t_color(0xFF, 0x7F, 0x50),  // "coral"
	t_color(0x40, 0xE0, 0xD0),  // "turquoise"
	t_color(0x93, 0x70, 0xDB),  // "mediumpurple"
	t_color(0x48, 0x3D, 0x8B),  // "darkslateblue"
	t_color(0xBD, 0xB7, 0x6B),  // "darkkhaki"
	t_color(0x44, 0x44, 0xFF),  // "lightmediumblue"
	t_color(0x8B, 0x45, 0x13),  // "saddlebrown"
	t_color(0xB2, 0x22, 0x22),  // "firebrick"
	t_color(0x32, 0xCD, 0x32)   // "limegreen"
};

// assert(predef_colors.size() == NUM_COLOR);

// Color names, also used in postscript generation
static const char *ps_cnames[NUM_COLOR] = {
	"white",
	"black",
	"grey55",
	"grey75",
	"red",
	"orange",
	"yellow",
	"green",
	"cyan",
	"blue",
	"purple",
	"pink",
	"lightpink",
	"darkgreen",
	"magenta",
	"bisque",
	"lightskyblue",
	"thistle",
	"plum",
	"khaki",
	"coral",
	"turquoise",
	"mediumpurple",
	"darkslateblue",
	"darkkhaki",
	"lightmediumblue", // (Non X11, but that won't be a problem)
	"saddlebrown",
	"firebrick",
	"limegreen"
};

#ifdef X11

/*************************************************
 *     X-Windows Specific File-scope Variables   *
 *************************************************/

// Stores all state variables for X Windows
t_x11_state x11_state;

#endif /* X11 file scope variables */

#ifdef WIN32

/*****************************************************
 *  Microsoft Windows (Win32) File Scope Variables   *
 *****************************************************/

/* Linestyle references for Win32. */
static const int win32_line_styles[2] = { PS_SOLID, PS_DASH };

/* Name of each window */
static wchar_t szAppName[256], szGraphicsName[] = L"VPR Graphics",
			 szStatusName[] = L"VPR Status", szButtonsName[] = L"VPR Buttons";

/* Stores all state variables for Win32 */
static t_win32_state win32_state = {false, WINDOW_DEACTIVATED, -1};

#endif /* WIN32 file scope variables */


/*********************************************
 *       Common Subroutine Declarations      *
 *********************************************/

static void *my_malloc(int ibytes);
static void *my_realloc(void *memblk, int ibytes);

/* translation from screen coordinates to the world coordinates *
 * used by the client program.                                  */
static float xscrn_to_world(int x);
static float yscrn_to_world(int y); 
// static t_point scrn_to_world(const t_point& point); // uncomment if needed
// static t_bound_box scrn_to_world(const t_bound_box& box); // uncomment if needed

/* translation from world to screen coordinates */
static int xworld_to_scrn (float worldx);
static int yworld_to_scrn (float worldy);
static float xworld_to_scrn_fl (float worldx); // without rounding to nearest pixel
static float yworld_to_scrn_fl (float worldy);
static t_point world_to_scrn(const t_point& point);
static t_bound_box world_to_scrn(const t_bound_box& box);

/* translation from world to PostScript coordinates */
static float xworld_to_post(float worldx); 
static float yworld_to_post(float worldy);
// static t_point world_to_post(const t_point& point); // uncomment if needed
// static t_bound_box world_to_post(const t_bound_box& box); // uncomment if needed

static void force_setcolor(int cindex);
static void force_setcolor(const t_color& new_color);
static void update_brushes();
static void force_setlinestyle(int linestyle);
static void force_setlinewidth(int linewidth);
static void force_settextattrs(int pointsize, float degrees);
static font_ptr do_font_loading(int pointsize, float degrees);
static void close_font(font_ptr font);

static void reset_common_state ();
static void build_default_menu (void); 

/* Function declarations for button responses */
static void translate_up (void (*drawscreen) (void)); 
static void translate_left (void (*drawscreen) (void)); 
static void translate_right (void (*drawscreen) (void)); 
static void translate_down (void (*drawscreen) (void)); 
static void panning_execute (int x, int y, void (*drawscreen) (void));
static void panning_on (int start_x, int start_y);
static void panning_off (void);
static void zoom_in (void (*drawscreen) (void));
static void zoom_out (void (*drawscreen) (void));
static void handle_zoom_in (float x, float y, void (*drawscreen) (void));
static void handle_zoom_out (float x, float y, void (*drawscreen) (void));
static void zoom_fit (void (*drawscreen) (void));
static void adjustwin (void (*drawscreen) (void)); 
static void postscript (void (*drawscreen) (void));
static void proceed (void (*drawscreen) (void));
static void quit (void (*drawscreen) (void)); 
static void map_button (int bnum); 
static void unmap_button (int bnum);

#ifdef X11

/****************************************************
 *     X-Windows Specific Subroutine Declarations    *
 *****************************************************/

/* Helper functions for X11; not visible to client program. */
static void x11_init_graphics (const char* window_name);
static void x11_event_loop (void (*act_on_mousebutton)
									(float x, float y, t_event_buttonPressed button_info),
							void (*act_on_mousemove)(float x, float y), 
							void (*act_on_keypress)(char key_pressed),
							void (*drawscreen) (void));

static Bool x11_test_if_exposed (Display *disp, XEvent *event_ptr, 
							 XPointer dummy);
static void x11_build_textarea (void);
static void x11_drawbut (int bnum);
static int x11_which_button (Window win);

static void x11_turn_on_off (int pressed);
static void x11_drawmenu(void);
static void menutext(XftDraw* draw, int xc, int yc, const char *text);

static void x11_handle_expose (XEvent report, void (*drawscreen) (void));
static void x11_handle_configure_notify (XEvent report);
static void x11_handle_button_info (t_event_buttonPressed *button_info,
								int buttonNumber, int Xbutton_state);

#endif /* X11 Declarations */

#ifdef WIN32

/*******************************************************************
 *    Win32-specific subroutine declarations                       *
 *******************************************************************/

/* Helper function called by init_graphics(). Not visible to client program. */
static void win32_init_graphics (const char* window_name);

/* Callback functions for the top-level window and 3 sub-windows.  
 * Windows uses an odd mix of events and callbacks, so it needs these.
 */
static LRESULT CALLBACK WIN32_GraphicsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WIN32_StatusWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WIN32_ButtonsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WIN32_MainWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// For Win32, need to save pointers to these callback functions at file
// scope, since windows has a bizarre event loop structure where you poll
// for events, but then dispatch the event and get  called via a callback 
// from windows (WIN32_GraphicsWND, below).  I can't figure out why windows 
// does things this way, but it is what makes saving these function pointers
// necessary.  VB.
static void (*win32_mouseclick_ptr)(float x, float y, t_event_buttonPressed button_info);
static void (*win32_mousemove_ptr)(float x, float y);
static void (*win32_keypress_ptr)(char entered_char);
static void (*win32_drawscreen_ptr)(void);

// Helper functions for WIN32_GraphicsWND callback function
static void win32_GraphicsWND_handle_WM_PAINT(HWND hwnd, PAINTSTRUCT &ps, HPEN &hDotPen, 
											  RECT &oldAdjustRect);
static void win32_GraphicsWND_handle_WM_LRBUTTONDOWN(UINT message, WPARAM wParam, LPARAM lParam, 
													 int &X, int &Y, RECT &oldAdjustRect);
static void win32_GraphicsWND_handle_WM_MBUTTONDOWN(HWND hwnd, UINT message, WPARAM wParam, 
													 LPARAM lParam);
static void win32_GraphicsWND_handle_WM_MOUSEMOVE(LPARAM lParam, int &X, int &Y, 
											      RECT &oldAdjustRect);

// Functions for displaying errors in a message box on windows.
static void WIN32_SELECT_ERROR(); 
static void WIN32_DELETE_ERROR(); 
static void WIN32_CREATE_ERROR();
static void WIN32_DRAW_ERROR();  

static void win32_invalidate_screen();  

static void win32_reset_state ();
static void win32_drain_message_queue ();

static void win32_handle_mousewheel_zooming(WPARAM wParam, LPARAM lParam, bool draw_screen);
static void win32_handle_button_info (t_event_buttonPressed &button_info, UINT message, 
									  WPARAM wParam);

COLORREF convert_to_win_color(const t_color& src);

#endif /* Win32 Declarations */


/*********************************************************
 *          Common Subroutine Definitions                *
 *********************************************************/


/* safer malloc */
static void *my_malloc(int ibytes) {
	void *mem;
	
	mem = (void*)malloc(ibytes);
	if (mem == NULL) {
		printf("memory allocation failed!");
		exit(-1);
	}

	return mem;
}


/* safer realloc */
static void *my_realloc(void *memblk, int ibytes) {
	void *mem;
	
	mem = (void*)realloc(memblk, ibytes);
	if (mem == NULL) {
		printf("memory allocation failed!");
		exit(-1);
	}

	return mem;
}


/* translation from screen coordinates to the world coordinates *
 * in the x direction.											*/
static float xscrn_to_world(int x)
{
	float world_coord_x;
	world_coord_x = ((float) x)*trans_coord.stow_xmult + trans_coord.xleft;

	return world_coord_x;
}


/* translation from screen coordinates to the world coordinates *
 * in the y direction.											*/
static float yscrn_to_world(int y)
{
	float world_coord_y;
	world_coord_y = ((float) y)*trans_coord.stow_ymult + trans_coord.ytop;

	return world_coord_y;
}

// static t_point scrn_to_world(const t_point& point) {
// 	return t_point(
// 		xscrn_to_world(point.x),
// 		yscrn_to_world(point.y)
// 	);
// }

// static t_bound_box scrn_to_world(const t_bound_box& box) {
// 	return t_bound_box(
// 		scrn_to_world(box.bottom_left()),
// 		scrn_to_world(box.top_right())
// 	);
// }

/* Translates from world (client program) coordinates to screen coordinates
 * (pixels) in the x direction.  Add 0.5 at end for extra half-pixel accuracy. 
 */
static int xworld_to_scrn (float worldx) {
	return (int) (xworld_to_scrn_fl(worldx) + 0.5);
}

static float xworld_to_scrn_fl (float worldx) 
{
	float winx;
	
	winx = (worldx-trans_coord.xleft)*trans_coord.wtos_xmult;

#ifdef WIN32
	/* Avoids overflow in the  Window routines.  This will allow horizontal *
	* and vertical lines to be drawn correctly regardless of zooming, but   *
	* will cause diagonal lines that go way off screen to change their      *
	* slope as you zoom in.  The only way I can think of to completely fix  *
	* this problem is to do all the clipping in advance in floating point,  *
	* then convert to integers and call   Windows.  This is a lot of extra  *
	* coding, and means that coordinates will be clipped twice, even though *
	* this "Super Zoom" problem won't occur unless users zoom way in on     * 
	* the graphics.                                                         */ 

	winx = max (winx, MINPIXEL);
	winx = min (winx, MAXPIXEL);
#endif

	return (winx);
}


/* Translates from world (client program) coordinates to screen coordinates
 * (pixels) in the y direction.  Add 0.5 at end for extra half-pixel accuracy. 
 */
static int yworld_to_scrn (float worldy) {
	return (int) (yworld_to_scrn_fl(worldy) + 0.5);
}

static float yworld_to_scrn_fl (float worldy) 
{
	float winy;
	
	winy = (worldy-trans_coord.ytop)*trans_coord.wtos_ymult;

#ifdef WIN32
	/* Avoid overflow in the X/Win32 Window routines. */
	winy = max (winy, MINPIXEL);
	winy = min (winy, MAXPIXEL);
#endif

	return (winy);
}

static t_point world_to_scrn(const t_point& point) {
	return t_point(
		xworld_to_scrn_fl(point.x),
		yworld_to_scrn_fl(point.y)
	);
}

static t_bound_box world_to_scrn(const t_bound_box& box) {
	return t_bound_box(
		world_to_scrn(box.bottom_left()),
		world_to_scrn(box.top_right())
	);
}

/* translation from world to PostScript coordinates in the x direction */
static float xworld_to_post(float worldx)
{
	float ps_coord_x;
	ps_coord_x = ((worldx)-trans_coord.xleft)*trans_coord.ps_xmult + trans_coord.ps_left;

	return ps_coord_x;
}


/* translation from world to PostScript coordinates in the y direction */
static float yworld_to_post(float worldy)
{
	float ps_coord_y;
	ps_coord_y = ((worldy)-trans_coord.ybot)*trans_coord.ps_ymult + trans_coord.ps_bot;

	return ps_coord_y;
}

// static t_point world_to_post(const t_point& point) {
// 	return t_point(
// 		xworld_to_post(point.x),
// 		yworld_to_post(point.y)
// 	);
// }

// static t_bound_box world_to_post(const t_bound_box& box) {
// 	return t_bound_box(
// 		world_to_post(box.bottom_left()),
// 		world_to_post(box.top_right())
// 	);
// }

/* Sets the current graphics context colour to cindex, regardless of whether we think it is 
 * needed or not. 
 */
static void force_setcolor (int cindex) {
	gl_state.foreground_color = predef_colors[cindex];
	update_brushes();
}

static void force_setcolor(const t_color& new_color) {
	gl_state.foreground_color = new_color;
	update_brushes();
}

static void update_brushes() {
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XSetForeground (
			x11_state.display,
			x11_state.current_gc,
			gl_state.foreground_color.as_rgb_int()
		);
		XRenderColor xr_textcolor;
		xr_textcolor.red   = (gl_state.foreground_color.red   << 8);
		xr_textcolor.green = (gl_state.foreground_color.green << 8);
		xr_textcolor.blue  = (gl_state.foreground_color.blue  << 8);
		xr_textcolor.alpha = 0xffff;
		XftColorAllocValue(
			x11_state.display,
			x11_state.visual_info.visual,
			x11_state.colormap_to_use,
			&xr_textcolor,
			&x11_state.xft_currentcolor
		);
#else /* Win32 */
		int win_linestyle, linewidth;
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = convert_to_win_color(gl_state.foreground_color);
		lb.lbHatch = (LONG)NULL;
		win_linestyle = win32_line_styles[gl_state.currentlinestyle];
		linewidth = max (gl_state.currentlinewidth, 1);  // Win32 won't draw 0 width dashed lines.
		
		/* Delete previously used pen */
		if(!DeleteObject(win32_state.hGraphicsPen))  
			WIN32_DELETE_ERROR(); 
		/* Create a new pen */
		win32_state.hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);  
		if(!win32_state.hGraphicsPen)
			WIN32_CREATE_ERROR();
		/* Select created pen into specified device context */
		if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsPen))
			WIN32_SELECT_ERROR();	

		/* Delete previously used brush */
		if(!DeleteObject(win32_state.hGraphicsBrush)) 
			WIN32_DELETE_ERROR();
		/* Create a new brush */
		win32_state.hGraphicsBrush = CreateSolidBrush(
			convert_to_win_color(gl_state.foreground_color)
		);
		if(!win32_state.hGraphicsBrush)
			WIN32_CREATE_ERROR();
		/* Select created brush into specified device context */
		if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsBrush))
			WIN32_SELECT_ERROR();
#endif
	}
	else {
		auto color_index = std::find(
			predef_colors.begin(),
			predef_colors.end(),
			gl_state.foreground_color
		);
		if (color_index != predef_colors.end()) {
			fprintf(gl_state.ps, "%s\n", ps_cnames[color_index - predef_colors.begin()]);
		} else {
			fprintf(
				gl_state.ps,
				"%f %f %f setrgbcolor\n",
				gl_state.foreground_color.red/(float)0xFF,
				gl_state.foreground_color.green/(float)0xFF,
				gl_state.foreground_color.blue/(float)0xFF
			);
		}
	}
}


/* Sets the current graphics context colour to cindex if it differs from the old colour */
void setcolor (int cindex) {
	if (gl_state.foreground_color != predef_colors[cindex]) { 
		force_setcolor (cindex);
	}
}

void setcolor (const t_color& color) {
	if (gl_state.foreground_color != color) {
		force_setcolor(color);
	}
}

void setcolor (uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) {
	setcolor(t_color(r,g,b));
}

/* Sets the current graphics context color to the index that corresponds to the
 * string name passed in.  Slower, but maybe more convenient for simple 
 * client code.
 */
void setcolor_by_name (string cname) {
   int icolor = -1;
   for (int i = 0; i < NUM_COLOR; i++) {
      if (cname == ps_cnames[i]) {
         icolor = i;
         break;
      }
   }
   if (icolor == -1) {
      cout << "Error: unknown color " << cname << endl;
   }
   else {
      setcolor (icolor);
   }   
}


t_color getcolor() {
	return gl_state.foreground_color;
}


/* Sets the current linestyle to linestyle in the graphics context.
 * Note SOLID is 0 and DASHED is 1 for linestyle. 
 */
static void force_setlinestyle (int linestyle) 
{
	gl_state.currentlinestyle = linestyle;
	
	if (gl_state.disp_type == SCREEN) {

#ifdef X11
		static int x_vals[2] = {LineSolid, LineOnOffDash};
		XSetLineAttributes (x11_state.display, x11_state.current_gc, gl_state.currentlinewidth, 
							x_vals[linestyle], CapButt, JoinMiter);
#else  // Win32
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = convert_to_win_color(gl_state.foreground_color);
		lb.lbHatch = (LONG)NULL;
		int win_linestyle = win32_line_styles[linestyle];
		/* Win32 won't draw 0 width dashed lines. */
		int linewidth = max (gl_state.currentlinewidth, 1); 

		/* Delete previously used pen */
		if(!DeleteObject(win32_state.hGraphicsPen))
			WIN32_DELETE_ERROR();
		/* Create a new pen */
		win32_state.hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);
		if(!win32_state.hGraphicsPen)
			WIN32_CREATE_ERROR();
		/* Select created pen into specified device context */
		if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsPen))
			WIN32_SELECT_ERROR();	
#endif
	}

	else {  
		if (linestyle == SOLID) 
		   fprintf (gl_state.ps,"linesolid\n");
		else if (linestyle == DASHED)
			fprintf (gl_state.ps, "linedashed\n");
		else {
			printf ("Error: invalid linestyle: %d\n", linestyle);
			exit (1);
		}
	}
}


/* Change the linestyle in the graphics context only if it differs from the current 
 * linestyle.
 */
void setlinestyle (int linestyle) 
{
	if (linestyle != gl_state.currentlinestyle) 
		force_setlinestyle (linestyle);
}


/* Sets current linewidth in the graphics context.
 * linewidth should be greater than or equal to 0 to make any sense. 
 */
static void force_setlinewidth (int linewidth) 
{	
	gl_state.currentlinewidth = linewidth;
	
	if (gl_state.disp_type == SCREEN) {

#ifdef X11
		static int x_vals[2] = {LineSolid, LineOnOffDash};
		XSetLineAttributes (x11_state.display, x11_state.current_gc, linewidth, 
							x_vals[gl_state.currentlinestyle], CapButt, JoinMiter);
#else /* Win32 */
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = convert_to_win_color(gl_state.foreground_color);
		lb.lbHatch = (LONG)NULL;
		int win_linestyle = win32_line_styles[gl_state.currentlinestyle];

		if (linewidth == 0) 
			linewidth = 1;  // Win32 won't draw dashed 0 width lines.
		
		/* Delete previously used pen */
		if(!DeleteObject(win32_state.hGraphicsPen)) 
			WIN32_DELETE_ERROR();
		/* Create a new pen */
		win32_state.hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);
		if(!win32_state.hGraphicsPen)
			WIN32_CREATE_ERROR();
		/* Select created pen into specified device context */
		if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsPen))
			WIN32_SELECT_ERROR();	
#endif
	}

	else {
		fprintf(gl_state.ps,"%d setlinewidth\n", linewidth);
	}
}


/* Sets the linewidth in the grahpics context, if it differs from the current value.
 */
void setlinewidth (int linewidth) 
{
	if (linewidth != gl_state.currentlinewidth)
		force_setlinewidth (linewidth);
}

/* Force the selected fontsize to be applied to the graphics context, 
 * whether or not it appears to match the current fontsize. This is necessary
 * when switching between postscript and window out, for example.
 * Valid point sizes are between 1 and MAX_FONT_SIZE.
 */
static void force_settextattrs(int pointsize, float degrees) {
	
	if (pointsize < 1) 
		pointsize = 1;
	
	gl_state.currentfontsize = pointsize;
	gl_state.currentfontrotation = degrees;
	
	if (gl_state.disp_type == SCREEN) {
		#ifdef WIN32
			if (win32_state.hGraphicsFont != NULL) {
				/* Delete previously used font */
				if (!DeleteObject(win32_state.hGraphicsFont)) {
					WIN32_DELETE_ERROR();
				}
			}
			win32_state.hGraphicsFont = NULL; // expected by drawtext(..)
		#endif
	} else {
		fprintf(gl_state.ps,"%d setfontsize\n",pointsize);
	}
}


/* For efficiency, these routines doesn't do anything if no change is
 * implied.  If you want to force the graphics context or PS file
 * to have font info set, call force_settextattrs (this is necessary
 * in initialization and screen / Postscript switches).
 */
void setfontsize(int pointsize) {
	if (pointsize != gl_state.currentfontsize) {
		force_settextattrs(pointsize, gl_state.currentfontrotation);
	}
}

int getfontsize() { return gl_state.currentfontsize; }

void settextrotation(float degrees) {
	if (degrees != gl_state.currentfontrotation) {
		force_settextattrs(gl_state.currentfontsize, degrees);
	}
}

float gettextrotation() { return gl_state.currentfontrotation; }

void settextattrs(int pointsize, float degrees) {
	if (degrees != gl_state.currentfontrotation
		|| pointsize != gl_state.currentfontsize) {
		force_settextattrs(pointsize, degrees);
	}
}

/* Puts a triangle in the poly array for button[bnum]. Haven't made this work for 
 * win32 yet and instead put "U", "D" excetra on the arrow buttons.
 * VB To-do: make work for win32 someday.
 */
#ifdef X11
static void setpoly (int bnum, int xc, int yc, int r, float theta) 
{
	int i;
	
	button_state.button[bnum].type = BUTTON_POLY;
	for (i=0;i<3;i++) {
		button_state.button[bnum].poly[i][0] = (int) (xc + r*cos(theta) + 0.5);
		button_state.button[bnum].poly[i][1] = (int) (yc + r*sin(theta) + 0.5);
		theta += (float)(2*PI/3);
	}
}
#endif // X11


/* Maps a button onto the screen and set it up for input, etc.        */
static void map_button (int bnum) 
{
	button_state.button[bnum].ispressed = 0;

	if (button_state.button[bnum].type != BUTTON_SEPARATOR) {
#ifdef X11
		button_state.button[bnum].win = XCreateSimpleWindow(
			x11_state.display,x11_state.menu,
			button_state.button[bnum].xleft,
			button_state.button[bnum].ytop,
			button_state.button[bnum].width,
			button_state.button[bnum].height, 0,
			predef_colors[WHITE].as_rgb_int(),
			predef_colors[LIGHTGREY].as_rgb_int()
		);

		XMapWindow (x11_state.display, button_state.button[bnum].win);
		XSelectInput (x11_state.display, button_state.button[bnum].win, ButtonPressMask);
		button_state.button[bnum].draw = XftDrawCreate(
			x11_state.display,
			button_state.button[bnum].win,
			x11_state.visual_info.visual,
			x11_state.colormap_to_use
		);
#else
		wchar_t* WIN32_wchar_button_text = new wchar_t[BUTTON_TEXT_LEN];
			MultiByteToWideChar(CP_UTF8, 0, button_state.button[bnum].text, -1,
				WIN32_wchar_button_text, BUTTON_TEXT_LEN);
		button_state.button[bnum].hwnd = CreateWindowW(
			L"button",
			WIN32_wchar_button_text,
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			button_state.button[bnum].xleft,
			button_state.button[bnum].ytop,
			button_state.button[bnum].width,
			button_state.button[bnum].height,
			win32_state.hButtonsWnd, (HMENU)(200+bnum),
			(HINSTANCE) GetWindowLong(win32_state.hMainWnd, GWL_HINSTANCE),
			NULL
		);

		delete[] WIN32_wchar_button_text;

		if(!InvalidateRect(win32_state.hButtonsWnd, NULL, TRUE))
			WIN32_DRAW_ERROR();
		if(!UpdateWindow(win32_state.hButtonsWnd))
			WIN32_DRAW_ERROR();
#endif
	}
	else {   // Separator, not a button.
#ifdef X11
		button_state.button[bnum].win = -1;
#else // WIN32
		button_state.button[bnum].hwnd = NULL;
		if(!InvalidateRect(win32_state.hButtonsWnd, NULL, TRUE))
			WIN32_DRAW_ERROR();
		if(!UpdateWindow(win32_state.hButtonsWnd))
			WIN32_DRAW_ERROR();
#endif
	}
}


static void unmap_button (int bnum) 
{
	/* Unmaps (removes) a button from the screen.        */
	if (button_state.button[bnum].type != BUTTON_SEPARATOR) {
#ifdef X11
		XUnmapWindow (x11_state.display, button_state.button[bnum].win);
		XftDrawDestroy(button_state.button[bnum].draw);
#else
		if(!DestroyWindow(button_state.button[bnum].hwnd))
			WIN32_DRAW_ERROR();
		if(!InvalidateRect(win32_state.hButtonsWnd, NULL, TRUE))
			WIN32_DRAW_ERROR();
		if(!UpdateWindow(win32_state.hButtonsWnd))
			WIN32_DRAW_ERROR();
#endif
	}
}


/* Creates a new button below the button containing prev_button_text.       *
* The text and button function are set according to button_text and        *
* button_func, respectively.                                               */
void create_button (const char *prev_button_text , const char *button_text, 
					void (*button_func) (void (*drawscreen) (void))) 
{
	int i, bnum, space, bheight;
	t_button_type button_type = BUTTON_TEXT;

	
	space = 8;
	
	/* Only allow new buttons that are text or separator (not poly) types.                  
    * They can also only go after buttons that are text buttons.
    */
	
	bnum = -1;
      
	for (i=0; i < button_state.num_buttons;i++) {
		if (button_state.button[i].type == BUTTON_TEXT && 
			strcmp (button_state.button[i].text, prev_button_text) == 0) {
			bnum = i + 1;
			break;
		}
	}
	
	if (bnum == -1) {
		printf ("Error in create_button:  button with text %s not found.\n",
			prev_button_text);
		exit (1);
	}
	
	button_state.button = (t_button *) my_realloc (button_state.button, 
												   (button_state.num_buttons+1) * sizeof (t_button));
	/* NB:  Requirement that you specify the button that this button goes under *
	* guarantees that button[num_buttons-2] exists and is a text button.       */
	
   /* Special string to make a separator. */
	if (!strncmp(button_text, "---", 3)) {
		bheight = 2;
		button_type = BUTTON_SEPARATOR;
	}
	else
		bheight = 26;

	for (i=button_state.num_buttons;i>bnum;i--) {
		button_state.button[i].xleft = button_state.button[i-1].xleft;
		button_state.button[i].ytop = button_state.button[i-1].ytop + bheight + space;
		button_state.button[i].height = button_state.button[i-1].height;
		button_state.button[i].width = button_state.button[i-1].width;
		button_state.button[i].type = button_state.button[i-1].type;
		strcpy (button_state.button[i].text, button_state.button[i-1].text);
		button_state.button[i].fcn = button_state.button[i-1].fcn;
		button_state.button[i].ispressed = button_state.button[i-1].ispressed;
		button_state.button[i].enabled = button_state.button[i-1].enabled;
		unmap_button (i-1);
	}

	i = bnum;
	button_state.button[i].xleft = 6;
	button_state.button[i].ytop = button_state.button[i-1].ytop + button_state.button[i-1].height 
									+ space;
	button_state.button[i].height = bheight;
	button_state.button[i].width = 90;
	button_state.button[i].type = button_type;
	strncpy (button_state.button[i].text, button_text, BUTTON_TEXT_LEN);
	button_state.button[i].fcn = button_func;
	button_state.button[i].ispressed = false;
	button_state.button[i].enabled = true;

	button_state.num_buttons++;

	for (; i < button_state.num_buttons; i++)
		map_button (i);
}


/* Destroys the button with text button_text. */
void
destroy_button (const char *button_text) 
{
	int i, bnum, space, bheight;
	
	bnum = -1;
	space = 8;
	for (i = 0; i < button_state.num_buttons; i++) {
		if (button_state.button[i].type == BUTTON_TEXT && 
			strcmp (button_state.button[i].text, button_text) == 0) {
			bnum = i;
			break;
		}
	}
	
	if (bnum == -1) {
		printf ("Error in destroy_button:  button with text %s not found.\n",
			button_text);
		exit (1);
	}

	bheight = button_state.button[bnum].height;

	for (i=bnum+1;i<button_state.num_buttons;i++) {
		button_state.button[i-1].xleft = button_state.button[i].xleft;
		button_state.button[i-1].ytop = button_state.button[i].ytop - bheight - space;
		button_state.button[i-1].height = button_state.button[i].height;
		button_state.button[i-1].width = button_state.button[i].width;
		button_state.button[i-1].type = button_state.button[i].type;
		strcpy (button_state.button[i-1].text, button_state.button[i].text);
		button_state.button[i-1].fcn = button_state.button[i].fcn;
		button_state.button[i-1].ispressed = button_state.button[i].ispressed;
		button_state.button[i-1].enabled = button_state.button[i].enabled;
		unmap_button (i);
	}
	unmap_button(bnum);

	button_state.button = (t_button *) my_realloc (button_state.button, 
												   (button_state.num_buttons-1) * sizeof (t_button));

	button_state.num_buttons--;

	for (i=bnum; i<button_state.num_buttons;i++) 
		map_button (i);
}


/* Open the toplevel window, get the colors, 2 graphics         *
* contexts, load a font, and set up the toplevel window        *
* Calls build_default_menu to set up the default menu.         */
void 
init_graphics (const char *window_name, int cindex) 
{
	init_graphics(window_name, predef_colors[cindex]);
}

void init_graphics (const char *window_name, const t_color& background) {
   if (gl_state.initialized)  // Singleton graphics.
      return;

   reset_common_state ();

   gl_state.disp_type = SCREEN;
   gl_state.background_color = background;

#ifdef X11
   x11_init_graphics (window_name);
#else /* WIN32 */
   win32_init_graphics (window_name);
#endif

   gl_state.initialized = true;
}


static void reset_common_state () {
   gl_state.foreground_color = predef_colors[BLACK];
   gl_state.currentlinestyle = SOLID;
   gl_state.currentlinewidth = 0;
   gl_state.currentfontsize = 12;
   gl_state.currentfontrotation = 0;
   gl_state.current_draw_mode = DRAW_NORMAL;
   gl_state.font_info.clear();
   gl_state.ProceedPressed = false;
   gl_state.get_keypress_input = false;
   gl_state.get_mouse_move_input = false;
   
#ifdef WIN32
   win32_reset_state ();
#endif
}


static void 
update_transform (void) 
{
/* Set up the factors for transforming from the user world to X/Win32 screen
 * (pixel) coordinates.                                                         */
	
	float mult, diff, y1, y2, x1, x2;
	
	/* X Window coordinates go from (0,0) to (width-1,height-1) */
	diff = trans_coord.xright - trans_coord.xleft;
	trans_coord.wtos_xmult = (trans_coord.top_width - 1 - MWIDTH) / diff;
	diff = trans_coord.ybot - trans_coord.ytop;
	trans_coord.wtos_ymult = (trans_coord.top_height - 1 - T_AREA_HEIGHT) / diff;

	/* Need to use same scaling factor to preserve aspect ratio */
	if (fabs(trans_coord.wtos_xmult) <= fabs(trans_coord.wtos_ymult)) {
		mult = (float)(fabs(trans_coord.wtos_ymult/trans_coord.wtos_xmult));
		y1 = trans_coord.ytop - (trans_coord.ybot-trans_coord.ytop)*(mult-1)/2;
		y2 = trans_coord.ybot + (trans_coord.ybot-trans_coord.ytop)*(mult-1)/2;
		trans_coord.ytop = y1;
		trans_coord.ybot = y2;
	}
	else {
		mult = (float)(fabs(trans_coord.wtos_xmult/trans_coord.wtos_ymult));
		x1 = trans_coord.xleft - (trans_coord.xright-trans_coord.xleft)*(mult-1)/2;
		x2 = trans_coord.xright + (trans_coord.xright-trans_coord.xleft)*(mult-1)/2;
		trans_coord.xleft = x1;
		trans_coord.xright = x2;
	}
	diff = trans_coord.xright - trans_coord.xleft;
	trans_coord.wtos_xmult = (trans_coord.top_width - 1 - MWIDTH) / diff;
	diff = trans_coord.ybot - trans_coord.ytop;
	trans_coord.wtos_ymult = (trans_coord.top_height - 1 - T_AREA_HEIGHT) / diff;

	trans_coord.stow_xmult = 1/trans_coord.wtos_xmult;
	trans_coord.stow_ymult = 1/trans_coord.wtos_ymult;
}


static void 
update_ps_transform (void) 
{
	
/* Postscript coordinates start at (0,0) for the lower left hand corner *
* of the page and increase upwards and to the right.  For 8.5 x 11     *
* sheet, coordinates go from (0,0) to (612,792).  Spacing is 1/72 inch.*
* I'm leaving a minimum of half an inch (36 units) of border around    *
	* each edge.                                                           */
	
	float mult, ps_width, ps_height;
	
	ps_width = 540.;    /* 72 * 7.5 */
	ps_height = 720.;   /* 72 * 10  */
	
	trans_coord.ps_xmult = ps_width / (trans_coord.xright - trans_coord.xleft);
	trans_coord.ps_ymult = ps_height / (trans_coord.ytop - trans_coord.ybot);
	/* Need to use same scaling factor to preserve aspect ratio.   *
	* I show exactly as much on paper as the screen window shows, *
	* or the user specifies.                                      */
	if (fabs(trans_coord.ps_xmult) <= fabs(trans_coord.ps_ymult)) {
		trans_coord.ps_left = 36.;
		trans_coord.ps_right = (float)(36. + ps_width);
		mult = fabs(trans_coord.ps_xmult * (trans_coord.ytop - trans_coord.ybot));
		trans_coord.ps_bot = (float)(396. - mult/2);
		mult = fabs(trans_coord.ps_xmult * (trans_coord.ytop - trans_coord.ybot));
		trans_coord.ps_top = (float)(396. + mult/2);
		/* Maintain aspect ratio but watch signs */
		trans_coord.ps_ymult = (trans_coord.ps_xmult*trans_coord.ps_ymult < 0) ?
									-trans_coord.ps_xmult : trans_coord.ps_xmult;
	}
	else {  
		trans_coord.ps_bot = 36.;
		trans_coord.ps_top = (float)(36. + ps_height);
		mult = fabs(trans_coord.ps_ymult * (trans_coord.xright - trans_coord.xleft));
		trans_coord.ps_left = (float)(306. - mult/2);
		mult = fabs(trans_coord.ps_ymult * (trans_coord.xright - trans_coord.xleft));
		trans_coord.ps_right = (float)(306. + mult/2);
		/* Maintain aspect ratio but watch signs */
		trans_coord.ps_xmult = (trans_coord.ps_xmult*trans_coord.ps_ymult < 0) ?
									-trans_coord.ps_ymult : trans_coord.ps_ymult;
	}
}

bool is_droppable_event(
	#ifdef X11
		XEvent* event
	#elif WIN32
		MSG* message
	#endif
	) {
	#ifdef X11
		return
		event->type == MotionNotify || (
			event->type == ButtonPress && (
				event->xbutton.button == Button4 ||
				event->xbutton.button == Button5
			)
		);
	#elif WIN32
		return message->message == WM_MOUSEWHEEL
		||     message->message == WM_MOUSEMOVE;
	#endif
}

/* The program's main event loop.  Must be passed a user routine        
* drawscreen which redraws the screen.  It handles all window resizing 
* zooming etc. itself.  If the user clicks a mousebutton in the graphics    
* (toplevel) area, the act_on_mousebutton routine passed in is called.      
*/
void 
event_loop (void (*act_on_mousebutton)(float x, float y, t_event_buttonPressed button_info),
			void (*act_on_mousemove)(float x, float y), 
			void (*act_on_keypress)(char key_pressed),
			void (*drawscreen) (void)) 
{
#ifdef X11
	x11_event_loop(act_on_mousebutton, act_on_mousemove, act_on_keypress, drawscreen);
#else /* Win32 */
	
	// For event handling with Win32, need to set these file scope function
	// pointers, then dispatch the event and get called via callbacks from
	// Windows (eg. WIN32_GraphicsWND()). Actual event handling is done
	// in these Win32-specific callback functions.

	MSG msg;
	
	win32_mouseclick_ptr = act_on_mousebutton;
	win32_mousemove_ptr = act_on_mousemove;
	win32_keypress_ptr = act_on_keypress;
	win32_drawscreen_ptr = drawscreen;
	gl_state.ProceedPressed = false;
	win32_state.InEventLoop = true;

	win32_invalidate_screen();

	bool dropped_something = false;

	// Windows event dropping explanation:
	// like X11, it will drop events which match is_droppable_event(...), and
	// are followed by another event which matches is_droppable_event(...).
	// However, there is no way to sync events with the system; no way to make
	// sure that this application has all pending events in it's queue to
	// process. So, hopefully this doesn't cause any problems. Also, due to the
	// fact that windows groups multiple fast scrolls into one message, there
	// is logic in win32_handle_mousewheel_zooming to address this.
	while(!gl_state.ProceedPressed && GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		if (msg.message == WM_CHAR) { // only the top window can get keyboard events
			msg.hwnd = win32_state.hMainWnd;
		}
		if (is_droppable_event(&msg)) {
			MSG next_msg;
			if (PeekMessage(&next_msg, NULL, 0, 0, PM_NOREMOVE)) {
				// if the current event is droppable, check if
				// if the next event is droppable.
				if (is_droppable_event(&next_msg)) {
					// if both droppable, skip the event.
					if (msg.message == WM_MOUSEWHEEL) {
						// if dropping scroll event, do the scroll, but don't redraw.
						win32_handle_mousewheel_zooming(msg.wParam, msg.lParam, false);
					}
					dropped_something = true;
					continue;
				}
			}
		}
		DispatchMessage(&msg);
	}
	win32_state.InEventLoop = false;
#endif
}


void 
clearscreen (void) 
{
   t_color savecolor;
   if (gl_state.disp_type == SCREEN) {
#ifdef X11
      XClearWindow (x11_state.display, x11_state.toplevel);
#else /* Win32 */
      savecolor = gl_state.foreground_color;
      setcolor(gl_state.background_color);
      fillrect (trans_coord.xleft, trans_coord.ytop,
    		  	  trans_coord.xright, trans_coord.ybot);
      setcolor(savecolor);
#endif
   }
   else {  // Postscript
     /* erases current page.  Don't use erasepage, since this will erase *
      * everything, (even stuff outside the clipping path) causing       *
      * problems if this picture is incorporated into a larger document. */
      savecolor = gl_state.foreground_color;
      setcolor (gl_state.background_color);
      fprintf(gl_state.ps,"clippath fill\n\n");
      setcolor (savecolor);
   }
}


/* Return 1 if I can quarantee no part of this rectangle will         *
* lie within the user drawing area.  Otherwise return 0.             *
* Note:  this routine is only used to help speed (and to shrink ps   *
* files) -- it will be highly effective when the graphics are zoomed *
* in and lots are off-screen.  I don't have to pre-clip for          *
* correctness.                                                       */
static int 
rect_off_screen (float x1, float y1, float x2, float y2) 
{
	
	float xmin, xmax, ymin, ymax;
	
	xmin = min (trans_coord.xleft, trans_coord.xright);
	if (x1 < xmin && x2 < xmin) 
		return (1);
	
	xmax = max (trans_coord.xleft, trans_coord.xright);
	if (x1 > xmax && x2 > xmax)
		return (1);
	
	ymin = min (trans_coord.ytop, trans_coord.ybot);
	if (y1 < ymin && y2 < ymin)
		return (1);
	
	ymax = max (trans_coord.ytop, trans_coord.ybot);
	if (y1 > ymax && y2 > ymax)
		return (1);
	
	return (0);
}

static int rect_off_screen(const t_bound_box& bbox) {
	return rect_off_screen(bbox.left(), bbox.bottom(), bbox.right(), bbox.top());
}

t_bound_box get_visible_world() {
	return t_bound_box(
		trans_coord.xleft,
		trans_coord.ybot,
		trans_coord.xright,
		trans_coord.ytop
	);
}

bool LOD_screen_area_test(t_bound_box test, float screen_area_threshold) {
	return world_to_scrn(test).area() > screen_area_threshold;
}

void drawline (const t_point& p1, const t_point& p2) {
	drawline(p1.x, p1.y, p2.x, p2.y);
}

void 
drawline (float x1, float y1, float x2, float y2) 
{
/* Draw a line from (x1,y1) to (x2,y2) in the user-drawable area. *
 * Coordinates are in world (user) space.                         */
	
	/* Pre-clipping has been tested on both Windows and Linux, and it was found to be useful *
	 * for speeding up drawscreen() runtime.												 */
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		/* Xlib.h prototype has x2 and y1 mixed up. */ 
		XDrawLine(x11_state.display, x11_state.toplevel, x11_state.current_gc, xworld_to_scrn(x1), 
				  yworld_to_scrn(y1), xworld_to_scrn(x2), yworld_to_scrn(y2));
#else /* Win32 */
		if (!BeginPath(win32_state.hGraphicsDC))
			WIN32_DRAW_ERROR();
		if(!MoveToEx (win32_state.hGraphicsDC, xworld_to_scrn(x1), yworld_to_scrn(y1), NULL))
			WIN32_DRAW_ERROR();
		if(!LineTo (win32_state.hGraphicsDC, xworld_to_scrn(x2), yworld_to_scrn(y2)))
			WIN32_DRAW_ERROR();
		if (!EndPath(win32_state.hGraphicsDC))
			WIN32_DRAW_ERROR();
		if (!StrokePath(win32_state.hGraphicsDC))
			WIN32_DRAW_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps,"%.2f %.2f %.2f %.2f drawline\n",xworld_to_post(x1),yworld_to_post(y1),
			xworld_to_post(x2),yworld_to_post(y2));
	}
}

void drawrect (const t_bound_box& rect) {
	drawrect(rect.bottom_left(), rect.top_right());
}

void drawrect (const t_point& bottomleft, const t_point& upperright) {
	drawrect(bottomleft.x, bottomleft.y, upperright.x, upperright.y);
}

/* (x1,y1) and (x2,y2) are diagonally opposed corners, in world coords. */
void 
drawrect (float x1, float y1, float x2, float y2) 
{
	int xw1, yw1, xw2, yw2;
	
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) { 
		/* translate to X Windows calling convention. */
		xw1 = xworld_to_scrn(x1);
		xw2 = xworld_to_scrn(x2);
		yw1 = yworld_to_scrn(y1);
		yw2 = yworld_to_scrn(y2); 
#ifdef X11
		unsigned int width, height;
		int xl, yt;

		xl = min(xw1,xw2);
		yt = min(yw1,yw2);
		width = abs (xw1-xw2);
		height = abs (yw1-yw2);
		XDrawRectangle(x11_state.display, x11_state.toplevel, 
					   x11_state.current_gc, xl, yt, width, height);
#else /* Win32 */
		if(xw1 > xw2) {
			int temp = xw1;
			xw1 = xw2;
			xw2 = temp;
		}
		if(yw1 > yw2) {
			int temp = yw1;
			yw1 = yw2;
			yw2 = temp;
		}
		
		HBRUSH hOldBrush;

		/* NULL_BRUSH is a Windows stock object which does not fill any space. Thus, a  *
		 * rectangle can be drawn by outlining it with the current pen and not filling  *
		 * it since NULL_BRUSH is used. Another alternative to drawing a rectangle is   *
		 * to draw four lines individually.												*/
		hOldBrush = (HBRUSH)SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_BRUSH));
		if(!(hOldBrush))
			WIN32_SELECT_ERROR();

		if(!Rectangle(win32_state.hGraphicsDC, xw1, yw1, xw2, yw2))
			WIN32_DRAW_ERROR();

		/* Need to restore the previous brush into the specified device context after   *
		 * the rectangle is drawn.													    */
		if(!SelectObject(win32_state.hGraphicsDC, hOldBrush))
			WIN32_SELECT_ERROR();
#endif
		
	}
	else {
		fprintf(gl_state.ps,"%.2f %.2f %.2f %.2f drawrect\n",xworld_to_post(x1),yworld_to_post(y1),
			xworld_to_post(x2),yworld_to_post(y2));
	}
}

void fillrect (const t_bound_box& rect) {
	fillrect(rect.bottom_left(), rect.top_right());
}

void fillrect (const t_point& bottomleft, const t_point& upperright) {
	fillrect(bottomleft.x, bottomleft.y, upperright.x, upperright.y);
}

/* (x1,y1) and (x2,y2) are diagonally opposed corners in world coords. */
void 
fillrect (float x1, float y1, float x2, float y2) 
{
	int xw1, yw1, xw2, yw2;
	
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) {
		/* translate to X Windows calling convention. */
		xw1 = xworld_to_scrn(x1);
		xw2 = xworld_to_scrn(x2);
		yw1 = yworld_to_scrn(y1);
		yw2 = yworld_to_scrn(y2); 
#ifdef X11
		unsigned int width, height;
		int xl, yt;

		xl = min(xw1,xw2);
		yt = min(yw1,yw2);
		width = abs (xw1-xw2);
		height = abs (yw1-yw2);
		XFillRectangle(x11_state.display, x11_state.toplevel, 
					   x11_state.current_gc, xl, yt, width, height);
#else /* Win32 */
		if(xw1 > xw2) {
			int temp = xw1;
			xw1 = xw2;
			xw2 = temp;
		}
		if(yw1 > yw2) {
			int temp = yw1;
			yw1 = yw2;
			yw2 = temp;
		}
		
		if(!Rectangle(win32_state.hGraphicsDC, xw1, yw1, xw2, yw2))
			WIN32_DRAW_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps,"%.2f %.2f %.2f %.2f fillrect\n",xworld_to_post(x1),
			yworld_to_post(y1),xworld_to_post(x2),yworld_to_post(y2));
	}
}


/* Normalizes an angle to be between 0 and 360 degrees. */
static float 
angnorm (float ang) 
{
	int scale;
	
	if (ang < 0) {
		scale = (int) (ang / 360. - 1);
	}
	else {
		scale = (int) (ang / 360.);
	}
	ang = ang - scale * 360;
	return (ang);
}

void drawellipticarc (
	const t_point& center, float radx, float rady, float startang, float angextent) {

	drawellipticarc(center.x, center.y, radx, rady, startang, angextent);
}

void 
drawellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent) 
{
	int xl, yt;
	unsigned int width, height;
	
	/* Conservative (but fast) clip test -- check containing rectangle of *
	 * an ellipse.                                                         */
	
	if (rect_off_screen (xc-radx,yc-rady,xc+radx,yc+rady))
		return;
	
	/* X Windows has trouble with very large angles. (Over 360).    *
	 * Do following to prevent its inaccurate (overflow?) problems. */
	if (fabs(angextent) > 360.) 
		angextent = 360.;
	
	startang = angnorm (startang);
	
	if (gl_state.disp_type == SCREEN) {
		xl = (int) (xworld_to_scrn(xc) - fabs(trans_coord.wtos_xmult*radx));
		yt = (int) (yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady));
		width = (unsigned int) (2*fabs(trans_coord.wtos_xmult*radx));
		height = (unsigned int) (2*fabs(trans_coord.wtos_ymult*rady));
#ifdef X11
		XDrawArc (x11_state.display, x11_state.toplevel, x11_state.current_gc, xl, yt, width, height,
			(int) (startang*64), (int) (angextent*64));
#else  // Win32
        int p1, p2, p3, p4;

		/* set arc direction */
		float startang_rad, endang_rad;
		if (angextent > 0) {
			startang_rad = (float) DEGTORAD(startang);
			endang_rad = (float) DEGTORAD(startang+angextent-.001);
		}
		else {
			startang_rad = (float) DEGTORAD(startang+angextent+.001);
			endang_rad = (float) DEGTORAD(startang);
		}
		p1 = (int)(xworld_to_scrn(xc) + fabs(trans_coord.wtos_xmult*radx)*cos(startang_rad));
		p2 = (int)(yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady)*sin(startang_rad));
		p3 = (int)(xworld_to_scrn(xc) + fabs(trans_coord.wtos_xmult*radx)*cos(endang_rad));
		p4 = (int)(yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady)*sin(endang_rad));

		if(!Arc(win32_state.hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4))
			WIN32_DRAW_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps, "gsave\n");
		fprintf(gl_state.ps, "%.2f %.2f translate\n", xworld_to_post(xc), yworld_to_post(yc));
		fprintf(gl_state.ps, "%.2f 1 scale\n",
					fabs(radx*trans_coord.ps_xmult)/fabs(rady*trans_coord.ps_ymult));
		fprintf(gl_state.ps, "0 0 %.2f %.2f %.2f %s\n", /*xworld_to_post(xc)*/ 
					/*yworld_to_post(yc)*/ fabs(rady*trans_coord.ps_xmult), startang,
					startang+angextent,	(angextent < 0) ? "drawarcn" : "drawarc") ;
		fprintf(gl_state.ps, "grestore\n");
	}
}


/* Startang is relative to the Window's positive x direction.  Angles in degrees.  
 */
void 
drawarc (float xc, float yc, float rad, float startang, 
		 float angextent) 
{
	drawellipticarc(xc, yc, rad, rad, startang, angextent);
}


/* Fills a elliptic arc.  Startang is relative to the Window's positive x   
 * direction.  Angles in degrees.                                           
 */

void fillellipticarc (
	const t_point& center, float radx, float rady, float startang, float angextent) {

	fillellipticarc(center.x, center.y, radx, rady, startang, angextent);
}

void 
fillellipticarc (float xc, float yc, float radx, float rady, float startang, 
		 float angextent) 
{
	int xl, yt;
	unsigned int width, height;
	
	/* Conservative (but fast) clip test -- check containing rectangle of *
	 * a circle.                                                          */
	
	if (rect_off_screen (xc-radx,yc-rady,xc+radx,yc+rady))
		return;
	
    /* X Windows has trouble with very large angles. (Over 360).    *
	 * Do following to prevent its inaccurate (overflow?) problems. */
	
	if (fabs(angextent) > 360.) 
		angextent = 360.;
	
	startang = angnorm (startang);
	
	if (gl_state.disp_type == SCREEN) {
		xl = (int) (xworld_to_scrn(xc) - fabs(trans_coord.wtos_xmult*radx));
		yt = (int) (yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady));
		width = (unsigned int) (2*fabs(trans_coord.wtos_xmult*radx));
		height = (unsigned int) (2*fabs(trans_coord.wtos_ymult*rady));
#ifdef X11
		XFillArc (x11_state.display, x11_state.toplevel, x11_state.current_gc, xl, yt, width, height,
			(int) (startang*64), (int) (angextent*64));
#else  // Win32
		HPEN hOldPen;
		int p1, p2, p3, p4;

		/* set pie direction */
		float startang_rad, endang_rad;
		if (angextent > 0) {
			startang_rad = (float) DEGTORAD(startang);
			endang_rad = (float) DEGTORAD(startang+angextent-.001);
		}
		else {
			startang_rad = (float) DEGTORAD(startang+angextent+.001);
			endang_rad = (float) DEGTORAD(startang);
		}
		p1 = (int)(xworld_to_scrn(xc) + fabs(trans_coord.wtos_xmult*radx)*cos(startang_rad));
		p2 = (int)(yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady)*sin(startang_rad));
		p3 = (int)(xworld_to_scrn(xc) + fabs(trans_coord.wtos_xmult*radx)*cos(endang_rad));
		p4 = (int)(yworld_to_scrn(yc) - fabs(trans_coord.wtos_ymult*rady)*sin(endang_rad));
	
		/* NULL_PEN is a Windows stock object which does not draw anything. Set current *
		 * pen to NULL_PEN in order to fill the arc without drawing the outline.        */
		hOldPen = (HPEN)SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_PEN));
		if(!(hOldPen))
			WIN32_SELECT_ERROR();
		
		// Win32 API says a zero return value indicates an error, but it seems to always
		// return zero.  Don't check for an error on Pie.
        Pie(win32_state.hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4);
		//if(!Pie(win32_state.hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4));
			//WIN32_DRAW_ERROR();
		
		/* Need to restore the original pen into the device context after filling. */
		if(!SelectObject(win32_state.hGraphicsDC, hOldPen))
			WIN32_SELECT_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps, "gsave\n");
		fprintf(gl_state.ps, "%.2f %.2f translate\n", xworld_to_post(xc), yworld_to_post(yc));
		fprintf(gl_state.ps, "%.2f 1 scale\n",
					fabs(radx*trans_coord.ps_xmult)/fabs(rady*trans_coord.ps_ymult));
		fprintf(gl_state.ps, "%.2f %.2f %.2f 0 0 %s\n", /*xworld_to_post(xc)*/ 
					/*yworld_to_post(yc)*/ fabs(rady*trans_coord.ps_xmult), startang,
					startang+angextent, (angextent < 0) ? "fillarcn" : "fillarc") ;
		fprintf(gl_state.ps, "grestore\n");
	}
}

void fillarc (const t_point& center, float rad, float startang, float angextent) { 
	fillellipticarc(center.x, center.y, rad, rad, startang, angextent);
}

void fillarc (float xc, float yc, float rad, float startang, float angextent) {
	fillellipticarc(xc, yc, rad, rad, startang, angextent);
}

void 
fillpoly (t_point *points, int npoints) 
{
#ifdef X11
	XPoint transpoints[MAXPTS];
#else
	POINT transpoints[MAXPTS];
#endif
	int i;
	float xmin, ymin, xmax, ymax;
	
	if (npoints > MAXPTS) {
		printf("Error in fillpoly:  Only %d points allowed per polygon.\n",
			MAXPTS);
		printf("%d points were requested.  Polygon is not drawn.\n",npoints);
		return;
	}
	
	/* Conservative (but fast) clip test -- check containing rectangle of *
	 * polygon.                                                           */
	
	xmin = xmax = points[0].x;
	ymin = ymax = points[0].y;
	
	for (i=1;i<npoints;i++) {
		xmin = min (xmin,points[i].x);
		xmax = max (xmax,points[i].x);
		ymin = min (ymin,points[i].y);
		ymax = max (ymax,points[i].y);
	}
	
	if (rect_off_screen(xmin,ymin,xmax,ymax))
		return;
	
	if (gl_state.disp_type == SCREEN) {
		for (i=0;i<npoints;i++) {
			transpoints[i].x = (long) xworld_to_scrn (points[i].x);
			transpoints[i].y = (long) yworld_to_scrn (points[i].y);
		}
#ifdef X11
		XFillPolygon(x11_state.display, x11_state.toplevel, x11_state.current_gc, 
					 transpoints, npoints, Complex, CoordModeOrigin);
#else
		HPEN hOldPen;
		/* NULL_PEN is a Windows stock object which does not draw anything. Set current *
		 * pen to NULL_PEN in order to fill polygon without drawing its outline.        */
		hOldPen = (HPEN)SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_PEN));
		if(!(hOldPen))
			WIN32_SELECT_ERROR();
		
		if(!Polygon (win32_state.hGraphicsDC, transpoints, npoints))
			WIN32_DRAW_ERROR();

		/* Need to restore the original pen into the device context after drawing. */
		if(!SelectObject(win32_state.hGraphicsDC, hOldPen))
			WIN32_SELECT_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps,"\n");
		
		for (i=npoints-1;i>=0;i--) 
			fprintf (gl_state.ps, "%.2f %.2f\n", xworld_to_post(points[i].x), 
					 yworld_to_post(points[i].y));
		
		fprintf (gl_state.ps, "%d fillpoly\n", npoints);
	}
}

/**
 * drawtext convenience functions
 */

void drawtext_in (const t_bound_box& bbox, const char* text) {
	drawtext(bbox.get_center(), text, bbox);
}

void drawtext_in (const t_bound_box& bbox, const char* text, float tolerance) {
	drawtext(bbox.get_center(), text, bbox, tolerance);
}

void drawtext (
	const t_point& text_center, const char* text, const t_bound_box& bounds, float tolerance) {

	t_point tolerance_pt(tolerance, tolerance);
	t_bound_box tolerance_bounds = bounds;

	tolerance_bounds.bottom_left() -= tolerance_pt;
	tolerance_bounds.top_right() += tolerance_pt;

	drawtext(text_center, text, tolerance_bounds);
}

void drawtext (const t_point& text_center, const char* text, const t_bound_box& bounds) {
	if (bounds.intersects(text_center) != true) {
		printf("the center of the text \"%s\" isn't in it's bounding box", text);
	}
	t_point bottomleft_bounds = text_center - bounds.bottom_left();
	t_point topright_bounds = bounds.top_right() - text_center;
	t_point min_bounds(
		min(bottomleft_bounds.x, topright_bounds.x),
		min(bottomleft_bounds.y, topright_bounds.y)
	);
	min_bounds *= 2;
	drawtext(text_center, text, min_bounds.x, min_bounds.y);
}

void drawtext (const t_point& text_center, const char *text, float boundx, float boundy) {
	drawtext(text_center.x, text_center.y, text, boundx, boundy);
}

/**
 * the real drawtext function.
 * First, using a non rotated font (probably already loaded), it conservatively checks
 * if an approximation of the bbox is on the screen, and not to large for the given
 * bound{x,y}. Then it loads the proper (possibly rotated) font, and checks the real
 * bounds of the text and performs the same checks. Then it draws the text to the
 * XftDraw (X11) or screen (WIN32)
 */
void drawtext (float xc, float yc, const char *text, float boundx, float boundy) 
{
	int text_byte_length = strlen(text);
	float angle = PI*gl_state.currentfontrotation/180;
	float abscos = fabs(cos(angle));
	float abssin = fabs(sin(angle));
	#ifdef WIN32
		wchar_t* WIN32_wchar_text = new wchar_t[text_byte_length+1];
		size_t WIN32_wchar_text_len =
			MultiByteToWideChar(CP_UTF8, 0, text, text_byte_length, WIN32_wchar_text, text_byte_length);
		WIN32_wchar_text[text_byte_length] = 0;
	#endif

	// exit early (conservatively) to prevent filling the font cache with fonts not visible
	if (rect_off_screen(t_bound_box(t_point(xc-boundx/2, yc-boundy/2), boundx, boundy))) {
		// if the largest bbox the text could have is off the screen, don't even load the font.
		return;
	} else {
		// approximate width & height and check against bound{x,y}
		font_ptr zero_font = gl_state.font_info.get_font_info(gl_state.currentfontsize, 0);

#ifdef X11
		XGlyphInfo zero_extents;
		XftTextExtentsUtf8(
			x11_state.display, zero_font, (const FcChar8*)text, text_byte_length, &zero_extents);
		int zero_approx_height = zero_extents.height;
		int zero_approx_width  = zero_extents.width;

#elif WIN32
		HFONT zero_hfont = CreateFontIndirect(zero_font);
		if (!zero_hfont) {
			WIN32_CREATE_ERROR();
		}
		// Select zero font into specified device context
		if (!SelectObject(win32_state.hGraphicsDC, zero_hfont)) {
			WIN32_SELECT_ERROR();
		}

		SIZE textsize;
		if (!GetTextExtentPoint32W(
				win32_state.hGraphicsDC,
				WIN32_wchar_text,
				WIN32_wchar_text_len,
				&textsize)
			) {
			WIN32_DRAW_ERROR();
		}

		int zero_approx_width  = textsize.cx;
		int zero_approx_height = textsize.cy;

		// reselect normal font
		if (win32_state.hGraphicsFont != NULL) {
			if (!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsFont)) {
				WIN32_SELECT_ERROR();
			}
		}

		// eventhough it might be selected, we wont't be drawing with it, because
		// the HFONT in wit32_state is NULL, so it will be set before something
		// tries to draw text. Putting it in a later, more proper, place in this
		// function causes an draw error on font cache cache overflow, for some reason.
		if (zero_hfont != NULL) {
			if (!DeleteObject(zero_hfont)) {
				WIN32_DELETE_ERROR();
			}
		}
#endif
		// conservatively check if the bbox is to large
		if (
			fabs(0.9*
				(abssin*zero_approx_width + abscos*zero_approx_height)*trans_coord.stow_ymult
			) > boundy ||
			fabs(0.9*
				(abscos*zero_approx_width + abssin*zero_approx_height)*trans_coord.stow_xmult
			) > boundx) {

			return;
		}
	}

	int width, height;
	font_ptr current_font = gl_state.font_info.get_font_info(
		gl_state.currentfontsize,
		gl_state.currentfontrotation
	);
#ifdef X11
	XGlyphInfo extents;
	XftTextExtentsUtf8(x11_state.display, current_font, (const FcChar8*)text, text_byte_length, &extents);
	width = extents.width;
	height = extents.height;
#else /* WC : WIN32 */
	if (win32_state.hGraphicsFont == NULL) {
		// if the font isn't already created, create it.
		// note: set to NULL by settextattrs(...) if something changed
		win32_state.hGraphicsFont = CreateFontIndirect(current_font);

		if (!win32_state.hGraphicsFont) {
			WIN32_CREATE_ERROR();
		}
		// Select created font into specified device context
		if (!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsFont)) {
			WIN32_SELECT_ERROR();
		}
	}

	if (SetTextColor(
			win32_state.hGraphicsDC,
			convert_to_win_color(gl_state.foreground_color)
	) == CLR_INVALID) {
		WIN32_DRAW_ERROR();
	}

	SIZE textsize;

	if (!GetTextExtentPoint32W(
		win32_state.hGraphicsDC,
		WIN32_wchar_text,
		WIN32_wchar_text_len,
		&textsize)
		) {
		WIN32_DRAW_ERROR();
	}

	width  = textsize.cx*abscos + textsize.cy*abssin;
	height = textsize.cx*abssin + textsize.cy*abscos;
#endif	

	float wrld_width = width*trans_coord.stow_xmult;
	float wrld_height = height*trans_coord.stow_ymult;

	if ((fabs(wrld_width) > fabs(boundx)) || (fabs(wrld_height) > fabs(boundy))) {
		return; // don't draw if it won't fit in xbound or ybound.
	}

	// These are more-or-less magic offsets. The members of extents are not documented
	// anywhere I could find, and after much experimentation, I determined some of their
	// meaning, and derived offsets to find the true center of the text - ie the center
	// point in the x direction, with the y center being halfway up a normal letter
	// (eg. 'a', ie. nothing below the baseline (like 'p'), or anything extra tall (like 'b')).
	//
	// XGlyphInfo.{x,y} - relative location of the start of the text to the point passed
	//     to XftDrawString*().
	// XGlyphInfo.{x,y}Off - relative location of the end of the text to (x,y); where
	//     the drawing of the next character would start, I believe.
	// XGlyphInfo.{width,height} - these ones mean what they say - ie. width and height
	//     of a bounding rectangle, but note that they will change based on rotation and
	//     whether or not the text contains non short letters like 'p' or 'b').
	//
	// Also, it should be noted that with rotation, 
	// XftFont.{height,descent,ascent,max_advance_width} cannot be trusted.
	// You would have to take those values from an unrotated font.
#ifdef X11
	// these two work almost perfectly, with aligned baselines, but only for the interval [0-90]
	//float X11_xmagicoffset = (extents.width - (extents.x + extents.xOff))*trans_coord.stow_xmult;
	// float X11_ymagicoffset = - (extents.y - extents.height)*trans_coord.stow_ymult;

	// however just leaving them at 0 works good enough for [0-360], however baselines
	// will not be aligned, and text with different heights and the same yc will look bad next
	// to each other. If you would like aligned baselines, use the previous equations, but note
	// their caveats.
	float X11_xmagicoffset = 0;
	float X11_ymagicoffset = 0;
#endif

	t_bound_box text_bbox(
#ifdef X11
		t_point(
			xc + ( -wrld_width  + X11_xmagicoffset) / 2.0,
			yc + ( -wrld_height + X11_ymagicoffset) / 2.0
		),
#elif WIN32
		t_point(xc - wrld_width / 2, yc - wrld_height / 2),
#endif
		wrld_width,
		wrld_height
	);

	if (rect_off_screen(text_bbox)) {
		return;
	}

	// #define SHOW_TEXT_BBOX // useful for debugging text placement.
	#ifdef SHOW_TEXT_BBOX
		drawrect(text_bbox);
		t_color save = getcolor();
		setcolor(RED);

		if (fabs(wrld_width) < fabs(boundx)) {
			drawline(xc, yc - boundy / 2, xc, yc + boundy / 2);
		}
		if (fabs(wrld_height) < fabs(boundy)) {
			drawline(xc - boundx / 2, yc, xc + boundx / 2, yc);
		}

		setcolor(GREEN);
		drawline(xc, yc - wrld_height / 2, xc, yc + wrld_height / 2);
		drawline(xc - wrld_width / 2, yc, xc + wrld_width / 2, yc);
		setcolor(save);
	#endif

	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XftDrawStringUtf8(
			x11_state.toplevel_draw,
			&x11_state.xft_currentcolor,
			current_font,
			// more magic offsets
			xworld_to_scrn(text_bbox.left()) + extents.x,
			yworld_to_scrn(text_bbox.top()) + extents.y - extents.height,
			(const FcChar8*)text,
			text_byte_length
		);
#elif WIN32
		float WIN_xtextoffset = 0;
		float WIN_ytextoffset = 0;
		float normalized_angle = (angle - (int)(angle/(2*PI))*2*PI);
		if (normalized_angle < PI/2) { // quadrant I
			WIN_ytextoffset = textsize.cx*abssin;
		} else if (normalized_angle < PI) { // quadrant II
			WIN_ytextoffset = height;
			WIN_xtextoffset = textsize.cx*abscos;
		} else if (normalized_angle < PI*3/2) { // quadrant III
			WIN_xtextoffset = width;
			WIN_ytextoffset = textsize.cy*abscos;
		} else { // quadrant IV
			WIN_xtextoffset = textsize.cy*abssin;
		}
		SetBkMode(win32_state.hGraphicsDC, TRANSPARENT);
		if (TextOutW(
			win32_state.hGraphicsDC,
			xworld_to_scrn(text_bbox.left()) + WIN_xtextoffset,
			yworld_to_scrn(text_bbox.bottom()) + WIN_ytextoffset, 
			WIN32_wchar_text,
			WIN32_wchar_text_len
		) == 0) {
			WIN32_DRAW_ERROR();
		}

		delete[] WIN32_wchar_text;
#endif
	}
	else {
		fprintf(gl_state.ps, "gsave\n");
		fprintf(gl_state.ps, "%.2f %.2f moveto\n", xworld_to_post(xc),yworld_to_post(yc));
		fprintf(gl_state.ps, "0.8 0.8 scale\n"); // text comes out a little bit bigger in ps than X11
		fprintf(gl_state.ps, "%d rotate\n",  (((int)gl_state.currentfontrotation) % 360));
		fprintf(gl_state.ps, "(%s) 0 0 rcenshow\n", text);
		fprintf(gl_state.ps, "grestore\n");
	}
}


void 
flushinput (void) 
{
	if (gl_state.disp_type != SCREEN) 
		return;
#ifdef X11
	XFlush(x11_state.display);
#endif
}


void init_world(float left, float bottom, float right, float top) {
	init_world(t_bound_box(left,bottom,right,top));
}

void init_world(const t_bound_box& bounds) {
	/* Sets the coordinate system the user wants to draw into.          */
	
	trans_coord.xleft = bounds.left();
	trans_coord.xright = bounds.right();
	trans_coord.ytop = bounds.top();
	trans_coord.ybot = bounds.bottom();
	
	/* Save initial world coordinates to allow full view button
	 * to zoom all the way out.
	 */
	trans_coord.init_xleft = trans_coord.xleft;
	trans_coord.init_xright = trans_coord.xright;
	trans_coord.init_ytop = trans_coord.ytop;
	trans_coord.init_ybot = trans_coord.ybot;
	
	if (gl_state.disp_type == SCREEN) {
		update_transform();
	}
	else {
		update_ps_transform();
	}
}


/* Draw the current message in the text area at the screen bottom. */
void 
draw_message (void) 
{
	int savefontsize;
	t_color savecolor;
	float ylow;
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XClearWindow(x11_state.display, x11_state.textarea);

		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[WHITE].as_rgb_int());
		XDrawRectangle(x11_state.display, x11_state.textarea, x11_state.gc_menus, 0, 0,
						trans_coord.top_width - MWIDTH, T_AREA_HEIGHT);
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[BLACK].as_rgb_int());
		XDrawLine(x11_state.display, x11_state.textarea, x11_state.gc_menus, 0, T_AREA_HEIGHT-1,
					trans_coord.top_width-MWIDTH, T_AREA_HEIGHT-1);
		XDrawLine(x11_state.display, x11_state.textarea, x11_state.gc_menus, 
				  trans_coord.top_width-MWIDTH-1, 0, trans_coord.top_width-MWIDTH-1, 
				  T_AREA_HEIGHT-1);
		
		menutext(
			x11_state.textarea_draw,
			(trans_coord.top_width - MWIDTH)/2,
			T_AREA_HEIGHT / 2,
			gl_state.statusMessage
		);
#else
		if(!InvalidateRect(win32_state.hStatusWnd, NULL, TRUE))
			WIN32_DRAW_ERROR();
		if(!UpdateWindow(win32_state.hStatusWnd))
			WIN32_DRAW_ERROR();
#endif
	}
	
	else {
	/* Draw the message in the bottom margin.  Printer's generally can't  *
		* print on the bottom 1/4" (area with y < 18 in PostScript coords.)  */
		
		savecolor = gl_state.foreground_color;
		setcolor (BLACK);
		savefontsize = gl_state.currentfontsize;
		setfontsize (MENU_FONT_SIZE - 2);  /* Smaller OK on paper */
		ylow = trans_coord.ps_bot - 8;
		fprintf(gl_state.ps,"(%s) %.2f %.2f censhow\n",gl_state.statusMessage,
					(trans_coord.ps_left+trans_coord.ps_right)/2., ylow);
		setcolor (savecolor);
		setfontsize (savefontsize);
	}
}


/* Changes the message to be displayed on screen.   */
void 
update_message (const char *msg) 
{
	strncpy (gl_state.statusMessage, msg, BUFSIZE);
	draw_message ();
#ifdef X11
// Make this appear immediately.  Win32 does that automaticaly.
	XFlush (x11_state.display);  
#endif // X11
}


/* Zooms in menu button pressed. Zoom in at center
 * of graphics area.
 */
static void 
zoom_in (void (*drawscreen) (void)) 
{
	float xcen, ycen;
	
	xcen = (trans_coord.xright + trans_coord.xleft)/2;
	ycen = (trans_coord.ybot + trans_coord.ytop)/2;

	handle_zoom_in(xcen, ycen, drawscreen);
}


/* Zoom out menu button pressed. Zoom out from center
 * of graphics area.
 */
static void 
zoom_out (void (*drawscreen) (void)) 
{
	float xcen, ycen;
	
	xcen = (trans_coord.xright + trans_coord.xleft)/2;
	ycen = (trans_coord.ybot + trans_coord.ytop)/2;

	handle_zoom_out(xcen, ycen, drawscreen);
}


/* Zooms in by a factor of ZOOM_FACTOR */
static void
handle_zoom_in (float x, float y, void (*drawscreen) (void))
{
	//make xright - xleft = 0.6 of the original distance 
	trans_coord.xleft = x - (x-trans_coord.xleft)/(float) ZOOM_FACTOR;
	trans_coord.xright = x + (trans_coord.xright-x)/(float) ZOOM_FACTOR;
	//make ybot - ytop = 0.6 of the original distance
	trans_coord.ytop = y - (y-trans_coord.ytop)/(float) ZOOM_FACTOR;
	trans_coord.ybot = y + (trans_coord.ybot -y)/(float) ZOOM_FACTOR;

	update_transform ();
	if (drawscreen != NULL) {
		drawscreen();
	}
}


/* Zooms out by a factor of ZOOM_FACTOR */
static void
handle_zoom_out (float x, float y, void (*drawscreen) (void))
{
	//restore the original distances before previous zoom in
	trans_coord.xleft = x - (x-trans_coord.xleft)*((float) ZOOM_FACTOR);
	trans_coord.xright = x + (trans_coord.xright-x)*((float) ZOOM_FACTOR);
	trans_coord.ytop = y - (y-trans_coord.ytop)*((float) ZOOM_FACTOR);
	trans_coord.ybot = y + (trans_coord.ybot -y)*((float) ZOOM_FACTOR);

	update_transform ();
	if (drawscreen != NULL) {
		drawscreen();
	}
}


/* Sets the view back to the initial view set by init_world (i.e. a full     *
* view) of all the graphics.                                                */
static void 
zoom_fit (void (*drawscreen) (void)) 
{
	trans_coord.xleft = trans_coord.init_xleft;
	trans_coord.xright = trans_coord.init_xright;
	trans_coord.ytop = trans_coord.init_ytop;
	trans_coord.ybot = trans_coord.init_ybot;
	
	update_transform ();
	drawscreen();
}


/* Moves view 1/2 screen up. */
static void 
translate_up (void (*drawscreen) (void)) 
{
	float ystep;
	
	ystep = (trans_coord.ybot - trans_coord.ytop)/2;
	trans_coord.ytop -= ystep;
	trans_coord.ybot -= ystep;
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen down. */
static void 
translate_down (void (*drawscreen) (void)) 
{
	float ystep;
	
	ystep = (trans_coord.ybot - trans_coord.ytop)/2;
	trans_coord.ytop += ystep;
	trans_coord.ybot += ystep;
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen left. */
static void 
translate_left (void (*drawscreen) (void)) 
{
	
	float xstep;
	
	xstep = (trans_coord.xright - trans_coord.xleft)/2;
	trans_coord.xleft -= xstep;
	trans_coord.xright -= xstep;
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen right. */
static void 
translate_right (void (*drawscreen) (void)) 
{
	float xstep;
	
	xstep = (trans_coord.xright - trans_coord.xleft)/2;
	trans_coord.xleft += xstep;
	trans_coord.xright += xstep;
	update_transform();         
	drawscreen();
}


/* Panning is enabled by pressing and holding down mouse wheel 
 * (or middle mouse button) 
 */
static void
panning_execute (int x, int y, void (*drawscreen) (void))
{
	float x_change_world, y_change_world;

	x_change_world = xscrn_to_world (x) - xscrn_to_world (pan_state.previous_x);
	y_change_world = yscrn_to_world (y) - yscrn_to_world (pan_state.previous_y);
	trans_coord.xleft -= x_change_world;
	trans_coord.xright -= x_change_world;
	trans_coord.ybot -= y_change_world;
	trans_coord.ytop -= y_change_world;

	update_transform();
	drawscreen();

	pan_state.previous_x = x;
	pan_state.previous_y = y;
}


/* Turn panning_enabled on */
static void
panning_on (int start_x, int start_y)
{
	pan_state.previous_x = start_x;
	pan_state.previous_y = start_y;
	pan_state.panning_enabled = true;
}


/* Turn panning_enabled off */
static void
panning_off (void)
{
	pan_state.panning_enabled = false;
}


// Updates the graphics transformation so that the graphics drawn within the
// box (in pixels) defined by x[0],y[0] to x[1],y[1] will be scaled to fill
// the whole window area.
static void 
update_win (int x[2], int y[2], void (*drawscreen)(void)) 
{
	float x1, x2, y1, y2;
	
	x[0] = min(x[0],trans_coord.top_width-MWIDTH);  /* Can't go under menu */
	x[1] = min(x[1],trans_coord.top_width-MWIDTH);
	y[0] = min(y[0],trans_coord.top_height-T_AREA_HEIGHT); /* Can't go under text area */
	y[1] = min(y[1],trans_coord.top_height-T_AREA_HEIGHT);
	
	if ((x[0] == x[1]) || (y[0] == y[1])) {
		printf("Illegal (zero area) window.  Window unchanged.\n");
		return;
	}
	x1 = xscrn_to_world(min(x[0],x[1]));
	x2 = xscrn_to_world(max(x[0],x[1]));
	y1 = yscrn_to_world(min(y[0],y[1]));
	y2 = yscrn_to_world(max(y[0],y[1]));
	trans_coord.xleft = x1;
	trans_coord.xright = x2;
	trans_coord.ytop = y1;
	trans_coord.ybot = y2;
	update_transform();
	drawscreen();
}


/* The window button was pressed.  Let the user click on the two *
* diagonally opposed corners, and zoom in on this area.         */
static void 
adjustwin (void (*drawscreen) (void)) 
{
#ifdef X11	
	
	XEvent report;
	int corner, xold, yold, x[2], y[2];
	
	corner = 0;
	xold = -1;
	yold = -1;    /* Don't need to init yold, but stops compiler warning. */
	
	while (corner<2) {
		XNextEvent (x11_state.display, &report);
		switch (report.type) {
		case Expose:
			if (report.xexpose.count != 0)
				break;
			x11_handle_expose(report, drawscreen);
			if (report.xexpose.window == x11_state.toplevel) {
				xold = -1;   /* No rubber band on screen */
			}
			break;
		case ConfigureNotify:
			x11_handle_configure_notify(report);
			break;
		case ButtonPress:
#ifdef VERBOSE 
			printf("Got a buttonpress.\n");
			printf("Window ID is: %ld.\n",report.xbutton.window);
			printf("Location (%d, %d).\n", report.xbutton.x,
				report.xbutton.y);
#endif
			if (report.xbutton.window != x11_state.toplevel) break;
			x[corner] = report.xbutton.x;
			y[corner] = report.xbutton.y; 
			if (corner == 0) {
			/*	XSelectInput (x11_state.display, x11_state.toplevel, ExposureMask | 
					StructureNotifyMask | ButtonPressMask | PointerMotionMask); */
			}
			else {
				update_win(x,y,drawscreen);
			}
			corner++;
			break;
		case MotionNotify:
			if (corner) {
#ifdef VERBOSE 
				printf("Got a MotionNotify Event.\n");
				printf("x: %d    y: %d\n",report.xmotion.x,report.xmotion.y);
#endif
				if (xold >= 0) {  /* xold set -ve before we draw first box */
                                        // Erase prior box.
					set_draw_mode(DRAW_XOR);
					XDrawRectangle(x11_state.display,x11_state.toplevel, x11_state.gc_xor,
								   min(x[0],xold), min(y[0],yold), abs(x[0]-xold), abs(y[0]-yold));
					set_draw_mode(DRAW_NORMAL);
				}
				/* Don't allow user to window under menu region */
				xold = min(report.xmotion.x,trans_coord.top_width-1-MWIDTH);
				yold = report.xmotion.y;
				set_draw_mode(DRAW_XOR);

                                // Use forcing versions, as we want these modes to apply
                                // to the xor drawing context, and the regular versions
                                // won't update the drawing context if there is no change in line
                                // width etc. (but they might only be on the normal context)
                                force_setlinewidth(1);  
				force_setlinestyle(DASHED);
				force_setcolor(gl_state.background_color);

                                // Draw rubber band box.
				XDrawRectangle(x11_state.display,x11_state.toplevel,x11_state.gc_xor,min(x[0],xold),
					min(y[0],yold),abs(x[0]-xold),abs(y[0]-yold));
				set_draw_mode(DRAW_NORMAL);
			}
			break;
		}
	}
	/* XSelectInput (x11_state.display, x11_state.toplevel, ExposureMask | StructureNotifyMask
		| ButtonPressMask); */
#else /* Win32 */
	/* Implemented as WM_LB... events */
	
	/* Begin window adjust */
	if (win32_state.windowAdjustFlag == WINDOW_DEACTIVATED) {
		win32_state.windowAdjustFlag = WAITING_FOR_FIRST_CORNER_POINT;
	} 
#endif
}


static void 
postscript (void (*drawscreen) (void)) 
{
/* Takes a snapshot of the screen and stores it in pic?.ps.  The *
	* first picture goes in pic1.ps, the second in pic2.ps, etc.    */
	
	static int piccount = 1;
	int success;
	char fname[BUFSIZE];
	
	sprintf(fname,"pic%d.ps",piccount);
	printf("Writing postscript output to file %s\n", fname);
	success = init_postscript (fname);
	
	if (success) {	
		drawscreen();
		close_postscript ();
		piccount++;
	}
	else {
		printf ("Error initializing for postscript output.\n");
#ifdef WIN32
		MessageBoxW(win32_state.hMainWnd, L"Error initializing postscript output.", NULL, MB_OK);
#endif
	}
}


static void 
proceed (void (*drawscreen) (void)) 
{
	(void)drawscreen; // suppress unused warning
	gl_state.ProceedPressed = true;
}


static void 
quit (void (*drawscreen) (void)) 
{
	(void)drawscreen; // suppress unused warning
	close_graphics();
	exit(0);
}


/* Release all my drawing structures (through the X server) and *
* close down the connection.                                   */
void 
close_graphics (void) 
{
	if (!gl_state.initialized)
		return;

	gl_state.font_info.clear();

	for (int i = 0; i < button_state.num_buttons; ++i) {
		unmap_button(i);
	}
	free(button_state.button);
	button_state.button = NULL;
	button_state.num_buttons = 0;

#ifdef X11
	XFreeGC(x11_state.display,x11_state.gc_normal);
	XFreeGC(x11_state.display,x11_state.gc_xor);
	XFreeGC(x11_state.display,x11_state.gc_menus);
	
	XftDrawDestroy(x11_state.toplevel_draw);
	XftDrawDestroy(x11_state.menu_draw);
	XftDrawDestroy(x11_state.textarea_draw);

	x11_state.colormap_to_use = -1; // is free()'d by XCloseDisplay
	memset(&x11_state.visual_info, 0, sizeof(x11_state.visual_info)); // dont need to free this

	XCloseDisplay(x11_state.display);
#elif WIN32
   // Destroy the window
	if(!DestroyWindow(win32_state.hMainWnd))
		WIN32_DRAW_ERROR();

   // free the window class (type information held by MS Windows) 
   // for each of the four window types we created.  Otherwise a 
   // later call to init_graphics to open the graphics window up again
   // will fail.
	if (!UnregisterClassW(szAppName, GetModuleHandle(NULL)) )
		WIN32_DRAW_ERROR();
	if (!UnregisterClassW(szGraphicsName, GetModuleHandle(NULL)) )
		WIN32_DRAW_ERROR();
	if (!UnregisterClassW(szStatusName, GetModuleHandle(NULL)) )
		WIN32_DRAW_ERROR();
	if (!UnregisterClassW(szButtonsName, GetModuleHandle(NULL)) )
		WIN32_DRAW_ERROR();
#endif

	gl_state.initialized = false;
}


/* Opens a file for PostScript output.  The header information,  *
* clipping path, etc. are all dumped out.  If the file could    *
* not be opened, the routine returns 0; otherwise it returns 1. */
int init_postscript (const char *fname) 
{
	gl_state.ps = fopen (fname,"w");
	if (gl_state.ps == NULL) {
		printf("Error: could not open %s for PostScript output.\n",fname);
		printf("Drawing to screen instead.\n");
		return (0);
	}
	gl_state.disp_type = POSTSCRIPT;  /* Graphics go to postscript file now. */
	
	/* Header for minimal conformance with the Adobe structuring convention */
	fprintf(gl_state.ps,"%%!PS-Adobe-1.0\n");
	fprintf(gl_state.ps,"%%%%DocumentFonts: Helvetica\n");
	fprintf(gl_state.ps,"%%%%Pages: 1\n");
	/* Set up postscript transformation macros and page boundaries */
	update_ps_transform();
	/* Bottom margin is at ps_bot - 15. to leave room for the on-screen message. */
	fprintf(gl_state.ps,"%%%%HiResBoundingBox: %.2f %.2f %.2f %.2f\n",
				trans_coord.ps_left, trans_coord.ps_bot - 15.,
				trans_coord.ps_right, trans_coord.ps_top);
	fprintf(gl_state.ps,"%%%%EndComments\n");
	
	fprintf(gl_state.ps,"/censhow   %%draw a centered string\n");
	fprintf(gl_state.ps," { moveto               %% move to proper spot\n");
	fprintf(gl_state.ps,"   dup stringwidth pop  %% get x length of string\n");
	fprintf(gl_state.ps,"   -2 div               %% Proper left start\n");
	fprintf(gl_state.ps,"   yoff rmoveto         %% Move left that much and down half font height\n");
	fprintf(gl_state.ps,"   show newpath } def   %% show the string\n\n"); 

	fprintf(gl_state.ps,"/rcenshow  %%draw a centered string\n"
	                    " { rmoveto              %% move to proper spot\n"
	                    "   dup stringwidth pop  %% get x length of string\n"
	                    "   -2 div               %% Proper left start\n"
	                    "   yoff rmoveto         %% Move left that much and down half font height\n"
	                    "   show newpath } def   %% show the string\n");
	
	fprintf(gl_state.ps,"/setfontsize     %% set font to desired size and compute "
		"centering yoff\n");
	fprintf(gl_state.ps," { /Helvetica findfont\n");
	fprintf(gl_state.ps,"   exch scalefont\n");
	fprintf(gl_state.ps,"   setfont         %% Font size set ...\n\n");
	fprintf(gl_state.ps,"   0 0 moveto      %% Get vertical centering offset\n");
	fprintf(gl_state.ps,"   (Xg) true charpath\n");
	fprintf(gl_state.ps,"   flattenpath pathbbox\n");
	fprintf(gl_state.ps,"   /ascent exch def pop -1 mul /descent exch def pop\n");
	fprintf(gl_state.ps,"   newpath\n");
	fprintf(gl_state.ps,"   descent ascent sub 2 div /yoff exch def } def\n\n");
	
	fprintf(gl_state.ps,"%% Next two lines for debugging only.\n");
	fprintf(gl_state.ps,"/str 20 string def\n");
	fprintf(gl_state.ps,"/pnum {str cvs print (  ) print} def\n");
	
	fprintf(gl_state.ps,"/drawline      %% draw a line from (x2,y2) to (x1,y1)\n");
	fprintf(gl_state.ps," { moveto lineto stroke } def\n\n");
	
	fprintf(gl_state.ps,"/rect          %% outline a rectangle \n");
	fprintf(gl_state.ps," { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
	fprintf(gl_state.ps,"   x1 y1 moveto\n");
	fprintf(gl_state.ps,"   x2 y1 lineto\n");
	fprintf(gl_state.ps,"   x2 y2 lineto\n");
	fprintf(gl_state.ps,"   x1 y2 lineto\n");
	fprintf(gl_state.ps,"   closepath } def\n\n");
	
	fprintf(gl_state.ps,"/drawrect      %% draw outline of a rectanagle\n");
	fprintf(gl_state.ps," { rect stroke } def\n\n");
	
	fprintf(gl_state.ps,"/fillrect      %% fill in a rectanagle\n");
	fprintf(gl_state.ps," { rect fill } def\n\n");
	
	fprintf (gl_state.ps,"/drawarc { arc stroke } def           %% draw an arc\n");
	fprintf (gl_state.ps,"/drawarcn { arcn stroke } def "
		"        %% draw an arc in the opposite direction\n\n");
	
	fprintf (gl_state.ps,"%%Fill a counterclockwise or clockwise arc sector, "
		"respectively.\n");
	fprintf (gl_state.ps,"/fillarc { moveto currentpoint 5 2 roll arc closepath fill } "
		"def\n");
	fprintf (gl_state.ps,"/fillarcn { moveto currentpoint 5 2 roll arcn closepath fill } "
		"def\n\n");
	
	fprintf (gl_state.ps,"/fillpoly { 3 1 roll moveto         %% move to first point\n"
		"   2 exch 1 exch {pop lineto} for   %% line to all other points\n"
		"   closepath fill } def\n\n");	
	
	fprintf(gl_state.ps,"%%Color Definitions:\n");
	fprintf(gl_state.ps,"/white { 1 setgray } def\n");
	fprintf(gl_state.ps,"/black { 0 setgray } def\n");
	fprintf(gl_state.ps,"/grey55 { .55 setgray } def\n");
	fprintf(gl_state.ps,"/grey75 { .75 setgray } def\n");
	fprintf(gl_state.ps,"/blue { 0 0 1 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/green { 0 1 0 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/yellow { 1 1 0 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/cyan { 0 1 1 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/red { 1 0 0 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/pink { 1 0.75 0.8 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/lightpink { 1 0.71 0.76 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/darkgreen { 0 0.5 0 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/magenta { 1 0 1 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/bisque { 1 0.89 0.77 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/lightskyblue { 0.53 0.81 0.98 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/thistle { 0.85 0.75 0.85 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/plum { 0.87 0.63 0.87 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/khaki { 0.94 0.9 0.55 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/coral { 1 0.5 0.31 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/turquoise { 0.25 0.88 0.82 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/mediumpurple { 0.58 0.44 0.86 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/darkslateblue { 0.28 0.24 0.55 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/darkkhaki { 0.74 0.72 0.42 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/lightmediumblue { 0.33 0.33 1.00 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/saddlebrown { 0.55 0.27 0.07 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/firebrick { 0.70 0.13 0.13 setrgbcolor } def\n");
	fprintf(gl_state.ps,"/limegreen { 0.20 0.80 0.20 setrgbcolor } def\n");

	
	fprintf(gl_state.ps,"\n%%Solid and dashed line definitions:\n");
	fprintf(gl_state.ps,"/linesolid {[] 0 setdash} def\n");
	fprintf(gl_state.ps,"/linedashed {[3 3] 0 setdash} def\n");
	
	fprintf(gl_state.ps,"\n%%%%EndProlog\n");
	fprintf(gl_state.ps,"%%%%Page: 1 1\n\n");
	
	/* Set up PostScript graphics state to match current one. */
	force_setcolor (gl_state.foreground_color);
	force_setlinestyle (gl_state.currentlinestyle);
	force_setlinewidth (gl_state.currentlinewidth);
	force_settextattrs(gl_state.currentfontsize, gl_state.currentfontrotation);
	
	/* Draw this in the bottom margin -- must do before the clippath is set */
	draw_message ();
	
	/* Set clipping on page. */
	fprintf(gl_state.ps,"%.2f %.2f %.2f %.2f rect ",trans_coord.ps_left, trans_coord.ps_bot,
				trans_coord.ps_right, trans_coord.ps_top);
	fprintf(gl_state.ps,"clip newpath\n\n");
	
	return (1);
}


/* Properly ends postscript output and redirects output to screen. */
void close_postscript (void) 
{
		
	fprintf(gl_state.ps,"showpage\n");
	fprintf(gl_state.ps,"\n%%%%Trailer\n");
	fclose (gl_state.ps);
	gl_state.disp_type = SCREEN;
	update_transform();   /* Ensure screen world reflects any changes      *
	* made while printing.                          */
	
	/* Need to make sure that we really set up the graphics context.  
	 * The current font set indicates the last font used in a postscript call, 
     * etc., *NOT* the font set in the X11 or Win32 graphics context.  Force the
     * current font, colour etc. to be applied to the graphics context, so 
     * subsequent drawing commands work properly.
     */
	
	force_setcolor (gl_state.foreground_color);
	force_setlinestyle (gl_state.currentlinestyle);
	force_setlinewidth (gl_state.currentlinewidth);
	force_settextattrs(gl_state.currentfontsize, gl_state.currentfontrotation);
}


/* Sets up the default menu buttons on the right hand side of the window. */
static void 
build_default_menu (void) 
{
	int i, xcen, x1, y1, bwid, bheight, space;
	const int NUM_ARROW_BUTTONS = 4, NUM_STANDARD_BUTTONS = 12, SEPARATOR_BUTTON_INDEX = 8;
   	
#ifdef X11
	unsigned long valuemask;
	XSetWindowAttributes menu_attributes;
	
	x11_state.menu = XCreateSimpleWindow(
		x11_state.display,x11_state.toplevel,
		trans_coord.top_width-MWIDTH, 0, MWIDTH,
		trans_coord.display_height, 0,
		predef_colors[BLACK].as_rgb_int(),
		predef_colors[LIGHTGREY].as_rgb_int()
	);

	x11_state.menu_draw = XftDrawCreate(
		x11_state.display,
		x11_state.menu,
		x11_state.visual_info.visual,
		x11_state.colormap_to_use
	);
	menu_attributes.event_mask = ExposureMask;
	/* Ignore button presses on the menu background. */
	menu_attributes.do_not_propagate_mask = ButtonPressMask;
	/* Keep menu on top right */
	menu_attributes.win_gravity = NorthEastGravity; 
	valuemask = CWWinGravity | CWEventMask | CWDontPropagate;
	XChangeWindowAttributes(x11_state.display, x11_state.menu, valuemask, &menu_attributes);
	XMapWindow (x11_state.display, x11_state.menu);
#endif
	
	button_state.button = (t_button *) my_malloc (NUM_STANDARD_BUTTONS * sizeof (t_button));
	
	/* Now do the arrow buttons */
	bwid = 28;
	space = 3;
	y1 = 10;
	xcen = 51;
	x1 = xcen - bwid/2; 
	button_state.button[0].xleft = x1;
	button_state.button[0].ytop = y1;
#ifdef X11
	setpoly (0, bwid/2, bwid/2, bwid/3, -PI/2.); /* Up */
#else
	button_state.button[0].type = BUTTON_TEXT;
#endif
	strcpy(button_state.button[0].text, "U");
	button_state.button[0].fcn = translate_up;
	
	y1 += bwid + space;
	x1 = xcen - 3*bwid/2 - space;
	button_state.button[1].xleft = x1;
	button_state.button[1].ytop = y1;
#ifdef X11
	setpoly (1, bwid/2, bwid/2, bwid/3, PI);  /* Left */
#else
	button_state.button[1].type = BUTTON_TEXT;
#endif
	strcpy(button_state.button[1].text, "L");
	button_state.button[1].fcn = translate_left;
	
	x1 = xcen + bwid/2 + space;
	button_state.button[2].xleft = x1;
	button_state.button[2].ytop = y1;
#ifdef X11
	setpoly (2, bwid/2, bwid/2, bwid/3, 0);  /* Right */
#else
	button_state.button[2].type = BUTTON_TEXT;
#endif
	strcpy(button_state.button[2].text, "R");
	button_state.button[2].fcn = translate_right;
	
	y1 += bwid + space;
	x1 = xcen - bwid/2;
	button_state.button[3].xleft = x1;
	button_state.button[3].ytop = y1;
#ifdef X11
	setpoly (3, bwid/2, bwid/2, bwid/3, +PI/2.);  /* Down */
#else
	button_state.button[3].type = BUTTON_TEXT;
#endif
	strcpy(button_state.button[3].text, "D");
	button_state.button[3].fcn = translate_down;
	
	for (i = 0; i < NUM_ARROW_BUTTONS; i++) {
		button_state.button[i].width = bwid;
		button_state.button[i].height = bwid;
		button_state.button[i].enabled = 1;
	} 
	
	/* Rectangular buttons */
	
	y1 += bwid + space + 6;
	space = 8;
	bwid = 90;
	bheight = 26;
	x1 = xcen - bwid/2;
	for (i = NUM_ARROW_BUTTONS; i < NUM_STANDARD_BUTTONS; i++) {
		button_state.button[i].xleft = x1;
		button_state.button[i].ytop = y1;
		button_state.button[i].type = BUTTON_TEXT;
		button_state.button[i].width = bwid;
		button_state.button[i].enabled = 1;
		if (i != SEPARATOR_BUTTON_INDEX) {
			button_state.button[i].height = bheight;
			y1 += bheight + space;
		}
		else {
			button_state.button[i].height = 2;
			button_state.button[i].type = BUTTON_SEPARATOR;
			y1 += 2 + space;
		}
	}

	strcpy (button_state.button[4].text,"Zoom In");
	strcpy (button_state.button[5].text,"Zoom Out");
	strcpy (button_state.button[6].text,"Zoom Fit");
	strcpy (button_state.button[7].text,"Window");
	strcpy (button_state.button[8].text,"---1");
	strcpy (button_state.button[9].text,"PostScript");
	strcpy (button_state.button[10].text,"Proceed");
	strcpy (button_state.button[11].text,"Exit");
	
	button_state.button[4].fcn = zoom_in;
	button_state.button[5].fcn = zoom_out;
	button_state.button[6].fcn = zoom_fit;
	button_state.button[7].fcn = adjustwin;  // see 'adjustButton' below in WIN32 section
	button_state.button[8].fcn = NULL;
	button_state.button[9].fcn = postscript;
	button_state.button[10].fcn = proceed;
	button_state.button[11].fcn = quit;
	
	for (i = 0; i < NUM_STANDARD_BUTTONS; i++) 
		map_button (i);
	button_state.num_buttons = NUM_STANDARD_BUTTONS;

#ifdef WIN32
	win32_state.adjustButton = 7;
	if(!InvalidateRect(win32_state.hButtonsWnd, NULL, TRUE))
		WIN32_DRAW_ERROR();
	if(!UpdateWindow(win32_state.hButtonsWnd))
		WIN32_DRAW_ERROR();
#endif
}

/**
 * Loads the font with given attributes, and puts the pointer to in in put_font_ptr_here
 */
static font_ptr
do_font_loading(int pointsize, float degrees) {

	bool success = false;
	font_ptr retval = NULL;

#ifdef X11
	for (int ifont = 0; ifont < NUM_FONT_TYPES; ifont++) {

		#ifdef VERBOSE
			printf ("Loading font: %s-%d\n", fontname_config[ifont], pointsize);
		#endif

		XftMatrix font_matrix;
		XftMatrixInit(&font_matrix);
		if (degrees != 0) {
			XftMatrixRotate(&font_matrix, cos(PI*(degrees) / 180), sin(PI*degrees / 180));
		}

		/* Load font and get font information structure. */
		retval = XftFontOpen(
			x11_state.display, x11_state.screen_num,
			XFT_FAMILY, XftTypeString, fontname_config[ifont],
			XFT_SIZE,   XftTypeDouble, (double)pointsize,
			XFT_MATRIX, XftTypeMatrix, &font_matrix,
			NULL // (sentinel)
		);

		if (retval == NULL) {
			#ifdef VERBOSE
				fprintf(stderr, "Cannot open font %s", fontname_config[ifont]);
				if (degrees != 0) { fprintf(stderr, "with rotation %f deg", degrees); }
				printf("\n");
			#endif
		} else {
			success = true;
			break; 
		}
	}
	if (success == false) {
		printf("Error in load_font: fontconfig couldn't find any font of pointsize %d.\n", pointsize);
		if (degrees != 0) { printf("and rotation %f deg", degrees); }
		printf("Use `fc-list` to list available fonts, `fc-match` to test, and then modify\n");
		printf("the font config array in easygl_constants.h .\n");
		exit(1);
	}
#elif WIN32
	LOGFONT *lf = retval = (LOGFONT*)my_malloc(sizeof(LOGFONT));
	ZeroMemory(lf, sizeof(LOGFONT));
	// lfHeight specifies the desired height of characters in logical units.
	// A positive value of lfHeight will request a font that is appropriate
	// for a line spacing of lfHeight. On the other hand, setting lfHeight
	// to a negative value will obtain a font height that is compatible with
	// the desired pointsize.
	lf->lfHeight = -pointsize;
	lf->lfWeight = FW_NORMAL;
	lf->lfCharSet = ANSI_CHARSET;
	lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf->lfQuality = PROOF_QUALITY;
	lf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	lf->lfEscapement = degrees * 10;
	lf->lfOrientation = degrees * 10;
	HFONT testfont;
	for (int ifont = 0; ifont < NUM_FONT_TYPES; ++ifont) {
		MultiByteToWideChar(CP_UTF8, 0, fontname_config[ifont], -1,
			lf->lfFaceName, sizeof(lf->lfFaceName)/sizeof(wchar_t));

		testfont = CreateFontIndirect(lf);
		if(testfont == NULL) {
			#ifdef VERBOSE
				fprintf(stderr, "Couldn't open font %s in pointsize %d.\n",
					fontname_config[ifont], pointsize);
			#endif
			continue;
		}

		if(DeleteObject(testfont) == 0) {
			WIN32_DELETE_ERROR();
		} else {
			success = true;
			break;
		}
	}
	if (success == false) {
		printf("Error in load_font: Windows couldn't find any font of pointsize %d.\n", pointsize);
		printf("check installed fonts, and then modify\n");
		printf("the font config array in easygl_constants.h .\n");
		exit(1);
	}
#endif
	return retval;
}

static void close_font(font_ptr font) {
	#ifdef X11
		XftFontClose(x11_state.display, font);
	#elif WIN32
		free(font);
	#endif
}

/* Return information useful for debugging.
 * Used to return the top-level window object too, but that made graphics.h
 * export all windows and X11 headers to the client program, so VB deleted 
 * that object (mainwnd) from this structure.
 */
void report_structure(t_report *report) {
	report->xmult = trans_coord.wtos_xmult;
	report->ymult = trans_coord.wtos_ymult;
	report->xleft = trans_coord.xleft;
	report->xright = trans_coord.xright;
	report->ytop = trans_coord.ytop;
	report->ybot = trans_coord.ybot;
	report->ps_xmult = trans_coord.ps_xmult;
	report->ps_ymult = trans_coord.ps_ymult;
	report->top_width = trans_coord.top_width;
	report->top_height = trans_coord.top_height;
}


void set_mouse_move_input (bool enable) {
	gl_state.get_mouse_move_input = enable;
}


void set_keypress_input (bool enable) {
	gl_state.get_keypress_input = enable;
}


void enable_or_disable_button (int ibutton, bool enabled) {

   if (button_state.button[ibutton].type != BUTTON_SEPARATOR) {
      button_state.button[ibutton].enabled = enabled;
#ifdef WIN32
      EnableWindow(button_state.button[ibutton].hwnd, button_state.button[ibutton].enabled);
#else  // X11
      x11_drawbut(ibutton);
      XFlush(x11_state.display);
#endif
   }
}


void set_draw_mode (enum e_draw_mode draw_mode) {
/* Set normal (overwrite) or xor (useful for rubber-banding)
 * drawing mode.
 */

   if (draw_mode == DRAW_NORMAL) {
#ifdef X11
      x11_state.current_gc = x11_state.gc_normal;
#else
      if (!SetROP2(win32_state.hGraphicsDC, R2_COPYPEN))
         WIN32_SELECT_ERROR();
#endif
   }
   else {  // DRAW_XOR
#ifdef X11
      x11_state.current_gc = x11_state.gc_xor;
#else
      if (!SetROP2(win32_state.hGraphicsDC, R2_XORPEN))
         WIN32_SELECT_ERROR();
#endif
   }
   gl_state.current_draw_mode = draw_mode;
}


void change_button_text(const char *button_name, const char *new_button_text) {
/* Change the text on a button with button_name to new_button_text.
 * Not a strictly necessary function, since you could intead just 
 * destroy button_name and make a new buton.
 */
	int i, bnum;
	
	bnum = -1;
	
	for (i=4;i<button_state.num_buttons;i++) {
		if (button_state.button[i].type == BUTTON_TEXT && 
			strcmp (button_state.button[i].text, button_name) == 0) {
			bnum = i;
			break;
		}
	}

	if (bnum != -1) {
		strncpy (button_state.button[i].text, new_button_text, BUTTON_TEXT_LEN);
#ifdef X11
		x11_drawbut (i);
#else // Win32
		wchar_t* WIN32_wchar_button_text = new wchar_t[BUTTON_TEXT_LEN];
		MultiByteToWideChar(CP_UTF8, 0, new_button_text, -1,
			WIN32_wchar_button_text, BUTTON_TEXT_LEN);

		SetWindowTextW(button_state.button[bnum].hwnd, WIN32_wchar_button_text);

		delete[] WIN32_wchar_button_text;
#endif
	}
}

/******************************************
 * begin FontCache function definitions *
 ******************************************/

font_ptr FontCache::get_font_info(size_t pointsize, float degrees) {
	if (degrees == 0) {
		return get_font_info(
			pointsize, degrees,
			order_zeros, descriptor2font_zeros, FONT_CACHE_SIZE_FOR_ZEROS
		);
	} else {
		return get_font_info(
			pointsize, degrees,
			order_rotated, descriptor2font_rotated, FONT_CACHE_SIZE_FOR_ROTATED
		);
	}
}

void FontCache::clear() {
	for(
		auto iter = descriptor2font_zeros.begin();
		iter != descriptor2font_zeros.end();
		++iter
	) {
		close_font(iter->second);
	}
	order_zeros.clear();
	descriptor2font_zeros.clear();

	for(
		auto iter = descriptor2font_rotated.begin();
		iter != descriptor2font_rotated.end();
		++iter
	) {
		close_font(iter->second);
	}
	order_rotated.clear();
	descriptor2font_rotated.clear();
}

template<class queue_type, class map_type>
font_ptr FontCache::get_font_info(
	size_t pointsize, float degrees,
	queue_type& orderqueue, map_type& descr2font_map, size_t max_size) {

	auto search_result = descr2font_map.find(std::make_pair(pointsize,degrees));
	if (search_result == descr2font_map.end()) {

		if (orderqueue.size() + 1 > max_size) {
			// if too many fonts, remove the oldest font from the cache.
			font_descriptor fontdesc_to_remove = orderqueue.back();
			auto font_to_remove = descr2font_map.find(fontdesc_to_remove);

			close_font(font_to_remove->second);

			descr2font_map.erase(font_to_remove);
			orderqueue.pop_back();
		}

		font_ptr new_font = do_font_loading(pointsize, degrees);
		font_descriptor new_font_desc = std::make_pair(pointsize,degrees);
		orderqueue.push_front(new_font_desc);
		descr2font_map.insert(std::make_pair(new_font_desc, new_font));
		return new_font;
	} else {
		return search_result->second;
	}
}


/**********************************
* X-Windows Specific Definitions *
*********************************/
#ifdef X11

/* Helper function called by init_graphics(). Not visible to client program. */
static void x11_init_graphics (const char *window_name)
{
   char *display_name = NULL;
   int x, y;                       /* window position */
   unsigned int border_width = 2;  /* ignored by OpenWindows */
   XTextProperty windowName;

   unsigned long valuemask = 0; /* ignore XGCvalues and use defaults */
   XGCValues values;
   XEvent event;

   /* connect to X server */
   if ( (x11_state.display=XOpenDisplay(display_name)) == NULL )
   {
      fprintf( stderr, "Cannot connect to X server %s\n",
   		XDisplayName(display_name));
      exit( -1 );
   }
	
   /* get screen size from display structure macro */
   x11_state.screen_num = DefaultScreen(x11_state.display);
   trans_coord.display_width = DisplayWidth(x11_state.display, x11_state.screen_num);
   trans_coord.display_height = DisplayHeight(x11_state.display, x11_state.screen_num);
   
   x = 0;
   y = 0;
	
   trans_coord.top_width = 2 * trans_coord.display_width / 3;
   trans_coord.top_height = 4 * trans_coord.display_height / 5;
   
	// select a 24 bit TrueColor visual. Note that setting this visual
	// for the top level window will make all children inherit it.
	if (XMatchVisualInfo(
			x11_state.display,
			x11_state.screen_num,
			24,	TrueColor,
			&x11_state.visual_info) == 0) {
		fprintf(stderr, "Cannot handle a non 24-bit TrueColour visual\n");
		exit(-1);
	}

	if (x11_state.visual_info.bits_per_rgb != 8 ||
		x11_state.visual_info.red_mask   != 0xFF0000 ||
		x11_state.visual_info.green_mask != 0x00FF00 ||
		x11_state.visual_info.blue_mask  != 0x0000FF) {
		fprintf(stderr, "Cannot handle strange 24-bit TrueColor visual\n");
		exit(-1);
	}

	Window root_window = XDefaultRootWindow(x11_state.display);

	x11_state.colormap_to_use = XCreateColormap(
		x11_state.display,
		root_window,
		x11_state.visual_info.visual,
		AllocNone
	);

	XSetWindowAttributes attrs;
	attrs.colormap = x11_state.colormap_to_use;
	attrs.border_pixel = predef_colors[BLACK].as_rgb_int();
	attrs.background_pixel = gl_state.background_color.as_rgb_int();

	x11_state.toplevel = XCreateWindow(
		x11_state.display,
		root_window,
		x, y,
		trans_coord.top_width, trans_coord.top_height,
		border_width,
		x11_state.visual_info.depth,
		InputOutput,
		x11_state.visual_info.visual,
		CWBackPixel | CWColormap | CWBorderPixel,
		&attrs
	);

	x11_state.toplevel_draw = XftDrawCreate(
		x11_state.display,
		x11_state.toplevel,
		x11_state.visual_info.visual,
		x11_state.colormap_to_use
	);
	
	XRenderColor xr_textcolor;
	xr_textcolor.red = 0x0;
	xr_textcolor.green = 0x0;
	xr_textcolor.blue = 0x0;
	xr_textcolor.alpha = 0xffff;
	XftColorAllocValue(
		x11_state.display,
		DefaultVisual(x11_state.display, x11_state.screen_num),
		DefaultColormap( x11_state.display, x11_state.screen_num),
		&xr_textcolor,
		&x11_state.xft_menutextcolor
	);

   XSelectInput (x11_state.display, x11_state.toplevel, ExposureMask | StructureNotifyMask |
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                    KeyPressMask);
	
   /* Create default Graphics Contexts.  valuemask = 0 -> use defaults. */
   x11_state.current_gc = x11_state.gc_normal = XCreateGC(x11_state.display, x11_state.toplevel, 
												   valuemask, &values);
   x11_state.gc_menus = XCreateGC(x11_state.display, x11_state.toplevel, valuemask, &values);
	
   /* Create XOR graphics context for Rubber Banding */
   values.function = GXxor;   
   values.foreground = gl_state.background_color.as_rgb_int();
   x11_state.gc_xor = XCreateGC(x11_state.display, x11_state.toplevel, (GCFunction | GCForeground),
                     &values);
	
   /* Set drawing defaults for user-drawable area.  Use whatever the *
   * initial values of the current stuff was set to.                */
   force_settextattrs(gl_state.currentfontsize, gl_state.currentfontrotation);
   force_setcolor (gl_state.foreground_color);
   force_setlinestyle (gl_state.currentlinestyle);
   force_setlinewidth (gl_state.currentlinewidth);
	
   // Need a non-const name to pass to XStringListTo... 
   // (even though X11 won't change it).
   char *window_name_copy = (char *) my_malloc (BUFSIZE * sizeof (char));
   strncpy (window_name_copy, window_name, BUFSIZE);
   XStringListToTextProperty(&window_name_copy, 1, &windowName);
   free (window_name_copy);
   window_name_copy = NULL;

   XSetWMName (x11_state.display, x11_state.toplevel, &windowName);
   /* XSetWMIconName (x11_state.display, x11_state.toplevel, &windowName); */
	
   /* XStringListToTextProperty copies the window_name string into            *
    * windowName.value.  Free this memory now.                                */
	
   free (windowName.value);  
	
   XMapWindow (x11_state.display, x11_state.toplevel);
   x11_build_textarea ();
   build_default_menu ();
	
   /* The following is completely unnecessary if the user is using the       *
    * interactive (event_loop) graphics.  It waits for the first Expose      *
    * event before returning so that I can tell the window manager has got   *
    * the top-level window up and running.  Thus the user can start drawing  *
    * into this window immediately, and there's no danger of the window not  *
    * being ready and output being lost.                                     */
   XPeekIfEvent (x11_state.display, &event, x11_test_if_exposed, NULL); 
   force_settextattrs(gl_state.currentfontsize, gl_state.currentfontrotation);
}

/* Helper function called by event_loop(). Not visible to client program. */
static void 
x11_event_loop (void (*act_on_mousebutton)(float x, float y, t_event_buttonPressed button_info),
				void (*act_on_mousemove)(float x, float y), 
				void (*act_on_keypress)(char key_pressed),
				void (*drawscreen) (void))
{
	XEvent report;
	unsigned int last_skipped_button_press_button = -1;
	int bnum;
	float x, y;
	
#define OFF 1
#define ON 0
	
	x11_turn_on_off (ON);
	while (true) {

		// X11 event dropping explanation:
		// this will drop events which match is_droppable_event(...), and are
		// followed by another event which matches is_droppable_event(...).
		// There are appropriate calls to XSync to make sure that this application
		// has all of it's pending events in it's queue. In X11, scrolls are
		// like button presses - they have press *and release* events. There is
		// logic to ignore the release event in the case where it's already dropped
		// the press one.
		XSync(x11_state.display, false);
		XNextEvent (x11_state.display, &report);
		if (is_droppable_event(&report) && XQLength(x11_state.display) > 0) {
			if (report.type == ButtonPress) {
				last_skipped_button_press_button = report.xbutton.button;
			}
			// if the current event is droppable, and there are more events
			// in the queue, check to see if the next event is droppable too.
			XEvent next_event;
			XPeekEvent(x11_state.display, &next_event);
			// if the next event is a matching ButtonRelease, then drop
			// it too, but only if the queue has more events still.
			if (next_event.type == ButtonRelease 
				&& next_event.xbutton.button == last_skipped_button_press_button
				&& XQLength(x11_state.display) > 1) {

				XSync(x11_state.display, false);
				XNextEvent(x11_state.display, &next_event);
				XPeekEvent(x11_state.display, &next_event);
				last_skipped_button_press_button = -1;
			}
			if (is_droppable_event(&next_event)) {
				// if so, skip this event.
				if (report.type == ButtonPress) {
					x = xscrn_to_world(report.xbutton.x);
					y = yscrn_to_world(report.xbutton.y);
					switch (report.xbutton.button) {
						case Button4:
							handle_zoom_in(x, y, NULL); // (same function as normal event logic)
						break;
						case Button5:
							handle_zoom_out(x, y, NULL); // (same function as normal event logic)
						break;
						default:
							// do nothing, also should be impossible
							// (we don't want to skip mouse presses)
						break;
					}
				}
				continue;
			}
		}

		switch (report.type) {  
		case Expose:
			if (report.xexpose.count != 0)
				break;
			x11_handle_expose(report, drawscreen);
			break;
		case ConfigureNotify:
			x11_handle_configure_notify(report);
			break; 
		case ButtonPress:
#ifdef VERBOSE 
			printf("Got a buttonpress.\n");
			printf("Window ID is: %ld.\n",report.xbutton.window);
			printf("Button pressed is: %d.\n(left click is 1; right click is 3; "
					"scroll wheel click is 2; scroll wheel forward rotate is 4; "
					"scroll wheel backward is 5.)\n",report.xbutton.button);
			printf("Mask is: %d.\n",report.xbutton.state);
#endif
			if (report.xbutton.window == x11_state.toplevel) {
				x = xscrn_to_world(report.xbutton.x);
				y = yscrn_to_world(report.xbutton.y);

				t_event_buttonPressed button_info; 
				/* t_event_buttonPressed is used as a structure for storing information about a	  *
				 * button press event. This information can be passed back to and used by a client*
				 * program.                                                                      */
				x11_handle_button_info(&button_info, report.xbutton.button, report.xbutton.state);
#ifdef VERBOSE
				if (button_info.shift_pressed == true) {
					printf("Shift is pressed at button press.\n");
				}
				if (button_info.ctrl_pressed == true) {
					printf("Ctrl is pressed at button press.\n");
				}
#endif
				switch (report.xbutton.button) {
				case Button1:  /* Left mouse click; pass back to client program */
				case Button3:  /* Right mouse click; pass back to client program */
					/* Pass information about the button press to client program */
					act_on_mousebutton (x, y, button_info);
					break;
				case Button2:  /* Scroll wheel pressed; start panning */
					panning_on(report.xbutton.x, report.xbutton.y);
					break;
				case Button4:  /* Scroll wheel rotated forward; screen does zoom_in */
					// note, this is also called in the skipping logic
					handle_zoom_in(x, y, drawscreen);
					break;
				case Button5:  /* Scroll wheel rotated backward; screen does zoom_out */
					// note, this is also called in the skipping logic
					handle_zoom_out(x, y, drawscreen);
					break;
				}
			} 
			else {  /* A menu button was pressed. */
				bnum = x11_which_button(report.xbutton.window);
#ifdef VERBOSE 
				printf("Button number is %d\n",bnum);
#endif
				if (button_state.button[bnum].enabled) {
					button_state.button[bnum].ispressed = 1;
					x11_drawbut(bnum);
					XFlush(x11_state.display);  /* Flash the button */
					button_state.button[bnum].fcn (drawscreen);
					button_state.button[bnum].ispressed = 0;
					x11_drawbut(bnum);
					if (button_state.button[bnum].fcn == proceed) {
						x11_turn_on_off(OFF);
						flushinput ();
						return;  /* Rather clumsy way of returning *
						* control to the simulator       */
					}
				}
			}
			break;
		case ButtonRelease:
#ifdef VERBOSE
			printf("Got a buttonrelease.\n");
			printf("Window ID is: %ld.\n",report.xbutton.window);
			printf("Button released is: %d.\n(left click is 1; right click is 3; "
					"scroll wheel click is 2; scroll wheel forward rotate is 4; "
					"scroll wheel backward is 5.)\n",report.xbutton.button);
			printf("Mask is: %d.\n",report.xbutton.state);
#endif
			switch (report.xbutton.button) {
			case Button2:  /* Scroll wheel released; stop panning */
				panning_off();
				break;
			}
		case MotionNotify:
#ifdef VERBOSE 
			printf("Got a MotionNotify Event.\n");
			printf("x: %d    y: %d\n",report.xmotion.x,report.xmotion.y);
#endif
			if (pan_state.panning_enabled)
				panning_execute(report.xmotion.x, report.xmotion.y, drawscreen);
			else if (gl_state.get_mouse_move_input &&
			    report.xmotion.x <= trans_coord.top_width-MWIDTH &&
			    report.xmotion.y <= trans_coord.top_height-T_AREA_HEIGHT)
				act_on_mousemove(xscrn_to_world(report.xmotion.x), yscrn_to_world(report.xmotion.y));
			break;
		case KeyPress:
#ifdef VERBOSE 
			printf("Got a KeyPress Event.\n");
#endif
			if (gl_state.get_keypress_input)
			{
			     char      keyb_buffer[20];
			     XComposeStatus composestatus;
			     KeySym         keysym;
			     int       length, max_bytes;

			     max_bytes = 1;

			     length = XLookupString( &report.xkey, keyb_buffer, max_bytes, &keysym,
                       				         &composestatus );

			     keyb_buffer[length] = '\0';   /* terminating NULL */
			     act_on_keypress(keyb_buffer[0]);
			}

			break;
		}
	}
}


/* Creates a small window at the top of the graphics area for text messages */
static void x11_build_textarea (void) 
{
	XSetWindowAttributes menu_attributes;
	unsigned long valuemask;
	
	x11_state.textarea = XCreateSimpleWindow(x11_state.display,x11_state.toplevel, 0,
		trans_coord.top_height-T_AREA_HEIGHT, trans_coord.display_width-MWIDTH,
		T_AREA_HEIGHT, 0, predef_colors[BLACK].as_rgb_int(), predef_colors[LIGHTGREY].as_rgb_int());

	x11_state.textarea_draw = XftDrawCreate(
		x11_state.display,
		x11_state.textarea,
		x11_state.visual_info.visual,
		x11_state.colormap_to_use
	);

	menu_attributes.event_mask = ExposureMask;
	/* ButtonPresses in this area are ignored. */
	menu_attributes.do_not_propagate_mask = ButtonPressMask;
	/* Keep text area on bottom left */
	menu_attributes.win_gravity = SouthWestGravity; 
	valuemask = CWWinGravity | CWEventMask | CWDontPropagate;
	XChangeWindowAttributes(x11_state.display, x11_state.textarea, valuemask, &menu_attributes);
	XMapWindow (x11_state.display, x11_state.textarea);
}


/* Returns True if the event passed in is an exposure event. Note that 
 * the bool type returned by this function is defined in Xlib.h.         
 */
static Bool x11_test_if_exposed (Display *disp, XEvent *event_ptr, XPointer dummy) 
{
	(void)disp;  // suppress unused warning
	(void)dummy; // suppress unused warning
	if (event_ptr->type == Expose) {
		return (True);
	}
	
	return (False);
}


/* draws UTF-8 text center at xc, yc -- used only by menu and button drawing stuff */
static void menutext(XftDraw* draw, int xc, int yc, const char *text) {
	int len, width;

	font_ptr menu_font = gl_state.font_info.get_font_info(MENU_FONT_SIZE, 0);
	
	len = strlen(text);
	XGlyphInfo extents;
	XftTextExtentsUtf8(
		x11_state.display,
		menu_font,
		(const FcChar8*)text,
		len,
		&extents
	);
	width = extents.width;

	XftDrawStringUtf8(
		draw,
		&x11_state.xft_menutextcolor,
		menu_font,
		xc - width/2,
		yc + (menu_font->ascent
			- menu_font->descent) / 2,
		(const FcChar8*) text,
		len
	);
}


/* Draws button bnum in either its pressed or unpressed state.    */
static void x11_drawbut (int bnum) 
{
	int width, height, thick, i, ispressed;
	XPoint mypoly[6];

	width = button_state.button[bnum].width;
	height = button_state.button[bnum].height;

	if (button_state.button[bnum].type == BUTTON_SEPARATOR) {
		int x,y;

		x = button_state.button[bnum].xleft;
		y = button_state.button[bnum].ytop;
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[WHITE].as_rgb_int());
		XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, x, y+1, x+width, y+1);
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[BLACK].as_rgb_int());
		XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, x, y, x+width, y);
		return;
	}

	ispressed = button_state.button[bnum].ispressed;
	thick = 2;
	/* Draw top and left edges of 3D box. */
	if (ispressed) {
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[BLACK].as_rgb_int());
	}
	else {
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[WHITE].as_rgb_int());
	}
	
	/* Note:  X Windows doesn't appear to draw the bottom pixel of *
	* a polygon with XFillPolygon, so I make this 1 pixel thicker *
	* to compensate.                                              */
	mypoly[0].x = 0;
	mypoly[0].y = height;
	mypoly[1].x = 0;
	mypoly[1].y = 0;
	mypoly[2].x = width;
	mypoly[2].y = 0;
	mypoly[3].x = width-thick;
	mypoly[3].y = thick;
	mypoly[4].x = thick;
	mypoly[4].y = thick;
	mypoly[5].x = thick;
	mypoly[5].y = height-thick;
	XFillPolygon(x11_state.display,button_state.button[bnum].win,x11_state.gc_menus,mypoly,6,Convex,
		CoordModeOrigin);
	
	/* Draw bottom and right edges of 3D box. */
	if (ispressed) {
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[WHITE].as_rgb_int());
	}
	else {
		XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[BLACK].as_rgb_int());
	} 
	mypoly[0].x = 0;
	mypoly[0].y = height;
	mypoly[1].x = width;
	mypoly[1].y = height;
	mypoly[2].x = width;
	mypoly[2].y = 0;
	mypoly[3].x = width-thick;
	mypoly[3].y = thick;
	mypoly[4].x = width-thick;
	mypoly[4].y = height-thick;
	mypoly[5].x = thick;
	mypoly[5].y = height-thick;
	XFillPolygon(x11_state.display,button_state.button[bnum].win,x11_state.gc_menus,mypoly,6,Convex,
		CoordModeOrigin);
	
	/* Draw background */
	if (ispressed) {
		XSetForeground(x11_state.display, x11_state.gc_menus,
			predef_colors[DARKGREY].as_rgb_int()
		);
	} else {
		XSetForeground(x11_state.display, x11_state.gc_menus,
			predef_colors[LIGHTGREY].as_rgb_int()
		);
	}
	
	/* Give x,y of top corner and width and height */
	XFillRectangle (x11_state.display,button_state.button[bnum].win,x11_state.gc_menus,thick,thick,
		width-2*thick, height-2*thick);
	
	/* Draw polygon, if there is one */
	if (button_state.button[bnum].type == BUTTON_POLY) {
		for (i=0;i<3;i++) {
			mypoly[i].x = button_state.button[bnum].poly[i][0];
			mypoly[i].y = button_state.button[bnum].poly[i][1];
		}
		XSetForeground(x11_state.display, x11_state.gc_menus,
			predef_colors[BLACK].as_rgb_int()
		);
		XFillPolygon(x11_state.display,button_state.button[bnum].win,
					 x11_state.gc_menus,mypoly,3,Convex,CoordModeOrigin);
	}
	
	/* Draw text, if there is any */
	if (button_state.button[bnum].type == BUTTON_TEXT) {
		if (button_state.button[bnum].enabled) {
			XSetForeground(x11_state.display, x11_state.gc_menus,
				predef_colors[BLACK].as_rgb_int()
			);
		} else {
			XSetForeground(x11_state.display, x11_state.gc_menus,
				predef_colors[DARKGREY].as_rgb_int()
			);
		}
		menutext(button_state.button[bnum].draw,button_state.button[bnum].width/2,
			button_state.button[bnum].height/2,button_state.button[bnum].text);
	}
}


static int x11_which_button (Window win) 
{
	int i;
	
	for (i=0;i<button_state.num_buttons;i++) {
		if (button_state.button[i].win == win)
			return(i);
	}
	printf("Error:  Unknown button ID in which_button.\n");
	return(0);
}


/* Shows when the menu is active or inactive by colouring the 
 * buttons.                                                   
 */
static void x11_turn_on_off (int pressed) 
{
	int i;
	
	for (i=0;i<button_state.num_buttons;i++) {
		button_state.button[i].ispressed = pressed;
		x11_drawbut(i);
	}
}


static void x11_drawmenu(void) 
{
	int i;

	XClearWindow (x11_state.display, x11_state.menu);
	XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[WHITE].as_rgb_int());
	XDrawRectangle(x11_state.display, x11_state.menu, x11_state.gc_menus, 0, 0, MWIDTH, 
				   trans_coord.top_height);
	XSetForeground(x11_state.display, x11_state.gc_menus,predef_colors[BLACK].as_rgb_int());
	XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, 0, trans_coord.top_height-1, 
			  MWIDTH, trans_coord.top_height-1);
	XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, MWIDTH-1, 
			  trans_coord.top_height, MWIDTH-1, 0);
	
	for (i=0;i<button_state.num_buttons;i++)  {
		x11_drawbut(i);
	}
}


static void x11_handle_expose(XEvent report, void (*drawscreen) (void))
{
#ifdef VERBOSE
	printf("Got an expose event.\n");
	printf("Count is: %d.\n",report.xexpose.count);
	printf("Window ID is: %ld.\n",report.xexpose.window);
#endif
	if (report.xexpose.window == x11_state.menu)
		x11_drawmenu();
	else if (report.xexpose.window == x11_state.toplevel)
		drawscreen();
	else if (report.xexpose.window == x11_state.textarea)
		draw_message();
}


static void x11_handle_configure_notify(XEvent report)
{
	trans_coord.top_width = report.xconfigure.width;
	trans_coord.top_height = report.xconfigure.height;
	update_transform();
	x11_drawmenu();
	draw_message();
#ifdef VERBOSE
	printf("Got a ConfigureNotify.\n");
	printf("New width: %d  New height: %d.\n",trans_coord.top_width,trans_coord.top_height);
#endif
}


static void x11_handle_button_info (t_event_buttonPressed *button_info, 
										int buttonNumber, int Xbutton_state)
{

	button_info->button = buttonNumber;

	if (Xbutton_state & 1)
		button_info->shift_pressed = true;
	else
		button_info->shift_pressed = false;

	if (Xbutton_state & 4)
		button_info->ctrl_pressed = true;
	else
		button_info->ctrl_pressed = false;
}

#endif /* X-Windows Specific Definitions */


/*************************************************
* Microsoft Windows (WIN32) Specific Definitions *
*************************************************/
#ifdef WIN32

static void 
win32_init_graphics (const char *window_name)
{
   WNDCLASSW wndclass;
   HINSTANCE hInstance = GetModuleHandle(NULL);
   int x, y;
   LOGBRUSH lb;
   lb.lbStyle = BS_SOLID;
   lb.lbColor = convert_to_win_color(gl_state.foreground_color);
   lb.lbHatch = (LONG)NULL;
   x = 0;
   y = 0;

   /* get screen size from display structure macro */
   trans_coord.display_width = GetSystemMetrics( SM_CXSCREEN );
   if (!(trans_coord.display_width))
      WIN32_CREATE_ERROR();
   trans_coord.display_height = GetSystemMetrics( SM_CYSCREEN );
   if (!(trans_coord.display_height))
      WIN32_CREATE_ERROR();
   trans_coord.top_width = 2*trans_coord.display_width/3;
   trans_coord.top_height = 4*trans_coord.display_height/5;
	
	/* Copy the Application name */
	int text_byte_length = strlen(window_name);
	MultiByteToWideChar(CP_UTF8, 0, window_name, -1,
		szAppName, text_byte_length);

   win32_state.hGraphicsPen = ExtCreatePen(
      PS_GEOMETRIC | win32_line_styles[gl_state.currentlinestyle] | PS_ENDCAP_FLAT,
      1,
      &lb,
      (LONG)NULL,
      NULL
   );

   if(!win32_state.hGraphicsPen)
      WIN32_CREATE_ERROR();
   win32_state.hGraphicsBrush = CreateSolidBrush(convert_to_win_color(predef_colors[DARKGREY]));
   if(!win32_state.hGraphicsBrush)
      WIN32_CREATE_ERROR();
   win32_state.hGrayBrush = CreateSolidBrush(convert_to_win_color(predef_colors[LIGHTGREY]));
   if(!win32_state.hGrayBrush)
      WIN32_CREATE_ERROR();
	
   /* Register the Main Window class */
   wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wndclass.lpfnWndProc = WIN32_MainWND;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
   wndclass.hCursor = LoadCursor( NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) CreateSolidBrush(
      convert_to_win_color(gl_state.background_color)
   );
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = szAppName;
	
	if (!RegisterClassW(&wndclass)) {
		printf ("Error code: %d\n", GetLastError());
		MessageBoxW(NULL, L"Initialization of Windows graphics (init_graphics) failed.",
		            szAppName, MB_ICONERROR);
		exit(-1);
	}
	
   /* Register the Graphics Window class */
   wndclass.lpfnWndProc = WIN32_GraphicsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szGraphicsName;
	
	if(!RegisterClassW(&wndclass))
		WIN32_DRAW_ERROR();
	
   /* Register the Status Window class */
   wndclass.lpfnWndProc = WIN32_StatusWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szStatusName;
   wndclass.hbrBackground = win32_state.hGrayBrush;
   
	if(!RegisterClassW(&wndclass))
		WIN32_DRAW_ERROR();
	
   /* Register the Buttons Window class */
   wndclass.lpfnWndProc = WIN32_ButtonsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szButtonsName;
   wndclass.hbrBackground = win32_state.hGrayBrush;
	
	if (!RegisterClassW(&wndclass))
		WIN32_DRAW_ERROR();

	win32_state.hMainWnd = CreateWindowW(
		szAppName, szAppName,
		WS_OVERLAPPEDWINDOW, x, y, trans_coord.top_width,
		trans_coord.top_height, NULL, NULL, hInstance, NULL);

   if(!win32_state.hMainWnd)
      WIN32_DRAW_ERROR();
	
   /* Set drawing defaults for user-drawable area.  Use whatever the *
    * initial values of the current stuff was set to.                */
	
   if (ShowWindow(win32_state.hMainWnd, SW_SHOWNORMAL))
      WIN32_DRAW_ERROR();
   build_default_menu();
   if (!UpdateWindow(win32_state.hMainWnd))
      WIN32_DRAW_ERROR();
   win32_drain_message_queue ();
}


static LRESULT CALLBACK 
WIN32_MainWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MINMAXINFO FAR *lpMinMaxInfo;   
	
	switch(message)
	{
		case WM_CREATE:
			win32_state.hStatusWnd = CreateWindowW(szStatusName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
				0, 0, 0, 0, hwnd, (HMENU) 102, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			win32_state.hButtonsWnd = CreateWindowW(szButtonsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
				0, 0, 0, 0, hwnd, (HMENU) 103, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			win32_state.hGraphicsWnd = CreateWindowW(szGraphicsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
				0, 0, 0, 0, hwnd, (HMENU) 101, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			return 0;
		
		case WM_SIZE:
			/* Window has been resized.  Save the new client dimensions */
			trans_coord.top_width = LOWORD (lParam);
			trans_coord.top_height = HIWORD (lParam);
		
			/* Resize the children windows */
			if(!MoveWindow(win32_state.hGraphicsWnd, 1, 1, trans_coord.top_width - MWIDTH - 1,
							trans_coord.top_height - T_AREA_HEIGHT - 1, TRUE))
				WIN32_DRAW_ERROR();
			if(!MoveWindow(win32_state.hStatusWnd, 0, trans_coord.top_height - T_AREA_HEIGHT,
							trans_coord.top_width - MWIDTH, T_AREA_HEIGHT, TRUE))
				WIN32_DRAW_ERROR();
			if(!MoveWindow(win32_state.hButtonsWnd, trans_coord.top_width - MWIDTH, 0, MWIDTH,
							trans_coord.top_height, TRUE))
				WIN32_DRAW_ERROR();
		
			return 0;
		
		// WC : added to solve window resizing problem
		case WM_GETMINMAXINFO:
			// set the MINMAXINFO structure pointer 
			lpMinMaxInfo = (MINMAXINFO FAR *) lParam;
			lpMinMaxInfo->ptMinTrackSize.x = trans_coord.display_width / 2;
			lpMinMaxInfo->ptMinTrackSize.y = trans_coord.display_height / 2;
		
			return 0;
		
		
		case WM_DESTROY:
			if(!DeleteObject(win32_state.hGrayBrush))
				WIN32_DELETE_ERROR();
			PostQuitMessage(0);
			return 0;
	
		case WM_KEYDOWN:
			if (gl_state.get_keypress_input)
				 win32_keypress_ptr((char) wParam);
			return 0;

		// Controls graphics: does zoom in or zoom out depending on direction of mousewheel scrolling.
		// Only the window with the input focus will receive this message. In our case, the top-level 
		// window has the input focus, thus the code will not work if put in WIN32_GraphicsWND.
		case WM_MOUSEWHEEL:
			// t_event_buttonPressed is used as a structure for storing information about a mouse
			// button press event. This information can be passed back to and used by a client 
			// program.
			t_event_buttonPressed button_info;
			win32_handle_button_info(button_info, message, wParam);

			win32_handle_mousewheel_zooming(wParam, lParam, true);
			return 0;
		case WM_TIMER:
			win32_drawscreen_ptr();
			// fall out.
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


static LRESULT CALLBACK 
WIN32_GraphicsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TEXTMETRIC tm;
	
	PAINTSTRUCT ps;
	static RECT oldAdjustRect;
	static HPEN hDotPen = 0;
	static int X, Y, i;
	
	switch(message)
	{
		case WM_CREATE:
		
			/* Get the text metrics once upon creation (system font cannot change) */
			win32_state.hGraphicsDC = GetDC (hwnd);
			if(!win32_state.hGraphicsDC)
				WIN32_DRAW_ERROR();

			if(!SetBkMode(win32_state.hGraphicsDC, TRANSPARENT))
				WIN32_DRAW_ERROR();
		
			/* Setup the pens, etc */
			gl_state.currentlinestyle = SOLID;
			gl_state.foreground_color = predef_colors[BLACK];
			gl_state.currentlinewidth = 1;
			gl_state.currentfontsize = 12;
			return 0;
		
			case WM_PAINT:
			win32_GraphicsWND_handle_WM_PAINT(hwnd, ps, hDotPen, oldAdjustRect);
			return 0;
		
			case WM_SIZE:
			/* Window has been resized.  New client area dimensions can be retrieved from
			 * lParam using LOWORD() and HIWORD() macros. 
			 */

			update_transform();
			return 0;
		
		case WM_DESTROY:
			if(!DeleteObject(win32_state.hGraphicsPen))
				WIN32_DELETE_ERROR();
			if(!DeleteObject(win32_state.hGraphicsBrush))
				WIN32_DELETE_ERROR();
			if(win32_state.hGraphicsFont != NULL && !DeleteObject(win32_state.hGraphicsFont))
				WIN32_DELETE_ERROR();
			PostQuitMessage(0);
			return 0;
	
		// left click and right click have the same functionality
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			win32_GraphicsWND_handle_WM_LRBUTTONDOWN(message, wParam, lParam, X, Y, oldAdjustRect);
			return 0;
	
		// If the mouse device has a scroll wheel, then this is a click of the wheel.
		// Enable panning by holding down the mouse wheel and drag.
		case WM_MBUTTONDOWN: 	
			win32_GraphicsWND_handle_WM_MBUTTONDOWN(hwnd, message, wParam, lParam);
			return 0;
	
		// Release the middle mouse button (mouse wheel) to stop panning
		case WM_MBUTTONUP:
			// turn off panning_enabled
			panning_off();

			/* Stops the mouse capturing started by SetCapture(). */
			ReleaseCapture();
			return 0;
	
		case WM_MOUSEMOVE:
			win32_GraphicsWND_handle_WM_MOUSEMOVE(lParam, X, Y, oldAdjustRect);
			return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}

 
static void 
win32_GraphicsWND_handle_WM_PAINT(HWND hwnd, PAINTSTRUCT &ps, HPEN &hDotPen, RECT &oldAdjustRect)
{
	// was in xor mode, but got a general redraw.
    // switch to normal drawing so we repaint properly.
	if (gl_state.current_draw_mode == DRAW_XOR) {
		set_draw_mode(DRAW_NORMAL);
		win32_invalidate_screen();
		return;
	}
	BeginPaint(hwnd, &ps);
	if(!win32_state.hGraphicsDC)
		WIN32_DRAW_ERROR();
		
	if (win32_state.InEventLoop) {

		// if program was still executing the "Window" command and drawing rubber band
		if(win32_state.windowAdjustFlag == WAITING_FOR_SECOND_CORNER_POINT) {
			/* ps.rcPaint specifies the screen coordinates of the window's client area 
			 * in which drawing is requested. This information is used to indicate that 
			 * the application window has been minimized and then restored. If so, need 
			 * to redraw the screen before drawing new rubber band.
			 */
			if (ps.rcPaint.right == (trans_coord.top_width-MWIDTH-1)
				 && ps.rcPaint.bottom == (trans_coord.top_height-T_AREA_HEIGHT-1)) 
				win32_drawscreen_ptr();

			// Create pen for rubber band drawing
			hDotPen = CreatePen(PS_DASH, 1, convert_to_win_color(gl_state.background_color));
			if(!hDotPen)
				WIN32_CREATE_ERROR();
			if (!SetROP2(win32_state.hGraphicsDC, R2_XORPEN))
				WIN32_SELECT_ERROR();
			if(!SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_BRUSH)))
				WIN32_SELECT_ERROR();
			if(!SelectObject(win32_state.hGraphicsDC, hDotPen))
				WIN32_SELECT_ERROR();

			// Don't need to erase old rubber band if the window has been minimized and
			// restored, because previous drawings were invalidated when the window was 
			// minimized.
			if (ps.rcPaint.right != (trans_coord.top_width-MWIDTH-1)
				 || ps.rcPaint.bottom != (trans_coord.top_height-T_AREA_HEIGHT-1))
			{
				// Erase old rubber band before drawing a new one
				if(!Rectangle(win32_state.hGraphicsDC, oldAdjustRect.left, oldAdjustRect.top, 
							  oldAdjustRect.right, oldAdjustRect.bottom))
					WIN32_DRAW_ERROR();
			}

			// Draw new rubber band
			if(!Rectangle(win32_state.hGraphicsDC, win32_state.adjustRect.left, 
						  win32_state.adjustRect.top, win32_state.adjustRect.right,
				          win32_state.adjustRect.bottom))
				WIN32_DRAW_ERROR();

			oldAdjustRect = win32_state.adjustRect;
			if (!SetROP2(win32_state.hGraphicsDC, R2_COPYPEN))
				WIN32_SELECT_ERROR();
			if(!SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_PEN)))
				WIN32_SELECT_ERROR();
			if(!DeleteObject(hDotPen))
				WIN32_DELETE_ERROR();
		}
		else {
			win32_drawscreen_ptr();	
		}
	}
	EndPaint(hwnd, &ps);
	
	/* Crash hard if called at wrong time */
	/* win32_state.hGraphicsDC = NULL;*/
}


static void win32_GraphicsWND_handle_WM_LRBUTTONDOWN(UINT message, WPARAM wParam, LPARAM lParam, 
													 int &X, int &Y, RECT &oldAdjustRect)
{
	// t_event_buttonPressed is used as a structure for storing information about a mouse
	// button press event. This information can be passed back to and used by a client 
	// program.
	t_event_buttonPressed button_info;
	win32_handle_button_info(button_info, message, wParam);

	if (win32_state.windowAdjustFlag == WINDOW_DEACTIVATED) {  
		// Call function in client program
		win32_mouseclick_ptr(xscrn_to_world(LOWORD(lParam)), yscrn_to_world(HIWORD(lParam)),
					   button_info);
	} 
	else {
		// Special handling for the "Window" command, which takes multiple clicks. 
        // First you push the button, then you click for one corner, then you click for the other
        // corner.
		if(win32_state.windowAdjustFlag == WAITING_FOR_FIRST_CORNER_POINT) {
			win32_state.windowAdjustFlag = WAITING_FOR_SECOND_CORNER_POINT;
			X = win32_state.adjustRect.left = win32_state.adjustRect.right = LOWORD(lParam);
			Y = win32_state.adjustRect.top = win32_state.adjustRect.bottom = HIWORD(lParam);
			oldAdjustRect = win32_state.adjustRect;
		}
		else {
			int i;
			int adjustx[2], adjusty[2];
			
			win32_state.windowAdjustFlag = WINDOW_DEACTIVATED;
			button_state.button[win32_state.adjustButton].ispressed = 0;
			SendMessage(button_state.button[win32_state.adjustButton].hwnd, BM_SETSTATE, 0, 0);
							
			for (i=0; i<button_state.num_buttons; i++) {
				if (button_state.button[i].type != BUTTON_SEPARATOR
					 && button_state.button[i].enabled) 
				{
					if(!EnableWindow (button_state.button[i].hwnd, TRUE))
						WIN32_DRAW_ERROR();
				}
			}
			adjustx[0] = win32_state.adjustRect.left;
			adjustx[1] = win32_state.adjustRect.right;
			adjusty[0] = win32_state.adjustRect.top;
			adjusty[1] = win32_state.adjustRect.bottom;
				
			update_win(adjustx, adjusty, win32_invalidate_screen);
		}
	}
}


static void 
win32_GraphicsWND_handle_WM_MBUTTONDOWN(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// t_event_buttonPressed is used as a structure for storing information about a mouse
	// button press event. This information can be passed back to and used by a client 
	// program.
	t_event_buttonPressed button_info;
	win32_handle_button_info(button_info, message, wParam);
		
	// get x- and y- coordinates of the cursor. Do not use LOWORD or HIWORD macros 
	// to extract the coordinates because these macros can return incorrect results
	// on systems with multiple monitors.
	int xPos, yPos;
	xPos = GET_X_LPARAM(lParam);
	yPos = GET_Y_LPARAM(lParam);

	// turn on panning_enabled
	panning_on(xPos, yPos);

	/* Windows function specifically designed for mouse click and drag.
	 * This function sends all mouse message to hGraphicsWnd, even if
	 * the cursor is outside the client program's top-level window.
	 */
	SetCapture(hwnd);
}


static void 
win32_GraphicsWND_handle_WM_MOUSEMOVE(LPARAM lParam, int &X, int &Y, RECT &oldAdjustRect)
{
#ifdef VERBOSE
	printf("Got a MotionNotify Event.\n");
	printf("x: %d    y: %d\n",GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
#endif
	if(win32_state.windowAdjustFlag == WAITING_FOR_FIRST_CORNER_POINT) {
		return;
	}
	else if (win32_state.windowAdjustFlag == WAITING_FOR_SECOND_CORNER_POINT) {
		if ( X > LOWORD(lParam)) {
			win32_state.adjustRect.left = LOWORD(lParam);
			win32_state.adjustRect.right = X;
		}
		else {
			win32_state.adjustRect.left = X;
			win32_state.adjustRect.right = LOWORD(lParam);
		}
		if ( Y > HIWORD(lParam)) {
			win32_state.adjustRect.top = HIWORD(lParam);
			win32_state.adjustRect.bottom = Y;
		}
		else {
			win32_state.adjustRect.top = Y;
			win32_state.adjustRect.bottom = HIWORD(lParam);
		}
		if(!InvalidateRect(win32_state.hGraphicsWnd, &oldAdjustRect, FALSE))
			WIN32_DRAW_ERROR();
		if(!InvalidateRect(win32_state.hGraphicsWnd, &win32_state.adjustRect, FALSE))
			WIN32_DRAW_ERROR();
		if(!UpdateWindow(win32_state.hGraphicsWnd))
			WIN32_DRAW_ERROR();
			
		return;
	}
	else if (pan_state.panning_enabled) {
		// get x- and y- coordinates of the cursor. Do not use LOWORD or HIWORD macros 
		// to extract the coordinates because these macros can return incorrect results
		// on systems with multiple monitors.
		int xPos, yPos;
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);

		panning_execute(xPos, yPos, win32_drawscreen_ptr);
	} 
	else if (gl_state.get_mouse_move_input) { 
		win32_mousemove_ptr(xscrn_to_world(LOWORD(lParam)), yscrn_to_world(HIWORD(lParam)));
	}
}


static LRESULT CALLBACK 
WIN32_StatusWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	
	switch(message)
	{
		case WM_CREATE:
			hdc = GetDC(hwnd);
			if(!hdc)
				WIN32_DRAW_ERROR();
			if(!SetBkMode(hdc, TRANSPARENT))
				WIN32_DRAW_ERROR();
			if(!ReleaseDC(hwnd, hdc))
				WIN32_DRAW_ERROR();
			return 0;
		
		case WM_PAINT: {
			hdc = BeginPaint(hwnd, &ps);
			if(!hdc)
				WIN32_DRAW_ERROR();
		
			if(!GetClientRect(hwnd, &rect))
				WIN32_DRAW_ERROR();
		
			if(!SelectObject(hdc, GetStockObject(NULL_BRUSH)))
				WIN32_SELECT_ERROR();
			if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
				WIN32_SELECT_ERROR();
			if(!Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom))
				WIN32_DRAW_ERROR();
			if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
				WIN32_SELECT_ERROR();
			if(!MoveToEx(hdc, rect.left, rect.bottom-1, NULL))
				WIN32_DRAW_ERROR();
			if(!LineTo(hdc, rect.right-1, rect.bottom-1))
				WIN32_DRAW_ERROR();
			if(!LineTo(hdc, rect.right-1, rect.top))
				WIN32_DRAW_ERROR();

			int text_byte_length = strlen(gl_state.statusMessage);
			wchar_t* WIN32_wchar_text = new wchar_t[text_byte_length];
			size_t WIN32_wchar_text_len =
				MultiByteToWideChar(
					CP_UTF8, 0,
					gl_state.statusMessage, text_byte_length,
					WIN32_wchar_text, text_byte_length
				);

			if(!DrawTextW(hdc, WIN32_wchar_text, WIN32_wchar_text_len, &rect, 
						 DT_CENTER | DT_VCENTER | DT_SINGLELINE))
				WIN32_DRAW_ERROR();
		
			delete[] WIN32_wchar_text;

			if(!EndPaint(hwnd, &ps))
				WIN32_DRAW_ERROR();
			return 0;
		
		} case WM_SIZE:
			return 0;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


static LRESULT CALLBACK 
WIN32_ButtonsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	static HBRUSH hBrush;
	int i;
	
	switch(message)
	{
		case WM_COMMAND:
			if (win32_state.windowAdjustFlag == WINDOW_DEACTIVATED) {
				button_state.button[LOWORD(wParam) - 200].fcn(win32_invalidate_screen);
				if (win32_state.windowAdjustFlag != WINDOW_DEACTIVATED) {
					win32_state.adjustButton = LOWORD(wParam) - 200;
					button_state.button[win32_state.adjustButton].ispressed = 1;
					for (i=0; i<button_state.num_buttons; i++) {
						EnableWindow(button_state.button[i].hwnd, FALSE);
						SendMessage(button_state.button[i].hwnd, BM_SETSTATE, 
									button_state.button[i].ispressed, 0);
					}
				}
			}
			SetFocus(win32_state.hMainWnd);
			return 0;
		
		case WM_CREATE:
			hdc = GetDC(hwnd);
			if(!hdc)
				WIN32_DRAW_ERROR();
			hBrush = CreateSolidBrush(convert_to_win_color(predef_colors[LIGHTGREY]));
			if(!hBrush)
				WIN32_CREATE_ERROR();
			if(!SelectObject(hdc, hBrush))
				WIN32_SELECT_ERROR();
			if(!SetBkMode(hdc, TRANSPARENT))
				WIN32_DRAW_ERROR();
			if(!ReleaseDC(hwnd, hdc))
				WIN32_DRAW_ERROR();
		
			return 0;
		
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			if(!hdc)
				WIN32_DRAW_ERROR();
		
			if(!GetClientRect(hwnd, &rect))
				WIN32_DRAW_ERROR();
		
			if(!SelectObject(hdc, GetStockObject(NULL_BRUSH)))
				WIN32_SELECT_ERROR();
			if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
				WIN32_SELECT_ERROR();
			if(!Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom))
				WIN32_DRAW_ERROR();
			if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
				WIN32_SELECT_ERROR();
			if(!MoveToEx(hdc, rect.left, rect.bottom-1, NULL))
				WIN32_DRAW_ERROR();
			if(!LineTo(hdc, rect.right-1, rect.bottom-1))
				WIN32_DRAW_ERROR();
			if(!LineTo(hdc, rect.right-1, rect.top))
				WIN32_DRAW_ERROR();

			for (i=0; i < button_state.num_buttons; i++) {
				if(button_state.button[i].type == BUTTON_SEPARATOR) {
					int x, y, w;

					x = button_state.button[i].xleft;
					y = button_state.button[i].ytop;
					w = button_state.button[i].width;

					if(!MoveToEx (hdc, x, y, NULL))
						WIN32_DRAW_ERROR();
					if(!LineTo (hdc, x + w, y))
						WIN32_DRAW_ERROR();			
					if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
						WIN32_SELECT_ERROR();
					if(!MoveToEx (hdc, x, y+1, NULL))
						WIN32_DRAW_ERROR();
					if(!LineTo (hdc, x + w, y+1))
						WIN32_DRAW_ERROR();			
					if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
						WIN32_SELECT_ERROR();
				}
			}
			if(!EndPaint(hwnd, &ps))
				WIN32_DRAW_ERROR();
			return 0;
		
		case WM_DESTROY:
			for (i=0; i<button_state.num_buttons; i++) {
			}
			if(!DeleteObject(hBrush))
				WIN32_DELETE_ERROR();
			PostQuitMessage(0);
			return 0;
		}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


static void WIN32_SELECT_ERROR()
{ 
	wchar_t msg[BUFSIZE];
	wsprintf (msg, L"Error %i: Couldn't select graphics object on line %d of graphics.c\n",
			 GetLastError(), __LINE__);

	wprintf(msg);
	MessageBoxW(NULL, msg, NULL, MB_OK);
	exit(-1); 
}


static void WIN32_DELETE_ERROR()
{ 
	wchar_t msg[BUFSIZE];
	wsprintf (msg, L"Error %i: Couldn't delete graphics object on line %d of graphics.c\n",
			 GetLastError(), __LINE__);
	
	wprintf(msg);
	MessageBoxW(NULL, msg, NULL, MB_OK);
	exit(-1); 
}


static void WIN32_CREATE_ERROR() 
{ 
	wchar_t msg[BUFSIZE];
	wsprintf (msg, L"Error %i: Couldn't create graphics object on line %d of graphics.c\n",
			 GetLastError(), __LINE__);

	wprintf(msg);
	MessageBoxW(NULL, msg, NULL, MB_OK);
	exit(-1); 
}


static void WIN32_DRAW_ERROR()
{ 
	wchar_t msg[BUFSIZE];
	wsprintf (msg, L"Error %i: Couldn't draw graphics object on line %d of graphics.c\n",
			 GetLastError(), __LINE__);

	wprintf(msg);
	MessageBoxW(NULL, msg, NULL, MB_OK);
	exit(-1); 
}


static void win32_invalidate_screen(void)
{
/* Tells the graphics engine to redraw the graphics display since information has changed */

	if(!InvalidateRect(win32_state.hGraphicsWnd, NULL, FALSE))
		WIN32_DRAW_ERROR();
	if(!UpdateWindow(win32_state.hGraphicsWnd))
		WIN32_DRAW_ERROR();
}


static void win32_reset_state () {
   // Not sure exactly what needs to be reset to NULL etc. 
   // Resetting everthing to be safe.
   win32_state.hGraphicsPen = 0;
   win32_state.hGraphicsBrush = 0;
   win32_state.hGrayBrush = 0;
   win32_state.hGraphicsDC = 0;
   win32_state.hGraphicsFont = 0;

   /* These are used for the "Window" graphics button. They keep track of whether we're entering
    * the window rectangle to zoom to, etc.
    */
   win32_state.windowAdjustFlag = WINDOW_DEACTIVATED;
   win32_state.adjustButton = -1;
   win32_state.InEventLoop = false;
}


static void win32_drain_message_queue () {
   // Drain the message queue, so we don't have a quit message lying around
   // that will stop us from re-opening a window later if desired.
   MSG msg; 
   while (PeekMessage(&msg, win32_state.hMainWnd,  0, 0, PM_REMOVE)) { 
      if (msg.message == WM_QUIT) {
         printf ("Got the quit message.\n");
      }
   } 
}


static void win32_handle_mousewheel_zooming(WPARAM wParam, LPARAM lParam, bool draw_screen) {
	// zDelta indicates the distance which the mouse wheel is rotated. 
	// The value for zDelta is a multiple of WHEEL_DELTA, which is 120.
	// WHEEL_DELTA is the value for scrolling the mouse wheel by one increment.
	short zDelta;
	zDelta = GET_WHEEL_DELTA_WPARAM(wParam); 

	// roll_detent captures how many increments the mouse wheel has been scrolled by.
	int roll_detent;
	roll_detent = zDelta/WHEEL_DELTA;
	// roll_detent will be negative if wheel was roatated backward, toward the user.
	if (roll_detent <0)
		roll_detent *= -1;

	// get x- and y- coordinates of the cursor. WM_MOUSEWHEEL receives coordinates of
	// the cursor relative to the upper-left corner of the screen instead of the client
	// program. Therefore, call ScreenToClient() to convert.
	POINT mousePos;
	mousePos.x = GET_X_LPARAM(lParam);
	mousePos.y = GET_Y_LPARAM(lParam);
	ScreenToClient(win32_state.hMainWnd, &mousePos);


	for (int i = 1; i <= roll_detent; i++) {
		// Positive value for zDelta indicates that the wheel was rotated forward, which
		// will trigger zoom_in. Otherwise, zoom_out is called.
		// Also, only redraw the screen on the last one.
		if (zDelta > 0)
			handle_zoom_in(xscrn_to_world(mousePos.x), yscrn_to_world(mousePos.y), 
				(draw_screen && (i == roll_detent)) ? win32_drawscreen_ptr : NULL);
		else
			handle_zoom_out(xscrn_to_world(mousePos.x), yscrn_to_world(mousePos.y), 
				(draw_screen && (i == roll_detent)) ? win32_drawscreen_ptr : NULL);
	}
}


static void win32_handle_button_info (t_event_buttonPressed &button_info, 
										UINT message, WPARAM wParam)
{
	/* The parameter "wParam" is an unsigned int. In this case, it contains information indicating *
	 * whether various virtual keys (ie. modifier keys) are held during a mouse button press       *
	 * event.																					   */
	if (wParam & MK_SHIFT) 
		button_info.shift_pressed = true;
	else
		button_info.shift_pressed = false;
	
	if (wParam & MK_CONTROL) 
		button_info.ctrl_pressed = true;
	else
		button_info.ctrl_pressed = false;
	
	/* Parameter "message" indicates what button is pressed: pass 1 for left click, 
	 * 3 for right click, 2 for scroll wheel click, 4 for scroll wheel forward rotate, 
	 * and 5 for scroll wheel backward rotate. 
	 * We follow this convention in order to be consistent for both X11 and WIN32.         
	 */
	switch (message)
	{
		case (WM_LBUTTONDOWN):
			button_info.button = 1;
			break;
		case (WM_RBUTTONDOWN):
			button_info.button = 3;
			break;
		case (WM_MBUTTONDOWN):
			button_info.button = 2;
			break;
		case (WM_MOUSEWHEEL):
			short zDelta;
			zDelta = GET_WHEEL_DELTA_WPARAM(wParam); 
			// Positive value for zDelta indicates that the wheel was rotated forward, 
			// away from user, and negative value indicates wheel rotated backward.
			if (zDelta > 0)
				button_info.button = 4;
			else
				button_info.button = 5;
			break;
	}	

#ifdef VERBOSE
	printf("Button pressed is: %d.\n(left click is 1; right click is 3; "
			"scroll wheel click is 2; scroll wheel forward rotate is 4; "
			"scroll wheel backward is 5.)\n",button_info.button);
	if (button_info.shift_pressed == true)
		printf("Shift is pressed at button press.\n");
	if (button_info.ctrl_pressed == true)
		printf("Ctrl is pressed at button press.\n");
#endif
}


static void _drawcurve(t_point *points, int npoints, int fill) {
/* Draw a beizer curve.
 * Must have 3I+1 points, since each Beizer curve needs 3 points and we also 
 * need an initial starting point 
 */
	float xmin, ymin, xmax, ymax;
	int i;
	
	if ((npoints - 1) % 3 != 0 || npoints > MAXPTS)
		WIN32_DRAW_ERROR();
	
	/* Conservative (but fast) clip test -- check containing rectangle of *
	* polygon.                                                           */
	
	xmin = xmax = points[0].x;
	ymin = ymax = points[0].y;
	
	for (i=1;i<npoints;i++) {
		xmin = min (xmin,points[i].x);
		xmax = max (xmax,points[i].x);
		ymin = min (ymin,points[i].y);
		ymax = max (ymax,points[i].y);
	}

	if (rect_off_screen(xmin,ymin,xmax,ymax))
		return;
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		/* implement X11 version here */
#else /* Win32 */
		// create POINT array
		HPEN hOldPen;
		POINT pts[MAXPTS];
		int i;

		for (i = 0; i < npoints; i++) {
			pts[i].x = xworld_to_scrn(points[i].x);
			pts[i].y = yworld_to_scrn(points[i].y);
		}

		if (fill) {
			/* NULL_PEN is a Windows stock object which does not draw anything. Set current *
		     * pen to NULL_PEN in order to fill the curve without drawing the outline.      */
			hOldPen = (HPEN)SelectObject(win32_state.hGraphicsDC, GetStockObject(NULL_PEN));
			if(!(hOldPen))
				WIN32_SELECT_ERROR();
		}

		if (!BeginPath(win32_state.hGraphicsDC))
			WIN32_DRAW_ERROR();
		if(!PolyBezier(win32_state.hGraphicsDC, pts, npoints))
			WIN32_DRAW_ERROR();
		if (!EndPath(win32_state.hGraphicsDC))
			WIN32_DRAW_ERROR();

		if (!fill) {
			if (!StrokePath(win32_state.hGraphicsDC))
				WIN32_DRAW_ERROR();
		}
		else {
			if (!FillPath(win32_state.hGraphicsDC))
				WIN32_DRAW_ERROR();
		}

		if (fill) {
			/* Need to restore the original pen into the device context after filling. */
			if(!SelectObject(win32_state.hGraphicsDC, hOldPen))
				WIN32_SELECT_ERROR();
		}
#endif
	}
	else {
		int i;

		fprintf(gl_state.ps, "newpath\n");
		fprintf(gl_state.ps, "%.2f %.2f moveto\n", xworld_to_post(points[0].x), 
				yworld_to_post(points[0].y));
		for (i = 1; i < npoints; i+= 3)
			fprintf(gl_state.ps,"%.2f %.2f %.2f %.2f %.2f %.2f curveto\n", 
					xworld_to_post(points[i].x), yworld_to_post(points[i].y), 
					xworld_to_post(points[i+1].x), yworld_to_post(points[i+1].y), 
					xworld_to_post(points[i+2].x), yworld_to_post(points[i+2].y));
		if (!fill)
			fprintf(gl_state.ps, "stroke\n");
		else
			fprintf(gl_state.ps, "fill\n");
	}
}


void win32_drawcurve(t_point *points,
			   int npoints) {
	_drawcurve(points, npoints, 0);
}


void win32_fillcurve(t_point *points,
			   int npoints) {
	_drawcurve(points, npoints, 1);
}

#endif /******** Win32 Specific Definitions ********************/


#else  /***** NO_GRAPHICS *******/
/* No graphics at all. Stub everything out so calling program doesn't have to change
 * but of course graphics won't do anything.
 */

#include "graphics.h"

void event_loop (void (*act_on_mousebutton) (float x, float y, t_event_buttonPressed button_info),
				 void (*act_on_mousemove) (float x, float y),
				 void (*act_on_keypress) (char key_pressed),
                 void (*drawscreen) (void)) { }

void init_graphics (const char *window_name, int cindex) { }
void init_graphics (const char *window_name, const t_color& background) { }
void close_graphics (void) { }
void update_message (const char *msg) { }
void draw_message (void) { }
void init_world (float xl, float yt, float xr, float yb) { }
void init_world(const t_bound_box& bounds) { }
void flushinput (void) { }
void setcolor (int cindex) { }
void setcolor (const t_color& c) { }
void setcolor (uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) { }
void setcolor_by_name (std::string cname) { }
t_color getcolor (void) { return t_color(0,0,0); }
void setlinestyle (int linestyle) { }
void setlinewidth (int linewidth) { }
void setfontsize (int pointsize) { }
int getfontsize() { return 0; }
void settextrotation (float degrees) { }
float gettextrotation() { return 0; }
void settextattrs(int pointsize, float degrees) { }
void drawline (const t_point& p1, const t_point& p2) { }
void drawline (float x1, float y1, float x2, float y2) { }
void drawrect (const t_bound_box& rect) { }
void drawrect (const t_point& bottomleft, const t_point& upperright) { }
void drawrect (float x1, float y1, float x2, float y2) { }
void fillrect (const t_bound_box& rect) { }
void fillrect (const t_point& bottomleft, const t_point& upperright) { }
void fillrect (float x1, float y1, float x2, float y2) { }
void fillpoly (t_point *points, int npoints) { }
void drawarc (float xcen, float ycen, float rad, float startang,
			  float angextent) { }
void drawellipticarc (
	const t_point& center, float radx, float rady, float startang, float angextent) { }
void drawellipticarc (float xc, float yc, float radx, float rady, 
					  float startang, float angextent) { }
void fillarc (const t_point& center, float rad, float startang, float angextent) { }
void fillarc (float xcen, float ycen, float rad, float startang,
			  float angextent) { }
void fillellipticarc (
	const t_point& center, float radx, float rady, float startang, float angextent) { }
void fillellipticarc (float xc, float yc, float radx, float rady, 
					  float startang, float angextent) { }

void drawtext_in (const t_bound_box& bbox, const char* text) { }
void drawtext_in (const t_bound_box& bbox, const char* text, float tolerance) { }
void drawtext (const t_point& text_center, const char* text, const t_bound_box& bounds) { }
void drawtext (const t_point& text_center, const char* text, 
	const t_bound_box& bounds, float tolerance) { }
void drawtext (const t_point& text_center, const char* text, float boundx, float boundy) { }
void drawtext (float xc, float yc, const char* text, float boundx, float boundy) { }

void clearscreen (void) { }

t_bound_box get_visible_world() { return t_bound_box(0,0,0,0); }

void create_button (const char *prev_button_text , const char *button_text,
					void (*button_func) (void (*drawscreen) (void))) { }

void destroy_button (const char *button_text) { }

int init_postscript (const char *fname) { 
	return (1);
}

void close_postscript (void) { }

void report_structure(t_report*) { }

void set_mouse_move_input (bool) { }

void set_keypress_input (bool) { }

void set_draw_mode (enum e_draw_mode draw_mode) { }

void enable_or_disable_button(int ibutton, bool enabled) { }

void change_button_text(const char *button_text, const char *new_button_text) { }

bool LOD_screen_area_test(t_bound_box test, float screen_area_threshold) { return false; }

#ifdef WIN32
void win32_drawcurve(t_point *points, int npoints) { }

void win32_fillcurve(t_point *points, int npoints) { }

#endif  // WIN32 (subset of commands)

#endif  // NO_GRAPHICS

/****************** begin definition of data structure members *********************/

/******************************************
 * begin t_point function definitions *
 ******************************************/

void t_point::set(float _x, float _y) { x = _x; y = _y; }
void t_point::set(const t_point& src) { x = src.x; y = src.y; }

void t_point::offset(float _x, float _y) {
	x += _x;
	y += _y;
}

t_point t_point::operator- (const t_point& rhs) const {
	t_point result = *this;
	result -= rhs;
	return result;
}

t_point t_point::operator+ (const t_point& rhs) const {
	t_point result = *this;
	result += rhs;
	return result;
}

t_point t_point::operator* (float rhs) const {
	t_point result = *this;
	result *= rhs;
	return result;
}

t_point& t_point::operator+= (const t_point& rhs) {
	this->x += rhs.x;
	this->y += rhs.y;
	return *this;
}

t_point& t_point::operator-= (const t_point& rhs) {
	this->x -= rhs.x;
	this->y -= rhs.y;
	return *this;
}

t_point& t_point::operator *= (float rhs) {
	this->x *= rhs;
	this->y *= rhs;
	return *this;
}

t_point& t_point::operator= (const t_point& src) {
	this->x = src.x;
	this->y = src.y;
	return *this;
}

t_point::t_point() : x(0), y(0) { }

t_point::t_point(const t_point& src) :
	x(src.x), y(src.y) {
}

t_point::t_point(float _x, float _y) : x(_x), y(_y) { }

t_point operator*(float lhs, const t_point& rhs) {
	return rhs*lhs;
}

/******************************************
 * begin t_bound_box function definitions *
 ******************************************/

const float& t_bound_box::left() const { return bottom_left().x; }
const float& t_bound_box::right() const { return top_right().x; }
const float& t_bound_box::bottom() const { return bottom_left().y; }
const float& t_bound_box::top() const { return top_right().y; }
const t_point& t_bound_box::bottom_left() const { return bottomleft; }
const t_point& t_bound_box::top_right() const { return topright; }
float& t_bound_box::left() { return bottom_left().x; }
float& t_bound_box::right() { return top_right().x; }
float& t_bound_box::bottom() { return bottom_left().y; }
float& t_bound_box::top() { return top_right().y; }
t_point& t_bound_box::bottom_left() { return bottomleft; }
t_point& t_bound_box::top_right() { return topright; }

float t_bound_box::get_xcenter() const {
	return (right() + left()) / 2;
}

float t_bound_box::get_ycenter() const {
	return (top() + bottom()) / 2;
}

t_point t_bound_box::get_center() const {
	return t_point(get_xcenter(), get_ycenter());
}

float t_bound_box::get_width() const {
	return right() - left();
}

float t_bound_box::get_height() const {
	return top() - bottom();
}

void t_bound_box::offset(const t_point& relative_to) {
	this->bottomleft += relative_to;
	this->topright += relative_to;
}

void t_bound_box::offset(float by_x, float by_y) {
	this->bottomleft.offset(by_x, by_y);
	this->topright.offset(by_x, by_y);
}

bool t_bound_box::intersects(const t_point& test_pt) const {
	return intersects(test_pt.x, test_pt.y);
}

bool t_bound_box::intersects(float x, float y) const {
	if (x < left() || right() < x || y < bottom() || top() < y) {
		return false;
	} else {
		return true;
	}
}

float t_bound_box::area() const {
	return fabs(get_width() * get_height());
}

t_bound_box t_bound_box::operator+ (const t_point& rhs) const {
	t_bound_box result = *this;
	result.offset(rhs);
	return result;
}

t_bound_box t_bound_box::operator- (const t_point& rhs) const {
	t_bound_box result = *this;
	result.offset(t_point(-rhs.x, -rhs.y));
	return result;
}

t_bound_box& t_bound_box::operator+= (const t_point& rhs) {
	this->offset(rhs);
	return *this;
}

t_bound_box& t_bound_box::operator-= (const t_point& rhs) {
	this->offset(t_point(-rhs.x, -rhs.y));
	return *this;
}

t_bound_box& t_bound_box::operator= (const t_bound_box& src) {
	this->bottom_left() = src.bottom_left();
	this->top_right() = src.top_right();
	return *this;
}

t_bound_box::t_bound_box() :
	bottomleft(), topright() {
}

t_bound_box::t_bound_box(const t_bound_box& src) :
	bottomleft(src.bottom_left()), topright(src.top_right()) {
}
t_bound_box::t_bound_box(float _left, float _bottom, float _right, float _top) :
	bottomleft(_left,_bottom), topright(_right,_top) {
}

t_bound_box::t_bound_box(const t_point& _bottomleft, const t_point& _topright) :
	bottomleft(_bottomleft), topright(_topright) {
}

t_bound_box::t_bound_box(const t_point& _bottomleft, float width, float height) :
	bottomleft(_bottomleft) , topright(_bottomleft) {
	topright.offset(width, height);
}

/******************************************
 * begin t_color function definitions *
 ******************************************/

t_color::t_color(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b) :
	red(r),
	green(g),
	blue(b) {
}

t_color::t_color(const t_color& src) :
	red(src.red),
	green(src.green),
	blue(src.blue) {
}

t_color::t_color() :
	red(0),
	green(0),
	blue(0) {
}

t_color::t_color(color_types src) {
	*this = src;
}

bool t_color::operator== (const t_color& rhs) const {
	return red == rhs.red
	&&   green == rhs.green
	&&    blue == rhs.blue;
}

bool t_color::operator!= (const t_color& rhs) const {
	return !(*this == rhs);
}

unsigned long t_color::as_rgb_int() const {
	return (((red << 8) | green) << 8) | blue;
}

#ifndef NO_GRAPHICS

	color_types t_color::operator=(color_types color_enum) {
		*this = predef_colors[color_enum];
		return color_enum;
	}

	bool t_color::operator== (color_types rhs) const {
		const t_color& test = predef_colors[rhs];
		if (test == *this) {
			return true;
		} else {
			return false;
		}
	}

#else /* WITHOUT GRAPHICS */

	color_types t_color::operator=(color_types color_enum) {
		*this = t_color(0,0,0);
		return BLACK;
	}

	bool t_color::operator== (color_types rhs) const {
		return false;
	}

#endif /* NO_GRAPHICS */

bool t_color::operator!= (color_types rhs) const {
	return !(*this == rhs);
}

// bool t_color::operator> (color_types rhs) const {
// 	auto color_index = std::find(predef_colors.begin(), predef_colors.end(), *this);
// 	if (color_index != predef_colors.end()) {
// 		return (color_index - predef_colors.begin()) > rhs;
// 	} else {
// 		return false;
// 	}
// }

// bool t_color::operator< (color_types rhs) const {
// 	auto color_index = std::find(predef_colors.begin(), predef_colors.end(), *this);
// 	if (color_index != predef_colors.end()) {
// 		return (color_index - predef_colors.begin()) < rhs;
// 	} else {
// 		return false;
// 	}
// }



#ifdef WIN32
COLORREF convert_to_win_color(const t_color& src) {
	return RGB(src.red, src.green, src.blue);
}
#endif /* WIN32 */
