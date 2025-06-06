#pragma once

#include "capnp/common.h"
#include <string>
#include "kj/array.h"

// Platform independent mmap, useful for reading large capnp's.
class MmapFile {
  public:
    explicit MmapFile(const std::string& file);
    const kj::ArrayPtr<const ::capnp::word> getData() const;

  private:
    size_t size_;
    kj::Array<const kj::byte> data_;
};
