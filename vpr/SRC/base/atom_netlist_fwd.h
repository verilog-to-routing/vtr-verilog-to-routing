#ifndef ATOM_NETLIST_FWD_H
#define ATOM_NETLIST_FWD_H
#include "vtr_strong_id.h"
/*
 * This header forward delcares the AtomNetlist class, and defines common types by it
 */

//Forward delcaration
class AtomNetlist;

/*
 * Ids
 *
 * The AtomNetlist uses unique IDs to identify any component of the netlist.
 * To avoid type-conversion errors (e.g. passing an AtomPinId where an AtomNetId 
 * was expected), we use vtr::StrongId's to dissallow such conversions. See
 * vtr_strong_id.h for details.
 */

//Type tags for Ids
struct atom_blk_id_tag;
struct atom_net_id_tag;
struct atom_port_id_tag;
struct atom_pin_id_tag;
struct atom_string_id_tag;

//A unique identifier for a block/primitive in the atom netlist
typedef vtr::StrongId<atom_blk_id_tag> AtomBlockId;

//A unique identifier for a net in the atom netlist
typedef vtr::StrongId<atom_net_id_tag> AtomNetId;

//A unique identifier for a port in the atom netlist
typedef vtr::StrongId<atom_port_id_tag> AtomPortId;

//A unique identifier for a pin in the atom netlist
typedef vtr::StrongId<atom_pin_id_tag> AtomPinId;

//A unique identifier for a string in the atom netlist
typedef vtr::StrongId<atom_string_id_tag> AtomStringId;

//An signal index in a port
typedef unsigned BitIndex;

//The type of a port in the AtomNetlist
enum class AtomPortType : char {
    INPUT,  //The port is a data-input
    OUTPUT, //The port is an output (usually data, but potentially a clock)
    CLOCK   //The port is an input clock
};

//The type of a pin in the AtomNetlist
enum class AtomPinType : char {
    DRIVER, //The pin drives a net
    SINK    //The pin is a net sink
};

//The type of a block in the AtomNetlist
enum class AtomBlockType : char {
    INPAD,          //A primary input
    OUTPAD,         //A primary output
    COMBINATIONAL,  //A combinational block (e.g. LUT)
    SEQUENTIAL      //A sequential (i.e. clocked) block (e.g. Flip-Flop)
};

#endif
