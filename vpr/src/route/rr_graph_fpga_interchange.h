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
                                     bool do_check_rr_graph);

namespace hash_tuple{
    namespace {
        template <typename TT>
        struct hash {
            size_t operator()(TT const& tt) const {
                return std::hash<TT>()(tt);
            }
        };

        template<class T1, class T2>
        struct hash <std::pair<T1, T2>> {
            size_t operator()(const std::pair<T1, T2> &p) const noexcept(true) {
                size_t lhs, rhs;
                lhs = std::hash<T1>()(p.first);
                rhs = std::hash<T2>()(p.second);
                lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
                return lhs;
            }
        };
        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v) {
            seed ^= hash_tuple::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }
        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl {
          static void apply(size_t& seed, Tuple const& tuple) {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
          }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0> {
          static void apply(size_t& seed, Tuple const& tuple) {
            hash_combine(seed, std::get<0>(tuple));
          }
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>> {
        size_t operator()(std::tuple<TT...> const& tt) const {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }
    };
}

#endif /* RR_GRAPH_FPGA_INTERCHANGE_H */
