#ifndef VTR_TURN_MODEL_ROUTING_H
#define VTR_TURN_MODEL_ROUTING_H

#include "noc_routing.h"
#include <limits>

class TurnModelRouting : public NocRouting {
  public:
    TurnModelRouting(const NocStorage& noc_model,
                     const std::optional<std::reference_wrapper<const NocVirtualBlockStorage>>& noc_virtual_blocks);
    ~TurnModelRouting() override;

    bool route_flow(NocRouterId src_router_id,
                    NocRouterId sink_router_id,
                    NocTrafficFlowId traffic_flow_id,
                    std::vector<NocLinkId>& flow_route) override;

  protected:
    /**
     * @brief This enum describes the all the possible
     * directions the turn model routing algorithms can
     * choose to travel.
     */
    enum class Direction {
        LEFT,   /*!< Moving towards the negative X-axis*/
        RIGHT,  /*!< Moving towards the positive X-axis*/
        UP,     /*!< Moving towards the positive Y-axis*/
        DOWN,   /*!< Moving towards the negative Y-axis*/
        INVALID /*!< Invalid direction*/
    };

    size_t get_hash_value(NocRouterId src_router_id, NocRouterId dst_router_id, NocRouterId curr_router_id, NocTrafficFlowId traffic_flow_id);

    TurnModelRouting::Direction select_vertical_direction(const std::vector<TurnModelRouting::Direction>& directions);

    TurnModelRouting::Direction select_horizontal_direction(const std::vector<TurnModelRouting::Direction>& directions);

    TurnModelRouting::Direction select_direction_other_than(const std::vector<TurnModelRouting::Direction>& directions,
                                                            TurnModelRouting::Direction other_than);

  private:
    NocLinkId move_to_next_router(NocRouterId& curr_router_id,
                                  const t_physical_tile_loc& curr_router_position,
                                  TurnModelRouting::Direction next_step_direction,
                                  std::unordered_set<NocRouterId>& visited_routers);

    inline uint32_t murmur_32_scramble(uint32_t k);

    uint32_t murmur3_32(const std::vector<uint32_t>& key, uint32_t seed);

    template<class T>
    inline void hash_combine(std::size_t& seed, T const& v);

    virtual const std::vector<TurnModelRouting::Direction>& get_legal_directions(NocRouterId src_router_id,
                                                                                 NocRouterId curr_router_id,
                                                                                 NocRouterId dst_router_id)
        = 0;

    virtual TurnModelRouting::Direction select_next_direction(const std::vector<TurnModelRouting::Direction>& legal_directions,
                                                              NocRouterId src_router_id,
                                                              NocRouterId dst_router_id,
                                                              NocRouterId curr_router_id,
                                                              NocTrafficFlowId traffic_flow_id)
        = 0;

    virtual bool routability_early_check(NocRouterId src_router_id, NocRouterId virt_router_id, NocRouterId dst_router_id) = 0;

  protected:
    // get_legal_directions() return a reference to this vector to avoid allocating a new vector
    // each time it is called
    std::vector<TurnModelRouting::Direction> returned_legal_direction{4};

  private:
    std::vector<uint32_t> inputs_to_murmur3_hahser{4};

};

template<class T>
void TurnModelRouting::hash_combine(std::size_t& seed, const T& v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

#endif //VTR_TURN_MODEL_ROUTING_H
