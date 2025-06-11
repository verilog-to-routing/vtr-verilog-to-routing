#pragma once

#include "capnp/message.h"
#include <cstdint>
#include <limits>
#include <string>

// Platform indepedent way to file message to a file on disk.
void writeMessageToFile(const std::string& file,
        ::capnp::MessageBuilder* builder);

inline ::capnp::ReaderOptions default_large_capnp_opts() {
    ::capnp::ReaderOptions opts = ::capnp::ReaderOptions();

    /* Remove traversal limit */
    opts.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
    return opts;
}
