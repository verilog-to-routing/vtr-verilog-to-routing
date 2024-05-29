#ifndef TELEGRAMHEADER_H
#define TELEGRAMHEADER_H

#ifndef NO_SERVER

#include "bytearray.h"

#include <string>
#include <cstring>

namespace comm {

/**
 * @brief The fixed size bytes sequence where the metadata of a telegram message is stored.
 *
 * This structure is used to describe the message frame sequence in order to successfully extract it.
 * The TelegramHeader structure follows this format:
 * ------------------------------------------------------
 * [ 4 bytes ][  4 bytes  ][   4 bytes   ][    1 byte   ]
 * [SIGNATURE][DATA_LENGTH][DATA_CHECKSUM][COMPRESSOR_ID]
 * ------------------------------------------------------
 *
 * The SIGNATURE is a 4-byte constant sequence "I", "P", "A", "\0" which indicates the valid start of a TelegramHeader.
 * The DATA_LENGTH is a 4-byte field where the data length is stored, allowing for proper identification of the start and end of the TelegramFrame sequence.
 * The DATA_CHECKSUM is a 4-byte field where the data checksum is stored to validate the attached data.
 * The COMPRESSOR_ID is a 1-byte field where the compressor id is stored. If it's NULL, it means the data is not compressed (in text/json format).
 */
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

    static comm::TelegramHeader construct_from_data(const std::string_view& body, uint8_t compressor_id = 0);

    static constexpr size_t size() {
        return SIGNATURE_SIZE + LENGTH_SIZE + CHECKSUM_SIZE + COMPRESSORID_SIZE;
    }

    bool is_valid() const { return m_is_valid; }

    const ByteArray& buffer() const { return m_buffer; }

    uint32_t body_bytes_num() const { return m_body_bytes_num; }
    uint32_t body_check_sum() const { return m_body_check_sum; }
    uint8_t compressor_id() const { return m_compressor_id; }

    bool is_body_compressed() const { return m_compressor_id != 0; }

    std::string info() const;

private:
    bool m_is_valid = false;
    ByteArray m_buffer;

    uint32_t m_body_bytes_num = 0;
    uint32_t m_body_check_sum = 0;
    uint8_t m_compressor_id = 0;
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* TELEGRAMHEADER_H */
