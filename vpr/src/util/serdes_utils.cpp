#include "serdes_utils.h"

#include <fcntl.h>
#include <unistd.h>

#include "vpr_error.h"
#include "kj/filesystem.h"

void writeMessageToFile(const std::string& file, ::capnp::MessageBuilder* builder) {
    try {
        auto path = kj::Path::parse(file);
        auto fs = kj::newDiskFilesystem();
        const auto& dir = fs->getCurrent();
        auto f = dir.appendFile(path, kj::WriteMode::CREATE);
        capnp::writeMessage(*f, *builder);
    } catch (kj::Exception& e) {
        vpr_throw(VPR_ERROR_OTHER, e.getFile(), e.getLine(), e.getDescription().cStr());
    }
}
