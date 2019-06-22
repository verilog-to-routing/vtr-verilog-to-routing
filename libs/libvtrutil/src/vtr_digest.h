#ifndef VTR_DIGEST_H
#define VTR_DIGEST_H
#include <iosfwd>
#include <string>

namespace vtr {

//Generate a secure hash of the file at filepath
std::string secure_digest_file(const std::string& filepath);

//Generate a secure hash of a stream
std::string secure_digest_stream(std::istream& is);

} // namespace vtr

#endif
