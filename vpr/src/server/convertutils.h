#ifndef CONVERTUTILS_H
#define CONVERTUTILS_H

#ifndef NO_SERVER

#include <cstdint>
#include <optional>
#include <string>

const std::size_t DEFAULT_PRINT_STRING_MAX_NUM = 100;

std::optional<int> try_convert_to_int(const std::string&);
std::string get_pretty_duration_str_from_ms(int64_t durationMs);
std::string get_pretty_size_str_from_bytes_num(int64_t bytesNum);
std::string get_truncated_middle_str(const std::string& src, std::size_t num = DEFAULT_PRINT_STRING_MAX_NUM);

#endif /* NO_SERVER */

#endif /* CONVERTUTILS_H */
