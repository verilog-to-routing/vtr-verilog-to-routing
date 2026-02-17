#pragma once

#include "rr_graph_view.h"

class InterposerDelayLookahead {
  public:
    InterposerDelayLookahead(const RRGraphView& rr_graph, const DeviceGrid& grid);

    float get_interposer_lookahead_delay(RRNodeId from_node, RRNodeId to_node) const;

  private:
    vtr::NdMatrix<float, 2> die_to_die_delay_matrix_;
    const RRGraphView& rr_graph_;
    const DeviceGrid& grid_;
};
