#pragma once

#include <map>

class t_pb_graph_node;
class t_pb_graph_pin;

class ClusterReachabilityCache {
  public:
    using PinPair = std::pair<int, int>;
    using ReachabilityMap = std::map<PinPair, bool>;

    bool is_reachable(const t_pb_graph_pin& src_pin, const t_pb_graph_pin& sink_pin);

  private:
    using RootReachabilityMap = std::map<const t_pb_graph_node*, ReachabilityMap>;

    RootReachabilityMap reachability_by_root_;
};
