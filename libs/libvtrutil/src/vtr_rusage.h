#pragma once

#include <cstddef>

namespace vtr {

///@brief Returns the maximum resident set size in bytes, or zero if unable to determine.
size_t get_max_rss();
} // namespace vtr
