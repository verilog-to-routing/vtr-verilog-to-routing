#ifndef TELEGRAMFRAME_H
#define TELEGRAMFRAME_H

#ifndef NO_SERVER

#include "telegramheader.h"
#include "bytearray.h"

#include <memory>

namespace comm {


/**
* @brief Structure representing a TelegramFrame.
*
* A TelegramFrame consists of a TelegramHeader followed by data.
*
* @var header The TelegramHeader containing metadata about the telegram message. @see TelegramHeader
* @var data The actual data of the telegram message.
*/
struct TelegramFrame {
    TelegramHeader header;
    ByteArray data;
};
using TelegramFramePtr = std::shared_ptr<TelegramFrame>;

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMFRAME_H */
