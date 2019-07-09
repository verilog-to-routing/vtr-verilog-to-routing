#ifndef MMAP_FILE_H_
#define MMAP_FILE_H_

#include <string>
#include "capnp/message.h"

class MmapFile {
  public:
    explicit MmapFile(const std::string& file);
    ~MmapFile();
    const kj::ArrayPtr<const ::capnp::word> getData() const;

  private:
    size_t size_;
    void* mapped_;
};

#endif /* MMAP_FILE_H_ */
