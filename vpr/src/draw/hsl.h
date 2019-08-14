#ifndef HSL_H
#define HSL_H

#ifndef NO_GRAPHICS

#    include "ezgl/color.hpp"

struct hsl {
    double h; // hue            a fraction between 0 and 1
    double s; // saturation     a fraction between 0 and 1
    double l; // luminesence    a fraction between 0 and 1
};

/* conversions between color (red, green, and blue) and hsl (hue, saturation, and luminesence) */
hsl color2hsl(ezgl::color in);
ezgl::color hsl2color(hsl in);

#endif /* NO_GRAPHICS */

#endif
