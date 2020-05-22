#include "serdes_utils.h"

#include <fcntl.h>
#include <unistd.h>

#include "vtr_error.h"
#include "kj/filesystem.h"

void writeMessageToFile(const std::string& file, ::capnp::MessageBuilder* builder) {
    try {
        auto fs = kj::newDiskFilesystem();
        auto path = fs->getCurrentPath().evalNative(file);

        const auto& dir = fs->getRoot();
        auto f = dir.openFile(path, kj::WriteMode::CREATE | kj::WriteMode::MODIFY);
        f->truncate(0);
        auto f_app = kj::newFileAppender(std::move(f));
        capnp::writeMessage(*f_app, *builder);
    } catch (kj::Exception& e) {
        throw vtr::VtrError(e.getDescription().cStr(), e.getFile(), e.getLine());
    }
}
