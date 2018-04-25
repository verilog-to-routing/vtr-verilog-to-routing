#ifndef EASYGL_SURFACE_IMPL_H
#define EASYGL_SURFACE_IMPL_H

#include <memory>

#if !defined(NO_GRAPHICS) && !defined(WIN32)
# define USE_CAIRO_SURFACE
#endif

#ifdef USE_CAIRO_SURFACE
# include <cairo.h>
# include <cairo-xlib.h>
#else
//Graphics disabled, may not have access to cairo headers so define a dummy type
typedef void cairo_surface_t;
#endif

class SurfaceImpl {
    public:
        SurfaceImpl() = default;
        SurfaceImpl(const char* filePath);
        ~SurfaceImpl() = default;
        SurfaceImpl& operator=(const SurfaceImpl& rhs) = default; // assignment operator
        SurfaceImpl(const SurfaceImpl& surface) = default; // cctor

        void setSurface(const char* filePath);
        cairo_surface_t* getSurface() const;

    private:
        std::shared_ptr<cairo_surface_t> mSurface;
};

#endif
