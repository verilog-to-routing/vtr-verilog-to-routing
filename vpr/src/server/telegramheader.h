#ifndef TELEGRAMHEADER_H
#define TELEGRAMHEADER_H

#ifndef NO_SERVER

#include "bytearray.h"

#include <string>
#include <cstring>
#include <cstdint>

namespace comm {

/**
 * @brief The fixed size byte sequence where the metadata of a telegram message is stored.
 *
 * This structure is used to describe the message frame sequence in order to successfully extract it.
 * The TelegramHeader structure follows this format:
 * - [ 4 bytes ]: SIGNATURE - A 4-byte constant sequence "I", "P", "A", "\0" which indicates the valid start of a TelegramHeader.
 * - [ 4 bytes ]: DATA_LENGTH - A 4-byte field where the data length is stored, allowing for proper identification of the start and end of the TelegramFrame sequence.
 * - [ 4 bytes ]: DATA_CHECKSUM - A 4-byte field where the data checksum is stored to validate the attached data.
 * - [ 1 byte  ]: COMPRESSOR_ID - A 1-byte field where the compressor ID is stored. If it's null, it means the telegram body is not compressed (in text/json format).
 * Otherwise, the telegram body is compressed. Currently, only zlib compression for the telegram body is supported, which is specified with COMPRESSOR_ID='z'.
 *
 * @note: The DATA_CHECKSUM field can be used to check the integrity of the telegram body on the client app side.
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

    /**
     * @brief Constructs a TelegramHeader object with the specified length, checksum, and optional compressor ID.
     *
     * @param length The length of the telegram body.
     * @param check_sum The checksum of the telegram body.
     * @param compressor_id The compressor ID used for compressing the telegram body (default is 0).
     */
    explicit TelegramHeader(uint32_t length, uint32_t check_sum, uint8_t compressor_id = 0);

    /**
     * @brief Constructs a TelegramHeader object from the provided byte buffer.
     *
     * This constructor initializes a TelegramHeader object using the length and checksum information taken from the provided byte buffer.
     *
     * @param buffer The ByteArray containing the header data of the telegram.
     */
    explicit TelegramHeader(const ByteArray& buffer);

    ~TelegramHeader()=default;

    /**
     * @brief Constructs a TelegramHeader based on the provided body data.
     *
     * @param body The body data used to calculate the size and checksum.
     * @param compressor_id The ID of the compressor used for compression (default is 0, means no compressor is used).
     * @return A TelegramHeader object constructed from the provided body data.
     */
    static comm::TelegramHeader construct_from_body(const std::string_view& body, uint8_t compressor_id = 0);

    /**
     * @brief Returns the total size of the TelegramHeader.
     *
     * This static constexpr method returns the total size of the TelegramHeader, including all its components.
     *
     * @return The total size of the TelegramHeader.
     */
    static constexpr size_t size() {
        return SIGNATURE_SIZE + LENGTH_SIZE + CHECKSUM_SIZE + COMPRESSORID_SIZE;
    }

    /**
     * @brief To checks if the TelegramHeader is valid.
     *
     * @return True if the TelegramHeader is valid, false otherwise.
     */
    bool is_valid() const { return m_is_valid; }

    /**
     * @brief Retrieves the buffer associated with the TelegramHeader.
     *
     * This method returns a constant reference to the buffer associated with the TelegramHeader.
     *
     * @return A constant reference to the buffer.
     */
    const ByteArray& buffer() const { return m_buffer; }

    /**
     * @brief Retrieves the number of bytes in the telegram body.
     *
     * @return The number of bytes in the telegram body.
     */
    uint32_t body_bytes_num() const { return m_body_bytes_num; }

    /**
     * @brief Retrieves the checksum of telegram body.
     *
     * @return The checksum of telegram body.
     */
    uint32_t body_check_sum() const { return m_body_check_sum; }

    /**
     * @brief Retrieves the compressor ID used for compressing telegram body.
     *
     * @return The compressor ID of the telegram body. 0 if the telegram body is not compressed.
     */
    uint8_t compressor_id() const { return m_compressor_id; }

    /**
     * @brief Checks if the telegram body is compressed.
     *
     * @return True if the telegram body is compressed; otherwise, false.
     */
    bool is_body_compressed() const { return m_compressor_id != 0; }

    /**
     * @brief Retrieves information about the telegram header.
     *
     * @return A string containing information about the telegram header.
     */
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
