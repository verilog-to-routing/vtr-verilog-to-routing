/*                                                                         *
* Easygl Version 2.0.1                                                     *
* Written by Vaughn Betz at the University of Toronto, Department of       *
* Electrical and Computer Engineering, with additions by Paul Leventis     *
* and William Chow of Altera, Guy Lemieux of the University of Brish       *
* Columbia, and Long Yu (Mike) Wang of the University of Toronto.          *
* All rights reserved by U of T, etc.                                      *
*                                                                          *
* You may freely use this graphics interface for non-commercial purposes   *
* as long as you leave the author info above in it.                        *
*                                                                          *
* Revision History:                                                        *
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

#ifndef NO_GRAPHICS  // Strip everything out and just put in stubs if NO_GRAPHICS defined

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
#include "graphics.h"
using namespace std;


#if defined(X11) || defined(WIN32)

// VERBOSE is very helpful for developing event handling features. Outputs
// useful information when user interacts with the graphic interface.
// Uncomment the line below to turn on VERBOSE.
// #define VERBOSE

#define MWIDTH			104 /* Width of menu window */
#define MENU_FONT_SIZE  12  /* Font for menus and dialog boxes. */
#define T_AREA_HEIGHT	24  /* Height of text window */
#define MAX_FONT_SIZE	24  /* Largest point size of text.   */
                            // Some computers only have up to 24 point 
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

/* Uncomment the line below if your X11 header files don't define XPointer */
/* typedef char *XPointer;                                                 */

#endif /* X11 Preprocessor Directives */

#ifdef WIN32

/*************************************************************
 * Microsoft Windows (WIN32) Specific Preprocessor Directives *
 *************************************************************/

#pragma warning(disable : 4996)   // Turn off annoying warnings about strcmp.

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
   void (*fcn) (void (*drawscreen) (void));
#ifdef X11
   Window win; 
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


/* Structure used to store overall graphics state variables.
 * initialized:  true if the graphics window & state have been 
 *      created and initialized, false otherwise.
 * disp_type: Selects SCREEN or POSTSCRIPT 
 * background_cindex: index of the window (or page for PS) background colour
 * currentcolor: current color in the graphics context
 * currentlinestyle: current linestyle in the graphics context
 * currentlinewidth: current linewidth in the graphics context
 * currentfontsize: current font size in the graphics context
 * current_draw_mode: select DRAW_NORMAL (for overwrite) or DRAW_XOR (for rubber-banding)
 * ps: for PostScript output
 * ProceedPressed: whether the Proceed button has been pressed
 * statusMessage: user message to display
 * font_is_loaded: whether a specified font size is loaded
 * get_keypress_input: whether keypresses are sent back to callback functions
 * get_mouse_move_input: whether mouse movements are sent back to callback functions
 */
typedef struct {
   bool initialized;
   t_display_type disp_type;
   int background_cindex;
   int currentcolor;
   int currentlinestyle;
   int currentlinewidth;
   int currentfontsize;
   e_draw_mode current_draw_mode;
   FILE *ps;
   bool ProceedPressed;
   char statusMessage[BUFSIZE];
   bool font_is_loaded[MAX_FONT_SIZE + 1];
   bool get_keypress_input, get_mouse_move_input;
} t_gl_state;


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

#ifdef X11

/*************************************************************
 * X11 Structure Definitions                                 *
 *************************************************************/

/* Structure used to store X Windows state variables.
 * display: Structure containing information needed to 
 *			communicate with Xlib.
 * screen_num: Value the Xlib server uses to identify every 
 *			   connected screen.
 * colors: Color indices paased back from X Windows.
 * private_cmap: "None" unless a private cmap was allocated.
 * toplevel, menu, textarea: Toplevel window and 2 child windows.
 * gc_normal, gc_xor, current_gc: Graphic contexts for drawing 
 *				in the graphics area. (gc_normal for overwrite 
 *				drawing, gc_xor for rubber band drawing, and 
 *				current_gc is the current gc used)
 * gc_menus: Graphic context for drawing in the status message
 *			 and menu area
 * font_info: Data for each font size.
 */
typedef struct {
   Display *display; 
   int screen_num; 
   int colors[NUM_COLOR];
   Colormap private_cmap; 
   Window toplevel, menu, textarea; 
   GC gc_normal, gc_xor, gc_menus, current_gc; 
   XFontStruct *font_info[MAX_FONT_SIZE+1]; 
} t_x11_state;

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
 * font_info: Data for each font size.
 */
typedef struct {
   bool InEventLoop;
   t_window_button_state windowAdjustFlag;
   int adjustButton;
   RECT adjustRect;
   HWND hMainWnd, hGraphicsWnd, hButtonsWnd, hStatusWnd;
   HDC hGraphicsDC;
   HPEN hGraphicsPen;
   HBRUSH hGraphicsBrush, hGrayBrush;
   HFONT hGraphicsFont;
   LOGFONT *font_info[MAX_FONT_SIZE+1];
} t_win32_state;

#endif


/*********************************************************************
 *        File scope variables.              *
 *********************************************************************/

// Need to initialize graphics_loaded to false, since checking it is 
// how we avoid multiple construction or destruction of the graphics
// window. Initializing display type and background color index is not 
// necessary since they are set in init_graphics(), but doing so
// just for safety.
static t_gl_state gl_state = {false, SCREEN, 0};

// Stores all the menu buttons created. Initialize the button pointer 
// and num_buttons for safety.
static t_button_state button_state = {NULL, 0};

// Contains all coordinates useful for graphics transformation
static t_transform_coordinates trans_coord;

// Initialize panning_enabled to false, so panning is not activated
static t_panning_state pan_state = {0, 0, false};

// Color references 
static const char *ps_cnames[NUM_COLOR] = {"white", "black", "grey55", "grey75",
		"blue", "green", "yellow", "cyan", "red", "pink", "lightpink", "darkgreen", 
		"magenta", "bisque", "lightskyblue", "thistle", "plum", "khaki", "coral",
		"turquoise", "mediumpurple", "darkslateblue", "darkkhaki"};

#ifdef X11

/*************************************************
*     X-Windows Specific File-scope Variables    *
**************************************************/

// Stores all state variables for X Windows
t_x11_state x11_state;

#endif /* X11 file scope variables */

#ifdef WIN32

/*****************************************************
 *  Microsoft Windows (Win32) File Scope Variables   * 
 *****************************************************/

/* Linestyle references for Win32. */
static const int win32_line_styles[2] = { PS_SOLID, PS_DASH };

/* Color references for Win32. Colors have to be specified by RGB values for Win32. */
static const COLORREF win32_colors[NUM_COLOR] = { RGB(255, 255, 255),
RGB(0, 0, 0), RGB(128, 128, 128), RGB(192, 192, 192), RGB(0, 0, 255),
RGB(0, 255, 0), RGB(255, 255, 0), RGB(0, 255, 255), RGB(255, 0, 0), RGB(255, 192, 203), 
RGB(255, 182, 193), RGB(0, 128, 0), RGB(255, 0, 255), RGB(255, 228, 196), RGB(135, 206, 250), 
RGB(216, 191, 216), RGB(221, 160, 221), RGB(240, 230, 140), RGB(255, 127, 80), 
RGB(64, 224, 208), RGB(147, 112, 219), RGB(72, 61, 139), RGB(189, 183, 107)};

/* Name of each window */
static TCHAR szAppName[256], szGraphicsName[] = TEXT("VPR Graphics"), 
			 szStatusName[] = TEXT("VPR Status"), szButtonsName[] = TEXT("VPR Buttons");

/* Stores all state variables for Win32 */
static t_win32_state win32_state = {false, WINDOW_DEACTIVATED, -1};

#endif /* WIN32 file scope variables */


/*********************************************
 *       Common Subroutine Declarations      *
 *********************************************/

static void *my_malloc(int ibytes);
static void *my_realloc(void *memblk, int ibytes);

/* translation from screen coordinates to the world coordinates *
 * used by the client program.									*/
static float xscrn_to_world(int x);
static float yscrn_to_world(int y); 
/* translation from world to screen coordinates */
static int xworld_to_scrn (float worldx);
static int yworld_to_scrn (float worldy);
/* translation from world to PostScript coordinates */
static float xworld_to_post(float worldx); 
static float yworld_to_post(float worldy);

static void force_setcolor(int cindex);
static void force_setlinestyle(int linestyle);
static void force_setlinewidth(int linewidth);
static void force_setfontsize (int pointsize);
static void load_font(int pointsize);

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
static void x11_init_graphics (const char* window_name, int cindex);
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
static void win32_init_graphics (const char* window_name, int cindex);

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

static void win32_handle_mousewheel_zooming(WPARAM wParam, LPARAM lParam);
static void win32_handle_button_info (t_event_buttonPressed &button_info, UINT message, 
									  WPARAM wParam);

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


/* Translates from world (client program) coordinates to screen coordinates
 * (pixels) in the x direction.  Add 0.5 at end for extra half-pixel accuracy. 
 */
static int xworld_to_scrn (float worldx) 
{
	int winx;
	
	winx = (int) ((worldx-trans_coord.xleft)*trans_coord.wtos_xmult + 0.5);

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
static int yworld_to_scrn (float worldy) 
{
	int winy;
	
	winy = (int) ((worldy-trans_coord.ytop)*trans_coord.wtos_ymult + 0.5);

#ifdef WIN32
	/* Avoid overflow in the X/Win32 Window routines. */
	winy = max (winy, MINPIXEL);
	winy = min (winy, MAXPIXEL);
#endif

	return (winy);
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


/* Sets the current graphics context colour to cindex, regardless of whether we think it is 
 * needed or not. 
 */
static void force_setcolor (int cindex) 
{
	gl_state.currentcolor = cindex;

	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XSetForeground (x11_state.display, x11_state.current_gc, x11_state.colors[cindex]);
#else /* Win32 */
		int win_linestyle, linewidth;
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = win32_colors[cindex];
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
		win32_state.hGraphicsBrush = CreateSolidBrush(win32_colors[gl_state.currentcolor]);
		if(!win32_state.hGraphicsBrush)
			WIN32_CREATE_ERROR();
		/* Select created brush into specified device context */
		if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsBrush))
			WIN32_SELECT_ERROR();
#endif
	}
	else {
		fprintf (gl_state.ps,"%s\n", ps_cnames[cindex]);
	}
}


/* Sets the current graphics context colour to cindex if it differs from the old colour */
void setcolor (int cindex) 
{
	if (gl_state.currentcolor != cindex) 
		force_setcolor (cindex);
   
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


int getcolor() {
	return gl_state.currentcolor;
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
		lb.lbColor = win32_colors[gl_state.currentcolor];
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
		lb.lbColor = win32_colors[gl_state.currentcolor];
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
static void force_setfontsize (int pointsize) 
{
	
	if (pointsize < 1) 
		pointsize = 1;

	if (pointsize > MAX_FONT_SIZE) 
		pointsize = MAX_FONT_SIZE;
	
	gl_state.currentfontsize = pointsize;
	
	if (gl_state.disp_type == SCREEN) {
			load_font (pointsize);
#ifdef X11
		XSetFont(x11_state.display, x11_state.current_gc, x11_state.font_info[pointsize]->fid); 
#else /* Win32 */
		/* Delete previously used font */
		if(!DeleteObject(win32_state.hGraphicsFont))
			WIN32_DELETE_ERROR();
		/* Create a new font */
		win32_state.hGraphicsFont = CreateFontIndirect(win32_state.font_info[pointsize]);
		if(!win32_state.hGraphicsFont)
			WIN32_CREATE_ERROR();
		/* Select created font into specified device context */
      	if(!SelectObject(win32_state.hGraphicsDC, win32_state.hGraphicsFont) )
		   WIN32_SELECT_ERROR();

#endif
	}
	
	else {
		/* PostScript:  set up font and centering function */
		fprintf(gl_state.ps,"%d setfontsize\n",pointsize);
	} 
}


/* For efficiency, this routine doesn't do anything if no change is  
 * implied.  If you want to force the graphics context or PS file    
 * to have font info set, call force_setfontsize (this is necessary  
 * in initialization and screen / Postscript switches).                
 */
void setfontsize (int pointsize) 
{
	
	if (pointsize != gl_state.currentfontsize) 
		force_setfontsize (pointsize);
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
		button_state.button[bnum].win = XCreateSimpleWindow(x11_state.display,x11_state.menu,
															button_state.button[bnum].xleft, 
															button_state.button[bnum].ytop, 
															button_state.button[bnum].width, 
															button_state.button[bnum].height, 0, 
															x11_state.colors[WHITE], 
															x11_state.colors[LIGHTGREY]); 
		XMapWindow (x11_state.display, button_state.button[bnum].win);
		XSelectInput (x11_state.display, button_state.button[bnum].win, ButtonPressMask);
#else
		button_state.button[bnum].hwnd = CreateWindow(TEXT("button"), 
													  TEXT(button_state.button[bnum].text),
													  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 
													  button_state.button[bnum].xleft, 
													  button_state.button[bnum].ytop,
													  button_state.button[bnum].width, 
													  button_state.button[bnum].height, 
													  win32_state.hButtonsWnd, (HMENU)(200+bnum), 
													  (HINSTANCE) GetWindowLong(win32_state.hMainWnd, 
																				GWL_HINSTANCE), 
													  NULL);
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

	for (i = 0; i<button_state.num_buttons;i++) 
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
   if (gl_state.initialized)  // Singleton graphics.
      return;

   reset_common_state ();

   gl_state.disp_type = SCREEN;
   gl_state.background_cindex = cindex;

#ifdef X11
   x11_init_graphics (window_name, cindex);
#else /* WIN32 */
   win32_init_graphics (window_name, cindex);
#endif

   gl_state.initialized = true;
}


static void reset_common_state () {
   gl_state.currentcolor = BLACK;
   gl_state.currentlinestyle = SOLID;
   gl_state.currentlinewidth = 0;
   gl_state.currentfontsize = 12;
   gl_state.current_draw_mode = DRAW_NORMAL;

   for (int i=0;i<=MAX_FONT_SIZE;i++) 
      gl_state.font_is_loaded[i] = false;     /* No fonts loaded yet. */

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
	
	while(!gl_state.ProceedPressed && GetMessage(&msg, NULL, 0, 0)) {
		//TranslateMessage(&msg);
		if (msg.message == WM_CHAR) { // only the top window can get keyboard events
			msg.hwnd = win32_state.hMainWnd;
		}
		DispatchMessage(&msg);
	}
	win32_state.InEventLoop = false;
#endif
}


void 
clearscreen (void) 
{
   int savecolor;
   if (gl_state.disp_type == SCREEN) {
#ifdef X11
      XClearWindow (x11_state.display, x11_state.toplevel);
#else /* Win32 */
      savecolor = gl_state.currentcolor;
      setcolor(gl_state.background_cindex);
      fillrect (trans_coord.xleft, trans_coord.ytop,
    		  	  trans_coord.xright, trans_coord.ybot);
      setcolor(savecolor);
#endif
   }
   else {  // Postscript
     /* erases current page.  Don't use erasepage, since this will erase *
      * everything, (even stuff outside the clipping path) causing       *
      * problems if this picture is incorporated into a larger document. */
      savecolor = gl_state.currentcolor;
      setcolor (gl_state.background_cindex);
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


void 
fillarc (float xc, float yc, float rad, float startang, float angextent) {
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


/* Draws text centered on xc,yc if it fits in boundx */
void 
drawtext (float xc, float yc, const char *text, float boundx) 
{
	int len, width, xw_off, yw_off, font_ascent, font_descent;
	
#ifdef X11
	len = strlen(text);
	width = XTextWidth(x11_state.font_info[gl_state.currentfontsize], text, len);
	font_ascent = x11_state.font_info[gl_state.currentfontsize]->ascent;
	font_descent = x11_state.font_info[gl_state.currentfontsize]->descent;
#else /* WC : WIN32 */
	SIZE textsize;
	TEXTMETRIC textmetric;
	
	if(SetTextColor(win32_state.hGraphicsDC, win32_colors[gl_state.currentcolor]) == CLR_INVALID)
		WIN32_DRAW_ERROR();
	
	len = strlen(text);
	if (!GetTextExtentPoint32(win32_state.hGraphicsDC, text, len, &textsize))
		WIN32_DRAW_ERROR();
	width = textsize.cx;
	if (!GetTextMetrics(win32_state.hGraphicsDC, &textmetric))
		WIN32_DRAW_ERROR();
	font_ascent = textmetric.tmAscent;
	font_descent = textmetric.tmDescent;
#endif	
	
	if (width > fabs(boundx*trans_coord.wtos_xmult)) 
		return; /* Don't draw if it won't fit */
	
	xw_off = (int)(width/(2.*trans_coord.wtos_xmult));      /* NB:  sign doesn't matter. */
	
	/* NB:  2 * descent makes this slightly conservative but simplifies code. */
	yw_off = (int)((font_ascent + 2 * font_descent)/(2.*trans_coord.wtos_ymult));
	
	/* Note:  text can be clipped when a little bit of it would be visible *
	* right now.  Perhaps X doesn't return extremely accurate width and   *
	* ascent values, etc?  Could remove this completely by multiplying    *
	* xw_off and yw_off by, 1.2 or 1.5.                                   */
	if (rect_off_screen(xc-xw_off, yc-yw_off, xc+xw_off, yc+yw_off)) 
		return;
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XDrawString(x11_state.display, x11_state.toplevel, x11_state.current_gc, 
					xworld_to_scrn(xc)-width/2, 
					yworld_to_scrn(yc)+(x11_state.font_info[gl_state.currentfontsize]->ascent 
										 -x11_state.font_info[gl_state.currentfontsize]->descent)/2,
					text, len);
#else /* Win32 */
		SetBkMode(win32_state.hGraphicsDC, TRANSPARENT); 
		if(!TextOut (win32_state.hGraphicsDC, xworld_to_scrn(xc)-width/2, 
						yworld_to_scrn(yc) - (font_ascent + font_descent)/2, text, len))
			WIN32_DRAW_ERROR();
#endif
	}
	else {
		fprintf(gl_state.ps,"(%s) %.2f %.2f censhow\n",text,xworld_to_post(xc),yworld_to_post(yc));
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


void 
init_world (float x1, float y1, float x2, float y2) 
{
	/* Sets the coordinate system the user wants to draw into.          */
	
	trans_coord.xleft = x1;
	trans_coord.xright = x2;
	trans_coord.ytop = y1;
	trans_coord.ybot = y2;
	
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
	int savefontsize, savecolor;
	float ylow;
#ifdef X11
	int len, width;
#endif
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XClearWindow (x11_state.display, x11_state.textarea);
		len = strlen (gl_state.statusMessage);
		width = XTextWidth(x11_state.font_info[MENU_FONT_SIZE], gl_state.statusMessage, len);
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[WHITE]);
		XDrawRectangle(x11_state.display, x11_state.textarea, x11_state.gc_menus, 0, 0,
						trans_coord.top_width - MWIDTH, T_AREA_HEIGHT);
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
		XDrawLine(x11_state.display, x11_state.textarea, x11_state.gc_menus, 0, T_AREA_HEIGHT-1,
					trans_coord.top_width-MWIDTH, T_AREA_HEIGHT-1);
		XDrawLine(x11_state.display, x11_state.textarea, x11_state.gc_menus, 
				  trans_coord.top_width-MWIDTH-1, 0, trans_coord.top_width-MWIDTH-1, 
				  T_AREA_HEIGHT-1);
		
		XDrawString(x11_state.display, x11_state.textarea, x11_state.gc_menus, 
			(trans_coord.top_width - MWIDTH - width)/2,
			T_AREA_HEIGHT/2 + (x11_state.font_info[MENU_FONT_SIZE]->ascent - 
			x11_state.font_info[MENU_FONT_SIZE]->descent)/2, gl_state.statusMessage, len);
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
		
		savecolor = gl_state.currentcolor;
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
	drawscreen();
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
	drawscreen();
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
				force_setcolor(gl_state.background_cindex);

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
		MessageBox(win32_state.hMainWnd, "Error initializing postscript output.", NULL, MB_OK);
#endif
	}
}


static void 
proceed (void (*drawscreen) (void)) 
{
	gl_state.ProceedPressed = true;
}


static void 
quit (void (*drawscreen) (void)) 
{
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
#ifdef X11
	int i;
	for (i=0;i<=MAX_FONT_SIZE;i++) {
		if (gl_state.font_is_loaded[i])
			XFreeFont(x11_state.display,x11_state.font_info[i]);
	}
	
	XFreeGC(x11_state.display,x11_state.gc_normal);
	XFreeGC(x11_state.display,x11_state.gc_xor);
	XFreeGC(x11_state.display,x11_state.gc_menus);
	
	if (x11_state.private_cmap != None) 
		XFreeColormap (x11_state.display, x11_state.private_cmap);
	
	XCloseDisplay(x11_state.display);
#else /* Win32 */
	int i;
   // Free the font data structure for each loaded font.
   for (i = 0; i <= MAX_FONT_SIZE; i++) {
      if (gl_state.font_is_loaded[i]) {
         free (win32_state.font_info[i]);
      }
   }

   // Destroy the window
	if(!DestroyWindow(win32_state.hMainWnd))
		WIN32_DRAW_ERROR();

   // free the window class (type information held by MS Windows) 
   // for each of the four window types we created.  Otherwise a 
   // later call to init_graphics to open the graphics window up again
   // will fail.
   if (!UnregisterClass (szAppName, GetModuleHandle(NULL)) )
      WIN32_DRAW_ERROR();
   if (!UnregisterClass (szGraphicsName, GetModuleHandle(NULL)) )
      WIN32_DRAW_ERROR();
   if (!UnregisterClass (szStatusName, GetModuleHandle(NULL)) )
      WIN32_DRAW_ERROR();
   if (!UnregisterClass (szButtonsName, GetModuleHandle(NULL)) )
      WIN32_DRAW_ERROR();
#endif

   free(button_state.button);
   button_state.button = NULL;

   for (i = 0; i <= MAX_FONT_SIZE; i++) {
      gl_state.font_is_loaded[i] = false;
#ifdef X11
	  x11_state.font_info[i] = NULL;
#else /* Win32 */
      win32_state.font_info[i] = NULL;
#endif
   }
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
	
	fprintf(gl_state.ps,"\n%%Solid and dashed line definitions:\n");
	fprintf(gl_state.ps,"/linesolid {[] 0 setdash} def\n");
	fprintf(gl_state.ps,"/linedashed {[3 3] 0 setdash} def\n");
	
	fprintf(gl_state.ps,"\n%%%%EndProlog\n");
	fprintf(gl_state.ps,"%%%%Page: 1 1\n\n");
	
	/* Set up PostScript graphics state to match current one. */
	force_setcolor (gl_state.currentcolor);
	force_setlinestyle (gl_state.currentlinestyle);
	force_setlinewidth (gl_state.currentlinewidth);
	force_setfontsize (gl_state.currentfontsize); 
	
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
	
	force_setcolor (gl_state.currentcolor);
	force_setlinestyle (gl_state.currentlinestyle);
	force_setlinewidth (gl_state.currentlinewidth);
	force_setfontsize (gl_state.currentfontsize); 
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
	
	x11_state.menu = XCreateSimpleWindow(x11_state.display,x11_state.toplevel, 
										 trans_coord.top_width-MWIDTH, 0, MWIDTH, 
										 trans_coord.display_height, 0, x11_state.colors[BLACK], 
										 x11_state.colors[LIGHTGREY]);
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
	strcpy(button_state.button[0].text, "U");
#endif
	button_state.button[0].fcn = translate_up;
	
	y1 += bwid + space;
	x1 = xcen - 3*bwid/2 - space;
	button_state.button[1].xleft = x1;
	button_state.button[1].ytop = y1;
#ifdef X11
	setpoly (1, bwid/2, bwid/2, bwid/3, PI);  /* Left */
#else
	button_state.button[1].type = BUTTON_TEXT;
	strcpy(button_state.button[1].text, "L");
#endif
	button_state.button[1].fcn = translate_left;
	
	x1 = xcen + bwid/2 + space;
	button_state.button[2].xleft = x1;
	button_state.button[2].ytop = y1;
#ifdef X11
	setpoly (2, bwid/2, bwid/2, bwid/3, 0);  /* Right */
#else
	button_state.button[2].type = BUTTON_TEXT;
	strcpy(button_state.button[2].text, "R");
#endif
	button_state.button[2].fcn = translate_right;
	
	y1 += bwid + space;
	x1 = xcen - bwid/2;
	button_state.button[3].xleft = x1;
	button_state.button[3].ytop = y1;
#ifdef X11
	setpoly (3, bwid/2, bwid/2, bwid/3, +PI/2.);  /* Down */
#else
	button_state.button[3].type = BUTTON_TEXT;
	strcpy(button_state.button[3].text, "D");
#endif
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


/* Makes sure the font of the specified size is loaded.  Point_size   *
* MUST be between 1 and MAX_FONT_SIZE. */
static void 
load_font(int pointsize) 
{
   if (pointsize > MAX_FONT_SIZE || pointsize < 1) {
      printf ("Error:  font size %d is out of valid range, 1 to %d.\n", 
              pointsize, MAX_FONT_SIZE);
      return;
   }

   if (gl_state.font_is_loaded[pointsize])  // Nothing to do.
		return;

#ifdef X11
   #define NUM_FONT_TYPES 3
   char fontname[NUM_FONT_TYPES][BUFSIZE];
   int ifont;
   bool success = false;

   /* Use proper point-size medium-weight upright helvetica font */
   // Exists on most X11 systems.
   // Backup font:  lucidasans, in the new naming style.
   sprintf(fontname[0],"-*-helvetica-medium-r-*--*-%d0-*-*-*-*-*-*",
             pointsize);
   sprintf(fontname[1], "lucidasans-%d", pointsize);
   sprintf(fontname[2],"-schumacher-clean-medium-r-*--*-%d0-*-*-*-*-*-*",
             pointsize);
	
   for (ifont = 0; ifont < NUM_FONT_TYPES; ifont++) {
#ifdef VERBOSE
      printf ("Loading font: point size: %d, fontname: %s\n",pointsize, 
               fontname[ifont]);
#endif
      /* Load font and get font information structure. */
      x11_state.font_info[pointsize] = XLoadQueryFont(x11_state.display,fontname[ifont]);
      if (x11_state.font_info[pointsize] == NULL) {
#ifdef VERBOSE
         fprintf( stderr, "Cannot open font %s\n", fontname[ifont]);
#endif
      }
      else {
         success = true;
         break; 
      }
   }
   if (!success) {
      printf ("Error in load_font:  cannot load any font of pointsize %d.\n",
               pointsize);
      printf ("Use xlsfonts to list available fonts, and modify load_font\n");
      printf ("in graphics.cpp.\n");
      exit (1);
   }
#else /* WIN32 */
   LOGFONT *lf = win32_state.font_info[pointsize] = (LOGFONT*)my_malloc(sizeof(LOGFONT));
   ZeroMemory(lf, sizeof(LOGFONT));
   /* lfHeight specifies the desired height of characters in logical units. *
    * A positive value of lfHeight will request a font that is appropriate  *
	* for a line spacing of lfHeight. On the other hand, setting lfHeight   *
	* to a negative value will obtain a font height that is compatible with *
	* the desired pointsize.												*/
   lf->lfHeight = -pointsize; 
   lf->lfWeight = FW_NORMAL;
   lf->lfCharSet = ANSI_CHARSET;
   lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
   lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
   lf->lfQuality = PROOF_QUALITY;
   lf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
   strcpy(lf->lfFaceName, "Arial");	 
#endif

   gl_state.font_is_loaded[pointsize] = true;
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
		SetWindowText(button_state.button[bnum].hwnd, new_button_text);
#endif
	}
}


/**********************************
* X-Windows Specific Definitions *
*********************************/
#ifdef X11

/* Helper function called by init_graphics(). Not visible to client program. */
static void x11_init_graphics (const char *window_name, int cindex)
{
   char *display_name = NULL;
   int x, y;                       /* window position */
   unsigned int border_width = 2;  /* ignored by OpenWindows */
   XTextProperty windowName;
   
   /* X Windows' names for my colours. */
   const char *cnames[NUM_COLOR] = {"white", "black", "grey55", "grey75", "blue", 
   	"green", "yellow", "cyan", "red", "pink", "lightpink", "RGBi:0.0/0.39/0.0", 
	"magenta", "bisque", "lightskyblue", "thistle", "plum", "khaki", "coral",
	"turquoise", "mediumpurple", "darkslateblue", "darkkhaki" };
	
   XColor exact_def;
   Colormap cmap;
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
   
   cmap = DefaultColormap(x11_state.display, x11_state.screen_num);
   x11_state.private_cmap = None;
	
   for (int i=0;i<NUM_COLOR;i++) {
      if (!XParseColor(x11_state.display,cmap,cnames[i],&exact_def)) {
         fprintf(stderr, "Color name %s not in database", cnames[i]);
         exit(-1);
      }
      if (!XAllocColor(x11_state.display, cmap, &exact_def)) {
         fprintf(stderr, "Couldn't allocate color %s.\n",cnames[i]); 
   		
         if (x11_state.private_cmap == None) {
            fprintf(stderr, "Will try to allocate a private colourmap.\n");
            fprintf(stderr, "Colours will only display correctly when your "
                            "cursor is in the graphics window.\n"
                            "Exit other colour applications and rerun this "
                            "program if you don't like that.\n\n");
				
            x11_state.private_cmap = XCopyColormapAndFree (x11_state.display, cmap);
            cmap = x11_state.private_cmap;
            if (!XAllocColor (x11_state.display, cmap, &exact_def)) {
               fprintf (stderr, "Couldn't allocate color %s as private.\n",
                                 cnames[i]);
               exit (1);
            }
         }
			
         else {
            fprintf (stderr, "Couldn't allocate color %s as private.\n",
               cnames[i]);
            exit (1);
         }
      }
      x11_state.colors[i] = exact_def.pixel;
   }  // End setting up colours
	
   x11_state.toplevel = XCreateSimpleWindow(x11_state.display,
											RootWindow(x11_state.display,x11_state.screen_num),
              								x, y, trans_coord.top_width, trans_coord.top_height,
              								border_width, x11_state.colors[BLACK], 
											x11_state.colors[cindex]);

   if (x11_state.private_cmap != None) 
      XSetWindowColormap (x11_state.display, x11_state.toplevel, x11_state.private_cmap);
	
   XSelectInput (x11_state.display, x11_state.toplevel, ExposureMask | StructureNotifyMask |
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                    KeyPressMask);
	
   /* Create default Graphics Contexts.  valuemask = 0 -> use defaults. */
   x11_state.current_gc = x11_state.gc_normal = XCreateGC(x11_state.display, x11_state.toplevel, 
												   valuemask, &values);
   x11_state.gc_menus = XCreateGC(x11_state.display, x11_state.toplevel, valuemask, &values);
	
   /* Create XOR graphics context for Rubber Banding */
   values.function = GXxor;   
   values.foreground = x11_state.colors[cindex];
   x11_state.gc_xor = XCreateGC(x11_state.display, x11_state.toplevel, (GCFunction | GCForeground),
                     &values);
	
   /* specify font for menus.  */
   load_font(MENU_FONT_SIZE);
   XSetFont(x11_state.display, x11_state.gc_menus, x11_state.font_info[MENU_FONT_SIZE]->fid);
	
   /* Set drawing defaults for user-drawable area.  Use whatever the *
   * initial values of the current stuff was set to.                */
   force_setfontsize(gl_state.currentfontsize);
   force_setcolor (gl_state.currentcolor);
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
}


/* Helper function called by event_loop(). Not visible to client program. */
static void 
x11_event_loop (void (*act_on_mousebutton)(float x, float y, t_event_buttonPressed button_info),
				void (*act_on_mousemove)(float x, float y), 
				void (*act_on_keypress)(char key_pressed),
				void (*drawscreen) (void))
{
	XEvent report;
	int bnum;
	float x, y;
	
#define OFF 1
#define ON 0
	
	x11_turn_on_off (ON);
	while (1) {
		XNextEvent (x11_state.display, &report);
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
				/* t_event_buttonPressed is used as a structure for storing information about a	   *
				 * button press event. This information can be passed back to and used by a client *
				 * program.																		   */
				x11_handle_button_info(&button_info, report.xbutton.button, report.xbutton.state);
#ifdef VERBOSE
			if (button_info.shift_pressed == true)
				printf("Shift is pressed at button press.\n");
			if (button_info.ctrl_pressed == true)
				printf("Ctrl is pressed at button press.\n");
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
					handle_zoom_in(x, y, drawscreen);
					break;
				case Button5:  /* Scroll wheel rotated backward; screen does zoom_out */
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
			   	   T_AREA_HEIGHT, 0, x11_state.colors[BLACK], x11_state.colors[LIGHTGREY]);
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
	if (event_ptr->type == Expose) {
		return (True);
	}
	
	return (False);
}


/* draws text center at xc, yc -- used only by menu drawing stuff */
static void menutext(Window win, int xc, int yc, const char *text) 
{
	int len, width; 
	
	len = strlen(text);
	width = XTextWidth(x11_state.font_info[MENU_FONT_SIZE], text, len);
	XDrawString(x11_state.display, win, x11_state.gc_menus, xc-width/2, 
				yc + (x11_state.font_info[MENU_FONT_SIZE]->ascent 
					   - x11_state.font_info[MENU_FONT_SIZE]->descent)/2,
				text, len);
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
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[WHITE]);
		XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, x, y+1, x+width, y+1);
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
		XDrawLine(x11_state.display, x11_state.menu, x11_state.gc_menus, x, y, x+width, y);
		return;
	}

	ispressed = button_state.button[bnum].ispressed;
	thick = 2;
	/* Draw top and left edges of 3D box. */
	if (ispressed) {
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
	}
	else {
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[WHITE]);
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
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[WHITE]);
	}
	else {
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
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
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[DARKGREY]);
	}
	else {
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[LIGHTGREY]);
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
		XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
		XFillPolygon(x11_state.display,button_state.button[bnum].win,
					 x11_state.gc_menus,mypoly,3,Convex,CoordModeOrigin);
	}
	
	/* Draw text, if there is any */
	if (button_state.button[bnum].type == BUTTON_TEXT) {
		if (button_state.button[bnum].enabled)
			XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
		else
			XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[DARKGREY]);
		menutext(button_state.button[bnum].win,button_state.button[bnum].width/2,
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
	XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[WHITE]);
	XDrawRectangle(x11_state.display, x11_state.menu, x11_state.gc_menus, 0, 0, MWIDTH, 
				   trans_coord.top_height);
	XSetForeground(x11_state.display, x11_state.gc_menus,x11_state.colors[BLACK]);
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
win32_init_graphics (const char *window_name, int cindex)
{
   WNDCLASS wndclass;
   HINSTANCE hInstance = GetModuleHandle(NULL);
   int x, y;
   LOGBRUSH lb;
   lb.lbStyle = BS_SOLID;
   lb.lbColor = win32_colors[gl_state.currentcolor];
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
	
   /* Grab the Application name */
   wsprintf(szAppName, TEXT(window_name));
	
   //win32_state.hGraphicsPen = CreatePen(win32_line_styles[SOLID], 1, win32_colors[BLACK]);
   win32_state.hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win32_line_styles[gl_state.currentlinestyle] 
										    | PS_ENDCAP_FLAT, 1, &lb, (LONG)NULL, NULL);
   if(!win32_state.hGraphicsPen)
      WIN32_CREATE_ERROR();
   win32_state.hGraphicsBrush = CreateSolidBrush(win32_colors[DARKGREY]); 
   if(!win32_state.hGraphicsBrush)
      WIN32_CREATE_ERROR();
   win32_state.hGrayBrush = CreateSolidBrush(win32_colors[LIGHTGREY]);
   if(!win32_state.hGrayBrush)
      WIN32_CREATE_ERROR();

   load_font (gl_state.currentfontsize);
   win32_state.hGraphicsFont = CreateFontIndirect(win32_state.font_info[gl_state.currentfontsize]);
   if (!win32_state.hGraphicsFont)
      WIN32_CREATE_ERROR();
	
   /* Register the Main Window class */
   wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wndclass.lpfnWndProc = WIN32_MainWND;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hInstance;
   wndclass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
   wndclass.hCursor = LoadCursor( NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) CreateSolidBrush(win32_colors[cindex]);
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = szAppName;
	
   if (!RegisterClass(&wndclass)) {
      printf ("Error code: %d\n", GetLastError());
      MessageBox(NULL, TEXT("Initialization of Windows graphics (init_graphics) failed."),
                    szAppName, MB_ICONERROR);
      exit(-1);
   }
	
   /* Register the Graphics Window class */
   wndclass.lpfnWndProc = WIN32_GraphicsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szGraphicsName;
	
   if(!RegisterClass(&wndclass))
      WIN32_DRAW_ERROR();
	
   /* Register the Status Window class */
   wndclass.lpfnWndProc = WIN32_StatusWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szStatusName;
   wndclass.hbrBackground = win32_state.hGrayBrush;
   
   if(!RegisterClass(&wndclass))
      WIN32_DRAW_ERROR();
	
   /* Register the Buttons Window class */
   wndclass.lpfnWndProc = WIN32_ButtonsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szButtonsName;
   wndclass.hbrBackground = win32_state.hGrayBrush;
	
   if (!RegisterClass(&wndclass))
      WIN32_DRAW_ERROR();
	
   win32_state.hMainWnd = CreateWindow(szAppName, TEXT(window_name),
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
			win32_state.hStatusWnd = CreateWindow(szStatusName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
				0, 0, 0, 0, hwnd, (HMENU) 102, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			win32_state.hButtonsWnd = CreateWindow(szButtonsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
				0, 0, 0, 0, hwnd, (HMENU) 103, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
			win32_state.hGraphicsWnd = CreateWindow(szGraphicsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
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

			win32_handle_mousewheel_zooming(wParam, lParam);
			return 0;
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
			gl_state.currentcolor = BLACK;
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
			if(!DeleteObject(win32_state.hGraphicsFont))
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
			hDotPen = CreatePen(PS_DASH, 1, win32_colors[gl_state.background_cindex]);
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

			if(!DrawText(hdc, TEXT(gl_state.statusMessage), -1, &rect, 
						 DT_CENTER | DT_VCENTER | DT_SINGLELINE))
				WIN32_DRAW_ERROR();
		
			if(!EndPaint(hwnd, &ps))
				WIN32_DRAW_ERROR();
			return 0;
		
		case WM_SIZE:
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
			hBrush = CreateSolidBrush(win32_colors[LIGHTGREY]);
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
	char msg[BUFSIZE];
	sprintf (msg, "Error %i: Couldn't select graphics object on line %d of graphics.c\n", 
			 GetLastError(), __LINE__); 

	MessageBox(NULL, msg, NULL, MB_OK); 
	exit(-1); 
}


static void WIN32_DELETE_ERROR()
{ 
	char msg[BUFSIZE]; 
	sprintf (msg, "Error %i: Couldn't delete graphics object on line %d of graphics.c\n", 
			 GetLastError(), __LINE__); 
	
	MessageBox(NULL, msg, NULL, MB_OK); 
	exit(-1); 
}


static void WIN32_CREATE_ERROR() 
{ 
	char msg[BUFSIZE]; 
	sprintf (msg, "Error %i: Couldn't create graphics object on line %d of graphics.c\n", 
			 GetLastError(), __LINE__); 
	
	MessageBox(NULL, msg, NULL, MB_OK); 
	exit(-1); 
}


static void WIN32_DRAW_ERROR()
{ 
	char msg[BUFSIZE]; 
	sprintf (msg, "Error %i: Couldn't draw graphics object on line %d of graphics.c\n", 
			 GetLastError(), __LINE__); 

	MessageBox(NULL, msg, NULL, MB_OK); 
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


static void win32_handle_mousewheel_zooming(WPARAM wParam, LPARAM lParam) 
{
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

	int i;
	for (i=0; i<roll_detent; i++) {
		// Positive value for zDelta indicates that the wheel was rotated forward, which
		// will trigger zoom_in. Otherwise, zoom_out is called.
		if (zDelta > 0)
			handle_zoom_in(xscrn_to_world(mousePos.x), yscrn_to_world(mousePos.y), 
									win32_drawscreen_ptr);
		else
			handle_zoom_out(xscrn_to_world(mousePos.x), yscrn_to_world(mousePos.y), 
									win32_drawscreen_ptr);
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
void close_graphics (void) { }
void update_message (const char *msg) { }
void draw_message (void) { }
void init_world (float xl, float yt, float xr, float yb) { }
void flushinput (void) { }
void setcolor (int cindex) { }
int getcolor (void) { return 0; }
void setlinestyle (int linestyle) { }
void setlinewidth (int linewidth) { }
void setfontsize (int pointsize) { }
void drawline (float x1, float y1, float x2, float y2) { }
void drawrect (float x1, float y1, float x2, float y2) { }
void fillrect (float x1, float y1, float x2, float y2) { }
void fillpoly (t_point *points, int npoints) { }
void drawarc (float xcen, float ycen, float rad, float startang,
			  float angextent) { }
void drawellipticarc (float xc, float yc, float radx, float rady, 
					  float startang, float angextent) { }

void fillarc (float xcen, float ycen, float rad, float startang,
			  float angextent) { }
void fillellipticarc (float xc, float yc, float radx, float rady, 
					  float startang, float angextent) { }

void drawtext (float xc, float yc, const char *text, float boundx) { }
void clearscreen (void) { }

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

#ifdef WIN32
void win32_drawcurve(t_point *points, int npoints) { }

void win32_fillcurve(t_point *points, int npoints) { }


#endif  // WIN32 (subset of commands)

#endif  // NO_GRAPHICS
