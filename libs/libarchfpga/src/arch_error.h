#ifndef ARCH_ERROR_H
#define ARCH_ERROR_H

#include "vtr_error.h"
#include <cstdarg>

//Note that we mark this function with the C++11 attribute 'noreturn'
//as it will throw exceptions and not return normally. This can help
//reduce false-positive compiler warnings.
[[noreturn]] void archfpga_throw(const char* filename, int line, const char* fmt, ...);

class ArchFpgaError : public vtr::VtrError {
  public:
    ArchFpgaError(std::string msg = "", std::string new_filename = "", size_t new_linenumber = -1)
        : vtr::VtrError(msg, new_filename, new_linenumber) {}
};

#endif
