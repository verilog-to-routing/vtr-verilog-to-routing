#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#ifndef NO_SERVER

#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

namespace comm {

/**
 * @brief ByteArray as a simple wrapper over std::vector<uint8_t>
*/
class ByteArray : public std::vector<uint8_t> {
public:
    static const std::size_t DEFAULT_SIZE_HINT = 1024;

    ByteArray(const char* data)
        : std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                               reinterpret_cast<const uint8_t*>(data + std::strlen(data)))
    {}

    ByteArray(const char* data, std::size_t size)
        : std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                               reinterpret_cast<const uint8_t*>(data + size))
    {}

    ByteArray(std::size_t size_hint = DEFAULT_SIZE_HINT) {
        reserve(size_hint);
    }

    template<typename Iterator>
    ByteArray(Iterator first, Iterator last): std::vector<uint8_t>(first, last) {}

    void append(const ByteArray& appendix) {
        insert(end(), appendix.begin(), appendix.end());
    }

    void append(uint8_t b) {
        push_back(b);
    }

    std::pair<bool, std::size_t> find_sequence(const char* sequence, std::size_t sequence_size) {
        const std::size_t ssize = size();
        if (ssize >= sequence_size) {
            for (std::size_t i = 0; i <= ssize - sequence_size; ++i) {
                bool found = true;
                for (std::size_t j = 0; j < sequence_size; ++j) {
                    if (at(i + j) != sequence[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return std::make_pair(true, i);
                }
            }
        }
        return std::make_pair(false, 0);
    }

    std::string to_string() const {
        return std::string(reinterpret_cast<const char*>(this->data()), this->size());
    }

    uint32_t calc_check_sum() {
        return calc_check_sum<ByteArray>(*this);
    }

    template<typename T>
    static uint32_t calc_check_sum(const T& iterable) {
        uint32_t sum = 0;
        for (uint8_t c : iterable) {
            sum += static_cast<unsigned int>(c);
        }
        return sum;
    }
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* BYTEARRAY_H */
