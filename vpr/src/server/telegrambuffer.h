#ifndef TELEGRAMBUFFER_H
#define TELEGRAMBUFFER_H

#include <vector>
#include <string>
#include <cstring>

namespace server {

/** 
 * @brief Implements dynamic bytes array with simple interface.
*/
class ByteArray {
public:
    static const std::size_t DEFAULT_SIZE_HINT = 1024;

    ByteArray(const char* data)
        : m_data(reinterpret_cast<const unsigned char*>(data),
                 reinterpret_cast<const unsigned char*>(data + strlen(data)))
    {}
    ByteArray(std::size_t sizeHint = DEFAULT_SIZE_HINT) {
        m_data.reserve(sizeHint);
    }

    void append(const ByteArray& appendix) {
        for (unsigned char b: appendix.data()) {
            m_data.push_back(b);
        }
    }

    void append(unsigned char b) {
        m_data.push_back(b);
    }

    std::string to_string() const {
        return std::string(reinterpret_cast<const char*>(m_data.data()), m_data.size());
    }

    bool empty() const { return m_data.empty(); }
    const std::vector<unsigned char>& data() const { return m_data; }
    void clear() { m_data.clear(); }

private:
    std::vector<unsigned char> m_data;
};

/** 
 * @brief Implements Telegram Buffer as a wrapper over BytesArray
 * 
 * It aggregates received bytes and return only well filled frames, separated by telegram delimerer byte.
*/
class TelegramBuffer
{
    static const std::size_t DEFAULT_SIZE_HINT = 1024;
public:
    TelegramBuffer(std::size_t sizeHint = DEFAULT_SIZE_HINT): m_rawBuffer(sizeHint) {}
    ~TelegramBuffer()=default;

    void clear() { m_rawBuffer.clear(); }

    void append(const ByteArray&);
    std::vector<ByteArray> takeFrames();

    const ByteArray& data() const { return m_rawBuffer; }

private:
    ByteArray m_rawBuffer;
};

} // namespace server 

#endif // TELEGRAMBUFFER_H
