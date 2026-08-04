#ifndef MOSAIC_VTR_ARCH_CLOCKS_H
#define MOSAIC_VTR_ARCH_CLOCKS_H

#include <string>
#include <vector>

// names of <model>s with an is_clock input port, for max_level -vtr_arch
// clk2clk cut points (clk_domains.cc). only that model list is needed here;
// routing/layout/tile data is used later by vpr, not by this scan.
namespace mosaic {

// on read failure returns empty and sets errorOut when non-null.
std::vector<std::string> readClockedModelNames(const std::string &xmlPath,
                                               std::string *errorOut = nullptr);

} // namespace mosaic

#endif // MOSAIC_VTR_ARCH_CLOCKS_H
