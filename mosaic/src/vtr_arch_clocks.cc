#include "vtr_arch_clocks.h"

#include "vtr_arch_info.h"

namespace mosaic {

// USE: thin wrapper so clk_domains.cc can read clocked models without linking
// libarchfpga or duplicating the xml scan in vtr_arch_info.
std::vector<std::string> readClockedModelNames(const std::string &xmlPath, std::string *errorOut) {
    VtrArchInfo info;
    if (!readArchInfo(xmlPath, info, ClassicModelNames(), errorOut))
        return {};
    return info.clockedModels;
}

} // namespace mosaic
