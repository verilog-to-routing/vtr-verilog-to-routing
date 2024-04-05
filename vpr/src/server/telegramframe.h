#ifndef TELEGRAMFRAME_H
#define TELEGRAMFRAME_H

#include "telegramheader.h"
#include "bytearray.h"

#include <memory>

namespace comm {

struct TelegramFrame {
    TelegramHeader header;
    ByteArray data;
};
using TelegramFramePtr = std::shared_ptr<TelegramFrame>;

} // namespace comm

#endif /* TELEGRAMFRAME_H */
