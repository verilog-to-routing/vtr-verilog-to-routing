#pragma once

#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "clustered_netlist_fwd.h"
#include "routing_netlist.h"
#include "vtr_ragged_matrix.h"

template<typename T, typename NetId>
using NetPinsMatrix_ = vtr::FlatRaggedMatrix<T, NetId, int>;

template<typename T>
using NetPinsMatrix = NetPinsMatrix_<T, RoutingNetId>;

template<typename T>
using ClbNetPinsMatrix = NetPinsMatrix_<T, ClusterNetId>;

template<typename T>
using AtomNetPinsMatrix = NetPinsMatrix_<T, AtomNetId>;

template<typename T>
ClbNetPinsMatrix<T> make_net_pins_matrix(const ClusteredNetlist& nlist, T default_value = T()) {
    auto pins_in_net = [&](ClusterNetId net) {
        return nlist.net_pins(net).size();
    };

    return ClbNetPinsMatrix<T>(nlist.nets().size(), pins_in_net, default_value);
}

template<typename T>
AtomNetPinsMatrix<T> make_net_pins_matrix(const AtomNetlist& nlist, T default_value = T()) {
    auto pins_in_net = [&](AtomNetId net) {
        return nlist.net_pins(net).size();
    };

    return AtomNetPinsMatrix<T>(nlist.nets().size(), pins_in_net, default_value);
}

template<typename T>
NetPinsMatrix<T> make_net_pins_matrix(const RoutingNetlist& nlist, T default_value = T()) {
    auto pins_in_net = [&](RoutingNetId net) {
        return nlist.net_pins(net).size();
    };

    return NetPinsMatrix<T>(nlist.nets().size(), pins_in_net, default_value);
}
