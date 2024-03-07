#ifndef VTR_ODD_EVEN_ROUTING_H
#define VTR_ODD_EVEN_ROUTING_H

#include "turn_model_routing.h"

class OddEvenRouting : public TurnModelRouting{
  public:
    ~OddEvenRouting() override;

  private:
    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                         NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         const NocStorage& noc_model) override;

    TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                      NocRouterId src_router_id,
                                                      NocRouterId dst_router_id,
                                                      NocRouterId curr_router_id,
                                                      NocTrafficFlowId traffic_flow_id,
                                                      const NocStorage& noc_model) override;

    bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const override;

    static inline bool is_odd(int number);
    static inline bool is_even(int number);
};

#endif //VTR_ODD_EVEN_ROUTING_H
