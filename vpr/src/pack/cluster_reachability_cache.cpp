#include "cluster_reachability_cache.h"

#include <queue>
#include <unordered_set>

#include "physical_types.h"

namespace {

const t_pb_graph_node* root_pb_graph_node(const t_pb_graph_pin& pin) {
    const t_pb_graph_node* node = pin.parent_node;
    while (!node->is_root()) {
        node = node->parent_pb_graph_node;
    }
    return node;
}

bool compute_reachability(const t_pb_graph_pin& src_pin, const t_pb_graph_pin& sink_pin) {
    if (&src_pin == &sink_pin) {
        return true;
    }

    std::queue<const t_pb_graph_pin*> pins_to_visit;
    std::unordered_set<const t_pb_graph_pin*> visited_pins;

    pins_to_visit.push(&src_pin);
    visited_pins.insert(&src_pin);

    while (!pins_to_visit.empty()) {
        const t_pb_graph_pin* curr_pin = pins_to_visit.front();
        pins_to_visit.pop();

        for (int iedge = 0; iedge < curr_pin->num_output_edges; ++iedge) {
            const t_pb_graph_edge* edge = curr_pin->output_edges[iedge];

            for (int ipin = 0; ipin < edge->num_output_pins; ++ipin) {
                const t_pb_graph_pin* next_pin = edge->output_pins[ipin];

                if (next_pin == &sink_pin) {
                    return true;
                }

                if (visited_pins.insert(next_pin).second) {
                    pins_to_visit.push(next_pin);
                }
            }
        }
    }

    return false;
}

} // namespace

bool ClusterReachabilityCache::is_reachable(const t_pb_graph_pin& src_pin, const t_pb_graph_pin& sink_pin) {
    const t_pb_graph_node* src_root = root_pb_graph_node(src_pin);
    const t_pb_graph_node* sink_root = root_pb_graph_node(sink_pin);

    if (src_root != sink_root) {
        return false;
    }

    ReachabilityMap& reachability = reachability_by_root_[src_root];
    PinPair pin_pair(src_pin.pin_count_in_cluster, sink_pin.pin_count_in_cluster);

    auto iter = reachability.find(pin_pair);
    if (iter != reachability.end()) {
        return iter->second;
    }

    bool reachable = compute_reachability(src_pin, sink_pin);
    reachability.emplace(pin_pair, reachable);
    return reachable;
}
