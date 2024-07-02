#ifndef VTR_TURN_MODEL_ROUTING_H
#define VTR_TURN_MODEL_ROUTING_H

/**
 * @file
 * @brief This file declares the TurnModelRouting class, which abstract all
 * Turn Model routing algorithms. Classes implementing specific Turn Model
 * algorithms are expected to inherit from this class and comply with its
 * interface.
 *
 * Overview
 * ========
 * The TurnModelRouting class abstracts Turn Model routing algorithms.
 * The route_flow() method implemented in this class, routes a traffic flow
 * by adding a NoC link at a time to form a route between two NoC routers.
 * The destination router of the last added NoC link is called the current
 * NoC router.
 * The main idea in Turn Model algorithm is to forbid specific turns
 * for traffic flows to avoid deadlock. If at least one clockwise turn
 * and one anticlockwise turn are forbidden, no cyclic dependency can
 * happen, making deadlock impossible. Turn model algorithms forbid
 * specific turns based on the source, destination, and current NoC
 * router locations in a mesh or torus topology. TurnModelRouting class
 * exposes a shared interface for all Turn Model routing algorithms.
 * Derived classes can implement specific routing algorithms by implementing
 * their override of the exposed interface. More specifically,
 * the get_legal_directions() method returns legal directions that a
 * traffic flow can follow based on where the source, destination, and current
 * NoC routers are located. select_next_direction() selects one of these
 * legal directions. The TurnModelRouting() class does not implement these
 * methods, but calls them in route_flow(). For example, XYRouting can be
 * implemented by overriding these two methods. get_legal_directions()
 * should return horizontal directions when the current router and the
 * destination are not in the same column. When the traffic flow arrives
 * a router located in the same column as the destination,
 * get_legal_directions() should return vertical directions.
 * select_next_direction() selects one of two available directions to get
 * closer to the destination.
 *
 * When the routing algorithm presents two possible direction choices
 * at a router, we use a biased coin flip to randomly select one of them.
 * This random decision is biased towards choosing the direction in which
 * the distance to the destination is longer. For example, if the last router
 * added to a partial route is located at (3, 5) while the route is destined
 * for (7, 7), the random decision favors the X-direction twice as much as
 * the Y-direction. This approach distributes the chance of selecting
 * a shortest path more evenly among all possible paths between two
 * NoC routers.
 *
 * TurnModelRouting also provides multiple helper methods that can be used
 * by derived classes.
 */

#include "noc_routing.h"
#include <array>

class TurnModelRouting : public NocRouting {
  public:
    ~TurnModelRouting() override;

    /**
     * @brief Finds a minimal route that goes from the starting router in a
     * traffic flow to the destination router. Uses one of Turn Model
     * routing algorithms to determine the route. This method does not
     * implement any routing algorithm itself. It only calls get_legal_directions()
     * to find legal directions and select_next_direction() to select one of them.
     * The routing algorithm is specified by a derived class implementing the
     * mentioned methods.
     * A route consists of a series of links that should be traversed when
     * travelling between two routers within the NoC.
     *
     * @param src_router_id The source router of a traffic flow. Identifies
     * the starting point of the route within the NoC. This represents a
     * physical router on the FPGA.
     * @param sink_router_id The destination router of a traffic flow.
     * Identifies the ending point of the route within the NoC.This represents a
     * physical router on the FPGA.
     * @param traffic_flow_id The unique ID for the traffic flow being routed.
     * @param flow_route Stores the path returned by this function
     * as a series of NoC links found by
     * a NoC routing algorithm between two routers in a traffic flow.
     * The function will clear any
     * previously stored path and re-insert the new found path between the
     * two routers.
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     */
    void route_flow(NocRouterId src_router_id,
                    NocRouterId sink_router_id,
                    NocTrafficFlowId traffic_flow_id,
                    std::vector<NocLinkId>& flow_route,
                    const NocStorage& noc_model) override;

    /**
     * @brief Turn model algorithms forbid specific turns in the mesh topology
     * to guarantee deadlock-freedom. This function finds all illegal turns
     * implied by a turn model routing algorithm.
     *
     * @param noc_model Contains NoC router and link connectivity information.
     * @return A vector of std::pair<NocLinkId, NocLinkId>. In each pair,
     * a traffic flow cannot traverse the second link after the first link.
     */
    std::vector<std::pair<NocLinkId, NocLinkId>> get_all_illegal_turns(const NocStorage& noc_model) const;

  protected:
    /**
     * @brief This enum describes the all the possible
     * directions the turn model routing algorithms can
     * choose to travel.
     */
    enum class Direction {
        LEFT,        /*!< Moving towards the negative X-axis*/
        RIGHT,       /*!< Moving towards the positive X-axis*/
        UP,          /*!< Moving towards the positive Y-axis*/
        DOWN,        /*!< Moving towards the negative Y-axis*/
        INVALID      /*!< Invalid direction*/
    };

    /**
     * Generates a hash value for the combination of given arguments.
     * @param src_router_id A unique ID identifying the source NoC router.
     * @param dst_router_id A unique ID identifying the destination NoC router.
     * @param curr_router_id A unique ID identifying the current NoC router.
     * @param traffic_flow_id A unique ID identifying the traffic flow to be routed.
     * @return The computed hash value.
     */
    size_t get_hash_value(NocRouterId src_router_id,
                          NocRouterId dst_router_id,
                          NocRouterId curr_router_id,
                          NocTrafficFlowId traffic_flow_id);

    /**
     * @brief Returns the first vertical direction found among given directions.
     *
     * @param directions  A list of directions.
     *
     * @return Direction The first vertical direction found or INVALID if there
     * is no vertical direction among given directions.
     */
    TurnModelRouting::Direction select_vertical_direction(const std::vector<TurnModelRouting::Direction>& directions);

    /**
     * @brief Returns the first horizontal direction found among given directions.
     *
     * @param directions  A list of directions.
     *
     * @return Direction The first horizontal direction found or INVALID if there
     * is no horizontal direction among given directions.
     */
    TurnModelRouting::Direction select_horizontal_direction(const std::vector<TurnModelRouting::Direction>& directions);

    /**
     * @brief Returns the first direction among given direction
     * that differs with "other_than" direction
     *
     * @param directions  A list of directions.
     * @param other_than Specifies the direction that should not be returned.
     *
     * @return Direction The first direction that is different than "other_than"
     * or INVALID if only "other_than" was among possible choices.
     */
    TurnModelRouting::Direction select_direction_other_than(const std::vector<TurnModelRouting::Direction>& directions,
                                                            TurnModelRouting::Direction other_than);

  private:
    /**
     * @brief Given the direction to travel next, this function determines
     * the outgoing link that should be used to travel in the intended direction.
     * Each router may have any number of outgoing links and each link is not
     * guaranteed to point in the intended direction, So this function makes
     * sure that the link chosen points in the intended direction.
     *
     * @param curr_router_id The physical router on the FPGA that the routing
     * algorithm is currently visiting. This argument is updated in this method
     * returned by reference.
     * @param curr_router_position The grid position of the router that is
     * currently being visited on the FPGA
     * @param next_step_direction The direction to travel next
     * @param visited_routers Keeps track of which routers have been reached
     * already while traversing the NoC.
     * @param noc_model A model of the NoC. This is used to traverse the
     * NoC and find a route between the two routers.
     * @return NocLinkId A unique ID specifying the NoC link that should be
     * traversed to move towards the given direction. If none of the current
     * NoC router's links travel in the given direction, NoCLinkID::INVALID
     * is returned.
     */
    NocLinkId move_to_next_router(NocRouterId& curr_router_id,
                                  const t_physical_tile_loc& curr_router_position,
                                  TurnModelRouting::Direction next_step_direction,
                                  std::unordered_set<NocRouterId>& visited_routers,
                                  const NocStorage& noc_model);


    /**
     * @brief Computes MurmurHash3 for an array of 32-bit words initialized
     * with seed. As discussed in the comment at the top of this file,
     * when two possible directions are presented by get_legal_directions(),
     * we flip a biased coin by favoring the direction along which the distance
     * to the destination is longer. This hash function is used to generate
     * a hash value, which is treated as random value. The generated
     * hash value is compared with a threshold to determine the next
     * direction to be taken
     * @param key Contains elements to be hashed
     * @param seed The initialization value.
     * @return uint32_t Computed hash value.
     */
    uint32_t murmur3_32(const std::vector<uint32_t>& key, uint32_t seed);

    /**
     * @brief Returns legal directions that the traffic flow can follow.
     * The legal directions might be a subset of all directions to guarantee
     * deadlock freedom.
     *
     * @param src_router_id  A unique ID identifying the source NoC router.
     * @param curr_router_id A unique ID identifying the current NoC router.
     * @param dst_router_id  A unique ID identifying the destination NoC router.
     * @param noc_model A model of the NoC. This might be used by the derived class
     * to determine the position of NoC routers.
     *
     * @return std::vector<TurnModelRouting::Direction> All legal directions that the
     * a traffic flow can follow.
     */
    virtual const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                                 NocRouterId curr_router_id,
                                                                                 NocRouterId dst_router_id,
                                                                                 const NocStorage& noc_model) = 0;

    /**
     * @brief Selects a direction from legal directions. The traffic flow
     * travels travels in that direction.
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
    virtual TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                              NocRouterId src_router_id,
                                                              NocRouterId dst_router_id,
                                                              NocRouterId curr_router_id,
                                                              NocTrafficFlowId traffic_flow_id,
                                                              const NocStorage& noc_model)
        = 0;

    /**
     * @brief Determines whether a turn specified by 3 NoC routers visited in the turn
     * is legal. Turn model routing algorithms forbid specific turns in the mesh topology
     * to guarantee deadlock-freedom. In addition to turns forbidden by the turn model algorithm,
     * 180-degree turns are also illegal.
     *
     * @param noc_routers Three NoC routers visited in a turn.
     * @return True if the turn is legal, otherwise false.
     */
    virtual bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers) const = 0;

  protected:
    // get_legal_directions() return a reference to this vector to avoid allocating a new vector
    // each time it is called
    std::vector<TurnModelRouting::Direction> returned_legal_direction{4};

  private:
    std::vector<uint32_t> inputs_to_murmur3_hasher{4};

};

#endif //VTR_TURN_MODEL_ROUTING_H
