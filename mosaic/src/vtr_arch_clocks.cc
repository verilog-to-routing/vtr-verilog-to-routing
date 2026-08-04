#include "vtr_arch_clocks.h"

#include "vtr_arch_info.h"

namespace mosaic {

std::vector<std::string> readClockedModelNames(const std::string &xmlPath,
                                               std::string *errorOut) {
  VtrArchInfo info;
  if (!readArchInfo(xmlPath, info, errorOut))
    return {};
  return info.clockedModels;
}

} // namespace mosaic
