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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
class Netlist;

//A signal index in a port
typedef unsigned BitIndex;

//The type of a port in the Netlist
enum class PortType : char {
    INPUT,  //The port is a data-input
    OUTPUT, //The port is an output (usually data, but potentially a clock)
    CLOCK   //The port is an input clock
};

enum class PinType : char {
    DRIVER, //The pin drives a net
    SINK,   //The pin is a net sink
    OPEN    //The pin is an open connection (undecided)
};

#endif