#include <iostream>
#include "Surface.h"
#include "SurfaceImpl.h"

Surface::Surface(const char* filePath)
    : impl_(new SurfaceImpl(filePath)) {
}

Surface::Surface(const Surface& surface)
    : impl_(new SurfaceImpl(*surface.impl_)) {
}

void Surface::setSurface(const char* filePath) {
    impl_->setSurface(filePath);
}
