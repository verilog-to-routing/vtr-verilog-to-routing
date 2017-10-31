#ifndef GRAPHICS_AUTOMARK_H
#define GRAPHICS_AUTOMARK_H

// Useful functions for automarking. Call before the student code could call event_loop().

// Call with new_setting == true to make the event_loop immediately return.
void set_disable_event_loop (bool new_setting);

// Call with new_setting == true to make event_loop immediately draw to postscript.
// Normally used when disable_event_loop is also set to force a return after the output.
void set_redirect_to_postscript (bool new_setting);

#endif /* GRAPHICS_AUTOMARK_H */

