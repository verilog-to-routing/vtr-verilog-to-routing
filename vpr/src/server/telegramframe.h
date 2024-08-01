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
*/
struct TelegramFrame {
    /**
    * @brief header The TelegramHeader containing metadata about the telegram message.
    */
    TelegramHeader header;

    /**
    * @brief body The actual data of the telegram message.
    */
    ByteArray body;
};
using TelegramFramePtr = std::shared_ptr<TelegramFrame>;

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMFRAME_H */
