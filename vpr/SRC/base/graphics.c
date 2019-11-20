/*
* Easygl Version 2.0.1
* Written by Vaughn Betz at the University of Toronto, Department of       *
* Electrical and Computer Engineering, with additions by Paul Leventis     *
* and William Chow of Altera, and Guy Lemieux of the University of         *
* Brish Columbia.                                                          *
* All rights reserved by U of T, etc.                                      *
*                                                                          *
* You may freely use this graphics interface for non-commercial purposes   *
* as long as you leave the author info above in it.
*                                                                          *
* Revision History:                                                        *
*                                                                          *
* V2.0.1 Sept. 2012 (Vaughn Betz)
* - Fixed a bug in Win32 where postscript output would make the graphics
*   crash when you redrew.
* - Made a cleaner makefile to simplify platform selection.
* - Commented and reorganized some of the code.  Started cleaning up some of the
*   win32 code.  Win32 looks inefficient; it is saving and updating graphics contexts 
*   all the time even though we know when the context is valid vs. not-valid.
*   TODO: make win32 work more like X11 (minimize gc updates).
* 
* V2.0:  Nov. 21, 2011 (Vaughn Betz)
* - Updated example code, and some cleanup and bug fixes to win32 code.
* - Removed some win32 code that had no X11 equivalent or wasn't well
*   documented.
* - Used const char * where appropriate to get rid of g++ warnings.
* - Made interface to things like xor drawing more consistent, and added to
*   example program.
* - Made a simpler (easygl.cpp) interface to the graphics library for
*   use by undergraduate students.
* 
* V1.06 : July 23, 2003 : (Guy Lemieux)
* - added some typecasts to cleanly compile with g++ and MS c++ tools
* - if WIN32 not defined, it defines X11 automatically
* - fixed X11 compilation; WIN32 broke some things
*
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
* - Added invalidate_screen() call to graphics.c that in turn calls        *
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

#ifndef NO_GRAPHICS  // Strip everything out and just put in stubs if NO_GRAPHICS defined

/**********************************
 * Common Preprocessor Directives *
 **********************************/

#define TRUE 1
#define FALSE 0

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include "graphics.h"
using namespace std;


#if defined(X11) || defined(WIN32)

/* Macros for translation from world to PostScript coordinates */
#define XPOST(worldx) (((worldx)-xleft)*ps_xmult + ps_left)
#define YPOST(worldy) (((worldy)-ybot)*ps_ymult + ps_bot)

/* Macros to convert from X Windows Internal Coordinates to my  *
* World Coordinates.  (This macro is used only rarely, so       *
* the divides don't hurt speed).                               */
#define XTOWORLD(x) (((float) x)*xdiv + xleft)
#define YTOWORLD(y) (((float) y)*ydiv + ytop)

#ifndef max
#define max(a,b) (((a) > (b))? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) > (b)? (b) : (a))
#endif

#define MWIDTH			104 /* width of menu window */
#define T_AREA_HEIGHT	24  /* Height of text window */
#define MAX_FONT_SIZE	24  /* Largest point size of text.   */
                            // Some computers only have up to 24 point 
#define PI				3.141592654

#define BUTTON_TEXT_LEN	100
#define BUFSIZE			1000
#endif

/*********************************************
* X-Windows Specific Preprocessor Directives *
*********************************************/
#ifdef X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

/* Uncomment the line below if your X11 header files don't define XPointer */
/* typedef char *XPointer;                                                 */

// Really large pixel values cna make some X11 implementations draw crazy things
// (internal overflow in the X11 library).  Use these constants to clip. 
#define MAXPIXEL 15000   
#define MINPIXEL -15000 

#endif /* X11 Preprocessor Directives */


/*************************************************************
 * Microsoft Windows (WIN32) Specific Preprocessor Directives *
 *************************************************************/
#ifdef WIN32
#pragma warning(disable : 4996)   // Turn off annoying warnings about strcmp.

#include <windows.h>

// Lines below are for displaying errors in a message box on windows.
#define SELECT_ERROR() { char msg[BUFSIZE]; sprintf (msg, "Error %i: Couldn't select graphics object on line %d of graphics.c\n", GetLastError(), __LINE__); MessageBox(NULL, msg, NULL, MB_OK); exit(-1); }
#define DELETE_ERROR() { char msg[BUFSIZE]; sprintf (msg, "Error %i: Couldn't delete graphics object on line %d of graphics.c\n", GetLastError(), __LINE__); MessageBox(NULL, msg, NULL, MB_OK); exit(-1); }
#define CREATE_ERROR() { char msg[BUFSIZE]; sprintf (msg, "Error %i: Couldn't create graphics object on line %d of graphics.c\n", GetLastError(), __LINE__); MessageBox(NULL, msg, NULL, MB_OK); exit(-1); }
#define DRAW_ERROR()   { char msg[BUFSIZE]; sprintf (msg, "Error %i: Couldn't draw graphics object on line %d of graphics.c\n", GetLastError(), __LINE__); MessageBox(NULL, msg, NULL, MB_OK); exit(-1); }

/* Avoid funny clipping problems under windows that I suspect are caused by round-off 
 * in the Win32 libraries.
 */
#define MAXPIXEL 3000
#define MINPIXEL -3000

#define DEGTORAD(x) ((x)/180.*PI)
#define FONTMAG 1.3
#endif /* Win32 preprocessor Directives */


/*************************************************************
 * Common Type Definitions                                   *
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


/* Structure used to store overall graphics state variables.
 * TODO: Gradually move more file scope variables in here.
 * initialized:  true if the graphics window & state have been 
 *      created and initialized, false otherwise.
 * disp_type: Selects SCREEN or POSTSCRIPT 
 * background_cindex: index of the window (or page for PS) background colour
 */
typedef struct {
   bool initialized;
   int disp_type;
   int background_cindex;
} t_gl_state;


/*********************************************************************
 *        File scope variables.  TODO: group in structs              *
 *********************************************************************/

// Need to initialize graphics_loaded to false, since checking it is 
// how we avoid multiple construction or destruction of the graphics
// window.
static t_gl_state gl_state = {false, SCREEN, 0};

static const int menu_font_size = 12;   /* Font for menus and dialog boxes. */

static t_button *button = NULL;                 /* [0..num_buttons-1] */
static int num_buttons = 0;                  /* Number of menu buttons */

static int display_width, display_height;  /* screen size */
static int top_width, top_height;      /* window size */
static float xleft, xright, ytop, ybot;         /* world coordinates */
static float saved_xleft, saved_xright, saved_ytop, saved_ybot; 

static float ps_left, ps_right, ps_top, ps_bot; /* Figure boundaries for *
* PostScript output, in PostScript coordinates.  */
static float ps_xmult, ps_ymult;     /* Transformation for PostScript. */
static float xmult, ymult;                  /* Transformation factors */
static float xdiv, ydiv;

static int currentcolor;
static int currentlinestyle;
static int currentlinewidth;
static int currentfontsize;
static e_draw_mode current_draw_mode;

/* For PostScript output */
static FILE *ps;

static int ProceedPressed;

static char statusMessage[BUFSIZE] = ""; /* User message to display */

static bool font_is_loaded[MAX_FONT_SIZE + 1];
static bool get_keypress_input, get_mouse_move_input;
static const char *ps_cnames[NUM_COLOR] = {"white", "black", "grey55", "grey75",
		"blue", "green", "yellow", "cyan", "red", "darkgreen", "magenta",
		"bisque", "lightblue", "thistle", "plum", "khaki", "coral",
		"turquoise", "mediumpurple", "darkslateblue", "darkkhaki"};


/*********************************************
 *       Common Subroutine Declarations      *
 *********************************************/

static void *my_malloc(int ibytes);
static void *my_realloc(void *memblk, int ibytes);
static int xcoord (float worldx);
static int ycoord (float worldy);
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
static void zoom_in (void (*drawscreen) (void));
static void zoom_out (void (*drawscreen) (void));
static void zoom_fit (void (*drawscreen) (void));
static void adjustwin (void (*drawscreen) (void)); 
static void postscript (void (*drawscreen) (void));
static void proceed (void (*drawscreen) (void));
static void quit (void (*drawscreen) (void)); 
static void map_button (int bnum); 
static void unmap_button (int bnum); 

#ifdef X11

/*************************************************
*     X-Windows Specific File-scope Variables    *
**************************************************/
static Display *display;
static int screen_num;
static GC gc, gcxor, gc_menus, current_gc;
static XFontStruct *font_info[MAX_FONT_SIZE+1]; /* Data for each size */
static Window toplevel, menu, textarea;  /* various windows */
static Colormap private_cmap; /* "None" unless a private cmap was allocated. */

/* Color indices passed back from X Windows. */
static int colors[NUM_COLOR];


/****************************************************
*     X-Windows Specific Subroutine Declarations    *
*****************************************************/

static Bool test_if_exposed (Display *disp, XEvent *event_ptr, 
							 XPointer dummy);
static void build_textarea (void);
static void drawbut (int bnum);
static int which_button (Window win);

static void turn_on_off (int pressed);
static void drawmenu(void);

#endif /* X11 Declarations */



#ifdef WIN32

/*****************************************************
 *  Microsoft Windows (Win32) File Scope Variables   * 
 *****************************************************/
static const int win32_line_styles[2] = { PS_SOLID, PS_DASH };

static const COLORREF win32_colors[NUM_COLOR] = { RGB(255, 255, 255),
RGB(0, 0, 0), RGB(128, 128, 128), RGB(192, 192, 192), RGB(0, 0, 255),
RGB(0, 255, 0), RGB(255, 255, 0), RGB(0, 255, 255), RGB(255, 0, 0), RGB(0, 128, 0),
RGB(255, 0, 255), RGB(255, 228, 196), RGB(173, 216, 230), RGB(216, 191, 216), RGB(221, 160, 221),
RGB(240, 230, 140), RGB(255, 127, 80), RGB(64, 224, 208), RGB(147, 112, 219), RGB(72, 61, 139),
RGB(189, 183, 107)};

static TCHAR szAppName[256], 
szGraphicsName[] = TEXT("VPR Graphics"), 
szStatusName[] = TEXT("VPR Status"),
szButtonsName[] = TEXT("VPR Buttons");
static HPEN hGraphicsPen;
static HBRUSH hGraphicsBrush, hGrayBrush;
static HDC hGraphicsDC, hForegroundDC, hBackgroundDC,
hCurrentDC, /* WC : double-buffer */
hObjtestDC, hAllObjtestDC; /* object test */

/* WC */
static HFONT hGraphicsFont;
static LOGFONT *font_info[MAX_FONT_SIZE+1]; /* Data for each size */

/* Handles to the top level window and 3 subwindows. */
static HWND hMainWnd, hGraphicsWnd, hButtonsWnd, hStatusWnd;

static int cxClient, cyClient;

/* These are used for the "Window" graphics button. They keep track of whether we're entering
 * the window rectangle to zoom to, etc.
 */
static int windowAdjustFlag = 0, adjustButton = -1;
static RECT adjustRect, updateRect;

static boolean InEventLoop = FALSE;

//static HBITMAP buttonImages[4];


/*******************************************************************
 *    Win32-specific subroutine declarations                       *
 *******************************************************************/

/* Callback functions for the top-level window and 3 sub-windows.  
 * Windows uses an odd mix of events and callbacks, so it needs these.
 */
static LRESULT CALLBACK GraphicsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK StatusWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ButtonsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK MainWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


// For Win32, need to save pointers to these callback functions at file
// scope, since windows has a bizarre event loop structure where you poll
// for events, but then dispatch the event and get  called via a callback 
// from windows (GraphicsWND, below).  I can't figure out why windows 
// does things this way, but it is what makes saving these function pointers
// necessary.  VB.

static void (*mouseclick_ptr)(float x, float y);
static void (*mousemove_ptr)(float x, float y);
static void (*keypress_ptr)(char entered_char);
static void (*drawscreen_ptr)(void);

static void invalidate_screen();  

static void reset_win32_state ();
static void win32_drain_message_queue ();

void drawtoscreen();
void displaybuffer();


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


/* Translates from my internal coordinates to real-world coordinates  *
* in the x direction.  Add 0.5 at end for extra half-pixel accuracy. */
static int xcoord (float worldx) 
{
	int winx;
	
	winx = (int) ((worldx-xleft)*xmult + 0.5);
	
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
	
	return (winx);
}


/* Translates from my internal coordinates to real-world coordinates  *
* in the y direction.  Add 0.5 at end for extra half-pixel accuracy. */
static int ycoord (float worldy) 
{
	int winy;
	
	winy = (int) ((worldy-ytop)*ymult + 0.5);
	
	/* Avoid overflow in the X/Win32 Window routines. */
	winy = max (winy, MINPIXEL);
	winy = min (winy, MAXPIXEL);
	
	return (winy);
}


#ifdef WIN32 
static void invalidate_screen(void)
{
/* Tells the graphics engine to redraw the graphics display since information has changed */

	if(!InvalidateRect(hGraphicsWnd, NULL, FALSE))
		DRAW_ERROR();
	if(!UpdateWindow(hGraphicsWnd))
		DRAW_ERROR();
}
#endif

/* Sets the current graphics context colour to cindex, regardless of whether we think it is 
 * needed or not. 
 */
static void force_setcolor (int cindex) 
{
	currentcolor = cindex;

	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XSetForeground (display, current_gc, colors[cindex]);
#else /* Win32 */
		int win_linestyle;
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = win32_colors[cindex];
		lb.lbHatch = (LONG)NULL;
		win_linestyle = win32_line_styles[currentlinestyle];

		if(!DeleteObject(hGraphicsPen))
			DELETE_ERROR();

		int linewidth = max (currentlinewidth, 1);  // Win32 won't draw 0 width dashed lines.
		hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);
		if(!hGraphicsPen)
			CREATE_ERROR();
		
		if(!DeleteObject(hGraphicsBrush)) 
			DELETE_ERROR();
		hGraphicsBrush = CreateSolidBrush(win32_colors[currentcolor]);
		if(!hGraphicsBrush)
			CREATE_ERROR();
#endif
	}
	else {
		fprintf (ps,"%s\n", ps_cnames[cindex]);
	}
}


/* Sets the current graphics context colour to cindex if it differs from the old colour */
void setcolor (int cindex) 
{
	if (currentcolor != cindex) 
		force_setcolor (cindex);
   
}

 
/* Sets the current graphics context color to the index that corresponds to the
 * string name passed in.  Slower, but maybe more convenient for simple 
 * client code.
 */
void setcolor (string cname) {
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
	return currentcolor;
}

/* Sets the current linestyle to linestyle in the graphics context.
 * Note SOLID is 0 and DASHED is 1 for linestyle. 
 */
static void force_setlinestyle (int linestyle) 
{
   currentlinestyle = linestyle;
	
	if (gl_state.disp_type == SCREEN) {

#ifdef X11
		static int x_vals[2] = {LineSolid, LineOnOffDash};
		XSetLineAttributes (display, current_gc, currentlinewidth, x_vals[linestyle],
			CapButt, JoinMiter);
#else  // Win32
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = win32_colors[currentcolor];
		lb.lbHatch = (LONG)NULL;
		int win_linestyle = win32_line_styles[linestyle];
		
		if(!DeleteObject(hGraphicsPen))
			DELETE_ERROR();
		int linewidth = max (currentlinewidth, 1);  // Win32 won't draw 0 width dashed lines.
		hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);
		if(!hGraphicsPen)
			CREATE_ERROR();
#endif
	}

	else {  
		if (linestyle == SOLID) 
		   fprintf (ps,"linesolid\n");
		else if (linestyle == DASHED)
			fprintf (ps, "linedashed\n");
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
	if (linestyle != currentlinestyle) 
		force_setlinestyle (linestyle);
}


/* Sets current linewidth in the graphics context.
 * linewidth should be greater than or equal to 0 to make any sense. 
 */
static void force_setlinewidth (int linewidth) 
{	
	currentlinewidth = linewidth;
	
	if (gl_state.disp_type == SCREEN) {

#ifdef X11
		static int x_vals[2] = {LineSolid, LineOnOffDash};
		XSetLineAttributes (display, current_gc, linewidth, x_vals[currentlinestyle],
			CapButt, JoinMiter);
#else /* Win32 */
		LOGBRUSH lb;
		lb.lbStyle = BS_SOLID;
		lb.lbColor = win32_colors[currentcolor];
		lb.lbHatch = (LONG)NULL;
		int win_linestyle = win32_line_styles[currentlinestyle];

		if(!DeleteObject(hGraphicsPen)) 
			DELETE_ERROR();
		if (linewidth == 0) 
			linewidth = 1;  // Win32 won't draw dashed 0 width lines.
		hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win_linestyle |
			PS_ENDCAP_FLAT, linewidth, &lb, (LONG)NULL, NULL);
		if(!hGraphicsPen)
			CREATE_ERROR();
#endif
	}

	else {
		fprintf(ps,"%d setlinewidth\n", linewidth);
	}
}


/* Sets the linewidth in the grahpics context, if it differs from the current value.
 */
void setlinewidth (int linewidth) 
{
	if (linewidth != currentlinewidth)
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
#ifdef WIN32
	pointsize = (int)((float)pointsize * FONTMAG);
#endif
	if (pointsize > MAX_FONT_SIZE) 
		pointsize = MAX_FONT_SIZE;
	
	currentfontsize = pointsize;
	
	if (gl_state.disp_type == SCREEN) {
			load_font (pointsize);
#ifdef X11
		XSetFont(display, current_gc, font_info[pointsize]->fid); 
#else /* Win32 */
		if(!DeleteObject(hGraphicsFont))
			DELETE_ERROR();
		hGraphicsFont = CreateFontIndirect(font_info[pointsize]);
		if(!hGraphicsFont)
			CREATE_ERROR();
      		if(!SelectObject(hGraphicsDC, hGraphicsFont) )
		   SELECT_ERROR();

#endif
	}
	
	else {
		/* PostScript:  set up font and centering function */
		fprintf(ps,"%d setfontsize\n",pointsize);
	} 
}


/* For efficiency, this routine doesn't do anything if no change is  
 * implied.  If you want to force the graphics context or PS file    
 * to have font info set, call force_setfontsize (this is necessary  
 * in initialization and X11 / Postscript switches).                
 */
void setfontsize (int pointsize) 
{
	
	if (pointsize != currentfontsize) 
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
	
	button[bnum].type = BUTTON_POLY;
	for (i=0;i<3;i++) {
		button[bnum].poly[i][0] = (int) (xc + r*cos(theta) + 0.5);
		button[bnum].poly[i][1] = (int) (yc + r*sin(theta) + 0.5);
		theta += (float)(2*PI/3);
	}
}
#endif // X11


/* Maps a button onto the screen and set it up for input, etc.        */
static void map_button (int bnum) 
{
	button[bnum].ispressed = 0;

	if (button[bnum].type != BUTTON_SEPARATOR) {
#ifdef X11
		button[bnum].win = XCreateSimpleWindow(display,menu,
			button[bnum].xleft, button[bnum].ytop, button[bnum].width, 
			button[bnum].height, 0, colors[WHITE], colors[LIGHTGREY]); 
		XMapWindow (display, button[bnum].win);
		XSelectInput (display, button[bnum].win, ButtonPressMask);
#else
		button[bnum].hwnd = CreateWindow( TEXT("button"), TEXT(button[bnum].text),
			WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, button[bnum].xleft, button[bnum].ytop,
			button[bnum].width, button[bnum].height, hButtonsWnd, (HMENU)(200+bnum), 
			(HINSTANCE) GetWindowLong(hMainWnd, GWL_HINSTANCE), NULL);
		if(!InvalidateRect(hButtonsWnd, NULL, TRUE))
			DRAW_ERROR();
		if(!UpdateWindow(hButtonsWnd))
			DRAW_ERROR();
#endif
	}
	else {   // Separator, not a button.
#ifdef X11
		button[bnum].win = -1;
#else // WIN32
		button[bnum].hwnd = NULL;
		if(!InvalidateRect(hButtonsWnd, NULL, TRUE))
			DRAW_ERROR();
		if(!UpdateWindow(hButtonsWnd))
			DRAW_ERROR();
#endif
	}
}


static void unmap_button (int bnum) 
{
	/* Unmaps (removes) a button from the screen.        */
	if (button[bnum].type != BUTTON_SEPARATOR) {
#ifdef X11
		XUnmapWindow (display, button[bnum].win);
#else
		if(!DestroyWindow(button[bnum].hwnd))
			DRAW_ERROR();
		if(!InvalidateRect(hButtonsWnd, NULL, TRUE))
			DRAW_ERROR();
		if(!UpdateWindow(hButtonsWnd))
			DRAW_ERROR();
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
      
	for (i=0; i < num_buttons;i++) {
		if (button[i].type == BUTTON_TEXT && 
			strcmp (button[i].text, prev_button_text) == 0) {
			bnum = i + 1;
			break;
		}
	}
	
	if (bnum == -1) {
		printf ("Error in create_button:  button with text %s not found.\n",
			prev_button_text);
		exit (1);
	}
	
	button = (t_button *) my_realloc (button, (num_buttons+1) * sizeof (t_button));
	/* NB:  Requirement that you specify the button that this button goes under *
	* guarantees that button[num_buttons-2] exists and is a text button.       */
	
   /* Special string to make a separator. */
	if (!strncmp(button_text, "---", 3)) {
		bheight = 2;
	   button_type = BUTTON_SEPARATOR;
	}
	else
		bheight = 26;

	for (i=num_buttons;i>bnum;i--) {
		button[i].xleft = button[i-1].xleft;
		button[i].ytop = button[i-1].ytop + bheight + space;
		button[i].height = button[i-1].height;
		button[i].width = button[i-1].width;
		button[i].type = button[i-1].type;
		strcpy (button[i].text, button[i-1].text);
		button[i].fcn = button[i-1].fcn;
		button[i].ispressed = button[i-1].ispressed;
		button[i].enabled = button[i-1].enabled;
		unmap_button (i-1);
	}

	i = bnum;
	button[i].xleft = 6;
	button[i].ytop = button[i-1].ytop + button[i-1].height + space;
	button[i].height = bheight;
	button[i].width = 90;
	button[i].type = button_type;
	strncpy (button[i].text, button_text, BUTTON_TEXT_LEN);
	button[i].fcn = button_func;
	button[i].ispressed = false;
	button[i].enabled = true;

	num_buttons++;

	for (i = 0; i<num_buttons;i++) 
		map_button (i);
}


/* Destroys the button with text button_text. */
void
destroy_button (const char *button_text) 
{
	int i, bnum, space, bheight;
	
	bnum = -1;
	space = 8;
	for (i = 0; i < num_buttons; i++) {
		if (button[i].type == BUTTON_TEXT && 
			strcmp (button[i].text, button_text) == 0) {
			bnum = i;
			break;
		}
	}
	
	if (bnum == -1) {
		printf ("Error in destroy_button:  button with text %s not found.\n",
			button_text);
		exit (1);
	}

	bheight = button[bnum].height;

	for (i=bnum+1;i<num_buttons;i++) {
		button[i-1].xleft = button[i].xleft;
		button[i-1].ytop = button[i].ytop - bheight - space;
		button[i-1].height = button[i].height;
		button[i-1].width = button[i].width;
		button[i-1].type = button[i].type;
		strcpy (button[i-1].text, button[i].text);
		button[i-1].fcn = button[i].fcn;
		button[i-1].ispressed = button[i].ispressed;
		button[i-1].enabled = button[i].enabled;
		unmap_button (i);
	}
	unmap_button(bnum);

	button = (t_button *) my_realloc (button, (num_buttons-1) * sizeof (t_button));

	num_buttons--;

	for (i=bnum; i<num_buttons;i++) 
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
#ifdef WIN32
   reset_win32_state ();
#endif

   gl_state.disp_type = SCREEN;
   gl_state.background_cindex = cindex;


#ifdef X11
   char *display_name = NULL;
   int x, y;                                   /* window position */
   unsigned int border_width = 2;  /* ignored by OpenWindows */
   XTextProperty windowName;
   
   /* X Windows' names for my colours. */
   const char *cnames[NUM_COLOR] = {"white", "black", "grey55", "grey75", "blue", 
   	"green", "yellow", "cyan", "red", "RGBi:0.0/0.5/0.0", "magenta",
   	"bisque", "lightblue", "thistle", "plum", "khaki", "coral",
	"turquoise", "mediumpurple", "darkslateblue", "darkkhaki" };
	
   XColor exact_def;
   Colormap cmap;
   unsigned long valuemask = 0; /* ignore XGCvalues and use defaults */
   XGCValues values;
   XEvent event;

   /* connect to X server */
   if ( (display=XOpenDisplay(display_name)) == NULL )
   {
      fprintf( stderr, "Cannot connect to X server %s\n",
   		XDisplayName(display_name));
      exit( -1 );
   }
	
   /* get screen size from display structure macro */
   screen_num = DefaultScreen(display);
   display_width = DisplayWidth(display, screen_num);
   display_height = DisplayHeight(display, screen_num);
   
   x = 0;
   y = 0;
	
   top_width = 2 * display_width / 3;
   top_height = 4 * display_height / 5;
   
   cmap = DefaultColormap(display, screen_num);
   private_cmap = None;
	
   for (int i=0;i<NUM_COLOR;i++) {
      if (!XParseColor(display,cmap,cnames[i],&exact_def)) {
         fprintf(stderr, "Color name %s not in database", cnames[i]);
         exit(-1);
      }
      if (!XAllocColor(display, cmap, &exact_def)) {
         fprintf(stderr, "Couldn't allocate color %s.\n",cnames[i]); 
   		
         if (private_cmap == None) {
            fprintf(stderr, "Will try to allocate a private colourmap.\n");
            fprintf(stderr, "Colours will only display correctly when your "
                            "cursor is in the graphics window.\n"
                            "Exit other colour applications and rerun this "
                            "program if you don't like that.\n\n");
				
            private_cmap = XCopyColormapAndFree (display, cmap);
            cmap = private_cmap;
            if (!XAllocColor (display, cmap, &exact_def)) {
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
      colors[i] = exact_def.pixel;
   }  // End setting up colours
	
   toplevel = XCreateSimpleWindow(display,RootWindow(display,screen_num),
              x, y, top_width, top_height, border_width, colors[BLACK],
              colors[cindex]);  

   if (private_cmap != None) 
      XSetWindowColormap (display, toplevel, private_cmap);
	
   XSelectInput (display, toplevel, ExposureMask | StructureNotifyMask |
                    ButtonPressMask | PointerMotionMask | KeyPressMask);
	
   /* Create default Graphics Contexts.  valuemask = 0 -> use defaults. */
   current_gc = gc = XCreateGC(display, toplevel, valuemask, &values);
   gc_menus = XCreateGC(display, toplevel, valuemask, &values);
	
   /* Create XOR graphics context for Rubber Banding */
   values.function = GXxor;   
   values.foreground = colors[cindex];
   gcxor = XCreateGC(display, toplevel, (GCFunction | GCForeground),
                     &values);
	
   /* specify font for menus.  */
   load_font(menu_font_size);
   XSetFont(display, gc_menus, font_info[menu_font_size]->fid);
	
   /* Set drawing defaults for user-drawable area.  Use whatever the *
   * initial values of the current stuff was set to.                */
   force_setfontsize(currentfontsize);
   force_setcolor (currentcolor);
   force_setlinestyle (currentlinestyle);
   force_setlinewidth (currentlinewidth);
	
   // Need a non-const name to pass to XStringListTo... 
   // (even though X11 won't change it).
  char *window_name_copy = (char *) my_malloc (BUFSIZE * sizeof (char));
  strncpy (window_name_copy, window_name, BUFSIZE);
   XStringListToTextProperty(&window_name_copy, 1, &windowName);
   free (window_name_copy);
   window_name_copy = NULL;

   XSetWMName (display, toplevel, &windowName);
   /* XSetWMIconName (display, toplevel, &windowName); */
	
   /* XStringListToTextProperty copies the window_name string into            *
    * windowName.value.  Free this memory now.                                */
	
   free (windowName.value);  
	
   XMapWindow (display, toplevel);
   build_textarea ();
   build_default_menu ();
	
   /* The following is completely unnecessary if the user is using the       *
    * interactive (event_loop) graphics.  It waits for the first Expose      *
    * event before returning so that I can tell the window manager has got   *
    * the top-level window up and running.  Thus the user can start drawing  *
    * into this window immediately, and there's no danger of the window not  *
    * being ready and output being lost.                                     */
   XPeekIfEvent (display, &event, test_if_exposed, NULL); 
	
#else /* WIN32 */
   WNDCLASS wndclass;
   HINSTANCE hInstance = GetModuleHandle(NULL);
   int x, y;
   LOGBRUSH lb;
   lb.lbStyle = BS_SOLID;
   lb.lbColor = win32_colors[currentcolor];
   lb.lbHatch = (LONG)NULL;
   x = 0;
   y = 0;
	
   /* get screen size from display structure macro */
   display_width = GetSystemMetrics( SM_CXSCREEN );
   if (!(display_width))
      CREATE_ERROR();
   display_height = GetSystemMetrics( SM_CYSCREEN );
   if (!(display_height))
      CREATE_ERROR();
   top_width = 2*display_width/3;
   top_height = 4*display_height/5;
	
   /* Grab the Application name */
   wsprintf(szAppName, TEXT(window_name));
	
   //hGraphicsPen = CreatePen(win32_line_styles[SOLID], 1, win32_colors[BLACK]);
   hGraphicsPen = ExtCreatePen(PS_GEOMETRIC | win32_line_styles[currentlinestyle] |
                  PS_ENDCAP_FLAT, 1, &lb, (LONG)NULL, NULL);
   if(!hGraphicsPen)
      CREATE_ERROR();
   hGraphicsBrush = CreateSolidBrush(win32_colors[DARKGREY]); 
   if(!hGraphicsBrush)
      CREATE_ERROR();
   hGrayBrush = CreateSolidBrush(win32_colors[LIGHTGREY]);
   if(!hGrayBrush)
      CREATE_ERROR();

   load_font (currentfontsize);
   hGraphicsFont = CreateFontIndirect(font_info[currentfontsize]);
   if (!hGraphicsFont)
      CREATE_ERROR();
	
   /* Register the Main Window class */
   wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   wndclass.lpfnWndProc = MainWND;
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
   wndclass.lpfnWndProc = GraphicsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szGraphicsName;
	
   if(!RegisterClass(&wndclass))
      DRAW_ERROR();
	
   /* Register the Status Window class */
   wndclass.lpfnWndProc = StatusWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szStatusName;
   wndclass.hbrBackground = hGrayBrush;
   
   if(!RegisterClass(&wndclass))
      DRAW_ERROR();
	
   /* Register the Buttons Window class */
   wndclass.lpfnWndProc = ButtonsWND;
   wndclass.hIcon = NULL;
   wndclass.lpszClassName = szButtonsName;
   wndclass.hbrBackground = hGrayBrush;
	
   if (!RegisterClass(&wndclass))
      DRAW_ERROR();
	
   hMainWnd = CreateWindow(szAppName, TEXT(window_name),
                 WS_OVERLAPPEDWINDOW, x, y, top_width, top_height,
                 NULL, NULL, hInstance, NULL);
	
   if(!hMainWnd)
      DRAW_ERROR();
	
   /* Set drawing defaults for user-drawable area.  Use whatever the *
    * initial values of the current stuff was set to.                */
	
   if (ShowWindow(hMainWnd, SW_SHOWNORMAL))
      DRAW_ERROR();
   build_default_menu();
   if (!UpdateWindow(hMainWnd))
      DRAW_ERROR();
   win32_drain_message_queue ();
#endif
   gl_state.initialized = true;
}


static void reset_common_state () {
   currentcolor = BLACK;
   currentlinestyle = SOLID;
   currentlinewidth = 0;
   currentfontsize = 12;
   current_draw_mode = DRAW_NORMAL;

   for (int i=0;i<=MAX_FONT_SIZE;i++) 
      font_is_loaded[i] = false;     /* No fonts loaded yet. */

   ProceedPressed = false;
   get_keypress_input = false;
   get_mouse_move_input = false;
}


static void 
update_transform (void) 
{
/* Set up the factors for transforming from the user world to X Windows *
	* coordinates.                                                         */
	
	float mult, y1, y2, x1, x2;
	
	/* X Window coordinates go from (0,0) to (width-1,height-1) */
	xmult = (top_width - 1 - MWIDTH) / (xright - xleft);
	ymult = (top_height - 1 - T_AREA_HEIGHT)/ (ybot - ytop);

	/* Need to use same scaling factor to preserve aspect ratio */
	if (fabs(xmult) <= fabs(ymult)) {
		mult = (float)(fabs(ymult/xmult));
		y1 = ytop - (ybot-ytop)*(mult-1)/2;
		y2 = ybot + (ybot-ytop)*(mult-1)/2;
		ytop = y1;
		ybot = y2;
	}
	else {
		mult = (float)(fabs(xmult/ymult));
		x1 = xleft - (xright-xleft)*(mult-1)/2;
		x2 = xright + (xright-xleft)*(mult-1)/2;
		xleft = x1;
		xright = x2;
	}
	xmult = (top_width - 1 - MWIDTH) / (xright - xleft);
	ymult = (top_height - 1 - T_AREA_HEIGHT)/ (ybot - ytop);

	xdiv = 1/xmult;
	ydiv = 1/ymult;
}


static void 
update_ps_transform (void) 
{
	
/* Postscript coordinates start at (0,0) for the lower left hand corner *
* of the page and increase upwards and to the right.  For 8.5 x 11     *
* sheet, coordinates go from (0,0) to (612,792).  Spacing is 1/72 inch.*
* I'm leaving a minimum of half an inch (36 units) of border around    *
	* each edge.                                                           */
	
	float ps_width, ps_height;
	
	ps_width = 540.;    /* 72 * 7.5 */
	ps_height = 720.;   /* 72 * 10  */
	
	ps_xmult = ps_width / (xright - xleft);
	ps_ymult = ps_height / (ytop - ybot);
	/* Need to use same scaling factor to preserve aspect ratio.   *
	* I show exactly as much on paper as the screen window shows, *
	* or the user specifies.                                      */
	if (fabs(ps_xmult) <= fabs(ps_ymult)) {  
		ps_left = 36.;
		ps_right = (float)(36. + ps_width);
		ps_bot = (float)(396. - fabs(ps_xmult * (ytop - ybot))/2);
		ps_top = (float)(396. + fabs(ps_xmult * (ytop - ybot))/2);
		/* Maintain aspect ratio but watch signs */
		ps_ymult = (ps_xmult*ps_ymult < 0) ? -ps_xmult : ps_xmult;
	}
	else {  
		ps_bot = 36.;
		ps_top = (float)(36. + ps_height);
		ps_left = (float)(306. - fabs(ps_ymult * (xright - xleft))/2);
		ps_right = (float)(306. + fabs(ps_ymult * (xright - xleft))/2);
		/* Maintain aspect ratio but watch signs */
		ps_xmult = (ps_xmult*ps_ymult < 0) ? -ps_ymult : ps_ymult;
	}
}


/* The program's main event loop.  Must be passed a user routine        
* drawscreen which redraws the screen.  It handles all window resizing 
* zooming etc. itself.  If the user clicks a mousebutton in the graphics    
* (toplevel) area, the act_on_mousebutton routine passed in is called.      
*/
void 
event_loop (void (*act_on_mousebutton)(float x, float y), 
			void (*act_on_mousemove)(float x, float y), 
			void (*act_on_keypress)(char key_pressed),
			void (*drawscreen) (void)) 
{
#ifdef X11
	XEvent report;
	int bnum;
	float x, y;
	
#define OFF 1
#define ON 0
	
	turn_on_off (ON);
	while (1) {
		XNextEvent (display, &report);
		switch (report.type) {  
		case Expose:
#ifdef VERBOSE 
			printf("Got an expose event.\n");
			printf("Count is: %d.\n",report.xexpose.count);
			printf("Window ID is: %d.\n",report.xexpose.window);
#endif
			if (report.xexpose.count != 0)
				break;
			if (report.xexpose.window == menu)
				drawmenu(); 
			else if (report.xexpose.window == toplevel)
				drawscreen();
			else if (report.xexpose.window == textarea)
				draw_message();
			break;
		case ConfigureNotify:
			top_width = report.xconfigure.width;
			top_height = report.xconfigure.height;
			update_transform();
			drawmenu();
			draw_message();
#ifdef VERBOSE 
			printf("Got a ConfigureNotify.\n");
			printf("New width: %d  New height: %d.\n",top_width,top_height);
#endif
			break; 
		case ButtonPress:
#ifdef VERBOSE 
			printf("Got a buttonpress.\n");
			printf("Window ID is: %d.\n",report.xbutton.window);
#endif
			if (report.xbutton.window == toplevel) {
				x = XTOWORLD(report.xbutton.x);
				y = YTOWORLD(report.xbutton.y); 
				act_on_mousebutton (x, y);
			} 
			else {  /* A menu button was pressed. */
				bnum = which_button(report.xbutton.window);
#ifdef VERBOSE 
				printf("Button number is %d\n",bnum);
#endif
				if (button[bnum].enabled) {
					button[bnum].ispressed = 1;
					drawbut(bnum);
					XFlush(display);  /* Flash the button */
					button[bnum].fcn (drawscreen);
					button[bnum].ispressed = 0;
					drawbut(bnum);
					if (button[bnum].fcn == proceed) {
						turn_on_off(OFF);
						flushinput ();
						return;  /* Rather clumsy way of returning *
						* control to the simulator       */
					}
				}
			}
			break;
		case MotionNotify:
#ifdef VERBOSE 
			printf("Got a MotionNotify Event.\n");
			printf("x: %d    y: %d\n",report.xmotion.x,report.xmotion.y);
#endif
			if (get_mouse_move_input &&
			    report.xmotion.x <= top_width-MWIDTH &&
			    report.xmotion.y <= top_height-T_AREA_HEIGHT)
				act_on_mousemove(XTOWORLD(report.xmotion.x), YTOWORLD(report.xmotion.y));
			break;
		case KeyPress:
#ifdef VERBOSE 
			printf("Got a KeyPress Event.\n");
#endif
			if (get_keypress_input)
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
#else /* Win32 */
	MSG msg;
	
	mouseclick_ptr = act_on_mousebutton;
	mousemove_ptr = act_on_mousemove;
	keypress_ptr = act_on_keypress;
	drawscreen_ptr = drawscreen;
	ProceedPressed = FALSE;
	InEventLoop = TRUE;
	
	invalidate_screen();
	
	while(!ProceedPressed && GetMessage(&msg, NULL, 0, 0)) {
		//TranslateMessage(&msg);
		if (msg.message == WM_CHAR) { // only the top window can get keyboard events
			msg.hwnd = hMainWnd;
		}
		DispatchMessage(&msg);
	}
	InEventLoop = FALSE;
#endif
}

void 
clearscreen (void) 
{
   int savecolor;
   if (gl_state.disp_type == SCREEN) {
#ifdef X11
      XClearWindow (display, toplevel);
#else /* Win32 */
      savecolor = currentcolor;
      setcolor(gl_state.background_cindex);
      fillrect (xleft, ytop, xright, ybot);
      setcolor(savecolor);
#endif
   }
   else {  // Postscript
     /* erases current page.  Don't use erasepage, since this will erase *
      * everything, (even stuff outside the clipping path) causing       *
      * problems if this picture is incorporated into a larger document. */
      savecolor = currentcolor;
      setcolor (gl_state.background_cindex);
      fprintf(ps,"clippath fill\n\n");
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
	
	xmin = min (xleft, xright);
	if (x1 < xmin && x2 < xmin) 
		return (1);
	
	xmax = max (xleft, xright);
	if (x1 > xmax && x2 > xmax)
		return (1);
	
	ymin = min (ytop, ybot);
	if (y1 < ymin && y2 < ymin)
		return (1);
	
	ymax = max (ytop, ybot);
	if (y1 > ymax && y2 > ymax)
		return (1);
	
	return (0);
}

void 
drawline (float x1, float y1, float x2, float y2) 
{
/* Draw a line from (x1,y1) to (x2,y2) in the user-drawable area. *
	* Coordinates are in world (user) space.                         */
	
#ifdef WIN32
	HPEN hOldPen;
#endif
	
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		/* Xlib.h prototype has x2 and y1 mixed up. */ 
		XDrawLine(display, toplevel, current_gc, xcoord(x1), ycoord(y1), xcoord(x2), ycoord(y2));
#else /* Win32 */
		hOldPen = (HPEN)SelectObject(hGraphicsDC, hGraphicsPen);
		if(!(hOldPen))
			SELECT_ERROR();
		if (!BeginPath(hGraphicsDC))
			DRAW_ERROR();
		if(!MoveToEx (hGraphicsDC, xcoord(x1), ycoord(y1), NULL))
			DRAW_ERROR();
		if(!LineTo (hGraphicsDC, xcoord(x2), ycoord(y2)))
			DRAW_ERROR();
		if (!EndPath(hGraphicsDC))
			DRAW_ERROR();
		if (!StrokePath(hGraphicsDC))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps,"%.2f %.2f %.2f %.2f drawline\n",XPOST(x1),YPOST(y1),
			XPOST(x2),YPOST(y2));
	}
}

/* (x1,y1) and (x2,y2) are diagonally opposed corners, in world coords. */
void 
drawrect (float x1, float y1, float x2, float y2) 
{
	int xw1, yw1, xw2, yw2;
#ifdef WIN32
	HPEN hOldPen;
	HBRUSH hOldBrush;
#else
	unsigned int width, height;
	int xl, yt;
#endif
	
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) { 
		/* translate to X Windows calling convention. */
		xw1 = xcoord(x1);
		xw2 = xcoord(x2);
		yw1 = ycoord(y1);
		yw2 = ycoord(y2); 
#ifdef X11
		xl = min(xw1,xw2);
		yt = min(yw1,yw2);
		width = abs (xw1-xw2);
		height = abs (yw1-yw2);
		XDrawRectangle(display, toplevel, current_gc, xl, yt, width, height);
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
		
		hOldPen = (HPEN)SelectObject(hGraphicsDC, hGraphicsPen);
		if(!(hOldPen))
			SELECT_ERROR();
		hOldBrush = (HBRUSH)SelectObject(hGraphicsDC, GetStockObject(NULL_BRUSH));
		if(!(hOldBrush))
			SELECT_ERROR();
		if(!Rectangle(hGraphicsDC, xw1, yw1, xw2, yw2))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
		if(!SelectObject(hGraphicsDC, hOldBrush))
			SELECT_ERROR();
#endif
		
	}
	else {
		fprintf(ps,"%.2f %.2f %.2f %.2f drawrect\n",XPOST(x1),YPOST(y1),
			XPOST(x2),YPOST(y2));
	}
}


/* (x1,y1) and (x2,y2) are diagonally opposed corners in world coords. */
void 
fillrect (float x1, float y1, float x2, float y2) 
{
	int xw1, yw1, xw2, yw2;
#ifdef WIN32
	HPEN hOldPen;
	HBRUSH hOldBrush;
#else
	unsigned int width, height;
	int xl, yt;
#endif
	
	if (rect_off_screen(x1,y1,x2,y2))
		return;
	
	if (gl_state.disp_type == SCREEN) {
		/* translate to X Windows calling convention. */
		xw1 = xcoord(x1);
		xw2 = xcoord(x2);
		yw1 = ycoord(y1);
		yw2 = ycoord(y2); 
#ifdef X11
		xl = min(xw1,xw2);
		yt = min(yw1,yw2);
		width = abs (xw1-xw2);
		height = abs (yw1-yw2);
		XFillRectangle(display, toplevel, current_gc, xl, yt, width, height);
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
		
		hOldPen = (HPEN)SelectObject(hGraphicsDC, hGraphicsPen);
		if(!(hOldPen))
			SELECT_ERROR();	
		hOldBrush = (HBRUSH)SelectObject(hGraphicsDC, hGraphicsBrush);
		if(!(hOldBrush))
			SELECT_ERROR();
		if(!Rectangle(hGraphicsDC, xw1, yw1, xw2, yw2))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
		if(!SelectObject(hGraphicsDC, hOldBrush))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps,"%.2f %.2f %.2f %.2f fillrect\n",XPOST(x1),YPOST(y1),
			XPOST(x2),YPOST(y2));
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
#ifdef WIN32
	HPEN hOldPen;
        int p1, p2, p3, p4;
#endif
	
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
		xl = (int) (xcoord(xc) - fabs(xmult*radx));
		yt = (int) (ycoord(yc) - fabs(ymult*rady));
		width = (unsigned int) (2*fabs(xmult*radx));
		height = (unsigned int) (2*fabs(ymult*rady));
#ifdef X11
		XDrawArc (display, toplevel, current_gc, xl, yt, width, height,
			(int) (startang*64), (int) (angextent*64));
#else  // Win32
		/* set arc direction */
		if (angextent > 0) {
			p1 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang)));
			p2 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang)));
			p3 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang+angextent-.001)));
			p4 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang+angextent-.001)));
		}
		else {
			p1 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang+angextent+.001)));
			p2 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang+angextent+.001)));
			p3 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang)));
			p4 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang)));
		}
		
		hOldPen = (HPEN)SelectObject(hGraphicsDC, hGraphicsPen);
		if(!(hOldPen))
			SELECT_ERROR();		
		if(!Arc(hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps, "gsave\n");
		fprintf(ps, "%.2f %.2f translate\n", XPOST(xc), YPOST(yc));
		fprintf(ps, "%.2f 1 scale\n", fabs(radx*ps_xmult)/fabs(rady*ps_ymult));
		fprintf(ps, "0 0 %.2f %.2f %.2f %s\n", /*XPOST(xc)*/ 
			/*YPOST(yc)*/ fabs(rady*ps_xmult), startang, startang+angextent, 
			(angextent < 0) ? "drawarcn" : "drawarc") ;
		fprintf(ps, "grestore\n");
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
#ifdef WIN32
	HPEN hOldPen;
	HBRUSH hOldBrush;
   int p1, p2, p3, p4;
#endif
	
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
		xl = (int) (xcoord(xc) - fabs(xmult*radx));
		yt = (int) (ycoord(yc) - fabs(ymult*rady));
		width = (unsigned int) (2*fabs(xmult*radx));
		height = (unsigned int) (2*fabs(ymult*rady));
#ifdef X11
		XFillArc (display, toplevel, current_gc, xl, yt, width, height,
			(int) (startang*64), (int) (angextent*64));
#else  // Win32
		/* set pie direction */
		if (angextent > 0) {
			p1 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang)));
			p2 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang)));
			p3 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang+angextent-.001)));
			p4 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang+angextent-.001)));
		}
		else {
			p1 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang+angextent+.001)));
			p2 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang+angextent+.001)));
			p3 = (int)(xcoord(xc) + fabs(xmult*radx)*cos(DEGTORAD(startang)));
			p4 = (int)(ycoord(yc) - fabs(ymult*rady)*sin(DEGTORAD(startang)));
		}
	
		hOldPen = (HPEN)SelectObject(hGraphicsDC, GetStockObject(NULL_PEN));
		if(!(hOldPen))
			SELECT_ERROR();
		hOldBrush = (HBRUSH)SelectObject(hGraphicsDC, hGraphicsBrush);
		if(!(hOldBrush))
			SELECT_ERROR();
// Win32 API says a zero return value indicates an error, but it seems to always
// return zero.  Don't check for an error on Pie.
      Pie(hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4);

//		if(!Pie(hGraphicsDC, xl, yt, xl+width, yt+height, p1, p2, p3, p4));
//			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
		if(!SelectObject(hGraphicsDC, hOldBrush))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps, "gsave\n");
		fprintf(ps, "%.2f %.2f translate\n", XPOST(xc), YPOST(yc));
		fprintf(ps, "%.2f 1 scale\n", fabs(radx*ps_xmult)/fabs(rady*ps_ymult));
		fprintf(ps, "%.2f %.2f %.2f 0 0 %s\n", /*XPOST(xc)*/ 
			/*YPOST(yc)*/ fabs(rady*ps_xmult), startang, startang+angextent, 
			(angextent < 0) ? "fillarcn" : "fillarc") ;
		fprintf(ps, "grestore\n");
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
	HPEN hOldPen;
	HBRUSH hOldBrush;
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
			transpoints[i].x = (short) xcoord (points[i].x);
			transpoints[i].y = (short) ycoord (points[i].y);
		}
#ifdef X11
		XFillPolygon(display, toplevel, current_gc, transpoints, npoints, Complex,
			CoordModeOrigin);
#else
		hOldPen = (HPEN)SelectObject(hGraphicsDC, GetStockObject(NULL_PEN));
		if(!(hOldPen))
			SELECT_ERROR();
		hOldBrush = (HBRUSH)SelectObject(hGraphicsDC, hGraphicsBrush);
		if(!(hOldBrush))
			SELECT_ERROR();
		if(!Polygon (hGraphicsDC, transpoints, npoints))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();
		if(!SelectObject(hGraphicsDC, hOldBrush))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps,"\n");
		
		for (i=npoints-1;i>=0;i--) 
			fprintf (ps, "%.2f %.2f\n", XPOST(points[i].x), YPOST(points[i].y));
		
		fprintf (ps, "%d fillpoly\n", npoints);
	}
}


/* Draws text centered on xc,yc if it fits in boundx */
void 
drawtext (float xc, float yc, const char *text, float boundx) 
{
	int len, width, xw_off, yw_off, font_ascent, font_descent;
	
#ifdef X11
	len = strlen(text);
	width = XTextWidth(font_info[currentfontsize], text, len);
	font_ascent = font_info[currentfontsize]->ascent;
	font_descent = font_info[currentfontsize]->descent;
#else /* WC : WIN32 */
	HFONT hOldFont;
	SIZE textsize;
	TEXTMETRIC textmetric;
	
	hOldFont = (HFONT)SelectObject(hGraphicsDC, hGraphicsFont);
	if(!(hOldFont))
		SELECT_ERROR();
	if(SetTextColor(hGraphicsDC, win32_colors[currentcolor]) == CLR_INVALID)
		DRAW_ERROR();
	
	len = strlen(text);
	if (!GetTextExtentPoint32(hGraphicsDC, text, len, &textsize))
		DRAW_ERROR();
	width = textsize.cx;
	if (!GetTextMetrics(hGraphicsDC, &textmetric))
		DRAW_ERROR();
	font_ascent = textmetric.tmAscent;
	font_descent = textmetric.tmDescent;
#endif	
	if (width > fabs(boundx*xmult)) {
#ifdef WIN32
		if(!SelectObject(hGraphicsDC, hOldFont))
			SELECT_ERROR();
#endif
		return; /* Don't draw if it won't fit */
	}
	
	xw_off = (int)(width/(2.*xmult));      /* NB:  sign doesn't matter. */
	
	/* NB:  2 * descent makes this slightly conservative but simplifies code. */
	yw_off = (int)((font_ascent + 2 * font_descent)/(2.*ymult)); 
	
	/* Note:  text can be clipped when a little bit of it would be visible *
	* right now.  Perhaps X doesn't return extremely accurate width and   *
	* ascent values, etc?  Could remove this completely by multiplying    *
	* xw_off and yw_off by, 1.2 or 1.5.                                   */
	if (rect_off_screen(xc-xw_off, yc-yw_off, xc+xw_off, yc+yw_off)) {
#ifdef WIN32
		if(!SelectObject(hGraphicsDC, hOldFont))
			SELECT_ERROR();
#endif
		return;
	}
	
	if (gl_state.disp_type == SCREEN) {
#ifdef X11
		XDrawString(display, toplevel, current_gc, xcoord(xc)-width/2, ycoord(yc) + 
			(font_info[currentfontsize]->ascent - font_info[currentfontsize]->descent)/2,
			text, len);
#else /* Win32 */
		SetBkMode(hGraphicsDC, TRANSPARENT); 
		if(!TextOut (hGraphicsDC, xcoord(xc)-width/2, ycoord(yc) - (font_ascent + font_descent)/2, 
			text, len))
			DRAW_ERROR();
		if(!SelectObject(hGraphicsDC, hOldFont))
			SELECT_ERROR();
#endif
	}
	else {
		fprintf(ps,"(%s) %.2f %.2f censhow\n",text,XPOST(xc),YPOST(yc));
	}
}


void 
flushinput (void) 
{
	if (gl_state.disp_type != SCREEN) 
		return;
#ifdef X11
	XFlush(display);
#endif
}


void 
init_world (float x1, float y1, float x2, float y2) 
{
	/* Sets the coordinate system the user wants to draw into.          */
	
	xleft = x1;
	xright = x2;
	ytop = y1;
	ybot = y2;
	
	saved_xleft = xleft;     /* Save initial world coordinates to allow full */
	saved_xright = xright;   /* view button to zoom all the way out.         */
	saved_ytop = ytop;
	saved_ybot = ybot;
	
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
		XClearWindow (display, textarea);
		len = strlen (statusMessage);
		width = XTextWidth(font_info[menu_font_size], statusMessage, len);
		XSetForeground(display, gc_menus,colors[WHITE]);
		XDrawRectangle(display, textarea, gc_menus, 0, 0, top_width - MWIDTH, T_AREA_HEIGHT);
		XSetForeground(display, gc_menus,colors[BLACK]);
		XDrawLine(display, textarea, gc_menus, 0, T_AREA_HEIGHT-1, top_width-MWIDTH, T_AREA_HEIGHT-1);
		XDrawLine(display, textarea, gc_menus, top_width-MWIDTH-1, 0, top_width-MWIDTH-1, T_AREA_HEIGHT-1);
		
		XDrawString(display, textarea, gc_menus, 
			(top_width - MWIDTH - width)/2, 
			T_AREA_HEIGHT/2 + (font_info[menu_font_size]->ascent - 
			font_info[menu_font_size]->descent)/2, statusMessage, len);
#else
		if(!InvalidateRect(hStatusWnd, NULL, TRUE))
			DRAW_ERROR();
		if(!UpdateWindow(hStatusWnd))
			DRAW_ERROR();
#endif
	}
	
	else {
	/* Draw the message in the bottom margin.  Printer's generally can't  *
		* print on the bottom 1/4" (area with y < 18 in PostScript coords.)  */
		
		savecolor = currentcolor;
		setcolor (BLACK);
		savefontsize = currentfontsize;
		setfontsize (menu_font_size - 2);  /* Smaller OK on paper */
		ylow = ps_bot - 8; 
		fprintf(ps,"(%s) %.2f %.2f censhow\n",statusMessage,(ps_left+ps_right)/2.,
			ylow);
		setcolor (savecolor);
		setfontsize (savefontsize);
	}
}


/* Changes the message to be displayed on screen.   */
void 
update_message (const char *msg) 
{
	strncpy (statusMessage, msg, BUFSIZE);
	draw_message ();
#ifdef X11
// Make this appear immediately.  Win32 does that automaticaly.
        XFlush (display);  
#endif // X11
}


/* Zooms in by a factor of 1.666. */
static void 
zoom_in (void (*drawscreen) (void)) 
{
	float xdiff, ydiff;
	
	xdiff = xright - xleft; 
	ydiff = ybot - ytop;
	xleft += xdiff/5;
	xright -= xdiff/5;
	ytop += ydiff/5;
	ybot -= ydiff/5;
	
	update_transform ();
	drawscreen();
}


/* Zooms out by a factor of 1.666. */
static void 
zoom_out (void (*drawscreen) (void)) 
{
	float xdiff, ydiff;
	
	xdiff = xright - xleft; 
	ydiff = ybot - ytop;
	xleft -= xdiff/3;
	xright += xdiff/3;
	ytop -= ydiff/3;
	ybot += ydiff/3;
	
	update_transform ();
	drawscreen();
}


/* Sets the view back to the initial view set by init_world (i.e. a full     *
* view) of all the graphics.                                                */
static void 
zoom_fit (void (*drawscreen) (void)) 
{
	xleft = saved_xleft;
	xright = saved_xright;
	ytop = saved_ytop;
	ybot = saved_ybot;
	
	update_transform ();
	drawscreen();
}


/* Moves view 1/2 screen up. */
static void 
translate_up (void (*drawscreen) (void)) 
{
	float ystep;
	
	ystep = (ybot - ytop)/2;
	ytop -= ystep;
	ybot -= ystep;
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen down. */
static void 
translate_down (void (*drawscreen) (void)) 
{
	float ystep;
	
	ystep = (ybot - ytop)/2;
	ytop += ystep;
	ybot += ystep;
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen left. */
static void 
translate_left (void (*drawscreen) (void)) 
{
	
	float xstep;
	
	xstep = (xright - xleft)/2;
	xleft -= xstep;
	xright -= xstep; 
	update_transform();         
	drawscreen();
}


/* Moves view 1/2 screen right. */
static void 
translate_right (void (*drawscreen) (void)) 
{
	float xstep;
	
	xstep = (xright - xleft)/2;
	xleft += xstep;
	xright += xstep; 
	update_transform();         
	drawscreen();
}


static void 
update_win (int x[2], int y[2], void (*drawscreen)(void)) 
{
	float x1, x2, y1, y2;
	
	x[0] = min(x[0],top_width-MWIDTH);  /* Can't go under menu */
	x[1] = min(x[1],top_width-MWIDTH);
	y[0] = min(y[0],top_height-T_AREA_HEIGHT); /* Can't go under text area */
	y[1] = min(y[1],top_height-T_AREA_HEIGHT);
	
	if ((x[0] == x[1]) || (y[0] == y[1])) {
		printf("Illegal (zero area) window.  Window unchanged.\n");
		return;
	}
	x1 = XTOWORLD(min(x[0],x[1]));
	x2 = XTOWORLD(max(x[0],x[1]));
	y1 = YTOWORLD(min(y[0],y[1]));
	y2 = YTOWORLD(max(y[0],y[1]));
	xleft = x1;
	xright = x2;
	ytop = y1;
	ybot = y2;
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
		XNextEvent (display, &report);
		switch (report.type) {
		case Expose:
#ifdef VERBOSE 
			printf("Got an expose event.\n");
			printf("Count is: %d.\n",report.xexpose.count);
			printf("Window ID is: %d.\n",report.xexpose.window);
#endif
			if (report.xexpose.count != 0)
				break;
			if (report.xexpose.window == menu)
				drawmenu(); 
			else if (report.xexpose.window == toplevel) {
				drawscreen();
				xold = -1;   /* No rubber band on screen */
			}
			else if (report.xexpose.window == textarea)
				draw_message();
			break;
		case ConfigureNotify:
			top_width = report.xconfigure.width;
			top_height = report.xconfigure.height;
			update_transform();
			drawmenu();
			draw_message();
#ifdef VERBOSE 
			printf("Got a ConfigureNotify.\n");
			printf("New width: %d  New height: %d.\n",top_width,top_height);
#endif
			break;
		case ButtonPress:
#ifdef VERBOSE 
			printf("Got a buttonpress.\n");
			printf("Window ID is: %d.\n",report.xbutton.window);
			printf("Location (%d, %d).\n", report.xbutton.x,
				report.xbutton.y);
#endif
			if (report.xbutton.window != toplevel) break;
			x[corner] = report.xbutton.x;
			y[corner] = report.xbutton.y; 
			if (corner == 0) {
			/*	XSelectInput (display, toplevel, ExposureMask | 
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
					XDrawRectangle(display,toplevel,gcxor,min(x[0],xold),
						min(y[0],yold),abs(x[0]-xold),abs(y[0]-yold));
					set_draw_mode(DRAW_NORMAL);
				}
				/* Don't allow user to window under menu region */
				xold = min(report.xmotion.x,top_width-1-MWIDTH); 
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
				XDrawRectangle(display,toplevel,gcxor,min(x[0],xold),
					min(y[0],yold),abs(x[0]-xold),abs(y[0]-yold));
				set_draw_mode(DRAW_NORMAL);
			}
			break;
		}
	}
	/* XSelectInput (display, toplevel, ExposureMask | StructureNotifyMask
		| ButtonPressMask); */
#else /* Win32 */
	/* Implemented as WM_LB... events */
	
	/* Begin window adjust */
	if (!windowAdjustFlag) {
		windowAdjustFlag = 1;
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
		MessageBox(hMainWnd, "Error initializing postscript output.", NULL, MB_OK);
#endif
	}
}


static void 
proceed (void (*drawscreen) (void)) 
{
	ProceedPressed = TRUE;
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
		if (font_is_loaded[i])
			XFreeFont(display,font_info[i]);
	}
	
	XFreeGC(display,gc);
	XFreeGC(display,gcxor);
	XFreeGC(display,gc_menus);
	
	if (private_cmap != None) 
		XFreeColormap (display, private_cmap);
	
	XCloseDisplay(display);
#else /* Win32 */
	int i;
   // Free the font data structure for each loaded font.
   for (i = 0; i <= MAX_FONT_SIZE; i++) {
      if (font_is_loaded[i]) {
         free (font_info[i]);
      }
   }

   // Destroy the window
	if(!DestroyWindow(hMainWnd))
		DRAW_ERROR();

   // free the window class (type information held by MS Windows) 
   // for each of the four window types we created.  Otherwise a 
   // later call to init_graphics to open the graphics window up again
   // will fail.
   if (!UnregisterClass (szAppName, GetModuleHandle(NULL)) )
      DRAW_ERROR();
   if (!UnregisterClass (szGraphicsName, GetModuleHandle(NULL)) )
      DRAW_ERROR();
   if (!UnregisterClass (szStatusName, GetModuleHandle(NULL)) )
      DRAW_ERROR();
   if (!UnregisterClass (szButtonsName, GetModuleHandle(NULL)) )
      DRAW_ERROR();
#endif

	free(button);
   button = NULL;

   for (i = 0; i <= MAX_FONT_SIZE; i++) {
      font_is_loaded[i] = false;
      font_info[i] = NULL;
   }
   gl_state.initialized = false;
}


/* Opens a file for PostScript output.  The header information,  *
* clipping path, etc. are all dumped out.  If the file could    *
* not be opened, the routine returns 0; otherwise it returns 1. */
int init_postscript (const char *fname) 
{
	
	ps = fopen (fname,"w");
	if (ps == NULL) {
		printf("Error: could not open %s for PostScript output.\n",fname);
		printf("Drawing to screen instead.\n");
		return (0);
	}
	gl_state.disp_type = POSTSCRIPT;  /* Graphics go to postscript file now. */
	
	/* Header for minimal conformance with the Adobe structuring convention */
	fprintf(ps,"%%!PS-Adobe-1.0\n");
	fprintf(ps,"%%%%DocumentFonts: Helvetica\n");
	fprintf(ps,"%%%%Pages: 1\n");
	/* Set up postscript transformation macros and page boundaries */
	update_ps_transform();
	/* Bottom margin is at ps_bot - 15. to leave room for the on-screen message. */
	fprintf(ps,"%%%%HiResBoundingBox: %.2f %.2f %.2f %.2f\n",
		ps_left, ps_bot - 15., ps_right, ps_top); 
	fprintf(ps,"%%%%EndComments\n");
	
	fprintf(ps,"/censhow   %%draw a centered string\n");
	fprintf(ps," { moveto               %% move to proper spot\n");
	fprintf(ps,"   dup stringwidth pop  %% get x length of string\n");
	fprintf(ps,"   -2 div               %% Proper left start\n");
	fprintf(ps,"   yoff rmoveto         %% Move left that much and down half font height\n");
	fprintf(ps,"   show newpath } def   %% show the string\n\n"); 
	
	fprintf(ps,"/setfontsize     %% set font to desired size and compute "
		"centering yoff\n");
	fprintf(ps," { /Helvetica findfont\n");
	fprintf(ps,"   8 scalefont\n");
	fprintf(ps,"   setfont         %% Font size set ...\n\n");
	fprintf(ps,"   0 0 moveto      %% Get vertical centering offset\n");
	fprintf(ps,"   (Xg) true charpath\n");
	fprintf(ps,"   flattenpath pathbbox\n");
	fprintf(ps,"   /ascent exch def pop -1 mul /descent exch def pop\n");
	fprintf(ps,"   newpath\n");
	fprintf(ps,"   descent ascent sub 2 div /yoff exch def } def\n\n");
	
	fprintf(ps,"%% Next two lines for debugging only.\n");
	fprintf(ps,"/str 20 string def\n");
	fprintf(ps,"/pnum {str cvs print (  ) print} def\n");
	
	fprintf(ps,"/drawline      %% draw a line from (x2,y2) to (x1,y1)\n");
	fprintf(ps," { moveto lineto stroke } def\n\n");
	
	fprintf(ps,"/rect          %% outline a rectangle \n");
	fprintf(ps," { /y2 exch def /x2 exch def /y1 exch def /x1 exch def\n");
	fprintf(ps,"   x1 y1 moveto\n");
	fprintf(ps,"   x2 y1 lineto\n");
	fprintf(ps,"   x2 y2 lineto\n");
	fprintf(ps,"   x1 y2 lineto\n");
	fprintf(ps,"   closepath } def\n\n");
	
	fprintf(ps,"/drawrect      %% draw outline of a rectanagle\n");
	fprintf(ps," { rect stroke } def\n\n");
	
	fprintf(ps,"/fillrect      %% fill in a rectanagle\n");
	fprintf(ps," { rect fill } def\n\n");
	
	fprintf (ps,"/drawarc { arc stroke } def           %% draw an arc\n");
	fprintf (ps,"/drawarcn { arcn stroke } def "
		"        %% draw an arc in the opposite direction\n\n");
	
	fprintf (ps,"%%Fill a counterclockwise or clockwise arc sector, "
		"respectively.\n");
	fprintf (ps,"/fillarc { moveto currentpoint 5 2 roll arc closepath fill } "
		"def\n");
	fprintf (ps,"/fillarcn { moveto currentpoint 5 2 roll arcn closepath fill } "
		"def\n\n");
	
	fprintf (ps,"/fillpoly { 3 1 roll moveto         %% move to first point\n"
		"   2 exch 1 exch {pop lineto} for   %% line to all other points\n"
		"   closepath fill } def\n\n");
	
	
	fprintf(ps,"%%Color Definitions:\n");
	fprintf(ps,"/white { 1 setgray } def\n");
	fprintf(ps,"/black { 0 setgray } def\n");
	fprintf(ps,"/grey55 { .55 setgray } def\n");
	fprintf(ps,"/grey75 { .75 setgray } def\n");
	fprintf(ps,"/blue { 0 0 1 setrgbcolor } def\n");
	fprintf(ps,"/green { 0 1 0 setrgbcolor } def\n");
	fprintf(ps,"/yellow { 1 1 0 setrgbcolor } def\n");
	fprintf(ps,"/cyan { 0 1 1 setrgbcolor } def\n");
	fprintf(ps,"/red { 1 0 0 setrgbcolor } def\n");
	fprintf(ps,"/darkgreen { 0 0.5 0 setrgbcolor } def\n");
	fprintf(ps,"/magenta { 1 0 1 setrgbcolor } def\n");
	fprintf(ps,"/bisque { 1 0.89 0.77 setrgbcolor } def\n");
	fprintf(ps,"/lightblue { 0.68 0.85 0.9 setrgbcolor } def\n");
	fprintf(ps,"/thistle { 0.85 0.75 0.85 setrgbcolor } def\n");
	fprintf(ps,"/plum { 0.87 0.63 0.87 setrgbcolor } def\n");
	fprintf(ps,"/khaki { 0.94 0.9 0.55 setrgbcolor } def\n");
	fprintf(ps,"/coral { 1 0.5 0.31 setrgbcolor } def\n");
	fprintf(ps,"/turquoise { 0.25 0.88 0.82 setrgbcolor } def\n");
	fprintf(ps,"/mediumpurple { 0.58 0.44 0.86 setrgbcolor } def\n");
	fprintf(ps,"/darkslateblue { 0.28 0.24 0.55 setrgbcolor } def\n");
	fprintf(ps,"/darkkhaki { 0.74 0.72 0.42 setrgbcolor } def\n");
	
	fprintf(ps,"\n%%Solid and dashed line definitions:\n");
	fprintf(ps,"/linesolid {[] 0 setdash} def\n");
	fprintf(ps,"/linedashed {[3 3] 0 setdash} def\n");
	
	fprintf(ps,"\n%%%%EndProlog\n");
	fprintf(ps,"%%%%Page: 1 1\n\n");
	
	/* Set up PostScript graphics state to match current one. */
	force_setcolor (currentcolor);
	force_setlinestyle (currentlinestyle);
	force_setlinewidth (currentlinewidth);
	force_setfontsize (currentfontsize); 
	
	/* Draw this in the bottom margin -- must do before the clippath is set */
	draw_message ();
	
	/* Set clipping on page. */
	fprintf(ps,"%.2f %.2f %.2f %.2f rect ",ps_left, ps_bot,ps_right,ps_top);
	fprintf(ps,"clip newpath\n\n");
	
	return (1);
}

/* Properly ends postscript output and redirects output to screen. */
void close_postscript (void) 
{
		
	fprintf(ps,"showpage\n");
	fprintf(ps,"\n%%%%Trailer\n");
	fclose (ps);
	gl_state.disp_type = SCREEN;
	update_transform();   /* Ensure screen world reflects any changes      *
	* made while printing.                          */
	
	/* Need to make sure that we really set up the graphics context.  
	* The current font set indicates the last font used in a postscript call, 
   * etc., *NOT* the font set in the X11 or Win32 graphics context.  Force the
   * current font, colour etc. to be applied to the graphics context, so 
   * subsequent drawing commands work properly.
   */
	
	force_setcolor (currentcolor);
	force_setlinestyle (currentlinestyle);
	force_setlinewidth (currentlinewidth);
	force_setfontsize (currentfontsize); 
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
	
	menu = XCreateSimpleWindow(display,toplevel,
		top_width-MWIDTH, 0, MWIDTH, display_height, 0,
		colors[BLACK], colors[LIGHTGREY]); 
	menu_attributes.event_mask = ExposureMask;
	/* Ignore button presses on the menu background. */
	menu_attributes.do_not_propagate_mask = ButtonPressMask;
	/* Keep menu on top right */
	menu_attributes.win_gravity = NorthEastGravity; 
	valuemask = CWWinGravity | CWEventMask | CWDontPropagate;
	XChangeWindowAttributes(display, menu, valuemask, &menu_attributes);
	XMapWindow (display, menu);
#endif
	
	button = (t_button *) my_malloc (NUM_STANDARD_BUTTONS * sizeof (t_button));
	
	/* Now do the arrow buttons */
	bwid = 28;
	space = 3;
	y1 = 10;
	xcen = 51;
	x1 = xcen - bwid/2; 
	button[0].xleft = x1;
	button[0].ytop = y1;
#ifdef X11
	setpoly (0, bwid/2, bwid/2, bwid/3, -PI/2.); /* Up */
#else
	button[0].type = BUTTON_TEXT;
	strcpy(button[0].text, "U");
#endif
	button[0].fcn = translate_up;
	
	y1 += bwid + space;
	x1 = xcen - 3*bwid/2 - space;
	button[1].xleft = x1;
	button[1].ytop = y1;
#ifdef X11
	setpoly (1, bwid/2, bwid/2, bwid/3, PI);  /* Left */
#else
	button[1].type = BUTTON_TEXT;
	strcpy(button[1].text, "L");
#endif
	button[1].fcn = translate_left;
	
	x1 = xcen + bwid/2 + space;
	button[2].xleft = x1;
	button[2].ytop = y1;
#ifdef X11
	setpoly (2, bwid/2, bwid/2, bwid/3, 0);  /* Right */
#else
	button[2].type = BUTTON_TEXT;
	strcpy(button[2].text, "R");
#endif
	button[2].fcn = translate_right;
	
	y1 += bwid + space;
	x1 = xcen - bwid/2;
	button[3].xleft = x1;
	button[3].ytop = y1;
#ifdef X11
	setpoly (3, bwid/2, bwid/2, bwid/3, +PI/2.);  /* Down */
#else
	button[3].type = BUTTON_TEXT;
	strcpy(button[3].text, "D");
#endif
	button[3].fcn = translate_down;
	
	for (i = 0; i < NUM_ARROW_BUTTONS; i++) {
		button[i].width = bwid;
		button[i].height = bwid;
		button[i].enabled = 1;
	} 
	
	/* Rectangular buttons */
	
	y1 += bwid + space + 6;
	space = 8;
	bwid = 90;
	bheight = 26;
	x1 = xcen - bwid/2;
	for (i = NUM_ARROW_BUTTONS; i < NUM_STANDARD_BUTTONS; i++) {
		button[i].xleft = x1;
		button[i].ytop = y1;
		button[i].type = BUTTON_TEXT;
		button[i].width = bwid;
		button[i].enabled = 1;
		if (i != SEPARATOR_BUTTON_INDEX) {
			button[i].height = bheight;
			y1 += bheight + space;
		}
		else {
			button[i].height = 2;
			button[i].type = BUTTON_SEPARATOR;
			y1 += 2 + space;
		}
	}
	
	strcpy (button[4].text,"Zoom In");
	strcpy (button[5].text,"Zoom Out");
	strcpy (button[6].text,"Zoom Fit");
	strcpy (button[7].text,"Window");
	strcpy (button[8].text,"---1");
	strcpy (button[9].text,"PostScript");
	strcpy (button[10].text,"Proceed");
	strcpy (button[11].text,"Exit");
	
	button[4].fcn = zoom_in;
	button[5].fcn = zoom_out;
	button[6].fcn = zoom_fit;
	button[7].fcn = adjustwin;  // see 'adjustButton' below in WIN32 section
	button[8].fcn = NULL;
	button[9].fcn = postscript;
	button[10].fcn = proceed;
	button[11].fcn = quit;
	
	for (i = 0; i < NUM_STANDARD_BUTTONS; i++) 
		map_button (i);
	num_buttons = NUM_STANDARD_BUTTONS;

#ifdef WIN32
	adjustButton = 7;
	if(!InvalidateRect(hButtonsWnd, NULL, TRUE))
		DRAW_ERROR();
	if(!UpdateWindow(hButtonsWnd))
		DRAW_ERROR();
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

   	if (font_is_loaded[pointsize])  // Nothing to do.
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
      if ((font_info[pointsize] = XLoadQueryFont(display,fontname[ifont])) == NULL) {
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
   LOGFONT *lf = font_info[pointsize] = (LOGFONT*)my_malloc(sizeof(LOGFONT));
   ZeroMemory(lf, sizeof(LOGFONT));
   lf->lfHeight = pointsize;
   lf->lfWeight = FW_NORMAL;
   lf->lfCharSet = ANSI_CHARSET;
   lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
   lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
   lf->lfQuality = PROOF_QUALITY;
   lf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
   strcpy(lf->lfFaceName, "Arial");	 
#endif

   font_is_loaded[pointsize] = true;
}


/* Return information useful for debugging.
 * Used to return the top-level window object too, but that made graphics.h
 * export all windows and X11 headers to the client program, so VB deleted 
 * that object (mainwnd) from this structure.
 */
void report_structure(t_report *report) {
	report->xmult = xmult;
	report->ymult = ymult;
	report->xleft = xleft;
	report->xright = xright;
	report->ytop = ytop;
	report->ybot = ybot;
	report->ps_xmult = ps_xmult;
	report->ps_ymult = ps_ymult;
	report->top_width = top_width;
	report->top_height = top_height;
}


void set_mouse_move_input (bool enable) {
	get_mouse_move_input = enable;
}


void set_keypress_input (bool enable) {
	get_keypress_input = enable;
}


void enable_or_disable_button (int ibutton, bool enabled) {

   if (button[ibutton].type != BUTTON_SEPARATOR) {
      button[ibutton].enabled = enabled;
#ifdef WIN32
      EnableWindow(button[ibutton].hwnd, button[ibutton].enabled);
#else  // X11
      drawbut(ibutton);
      XFlush(display);
#endif
   }
}


void set_draw_mode (enum e_draw_mode draw_mode) {
/* Set normal (overwrite) or xor (useful for rubber-banding)
 * drawing mode.
 */

   if (draw_mode == DRAW_NORMAL) {
#ifdef X11
      current_gc = gc;
#else
      if (!SetROP2(hGraphicsDC, R2_COPYPEN))
         SELECT_ERROR();
#endif
   }
   else {  // DRAW_XOR
#ifdef X11
      current_gc = gcxor;
#else
      if (!SetROP2(hGraphicsDC, R2_XORPEN))
         SELECT_ERROR();
#endif
   }
   current_draw_mode = draw_mode;
}


void change_button_text(const char *button_name, const char *new_button_text) {
/* Change the text on a button with button_name to new_button_text.
 * Not a strictly necessary function, since you could intead just 
 * destroy button_name and make a new buton.
 */
	int i, bnum;
	
	bnum = -1;
	
	for (i=4;i<num_buttons;i++) {
		if (button[i].type == BUTTON_TEXT && 
			strcmp (button[i].text, button_name) == 0) {
			bnum = i;
			break;
		}
	}

	if (bnum != -1) {
		strncpy (button[i].text, new_button_text, BUTTON_TEXT_LEN);
#ifdef X11
		drawbut (i);
#else // Win32
		SetWindowText(button[bnum].hwnd, new_button_text);
#endif
	}
}




/**********************************
* X-Windows Specific Definitions *
*********************************/
#ifdef X11

/* Creates a small window at the top of the graphics area for text messages */
static void build_textarea (void) 
{
	XSetWindowAttributes menu_attributes;
	unsigned long valuemask;
	
	textarea = XCreateSimpleWindow(display,toplevel,
		0, top_height-T_AREA_HEIGHT, display_width-MWIDTH, T_AREA_HEIGHT, 0,
		colors[BLACK], colors[LIGHTGREY]); 
	menu_attributes.event_mask = ExposureMask;
	/* ButtonPresses in this area are ignored. */
	menu_attributes.do_not_propagate_mask = ButtonPressMask;
	/* Keep text area on bottom left */
	menu_attributes.win_gravity = SouthWestGravity; 
	valuemask = CWWinGravity | CWEventMask | CWDontPropagate;
	XChangeWindowAttributes(display, textarea, valuemask, &menu_attributes);
	XMapWindow (display, textarea);
}



static Bool test_if_exposed (Display *disp, XEvent *event_ptr, XPointer dummy) 
/* Returns True if the event passed in is an exposure event.   Note that 
 * the bool type returned by this function is defined in Xlib.h.         
 */
{
	
	
	if (event_ptr->type == Expose) {
		return (True);
	}
	
	return (False);
}


static void menutext(Window win, int xc, int yc, const char *text) 
{
	
	/* draws text center at xc, yc -- used only by menu drawing stuff */
	
	int len, width; 
	
	len = strlen(text);
	width = XTextWidth(font_info[menu_font_size], text, len);
	XDrawString(display, win, gc_menus, xc-width/2, yc + 
		(font_info[menu_font_size]->ascent - font_info[menu_font_size]->descent)/2,
		text, len);
}


static void drawbut (int bnum) 
{
	
	/* Draws button bnum in either its pressed or unpressed state.    */
	
	int width, height, thick, i, ispressed;
	XPoint mypoly[6];

	width = button[bnum].width;
	height = button[bnum].height;

	if (button[bnum].type == BUTTON_SEPARATOR) {
		int x,y;

		x = button[bnum].xleft;
		y = button[bnum].ytop;
		XSetForeground(display, gc_menus,colors[WHITE]);
		XDrawLine(display, menu, gc_menus, x, y+1, x+width, y+1);
		XSetForeground(display, gc_menus,colors[BLACK]);
		XDrawLine(display, menu, gc_menus, x, y, x+width, y);
		return;
	}

	ispressed = button[bnum].ispressed;
	thick = 2;
	/* Draw top and left edges of 3D box. */
	if (ispressed) {
		XSetForeground(display, gc_menus,colors[BLACK]);
	}
	else {
		XSetForeground(display, gc_menus,colors[WHITE]);
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
	XFillPolygon(display,button[bnum].win,gc_menus,mypoly,6,Convex,
		CoordModeOrigin);
	
	/* Draw bottom and right edges of 3D box. */
	if (ispressed) {
		XSetForeground(display, gc_menus,colors[WHITE]);
	}
	else {
		XSetForeground(display, gc_menus,colors[BLACK]);
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
	XFillPolygon(display,button[bnum].win,gc_menus,mypoly,6,Convex,
		CoordModeOrigin);
	
	/* Draw background */
	if (ispressed) {
		XSetForeground(display, gc_menus,colors[DARKGREY]);
	}
	else {
		XSetForeground(display, gc_menus,colors[LIGHTGREY]);
	}
	
	/* Give x,y of top corner and width and height */
	XFillRectangle (display,button[bnum].win,gc_menus,thick,thick,
		width-2*thick, height-2*thick);
	
	/* Draw polygon, if there is one */
	if (button[bnum].type == BUTTON_POLY) {
		for (i=0;i<3;i++) {
			mypoly[i].x = button[bnum].poly[i][0];
			mypoly[i].y = button[bnum].poly[i][1];
		}
		XSetForeground(display, gc_menus,colors[BLACK]);
		XFillPolygon(display,button[bnum].win,gc_menus,mypoly,3,Convex,
			CoordModeOrigin);
	}
	
	/* Draw text, if there is any */
	if (button[bnum].type == BUTTON_TEXT) {
		if (button[bnum].enabled)
			XSetForeground(display, gc_menus,colors[BLACK]);
		else
			XSetForeground(display, gc_menus,colors[DARKGREY]);
		menutext(button[bnum].win,button[bnum].width/2,
			button[bnum].height/2,button[bnum].text);
	}
}


static int which_button (Window win) 
{
	int i;
	
	for (i=0;i<num_buttons;i++) {
		if (button[i].win == win)
			return(i);
	}
	printf("Error:  Unknown button ID in which_button.\n");
	return(0);
}


static void turn_on_off (int pressed) {
/* Shows when the menu is active or inactive by colouring the 
 * buttons.                                                   
 */
	int i;
	
	for (i=0;i<num_buttons;i++) {
		button[i].ispressed = pressed;
		drawbut(i);
	}
}


static void drawmenu(void) 
{
	int i;

	XClearWindow (display, menu);
	XSetForeground(display, gc_menus,colors[WHITE]);
	XDrawRectangle(display, menu, gc_menus, 0, 0, MWIDTH, top_height);
	XSetForeground(display, gc_menus,colors[BLACK]);
	XDrawLine(display, menu, gc_menus, 0, top_height-1, MWIDTH, top_height-1);
	XDrawLine(display, menu, gc_menus, MWIDTH-1, top_height, MWIDTH-1, 0);
	
	for (i=0;i<num_buttons;i++)  {
		drawbut(i);
	}
}

#endif /* X-Windows Specific Definitions */



/*************************************************
* Microsoft Windows (WIN32) Specific Definitions *
*************************************************/
#ifdef WIN32

static LRESULT CALLBACK 
MainWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MINMAXINFO FAR *lpMinMaxInfo;   
	
	switch(message)
	{
		
	case WM_CREATE:
		hStatusWnd = CreateWindow(szStatusName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
			0, 0, 0, 0, hwnd, (HMENU) 102, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
		hButtonsWnd = CreateWindow(szButtonsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
			0, 0, 0, 0, hwnd, (HMENU) 103, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
		hGraphicsWnd = CreateWindow(szGraphicsName, NULL, WS_CHILDWINDOW | WS_VISIBLE,
			0, 0, 0, 0, hwnd, (HMENU) 101, (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE), NULL);
		return 0;
		
	case WM_SIZE:
		/* Window has been resized.  Save the new client dimensions */
		top_width = LOWORD (lParam);
		top_height = HIWORD (lParam);
		
		/* Resize the children windows */
		if(!MoveWindow(hGraphicsWnd, 1, 1, top_width - MWIDTH - 1, top_height - T_AREA_HEIGHT - 1, TRUE))
			DRAW_ERROR();
//		if (drawscreen_ptr)
//			zoom_fit(drawscreen_ptr);
		if(!MoveWindow(hStatusWnd, 0, top_height - T_AREA_HEIGHT, top_width - MWIDTH, T_AREA_HEIGHT, TRUE))
			DRAW_ERROR();
		if(!MoveWindow(hButtonsWnd, top_width - MWIDTH, 0, MWIDTH, top_height, TRUE))
			DRAW_ERROR();
		
		return 0;
		
		// WC : added to solve window resizing problem
	case WM_GETMINMAXINFO:
		// set the MINMAXINFO structure pointer 
		lpMinMaxInfo = (MINMAXINFO FAR *) lParam;
		lpMinMaxInfo->ptMinTrackSize.x = display_width / 2;
		lpMinMaxInfo->ptMinTrackSize.y = display_height / 2;
		
		return 0;
		
		
	case WM_DESTROY:
		if(!DeleteObject(hGrayBrush))
			DELETE_ERROR();
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYDOWN:
		if (get_keypress_input)
		     keypress_ptr((char) wParam);
		return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


static LRESULT CALLBACK 
GraphicsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TEXTMETRIC tm;
	
	PAINTSTRUCT ps;
	static RECT oldAdjustRect;
	static HPEN hDotPen = 0;
	static HBITMAP hbmBuffer = 0, hbmObjtest, hbmAllObjtest;
	static int X, Y, i;
	
	switch(message)
	{
	case WM_CREATE:
		
		/* Get the text metrics once upon creation (system font cannot change) */
		hCurrentDC = hGraphicsDC = hForegroundDC = GetDC (hwnd);
		if(!hGraphicsDC)
			DRAW_ERROR();
		
		hBackgroundDC = CreateCompatibleDC(hForegroundDC);
		if (!hBackgroundDC)
			CREATE_ERROR();
		if (!SetMapMode(hBackgroundDC, MM_TEXT))
			CREATE_ERROR();
		hbmBuffer = CreateCompatibleBitmap(hForegroundDC, display_width, display_height);
		if (!(hbmBuffer))
			CREATE_ERROR();
		if (!SelectObject(hBackgroundDC, hbmBuffer))
			SELECT_ERROR();

		// monochrome bitmap
		hObjtestDC = CreateCompatibleDC(hForegroundDC);
		if (!hObjtestDC)
			CREATE_ERROR();
		if (!SetMapMode(hObjtestDC, MM_TEXT))
			CREATE_ERROR();
		hbmObjtest = CreateCompatibleBitmap(hObjtestDC, display_width, display_height);
		if (!(hbmObjtest))
			CREATE_ERROR();
		if (!SelectObject(hObjtestDC, hbmObjtest))
			SELECT_ERROR();

		// monochrome bitmap
		hAllObjtestDC = CreateCompatibleDC(hForegroundDC);
		if (!hObjtestDC)
			CREATE_ERROR();
		if (!SetMapMode(hAllObjtestDC, MM_TEXT))
			CREATE_ERROR();
		hbmAllObjtest = CreateCompatibleBitmap(hAllObjtestDC, display_width, display_height);
		if (!(hbmAllObjtest))
			CREATE_ERROR();
		if (!SelectObject(hAllObjtestDC, hbmAllObjtest))
			SELECT_ERROR();

		//if(!GetTextMetrics (hGraphicsDC, &tm))
		//	DRAW_ERROR();
		if(!SetBkMode(hGraphicsDC, TRANSPARENT))
			DRAW_ERROR();
		
		/* Setup the pens, etc */
		currentlinestyle = SOLID;
		currentcolor = BLACK;
		currentlinewidth = 1;
		
		/*
		if(!ReleaseDC (hwnd, hGraphicsDC))
		DRAW_ERROR();
		
		  hGraphicsDC = 0;
		*/		currentfontsize = 12;
		return 0;
		
	case WM_PAINT:
		// was in xor mode, but got a general redraw.
                // switch to normal drawing so we repaint properly.
		if (current_draw_mode == DRAW_XOR) {
			set_draw_mode(DRAW_NORMAL);
			invalidate_screen();
			return 0;
		}
		hCurrentDC = hGraphicsDC;
		drawtoscreen();
		/*hGraphicsDC =*/ BeginPaint(hwnd, &ps);
		if(!hGraphicsDC)
			DRAW_ERROR();
		
		if (InEventLoop) {
			if(!GetUpdateRect(hwnd, &updateRect, FALSE)) {
				updateRect.left = 0;
				updateRect.right = top_width;
				updateRect.top = 0;
				updateRect.bottom = top_height;
			}
			
			if(windowAdjustFlag > 1) {
				hDotPen = CreatePen(PS_DASH, 1, win32_colors[gl_state.background_cindex]);
				if(!hDotPen)
					CREATE_ERROR();
				if (!SetROP2(hGraphicsDC, R2_XORPEN))
					SELECT_ERROR();
				if(!SelectObject(hGraphicsDC, GetStockObject(NULL_BRUSH)))
					SELECT_ERROR();
				if(!SelectObject(hGraphicsDC, hDotPen))
					SELECT_ERROR();
				if(!Rectangle(hGraphicsDC, oldAdjustRect.left, oldAdjustRect.top, 
					oldAdjustRect.right, oldAdjustRect.bottom))
					DRAW_ERROR();
				if(!Rectangle(hGraphicsDC, adjustRect.left, adjustRect.top, adjustRect.right,
					adjustRect.bottom))
					DRAW_ERROR();
				oldAdjustRect = adjustRect;
				if (!SetROP2(hGraphicsDC, R2_COPYPEN))
					SELECT_ERROR();
				if(!SelectObject(hGraphicsDC, GetStockObject(NULL_PEN)))
					SELECT_ERROR();
				if(!DeleteObject(hDotPen))
					DELETE_ERROR();
			}
			else
				drawscreen_ptr();		
		}
		EndPaint(hwnd, &ps);
		hGraphicsDC = hCurrentDC;
		
		/* Crash hard if called at wrong time */
		/*		hGraphicsDC = NULL;*/
		return 0;
		
	case WM_SIZE:
		/* Window has been resized.  Save the new client dimensions */
		cxClient = LOWORD (lParam);
		cyClient = HIWORD (lParam);
		update_transform();

		return 0;
		
	case WM_DESTROY:
		if(!DeleteObject(hGraphicsPen))
			DELETE_ERROR();
		if(!DeleteObject(hGraphicsBrush))
			DELETE_ERROR();
		if(!DeleteObject(hGraphicsFont))
			DELETE_ERROR();
		if (!DeleteObject(hbmBuffer))
			DELETE_ERROR();
		if (!DeleteObject(hbmObjtest))
			DELETE_ERROR();
		if (!DeleteObject(hbmAllObjtest))
			DELETE_ERROR();
		if(!DeleteDC(hBackgroundDC))
			DELETE_ERROR();
		if(!DeleteDC(hObjtestDC))
			DELETE_ERROR();
		if(!DeleteDC(hAllObjtestDC))
			DELETE_ERROR();
		if(!ReleaseDC(hwnd, hForegroundDC))
			DELETE_ERROR();
		PostQuitMessage(0);
		return 0;
		
	case WM_LBUTTONDOWN:
		if (!windowAdjustFlag) {  
			mouseclick_ptr(XTOWORLD(LOWORD(lParam)), YTOWORLD(HIWORD(lParam)));
		} 
      else {
         // Special handling for the "Window" command, which takes multiple clicks. 
         // First you push the button, then you click for one corner, then you click for the other
         // corner.
			if(windowAdjustFlag == 1) {
				windowAdjustFlag ++;
				X = adjustRect.left = adjustRect.right = LOWORD(lParam);
				Y = adjustRect.top = adjustRect.bottom = HIWORD(lParam);
				oldAdjustRect = adjustRect;
			}
         else {
				int i;
				int adjustx[2], adjusty[2];
				
				windowAdjustFlag = 0;
				button[adjustButton].ispressed = 0;
				SendMessage(button[adjustButton].hwnd, BM_SETSTATE, 0, 0);
							
				for (i=0; i<num_buttons; i++) {
					if (button[i].type != BUTTON_SEPARATOR && button[i].enabled) {
						if(!EnableWindow (button[i].hwnd, TRUE))
							DRAW_ERROR();
					}
				}
				adjustx[0] = adjustRect.left;
				adjustx[1] = adjustRect.right;
				adjusty[0] = adjustRect.top;
				adjusty[1] = adjustRect.bottom;
				
				update_win(adjustx, adjusty, invalidate_screen);
			}
		}
		return 0;

	// right click : a quick way to zoom in
	case WM_RBUTTONDOWN:
		if (!windowAdjustFlag) {
			// first disable some buttons
			//adjustButton = LOWORD(wParam) - 200;
			button[adjustButton].ispressed = 1;
			for (i=0; i<num_buttons; i++) {
				EnableWindow(button[i].hwnd, FALSE);
				SendMessage(button[i].hwnd, BM_SETSTATE, button[i].ispressed, 0);
			}

			windowAdjustFlag = 2;
			X = adjustRect.left = adjustRect.right = LOWORD(lParam);
			Y = adjustRect.top = adjustRect.bottom = HIWORD(lParam);
			oldAdjustRect = adjustRect;
		} 
      else {
			int i;
			int adjustx[2], adjusty[2];
				
			windowAdjustFlag = 0;
			button[adjustButton].ispressed = 0;
			SendMessage(button[adjustButton].hwnd, BM_SETSTATE, 0, 0);
							
			for (i=0; i<num_buttons; i++) {
				if (button[i].type != BUTTON_SEPARATOR && button[i].enabled) {
					if(!EnableWindow (button[i].hwnd, TRUE))
						DRAW_ERROR();
				}
			}
			adjustx[0] = adjustRect.left;
			adjustx[1] = adjustRect.right;
			adjusty[0] = adjustRect.top;
			adjusty[1] = adjustRect.bottom;
				
			update_win(adjustx, adjusty, invalidate_screen);
		}
		return 0;
	
	case WM_MOUSEMOVE:
		if(windowAdjustFlag == 1) {
			return 0;
		}
 else if (windowAdjustFlag >= 2) {
			if ( X > LOWORD(lParam)) {
				adjustRect.left = LOWORD(lParam);
				adjustRect.right = X;
			}
			else {
				adjustRect.left = X;
				adjustRect.right = LOWORD(lParam);
			}
			if ( Y > HIWORD(lParam)) {
				adjustRect.top = HIWORD(lParam);
				adjustRect.bottom = Y;
			}
			else {
				adjustRect.top = Y;
				adjustRect.bottom = HIWORD(lParam);
			}
			if(!InvalidateRect(hGraphicsWnd, &oldAdjustRect, FALSE))
				DRAW_ERROR();
			if(!InvalidateRect(hGraphicsWnd, &adjustRect, FALSE))
				DRAW_ERROR();
			if(!UpdateWindow(hGraphicsWnd))
				DRAW_ERROR();
			
			return 0;
		}
		else if (get_mouse_move_input)
			mousemove_ptr(XTOWORLD(LOWORD(lParam)), YTOWORLD(HIWORD(lParam)));

		return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


static LRESULT CALLBACK 
StatusWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	
	switch(message)
	{
	case WM_CREATE:
		hdc = GetDC(hwnd);
		if(!hdc)
			DRAW_ERROR();
		if(!SetBkMode(hdc, TRANSPARENT))
			DRAW_ERROR();
		if(!ReleaseDC(hwnd, hdc))
			DRAW_ERROR();
		return 0;
		
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if(!hdc)
			DRAW_ERROR();
		
		if(!GetClientRect(hwnd, &rect))
			DRAW_ERROR();
		
		if(!SelectObject(hdc, GetStockObject(NULL_BRUSH)))
			SELECT_ERROR();
		if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
			SELECT_ERROR();
		if(!Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom))
			DRAW_ERROR();
		if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
			SELECT_ERROR();
		if(!MoveToEx(hdc, rect.left, rect.bottom-1, NULL))
			DRAW_ERROR();
		if(!LineTo(hdc, rect.right-1, rect.bottom-1))
			DRAW_ERROR();
		if(!LineTo(hdc, rect.right-1, rect.top))
			DRAW_ERROR();

		if(!DrawText(hdc, TEXT(statusMessage), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE))
			DRAW_ERROR();
		
		if(!EndPaint(hwnd, &ps))
			DRAW_ERROR();
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
ButtonsWND(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	static HBRUSH hBrush;
	int i;
	
	switch(message)
	{
	case WM_COMMAND:
		if (!windowAdjustFlag) {
			button[LOWORD(wParam) - 200].fcn(invalidate_screen);
			if (windowAdjustFlag) {
				adjustButton = LOWORD(wParam) - 200;
				button[adjustButton].ispressed = 1;
				for (i=0; i<num_buttons; i++) {
					EnableWindow(button[i].hwnd, FALSE);
					SendMessage(button[i].hwnd, BM_SETSTATE, button[i].ispressed, 0);
				}
			}
		}
		SetFocus(hMainWnd);
		return 0;
		
	case WM_CREATE:
		hdc = GetDC(hwnd);
		if(!hdc)
			DRAW_ERROR();
		hBrush = CreateSolidBrush(win32_colors[LIGHTGREY]);
		if(!hBrush)
			CREATE_ERROR();
		if(!SelectObject(hdc, hBrush))
			SELECT_ERROR();
		if(!SetBkMode(hdc, TRANSPARENT))
			DRAW_ERROR();
		if(!ReleaseDC(hwnd, hdc))
			DRAW_ERROR();
		
		return 0;
		
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if(!hdc)
			DRAW_ERROR();
		
		if(!GetClientRect(hwnd, &rect))
			DRAW_ERROR();
		
		if(!SelectObject(hdc, GetStockObject(NULL_BRUSH)))
			SELECT_ERROR();
		if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
			SELECT_ERROR();
		if(!Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom))
			DRAW_ERROR();
		if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
			SELECT_ERROR();
		if(!MoveToEx(hdc, rect.left, rect.bottom-1, NULL))
			DRAW_ERROR();
		if(!LineTo(hdc, rect.right-1, rect.bottom-1))
			DRAW_ERROR();
		if(!LineTo(hdc, rect.right-1, rect.top))
			DRAW_ERROR();

		for (i=0; i < num_buttons; i++) {
			if(button[i].type == BUTTON_SEPARATOR) {
				int x, y, w;

				x = button[i].xleft;
				y = button[i].ytop;
				w = button[i].width;

				if(!MoveToEx (hdc, x, y, NULL))
					DRAW_ERROR();
				if(!LineTo (hdc, x + w, y))
					DRAW_ERROR();			
				if(!SelectObject(hdc, GetStockObject(WHITE_PEN)))
					SELECT_ERROR();
				if(!MoveToEx (hdc, x, y+1, NULL))
					DRAW_ERROR();
				if(!LineTo (hdc, x + w, y+1))
					DRAW_ERROR();			
				if(!SelectObject(hdc, GetStockObject(BLACK_PEN)))
					SELECT_ERROR();
			}
		}
		if(!EndPaint(hwnd, &ps))
			DRAW_ERROR();
		return 0;
		
	case WM_DESTROY:
		for (i=0; i<num_buttons; i++) {
		}
		if(!DeleteObject(hBrush))
			DELETE_ERROR();
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}


void reset_win32_state () {
   // Not sure exactly what needs to be reset to NULL etc. 
   // Resetting everthing to be safe.
   hGraphicsPen = 0;
   hGraphicsBrush = 0;
   hGrayBrush = 0;
   hGraphicsDC = 0;
   hForegroundDC = 0;
   hBackgroundDC = 0;
   hCurrentDC = 0, /* WC : double-buffer */
   hObjtestDC = 0;
   hAllObjtestDC = 0; /* object test */

   hGraphicsFont = 0;

   /* These are used for the "Window" graphics button. They keep track of whether we're entering
    * the window rectangle to zoom to, etc.
    */
   windowAdjustFlag = 0;
   adjustButton = -1;
   InEventLoop = FALSE;
}


void win32_drain_message_queue () {
   // Drain the message queue, so we don't have a quit message lying around
   // that will stop us from re-opening a window later if desired.
   MSG msg; 
   while (PeekMessage(&msg, hMainWnd,  0, 0, PM_REMOVE)) { 
      if (msg.message == WM_QUIT) {
         printf ("Got the quit message.\n");
      }
   } 
}


void drawtobuffer(void) {
	hGraphicsDC = hBackgroundDC;
}


void drawtoscreen(void) {
	hGraphicsDC = hForegroundDC;
}


void displaybuffer(void) {
    if (!BitBlt(hForegroundDC, xcoord(xleft), ycoord(ytop), 
		xcoord(xright)-xcoord(xleft), ycoord(ybot)-ycoord(ytop), hBackgroundDC,//hAllObjtestDC,
		0, 0, SRCCOPY))
		DRAW_ERROR();
}


static void _drawcurve(t_point *points, int npoints, int fill) {
/* Draw a beizer curve.
 * Must have 3I+1 points, since each Beizer curve needs 3 points and we also 
 * need an initial starting point 
 */
	HPEN hOldPen;
	HBRUSH hOldBrush;
	float xmin, ymin, xmax, ymax;
	int i;
	
	if ((npoints - 1) % 3 != 0 || npoints > MAXPTS)
		DRAW_ERROR();
	
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
		POINT pts[MAXPTS];
		int i;

		for (i = 0; i < npoints; i++) {
			pts[i].x = xcoord(points[i].x);
			pts[i].y = ycoord(points[i].y);
		}

		if (!fill) {
			hOldPen = (HPEN)SelectObject(hGraphicsDC, hGraphicsPen);
			if(!(hOldPen))
				SELECT_ERROR();
		}
		else {
			hOldPen = (HPEN)SelectObject(hGraphicsDC, GetStockObject(NULL_PEN));
			if(!(hOldPen))
				SELECT_ERROR();
			hOldBrush = (HBRUSH)SelectObject(hGraphicsDC, hGraphicsBrush);
			if(!(hOldBrush))
				SELECT_ERROR();
		}

		if (!BeginPath(hGraphicsDC))
			DRAW_ERROR();
		if(!PolyBezier(hGraphicsDC, pts, npoints))
			DRAW_ERROR();
		if (!EndPath(hGraphicsDC))
			DRAW_ERROR();

		if (!fill) {
			if (!StrokePath(hGraphicsDC))
				DRAW_ERROR();
		}
		else {
			if (!FillPath(hGraphicsDC))
				DRAW_ERROR();
		}

		if(!SelectObject(hGraphicsDC, hOldPen))
			SELECT_ERROR();

		if (fill) {
			if(!SelectObject(hGraphicsDC, hOldBrush))
				SELECT_ERROR();
		}
#endif
	}
	else {
		int i;

		fprintf(ps, "newpath\n");
		fprintf(ps, "%.2f %.2f moveto\n", XPOST(points[0].x), YPOST(points[0].y));
		for (i = 1; i < npoints; i+= 3)
			fprintf(ps,"%.2f %.2f %.2f %.2f %.2f %.2f curveto\n", XPOST(points[i].x), YPOST(points[i].y),
			XPOST(points[i+1].x), YPOST(points[i+1].y), XPOST(points[i+2].x), YPOST(points[i+2].y));
		if (!fill)
			fprintf(ps, "stroke\n");
		else
			fprintf(ps, "fill\n");
	}
}


void drawcurve(t_point *points,
			   int npoints) {
	_drawcurve(points, npoints, 0);
}


void fillcurve(t_point *points,
			   int npoints) {
	_drawcurve(points, npoints, 1);
}


void object_start(int all) {
	if (all)
		hGraphicsDC = hAllObjtestDC;
	else
		hGraphicsDC = hObjtestDC;
	setcolor(WHITE);
	fillrect (xleft, ytop, xright, ybot);
	setcolor(BLACK);
}


void object_end() {
	hGraphicsDC = hCurrentDC; 

}


int pt_on_object(int all, float x, float y) {
	COLORREF c;

	if (all)
		c = GetPixel(hAllObjtestDC, xcoord(x), ycoord(y));
	else
		c = GetPixel(hObjtestDC, xcoord(x), ycoord(y));

//	printf("c = %x\n", c);

	return c == win32_colors[BLACK];
}

static int check_fontsize(int pointsize,
						  float ymax) {
	// return 0 if matches, 1 if font too big, -1 if font too small
	// a font matches if it's height is 90-100% of ymax tall
	float height;
	TEXTMETRIC textmetric;
	HFONT hOldFont;
	int ret;

	setfontsize(pointsize);
	
	hOldFont = (HFONT)SelectObject(hGraphicsDC, hGraphicsFont);
	if(!(hOldFont))
		SELECT_ERROR();

	if (!GetTextMetrics(hGraphicsDC, &textmetric))
		DRAW_ERROR();
	height = (textmetric.tmAscent + 2 * textmetric.tmDescent) / ymult;

	if (height >= ymax * 0.9) {
		if (height <= ymax)
			ret = 0;
		else
			ret = 1;
	}
	else
		ret = -1;

	if(!SelectObject(hGraphicsDC, hOldFont))
		SELECT_ERROR();

	return ret;
}

int findfontsize(float ymax) {
// find the correct point size which will fit in the specified ymax as the max 
// height of the font, using a binary search 
	int bot = 1;
	int top = MAX_FONT_SIZE;
	int mid, check;

	while (bot <= top) {
		mid = (bot+top)/2;

		check = check_fontsize(mid, ymax);
		if (!(check))
			return mid;
		else if (check > 0) // too big
			top = mid - 1;
		else // too small
			bot = mid + 1;
	}
	if (bot > MAX_FONT_SIZE)
		return MAX_FONT_SIZE;

	return -1; // can't fit
}

#endif /******** Win32 Specific Definitions ********************/


#else  /***** NO_GRAPHICS *******/
/* No graphics at all. Stub everything out so calling program doesn't have to change
 * but of course graphics won't do anything.
 */

#include "graphics.h"

void event_loop (void (*act_on_mousebutton) (float x, float y),
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
void drawellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent) { }

void fillarc (float xcen, float ycen, float rad, float startang,
			  float angextent) { }
void fillellipticarc (float xc, float yc, float radx, float rady, float startang, float angextent) { }

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
void drawtobuffer(void) { }

void drawtoscreen(void) { }

void displaybuffer(void) { }

void drawcurve(t_point *points, int npoints) { }

void fillcurve(t_point *points, int npoints) { }

void object_start(int all) { }

void object_end() { }

int pt_on_object(float x, float y) { }

int findfontsize(float ymax) { }


#endif  // WIN32 (subset of commands)

#endif  // NO_GRAPHICS
