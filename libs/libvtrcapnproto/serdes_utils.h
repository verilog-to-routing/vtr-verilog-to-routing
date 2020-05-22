#ifndef SERDES_UTILS_H_
#define SERDES_UTILS_H_

#include <limits>
#include <string>
#include "capnp/serialize.h"

// Platform indepedent way to file message to a file on disk.
void writeMessageToFile(const std::string& file,
        ::capnp::MessageBuilder* builder);

inline ::capnp::ReaderOptions default_large_capnp_opts() {
    ::capnp::ReaderOptions opts = ::capnp::ReaderOptions();

    /* Remove traversal limit */
    opts.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
    return opts;
}

#endif /* SERDES_UTILS_H_ */
