#ifndef RR_EDGE_H
#define RR_EDGE_H

/* TODO: MUST change the node id to RRNodeId before refactoring is finished! */
struct t_rr_edge_info {
    t_rr_edge_info(int from, int to, short type) noexcept
        : from_node(from)
        , to_node(to)
        , switch_type(type) {}

    int from_node = OPEN;
    int to_node = OPEN;
    short switch_type = OPEN;

    friend bool operator<(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        return std::tie(lhs.from_node, lhs.to_node, lhs.switch_type) < std::tie(rhs.from_node, rhs.to_node, rhs.switch_type);
    }

    friend bool operator==(const t_rr_edge_info& lhs, const t_rr_edge_info& rhs) {
        return std::tie(lhs.from_node, lhs.to_node, lhs.switch_type) == std::tie(rhs.from_node, rhs.to_node, rhs.switch_type);
    }
};

typedef std::vector<t_rr_edge_info> t_rr_edge_info_set;

#endif /* RR_EDGE */
