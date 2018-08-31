#ifndef GRAPHICS_STATE_H
#define GRAPHICS_STATE_H

#include "easygl_constants.h"
#include "fontcache.h"
#include "graphics_types.h"

/**********************************
 * Common Preprocessor Directives *
 **********************************/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sys/timeb.h>
#include <math.h>

// Set X11 by default, if neither NO_GRAPHICS nor WIN32 are defined
#ifndef NO_GRAPHICS
#ifndef WIN32
#ifndef X11
#define X11
#endif
#endif // !WIN32
#endif // !NO_GRAPHICS

#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <cairo.h>
#include <cairo-xlib.h>
#endif

#if defined(WIN32) || defined(CYGWIN)
#include <windows.h>
#include <windowsx.h>
#endif

#define BUFSIZE 1000

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
class t_x11_state {
public:
    t_x11_state();
    ~t_x11_state();

    Display *display = nullptr;
    int screen_num;
    Window toplevel, menu, textarea;
    GC gc_normal, gc_xor, gc_menus, current_gc;
    XftDraw *toplevel_draw = nullptr,
            *menu_draw = nullptr,
            *textarea_draw = nullptr;
    XftColor xft_menutextcolor;
    XftColor xft_currentcolor;
    XVisualInfo visual_info;
    Colormap colormap_to_use;

    // single multi-buffering variables
    Drawable* draw_area = nullptr;
    Pixmap draw_buffer;
    XftDraw *draw_buffer_draw = nullptr,
            *draw_area_draw = nullptr;

    // window attributes here so I only have to call XGetWindowAttributes on resize
    XWindowAttributes attributes;

    // Cairo related things
    cairo_surface_t *cairo_surface = nullptr;
    cairo_t *ctx = nullptr;

    static t_x11_state *getInstance();
private:
    // Pointer to the most recently constructed state. Is set to NULL upon any destruction
    static t_x11_state *instance;
};

#elif defined(WIN32)

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
class t_win32_state {
public:
	t_win32_state();
	t_win32_state(bool, t_window_button_state, int);
    ~t_win32_state();

    bool InEventLoop;
    t_window_button_state windowAdjustFlag;
    int adjustButton;
    RECT adjustRect;
    HWND hMainWnd, hGraphicsWnd, hButtonsWnd, hStatusWnd; // <Addition/Mod: Charles>
    HDC hGraphicsDC;	// Current Active Drawing buffer // <Addition/Mod: Charles>
	HDC hGraphicsDCPassive;	// Front Buffer for display purpose // <Addition/Mod: Charles>
	HGDIOBJ hGraphicsPassive;	// Backup of old drawing context object, usage see set_drawing_buffer() // <Addition/Mod: Charles>
    HPEN hGraphicsPen;
    HBRUSH hGraphicsBrush, hGrayBrush;
    HFONT hGraphicsFont;

    static t_win32_state *getInstance();

private:
    // Pointer to the most recently constructed state. Is set to NULL upon any destruction
    static t_win32_state *instance;
};

#endif // WIN32


/*************************************************************
 * Operating System Independent                              *
 *************************************************************/

/* Used to define where the output of drawscreen (graphics primitives in the user-controlled
 * area) currently goes to: the screen or a postscript file.
 */
typedef enum {
    SCREEN = 0,
    POSTSCRIPT = 1
} t_display_type;

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
 *        Using integer degrees to avoid huge numbers of distinct rotations
 *        that cause many (slow) font loads.
 * current_draw_to: OFF_SCREEN buffer or ON_SCREEN (window). Controls double buffering.
 * current_draw_mode: select DRAW_NORMAL (for overwrite) or DRAW_XOR (for rubber-banding)
 * ps: for PostScript output
 * ProceedPressed: whether the Proceed button has been pressed
 * statusMessage: user message to display
 * font_info: a font cache
 * get_keypress_input: whether keypresses are sent back to callback functions
 * get_mouse_move_input: whether mouse movements are sent back to callback functions
 * redraw_needed: true if there has been an expose or other event that requires a
 * redraw, but we haven't done one yet.
 * disable_event_loop: set for automarking of student assignments where
 *                     you don't want to hang in the event_loop
 * redirect_to_postscript: when set, a call to event_loop will instead draw to
 *                         postscript. Useful for automarking.
 *
 * Need to initialize graphics_loaded to false, since checking it is
 * how we avoid multiple construction or destruction of the graphics
 * window. Initializing display type and background color index is not
 * necessary since they are set in init_graphics(), but doing so
 * just for safety.
 */
struct t_gl_state {
    bool initialized = false;
    t_display_type disp_type = SCREEN;
    t_color background_color = t_color(0xFF, 0xFF, 0xCC);
    t_color foreground_color = BLACK;
    int currentlinestyle = SOLID;
    int currentlinecap = 0;
    int currentlinewidth = 0;
    int currentfontsize = 12;
    int currentfontrotation = 0;
    t_coordinate_system currentcoordinatesystem = GL_WORLD;
    t_draw_to current_draw_to = ON_SCREEN;
    e_draw_mode current_draw_mode = DRAW_NORMAL;
    FILE *ps = nullptr;
    bool ProceedPressed = false;
    char statusMessage[BUFSIZE] = "";
    FontCache font_info;
    bool get_keypress_input = false;
    bool get_mouse_move_input = false;
    bool redraw_needed = false;
    bool disable_event_loop = false;
    bool redirect_to_postscript = false;
    float zoom_factor = 1.6667;
};

#endif // GRAPHICS_STATE_H
