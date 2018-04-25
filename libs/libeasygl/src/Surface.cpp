#include <iostream>
#include "Surface.h"
#include "SurfaceImpl.h"

Surface::Surface(const char* filePath)
    : impl_(new SurfaceImpl(filePath)) {
}

Surface::Surface()
    : impl_(nullptr) {
    //Empty but explicitly provided to ensure unique_ptr
    //works correctly with an incomplete type in the header
}

Surface::~Surface() {
    //Empty but explicitly provided to ensure unique_ptr
    //works correctly with an incomplete type in the header
}

Surface::Surface(const Surface& surface)
    : impl_(new SurfaceImpl(*surface.impl_)) {
}

void Surface::setSurface(const char* filePath) {
    impl_->setSurface(filePath);
}

Surface& Surface::operator=(Surface rhs) {
    //Copy-swap idiom
    swap(*this, rhs);
    return *this;
}

void swap(Surface& lhs, Surface& rhs) {
    std::swap(lhs.impl_, rhs.impl_);
}
