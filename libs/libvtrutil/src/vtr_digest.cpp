#include "vtr_digest.h"
#include "vtr_error.h"

#include <iostream>
#include <fstream>
#include <array>

#include "picosha2.h"

namespace vtr {

std::string secure_digest_file(const std::string& filepath) {
    std::ifstream is(filepath);
    if (!is) {
        throw VtrError("Failed to open file", filepath);
    }
    return secure_digest_stream(is);
}

std::string secure_digest_stream(std::istream& is) {
    //Read the stream in chunks and calculate the SHA256 digest
    picosha2::hash256_one_by_one hasher;

    std::array<char, 1024> buf;
    while (!is.eof()) {
        //Process a chunk
        is.read(buf.data(), buf.size());
        hasher.process(buf.begin(), buf.begin() + is.gcount());
    }
    hasher.finish();

    //Return the digest as a hex string, prefixed with the hash type
    //
    //Prefixing with the hash type should allow us to differentiate if the
    //hash type is ever changed in the future
    return "SHA256:" + picosha2::get_hash_hex_string(hasher);
}

} // namespace vtr
