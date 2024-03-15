#include "zlibutils.h"

#include <cstring> // Include cstring for memset
#include <zlib.h>

std::string tryCompress(const std::string& decompressed) 
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
        return "";
    }

    zs.next_in = (Bytef*)decompressed.data();
    zs.avail_in = decompressed.size();

    int retCode;
    char resultBuffer[32768];
    std::string result;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(resultBuffer);
        zs.avail_out = sizeof(resultBuffer);

        retCode = deflate(&zs, Z_FINISH);

        if (result.size() < zs.total_out) {
            result.append(resultBuffer, zs.total_out - result.size());
        }
    } while (retCode == Z_OK);

    deflateEnd(&zs);

    if (retCode != Z_STREAM_END) {
        return "";
    }

    return result;
}

std::string tryDecompress(const std::string& compressed) 
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        return "";
    }

    zs.next_in = (Bytef*)compressed.data();
    zs.avail_in = compressed.size();

    int retCode;
    char resultBuffer[32768];
    std::string result;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(resultBuffer);
        zs.avail_out = sizeof(resultBuffer);

        retCode = inflate(&zs, 0);

        if (result.size() < zs.total_out) {
            result.append(resultBuffer, zs.total_out - result.size());
        }

    } while (retCode == Z_OK);

    inflateEnd(&zs);

    if (retCode != Z_STREAM_END) {
        return "";
    }

    return result;
}


