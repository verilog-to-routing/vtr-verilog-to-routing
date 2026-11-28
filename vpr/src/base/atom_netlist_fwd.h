#pragma once

#include "vtr_strong_id.h"
#include "netlist_fwd.h"
/*
 * This header forward declares the AtomNetlist class, and defines common types by it
 */

//Forward declaration
class AtomNetlist;
class AtomLookup;

/*
 * Ids
 *
 * The AtomNetlist uses unique IDs to identify any component of the netlist.
 * To avoid type-conversion errors (e.g. passing an AtomPinId where an AtomNetId
 * was expected), we use vtr::StrongId's to disallow such conversions. See
 * vtr_strong_id.h for details.
 */

//A unique identifier for a block/primitive in the atom netlist
typedef vtr::StrongId<struct atom_block_id_tag> AtomBlockId;

//A unique identifier for a net in the atom netlist
typedef vtr::StrongId<struct atom_net_id_tag> AtomNetId;

//A unique identifier for a port in the atom netlist
typedef vtr::StrongId<struct atom_port_id_tag> AtomPortId;

//A unique identifier for a pin in the atom netlist
typedef vtr::StrongId<struct atom_pin_id_tag> AtomPinId;

//A signal index in a port
typedef unsigned BitIndex;

//The type of a block in the AtomNetlist
enum class AtomBlockType : char {
    INPAD,  //A primary input
    OUTPAD, //A primary output
    BLOCK   //A basic atom block (LUT, FF, blackbox etc.)
};
