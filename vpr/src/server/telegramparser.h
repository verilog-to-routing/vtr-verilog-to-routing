#ifndef TELEGRAMPARSER_H
#define TELEGRAMPARSER_H

#include <string>

namespace telegramparser {

int extractJobId(const std::string& message);
int extractCmd(const std::string& message);
std::string extractOptions(const std::string& message);
int extractPathIndex(const std::string& message);

} // telegramparser

#endif
