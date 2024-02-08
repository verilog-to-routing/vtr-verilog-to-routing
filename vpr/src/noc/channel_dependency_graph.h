#ifndef VTR_CHANNEL_DEPENDENCY_GRAPH_H
#define VTR_CHANNEL_DEPENDENCY_GRAPH_H

#include "vtr_vector.h"
#include "noc_data_types.h"

class ChannelDependencyGraph {
  public:
    ChannelDependencyGraph() = delete;

    ChannelDependencyGraph(size_t n_links,
                           const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& traffic_flow_routes);

    bool has_cycles();

  private:
    vtr::vector<NocLinkId, std::vector<NocLinkId>> adjacency_list_;
};

#endif //VTR_CHANNEL_DEPENDENCY_GRAPH_H
