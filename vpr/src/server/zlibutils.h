#ifndef ZLIBUTILS_H
#define ZLIBUTILS_H

#ifndef NO_SERVER

#include <string>
#include <optional>

constexpr const int BYTES_NUM_IN_32KB = 32768;

/**
* @brief Compresses the input sequence using zlib.
*
* This function takes a string representing the decompressed data as input
* and compresses it using zlib. If compression is successful, the compressed
* data is returned as an optional string. If compression fails, an empty optional
* is returned.
*
* @param decompressed The input string representing the decompressed data.
* @return An optional string containing the compressed data if compression is successful,
*         or an empty optional if compression fails.
*/
std::optional<std::string> try_compress(const std::string& decompressed);

/**
* @brief Decompresses the compressed sequence using zlib.
*
* This function takes a string representing the compressed data as input
* and decompresses it using zlib. If decompression is successful, the decompressed
* data is returned as an optional string. If decompression fails, an empty optional
* is returned.
*
* @param compressed The input string representing the compressed data.
* @return An optional string containing the decompressed data if decompression is successful,
*         or an empty optional if decompression fails.
*/
std::optional<std::string> try_decompress(const std::string& compressed);

#endif /* NO_SERVER */

#endif /* ZLIBUTILS_H */
