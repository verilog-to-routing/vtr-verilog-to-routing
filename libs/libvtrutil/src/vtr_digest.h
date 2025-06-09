#pragma once

#include <iosfwd>
#include <string>

namespace vtr {

///@brief Generate a secure hash of the file at filepath
std::string secure_digest_file(const std::string& filepath);

///@brief Generate a secure hash of a stream
std::string secure_digest_stream(std::istream& is);

} // namespace vtr
