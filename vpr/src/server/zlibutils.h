#ifndef ZLIBUTILS_H
#define ZLIBUTILS_H

#include <string>
#include <optional>

std::string tryCompress(const std::string& decompressed);
std::string tryDecompress(const std::string& compressed);

#endif
