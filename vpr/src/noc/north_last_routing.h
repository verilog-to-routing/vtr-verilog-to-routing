
#ifndef VTR_NORTH_LAST_ROUTING_H
#define VTR_NORTH_LAST_ROUTING_H

#include "turn_model_routing.h"

class NorthLastRouting : public TurnModelRouting {
  public:
    ~NorthLastRouting() override;

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
};

#endif //VTR_NORTH_LAST_ROUTING_H
