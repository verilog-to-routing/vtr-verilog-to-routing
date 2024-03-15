#ifndef TELEGRAMHEADER_H
#define TELEGRAMHEADER_H

#include "bytearray.h"

#include <string>
#include <cstring>

namespace comm {

class TelegramHeader {
public:
    static constexpr const char SIGNATURE[] = "IPA";
    static constexpr size_t SIGNATURE_SIZE = sizeof(SIGNATURE);
    static constexpr size_t LENGTH_SIZE = sizeof(uint32_t);
    static constexpr size_t CHECKSUM_SIZE = LENGTH_SIZE;
    static constexpr size_t COMPRESSORID_SIZE = 1;

    static constexpr size_t LENGTH_OFFSET = SIGNATURE_SIZE;
    static constexpr size_t CHECKSUM_OFFSET = LENGTH_OFFSET + LENGTH_SIZE;
    static constexpr size_t COMPRESSORID_OFFSET = CHECKSUM_OFFSET + CHECKSUM_SIZE;

    TelegramHeader()=default;
    explicit TelegramHeader(uint32_t length, uint32_t checkSum, uint8_t compressorId = 0);
    explicit TelegramHeader(const ByteArray& body);
    ~TelegramHeader()=default;

    template<typename T>
    static comm::TelegramHeader constructFromData(const T& body, uint8_t compressorId = 0) {
        uint32_t bodyCheckSum = ByteArray::calcCheckSum(body);
        return comm::TelegramHeader{static_cast<uint32_t>(body.size()), bodyCheckSum, compressorId};
    }

    static constexpr size_t size() {
        return SIGNATURE_SIZE + LENGTH_SIZE + CHECKSUM_SIZE + COMPRESSORID_SIZE;
    }

    bool isValid() const { return m_isValid; }

    const ByteArray& buffer() const { return m_buffer; }

    uint32_t bodyBytesNum() const { return m_bodyBytesNum; }
    uint32_t bodyCheckSum() const { return m_bodyCheckSum; }
    uint8_t compressorId() const { return m_compressorId; }

    bool isBodyCompressed() const { return m_compressorId != 0; }

    std::string info() const;

private:
    bool m_isValid = false;
    ByteArray m_buffer;

    uint32_t m_bodyBytesNum = 0;
    uint32_t m_bodyCheckSum = 0;
    uint8_t m_compressorId = 0;
};

} // namespace comm

#endif // TELEGRAMHEADER_H
