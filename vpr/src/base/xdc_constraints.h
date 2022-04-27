#ifndef XDC_CONSTRAINTS_H
#define XDC_CONSTRAINTS_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "atom_netlist.h"
#include "physical_types.h"
#include "vpr_constraints.h"

class XDCStream {
  public:
    inline XDCStream(std::string name_, std::unique_ptr<std::istream>&& stream_)
        : name(name_)
        , stream(std::move(stream_)) {}

    std::string name;
    std::unique_ptr<std::istream> stream;
};

/**
 * @brief Creates VprConstraints from input stream in XDC format.
 * @param xdc_stream XDC script
 * @param arch reference to loaded device architecture
 * @param netlist loaded AtomNetlist
 * @throws TCL_eException: base class for all exceptions
 *         TCL_eFailedToInitTcl: Failure during initializations of TCL subsystem
 *         TCL_eErroneousTCL: Failure when parsing XDC
 *         TCL_eCustomException: Anything else that doesn't fit he above categories.
 * @return VprConstraints created from an XDC script.
 * 
 * This function may modify the netlist by changing its blocks' properties.
 */
VprConstraints read_xdc_constraints_to_vpr(std::vector<XDCStream>& xdc_streams, const t_arch& arch, AtomNetlist& netlist);

/**
 * @brief Parse a file in XDC format and apply it to global FloorplanningContext.
 * @param xdc_paths paths to XDC files with constraints
 * @param arch reference to loaded device architecture
 * @param netlist loaded AtomNetlist
 * @throws TCL_eException: base class for all exceptions
 *         TCL_eFailedToInitTcl: Failure during initializations of TCL subsystem
 *         TCL_eErroneousTCL: Failure when parsing XDC
 *         TCL_eCustomException: Anything else that doesn't fit he above categories.
 * 
 * This function overwrites global floorplanning constraints (g_vpr.constraints_.constraints)\
 * This function may modify the netlist by changing its blocks' properties.
 */
void load_xdc_constraints_files(const std::vector<std::string> xdc_paths, const t_arch& arch, AtomNetlist& netlist);

#endif
