/* Defines the function used to load an rr graph written in xml format into vpr*/

#ifndef RR_GRAPH_FPGA_INTERCHANGE_H
#define RR_GRAPH_FPGA_INTERCHANGE_H

#include "rr_graph.h"
#include "device_grid.h"
#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"
#include <functional>

void build_rr_graph_fpga_interchange(const t_graph_type graph_type,
                                     const DeviceGrid& grid,
                                     const std::vector<t_segment_inf>& segment_inf,
                                     const enum e_base_cost_type base_cost_type,
                                     int* wire_to_rr_ipin_switch,
                                     std::string& read_rr_graph_filename,
                                     bool do_check_rr_graph);

namespace hash_tuple {
namespace {

// https://nullprogram.com/blog/2018/07/31/
inline size_t splittable64(size_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9U;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebU;
    x ^= x >> 31;
    return x;
}

template<typename TT>
struct hash {
    size_t operator()(TT const& tt) const {
        return splittable64(std::hash<TT>()(tt));
    }
};

template<class T1, class T2>
struct hash<std::pair<T1, T2>> {
    size_t operator()(const std::pair<T1, T2>& p) const noexcept(true) {
        size_t lhs, rhs;
        lhs = splittable64(std::hash<T1>()(p.first));
        rhs = splittable64(std::hash<T2>()(p.second));
        return (lhs * lhs + 3UL * lhs + 2UL * lhs * rhs + rhs + rhs * rhs) / 2UL;
    }
};
template<class T>
inline void hash_combine(std::size_t& seed, T const& v) {
    seed ^= hash_tuple::hash<T>()(v) + 0x9e3779b97f4a7c15UL + (seed << 12) + (seed >> 4);
}
// Recursive template code derived from Matthieu M.
template<class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl {
    static void apply(size_t& seed, Tuple const& tuple) {
        HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
        hash_combine(seed, std::get<Index>(tuple));
    }
};

template<class Tuple>
struct HashValueImpl<Tuple, 0> {
    static void apply(size_t& seed, Tuple const& tuple) {
        hash_combine(seed, std::get<0>(tuple));
    }
};
} // namespace

template<typename... TT>
struct hash<std::tuple<TT...>> {
    size_t operator()(std::tuple<TT...> const& tt) const {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
        return seed;
    }
};
} // namespace hash_tuple

#endif /* RR_GRAPH_FPGA_INTERCHANGE_H */
