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
    explicit TelegramBuffer(std::size_t sizeHint): m_rawBuffer(sizeHint) {}
    ~TelegramBuffer()=default;

    /**
     * @brief Check if internal byte buffer is empty.
     * 
     * @return true if the internal byte buffer is empty, false otherwise.
     */
    bool empty() { return m_rawBuffer.empty(); }

    /**
     * @brief Clear internal byte buffer.
     */
    void clear() { m_rawBuffer.clear(); }

    /**
     * @brief Append bytes to the internal byte buffer.
     * 
     * @param data The byte array whose contents will be appended to internal byte buffer.
     */
    void append(const ByteArray&);

    /**
     * @brief Extracts well-formed telegram frames from the internal byte buffer.
     * 
     * @param frames A reference to a vector where the extracted telegram frames will be stored.
     */
    void takeTelegramFrames(std::vector<TelegramFramePtr>&);

    /**
     * @brief Extracts well-formed telegram frames from the internal byte buffer.
     * 
     * @return std::vector<TelegramFramePtr> A vector containing pointers to the extracted telegram frames.
     */
    std::vector<TelegramFramePtr> takeTelegramFrames();

    void takeErrors(std::vector<std::string>&);

    /**
     * @brief Retrieves a constant reference to the internal byte buffer.
     * 
     * @return A constant reference to the internal byte buffer.
     */
    const ByteArray& data() const { return m_rawBuffer; }

private:
    ByteArray m_rawBuffer;
    std::vector<std::string> m_errors;
    std::optional<TelegramHeader> m_headerOpt;

    bool checkTelegramHeaderPresence();
};

} // namespace comm

#endif /* TELEGRAMBUFFER_H */
