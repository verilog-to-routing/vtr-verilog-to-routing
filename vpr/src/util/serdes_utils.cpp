#include "serdes_utils.h"

#include <fcntl.h>
#include <unistd.h>

#include "vpr_error.h"

void writeMessageToFile(const std::string& file, ::capnp::MessageBuilder* builder) {
    int fd = open(file.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        VPR_THROW(VPR_ERROR_ROUTE, "open %s failed: %s", file.c_str(), strerror(errno));
    }

    capnp::writeMessageToFd(fd, *builder);
    if (close(fd) < 0) {
        VPR_THROW(VPR_ERROR_ROUTE, "close %s failed: %s", file.c_str(), strerror(errno));
    }
}
