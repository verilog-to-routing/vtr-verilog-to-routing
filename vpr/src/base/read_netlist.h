#pragma once
/**
 * @file
 * @author Jason Luu
 * @date   May 2009
 *
 * @brief Read a circuit netlist in XML format and populate
 *        the netlist data structures for VPR
 */

#include "atom_netlist_fwd.h"
#include "clustered_netlist_fwd.h"
#include "physical_types.h"

ClusteredNetlist read_netlist(const char* net_file,
                              const t_arch* arch,
                              bool verify_file_digests,
                              int verbosity);

void set_atom_pin_mapping(const ClusteredNetlist& clb_nlist,
                          const AtomBlockId atom_blk,
                          const AtomPortId atom_port,
                          const t_pb_graph_pin* gpin);
