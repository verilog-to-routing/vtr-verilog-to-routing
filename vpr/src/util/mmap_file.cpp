#include "mmap_file.h"
#include "vpr_error.h"
#include "vtr_log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "kj/filesystem.h"

MmapFile::MmapFile(const std::string& file)
    : size_(0) {
    try {
        auto fs = kj::newDiskFilesystem();
        const auto &dir = fs->getCurrent();
        auto path = kj::Path::parse(file);
        auto stat = dir.lstat(path);
        auto f = dir.openFile(path);
        size_ = stat.size;
        data_ = f->mmap(0, stat.size);
    } catch (kj::Exception & e) {
        vpr_throw(VPR_ERROR_OTHER, e.getFile(), e.getLine(), e.getDescription().cStr());
    }
}

const kj::ArrayPtr<const ::capnp::word> MmapFile::getData() const {
    if ((size_ % sizeof(::capnp::word)) != 0) {
        VPR_THROW(VPR_ERROR_OTHER, "size_ %d is not a multiple of capnp::word", size_);
    }

    return kj::arrayPtr(reinterpret_cast<const ::capnp::word*>(data_.begin()),
                        size_ / sizeof(::capnp::word));
}
