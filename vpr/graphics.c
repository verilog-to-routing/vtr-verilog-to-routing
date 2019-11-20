#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <math.h>
#include <stdlib.h>
 
#include <stdio.h>
#include <string.h>
#include "graphics.h"

/* Stuff specific to my placer below. */
#include "util.h"
#include "pr.h"
#include "ext.h"
/* Need to define BUSIZE so I include util.h.  ext.h brings in show_nets */
/* End specific secton. */

/* Written by Vaughn Betz.  Graphics package Version 1.1  *
 * Modified to incorporate PostScript Support: Jan. 13/95 *
 * Modified to cut the PostScript linewidth to printer    *
 * minimum:  March/95.                                    *
 * You may freely use this graphics interface, as long as *
 * you leave the written by Vaughn Betz message in it --  *
 * who knows, maybe someday an employer will see it and   *
 * give me a job or large sums of money :).               */



/* Macros for translation from world to PostScript coordinates */
#define XPOST(worldx) (((worldx)-xleft)*ps_xmult + ps_left)
#define YPOST(worldy) (((worldy)-ybot)*ps_ymult + ps_bot)

/* Macros to convert from X Windows Internal Coordinates to my  *
 * World Coordinates.                                           */
#define XTOWORLD(x) (((float) x)/xmult + xleft)
#define YTOWORLD(y) (((float) y)/ymult + ytop)

#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))

#define MWIDTH 104    /* width of menu window */
#define T_AREA_HEIGHT 24  /* Height of text window */
#define NBUTTONS 11   /* number of buttons    */

struct but {int width; int height; void (*fcn) (int bnum);
            Window win; int istext; char text[20]; int ispoly; 
            int poly[3][2]; int ispressed;} button[NBUTTONS];
static int disp_type;    /* Selects SCREEN or POSTSCRIPT */
static Display *display;
static int screen_num;
static GC gc, gcxor;
static XFontStruct *font_info;
static unsigned int display_width, display_height;  /* screen size */
static unsigned int top_width, top_height;      /* window size */
static Window toplevel, menu, textarea;  /* various windows */
static float xleft, xright, ytop, ybot;         /* world coordinates */
static float ps_left, ps_right, ps_top, ps_bot; /* Figure boundaries for *
                        * PostScript output, in PostScript coordinates.  */
static float ps_xmult, ps_ymult;     /* Transformation for PostScript. */
static float xmult, ymult;                      /* Transformation factors */
/* Used to store graphics state for switches between X11 and PostScript */
static int currentcolor = BLACK, currentlinestyle = SOLID;
static char message[BUFSIZE] = "\0"; /* User message to display */


static int colors[NUM_COLOR];

/* For PostScript output */
static  FILE *ps;

int xcoord (float worldx);
int ycoord (float worldy);

/* MAXPIXEL and MINPIXEL are set to prevent what appears to be *
 * overflow with very large pixel values on the Sun X Server.  */

#define MAXPIXEL 15000   
#define MINPIXEL -15000 

int xcoord (float worldx) {
/* Translates from my internal coordinates to X Windows coordinates   *
 * in the x direction.  Add 0.5 at end for extra half-pixel accuracy. */

 int winx;

 winx = (int) ((worldx-xleft)*xmult + 0.5);
 
/* Avoid overflow in the X Window routines.  This will allow horizontal  *
 * and vertical lines to be drawn correctly regardless of zooming, but   *
 * will cause diagonal lines that go way off screen to change their      *
 * slope as you zoom in.  The only way I can think of to completely fix  *
 * this problem is to do all the clipping in advance in floating point,  *
 * then convert to integers and call X Windows.  This is a lot of extra  *
 * coding, and means that coordinates will be clipped twice, even though *
 * this "Super Zoom" problem won't occur unless users zoom way in on     * 
 * the graphics.                                                         */ 

 winx = max (winx, MINPIXEL);
 winx = min (winx, MAXPIXEL);

 return (winx);
}


int ycoord (float worldy) {
/* Translates from my internal coordinates to X Windows coordinates   *
 * in the y direction.  Add 0.5 at end for extra half-pixel accuracy. */

 int winy;

 winy = (int) ((worldy-ytop)*ymult + 0.5);
 
/* Avoid overflow in the X Window routines. */
 winy = max (winy, MINPIXEL);
 winy = min (winy, MAXPIXEL);

 return (winy);
}


void init_graphics (char *window_name) {
 /* Open the toplevel window, get the colors, 2 graphics  *
  * contexts, load a font, and set up the toplevel window *
  * Calls build_menu to set up the menu.                  */

 void build_textarea (void);
 void build_menu (void);  
 char *display_name = NULL;
 int x, y;                                   /* window position */
 unsigned int border_width = 2;  /* ignored by OpenWindows */
 XTextProperty windowName;

 XColor exact_def;
 Colormap default_cmap;
 char *cnames[NUM_COLOR] = {"white", "black", "grey55", "grey75",
    "blue", "green", "yellow", "cyan", "red" };
 int i;                                     /* just a counter */
 
 void load_font(XFontStruct **font_info);

 unsigned long valuemask = 0; /* ignore XGCvalues and use defaults */
 XGCValues values;

 disp_type = SCREEN;         /* Graphics go to screen, not ps */

 /* connect to X server */
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

 x = y = 0;
        
 top_width = 2*display_width/3;
 top_height = 4*display_height/5;
 
 default_cmap = DefaultColormap(display, screen_num);
 for (i=0;i<NUM_COLOR;i++) {
    if (!XParseColor(display,default_cmap,cnames[i],&exact_def)) {
       fprintf(stderr, "Color name %s not in database", cnames[i]);
       exit(-1);
    }
    if (!XAllocColor(display, default_cmap, &exact_def)) {
       fprintf(stderr, "Couldn't allocate color %s.\n",cnames[i]); 
       exit(-1);
    }
    colors[i] = exact_def.pixel;
 }
 toplevel = XCreateSimpleWindow(display,RootWindow(display,screen_num),
          x, y, top_width, top_height, border_width, colors[BLACK],
          colors[WHITE]);  

 /* hints stuff deleted. */

 XSelectInput (display, toplevel, ExposureMask | StructureNotifyMask |
       ButtonPressMask);
 
 load_font(&font_info);

 /* Create default Graphics Context */
 gc = XCreateGC(display, toplevel, valuemask, &values);

 /* Create XOR graphics context for Rubber Banding */
 values.function = GXxor;   
 values.foreground = colors[BLACK];
 gcxor = XCreateGC(display, toplevel, (GCFunction | GCForeground),
       &values);
 
 /* specify font */
 XSetFont(display, gc, font_info->fid);
 
 XSetForeground(display, gc, colors[RED]);

 XStringListToTextProperty(&window_name, 1, &windowName);
 XSetWMName (display, toplevel, &windowName);
/* XSetWMIconName (display, toplevel, &windowName); */

 
 /* set line attributes */
/* XSetLineAttributes(display, gc, line_width, line_style,
           cap_style, join_style); */
 
 /* set dashes */
 /* XSetDashes(display, gc, dash_offset, dash_list, list_length); */

 XMapWindow (display, toplevel);
 build_textarea();
 build_menu();
}

void load_font(XFontStruct **font_info)
{
/*  char *fontname = "9x15"; */
/* Use ten point medium-weight upright helvetica font */
  char *fontname = "-*-helvetica-medium-r-*-*-*-140-*-*-*-*-*-*"; 

  /* Load font and get font information structure. */
  if ((*font_info = XLoadQueryFont(display,fontname)) == NULL) {
     fprintf( stderr, "Cannot open 9x15 font\n");
     exit( -1 );
  }
}

void event_loop (void (*act_on_button) (float x, float y)) {
/* The program's main event loop.  It assumes a user routine drawscreen *
 * (void) exists to redraw the screen.  It handles all window resizing  *
 * zooming etc. itself.  If the user clicks a button in the graphics    *
 * (toplevel) area, the act_on_button routine passed in is called.      */

 XEvent report;
 int bnum;
 float x, y;

 void drawmenu (void);
 void drawbut(int bnum);
 int which_button (Window win); 
 void update_transform (void);
 void proceed (int bnum);
 void turn_on_off (int pressed);

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
          act_on_button (x, y);
       } 
       else {  /* A menu button was pressed. */
          bnum = which_button(report.xbutton.window);
#ifdef VERBOSE 
       printf("Button number is %d\n",bnum);
#endif
          button[bnum].ispressed = 1;
          drawbut(bnum);
          XFlush(display);  /* Flash the button */
          button[bnum].fcn(bnum);
          button[bnum].ispressed = 0;
          drawbut(bnum);
          if (button[bnum].fcn == proceed) {
             turn_on_off(OFF);
             flushinput ();
             return;  /* Rather clumsy way of returning *
                       * control to the simulator       */
          }
       }
       break;
    }
 }
}

void turn_on_off (int pressed) {
/* Shows when the menu is active or inactive by colouring the *
 * buttons.                                                   */
 int i;
 void drawbut(int bnum);

 for (i=0;i<NBUTTONS;i++) {
    button[i].ispressed = pressed;
    drawbut(i);
 }
}

int which_button (Window win) {
 int i;

 for (i=0;i<NBUTTONS;i++) {
    if (button[i].win == win)
       return(i);
 }
 printf("Error:  Unknown button ID in which_button.\n");
 return(0);
}

void drawmenu(void) {
 int i;
 void drawbut(int bnum);

 for (i=0;i<NBUTTONS;i++)  {
    drawbut(i);
 }
}

void clearscreen (void) {

 if (disp_type == SCREEN) {
    XClearWindow (display, toplevel);
 }
 else {
/* erases current page.  Don't use erasepage, since this will erase *
 * everything, (even stuff outside the clipping path) causing       *
 * problems if this picture is incorporated into a larger document. */
    fprintf(ps,"1 setgray\n");
    fprintf(ps,"clippath fill\n\n");
    setcolor (currentcolor);
 }
}

void drawline (float x1, float y1, float x2, float y2) {
 if (disp_type == SCREEN) {
    /* Xlib.h prototype has x2 and y1 mixed up. */ 
    XDrawLine(display, toplevel, gc, xcoord(x1), ycoord(y1), 
       xcoord(x2), ycoord(y2));
 }
 else {
    fprintf(ps,"%f %f moveto\n",XPOST(x1),YPOST(y1)); 
    fprintf(ps,"%f %f lineto\n",XPOST(x2),YPOST(y2));
    fprintf(ps,"stroke\n\n");
 }
}

void drawrect (float x1, float y1, float x2, float y2) {
/* (x1,y1) and (x2,y2) are diagonally opposed corners. */
 unsigned int width, height;
 int xw1, yw1, xw2, yw2, xl, yt;

 if (disp_type == SCREEN) { 
/* translate to X Windows calling convention. */
    xw1 = xcoord(x1);
    xw2 = xcoord(x2);
    yw1 = ycoord(y1);
    yw2 = ycoord(y2); 
    xl = min(xw1,xw2);
    yt = min(yw1,yw2);
    width = abs (xw1-xw2);
    height = abs (yw1-yw2);
    XDrawRectangle(display, toplevel, gc, xl, yt, width, height);
 }
 else {
    fprintf(ps,"%f %f moveto\n",XPOST(x1),YPOST(y1)); 
    fprintf(ps,"%f %f lineto\n",XPOST(x1),YPOST(y2));
    fprintf(ps,"%f %f lineto\n",XPOST(x2),YPOST(y2));
    fprintf(ps,"%f %f lineto\n",XPOST(x2),YPOST(y1));
    fprintf(ps,"closepath\n");
    fprintf(ps,"stroke\n\n");
 }
}

void fillrect (float x1, float y1, float x2, float y2) {
/* (x1,y1) and (x2,y2) are diagonally opposed corners. */
 unsigned int width, height;
 int xw1, yw1, xw2, yw2, xl, yt;

 if (disp_type == SCREEN) {
/* translate to X Windows calling convention. */
    xw1 = xcoord(x1);
    xw2 = xcoord(x2);
    yw1 = ycoord(y1);
    yw2 = ycoord(y2); 
    xl = min(xw1,xw2);
    yt = min(yw1,yw2);
    width = abs (xw1-xw2);
    height = abs (yw1-yw2);
    XFillRectangle(display, toplevel, gc, xl, yt, width, height);
 }
 else {
    fprintf(ps,"%f %f moveto\n",XPOST(x1),YPOST(y1)); 
    fprintf(ps,"%f %f lineto\n",XPOST(x1),YPOST(y2));
    fprintf(ps,"%f %f lineto\n",XPOST(x2),YPOST(y2));
    fprintf(ps,"%f %f lineto\n",XPOST(x2),YPOST(y1));
    fprintf(ps,"closepath\n");
    fprintf(ps,"fill\n\n");
 }
}

void drawarc (float xc, float yc, float rad, float startang, 
 float angextent) {
/* Draws a circular arc.  X11 can do elliptical arcs quite simply, and *
 * PostScript could do them by scaling the coordinate axes.  Too much  *
 * work for now, and probably too complex an object for users to draw  *
 * much, so I'm just doing circular arcs.  Startang is relative to the *
 * Window's positive x direction.  Angles in degrees.                  */
 int xl, yt;
 unsigned int width, height;
 float angnorm (float ang);

/* X Windows has trouble with very large angles. (Over 360).    *
 * Do following to prevent its inaccurate (overflow?) problems. */
 if (fabs(angextent) > 360.) angextent = 360.;
 startang = angnorm (startang);
 
 if (disp_type == SCREEN) {
    xl = (int) (xcoord(xc) - fabs(xmult*rad));
    yt = (int) (ycoord(yc) - fabs(ymult*rad));
    width = (unsigned int) (2*fabs(xmult*rad));
    height = width;
    XDrawArc (display, toplevel, gc, xl, yt, width, height,
      (int) (startang*64), (int) (angextent*64));
 }
 else {
    fprintf(ps,"%f %f %f %f %f %s\n",XPOST(xc), YPOST(yc),
       fabs(rad*ps_xmult), startang, startang+angextent, 
       (angextent < 0) ? "arcn" : "arc") ;
    fprintf(ps,"stroke\n\n");
 }
}

float angnorm (float ang) {
/* Normalizes an angle to be between 0 and 360 degrees. */

 int scale;

 if (ang < 0) {
    scale = (int) (ang / 360. - 1);
 }
 else {
    scale = (int) (ang / 360.);
 }
 ang = ang - scale * 360.;
 return (ang);
}

void fillpoly (s_point *points, int npoints) {
 XPoint transpoints[MAXPTS];
 int i;

 if (npoints > MAXPTS) {
    printf("Error in fillpoly:  Only %d points allowed per polygon.\n",
       MAXPTS);
    printf("%d points were requested.  Polygon is not drawn.\n",npoints);
    return;
 }

 if (disp_type == SCREEN) {
    for (i=0;i<npoints;i++) {
        transpoints[i].x = (short) xcoord (points[i].x);
        transpoints[i].y = (short) ycoord (points[i].y);
    }
    XFillPolygon(display, toplevel, gc, transpoints, npoints, Complex,
      CoordModeOrigin);
 }
 else {
    fprintf(ps,"%f %f moveto\n",XPOST(points[0].x),YPOST(points[0].y));
    for (i=1;i<npoints;i++) 
       fprintf(ps,"%f %f lineto\n",XPOST(points[i].x),YPOST(points[i].y));
    fprintf(ps,"closepath fill\n\n");
 }
}

void drawtext (float xc, float yc, char *text, float boundx) {
/* Draws text centered on xc,yc if it fits in boundx */
 int len, width; 
 
 len = strlen(text);
 width = XTextWidth(font_info, text, len);
 if (width > fabs(boundx*xmult)) return; /* Don't draw if it won't fit */
 if (disp_type == SCREEN) {
    XDrawString(display, toplevel, gc, xcoord(xc)-width/2, 
        ycoord(yc) + font_info->ascent/2, text, len);
 }
 else {
    fprintf(ps,"%f %f moveto\n",XPOST(xc),YPOST(yc));
    fprintf(ps,"(%s) censhow\nnewpath\n\n",text);
 }
}

void setcolor (int cindex) {
/* Postscript command for each color */
 char *colordefs[NUM_COLOR] = {
  "1 setgray", "0 setgray", ".55 setgray", ".75 setgray", 
  "0 0 1 setrgbcolor", "0 1 0 setrgbcolor", "1 1 0 setrgbcolor",
  "0 1 1 setrgbcolor", "1 0 0 setrgbcolor"};
 currentcolor = cindex;  /* Only needed for postscript - X switches */
 if (disp_type == SCREEN) {
    XSetForeground(display, gc,colors[cindex]);
 }
 else {
    fprintf(ps,"%s\n",colordefs[cindex]);
 }
}

void setlinestyle (int linestyle) {
/* Note SOLID is 0 and DASHED is 1 for linestyle.                      */

 char *ps_text[2] = {"[] 0", "[3 3] 0"};  /* PostScript commands needed */
 int x_vals[2] = {LineSolid, LineOnOffDash};

 currentlinestyle = linestyle; /* Only needed for PostScript X switches */
 if (disp_type == SCREEN) {
    XSetLineAttributes (display, gc, 0, x_vals[linestyle], CapButt, JoinMiter);
 }
 else {
    fprintf(ps,"%s setdash\n",ps_text[linestyle]);
 }
}

void flushinput (void) {
 if (disp_type != SCREEN) return;
 XFlush(display);
}

void init_world (float x1, float y1, float x2, float y2) {
 void update_transform (void); 
 void update_ps_transform (void);

 xleft = x1;
 xright = x2;
 ytop = y1;
 ybot = y2;
 if (disp_type == SCREEN) {
    update_transform();
 }
 else {
    update_ps_transform();
 }
}

void update_transform (void) {
/* Set up the factors for transforming from the user world to X Windows *
 * coordinates.                                                         */

 float mult, y1, y2, x1, x2;
/* X Window coordinates go from (0,0) to (width-1,height-1) */
 xmult = ((float) top_width - 1. - MWIDTH) / (xright - xleft);
 ymult = ((float) top_height - 1. - T_AREA_HEIGHT)/ (ybot - ytop);
/* Need to use same scaling factor to preserve aspect ratio */
 if (fabs(xmult) <= fabs(ymult)) {
    mult = fabs(ymult/xmult);
    y1 = ytop - (ybot-ytop)*(mult-1.)/2.;
    y2 = ybot + (ybot-ytop)*(mult-1.)/2.;
    ytop = y1;
    ybot = y2;
 }
 else {
    mult = fabs(xmult/ymult);
    x1 = xleft - (xright-xleft)*(mult-1.)/2.;
    x2 = xright + (xright-xleft)*(mult-1.)/2.;
    xleft = x1;
    xright = x2;
 }
 xmult = ((float) top_width - 1. - MWIDTH) / (xright - xleft);
 ymult = ((float) top_height - 1. - T_AREA_HEIGHT)/ (ybot - ytop);
}

void update_ps_transform (void) {
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
    ps_right = 36. + ps_width;
    ps_bot = 396. - fabs(ps_xmult * (ytop - ybot))/2.;
    ps_top = 396. + fabs(ps_xmult * (ytop - ybot))/2.;
/* Maintain aspect ratio but watch signs */
    ps_ymult = (ps_xmult*ps_ymult < 0) ? -ps_xmult : ps_xmult;
 }
 else {  
    ps_bot = 36.;
    ps_top = 36. + ps_height;
    ps_left = 306. - fabs(ps_ymult * (xright - xleft))/2.;
    ps_right = 306. + fabs(ps_ymult * (xright - xleft))/2.;
/* Maintain aspect ratio but watch signs */
    ps_xmult = (ps_xmult*ps_ymult < 0) ? -ps_ymult : ps_ymult;
 }
}

void build_textarea (void) {
/* Creates a small window at the top of the graphics area for text messages */

 XSetWindowAttributes menu_attributes;
 unsigned long valuemask;

 textarea = XCreateSimpleWindow(display,toplevel,
            0, top_height-T_AREA_HEIGHT, display_width, T_AREA_HEIGHT-4, 2,
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

void draw_message (void) {
/* Draw the current message in the text area at the screen bottom. */

 int len, width;
 float ylow;

 if (disp_type == SCREEN) {
    XClearWindow (display, textarea);
    len = strlen (message);
    width = XTextWidth(font_info, message, len);

    setcolor (BLACK);
    XDrawString(display, textarea, gc, (top_width - MWIDTH - width)/2,
       (T_AREA_HEIGHT - 4)/2 + font_info->ascent/2, message, len);
 }

 else {
/* Draw the message in the bottom margin.  Printer's generally can't  *
 * print on the bottom 1/4" (area with y < 18 in PostScript coords.)  */

    ylow = ps_bot - 8.; 
    fprintf(ps,"%f %f moveto\n", (ps_left + ps_right) / 2. , ylow);
    fprintf(ps,"(%s) censhow\nnewpath\n\n", message);
 }
}

void update_message (char *msg) {
/* Changes the message to be displayed on screen.   */

 strncpy (message, msg, BUFSIZE);
 draw_message ();
}

#define PI 3.141592654
void build_menu (void) {
/* Sets up all the menu buttons on the right hand side of the window. */

 XSetWindowAttributes menu_attributes;
 unsigned long valuemask;
 int i, xcen, x1, y1, bwid, bheight, space;
 void setpoly(int bnum, int xc, int yc, int r, float theta);
 void mapbut (int bnum, int x1, int y1, int width, int height);
 /* Function declarations for button responses */
 void translate(int bnum); 
 void zoom (int bnum);
 void adjustwin (int bnum); 
 void toggle_nets (int bnum);
 void postscript (int bnum);
 void proceed (int bnum);
 void quit (int bnum); 

 menu = XCreateSimpleWindow(display,toplevel,
          top_width-MWIDTH, 0, MWIDTH-4, display_height, 2,
          colors[BLACK], colors[LIGHTGREY]); 
 menu_attributes.event_mask = ExposureMask;
   /* Ignore button presses on the menu background. */
 menu_attributes.do_not_propagate_mask = ButtonPressMask;
   /* Keep menu on top right */
 menu_attributes.win_gravity = NorthEastGravity; 
 valuemask = CWWinGravity | CWEventMask | CWDontPropagate;
 XChangeWindowAttributes(display, menu, valuemask, &menu_attributes);
 XMapWindow (display, menu);

/* Now do the arrow buttons */
 bwid = 28;
 space = 3;
 y1 = 10;
 xcen = 51;
 x1 = xcen - bwid/2; 
 mapbut (0, x1, y1, bwid, bwid);
 setpoly (0, bwid/2, bwid/2, bwid/3, -PI/2.); /* Up */
 y1 += bwid + space;
 x1 = xcen - 3*bwid/2 - space;
 mapbut (1, x1, y1, bwid, bwid);
 setpoly (1, bwid/2, bwid/2, bwid/3, PI);  /* Left */
 x1 = xcen + bwid/2 + space;
 mapbut (2, x1, y1, bwid, bwid);
 setpoly (2, bwid/2, bwid/2, bwid/3, 0);  /* Right */
 y1 += bwid + space;
 x1 = xcen - bwid/2;
 mapbut (3, x1, y1, bwid, bwid);
 setpoly (3, bwid/2, bwid/2, bwid/3, +PI/2.);  /* Down */
 for (i=0;i<4;i++) {
    button[i].fcn = translate; 
    button[i].width = bwid;
    button[i].height = bwid;
 } 
 
/* Rectangular buttons */
 y1 += bwid + space + 6;
 space = 8;
 bwid = 90;
 bheight = 26;
 x1 = xcen - bwid/2;
 for (i=4;i<NBUTTONS;i++) {
    mapbut(i, x1, y1, bwid, bheight);
    y1 += bheight + space;
    button[i].istext = 1;
    button[i].ispoly = 0;
    button[i].width = bwid;
    button[i].height = bheight;
 }
 strcpy (button[4].text,"Zoom In");
 strcpy (button[5].text,"Zoom Out");
 strcpy (button[6].text,"Window");
 strcpy (button[7].text,"Toggle Nets"); 
 strcpy (button[8].text,"PostScript");
 strcpy (button[9].text,"Proceed");
 strcpy (button[10].text,"Exit");
 
 button[4].fcn = zoom;
 button[5].fcn = zoom;
 button[6].fcn = adjustwin;
 button[7].fcn = toggle_nets;
 button[8].fcn = postscript;
 button[9].fcn = proceed;
 button[10].fcn = quit;
 for (i=0;i<NBUTTONS;i++) {
    XSelectInput (display, button[i].win, ButtonPressMask);
    button[i].ispressed = 1;
 }
}

void mapbut (int bnum, int x1, int y1, int width, int height) {

 button[bnum].win = XCreateSimpleWindow(display,menu,
          x1, y1, width, height, 0, colors[WHITE], colors[LIGHTGREY]); 
 XMapWindow (display, button[bnum].win);
}
 
void drawbut (int bnum) {
/* Draws button bnum in either its pressed or unpressed state.    */

 int width, height, thick, i, ispressed;
 XPoint mypoly[6];
 void mytext(Window win, int xc, int yc, char *text);
 
 ispressed = button[bnum].ispressed;
 thick = 2;
 width = button[bnum].width;
 height = button[bnum].height;
/* Draw top and left edges of 3D box. */
 if (ispressed) {
    setcolor(BLACK);
 }
 else {
    setcolor(WHITE);
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
 XFillPolygon(display,button[bnum].win,gc,mypoly,6,Convex,CoordModeOrigin);

/* Draw bottom and right edges of 3D box. */
 if (ispressed) {
    setcolor(WHITE);
 }
 else {
    setcolor(BLACK);
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
 XFillPolygon(display,button[bnum].win,gc,mypoly,6,Convex,CoordModeOrigin);

/* Draw background */
 if (ispressed) {
    setcolor(DARKGREY);
 }
 else {
    setcolor(LIGHTGREY);
 }
/* Give x,y of top corner and width and height */
 XFillRectangle (display,button[bnum].win,gc,thick,thick,width-2*thick,
    height-2*thick);
 
/* Draw polygon, if there is one */
 if (button[bnum].ispoly) {
    for (i=0;i<3;i++) {
       mypoly[i].x = button[bnum].poly[i][0];
       mypoly[i].y = button[bnum].poly[i][1];
    }
    setcolor(BLACK);
    XFillPolygon(display,button[bnum].win,gc,mypoly,3,Convex,CoordModeOrigin);
 }
 
/* Draw text, if there is any */
 if (button[bnum].istext) {
    setcolor (BLACK);
    mytext(button[bnum].win,button[bnum].width/2,
      button[bnum].height/2,button[bnum].text);
 }
}

void mytext(Window win, int xc, int yc, char *text) {
/* draws text center at xc, yc */

 int len, width; 
 
 len = strlen(text);
 width = XTextWidth(font_info, text, len);
 XDrawString(display, win, gc, xc-width/2, yc + font_info->ascent/2,
     text, len);
}

void setpoly(int bnum, int xc, int yc, int r, float theta) {
/* Puts a triangle in the poly array for button[bnum] */

 int i;

 button[bnum].istext = 0;
 button[bnum].ispoly = 1;
 for (i=0;i<3;i++) {
    button[bnum].poly[i][0] = (int) (xc + r*cos(theta) + 0.5);
    button[bnum].poly[i][1] = (int) (yc + r*sin(theta) + 0.5);
    theta += 2*PI/3;
 }
}

void zoom(int bnum) {
/* Zooms in or out by a factor of 1.666. */

 float xdiff, ydiff;
 void update_transform (void); 

 xdiff = xright - xleft; 
 ydiff = ybot - ytop;
 if (strcmp(button[bnum].text,"Zoom In") == 0) {
    xleft += xdiff/5.;
    xright -= xdiff/5.;
    ytop += ydiff/5.;
    ybot -= ydiff/5.;
 }
 else {
    xleft -= xdiff/3.;
    xright += xdiff/3.;
    ytop -= ydiff/3.;
    ybot += ydiff/3.;
 }
 update_transform ();
 drawscreen();
}

void translate(int bnum) {
 float xstep, ystep;
 void update_transform (void); 

 xstep = (xright - xleft)/2.;
 ystep = (ybot - ytop)/2.;
 switch (bnum) {
 case 0:  ytop -= ystep;  /* up arrow */
          ybot -= ystep;
          break;
 case 1:  xleft -= xstep; /* left arrow */
          xright -= xstep; 
          break;
 case 2:  xleft += xstep;  /* right arrow */
          xright += xstep;
          break;
 case 3:  ytop += ystep;   /* down arrow */
          ybot += ystep;   
          break;
 default: printf("Graphics Error:  Unknown translation button.\n");
          printf("Got a button number of %d in routine translate.\n",
              bnum);
 }
 update_transform();         
 drawscreen();
}

void adjustwin (int bnum) {  
/* The window button was pressed.  Let the user click on the two *
 * diagonally opposed corners, and zoom in on this area.         */

 XEvent report;
 int corner, xold, yold, x[2], y[2];
 void drawmenu (void);
 void update_transform (void);
 void update_win (int x[2], int y[2]);

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
       XSelectInput (display, toplevel, ExposureMask | 
         StructureNotifyMask | ButtonPressMask | PointerMotionMask);
       }
       else {
          update_win(x,y);
       }
       corner++;
       break;
    case MotionNotify:
#ifdef VERBOSE 
       printf("Got a MotionNotify Event.\n");
       printf("x: %d    y: %d\n",report.xmotion.x,report.xmotion.y);
#endif
       if (xold >= 0) {  /* xold set -ve before we draw first box */
          XDrawRectangle(display,toplevel,gcxor,min(x[0],xold),
             min(y[0],yold),abs(x[0]-xold),abs(y[0]-yold));
       }
       /* Don't allow user to window under menu region */
       xold = min(report.xmotion.x,top_width-1-MWIDTH); 
       yold = report.xmotion.y;
       XDrawRectangle(display,toplevel,gcxor,min(x[0],xold),
          min(y[0],yold),abs(x[0]-xold),abs(y[0]-yold));
       break;
    }
 }
 XSelectInput (display, toplevel, ExposureMask | StructureNotifyMask
                | ButtonPressMask); 
}


void update_win (int x[2], int y[2]) {
 float x1, x2, y1, y2;
 void update_transform (void);
 
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

void toggle_nets (int bnum) {
 show_nets = abs(1-show_nets); 
 drawscreen();
}

void postscript (int bnum) {
/* Takes a snapshot of the screen and stores it in pic?.ps.  The *
 * first picture goes in pic1.ps, the second in pic2.ps, etc.    */

 static int piccount = 1;
 int success;
 char fname[20];

 sprintf(fname,"pic%d.ps",piccount);
 success = init_postscript (fname);
 if (success == 0) return;  /* Couldn't open file, abort. */
 drawscreen();
 close_postscript ();
 piccount++;
}

void proceed (int bnum) {
 /* Dummy routine.  Just exit the event loop. */

}

void quit(int bnum) {

 close_graphics();
 exit(0);
}

void close_graphics (void) {
 XUnloadFont(display,font_info->fid);
 XFreeGC(display,gc);
 XCloseDisplay(display);
}

int init_postscript (char *fname) {
/* Opens a file for PostScript output.  The header information,  *
 * clipping path, etc. are all dumped out.  If the file could    *
 * not be opened, the routine returns 0; otherwise it returns 1. */

 ps = fopen (fname,"w");
 if (ps == NULL) {
    printf("Error: could not open %s for PostScript output.\n", fname);
    printf("Drawing to screen instead.\n");
    return (0);
 }
 disp_type = POSTSCRIPT;  /* Graphics go to postscript file now. */

/* Header for minimal conformance with the Adobe structuring convention */
 fprintf(ps,"%%!PS-Adobe-1.0\n");
 fprintf(ps,"%%%%DocumentFonts: Helvetica\n");
 fprintf(ps,"%%%%Pages: 1\n");
/* Set up postscript transformation macros and page boundaries */
 update_ps_transform();
/* Bottom margin is at ps_bot - 14. to leave room for the on-screen message. */
 fprintf(ps,"%%%%BoundingBox: %f %f %f %f\n",ps_left, ps_bot - 14., ps_right,
      ps_top); 
 fprintf(ps,"%%%%EndComments\n");

/* Set up font and centering function */
 fprintf(ps,"\n/Helvetica findfont\n10 scalefont\nsetfont\n\n");
/* Find height of font to set up vertical centering */
 fprintf(ps,"0 0 moveto\n(X) true charpath\nflattenpath\n");
 fprintf(ps,
    "pathbbox /fontheight exch def pop pop pop %% get font height\n");
 fprintf(ps,"newpath\n");
 fprintf(ps,
  "fontheight -2 div /yoff exch def      %% vertical centering offset\n\n");
 fprintf(ps,"/censhow   %%draw a centered string\n");
 fprintf(ps," { dup stringwidth pop  %% get x length of string\n");
 fprintf(ps,"   -2 div               %% Proper left start\n");
 fprintf(ps, "   yoff rmoveto        %% Move left that much and down half font height\n");
 fprintf(ps,"   show } def           %% show the string\n\n"); 

/* Draw this in the bottom margin -- must do before the clippath is set */
 draw_message ();

/* Set clipping on page. */
 fprintf(ps,"%f %f moveto\n",ps_left, ps_bot);
 fprintf(ps,"%f %f lineto\n",ps_left, ps_top);
 fprintf(ps,"%f %f lineto\n",ps_right, ps_top);
 fprintf(ps,"%f %f lineto\n",ps_right,ps_bot);
 fprintf(ps,"closepath\n");
 fprintf(ps,"clip newpath\n\n");
/* Set linewidth to 0 units (Thinnest lines the device allows) */
 fprintf(ps,"0 setlinewidth\n\n");   /* Try 0. */
 
 fprintf(ps,"\n%%%%EndProlog\n");
 fprintf(ps,"%%%%Page: 1 1\n\n");
/* Set up PostScript graphics state to match current one. */
 setcolor (currentcolor);
 setlinestyle (currentlinestyle);

 return (1);
}

void close_postscript (void) {
 fprintf(ps,"showpage\n");
 fprintf(ps,"\n%%%%Trailer\n");
 fclose (ps);
 disp_type = SCREEN;
 update_transform();   /* Ensure screen world reflects any changes      *
                        * made while printing.                          */
 setcolor (currentcolor);
 setlinestyle (currentlinestyle);
}
