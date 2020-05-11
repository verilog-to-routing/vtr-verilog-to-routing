#ifndef VPR_NET_PINS_MATRIX_H
#define VPR_NET_PINS_MATRIX_H

#include "vtr_ragged_matrix.h"

template<typename T, typename NetId>
using NetPinsMatrix = vtr::FlatRaggedMatrix<T, NetId, int>;

template<typename T>
using ClbNetPinsMatrix = NetPinsMatrix<T, ClusterNetId>;

template<typename T>
using AtomNetPinsMatrix = NetPinsMatrix<T, AtomNetId>;

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

#endif
