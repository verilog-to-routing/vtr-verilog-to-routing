#include "hsl.h"

float hue2rgb(float v1, float v2, float vH);

hsl color2hsl(t_color color) {
    float r = color.red / 255.;
    float g = color.green / 255.;
    float b = color.blue / 255.;

    float xmin = std::min(std::min(r, g), b); //Min. value of RGB
    float xmax = std::max(std::max(r, g), b); //Max. value of RGB
    float range = xmax - xmin;

    float H = 0.;
    float S = 0.;
    float L = 0.;
    ;

    L = (xmax + xmin) / 2;

    if (range == 0.) { //Grey
        H = 0.;
        S = 0.;
    } else { //Colour
        if (L < 0.5)
            S = range / (xmax + xmin);
        else
            S = range / (2 - xmax - xmin);

        float delta_r = (((xmax - r) / 6) + (range / 2)) / range;
        float delta_g = (((xmax - g) / 6) + (range / 2)) / range;
        float delta_b = (((xmax - b) / 6) + (range / 2)) / range;

        if (r == xmax)
            H = delta_b - delta_g;
        else if (g == xmax)
            H = (1. / 3.) + delta_r - delta_b;
        else if (b == xmax)
            H = (2. / 3.) + delta_g - delta_r;

        if (H < 0) H += 1;
        if (H > 1) H -= 1;
    }

    hsl val;
    val.h = H;
    val.s = S;
    val.l = L;

    return val;
}

t_color hsl2color(hsl in) {
    float H = in.h;
    float S = in.s;
    float L = in.l;

    float R, G, B;
    if (S == 0) {
        R = L * 255;
        G = L * 255;
        B = L * 255;
    } else {
        float var_2;
        if (L < 0.5)
            var_2 = L * (1 + S);
        else
            var_2 = (L + S) - (S * L);

        float var_1 = 2 * L - var_2;

        R = 255 * hue2rgb(var_1, var_2, H + (1. / 3.));
        G = 255 * hue2rgb(var_1, var_2, H);
        B = 255 * hue2rgb(var_1, var_2, H - (1. / 3.));
    }

    return t_color(R, G, B);
}

float hue2rgb(float v1, float v2, float vH) {
    if (vH < 0) vH += 1.;
    if (vH > 1) vH -= 1.;
    if ((6 * vH) < 1) return v1 + (v2 - v1) * 6 * vH;
    if ((2 * vH) < 1) return v2;
    if ((3 * vH) < 2) return v1 + (v2 - v1) * ((2. / 3.) - vH) * 6;
    return (v1);
}
