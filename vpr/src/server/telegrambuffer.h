#ifndef TELEGRAMBUFFER_H
#define TELEGRAMBUFFER_H

#include "bytearray.h"
#include "telegramframe.h"

#include <vector>
#include <string>
#include <cstring>
#include <optional>

namespace comm {

/** 
 * @brief Implements Telegram Buffer as a wrapper over BytesArray
 * 
 * It aggregates received bytes and return only well filled frames.
*/
class TelegramBuffer
{
    static const std::size_t DEFAULT_SIZE_HINT = 1024;

public:
    TelegramBuffer(): m_rawBuffer(DEFAULT_SIZE_HINT) {}
    TelegramBuffer(std::size_t sizeHint): m_rawBuffer(sizeHint) {}
    ~TelegramBuffer()=default;

    bool empty() { return m_rawBuffer.empty(); }

    void clear() { m_rawBuffer.clear(); }

    void append(const ByteArray&);
    void takeTelegramFrames(std::vector<TelegramFramePtr>&);
    std::vector<TelegramFramePtr> takeTelegramFrames();
    void takeErrors(std::vector<std::string>&);

    const ByteArray& data() const { return m_rawBuffer; }

private:
    ByteArray m_rawBuffer;
    std::vector<std::string> m_errors;
    std::optional<TelegramHeader> m_headerOpt;

    bool checkRawBuffer();
};

} // namespace comm

#endif // TELEGRAMBUFFER_H
