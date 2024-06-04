#ifndef TELEGRAMBUFFER_H
#define TELEGRAMBUFFER_H

#ifndef NO_SERVER

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
    TelegramBuffer(): m_raw_buffer(DEFAULT_SIZE_HINT) {}
    explicit TelegramBuffer(std::size_t sizeHint): m_raw_buffer(sizeHint) {}
    ~TelegramBuffer()=default;

    /**
     * @brief Check if internal byte buffer is empty.
     * 
     * @return true if the internal byte buffer is empty, false otherwise.
     */
    bool empty() { return m_raw_buffer.empty(); }

    /**
     * @brief Clear internal byte buffer.
     */
    void clear() { m_raw_buffer.clear(); }

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
    void take_telegram_frames(std::vector<TelegramFramePtr>&);

    /**
     * @brief Extracts well-formed telegram frames from the internal byte buffer.
     * 
     * @return std::vector<TelegramFramePtr> A vector containing pointers to the extracted telegram frames.
     */
    std::vector<TelegramFramePtr> take_telegram_frames();

    /**
     * @brief Takes errors from the internal storage.
     *
     * This function retrieves errors stored internally and moves them into the provided vector.
     * After calling this function, the internal error storage will be cleared.
     *
     * @param errors A vector to which the errors will be moved.
     *
     * @note After calling this function, the internal error storage will be cleared.
     */
    void take_errors(std::vector<std::string>&);

    /**
     * @brief Retrieves a constant reference to the internal byte buffer.
     * 
     * @return A constant reference to the internal byte buffer.
     */
    const ByteArray& data() const { return m_raw_buffer; }

private:
    ByteArray m_raw_buffer;
    std::vector<std::string> m_errors;
    std::optional<TelegramHeader> m_header_opt;

    bool check_telegram_header_presence();
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMBUFFER_H */
