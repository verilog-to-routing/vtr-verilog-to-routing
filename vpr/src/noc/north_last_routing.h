#ifndef VTR_NORTH_LAST_ROUTING_H
#define VTR_NORTH_LAST_ROUTING_H

/**
 * @file
 * @brief This file declares the NorthLastRouting class, which implements
 * the north-last routing algorithm.
 *
 * Overview
 * ========
 * The NorthLastRouting class performs packet routing between routers in the
 * NoC using north-last routing algorithm. In this algorithm, once the north
 * direction is taken, no other direction can be taken. In other words,
 * a traffic flow is no longer allowed to turn when it started moving
 * towards the north. North-last algorithm generated deadlock free routes
 * in a mesh or torus topology.
 */

#include "turn_model_routing.h"

class NorthLastRouting : public TurnModelRouting {
  public:
    ~NorthLastRouting() override;

  private:
    /**
     * @brief Returns legal directions that the traffic flow can follow to
     * generate a minimal route using north-last algorithm.
     *
     * @param src_router_id  A unique ID identifying the source NoC router.
     * @param curr_router_id A unique ID identifying the current NoC router.
     * @param dst_router_id  A unique ID identifying the destination NoC router.
     * @param noc_model A model of the NoC. This is used to  determine the position
     * of NoC routers.
     *
     * @return std::vector<TurnModelRouting::Direction> All legal directions that the
     * a traffic flow can follow based on north-last algorithm
     */
    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                         NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         const NocStorage& noc_model) override;

    /**
     * @brief Selects a direction from legal directions. The traffic flow
     * travels in that direction. When there are both horizontal and
     * vertical directions available, this method selects one of the available
     * directions randomly. The chance of horizontal and vertical directions
     * are weighted by the horizontal and vertical distance between the
     * current NoC router and the destination router.
     *
     * @param legal_directions Legal directions that the traffic flow can follow.
     * Legal directions are usually a subset of all possible directions to ensure
     * deadlock freedom.
     * @param src_router_id  A unique ID identifying the source NoC router.
     * @param dst_router_id  A unique ID identifying the destination NoC router.
     * @param curr_router_id A unique ID identifying the current NoC router.
     * @param noc_model A model of the NoC. This might be used by the derived class
     * to determine the position of NoC routers.
     *
     * @return Direction The direction to travel next
     */
    TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                      NocRouterId src_router_id,
                                                      NocRouterId dst_router_id,
                                                      NocRouterId curr_router_id,
                                                      NocTrafficFlowId traffic_flow_id,
                                                      const NocStorage& noc_model) override;

    bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const override;
};

#endif //VTR_NORTH_LAST_ROUTING_H
