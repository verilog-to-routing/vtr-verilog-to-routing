#ifndef EASYGL_CONSTANTS_H
#define EASYGL_CONSTANTS_H

/* NO_COLOR can be used as a flag to indicate not drawn. WHITE is the first color. */
enum color_types {NO_COLOR = -1, WHITE = 0, BLACK, DARKGREY, LIGHTGREY, BLUE, GREEN, YELLOW,
CYAN, RED, PINK, LIGHTPINK, DARKGREEN, MAGENTA, BISQUE, LIGHTSKYBLUE, THISTLE, PLUM, KHAKI, CORAL,
TURQUOISE, MEDIUMPURPLE, DARKSLATEBLUE, DARKKHAKI, NUM_COLOR};

enum line_types {SOLID, DASHED};

#define MAXPTS 100    /* Maximum number of points drawable by fillpoly */

typedef struct {
	float x; 
	float y;
} t_point; /* Used in calls to fillpoly */

typedef struct {
	bool shift_pressed;  /* indicates whether a Shift key was pressed when a mouse button is pressed */
	bool ctrl_pressed;	/* indicates whether a Ctrl key was pressed when a mouse button is pressed */
	unsigned int button; /* indicates what button is pressed: left click is 1; right click is 3; */
						 /* scroll wheel click is 2; scroll wheel forward rotate is 4; */
						 /* scroll wheel backward is 5. */
} t_event_buttonPressed;   /* Used to pass information from event_loop when a mouse button or modifier key is pressed */

#endif // EASYGL_CONSTANTS_H
