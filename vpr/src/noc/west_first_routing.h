#ifndef VTR_WEST_FIRST_ROUTING_H
#define VTR_WEST_FIRST_ROUTING_H

/**
 * @file
 * @brief This file declares the WestFirstRouting class, which implements
 * the west-first routing algorithm.
 *
 * Overview
 * ========
 * The WestFirstRouting class performs packet routing between routers in the
 * NoC using west-first routing algorithm. In this algorithm, moving towards
 * west has a higher priority than other directions. When the traffic flow
 * moves in directions other than west, it is no longer allowed to move
 * in towards west.
 */

#include "turn_model_routing.h"

class WestFirstRouting : public TurnModelRouting {
  public:
    ~WestFirstRouting() override;

  private:
    /**
     * @brief Returns legal directions that the traffic flow can follow to
     * generate a minimal route using west-first algorithm.
     *
     * @param src_router_id  A unique ID identifying the source NoC router.
     * @param curr_router_id A unique ID identifying the current NoC router.
     * @param dst_router_id  A unique ID identifying the destination NoC router.
     * @param noc_model A model of the NoC. This is used to  determine the position
     * of NoC routers.
     *
     * @return std::vector<TurnModelRouting::Direction> All legal directions that the
     * a traffic flow can follow based on west-first algorithm
     */
    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                         NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         TurnModelRouting::Direction prev_dir,
                                                                         const NocStorage& noc_model) override;

    bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                       const NocStorage& noc_model) const override;
};

#endif //VTR_WEST_FIRST_ROUTING_H
