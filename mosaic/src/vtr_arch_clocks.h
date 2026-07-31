#ifndef WILDEBEEST_VTR_ARCH_CLOCKS_H
#define WILDEBEEST_VTR_ARCH_CLOCKS_H

#include <string>
#include <vector>

// self-contained reader for the one piece of the vtr architecture xml that the
// mosaic flow needs: the names of <model>s that own a clock input port.
//
// this reproduces what parmys pulls out of the arch through vtr's libarchfpga
// (models whose input <port> carries is_clock="1") but without linking any vtr
// static library, so the wildebeest plugin stays standalone. only the
// <models> section is scanned; routing/layout/tile data is ignored.
namespace wildebeestVtr {

// parse the arch xml at xmlPath and return the names of every <model> that has
// an <input_ports> child <port ... is_clock="1"/>. on read failure the result
// is empty and errorOut (when non-null) carries a message.
std::vector<std::string> readClockedModelNames(const std::string &xmlPath,
                                               std::string *errorOut = nullptr);

} // namespace wildebeestVtr

#endif // WILDEBEEST_VTR_ARCH_CLOCKS_H
