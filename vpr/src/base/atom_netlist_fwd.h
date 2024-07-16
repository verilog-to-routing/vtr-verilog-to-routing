#ifndef ATOM_NETLIST_FWD_H
#define ATOM_NETLIST_FWD_H
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

//Type tags for Ids
//struct atom_block_id_tag : general_blk_id_tag {};
//struct atom_net_id_tag : general_net_id_tag {};
//struct atom_port_id_tag : general_port_id_tag {};
//struct atom_pin_id_tag : general_pin_id_tag {};

//A unique identifier for a block/primitive in the atom netlist
//typedef vtr::StrongId<atom_block_id_tag> AtomBlockId;
class AtomBlockId : public ParentBlockId {
  public:
    static constexpr AtomBlockId INVALID() { return {}; }
    using ParentBlockId::ParentBlockId;

    friend std::hash<AtomBlockId>;
};

//A unique identifier for a net in the atom netlist
//typedef vtr::StrongId<atom_net_id_tag> AtomNetId;
class AtomNetId : public ParentNetId {
  public:
    static constexpr AtomNetId INVALID() { return AtomNetId(); }
    using ParentNetId::ParentNetId;
};

//A unique identifier for a port in the atom netlist
//typedef vtr::StrongId<atom_port_id_tag> AtomPortId;
class AtomPortId : public ParentPortId {
  public:
    static constexpr AtomPortId INVALID() { return AtomPortId(); }
    using ParentPortId::ParentPortId;
};

//A unique identifier for a pin in the atom netlist
//typedef vtr::StrongId<atom_pin_id_tag> AtomPinId;
class AtomPinId : public ParentPinId {
  public:
    static constexpr AtomPinId INVALID() { return AtomPinId(); }
    using ParentPinId::ParentPinId;
};

//A signal index in a port
typedef unsigned BitIndex;

//The type of a block in the AtomNetlist
enum class AtomBlockType : char {
    INPAD,  //A primary input
    OUTPAD, //A primary output
    BLOCK   //A basic atom block (LUT, FF, blackbox etc.)
};

///@brief Specialize std::hash for StrongId's (needed for std::unordered_map-like containers)
namespace std {
template<>
struct hash<AtomBlockId> {
    std::hash<ParentBlockId> parent_hash;
    std::size_t operator()(const AtomBlockId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<AtomNetId> {
    std::hash<ParentNetId> parent_hash;
    std::size_t operator()(const AtomNetId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<AtomPortId> {
    std::hash<ParentPortId> parent_hash;
    std::size_t operator()(const AtomPortId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<AtomPinId> {
    std::hash<ParentPinId> parent_hash;
    std::size_t operator()(const AtomPinId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
} // namespace std

#endif
