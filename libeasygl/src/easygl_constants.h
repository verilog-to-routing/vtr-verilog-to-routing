#ifndef EASYGL_CONSTANTS_H
#define EASYGL_CONSTANTS_H

/**
 * Some typical (and non typical...) colours, mostly taken from the X11 colours, and put in an enum,
 * so that you can get some reasonable colours from an index variable or something, and so you
 * don't have to generate your own. NUM_COLOR's value should be the same as the number of colours.
 *
 * If you would like to add you own, add them here, and to predef_colors and ps_colors in graphcis.c
 * Note that general custom colour constants should probably just be const t_color variables.
 */
enum color_types {
	WHITE = 0, BLACK, DARKGREY, LIGHTGREY,
	RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, // standard rainbow colours.
	PINK, LIGHTPINK, DARKGREEN, MAGENTA, // some other colours
	BISQUE,    // A weird colour, not unlike the "peach" colour of pencil crayons, but closer to "Blanched Almond" and "Moccasin"
	LIGHTSKYBLUE,    // A nice light blue
	THISTLE,   // A sort of desaturated purple, the colour of thistle flowers
	PLUM,      // much closer to pink than the colour of actual plums, and closer to its flower's colour
	KHAKI,     // Wikipedia says "a light shade of yellow-green" , but this one is more yellow, and very light
	CORAL,     // A burnt pinkish-orange perhaps?
	TURQUOISE, // turquoise
	MEDIUMPURPLE,    // A nice medium purple
	DARKSLATEBLUE,   // A deep blue, almost purple
	DARKKHAKI,       // darker khaki
	LIGHTMEDIUMBLUE, // A light blue, with nice contrast to white, but distinct from "BLUE" (NON-X11)
	SADDLEBROWN,  // The browest brown in X11
	FIREBRICK,    // dark red
	LIMEGREEN,    // a pleasing slightly dark green
	NUM_COLOR
};

/**
 * Line types for setlinestyle(..)
 */
enum line_types {SOLID, DASHED};

#define MAXPTS 100    /* Maximum number of points drawable by fillpoly */

typedef struct {
	bool shift_pressed;  /* indicates whether a Shift key was pressed when a mouse button is pressed */
	bool ctrl_pressed;	/* indicates whether a Ctrl key was pressed when a mouse button is pressed */
	unsigned int button; /* indicates what button is pressed: left click is 1; right click is 3; */
						 /* scroll wheel click is 2; scroll wheel forward rotate is 4; */
						 /* scroll wheel backward is 5. */
} t_event_buttonPressed;   /* Used to pass information from event_loop when a mouse button or modifier key is pressed */

/**
 * Linux/Mac:
 *   The strings listed below will be submitted to fontconfig, in this priority,
 *   with point size specified, and it will try to find something that matches. If it doesn't
 *   find anything, it will try the next font.
 *
 *   If fontconfig can't find any that look like any of the fonts you specify, (unlikely with the defaults)
 *   you must find one that it will. Run `fc-match <test_font_name>` to test you font name, and then modify
 *   this array with that name string. If you'd like, run `fc-list` to list available fonts.
 *   Note:
 *     - The fc-* commands are a CLI frontend for fontconfig,
 *       so anything that works there will work in this graphics library.
 *     - The font returned by fontconfig may not be the same, but will look similar.
 *     - If particular characters aren't showing up, try using a font with support for them
 *     - A font with a wide range of unicode code point support is symbola, but you may have to install it.
 *
 * Windows:
 *   Something will similar effect will be done, but it is harder to tell what will happen.
 *   Check your installed fonts if the program exits immediately or while changing font size.
 *
 */
#define NUM_FONT_TYPES 3
const char* const fontname_config[] {
	"symbola",
	"helvetica",
	"lucida sans",
	"schumacher"
};

#endif // EASYGL_CONSTANTS_H
