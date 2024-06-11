#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#ifndef NO_SERVER

#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

namespace comm {

/**
 * @brief ByteArray is a simple wrapper over std::vector<char> that provides a user-friendly interface for manipulating array data..
*/
class ByteArray : public std::vector<char> {
public:
    static const std::size_t DEFAULT_SIZE_HINT = 1024;

    /**
     * @brief Constructs a ByteArray from a null-terminated C string.
     *
     * Constructs a ByteArray object from the specified null-terminated C string.
     * The constructor interprets the input string as a sequence of bytes until the
     * null terminator '\0' is encountered, and initializes the ByteArray with those bytes.
     *
     * @param data A pointer to the null-terminated C string from which to construct the ByteArray.
     */
    explicit ByteArray(const char* data)
        : std::vector<char>(data, data + std::strlen(data))
    {}

    /**
     * @brief Constructs a ByteArray from a raw character array.
     *
     * Constructs a ByteArray object from the specified raw character array,
     * with the given size. This constructor interprets the input data as a sequence
     * of bytes and initializes the ByteArray with those bytes.
     *
     * @param data A pointer to the raw character array from which to construct the ByteArray.
     * @param size The size of the raw character array, in bytes.
     */
    ByteArray(const char* data, std::size_t size)
        : std::vector<char>(data, data + size)
    {}

    /**
     * @brief Constructs a byte array with the specified size hint.
     *
     * This constructor initializes the byte array with an initial capacity determined by the size hint.
     *
     * @param size_hint The initial capacity hint for the byte array.
     */
    explicit ByteArray(std::size_t size_hint = DEFAULT_SIZE_HINT) {
        reserve(size_hint);
    }

    /**
     * @brief Constructs a byte array from the elements in the range [first, last).
     *
     * This constructor initializes the byte array with the elements in the range [first, last),
     * where `first` and `last` are iterators defining the range.
     *
     * @tparam Iterator The type of iterator used to specify the range.
     * @param first An iterator to the first element in the range.
     * @param last An iterator to the last element in the range.
     */
    template<typename Iterator>
    ByteArray(Iterator first, Iterator last): std::vector<char>(first, last) {}

    /**
     * @brief Appends the content of another byte array to the end of this byte array.
     *
     * This function adds all the bytes from the specified byte array `appendix`
     * to the end of this byte array.
     *
     * @param appendix The byte array whose content is to be appended.
     */
    void append(const ByteArray& appendix) {
        insert(end(), appendix.begin(), appendix.end());
    }

    /**
     * @brief Appends a byte to the end of the byte array.
     *
     * This function adds the specified byte to the end of the byte array.
     *
     * @param b The byte to append to the byte array.
     */
    void append(char b) {
        push_back(b);
    }

    /**
     * @brief Finds the position of the specified sequence in the byte array.
     *
     * This function searches for the specified sequence of characters within the byte array.
     * If the sequence is found, it returns a pair containing `true` and the starting index of the sequence.
     * If the sequence is not found, it returns a pair containing `false` and `0`.
     *
     * @param sequence A pointer to the sequence of characters to search for.
     * @param sequence_size The size of the sequence to search for.
     * @return A `std::pair` where the first element is a boolean indicating whether the sequence was
     * found (`true`) or not (`false`), and the second element is the starting index of the sequence if
     * found.
     */
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

    /**
     * @brief Converts the container to a std::string_view.
     *
     * This operator allows the container to be implicitly converted to a `std::string_view`,
     * providing a non-owning view into the container's data.
     *
     * @return A `std::string_view` representing the container's data.
     */
    operator std::string_view() const {
        return std::string_view(this->data(), this->size());
    }

    /**
     * @brief Calculates the checksum of the elements in the container.
     *
     * This function iterates over each element in the container and adds their unsigned integer representations
     * to the sum. The result is returned as a 32-bit unsigned integer.
     *
     * @return The checksum of the elements in the container.
     */
    uint32_t calc_check_sum() {
        return calc_check_sum<ByteArray>(*this);
    }

    /**
     * @brief Calculates the checksum of the elements in the given iterable container.
     *
     * This function template calculates the checksum of the elements in the provided iterable container.
     * It iterates over each element in the container and adds their unsigned integer representations to the sum.
     * The result is returned as a 32-bit unsigned integer.
     *
     * @tparam T The type of the iterable container.
     * @param iterable The iterable container whose elements checksum is to be calculated.
     * @return The checksum of the elements in the iterable container.
     */
    template<typename T>
    static uint32_t calc_check_sum(const T& iterable) {
        uint32_t sum = 0;
        for (char c : iterable) {
            sum += static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
        return sum;
    }
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* BYTEARRAY_H */
