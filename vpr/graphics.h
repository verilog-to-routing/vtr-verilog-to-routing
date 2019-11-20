/* #define DEBUG 1 */

#define SCREEN 0 
#define POSTSCRIPT 1 

#define NUM_COLOR 9
enum color_types {WHITE, BLACK, DARKGREY, LIGHTGREY, BLUE, GREEN, YELLOW,
   CYAN, RED};

enum line_types {SOLID, DASHED};

#define MAXPTS 100    /* Maximum number of points drawable by fillpoly */
typedef struct {float x; float y;} s_point; /* Used in calls to fillpoly */

void event_loop (void (*act_on_button) (float x, float y));  
/* Routine for X Windows Input.  act_on_button responds to buttons  *
 * being pressed in the graphics area.                              */

void drawscreen (void); /* User's drawing routine */
void init_graphics (char *window_name);  /* Initializes X display */ 
void close_graphics (void); /* Closes X display */

/* Changes message in text area. */
void update_message (char *msg);

/* Normal users shouldn't have to use draw_message.  Should only be *
 * useful if using non-interactive graphics and you want to redraw  *
 * yourself because of an expose.                                   */
void draw_message (void);     

 /* Sets world coordinates */
void init_world (float xl, float yt, float xr, float yb); 
void flushinput (void);   /* Empties event queue */

/* Following routines draw to SCREEN if disp_type = SCREEN *
 * and to a PostScript file if disp_type = POSTSCRIPT      */
void setcolor (int cindex);  /* Use a constant from clist */
void setlinestyle (int linestyle);  
void drawline (float x1, float y1, float x2, float y2);
void drawrect (float x1, float y1, float x2, float y2);
void fillrect (float x1, float y1, float x2, float y2);
void fillpoly (s_point *points, int npoints); 
void drawarc (float xcen, float ycen, float rad, float startang,
  float angextent); /* Draw a circular arc.  Angles in degrees.          *
                     * startang measured from positive x-axis of Window. *
                     * Positive angextent means counterclockwise arc.    */
/* boundx specifies horizontal bounding box.  If text won't fit in    *
 * the space specified by boundx (world coordinates) text isn't drawn */
void drawtext (float xc, float yc, char *text, float boundx);
void clearscreen (void);   /* Erases the screen */

/* Postscript stuff */
/* Opens file for postscript commands and initializes it *
 * All subsequent drawing commands go to this file until *
 * close_postscript is called.                              */
int init_postscript (char *fname);   /* Returns 1 if successful */

/* Closes file and directs output to screen again.       */
void close_postscript (void);      

