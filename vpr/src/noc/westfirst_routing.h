#ifndef VTR_WESTFIRST_ROUTING_H
#define VTR_WESTFIRST_ROUTING_H

#include "turn_model_routing.h"

class WestFirstRouting : public TurnModelRouting {
  private:
    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         const NocStorage& noc_model) override;

    TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                      NocRouterId src_router_id,
                                                      NocRouterId dst_router_id,
                                                      NocRouterId curr_router_id,
                                                      NocTrafficFlowId traffic_flow_id,
                                                      const NocStorage& noc_model) override;

  private:
    const std::vector<TurnModelRouting::Direction> other_directions{TurnModelRouting::Direction::UP,
                                                                    TurnModelRouting::Direction::DOWN,
                                                                    TurnModelRouting::Direction::RIGHT};
};

#endif //VTR_WESTFIRST_ROUTING_H
