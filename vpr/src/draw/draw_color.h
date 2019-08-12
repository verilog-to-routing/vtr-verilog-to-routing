#ifndef DRAW_COLOR_H
#define DRAW_COLOR_H

#ifndef NO_GRAPHICS

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

static constexpr ezgl::color blk_BISQUE(0xFF, 0xE4, 0xC4);
static constexpr ezgl::color blk_LIGHTGREY(0xD3, 0xD3, 0xD3);
static constexpr ezgl::color blk_LIGHTSKYBLUE(0x87, 0xCE, 0xFA);
static constexpr ezgl::color blk_THISTLE(0xD8, 0xBF, 0xD8);
static constexpr ezgl::color blk_KHAKI(0xF0, 0xE6, 0x8C);
static constexpr ezgl::color blk_CORAL(0xFF, 0x7F, 0x50);
static constexpr ezgl::color blk_TURQUOISE(0x40, 0xE0, 0xD0);
static constexpr ezgl::color blk_MEDIUMPURPLE(0x93, 0x70, 0xDB);
static constexpr ezgl::color blk_DARKSLATEBLUE(0x48, 0x3D, 0x8B);
static constexpr ezgl::color blk_DARKKHAKI(0xBD, 0xB7, 0x6B);
static constexpr ezgl::color blk_LIGHTMEDIUMBLUE(0x44, 0x44, 0xFF);
static constexpr ezgl::color blk_SADDLEBROWN(0x8B, 0x45, 0x13);
static constexpr ezgl::color blk_FIREBRICK(0xB2, 0x22, 0x22);
static constexpr ezgl::color blk_LIMEGREEN(0x32, 0xCD, 0x32);
static constexpr ezgl::color blk_PLUM(0xDD, 0xA0, 0xDD);
static constexpr ezgl::color blk_DARKGREEN(0x00, 0x64, 0x00);
static constexpr ezgl::color blk_PALEVIOLETRED(0xDB, 0x70, 0x93);
static constexpr ezgl::color blk_BLUE(0x00, 0x00, 0xFF);
static constexpr ezgl::color blk_FORESTGREEN(0x22, 0x8B, 0x22);
static constexpr ezgl::color blk_WHEAT(0xF5, 0xDE, 0xB3);
static constexpr ezgl::color blk_GOLD(0xFF, 0xD7, 0x00);
static constexpr ezgl::color blk_MOCCASIN(0xFF, 0xE4, 0xB5);
static constexpr ezgl::color blk_MEDIUMORCHID(0xBA, 0x55, 0xD3);
static constexpr ezgl::color blk_SKYBLUE(0x87, 0xCE, 0xEB);
static constexpr ezgl::color blk_WHITESMOKE(0xF5, 0xF5, 0xF5);
static constexpr ezgl::color blk_LIME(0x00, 0xFF, 0x00);
static constexpr ezgl::color blk_MEDIUMSLATEBLUE(0x7B, 0x68, 0xEE);
static constexpr ezgl::color blk_TOMATO(0xFF, 0x63, 0x47);
static constexpr ezgl::color blk_CYAN(0x00, 0xFF, 0xFF);
static constexpr ezgl::color blk_OLIVE(0x80, 0x80, 0x00);
static constexpr ezgl::color blk_LIGHTGRAY(0xD3, 0xD3, 0xD3);
static constexpr ezgl::color blk_STEELBLUE(0x46, 0x82, 0xB4);
static constexpr ezgl::color blk_LIGHTCORAL(0xF0, 0x80, 0x80);
static constexpr ezgl::color blk_IVORY(0xFF, 0xFF, 0xF0);
static constexpr ezgl::color blk_MEDIUMVIOLETRED(0xC7, 0x15, 0x85);
static constexpr ezgl::color blk_SNOW(0xFF, 0xFA, 0xFA);
static constexpr ezgl::color blk_DARKGRAY(0xA9, 0xA9, 0xA9);
static constexpr ezgl::color blk_GREY(0x80, 0x80, 0x80);
static constexpr ezgl::color blk_GRAY(0x80, 0x80, 0x80);
static constexpr ezgl::color blk_YELLOW(0xFF, 0xFF, 0x00);
static constexpr ezgl::color blk_REBECCAPURPLE(0x66, 0x33, 0x99);
static constexpr ezgl::color blk_DARKCYAN(0x00, 0x8B, 0x8B);
static constexpr ezgl::color blk_MIDNIGHTBLUE(0x19, 0x19, 0x70);
static constexpr ezgl::color blk_ROSYBROWN(0xBC, 0x8F, 0x8F);
static constexpr ezgl::color blk_CORNSILK(0xFF, 0xF8, 0xDC);
static constexpr ezgl::color blk_NAVAJOWHITE(0xFF, 0xDE, 0xAD);
static constexpr ezgl::color blk_BLANCHEDALMOND(0xFF, 0xEB, 0xCD);
static constexpr ezgl::color blk_ORCHID(0xDA, 0x70, 0xD6);
static constexpr ezgl::color blk_LIGHTGOLDENRODYELLOW(0xFA, 0xFA, 0xD2);
static constexpr ezgl::color blk_MAROON(0x80, 0x00, 0x00);
static constexpr ezgl::color blk_GREENYELLOW(0xAD, 0xFF, 0x2F);
static constexpr ezgl::color blk_SILVER(0xC0, 0xC0, 0xC0);
static constexpr ezgl::color blk_PALEGOLDENROD(0xEE, 0xE8, 0xAA);
static constexpr ezgl::color blk_LAWNGREEN(0x7C, 0xFC, 0x00);
static constexpr ezgl::color blk_DIMGREY(0x69, 0x69, 0x69);
static constexpr ezgl::color blk_DARKVIOLET(0x94, 0x00, 0xD3);
static constexpr ezgl::color blk_DARKTURQUOISE(0x00, 0xCE, 0xD1);
static constexpr ezgl::color blk_INDIGO(0x4B, 0x00, 0x82);
static constexpr ezgl::color blk_DARKORANGE(0xFF, 0x8C, 0x00);
static constexpr ezgl::color blk_PAPAYAWHIP(0xFF, 0xEF, 0xD5);
static constexpr ezgl::color blk_MINTCREAM(0xF5, 0xFF, 0xFA);
static constexpr ezgl::color blk_GREEN(0x00, 0x80, 0x00);
static constexpr ezgl::color blk_DARKMAGENTA(0x8B, 0x00, 0x8B);
static constexpr ezgl::color blk_MAGENTA(0xFF, 0x00, 0xFF);
static constexpr ezgl::color blk_LIGHTSLATEGRAY(0x77, 0x88, 0x99);
static constexpr ezgl::color blk_CHARTREUSE(0x7F, 0xFF, 0x00);
static constexpr ezgl::color blk_GHOSTWHITE(0xF8, 0xF8, 0xFF);
static constexpr ezgl::color blk_LIGHTCYAN(0xE0, 0xFF, 0xFF);
static constexpr ezgl::color blk_SIENNA(0xA0, 0x52, 0x2D);
static constexpr ezgl::color blk_GOLDENROD(0xDA, 0xA5, 0x20);
static constexpr ezgl::color blk_DARKSLATEGRAY(0x2F, 0x4F, 0x4F);
static constexpr ezgl::color blk_OLDLACE(0xFD, 0xF5, 0xE6);
static constexpr ezgl::color blk_SEASHELL(0xFF, 0xF5, 0xEE);
static constexpr ezgl::color blk_SPRINGGREEN(0x00, 0xFF, 0x7F);
static constexpr ezgl::color blk_MEDIUMTURQUOISE(0x48, 0xD1, 0xCC);
static constexpr ezgl::color blk_LEMONCHIFFON(0xFF, 0xFA, 0xCD);
static constexpr ezgl::color blk_MISTYROSE(0xFF, 0xE4, 0xE1);
static constexpr ezgl::color blk_OLIVEDRAB(0x6B, 0x8E, 0x23);
static constexpr ezgl::color blk_LIGHTBLUE(0xAD, 0xD8, 0xE6);
static constexpr ezgl::color blk_CHOCOLATE(0xD2, 0x69, 0x1E);
static constexpr ezgl::color blk_SEAGREEN(0x2E, 0x8B, 0x57);
static constexpr ezgl::color blk_DEEPPINK(0xFF, 0x14, 0x93);
static constexpr ezgl::color blk_LIGHTSEAGREEN(0x20, 0xB2, 0xAA);
static constexpr ezgl::color blk_FLORALWHITE(0xFF, 0xFA, 0xF0);
static constexpr ezgl::color blk_CADETBLUE(0x5F, 0x9E, 0xA0);
static constexpr ezgl::color blk_AZURE(0xF0, 0xFF, 0xFF);
static constexpr ezgl::color blk_BURLYWOOD(0xDE, 0xB8, 0x87);
static constexpr ezgl::color blk_AQUAMARINE(0x7F, 0xFF, 0xD4);
static constexpr ezgl::color blk_BROWN(0xA5, 0x2A, 0x2A);
static constexpr ezgl::color blk_POWDERBLUE(0xB0, 0xE0, 0xE6);
static constexpr ezgl::color blk_HOTPINK(0xFF, 0x69, 0xB4);
static constexpr ezgl::color blk_MEDIUMBLUE(0x00, 0x00, 0xCD);
static constexpr ezgl::color blk_BLUEVIOLET(0x8A, 0x2B, 0xE2);
static constexpr ezgl::color blk_GREY75(0xBF, 0xBF, 0xBF);
static constexpr ezgl::color blk_PURPLE(0x80, 0x00, 0x80);
static constexpr ezgl::color blk_TEAL(0x00, 0x80, 0x80);
static constexpr ezgl::color blk_ANTIQUEWHITE(0xFA, 0xEB, 0xD7);
static constexpr ezgl::color blk_DEEPSKYBLUE(0x00, 0xBF, 0xFF);
static constexpr ezgl::color blk_SLATEGRAY(0x70, 0x80, 0x90);
static constexpr ezgl::color blk_SALMON(0xFA, 0x80, 0x72);
static constexpr ezgl::color blk_SLATEBLUE(0x6A, 0x5A, 0xCD);
static constexpr ezgl::color blk_DARKORCHID(0x99, 0x32, 0xCC);
static constexpr ezgl::color blk_LIGHTPINK(0xFF, 0xB6, 0xC1);
static constexpr ezgl::color blk_DARKBLUE(0x00, 0x00, 0x8B);
static constexpr ezgl::color blk_PEACHPUFF(0xFF, 0xDA, 0xB9);
static constexpr ezgl::color blk_PALEGREEN(0x98, 0xFB, 0x98);
static constexpr ezgl::color blk_DARKSALMON(0xE9, 0x96, 0x7A);
static constexpr ezgl::color blk_DARKOLIVEGREEN(0x55, 0x6B, 0x2F);
static constexpr ezgl::color blk_DARKSEAGREEN(0x8F, 0xBC, 0x8F);
static constexpr ezgl::color blk_VIOLET(0xEE, 0x82, 0xEE);
static constexpr ezgl::color blk_RED(0xFF, 0x00, 0x00);
static constexpr ezgl::color blk_DARKSLATEGREY(0x2F, 0x4F, 0x4F);
static constexpr ezgl::color blk_PALETURQUOISE(0xAF, 0xEE, 0xEE);
static constexpr ezgl::color blk_DARKRED(0x8B, 0x00, 0x00);
static constexpr ezgl::color blk_SLATEGREY(0x70, 0x80, 0x90);
static constexpr ezgl::color blk_HONEYDEW(0xF0, 0xFF, 0xF0);
static constexpr ezgl::color blk_AQUA(0x00, 0xFF, 0xFF);
static constexpr ezgl::color blk_LIGHTSTEELBLUE(0xB0, 0xC4, 0xDE);
static constexpr ezgl::color blk_DODGERBLUE(0x1E, 0x90, 0xFF);
static constexpr ezgl::color blk_MEDIUMSPRINGGREEN(0x00, 0xFA, 0x9A);
static constexpr ezgl::color blk_NAVY(0x00, 0x00, 0x80);
static constexpr ezgl::color blk_GAINSBORO(0xDC, 0xDC, 0xDC);
static constexpr ezgl::color blk_LIGHTYELLOW(0xFF, 0xFF, 0xE0);
static constexpr ezgl::color blk_CRIMSON(0xDC, 0x14, 0x3C);
static constexpr ezgl::color blk_FUCHSIA(0xFF, 0x00, 0xFF);
static constexpr ezgl::color blk_DARKGOLDENROD(0xB8, 0x86, 0x0B);
static constexpr ezgl::color blk_SANDYBROWN(0xF4, 0xA4, 0x60);
static constexpr ezgl::color blk_BEIGE(0xF5, 0xF5, 0xDC);
static constexpr ezgl::color blk_LINEN(0xFA, 0xF0, 0xE6);
static constexpr ezgl::color blk_ORANGERED(0xFF, 0x45, 0x00);
static constexpr ezgl::color blk_ROYALBLUE(0x41, 0x69, 0xE1);
static constexpr ezgl::color blk_LAVENDER(0xE6, 0xE6, 0xFA);
static constexpr ezgl::color blk_TAN(0xD2, 0xB4, 0x8C);
static constexpr ezgl::color blk_YELLOWGREEN(0x9A, 0xCD, 0x32);
static constexpr ezgl::color blk_CORNFLOWERBLUE(0x64, 0x95, 0xED);
static constexpr ezgl::color blk_LAVENDERBLUSH(0xFF, 0xF0, 0xF5);
static constexpr ezgl::color blk_MEDIUMSEAGREEN(0x3C, 0xB3, 0x71);
static constexpr ezgl::color blk_PINK(0xFF, 0xC0, 0xCB);
static constexpr ezgl::color blk_GREY55(0x8C, 0x8C, 0x8C);
static constexpr ezgl::color blk_PERU(0xCD, 0x85, 0x3F);
static constexpr ezgl::color blk_LIGHTGREEN(0x90, 0xEE, 0x90);
static constexpr ezgl::color blk_LIGHTSALMON(0xFF, 0xA0, 0x7A);
static constexpr ezgl::color blk_INDIANRED(0xCD, 0x5C, 0x5C);
static constexpr ezgl::color blk_DIMGRAY(0x69, 0x69, 0x69);
static constexpr ezgl::color blk_LIGHTSLATEGREY(0x77, 0x88, 0x99);
static constexpr ezgl::color blk_MEDIUMAQUAMARINE(0x66, 0xCD, 0xAA);
static constexpr ezgl::color blk_DARKGREY(0xA9, 0xA9, 0xA9);
static constexpr ezgl::color blk_ORANGE(0xFF, 0xA5, 0x00);
static constexpr ezgl::color blk_ALICEBLUE(0xF0, 0xF8, 0xFF);

//The colours used to draw block types
const std::vector<ezgl::color> block_colors{
    //This first set of colours is somewhat curated to yield
    //a nice colour pallette
    blk_BISQUE, //EMPTY type is usually the type with index 0, so this colour
                //usually unused
    blk_LIGHTGREY,
    blk_LIGHTSKYBLUE,
    blk_THISTLE,
    blk_KHAKI,
    blk_CORAL,
    blk_TURQUOISE,
    blk_MEDIUMPURPLE,
    blk_DARKSLATEBLUE,
    blk_DARKKHAKI,
    blk_LIGHTMEDIUMBLUE,
    blk_SADDLEBROWN,
    blk_FIREBRICK,
    blk_LIMEGREEN,
    blk_PLUM,

    //However, if we have lots of block types we just assign arbitrary HTML
    //colours. Note that these are shuffled (instead of in alphabetical order)
    //since some colours which are alphabetically close are also close in
    //shade (making them difficult to distinguish)
    blk_DARKGREEN,
    blk_PALEVIOLETRED,
    blk_BLUE,
    blk_FORESTGREEN,
    blk_WHEAT,
    blk_GOLD,
    blk_MOCCASIN,
    blk_MEDIUMORCHID,
    blk_SKYBLUE,
    blk_WHITESMOKE,
    blk_LIME,
    blk_MEDIUMSLATEBLUE,
    blk_TOMATO,
    blk_CYAN,
    blk_OLIVE,
    blk_LIGHTGRAY,
    blk_STEELBLUE,
    blk_LIGHTCORAL,
    blk_IVORY,
    blk_MEDIUMVIOLETRED,
    blk_SNOW,
    blk_DARKGRAY,
    blk_GREY,
    blk_GRAY,
    blk_YELLOW,
    blk_REBECCAPURPLE,
    blk_DARKCYAN,
    blk_MIDNIGHTBLUE,
    blk_ROSYBROWN,
    blk_CORNSILK,
    blk_NAVAJOWHITE,
    blk_BLANCHEDALMOND,
    blk_ORCHID,
    blk_LIGHTGOLDENRODYELLOW,
    blk_MAROON,
    blk_GREENYELLOW,
    blk_SILVER,
    blk_PALEGOLDENROD,
    blk_LAWNGREEN,
    blk_DIMGREY,
    blk_DARKVIOLET,
    blk_DARKTURQUOISE,
    blk_INDIGO,
    blk_DARKORANGE,
    blk_PAPAYAWHIP,
    blk_MINTCREAM,
    blk_GREEN,
    blk_DARKMAGENTA,
    blk_MAGENTA,
    blk_LIGHTSLATEGRAY,
    blk_CHARTREUSE,
    blk_GHOSTWHITE,
    blk_LIGHTCYAN,
    blk_SIENNA,
    blk_GOLDENROD,
    blk_DARKSLATEGRAY,
    blk_OLDLACE,
    blk_SEASHELL,
    blk_SPRINGGREEN,
    blk_MEDIUMTURQUOISE,
    blk_LEMONCHIFFON,
    blk_MISTYROSE,
    blk_OLIVEDRAB,
    blk_LIGHTBLUE,
    blk_CHOCOLATE,
    blk_SEAGREEN,
    blk_DEEPPINK,
    blk_LIGHTSEAGREEN,
    blk_FLORALWHITE,
    blk_CADETBLUE,
    blk_AZURE,
    blk_BURLYWOOD,
    blk_AQUAMARINE,
    blk_BROWN,
    blk_POWDERBLUE,
    blk_HOTPINK,
    blk_MEDIUMBLUE,
    blk_BLUEVIOLET,
    blk_GREY75,
    blk_PURPLE,
    blk_TEAL,
    blk_ANTIQUEWHITE,
    blk_DEEPSKYBLUE,
    blk_SLATEGRAY,
    blk_SALMON,
    blk_SLATEBLUE,
    blk_DARKORCHID,
    blk_LIGHTPINK,
    blk_DARKBLUE,
    blk_PEACHPUFF,
    blk_PALEGREEN,
    blk_DARKSALMON,
    blk_DARKOLIVEGREEN,
    blk_DARKSEAGREEN,
    blk_VIOLET,
    blk_RED,
    blk_DARKSLATEGREY,
    blk_PALETURQUOISE,
    blk_DARKRED,
    blk_SLATEGREY,
    blk_HONEYDEW,
    blk_AQUA,
    blk_LIGHTSTEELBLUE,
    blk_DODGERBLUE,
    blk_MEDIUMSPRINGGREEN,
    blk_NAVY,
    blk_GAINSBORO,
    blk_LIGHTYELLOW,
    blk_CRIMSON,
    blk_FUCHSIA,
    blk_DARKGOLDENROD,
    blk_SANDYBROWN,
    blk_BEIGE,
    blk_LINEN,
    blk_ORANGERED,
    blk_ROYALBLUE,
    blk_LAVENDER,
    blk_TAN,
    blk_YELLOWGREEN,
    blk_CORNFLOWERBLUE,
    blk_LAVENDERBLUSH,
    blk_MEDIUMSEAGREEN,
    blk_PINK,
    blk_GREY55,
    blk_PERU,
    blk_LIGHTGREEN,
    blk_LIGHTSALMON,
    blk_INDIANRED,
    blk_DIMGRAY,
    blk_LIGHTSLATEGREY,
    blk_MEDIUMAQUAMARINE,
    blk_DARKGREY,
    blk_ORANGE,
    blk_ALICEBLUE,
};

#endif /* NO_GRAPHICS */

#endif /* DRAW_COLOR_H */
