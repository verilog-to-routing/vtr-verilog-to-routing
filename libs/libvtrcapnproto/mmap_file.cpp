#include "mmap_file.h"
#include "vtr_error.h"
#include "vtr_util.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32 // Windows
// Windows does not provide POSIX mmap() or unistd.h.
// Use Windows-specific headers instead. Note that actual file mapping is
// handled via the KJ filesystem abstraction.
#include <io.h>
#else
// POSIX systems provide mmap() and unistd.h for file access and mapping.
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "kj/filesystem.h"

MmapFile::MmapFile(const std::string& file)
    : size_(0) {
    try {
        auto fs = kj::newDiskFilesystem();
        auto path = fs->getCurrentPath().evalNative(file);

        const auto& dir = fs->getRoot();
        auto stat = dir.lstat(path);
        auto f = dir.openFile(path);
        size_ = stat.size;
        data_ = f->mmap(0, stat.size);
    } catch (kj::Exception& e) {
        throw vtr::VtrError(e.getDescription().cStr(), e.getFile(), e.getLine());
    }
}

const kj::ArrayPtr<const ::capnp::word> MmapFile::getData() const {
    if ((size_ % sizeof(::capnp::word)) != 0) {
        throw vtr::VtrError(
            vtr::string_fmt("size_ %d is not a multiple of capnp::word", size_),
            __FILE__, __LINE__);
    }

    return kj::arrayPtr(reinterpret_cast<const ::capnp::word*>(data_.begin()),
                        size_ / sizeof(::capnp::word));
}
