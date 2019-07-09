#ifndef SERDES_UTILS_H_
#define SERDES_UTILS_H_

#include <string>
#include "capnp/serialize.h"

void writeMessageToFile(const std::string& file, ::capnp::MessageBuilder* builder);

#endif /* SERDES_UTILS_H_ */
