#ifndef HSL_H
#define HSL_H

#include "graphics_types.h"

struct hsl {
    double h; // a fraction between 0 and 1
    double s; // a fraction between 0 and 1
    double l; // a fraction between 0 and 1
};

hsl color2hsl(t_color in);
t_color hsl2color(hsl in);

#endif
