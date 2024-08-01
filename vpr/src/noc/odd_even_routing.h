#ifndef VTR_ODD_EVEN_ROUTING_H
#define VTR_ODD_EVEN_ROUTING_H

#include "turn_model_routing.h"

/**
 * @file
 * @brief This file declares the OddEvenRouting class, which implements
 * the odd-even routing algorithm.
 *
 * Overview
 * ========
 * The OddEvenRouting class performs packet routing between routers in the
 * NoC using the odd-even routing algorithm. Unlike other turn model algorithms
 * that forbid at least two turns, the odd-even algorithm does not forbid
 * any turns, but only limits the column where the turn can be taken.
 * More specifically, the odd-even routing algorithms forbids NW and SW turns
 * in odd columns, while EN and ES turns are not allowed in even columns.
 */

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

    /**
     * Checks whether the given umber is odd.
     * @param number An integer number
     * @return True if the passed number of odd, otherwise false.
     */
    static inline bool is_odd(int number);

    /**
     * Checks whether the given umber is even.
     * @param number An integer number
     * @return True if the passed number of even, otherwise false.
     */
    static inline bool is_even(int number);
};

#endif //VTR_ODD_EVEN_ROUTING_H
