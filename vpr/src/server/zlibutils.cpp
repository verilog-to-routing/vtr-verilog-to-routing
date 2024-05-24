#ifndef NO_SERVER

#include "zlibutils.h"

#include <cstring> // Include cstring for memset
#include <zlib.h>

std::optional<std::string> try_compress(const std::string& decompressed) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
        return std::nullopt;
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(decompressed.data()));
    zs.avail_in = decompressed.size();

    int ret_code;
    char* result_buffer = new char[BYTES_NUM_IN_32KB];
    std::string result;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(result_buffer);
        zs.avail_out = sizeof(result_buffer);

        ret_code = deflate(&zs, Z_FINISH);

        if (result.size() < zs.total_out) {
            result.append(result_buffer, zs.total_out - result.size());
        }
    } while (ret_code == Z_OK);

    delete[] result_buffer;

    deflateEnd(&zs);

    if (ret_code != Z_STREAM_END) {
        return std::nullopt;
    }

    return result;
}

std::optional<std::string> try_decompress(const std::string& compressed) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        return std::nullopt;
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    zs.avail_in = compressed.size();

    int ret_code;
    char* result_buffer = new char[BYTES_NUM_IN_32KB];
    std::string result;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(result_buffer);
        zs.avail_out = sizeof(result_buffer);

        ret_code = inflate(&zs, 0);

        if (result.size() < zs.total_out) {
            result.append(result_buffer, zs.total_out - result.size());
        }

    } while (ret_code == Z_OK);

    delete[] result_buffer;

    inflateEnd(&zs);

    if (ret_code != Z_STREAM_END) {
        return std::nullopt;
    }

    return result;
}

#endif /* NO_SERVER */
