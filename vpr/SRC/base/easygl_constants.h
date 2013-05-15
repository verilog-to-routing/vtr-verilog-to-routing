#ifndef EASYGL_CONSTANTS_H
#define EASYGL_CONSTANTS_H

enum color_types {WHITE, BLACK, DARKGREY, LIGHTGREY, BLUE, GREEN, YELLOW,
CYAN, RED, DARKGREEN, MAGENTA, BISQUE, LIGHTBLUE, THISTLE, PLUM, KHAKI, CORAL,
TURQUOISE, MEDIUMPURPLE, DARKSLATEBLUE, DARKKHAKI, NUM_COLOR};

enum line_types {SOLID, DASHED};

#define MAXPTS 100    /* Maximum number of points drawable by fillpoly */

typedef struct {
	float x; 
	float y;
} t_point; /* Used in calls to fillpoly */

#endif // EASYGL_CONSTANTS_H
