/**
 * Author: Jason Luu
 * Date: May 2009
 *
 * Read a circuit netlist in XML format and populate the netlist data structures for VPR
 */

#ifndef READ_NETLIST_H
#define READ_NETLIST_H

#include "vpr_types.h"

ClusteredNetlist read_netlist(const char* net_file,
                              const t_arch* arch,
                              bool verify_file_digests,
                              int verbosity);

#endif
