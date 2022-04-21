#ifndef XDC_CONSTRAINTS_H
#define XDC_CONSTRAINTS_H

#include <string>

#include "atom_netlist.h"
#include "physical_types.h"
#include "vpr_constraints.h"

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
VprConstraints read_xdc_constraints_to_vpr(std::istream& xdc_stream, const t_arch& arch, AtomNetlist& netlist);

/**
 * @brief Parse a file in XDC format and apply it to global FloorplanningContext.
 * @param read_xdc_constraints_name path to an XDC file with constraints
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
void load_xdc_constraints_file(const char* read_xdc_constraints_name, const t_arch& arch, AtomNetlist& netlist);

#endif
