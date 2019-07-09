#include "mmap_file.h"
#include "vpr_error.h"
#include "vtr_log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

MmapFile::MmapFile(const std::string& file)
    : size_(0)
    , mapped_(nullptr) {
    int fd = open(file.c_str(), O_RDONLY);
    if (fd < 0) {
        VPR_THROW(VPR_ERROR_OTHER, "open %s failed: %s", file.c_str(), strerror(errno));
    }

    struct stat s;
    if (fstat(fd, &s) < 0) {
        auto errno_copy = errno;
        if (close(fd) < 0) {
            VTR_LOG_WARN("close %s failed: %s", file.c_str(), strerror(errno));
        }
        VPR_THROW(VPR_ERROR_OTHER, "stat %s failed: %s", file.c_str(), strerror(errno_copy));
    }

    size_ = s.st_size;
    mapped_ = mmap(
        0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (close(fd) < 0) {
        VTR_LOG_WARN("close %s failed: %s", file.c_str(), strerror(errno));
    }
    if (mapped_ == MAP_FAILED) {
        mapped_ = nullptr;
        VPR_THROW(VPR_ERROR_OTHER, "mmap %s failed: %s", file.c_str(), strerror(errno));
    }
}

const kj::ArrayPtr<const ::capnp::word> MmapFile::getData() const {
    if ((size_ % sizeof(::capnp::word)) != 0) {
        VPR_THROW(VPR_ERROR_OTHER, "size_ %d is not a multiple of capnp::word", size_);
    }

    return kj::arrayPtr(static_cast<const ::capnp::word*>(mapped_),
                        size_ / sizeof(::capnp::word));
}

MmapFile::~MmapFile() {
    if (munmap(mapped_, size_) < 0) {
        VTR_LOG_WARN("Failed to unmap: %s\n", strerror(errno));
    }
}
