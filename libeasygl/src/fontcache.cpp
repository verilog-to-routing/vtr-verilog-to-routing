#include "fontcache.h"
#include "graphics_state.h"

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
 *
 * Windows:
 *   Something with similar effect will be done, but it is harder to tell what will happen.
 *   Check your installed fonts if the program exits immediately or while changing font size.
 *
 */
const char* const fontname_config[]{
    "helvetica",
    "lucida sans",
    "schumacher"
};

/******************************************
 * begin FontCache function definitions *
 ******************************************/

void FontCache::close_font(font_ptr font) {
#ifdef X11
    XftFontClose(t_x11_state::getInstance()->display, font);
#elif defined WIN32
    free(font);
#endif
}

/**
 * Loads the font with given attributes, and puts the pointer to in in put_font_ptr_here
 */
font_ptr FontCache::do_font_loading(int pointsize, int degrees) {

    bool success = false;
    font_ptr retval = NULL;

#ifdef X11
    for (int ifont = 0; ifont < NUM_FONT_TYPES; ifont++) {

#ifdef VERBOSE
        printf("Loading font: %s-%d\n", fontname_config[ifont], pointsize);
#endif

        XftMatrix font_matrix;
        XftMatrixInit(&font_matrix);
        if (degrees != 0) {
            XftMatrixRotate(&font_matrix, cos(PI * (degrees) / 180), sin(PI * degrees / 180));
        }

        /* Load font and get font information structure. */
        retval = XftFontOpen(
            t_x11_state::getInstance()->display, t_x11_state::getInstance()->screen_num,
            XFT_FAMILY, XftTypeString, fontname_config[ifont],
            XFT_SIZE, XftTypeDouble, static_cast<double>(pointsize),
            XFT_MATRIX, XftTypeMatrix, &font_matrix,
            NULL // (sentinel)
            );

        if (retval == NULL) {
#ifdef VERBOSE
            fprintf(stderr, "Cannot open font %s", fontname_config[ifont]);
            if (degrees != 0) {
                fprintf(stderr, "with rotation %f deg", degrees);
            }
            printf("\n");
#endif
        } else {
            success = true;
            break;
        }
    }
    if (success == false) {
        printf("Error in load_font: fontconfig couldn't find any font of pointsize %d.\n", pointsize);
        if (degrees != 0) {
            printf("and rotation %d deg", degrees);
        }
        printf("Use `fc-list` to list available fonts, `fc-match` to test, and then modify\n");
        printf("the font config array in easygl_constants.h .\n");
        exit(1);
    }
#elif defined WIN32
    LOGFONT *lf = retval = (LOGFONT*) my_malloc(sizeof (LOGFONT));
    ZeroMemory(lf, sizeof (LOGFONT));
    // lfHeight specifies the desired height of characters in logical units.
    // A positive value of lfHeight will request a font that is appropriate
    // for a line spacing of lfHeight. On the other hand, setting lfHeight
    // to a negative value will obtain a font height that is compatible with
    // the desired pointsize.
    lf->lfHeight = -pointsize;
    lf->lfWeight = FW_NORMAL;
    lf->lfCharSet = ANSI_CHARSET;
    lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf->lfQuality = PROOF_QUALITY;
    lf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    lf->lfEscapement = (LONG) degrees * 10;
    lf->lfOrientation = (LONG) degrees * 10;
    HFONT testfont;
    for (int ifont = 0; ifont < NUM_FONT_TYPES; ++ifont) {
        MultiByteToWideChar(CP_UTF8, 0, fontname_config[ifont], -1,
            lf->lfFaceName, sizeof (lf->lfFaceName) / sizeof (wchar_t));

        testfont = CreateFontIndirect(lf);
        if (testfont == NULL) {
#ifdef VERBOSE
            fprintf(stderr, "Couldn't open font %s in pointsize %d.\n",
                fontname_config[ifont], pointsize);
#endif
            continue;
        }

        if (DeleteObject(testfont) == 0) {
            WIN32_DELETE_ERROR();
        } else {
            success = true;
            break;
        }
    }
    if (success == false) {
        printf("Error in load_font: Windows couldn't find any font of pointsize %d.\n", pointsize);
        printf("check installed fonts, and then modify\n");
        printf("the font config array in easygl_constants.h .\n");
        exit(1);
    }
#endif
    return retval;
}

void FontCache::clear() {
    for (
        auto iter = descriptor2font_zeros.begin();
        iter != descriptor2font_zeros.end();
        ++iter
        ) {
        close_font(iter->second);
    }
    order_zeros.clear();
    descriptor2font_zeros.clear();

    for (
        auto iter = descriptor2font_rotated.begin();
        iter != descriptor2font_rotated.end();
        ++iter
        ) {
        close_font(iter->second);
    }
    order_rotated.clear();
    descriptor2font_rotated.clear();
}

font_ptr FontCache::get_font_info(size_t pointsize, int degrees) {
    if (degrees == 0) {
        return get_font_info(
            pointsize, degrees,
            order_zeros, descriptor2font_zeros, FONT_CACHE_SIZE_FOR_ZEROS
            );
    } else {
        return get_font_info(
            pointsize, degrees,
            order_rotated, descriptor2font_rotated, FONT_CACHE_SIZE_FOR_ROTATED
            );
    }
}

template<class queue_type, class map_type>
font_ptr FontCache::get_font_info(
    size_t pointsize, int degrees,
    queue_type& orderqueue, map_type& descr2font_map, size_t max_size) {

    auto search_result = descr2font_map.find(std::make_pair(pointsize, degrees));
    if (search_result == descr2font_map.end()) {

        if (orderqueue.size() + 1 > max_size) {
            // if too many fonts, remove the oldest font from the cache.
            font_descriptor fontdesc_to_remove = orderqueue.back();
            auto font_to_remove = descr2font_map.find(fontdesc_to_remove);

            close_font(font_to_remove->second);

            descr2font_map.erase(font_to_remove);
            orderqueue.pop_back();
            puts("font cache overflow");
        }

        font_ptr new_font = do_font_loading(pointsize, degrees);
        font_descriptor new_font_desc = std::make_pair(pointsize, degrees);
        orderqueue.push_front(new_font_desc);
        descr2font_map.insert(std::make_pair(new_font_desc, new_font));
        return new_font;
    } else {
        return search_result->second;
    }
}


