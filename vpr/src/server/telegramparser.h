#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#include <string>

namespace server {

/**
 * @brief Dummy JSON parser using regular expressions.
 * 
 * This module provides helper methods to extract values such as "id", "cmd", or "options" 
 * from a JSON schema structured as follows: {id: num, cmd: enum, options: string}.
 * The regular expressions implemented in this parser aim to retrieve specific fields' values 
 * from a given JSON structure, facilitating data extraction and manipulation.
 */
class TelegramParser {
public:
    static int extractJobId(const std::string& message);
    static int extractCmd(const std::string& message);
    static std::string extractOptions(const std::string& message);
};

} // namespace server

#endif // TELEGRAMPARSER_H
