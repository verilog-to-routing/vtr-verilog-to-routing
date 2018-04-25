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
    // Basic colors
    WHITE = 0,
    BLACK,

    // Remaining HTML colors
    ALICEBLUE,
    ANTIQUEWHITE,
    AQUA,
    AQUAMARINE,
    AZURE,
    BEIGE,
    BISQUE, // A weird colour, not unlike the "peach" colour of pencil crayons, but closer to "Blanched Almond" and "Moccasin"
    BLANCHEDALMOND,
    BLUE,
    BLUEVIOLET,
    BROWN,
    BURLYWOOD,
    CADETBLUE,
    CHARTREUSE,
    CHOCOLATE,
    CORAL, // A burnt pinkish-orange perhaps?
    CORNFLOWERBLUE,
    CORNSILK,
    CRIMSON,
    CYAN,
    DARKBLUE,
    DARKCYAN,
    DARKGOLDENROD,
    DARKGRAY,
    DARKGREEN,
    DARKGREY,
    DARKKHAKI,
    DARKMAGENTA,
    DARKOLIVEGREEN,
    DARKORANGE,
    DARKORCHID,
    DARKRED,
    DARKSALMON,
    DARKSEAGREEN,
    DARKSLATEBLUE,
    DARKSLATEGRAY,
    DARKSLATEGREY,
    DARKTURQUOISE,
    DARKVIOLET,
    DEEPPINK,
    DEEPSKYBLUE,
    DIMGRAY,
    DIMGREY,
    DODGERBLUE,
    FIREBRICK,
    FLORALWHITE,
    FORESTGREEN,
    FUCHSIA,
    GAINSBORO,
    GHOSTWHITE,
    GOLD,
    GOLDENROD,
    GRAY,
    GREEN,
    GREENYELLOW,
    GREY,
    GREY55,
    GREY75,
    HONEYDEW,
    HOTPINK,
    INDIANRED,
    INDIGO,
    IVORY,
    KHAKI, // Wikipedia says "a light shade of yellow-green" , but this one is more yellow, and very light
    LAVENDER,
    LAVENDERBLUSH,
    LAWNGREEN,
    LEMONCHIFFON,
    LIGHTBLUE,
    LIGHTCORAL,
    LIGHTCYAN,
    LIGHTGOLDENRODYELLOW,
    LIGHTGRAY,
    LIGHTGREEN,
    LIGHTGREY,
    LIGHTMEDIUMBLUE,
    LIGHTPINK,
    LIGHTSALMON,
    LIGHTSEAGREEN,
    LIGHTSKYBLUE, // A nice light blue
    LIGHTSLATEGRAY,
    LIGHTSLATEGREY,
    LIGHTSTEELBLUE,
    LIGHTYELLOW,
    LIME,
    LIMEGREEN,
    LINEN,
    MAGENTA,
    MAROON,
    MEDIUMAQUAMARINE,
    MEDIUMBLUE,
    MEDIUMORCHID,
    MEDIUMPURPLE,
    MEDIUMSEAGREEN,
    MEDIUMSLATEBLUE,
    MEDIUMSPRINGGREEN,
    MEDIUMTURQUOISE,
    MEDIUMVIOLETRED,
    MIDNIGHTBLUE,
    MINTCREAM,
    MISTYROSE,
    MOCCASIN,
    NAVAJOWHITE,
    NAVY,
    OLDLACE,
    OLIVE,
    OLIVEDRAB,
    ORANGE,
    ORANGERED,
    ORCHID,
    PALEGOLDENROD,
    PALEGREEN,
    PALETURQUOISE,
    PALEVIOLETRED,
    PAPAYAWHIP,
    PEACHPUFF,
    PERU,
    PINK,
    PLUM, // Much closer to pink than the colour of actual plums, and closer to its flower's colour
    POWDERBLUE,
    PURPLE,
    REBECCAPURPLE,
    RED,
    ROSYBROWN,
    ROYALBLUE,
    SADDLEBROWN,
    SALMON,
    SANDYBROWN,
    SEAGREEN,
    SEASHELL,
    SIENNA,
    SILVER,
    SKYBLUE,
    SLATEBLUE,
    SLATEGRAY,
    SLATEGREY,
    SNOW,
    SPRINGGREEN,
    STEELBLUE,
    TAN,
    TEAL,
    THISTLE, // A sort of desaturated purple, the colour of thistle flowers
    TOMATO,
    TURQUOISE,
    VIOLET,
    WHEAT,
    WHITESMOKE,
    YELLOW,
    YELLOWGREEN,

    NUM_COLOR
};


/**
 * Line types for setlinestyle(..)
 */
enum line_types {
    SOLID, DASHED
};
enum line_caps {
    BUTT = 0, ROUND
};

/**
 * To distinguish drawing to screen coordinates or world coordinates when using
 * a function such as drawline(world/screen coordates)
 */
typedef enum {
    GL_WORLD = 0,
    GL_SCREEN = 1
} t_coordinate_system;

/**
 * @brief Enum to distinguish drawing to screen or to offscreen buffer.
 */
typedef enum {
    ON_SCREEN = 0,
    OFF_SCREEN
} t_draw_to;

enum e_draw_mode {
    DRAW_NORMAL = 0,
    DRAW_XOR
};


/*
 * Used to pass information from event_loop when a mouse button is pressed
 * -- which button was pressed, and was shift and/or control being held
 * down when the button was pressed.
 */
typedef struct {
    bool shift_pressed; /* indicates whether a Shift key was pressed when a mouse button is pressed */
    bool ctrl_pressed; /* indicates whether a Ctrl key was pressed when a mouse button is pressed */
    unsigned int button; /* indicates what button is pressed: left click is 1; right click is 3; */
    /* scroll wheel click is 2; scroll wheel forward rotate is 4; */
    /* scroll wheel backward is 5. */
} t_event_buttonPressed;


#endif // EASYGL_CONSTANTS_H
