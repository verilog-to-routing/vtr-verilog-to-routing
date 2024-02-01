#ifndef TELEGRAMBUFFER_H
#define TELEGRAMBUFFER_H

#include <vector>
#include <string>
#include <cstring>

namespace comm {

/** 
 * @brief ByteArray as a simple wrapper over std::vector<unsigned char>
*/
class ByteArray : public std::vector<unsigned char> {
public:
    static const std::size_t DEFAULT_SIZE_HINT = 1024;

    ByteArray(const char* data)
        : std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(data),
                                     reinterpret_cast<const unsigned char*>(data + std::strlen(data)))
    {}

    ByteArray(const char* data, std::size_t size)
        : std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(data),
                                     reinterpret_cast<const unsigned char*>(data + size))
    {}
    ByteArray(std::size_t sizeHint = DEFAULT_SIZE_HINT) {
        this->reserve(sizeHint);
    }

    void append(const ByteArray& appendix) {
        for (unsigned char b: appendix) {
            this->push_back(b);
        }
    }

    void append(unsigned char b) {
        this->push_back(b);
    }

    std::string to_string() const {
        return std::string(reinterpret_cast<const char*>(this->data()), this->size());
    }
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

} // namespace comm 

#endif // TELEGRAMBUFFER_H
