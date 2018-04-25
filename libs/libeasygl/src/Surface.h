#ifndef SURFACE_H
#define SURFACE_H

#include <memory>

class SurfaceImpl;

// This class implements a Cairo surface (buffer of pixel data for graphics)
// which can be initialized from a png file, and then the surface (bitmap)
// can be drawn anywhere on the screen and in many places on the screen
// if desired. Surfaces are created by the constructor from a filePath.
// The std::shared_ptr reference counts how many copies of a surface have been
// made through assignment and copy construction, and deallocates the cairo
// data representing the surface only when the last copy is destroyed.

class Surface {
    public:
        Surface();
        Surface(const char* filePath);
        ~Surface();
        Surface& operator=(Surface rhs); // assignment operator
        Surface(const Surface& surface);

        void setSurface(const char* filePath);

    private:
        //Requires access to the underlying cairo surface object
        friend void draw_surface(const Surface& surface, float x, float y);

        friend void swap(Surface& lhs, Surface& rhs);


        //We use the PIMPL idiom to avoid the main graphics.h
        //interface becoming dependant on cario headers, which
        //may not be availabe if graphics is disabled
        std::unique_ptr<SurfaceImpl> impl_;
};

#endif
