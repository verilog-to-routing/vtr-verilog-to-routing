#define SCREEN 0
#define POSTSCRIPT 1

enum color_types
{ WHITE, BLACK, DARKGREY, LIGHTGREY, BLUE, GREEN, YELLOW,
  CYAN, RED, DARKGREEN, MAGENTA, BISQUE, LIGHTBLUE, THISTLE, PLUM, KHAKI, 
  CORAL, TURQUOISE, MEDIUMPURPLE, DARKSLATEBLUE, DARKKHAKI, NUM_COLOR
};

enum line_types
{ SOLID, DASHED };

#define MAXPTS 100		/* Maximum number of points drawable by fillpoly */
typedef struct
{
    float x;
    float y;
}
t_point;			/* Used in calls to fillpoly */


/* Routine for X Windows Input.  act_on_button responds to buttons  *
 * being pressed in the graphics area.  drawscreen is the user's    *
 * routine that can redraw all the graphics.                        */

void event_loop(void (*act_on_button) (float x,
				       float y),
		void (*drawscreen) (void));

void init_graphics(char *window_name);	/* Initializes X display */
void close_graphics(void);	/* Closes X display      */

/* Changes message in text area. */
void update_message(char *msg);

/* Normal users shouldn't have to use draw_message.  Should only be *
 * useful if using non-interactive graphics and you want to redraw  *
 * yourself because of an expose.                                   */

void draw_message(void);

/* Sets world coordinates */
void init_world(float xl,
		float yt,
		float xr,
		float yb);

void flushinput(void);		/* Empties event queue */

/* Following routines draw to SCREEN if disp_type = SCREEN *
 * and to a PostScript file if disp_type = POSTSCRIPT      */

void setcolor(int cindex);	/* Use a constant from clist */
void setlinestyle(int linestyle);
void setlinewidth(int linewidth);
void setfontsize(int pointsize);
void drawline(float x1,
	      float y1,
	      float x2,
	      float y2);
void drawrect(float x1,
	      float y1,
	      float x2,
	      float y2);
void fillrect(float x1,
	      float y1,
	      float x2,
	      float y2);
void fillpoly(t_point * points,
	      int npoints);

/* Draw or fill a circular arc, respectively.  Angles in degrees.  startang  *
 * measured from positive x-axis of Window.  Positive angextent means        *
 * counterclockwise arc.                                                     */

void drawarc(float xcen,
	     float ycen,
	     float rad,
	     float startang,
	     float angextent);
void fillarc(float xcen,
	     float ycen,
	     float rad,
	     float startang,
	     float angextent);

/* boundx specifies horizontal bounding box.  If text won't fit in    *
 * the space specified by boundx (world coordinates) text isn't drawn */

void drawtext(float xc,
	      float yc,
	      const char *text,
	      float boundx);
void clearscreen(void);		/* Erases the screen */


/* Functions for creating and destroying extra menu buttons. */

void create_button(char *prev_button_text,
		   char *button_text,
		   void (*button_func) (void (*drawscreen) (void)));
void destroy_button(char *button_text);


/* Opens file for postscript commands and initializes it.  All subsequent  *
 * drawing commands go to this file until close_postscript is called.      */

int init_postscript(char *fname);	/* Returns 1 if successful */

/* Closes file and directs output to screen again.       */

void close_postscript(void);
