#ifndef SERDES_UTILS_H_
#define SERDES_UTILS_H_

#include <string>
#include "capnp/serialize.h"

// Platform indepedent way to file message to a file on disk.
void writeMessageToFile(const std::string& file,
        ::capnp::MessageBuilder* builder);

inline ::capnp::ReaderOptions default_large_capnp_opts() {
    ::capnp::ReaderOptions opts = ::capnp::ReaderOptions();

    /* Increase reader limit to 1G words to allow for large files. */
    opts.traversalLimitInWords = 1024 * 1024 * 1024;
    return opts;
}

#endif /* SERDES_UTILS_H_ */
