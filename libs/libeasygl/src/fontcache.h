#ifndef FONTCACHE_H
#define FONTCACHE_H

/**********************************
 * Common Preprocessor Directives *
 **********************************/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <unordered_map>
#include <sys/timeb.h>
#include <math.h>

// Set X11 by default, if neither NO_GRAPHICS NOR WIN32 are defined
#ifndef NO_GRAPHICS
#ifndef WIN32
#ifndef X11
#define X11
#endif
#endif // !WIN32
#endif // ~NO_GRAPHICS

#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <windowsx.h>
#endif

/**
 * Maximum sizes for the font cache. The "ZEROS" one is for fonts of zero rotation, and
 * the "ROTATED" one is for fonts with other rotation. If you plan on rotating text to
 * many rotations, say random ones, and displaying them all at the same time, I suggest
 * something large for "ROTATED". The value of "ZEROS" is probably fine.
 *
 * What these values really mean to you is: because the text drawing is really lazy about
 * loading fonts, the "ZEROS" should be at least how many font sizes you expect to be visible
 * at a given time, and the "ROTATED" is for how many other different angle-fontsize combinations
 * you expect to be visible at a given time. If you program remains within these limits, there
 * will be no slow frames, except the first one where you change many visible combinations.
 * For after this frame, the font cache has filled up with the fonts that are visible
 *
 * A quick experiment gave these results:
 *    10 fonts =>     540 KiB
 *   100 fonts =>   1.7   MiB
 *  1000 fonts =>  13.5   Mib
 * 10000 fonts => 132.4   Mib
 */
#define FONT_CACHE_SIZE_FOR_ZEROS 40
#define FONT_CACHE_SIZE_FOR_ROTATED 1000
#define NUM_FONT_TYPES 3
#define PI 3.141592654

using namespace std;

#ifdef WIN32
typedef LOGFONT* font_ptr;
#elif defined X11
typedef XftFont* font_ptr;
#else
typedef void* font_ptr;
#endif

/**
 * This is a class that caches fonts. It separates out unrotated and rotated fonts,
 * for the reason that the unrotated ones are used for bounds estimation anyway, as
 * well as that they are generally convenient to have around. For each group, it uses
 * a queue to keep track of the order of creation, and when a new font would put a
 * cache over capacity, it deletes the oldest one first. The maps are to allow fast
 * lookup of fonts from their size and rotation.
 *
 * This cache is also not very complicated, and it could be, if there were a good reason.
 *
 * Also note that no references retrieved from this cache should be retained, as
 * another part of the program may cause the creation of another font(s) that will
 * push yours out. Instead a font should be retrieved every time it is needed.
 *
 * We're currently using only integer font rotations (in degrees), to avoid having
 * a tremendous number of possible font rotations.
 */
class FontCache {
public:

    FontCache()
    : order_zeros()
    , descriptor2font_zeros()
    , order_rotated()
    , descriptor2font_rotated() {
    }

    /**
     * Retrieve a font from this cache, or create it if it doesn't already exist,
     * pushing out, and deleting a font if the new font wont fit in its cache.
     * Gets a font from one of two caches depending on whether or not it is rotated.
     */
    font_ptr get_font_info(size_t pointsize, int degrees);

    /**
     * Clear out this cache so that it contains no fonts. This deletes all fonts,
     * as well, so make sure any fonts that were retrieved before cannot be referenced.
     */
    void clear();

private:
    typedef std::pair<size_t, int> font_descriptor;

    struct fontdesc_hasher {

        inline std::size_t operator()(const font_descriptor& v) const {
            std::hash<size_t> sizet_hasher;
            std::hash<int> int_hasher;
            return sizet_hasher(v.first) ^ int_hasher(v.second);
        }
    };
    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;

    static void close_font(font_ptr font);
    static font_ptr do_font_loading(int pointsize, int degrees);

    // this function actually does all the work of get_font_info, but only
    // looks/creates in the given map and queue.
    template<class queue_type, class map_type>
    static font_ptr get_font_info(
            size_t pointsize, int degrees,
            queue_type& orderqueue, map_type& descr2font_map, size_t max_size);

    typedef std::unordered_map<font_descriptor, font_ptr, fontdesc_hasher> font_lookup;
    typedef std::deque<font_descriptor> font_queue;

    // unrotated fonts (zero rotation)
    font_queue order_zeros;
    font_lookup descriptor2font_zeros;
    // rotated fonts
    font_queue order_rotated;
    font_lookup descriptor2font_rotated;
};

#endif // FONTCACHE_H
