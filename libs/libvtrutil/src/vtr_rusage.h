#ifndef VTR_RUSAGE_H
#define VTR_RUSAGE_H
#include <cstddef>

namespace vtr {

//Returns the maximum resident set size in bytes,
//or zero if unable to determine.
size_t get_max_rss();
} // namespace vtr

#endif
