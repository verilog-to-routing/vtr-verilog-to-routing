/**
 * @file
 * @brief Quadratic centroid affinity-spring objective term (see affinity_spring_term.h).
 */

#include "affinity_spring_term.h"

#include "partial_placement.h"
#include "preconditioner_math.h"
#include "vtr_assert.h"

using vtr::ap::affinity_spring_curvature;

AffinitySpringTerm::AffinitySpringTerm(const APNetlist& ap_netlist,
                                       double io_pair_attraction_weight,
                                       double pack_pattern_weight)
    : io_pair_attraction_weight_(io_pair_attraction_weight)
    , pack_pattern_weight_(pack_pattern_weight)
    , moveable_(ap_netlist.blocks().size(), false) {
    for (APBlockId blk_id : ap_netlist.blocks())
        moveable_[blk_id] = ap_netlist.block_mobility(blk_id) == APBlockMobility::MOVEABLE;
}

double AffinitySpringTerm::kernel_weight_(e_affinity_kind kind) const {
    switch (kind) {
        case e_affinity_kind::IO_PAIR:
            // Legacy I/O pair spring used grad += W * dx (no 1/n). Pack-math kernel
            // uses W_kernel / n; for n=2 set W_kernel = 2W to preserve strength.
            return 2. * io_pair_attraction_weight_;
        case e_affinity_kind::PACK_PATTERN:
            return pack_pattern_weight_;
        default:
            VTR_ASSERT_MSG(false, "Unhandled affinity kind");
            return 0.;
    }
}

double AffinitySpringTerm::evaluate(const PartialPlacement& p_placement,
                                    std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (groups_.empty())
        return 0.;

    double weighted_penalty = 0.;
    for (const AffinityGroup& group : groups_) {
        VTR_ASSERT_SAFE(group.blocks.size() >= 2);
        double weight = kernel_weight_(group.kind);
        if (weight == 0.)
            continue;

        double centroid_x = 0.;
        double centroid_y = 0.;
        for (APBlockId blk_id : group.blocks) {
            centroid_x += p_placement.block_x_locs[blk_id];
            centroid_y += p_placement.block_y_locs[blk_id];
        }

        double inv_group_size = 1. / static_cast<double>(group.blocks.size());
        centroid_x *= inv_group_size;
        centroid_y *= inv_group_size;

        double unweighted = 0.;
        for (APBlockId blk_id : group.blocks) {
            double dx = p_placement.block_x_locs[blk_id] - centroid_x;
            double dy = p_placement.block_y_locs[blk_id] - centroid_y;
            unweighted += 0.5 * inv_group_size * (dx * dx + dy * dy);
            if (grad && moveable_[blk_id]) {
                grad->get().dx[blk_id] += weight * inv_group_size * dx;
                grad->get().dy[blk_id] += weight * inv_group_size * dy;
            }
        }
        weighted_penalty += weight * unweighted;
    }
    return weighted_penalty;
}

void AffinitySpringTerm::add_curvature(vtr::vector<APBlockId, double>& diagonal) const {
    for (const AffinityGroup& group : groups_) {
        double curvature = affinity_spring_curvature(kernel_weight_(group.kind),
                                                     group.blocks.size());
        if (curvature == 0.)
            continue;
        for (APBlockId blk_id : group.blocks)
            diagonal[blk_id] += curvature;
    }
}
