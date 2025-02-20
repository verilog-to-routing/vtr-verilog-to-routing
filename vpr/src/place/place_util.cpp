/**
 * @file place_util.cpp
 * @brief Definitions of structure methods and routines declared in place_util.h.
 *        These are mainly utility functions used by the placer.
 */

#include "place_util.h"
#include "globals.h"
#include "draw_global.h"
#include "physical_types_util.h"
#include "place_constraints.h"
#include "noc_place_utils.h"

/**
 * @brief Initialize `grid_blocks`, the inverse structure of `block_locs`.
 *
 * The container at each grid block location should have a length equal to the
 * subtile capacity of that block. Unused subtile would be marked ClusterBlockId::INVALID().
 */
static GridBlock init_grid_blocks();

void init_placement_context(BlkLocRegistry& blk_loc_registry) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto& block_locs = blk_loc_registry.mutable_block_locs();
    auto& grid_blocks = blk_loc_registry.mutable_grid_blocks();

    /* Initialize the lookup of CLB block positions */
    block_locs.clear();
    block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    /* Initialize the reverse lookup of CLB block positions */
    grid_blocks = init_grid_blocks();
}

static GridBlock init_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();
    int num_layers = device_ctx.grid.get_num_layers();

    /* Structure should have the same dimensions as the grid. */
    auto grid_blocks = GridBlock(device_ctx.grid.width(), device_ctx.grid.height(), num_layers);

    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        for (int x = 0; x < (int)device_ctx.grid.width(); ++x) {
            for (int y = 0; y < (int)device_ctx.grid.height(); ++y) {
                auto type = device_ctx.grid.get_physical_type({x, y, layer_num});
                grid_blocks.initialized_grid_block_at_location({x, y, layer_num}, type->capacity);
            }
        }
    }

    return grid_blocks;
}

void t_placer_costs::update_norm_factors() {
    if (place_algorithm.is_timing_driven()) {
        bb_cost_norm = 1 / bb_cost;
        //Prevent the norm factor from going to infinity
        timing_cost_norm = std::min(1 / timing_cost, MAX_INV_TIMING_COST);
    } else {
        VTR_ASSERT_SAFE(place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE);
        bb_cost_norm = 1 / bb_cost; //Updating the normalization factor in bounding box mode since the cost in this mode is determined after normalizing the wirelength cost
    }

    if (noc_enabled) {
        NocCostHandler::update_noc_normalization_factors(*this);
    }
}

double t_placer_costs::get_total_cost(const t_placer_opts& placer_opts, const t_noc_opts& noc_opts) {
    double total_cost = 0.0;

    if (placer_opts.place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE) {
        // in bounding box mode we only care about wirelength
        total_cost = bb_cost * bb_cost_norm;
    } else if (placer_opts.place_algorithm.is_timing_driven()) {
        // in timing mode we include both wirelength and timing costs
        total_cost = (1 - placer_opts.timing_tradeoff) * (bb_cost * bb_cost_norm) + (placer_opts.timing_tradeoff) * (timing_cost * timing_cost_norm);
    }

    if (noc_opts.noc) {
        // in noc mode we include noc aggregate bandwidth, noc latency, and noc congestion
        total_cost += calculate_noc_cost(noc_cost_terms, noc_cost_norm_factors, noc_opts);
    }

    return total_cost;
}

t_placer_costs& t_placer_costs::operator+=(const NocCostTerms& noc_delta_cost) {
    noc_cost_terms += noc_delta_cost;

    return *this;
}

int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    float device_size = device_ctx.grid.width() * device_ctx.grid.height();
    size_t num_blocks = cluster_ctx.clb_nlist.blocks().size();

    int move_lim;
    if (placer_opts.effort_scaling == e_place_effort_scaling::CIRCUIT) {
        move_lim = int(annealing_sched.inner_num * pow(num_blocks, 1.3333));
    } else {
        VTR_ASSERT(placer_opts.effort_scaling == e_place_effort_scaling::DEVICE_CIRCUIT);
        move_lim = int(annealing_sched.inner_num * pow(device_size, 2. / 3.) * pow(num_blocks, 2. / 3.));
    }

    /* Avoid having a non-positive move_lim */
    move_lim = std::max(move_lim, 1);

    VTR_LOG("Moves per temperature: %d\n", move_lim);

    return move_lim;
}

///@brief Clear all data fields.
void t_placer_statistics::reset() {
    av_cost = 0.;
    av_bb_cost = 0.;
    av_timing_cost = 0.;
    sum_of_squares = 0.;
    success_sum = 0;
    success_rate = 0.;
    std_dev = 0.;
}

///@brief Calculate placer success rate and cost std_dev for this iteration.
void t_placer_statistics::single_swap_update(const t_placer_costs& costs) {
    success_sum++;
    av_cost += costs.cost;
    av_bb_cost += costs.bb_cost;
    av_timing_cost += costs.timing_cost;
    sum_of_squares += (costs.cost) * (costs.cost);
}

///@brief Update stats when a single swap move has been accepted.
void t_placer_statistics::calc_iteration_stats(const t_placer_costs& costs, int move_lim) {
    if (success_sum == 0) {
        av_cost = costs.cost;
        av_bb_cost = costs.bb_cost;
        av_timing_cost = costs.timing_cost;
    } else {
        av_cost /= success_sum;
        av_bb_cost /= success_sum;
        av_timing_cost /= success_sum;
    }
    success_rate = success_sum / float(move_lim);
    std_dev = get_std_dev(success_sum, sum_of_squares, av_cost);
}

double get_std_dev(int n, double sum_x_squared, double av_x) {
    double std_dev;
    if (n <= 1) {
        std_dev = 0.;
    } else {
        std_dev = (sum_x_squared - n * av_x * av_x) / (double)(n - 1);
    }

    /* Very small variances sometimes round negative. */
    return (std_dev > 0.) ? sqrt(std_dev) : 0.;
}


void alloc_and_load_legal_placement_locations(std::vector<std::vector<std::vector<t_pl_loc>>>& legal_pos) {
    auto& device_ctx = g_vpr_ctx.device();

    //alloc the legal placement positions
    int num_tile_types = device_ctx.physical_tile_types.size();
    legal_pos.resize(num_tile_types);

    for (const auto& type : device_ctx.physical_tile_types) {
        legal_pos[type.index].resize(type.sub_tiles.size());
    }

    //load the legal placement positions
    for (int layer_num = 0; layer_num < device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                auto tile = device_ctx.grid.get_physical_type({i, j, layer_num});

                for (const auto& sub_tile : tile->sub_tiles) {
                    auto capacity = sub_tile.capacity;

                    for (int k = 0; k < capacity.total(); k++) {
                        // If this is the anchor position of a block, add it to the legal_pos.
                        // Otherwise, don't, so large blocks aren't added multiple times.
                        if (device_ctx.grid.get_width_offset({i, j, layer_num}) == 0 && device_ctx.grid.get_height_offset({i, j, layer_num}) == 0) {
                            int itype = tile->index;
                            int isub_tile = sub_tile.index;
                            t_pl_loc temp_loc;
                            temp_loc.x = i;
                            temp_loc.y = j;
                            temp_loc.sub_tile = k + capacity.low;
                            temp_loc.layer = layer_num;
                            legal_pos[itype][isub_tile].push_back(temp_loc);
                        }
                    }
                }
            }
        }
    }
    //avoid any memory waste
    legal_pos.shrink_to_fit();
}

bool macro_can_be_placed(const t_pl_macro& pl_macro,
                         const t_pl_loc& head_pos,
                         bool check_all_legality,
                         const BlkLocRegistry& blk_loc_registry) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& grid_blocks = blk_loc_registry.grid_blocks();

    //Get block type of head member
    ClusterBlockId blk_id = pl_macro.members[0].blk_index;
    auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

    // Every macro can be placed until proven otherwise
    bool mac_can_be_placed = true;

    // Check whether all the members can be placed
    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

        //Check that the member location is on the grid
        if (!is_loc_on_chip({member_pos.x, member_pos.y, member_pos.layer})) {
            mac_can_be_placed = false;
            break;
        }

        /*
         * analytical placement approach do not need to make sure whether location could accommodate more blocks
         * since overused locations will be spread by legalizer afterward.
         * floorplan constraint is not supported by analytical placement yet, 
         * hence, if macro_can_be_placed is called from analytical placer, no further actions are required. 
         */
        if (check_all_legality) {
            continue;
        }

        //Check whether macro contains blocks with floorplan constraints
        bool macro_constrained = is_macro_constrained(pl_macro);

        /*
         * If the macro is constrained, check that the head member is in a legal position from
         * a floorplanning perspective. It is enough to do this check for the head member alone,
         * because constraints propagation was performed to calculate smallest floorplan region for the head
         * macro, based on the constraints on all of the blocks in the macro. So, if the head macro is in a
         * legal floorplan location, all other blocks in the macro will be as well.
         */
        if (macro_constrained && imember == 0) {
            bool member_loc_good = cluster_floorplanning_legal(pl_macro.members[imember].blk_index, member_pos);
            if (!member_loc_good) {
                mac_can_be_placed = false;
                break;
            }
        }

        // Check whether the location could accept block of this type
        // Then check whether the location could still accommodate more blocks
        // Also check whether the member position is valid, and the member_z is allowed at that location on the grid
        if (member_pos.x < int(device_ctx.grid.width()) && member_pos.y < int(device_ctx.grid.height())
            && is_tile_compatible(device_ctx.grid.get_physical_type({member_pos.x, member_pos.y, member_pos.layer}), block_type)
            && grid_blocks.block_at_location(member_pos) == ClusterBlockId::INVALID()) {
            // Can still accommodate blocks here, check the next position
            continue;
        } else {
            // Cant be placed here - skip to the next try
            mac_can_be_placed = false;
            break;
        }
    }

    return mac_can_be_placed;
}

NocCostTerms::NocCostTerms(double agg_bw, double lat, double lat_overrun, double congest)
    : aggregate_bandwidth(agg_bw)
    , latency(lat)
    , latency_overrun(lat_overrun)
    , congestion(congest) {}

NocCostTerms::NocCostTerms()
    : aggregate_bandwidth(0)
    , latency(0)
    , latency_overrun(0)
    , congestion(0) {}

NocCostTerms& NocCostTerms::operator+=(const NocCostTerms& noc_delta_cost) {
    aggregate_bandwidth += noc_delta_cost.aggregate_bandwidth;
    latency += noc_delta_cost.latency;
    latency_overrun += noc_delta_cost.latency_overrun;
    congestion += noc_delta_cost.congestion;

    return *this;
}
