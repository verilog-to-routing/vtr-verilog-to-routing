#ifndef RR_EDGE_H
#define RR_EDGE_H

#include "rr_graph_fwd.h"

struct t_rr_edge_info {
    t_rr_edge_info(RRNodeId from, RRNodeId to, short type, bool is_remapped) noexcept
        : from_node(from)
        , to_node(to)
        , switch_type(type)
        , remapped(is_remapped) {}

    RRNodeId from_node = RRNodeId::INVALID();
    RRNodeId to_node = RRNodeId::INVALID();
    short switch_type = OPEN;
    bool remapped = false;

    friend bool operator<(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        VTR_ASSERT(lhs.remapped == rhs.remapped);
        return std::tie(lhs.from_node, lhs.to_node, lhs.switch_type) < std::tie(rhs.from_node, rhs.to_node, rhs.switch_type);
    }

    friend bool operator==(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        VTR_ASSERT(lhs.remapped == rhs.remapped);
        return std::tie(lhs.from_node, lhs.to_node, lhs.switch_type) == std::tie(rhs.from_node, rhs.to_node, rhs.switch_type);
    }
};

typedef std::vector<t_rr_edge_info> t_rr_edge_info_set;

#endif /* RR_EDGE */
