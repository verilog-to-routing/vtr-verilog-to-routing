#ifndef NETLIST2_FWD_H
#define NETLIST2_FWD_H
#include "vtr_hash.h"
#include "vtr_strong_id.h"

//Forward delcarations
class AtomNetlist;

//Type tags for Ids
struct atom_blk_id_tag;
struct atom_net_id_tag;
struct atom_port_id_tag;
struct atom_pin_id_tag;
struct atom_string_id_tag;

typedef vtr::StrongId<atom_blk_id_tag> AtomBlockId;
typedef vtr::StrongId<atom_net_id_tag> AtomNetId;
typedef vtr::StrongId<atom_port_id_tag> AtomPortId;
typedef vtr::StrongId<atom_pin_id_tag> AtomPinId;
typedef vtr::StrongId<atom_string_id_tag> AtomStringId;

typedef unsigned BitIndex;

enum class AtomPortType : char {
    INPUT,
    OUTPUT,
    CLOCK
};

enum class AtomPinType : char {
    DRIVER,
    SINK
};

enum class AtomBlockType : char {
    INPAD,
    OUTPAD,
    COMBINATIONAL,
    SEQUENTIAL
};

#endif
