#ifndef CLUSTERED_NETLIST_FWD_H
#define CLUSTERED_NETLIST_FWD_H
#include "vtr_strong_id.h"
#include "netlist_fwd.h"

/*
 * StrongId's for the ClusteredNetlist class
 */

//Forward declaration
class ClusteredNetlist;

//Type tags for Ids
//struct cluster_block_id_tag : general_blk_id_tag {};
//struct cluster_net_id_tag : general_net_id_tag {};
//struct cluster_port_id_tag : general_port_id_tag {};
//struct cluster_pin_id_tag : general_pin_id_tag {};

///@brief A unique identifier for a block/primitive in the atom netlist
//typedef vtr::StrongId<cluster_block_id_tag> ClusterBlockId;
class ClusterBlockId : public ParentBlockId {
  public:
    static constexpr ClusterBlockId INVALID() { return ClusterBlockId(); }
    using ParentBlockId::ParentBlockId;
};

///@brief A unique identifier for a net in the cluster netlist
//typedef vtr::StrongId<cluster_net_id_tag> ClusterNetId;
class ClusterNetId : public ParentNetId {
  public:
    static constexpr ClusterNetId INVALID() { return ClusterNetId(); }
    using ParentNetId::ParentNetId;
};

///@brief A unique identifier for a port in the cluster netlist
//typedef vtr::StrongId<cluster_port_id_tag> ClusterPortId;
class ClusterPortId : public ParentPortId {
  public:
    static constexpr ClusterPortId INVALID() { return ClusterPortId(); }
    using ParentPortId::ParentPortId;
};

///@brief A unique identifier for a pin in the cluster netlist
//typedef vtr::StrongId<cluster_pin_id_tag> ClusterPinId;
class ClusterPinId : public ParentPinId {
  public:
    static constexpr ClusterPinId INVALID() { return ClusterPinId(); }
    using ParentPinId::ParentPinId;
};

///@brief Specialize std::hash for StrongId's (needed for std::unordered_map-like containers)
namespace std {
template<>
struct hash<ClusterBlockId> {
    std::hash<ParentBlockId> parent_hash;
    std::size_t operator()(const ClusterBlockId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<ClusterNetId> {
    std::hash<ParentNetId> parent_hash;
    std::size_t operator()(const ClusterNetId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<ClusterPortId> {
    std::hash<ParentPortId> parent_hash;
    std::size_t operator()(const ClusterPortId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
template<>
struct hash<ClusterPinId> {
    std::hash<ParentPinId> parent_hash;
    std::size_t operator()(const ClusterPinId k) const noexcept {
        return parent_hash(k); //Hash with the underlying type
    }
};
} // namespace std

#endif
