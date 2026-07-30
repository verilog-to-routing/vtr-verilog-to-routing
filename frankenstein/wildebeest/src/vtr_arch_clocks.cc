#include "vtr_arch_clocks.h"

#include "vtr_arch_info.h"

// thin compatibility wrapper: the clocked-model scan now lives in the
// generalized arch reader (vtr_arch_info). the standalone clock-scan code
// that used to live here has been folded into readArchInfo's models pass.
namespace wildebeestVtr {

std::vector<std::string> readClockedModelNames(const std::string &xmlPath,
                                               std::string *errorOut) {
  VtrArchInfo info;
  if (!readArchInfo(xmlPath, info, errorOut))
    return {};
  return info.clockedModels;
}

} // namespace wildebeestVtr
