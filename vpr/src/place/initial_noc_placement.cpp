
#include "initial_noc_placment.h"

#include "vpr_types.h"
#include "initial_placement.h"
#include "noc_place_utils.h"
#include "noc_place_checkpoint.h"
#include "place_constraints.h"

#include "sat_routing.h"

#include "vtr_math.h"
#include "vtr_time.h"

#include <queue>

/**
 * @brief Evaluates whether a NoC router swap should be accepted or not.
 * If delta cost is non-positive, the move is always accepted. If the cost
 * has increased, the probability of accepting the move is prob.
 *
 *   @param delta_cost Specifies how much the total cost would change if
 *   the proposed swap is accepted.
 *   @param prob The probability by which a router swap that increases
 *   the cost is accepted. The passed value should be in range [0, 1].
 *
 * @return true if the proposed swap is accepted, false if not.
 */
static bool accept_noc_swap(double delta_cost, double prob);

/**
 * @brief Places a constrained NoC router within its partition region.
 *
 *   @param router_blk_id NoC router cluster block ID
 *   @param blk_loc_registry Placement block location information. To be
 *   filled with the location where pl_macro is placed.
 */
static void place_constrained_noc_router(ClusterBlockId router_blk_id,
                                         BlkLocRegistry& blk_loc_registry);

/**
 * @brief Randomly places unconstrained NoC routers.
 *
 *   @param unfixed_routers Contains the cluster block ID for all unconstrained
 *   NoC routers.
 *   @param seed Used for shuffling NoC routers.
 *   @param blk_loc_registry Placement block location information. To be filled
 *   with the location where pl_macro is placed.
 */
static void place_noc_routers_randomly(std::vector<ClusterBlockId>& unfixed_routers,
                                       int seed,
                                       BlkLocRegistry& blk_loc_registry);

/**
 * @brief Runs a simulated annealing optimizer for NoC routers.
 *
 *   @param noc_opts Contains weighting factors for NoC cost terms.
 *   @param blk_loc_registry Placement block location information.
 *   To be filled with the location where pl_macro is placed.
 */
static void noc_routers_anneal(const t_noc_opts& noc_opts,
                               BlkLocRegistry& blk_loc_registry,
                               NocCostHandler& noc_cost_handler);

/**
 * @brief Returns the compressed grid of NoC.
 * @return const t_compressed_block_grid& The compressed grid of NoC.
 */
static const t_compressed_block_grid& get_compressed_noc_grid();

static const t_compressed_block_grid& get_compressed_noc_grid() {
    auto& noc_ctx = g_vpr_ctx.noc();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    // Get the logical block type for router
    const t_logical_block_type_ptr router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = place_ctx.compressed_block_grids[router_block_type->index];

    return compressed_noc_grid;
}

static bool accept_noc_swap(double delta_cost, double prob) {
    if (delta_cost <= 0.0) {
        return true;
    }

    if (prob == 0.0) {
        return false;
    }

    float random_num = vtr::frand();
    if (random_num < prob) {
        return true;
    } else {
        return false;
    }
}

static void place_constrained_noc_router(ClusterBlockId router_blk_id,
                                         BlkLocRegistry& blk_loc_registry) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    auto block_type = cluster_ctx.clb_nlist.block_type(router_blk_id);
    const PartitionRegion& pr = floorplanning_ctx.cluster_constraints[router_blk_id];

    // Create a macro with a single member
    t_pl_macro_member macro_member;
    macro_member.blk_index = router_blk_id;
    macro_member.offset = t_pl_offset(0, 0, 0, 0);
    t_pl_macro pl_macro;
    pl_macro.members.push_back(macro_member);

    bool macro_placed = false;
    for (int i_try = 0; i_try < MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY && !macro_placed; i_try++) {
        macro_placed = try_place_macro_randomly(pl_macro, pr, block_type, e_pad_loc_type::FREE, blk_loc_registry);
    }

    if (!macro_placed) {
        macro_placed = try_place_macro_exhaustively(pl_macro, pr, block_type, e_pad_loc_type::FREE, blk_loc_registry);
    }

    if (!macro_placed) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Could not place a router cluster within its constrained region");
    }
}

static void place_noc_routers_randomly(std::vector<ClusterBlockId>& unfixed_routers,
                                       int seed,
                                       BlkLocRegistry& blk_loc_registry) {
    const auto& compressed_grids = g_vpr_ctx.placement().compressed_block_grids;
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();

    /*
     * Unconstrained NoC routers are placed randomly, then NoC cost is optimized using simulated annealing.
     * For random placement, physical NoC routers are shuffled, the logical NoC routers are assigned
     * to shuffled physical routers. This is equivalent to placing each logical NoC router at a
     * randomly selected physical router. The only difference is that an occupied physical NoC router
     * might be selected multiple times. Shuffling makes sure that each physical NoC router is evaluated
     * only once.
     */

    // check if all NoC routers have already been placed
    if (unfixed_routers.empty()) {
        return;
    }

    // Make a copy of NoC physical routers because we want to change its order
    vtr::vector<NocRouterId, NocRouter> noc_phy_routers = noc_ctx.noc_model.get_noc_routers();

    // Shuffle physical NoC routers
    vtr::RandState rand_state = seed;
    vtr::shuffle(noc_phy_routers.begin(), noc_phy_routers.end(), rand_state);

    // Get the logical block type for router
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = compressed_grids[router_block_type->index];

    // Iterate over shuffled physical routers to place logical routers
    // Since physical routers are shuffled, router placement would be random
    for (const NocRouter& phy_router : noc_phy_routers) {
        t_physical_tile_loc router_phy_loc = phy_router.get_router_physical_location();

        // Find a compatible sub-tile
        const auto& phy_type = device_ctx.grid.get_physical_type(router_phy_loc);
        const auto& compatible_sub_tiles = compressed_noc_grid.compatible_sub_tiles_for_tile.at(phy_type->index);
        int sub_tile = compatible_sub_tiles[vtr::irand((int)compatible_sub_tiles.size() - 1)];

        t_pl_loc loc(router_phy_loc, sub_tile);

        if (grid_blocks.is_sub_tile_empty(router_phy_loc, sub_tile)) {
            // Pick one of the unplaced routers
            ClusterBlockId logical_router_bid = unfixed_routers.back();
            unfixed_routers.pop_back();

            // Create a macro with a single member
            t_pl_macro_member macro_member;
            macro_member.blk_index = logical_router_bid;
            macro_member.offset = t_pl_offset(0, 0, 0, 0);
            t_pl_macro pl_macro;
            pl_macro.members.push_back(macro_member);

            bool legal = try_place_macro(pl_macro, loc, blk_loc_registry);
            if (!legal) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE, "Could not place a router cluster into an empty physical router.");
            }

            // When all router clusters are placed, stop iterating over remaining physical routers
            if (unfixed_routers.empty()) {
                break;
            }
        }
    } // end for of random router placement
}

static void noc_routers_anneal(const t_noc_opts& noc_opts,
                               BlkLocRegistry& blk_loc_registry,
                               NocCostHandler& noc_cost_handler) {
    auto& noc_ctx = g_vpr_ctx.noc();
    const auto& block_locs = blk_loc_registry.block_locs();

    // Only NoC related costs are considered
    t_placer_costs costs;

    // Initialize NoC-related costs
    costs.noc_cost_terms.aggregate_bandwidth = noc_cost_handler.comp_noc_aggregate_bandwidth_cost();
    std::tie(costs.noc_cost_terms.latency, costs.noc_cost_terms.latency_overrun) = noc_cost_handler.comp_noc_latency_cost();
    costs.noc_cost_terms.congestion = noc_cost_handler.comp_noc_congestion_cost();
    noc_cost_handler.update_noc_normalization_factors(costs);
    costs.cost = calculate_noc_cost(costs.noc_cost_terms, costs.noc_cost_norm_factors, noc_opts);

    const auto& compressed_noc_grid = get_compressed_noc_grid();
    const size_t n_noc_layers = compressed_noc_grid.get_layer_nums().size();

    /* Maximum distance in each direction that a router can travel in a move.
     * The calculation below assumes that NoC routers are organized in a square grid;
     * if it is a 3D architecture it also assumes each layer has the same number of routers.
     * Breaking that assumption is OK, but the calculation may not compute the best initial range limit in that case.
     * Each router can initially move within the entire grid with a single swap.*/
    const size_t n_physical_routers = noc_ctx.noc_model.get_noc_routers().size();
    const float max_r_lim = ceilf(sqrtf((float)n_physical_routers / (float)n_noc_layers));

    // At most, two routers are swapped
    t_pl_blocks_to_be_moved blocks_affected(2);

    // Total number of moves grows linearly with the number of logical NoC routers.
    // The constant factor was selected experimentally by running the algorithm on
    // synthetic benchmarks. NoC-related metrics did not improve after increasing
    // the constant factor above 35000.
    // Get all the router clusters and figure out how many of them exist
    const int num_router_clusters = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist().size();
    const int N_MOVES_PER_ROUTER = 50000;
    const int N_MOVES = num_router_clusters * N_MOVES_PER_ROUTER;

    const double starting_prob = 0.5;
    const double prob_step = starting_prob / N_MOVES;

    // The checkpoint stored the placement with the lowest cost.
    NoCPlacementCheckpoint checkpoint(noc_cost_handler);

    /* Algorithm overview:
     * In each iteration, one logical NoC router and a physical NoC router are selected randomly.
     * If the selected physical NoC router is occupied, two logical NoC routers are swapped.
     * If not, the selected logical NoC router is moved to the vacant physical router.
     * Then, the cost difference of this swap is computed. If the swap reduces the cost,
     * it is always accepted. Swaps that increase the cost are accepted with a
     * gradually decreasing probability. The placement with the lowest cost is saved
     * as a checkpoint. When the annealing is over, if the checkpoint has a better
     * cost than the current placement, the checkpoint is restored.
     * Range limit and the probability of accepting swaps with positive delta cost
     * decrease linearly as more swaps are evaluated. Late in the annealing,
     * NoC routers are swapped only with their neighbors as the range limit approaches 1.
     */

    // Generate and evaluate router moves
    for (int i_move = 0; i_move < N_MOVES; i_move++) {
        blocks_affected.clear_move_blocks();
        // Shrink the range limit over time
        float r_lim_decayed = 1.0f + (N_MOVES - i_move) * (max_r_lim / N_MOVES);
        e_create_move create_move_outcome = propose_router_swap(blocks_affected, r_lim_decayed, blk_loc_registry);

        if (create_move_outcome != e_create_move::ABORT) {
            blk_loc_registry.apply_move_blocks(blocks_affected);

            NocCostTerms noc_delta_c;
            noc_cost_handler.find_affected_noc_routers_and_update_noc_costs(blocks_affected, noc_delta_c);
            double delta_cost = calculate_noc_cost(noc_delta_c, costs.noc_cost_norm_factors, noc_opts);

            double prob = starting_prob - i_move * prob_step;
            bool move_accepted = accept_noc_swap(delta_cost, prob);

            if (move_accepted) {
                costs.cost += delta_cost;
                blk_loc_registry.commit_move_blocks(blocks_affected);
                noc_cost_handler.commit_noc_costs();
                costs += noc_delta_c;
                // check if the current placement is better than the stored checkpoint
                if (costs.cost < checkpoint.get_cost() || !checkpoint.is_valid()) {
                    checkpoint.save_checkpoint(costs.cost, block_locs);
                }
            } else { // The proposed move is rejected
                blk_loc_registry.revert_move_blocks(blocks_affected);
                noc_cost_handler.revert_noc_traffic_flow_routes(blocks_affected);
            }
        }
    }

    if (checkpoint.get_cost() < costs.cost) {
        checkpoint.restore_checkpoint(costs, blk_loc_registry);
    }
}

void initial_noc_placement(const t_noc_opts& noc_opts,
                           const t_placer_opts& placer_opts,
                           BlkLocRegistry& blk_loc_registry,
                           NocCostHandler& noc_cost_handler) {
	vtr::ScopedStartFinishTimer timer("Initial NoC Placement");
    auto& noc_ctx = g_vpr_ctx.noc();
    const auto& block_locs = blk_loc_registry.block_locs();

    // Get all the router clusters
    const std::vector<ClusterBlockId>& router_blk_ids = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();
    // Holds all the routers that are not fixed into a specific location by constraints
    std::vector<ClusterBlockId> unfixed_routers;

    // Check for floorplanning constraints and place constrained NoC routers
    for (const ClusterBlockId router_blk_id : router_blk_ids) {
        // The block is fixed and was placed in mark_fixed_blocks()
        if (is_block_placed(router_blk_id, block_locs)) {
            continue;
        }

        if (is_cluster_constrained(router_blk_id)) {
            place_constrained_noc_router(router_blk_id, blk_loc_registry);
        } else {
            unfixed_routers.push_back(router_blk_id);
        }
    }

    // Place unconstrained NoC routers randomly
    place_noc_routers_randomly(unfixed_routers, placer_opts.seed, blk_loc_registry);

    // populate internal data structures to maintain route, bandwidth usage, and latencies
    noc_cost_handler.initial_noc_routing({});

    // Run the simulated annealing optimizer for NoC routers
    noc_routers_anneal(noc_opts, blk_loc_registry, noc_cost_handler);

    // check if there is any cycles
    bool has_cycle = noc_cost_handler.noc_routing_has_cycle();
    if (has_cycle) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "At least one cycle was found in NoC channel dependency graph. This may cause a deadlock "
                        "when packets wait on each other in a cycle.\n");
    }
}