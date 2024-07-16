#ifndef NETLIST_FWD_H
#define NETLIST_FWD_H
#include "vtr_strong_id.h"

/*
 * Ids
 * ---
 * The Netlist uses unique IDs passed in by derived classes to identify any component of the netlist.
 * To avoid type-conversion errors (e.g. passing a PinId where a NetId was expected),
 * we use vtr::StrongId's to disallow such conversions. See vtr_strong_id.h for details.
 */
struct general_blk_id_tag {};
struct general_port_id_tag {};
struct general_pin_id_tag {};
struct general_net_id_tag {};

//A unique identifier for a block/primitive in the atom netlist
typedef vtr::StrongId<general_blk_id_tag> ParentBlockId;

//A unique identifier for a net in the atom netlist
typedef vtr::StrongId<general_port_id_tag> ParentPortId;

//A unique identifier for a port in the atom netlist
typedef vtr::StrongId<general_pin_id_tag> ParentPinId;

//A unique identifier for a pin in the atom netlist
typedef vtr::StrongId<general_net_id_tag> ParentNetId;

template<typename BlockId, typename PortId, typename PinId, typename NetId>
class Netlist;

///@brief A signal index in a port
typedef unsigned BitIndex;

///@brief The type of a port in the Netlist
enum class PortType : char {
    INPUT,  ///<The port is a data-input
    OUTPUT, ///<The port is an output (usually data, but potentially a clock)
    CLOCK   ///<The port is an input clock
};

enum class PinType : char {
    DRIVER, ///<The pin drives a net
    SINK,   ///<The pin is a net sink
    OPEN    ///<The pin is an open connection (undecided)
};

#endif
