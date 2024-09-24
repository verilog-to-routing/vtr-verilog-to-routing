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

class OddEvenRouting : public TurnModelRouting {
  public:
    OddEvenRouting(const NocStorage& noc_model);

    ~OddEvenRouting() override;

  private:

    const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                         NocRouterId curr_router_id,
                                                                         NocRouterId dst_router_id,
                                                                         TurnModelRouting::Direction prev_dir,
                                                                         const NocStorage& noc_model) override;

    bool is_turn_legal(const std::array<std::reference_wrapper<const NocRouter>, 3>& noc_routers,
                       const NocStorage& noc_model) const override;

    /**
     * @brief Determines the legal directions that a traffic flow initiated from
     * `comp_src_loc` and destined to `comp_dst_loc` can take in a 2D NoC mesh topology
     * when it arrives at `comp_curr_loc`.
     *
     * @note `comp_xxxx_loc specifies the location of a NoC router in a 2D/3D NoC
     * mesh topology. The origin of the mesh is (x, y, layer) = (0, 0, 0), and each
     * coordinate can increment by 1.
     *
     * @param comp_src_loc The mesh location of the router from which the traffic flow originated.
     * @param comp_curr_loc The mesh location of the router to which the traffic flow has been routed so far.
     * @param comp_dst_loc The mesh location of the router to which the traffic flow is destined.
     * @param prev_dir The last direction the traffic flow took to arrive at `comp_curr_loc`.
     */
    inline void determine_legal_directions_2d(t_physical_tile_loc comp_src_loc,
                                              t_physical_tile_loc comp_curr_loc,
                                              t_physical_tile_loc comp_dst_loc,
                                              TurnModelRouting::Direction prev_dir);

    /**
     * @brief Determines the legal directions that a traffic flow initiated from
     * `comp_src_loc` and destined to `comp_dst_loc` can take in a 3D NoC mesh topology
     * when it arrives at `comp_curr_loc`.
     *
     * @note Arguments are similar to determine_legal_directions_2d() method.
     */
    inline void determine_legal_directions_3d(t_physical_tile_loc comp_src_loc,
                                              t_physical_tile_loc comp_curr_loc,
                                              t_physical_tile_loc comp_dst_loc,
                                              TurnModelRouting::Direction prev_dir);

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

  private:
    /**
     * @brief Stores the location of each NoC router in the NoC mesh grid.
     *
     * The odd-even routing algorithm assumes that NoC routers are organized
     * in a 2D/3D mesh topology. `compressed_noc_locs_` stores the location of each NoC
     * routes in the mesh. These locations are specified by (x, y, layer) tuples,
     * where x, y, and layer start from zero and take consecutive values.
     */
    vtr::vector<NocRouterId, t_physical_tile_loc> compressed_noc_locs_;
};

#endif //VTR_ODD_EVEN_ROUTING_H
