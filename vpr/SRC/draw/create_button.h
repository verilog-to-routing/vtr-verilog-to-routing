/* Note: everything in this file used to be part of graphics.c, 
   but was moved here because the template function create_button 
   needs to be defined in its own header file. */

#define BUTTON_TEXT_LEN	100
#include <string.h>
#include <functional>
#include <vector>

#ifdef X11

/*********************************************
* X-Windows Specific Preprocessor Directives *
*********************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

/* Uncomment the line below if your X11 header files don't define XPointer */
/* typedef char *XPointer;                                                 */

#endif

#ifdef WIN32
#ifndef UNICODE
#define UNICODE // force windows api into unicode (usually UTF-16) mode.
#endif
#include <windows.h>
#include <WindowsX.h>
#endif /* X11 Preprocessor Directives */

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

struct t_button {
    int width;
    int height;
    int xleft;
    int ytop;
    std::function<void(void(*drawscreen)(void))> fcn;
#ifdef X11
    Window win;
    XftDraw* draw;
#endif
#ifdef WIN32
    HWND hwnd;
#endif
    t_button_type type;
    char text[BUTTON_TEXT_LEN];
    int poly[3][2];
    bool ispressed;
    bool enabled;
    bool isproceed; // is this button's function "proceed"?
};

/* Structure used to store all the buttons created in the menu region. */
extern std::vector<t_button> buttons;

void proceed(void(*drawscreen) (void));

void map_button(int bnum);
void unmap_button(int bnum);

void *safe_malloc(int ibytes);
void *safe_calloc(int ibytes);

/* Look for a button on the menu bar called prev_button_text (UTF-8),
   then insert a new button one slot below with text button_text,
   which calls the callback function button_func when pressed.
   button_func takes a void function drawscreen as its first argument, 
   which performs screen redrawing.  button_func may optionally take
   additional arguments, which are passed to create_button through the
   variadic argument "Args&... args".  See draw.c for examples.
   */
template<class... Args>
void create_button(const char *prev_button_text, const char *button_text,
    void(*button_func)(void(*drawscreen)(void), Args&...), Args&... args)
{
    int i, bnum, space, bheight;
    t_button_type button_type = BUTTON_TEXT;

    space = 8;

    /* Only allow new buttons that are text or separator (not poly) types.
    * They can also only go after buttons that are text buttons.
    */

    bnum = -1;

    for (i = 0; i < int(buttons.size()); i++) {
        if (buttons[i].type == BUTTON_TEXT &&
            strcmp(buttons[i].text, prev_button_text) == 0) {
            bnum = i + 1;
            break;
        }
    }

    if (bnum == -1) {
        printf("Error in create_button:  button with text %s not found.\n",
            prev_button_text);
        exit(1);
    }

    buttons.resize(buttons.size() + 1);

    /* NB:  Requirement that you specify the button that this button goes under *
    * guarantees that button[num_buttons-2] exists and is a text button.       */

    /* Special string to make a separator. */
    if (!strncmp(button_text, "---", 3)) {
        bheight = 2;
        button_type = BUTTON_SEPARATOR;
    }
    else
        bheight = 26;

    for (i = buttons.size() - 1; i>bnum; i--) {
        buttons[i].xleft = buttons[i - 1].xleft;
        buttons[i].ytop = buttons[i - 1].ytop + bheight + space;
        buttons[i].height = buttons[i - 1].height;
        buttons[i].width = buttons[i - 1].width;
        buttons[i].type = buttons[i - 1].type;
        strcpy(buttons[i].text, buttons[i - 1].text);
        buttons[i].fcn = buttons[i - 1].fcn;
        buttons[i].ispressed = buttons[i - 1].ispressed;
        buttons[i].enabled = buttons[i - 1].enabled;
        buttons[i].isproceed = buttons[i - 1].isproceed;
        unmap_button(i - 1);
    }

    i = bnum;
    buttons[i].xleft = 6;
    buttons[i].ytop = buttons[i - 1].ytop + buttons[i - 1].height
        + space;
    buttons[i].height = bheight;
    buttons[i].width = 90;
    buttons[i].type = button_type;
    strncpy(buttons[i].text, button_text, BUTTON_TEXT_LEN);
    // lambdas with variadic arguments do't work on GCC versions older than ~2013, but std::bind does
    // buttons[i].fcn = [&](void(*drawscreen_ptr)(void)){button_func(drawscreen_ptr, args...); };
    buttons[i].fcn = std::bind(button_func, std::placeholders::_1, args...);
    buttons[i].isproceed = reinterpret_cast<void(*)(void(*drawscreen_ptr)(void))>(button_func) == proceed;
    buttons[i].ispressed = false;
    buttons[i].enabled = true;

    for (; i < int(buttons.size()); i++)
        map_button(i);
}