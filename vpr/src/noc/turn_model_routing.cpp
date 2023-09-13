#include "turn_model_routing.h"

TurnModelRouting::TurnModelRouting(const NocStorage& noc_model, const std::optional<std::reference_wrapper<const NocVirtualBlockStorage>>& noc_virtual_blocks)
    : NocRouting(noc_model, noc_virtual_blocks) {
}

TurnModelRouting::~TurnModelRouting() = default;

size_t TurnModelRouting::get_hash_value(NocRouterId src_router_id,
                                        NocRouterId dst_router_id,
                                        NocRouterId curr_router_id,
                                        NocTrafficFlowId traffic_flow_id) {
    // clear inputs from the last time this function was called
    inputs_to_murmur3_hahser.clear();

    // used to cast vtr::StrongId types to uint32_t
    auto cast_to_uint32 = [](const auto& input) {
        return static_cast<uint32_t>(static_cast<size_t>(input));
    };

    // insert IDs into the vector
    inputs_to_murmur3_hahser.push_back(cast_to_uint32(src_router_id));
    inputs_to_murmur3_hahser.push_back(cast_to_uint32(dst_router_id));
    inputs_to_murmur3_hahser.push_back(cast_to_uint32(curr_router_id));
    inputs_to_murmur3_hahser.push_back(cast_to_uint32(traffic_flow_id));

    uint32_t hash_val = murmur3_32(inputs_to_murmur3_hahser, 0);

    return hash_val;
}

void TurnModelRouting::route_flow(NocRouterId src_router_id,
                                  NocRouterId dst_router_id,
                                  NocTrafficFlowId traffic_flow_id,
                                  std::vector<NocLinkId>& flow_route) {
    // ensure that the route container is empty
    flow_route.clear();

    // get source and destination NoC routers
    const auto& src_router = noc_model_.get_single_noc_router(src_router_id);
    const auto& dst_router = noc_model_.get_single_noc_router(dst_router_id);

    // the last router added to the path, initialized with the source id
    NocRouterId curr_router_id = src_router_id;

    // get the physical location of the destination router
    const auto dst_loc = dst_router.get_router_physical_location();

    /**
     * Keeps track of which routers have been reached already
     * while traversing the NoC. This variable will help determine
     * cases where a route could not be found and the algorithm is
     * stuck going back and forth between routers it has already
     * visited.
     */
    std::unordered_set<NocRouterId> visited_routers;

    // The route is terminated when we reach at the destination router
    while (curr_router_id != dst_router_id) {
        // get the current router (the last one added to the route)
        const auto& curr_router = noc_model_.get_single_noc_router(curr_router_id);

        // get the physical location of the current router
        auto curr_router_pos = curr_router.get_router_physical_location();

        // get all directions that moves us closer to the destination router
        const auto legal_directions = get_legal_directions(src_router_id, curr_router_id, dst_router_id);

        // select the next direction from the available options
        auto next_step_direction = select_next_direction(legal_directions,
                                                         src_router_id,
                                                         dst_router_id,
                                                         curr_router_id,
                                                         traffic_flow_id);

        auto next_link = move_to_next_router(curr_router_id, curr_router_pos, next_step_direction, visited_routers);

        if (next_link) {
            flow_route.push_back(next_link);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "No route could be found from starting router with ID:'%d'"
                            "and the destination router with ID:'%d' using the XY-Routing algorithm.",
                            src_router.get_router_user_id(),
                            dst_router.get_router_user_id());
        }
    }
}

NocLinkId TurnModelRouting::move_to_next_router(NocRouterId& curr_router_id,
                                                const t_physical_tile_loc& curr_router_position,
                                                TurnModelRouting::Direction next_step_direction,
                                                std::unordered_set<NocRouterId>& visited_routers) {
    // represents the router that will be visited when taking an outgoing link
    NocRouterId next_router_id(-1);

    // next link to be added to the route, initialized with INVALID
    auto next_link = NocLinkId();

    // keeps track of whether a router was found that we can move to
    bool found_next_router = false;

    // When an acceptable link is found, this variable keeps track of whether the next router visited using the link was already visited or not.
    bool visited_next_router = false;

    // get all the outgoing links for the current router
    const auto& router_connections = noc_model_.get_noc_router_connections(curr_router_id);

    // go through each outgoing link and determine whether the link leads towards the intended route direction
    for (auto connecting_link : router_connections) {
        // get the current outgoing link which is being processed
        const NocLink& curr_outgoing_link = noc_model_.get_single_noc_link(connecting_link);

        // get the next router that we will visit if we travel across the current link
        next_router_id = curr_outgoing_link.get_sink_router();
        const NocRouter& next_router = noc_model_.get_single_noc_router(next_router_id);

        // get the coordinates of the next router
        auto next_router_position = next_router.get_router_physical_location();

        /* Using the position of the next router we will visit if we take the current link,
         * determine if the travel direction through the link matches
         * the direction the algorithm determined we must travel in.
         * If the directions do not match, then this link is not valid.
         */
        switch (next_step_direction) {
            case TurnModelRouting::Direction::LEFT:
                if (next_router_position.x < curr_router_position.x) {
                    found_next_router = true;
                }
                break;
            case TurnModelRouting::Direction::RIGHT:
                if (next_router_position.x > curr_router_position.x) {
                    found_next_router = true;
                }
                break;
            case TurnModelRouting::Direction::UP:
                if (next_router_position.y > curr_router_position.y) {
                    found_next_router = true;
                }
                break;
            case TurnModelRouting::Direction::DOWN:
                if (next_router_position.y < curr_router_position.y) {
                    found_next_router = true;
                }
                break;
            default:
                break;
        }
        // check whether the next router we will visit was already visited
        if (visited_routers.find(next_router_id) != visited_routers.end()) {
            visited_next_router = true;
        }

        // check if the current link was acceptable. If it is, then make sure that the next router was not previously visited.
        // If the next router was already visited, then this link is not valid, so indicate this and move onto processing the next link.
        if (found_next_router && !visited_next_router) {
            // if we are here then the link is legal to traverse,
            // so add it to the found route and traverse the link by moving to the router connected by this link
            next_link = connecting_link;
            curr_router_id = next_router_id;

            // we found a suitable router to visit next, so add it to the set of visited routers
            visited_routers.insert(next_router_id);

            break;
        } else {
            found_next_router = false;
            visited_next_router = false;
        }
    }

    return next_link;
}

uint32_t TurnModelRouting::murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t TurnModelRouting::murmur3_32(const std::vector<uint32_t>& key, uint32_t seed) {
    uint32_t h = seed;

    for (uint32_t k : key) {

        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }

    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
//    h ^= murmur_32_scramble(0);
    /* Finalize. */
    h ^= key.size() * 4;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

TurnModelRouting::Direction TurnModelRouting::select_vertical_direction(const std::vector<TurnModelRouting::Direction>& directions) {
    // iterate over the given iterations and return the first vertical one
    for (const auto& direction : directions) {
        if (direction == TurnModelRouting::Direction::DOWN || direction == TurnModelRouting::Direction::UP) {
            return direction;
        }
    }

    // if there was not any vertical directions, return INVALID
    return TurnModelRouting::Direction::INVALID;
}

TurnModelRouting::Direction TurnModelRouting::select_horizontal_direction(const std::vector<TurnModelRouting::Direction>& directions) {
    // iterate over the given iterations and return the first horizontal one
    for (const auto& direction : directions) {
        if (direction == TurnModelRouting::Direction::RIGHT || direction == TurnModelRouting::Direction::LEFT) {
            return direction;
        }
    }

    // if there was not any horizontal directions, return INVALID
    return TurnModelRouting::Direction::INVALID;
}

TurnModelRouting::Direction TurnModelRouting::select_direction_other_than(const std::vector<TurnModelRouting::Direction>& directions, TurnModelRouting::Direction other_than) {
    // Iterate over all given directions and return the first one which is not "other_than"
    for (const auto& direction : directions) {
        if (direction != other_than) {
            return direction;
        }
    }

    // if there was not any direction different from "other_than", return INVALID
    return TurnModelRouting::Direction::INVALID;
}