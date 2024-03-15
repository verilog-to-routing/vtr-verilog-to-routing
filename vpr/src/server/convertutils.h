#ifndef CONVERTUTILS_H
#define CONVERTUTILS_H

#include <cstdint>
#include <optional>
#include <string>

const std::size_t DEFAULT_PRINT_STRING_MAX_NUM = 100;

std::optional<int> tryConvertToInt(const std::string&);
std::string getPrettyDurationStrFromMs(int64_t durationMs);
std::string getPrettySizeStrFromBytesNum(int64_t bytesNum);
std::string getTruncatedMiddleStr(const std::string& src, std::size_t num = DEFAULT_PRINT_STRING_MAX_NUM);

#endif // CONVERTUTILS_H
