#include "interposer_cost_handler.h"

#include "clustered_netlist.h"
#include "device_grid.h"
#include "globals.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vtr_assert.h"

#include <algorithm>
#include <cmath>
#include <utility>

InterposerCostHandler::InterposerCostHandler(t_interposer_cost_params interposer_cost_params,
                                             std::function<const t_bb&(ClusterNetId net_id, bool use_ts)> get_net_bb)
    : interposer_cost_enabled_(interposer_cost_params.net_cost_factor > 0.)
    , interposer_cong_modeling_started_(false)
    , interposer_cong_threshold_(interposer_cost_params.cong_cost_threshold)
    , get_net_bb_(std::move(get_net_bb))
    , interposer_cost_type_(interposer_cost_params.net_cost_type)
    , two_stage_interposer_net_cost_first_stage_type_(interposer_cost_params.two_stage_net_cost_first_stage_type)
    , two_stage_interposer_net_cost_second_stage_type_(interposer_cost_params.two_stage_net_cost_second_stage_type)
    , interposer_net_cost_change_threshold_(interposer_cost_params.net_cost_change_threshold) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    const size_t num_layers = grid.get_num_layers();
    VTR_ASSERT(grid.has_interposer_cuts());
    VTR_ASSERT(interposer_cong_threshold_ >= 0. || interposer_cost_enabled_);

    if (interposer_cost_enabled_ && interposer_cost_type_ == e_interposer_net_cost_type::TWO_STAGE) {
        VTR_ASSERT(interposer_net_cost_change_threshold_ >= 0.);
        VTR_ASSERT(two_stage_interposer_net_cost_first_stage_type_ != e_interposer_net_cost_type::TWO_STAGE);
        VTR_ASSERT(two_stage_interposer_net_cost_second_stage_type_ != e_interposer_net_cost_type::TWO_STAGE);
    }
    VTR_ASSERT(get_net_bb_);

    // TODO: the class seems to support multi-layer interposer cuts,
    // but it has never been tested. Test this class with multi-layer interposer-based
    // architectures and remove this assertion.
    VTR_ASSERT(num_layers == 1);

    // Precompute 1/width and 1/height so interposer crossing cost can normalize BB spans with
    // multiplies instead of a divide per net (see get_net_interposer_cost_).
    inv_device_grid_width_ = 1.0 / static_cast<double>(grid.width());
    inv_device_grid_height_ = 1.0 / static_cast<double>(grid.height());

    const size_t num_nets = g_vpr_ctx.clustering().clb_nlist.nets().size();

    if (interposer_cost_enabled_) {
        net_interposer_cost_.resize(num_nets, -1.);
        proposed_net_interposer_cost_.resize(num_nets, -1.);

        VTR_ASSERT(std::ranges::is_sorted(grid.get_horizontal_interposer_cuts()));
        VTR_ASSERT(std::ranges::is_sorted(grid.get_vertical_interposer_cuts()));
    }

    if (interposer_cong_threshold_ > 0) {
        net_interposer_cong_cost_.resize(num_nets, -1.);
        proposed_net_interposer_cong_cost_.resize(num_nets, -1.);

        // Interposer cut locations can vary by layer, so allocate using the maximum cut count across layers.
        size_t max_h_cuts = 0, max_v_cuts = 0;
        for (size_t layer = 0; layer < num_layers; layer++) {
            max_h_cuts = std::max(max_h_cuts, grid.get_horizontal_interposer_cuts()[layer].size());
            max_v_cuts = std::max(max_v_cuts, grid.get_vertical_interposer_cuts()[layer].size());
        }
        horz_interposer_est_cong_.resize({num_layers, max_h_cuts, grid.width() + 1}, 0.);
        vert_interposer_est_cong_.resize({num_layers, max_v_cuts, grid.height() + 1}, 0.);
    }
}

bool InterposerCostHandler::has_active_cost_terms() const {
    return interposer_cost_enabled_ || interposer_cong_modeling_started_;
}

std::pair<double, double> InterposerCostHandler::recompute_costs() {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    std::pair<double, double> cost_terms;
    for (ClusterNetId net_id : clb_nlist.nets()) {
        if (clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        // Crossing and congestion costs are enabled independently; congestion starts once rlim
        // falls below a threshold (see annealer). Separate ifs update only the active term(s).
        if (interposer_cost_enabled_) {
            net_interposer_cost_[net_id] = get_net_interposer_cost_(net_id, /*use_ts=*/false);
            cost_terms.first += net_interposer_cost_[net_id];
        }

        if (interposer_cong_modeling_started_) {
            net_interposer_cong_cost_[net_id] = get_net_cube_interposer_cong_cost_(net_id, /*use_ts=*/false);
            cost_terms.second += net_interposer_cong_cost_[net_id];
        }
    }

    return cost_terms;
}

std::pair<double, double> InterposerCostHandler::total_committed_cost() const {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    std::pair<double, double> cost_terms;
    for (ClusterNetId net_id : clb_nlist.nets()) {
        if (!clb_nlist.net_is_ignored(net_id)) {
            if (interposer_cost_enabled_) {
                cost_terms.first += net_interposer_cost_[net_id];
            }

            if (interposer_cong_modeling_started_) {
                cost_terms.second += net_interposer_cong_cost_[net_id];
            }
        }
    }

    return cost_terms;
}

std::pair<double, double> InterposerCostHandler::total_proposed_cost(const std::vector<ClusterNetId>& net_ids) {
    std::pair<double, double> cost_terms_delta;
    for (ClusterNetId net_id : net_ids) {
        if (interposer_cost_enabled_) {
            proposed_net_interposer_cost_[net_id] = get_net_interposer_cost_(net_id, /*use_ts=*/true);
            cost_terms_delta.first += proposed_net_interposer_cost_[net_id] - net_interposer_cost_[net_id];
        }

        if (interposer_cong_modeling_started_) {
            proposed_net_interposer_cong_cost_[net_id] = get_net_cube_interposer_cong_cost_(net_id, /*use_ts=*/true);
            cost_terms_delta.second += proposed_net_interposer_cong_cost_[net_id] - net_interposer_cong_cost_[net_id];
        }
    }

    return cost_terms_delta;
}

void InterposerCostHandler::commit_costs(const std::vector<ClusterNetId>& net_ids) {
    for (ClusterNetId net_id : net_ids) {
        if (interposer_cost_enabled_) {
            net_interposer_cost_[net_id] = proposed_net_interposer_cost_[net_id];
            proposed_net_interposer_cost_[net_id] = -1;
        }

        if (interposer_cong_modeling_started_) {
            net_interposer_cong_cost_[net_id] = proposed_net_interposer_cong_cost_[net_id];
            proposed_net_interposer_cong_cost_[net_id] = -1;
        }
    }
}

std::pair<int, int> InterposerCostHandler::count_bb_interposer_cut_crossings_(const t_bb& bb) const {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();

    int num_horizontal_crossings = 0;
    int num_vertical_crossings = 0;

    for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {
        const std::vector<int>& layer_h_cuts = horizontal_cuts[layer];
        for (int cut_y : layer_h_cuts) {
            if (cut_y >= bb.ymin && cut_y < bb.ymax) {
                num_horizontal_crossings++;
            }
        }

        const std::vector<int>& layer_v_cuts = vertical_cuts[layer];
        for (int cut_x : layer_v_cuts) {
            if (cut_x >= bb.xmin && cut_x < bb.xmax) {
                num_vertical_crossings++;
            }
        }
    }

    return {num_horizontal_crossings, num_vertical_crossings};
}

double InterposerCostHandler::get_net_interposer_cost_(ClusterNetId net_id, bool use_ts) const {
    // This routine is O(total interposer cut lines): every cut is checked against the net BB.
    // If we support devices with many dice, consider a refactor (e.g. interval queries)
    // so cost updates do not scale linearly with cut count.

    const t_bb& bb = get_net_bb_(net_id, use_ts);
    const auto [num_horizontal_crossings, num_vertical_crossings] = count_bb_interposer_cut_crossings_(bb);
    const e_interposer_net_cost_type active_cost_type = get_active_net_cost_type_();

    if (active_cost_type == e_interposer_net_cost_type::MINIMIZE_INTERPOSER_CROSSING_BB) {
        // Weight crossings by the normalized BB span orthogonal to the cut direction:
        // - a horizontal cut spans X, so we scale by BB height / grid height
        // - a vertical cut spans Y, so we scale by BB width / grid width
        // Intuition: a tight BB that barely straddles a cut incurs less cost than a large BB that
        // spans most of the die; placement is nudged to shrink the BB so a later move can
        // pull the net off the interposer entirely.
        // TODO: compare against a constant per-crossing cost and pick whichever gives better QoR.
        const double bb_width_factor = double(bb.xmax - bb.xmin + 1) * inv_device_grid_width_;
        const double bb_height_factor = double(bb.ymax - bb.ymin + 1) * inv_device_grid_height_;

        double cost = num_horizontal_crossings * bb_height_factor + num_vertical_crossings * bb_width_factor;
        return cost;
    } else {
        VTR_ASSERT_SAFE(active_cost_type == e_interposer_net_cost_type::INTERPOSER_WIRE_AWARE_CROSSING_BB);

        if (num_horizontal_crossings == 0 && num_vertical_crossings == 0) {
            return 0;
        }

        const DeviceGrid& grid = g_vpr_ctx.device().grid;
        const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
        const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();

        double bb_width_factor = 0;
        double bb_height_factor = 0;

        for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {
            const std::vector<int>& layer_h_cuts = horizontal_cuts[layer];
            for (size_t i_cut = 0; i_cut < layer_h_cuts.size(); i_cut++) {
                int cut_y = layer_h_cuts[i_cut];
                if (cut_y >= bb.ymin && cut_y < bb.ymax) {
                    bb_height_factor += std::abs(g_vpr_ctx.device().horz_interposer_cut_min_seg_length[layer][i_cut] - (bb.ymax - bb.ymin + 1)) * inv_device_grid_height_;
                }
            }

            const std::vector<int>& layer_v_cuts = vertical_cuts[layer];
            for (size_t i_cut = 0; i_cut < layer_v_cuts.size(); i_cut++) {
                int cut_x = layer_v_cuts[i_cut];
                if (cut_x >= bb.xmin && cut_x < bb.xmax) {
                    bb_width_factor += std::abs(g_vpr_ctx.device().vert_interposer_cut_min_seg_length[layer][i_cut] - (bb.xmax - bb.xmin + 1)) * inv_device_grid_width_;
                }
            }
        }

        double cost = num_horizontal_crossings * bb_height_factor + num_vertical_crossings * bb_width_factor;
        return cost;
    }
}

bool InterposerCostHandler::try_change_interposer_cost_model(double current_cost) {
    if (interposer_cost_type_ != e_interposer_net_cost_type::TWO_STAGE || interposer_cost_stage_ != e_interposer_cost_stage::FIRST) {
        return false;
    }

    interposer_net_cost_history_.push_back(current_cost);
    if (!interposer_net_cost_history_.full()) {
        return false;
    }

    double avg_interposer_net_cost = 0.;
    for (double int_cost : interposer_net_cost_history_) {
        avg_interposer_net_cost += int_cost;
    }
    avg_interposer_net_cost /= interposer_net_cost_history_.size();

    double max_percent_diff_from_avg = 0.;
    if (avg_interposer_net_cost > 0.) {
        for (double int_cost : interposer_net_cost_history_) {
            double percent_diff_from_avg = std::fabs(int_cost - avg_interposer_net_cost) / avg_interposer_net_cost;
            max_percent_diff_from_avg = std::max(max_percent_diff_from_avg, percent_diff_from_avg);
        }
    }

    if (max_percent_diff_from_avg < interposer_net_cost_change_threshold_) {
        interposer_cost_stage_ = e_interposer_cost_stage::SECOND;
        return true;
    }

    return false;
}

e_interposer_net_cost_type InterposerCostHandler::get_active_net_cost_type_() const {
    if (interposer_cost_type_ == e_interposer_net_cost_type::TWO_STAGE) {
        return interposer_cost_stage_ == e_interposer_cost_stage::FIRST ? two_stage_interposer_net_cost_first_stage_type_ : two_stage_interposer_net_cost_second_stage_type_;
    }

    return interposer_cost_type_;
}

double InterposerCostHandler::get_net_cube_interposer_cong_cost_(ClusterNetId net_id, bool use_ts) const {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const t_bb& bb = get_net_bb_(net_id, use_ts);

    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();

    const float threshold = interposer_cong_threshold_;
    double cost = 0.;

    for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {
        // Horizontal cuts: net crosses if cut_y is in [bb.ymin, bb.ymax).
        // Bounding box for that cut is x in [bb.xmin, bb.xmax]; congestion is stored as prefix sum along x.
        const std::vector<int>& layer_h_cuts = horizontal_cuts[layer];
        for (size_t i_cut = 0; i_cut < layer_h_cuts.size(); i_cut++) {
            int cut_y = layer_h_cuts[i_cut];
            if (cut_y >= bb.ymin && cut_y < bb.ymax) {
                int count_x = bb.xmax - bb.xmin + 1;
                double sum = horz_interposer_est_cong_[layer][i_cut][bb.xmax + 1] - horz_interposer_est_cong_[layer][i_cut][bb.xmin];
                double avg = sum / count_x;

                cost += std::max(0.0, avg - threshold);
            }
        }

        // Vertical cuts: net crosses if cut_x is in [bb.xmin, bb.xmax).
        // Bounding box for that cut is y in [bb.ymin, bb.ymax]; congestion is stored as prefix sum along y.
        const std::vector<int>& layer_v_cuts = vertical_cuts[layer];
        for (size_t i_cut = 0; i_cut < layer_v_cuts.size(); i_cut++) {
            int cut_x = layer_v_cuts[i_cut];
            if (cut_x >= bb.xmin && cut_x < bb.xmax) {
                int count_y = bb.ymax - bb.ymin + 1;
                double sum = vert_interposer_est_cong_[layer][i_cut][bb.ymax + 1] - vert_interposer_est_cong_[layer][i_cut][bb.ymin];
                double avg = sum / count_y;
                cost += std::max(0.0, avg - threshold);
            }
        }
    }

    return cost;
}

int InterposerCostHandler::get_num_nets_crossing_interposer_cuts() const {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    int num_nets_crossing_interposer_cuts = 0;

    for (ClusterNetId net_id : clb_nlist.nets()) {
        if (!clb_nlist.net_is_ignored(net_id)) {
            const auto [num_h_cuts, num_v_cuts] = count_bb_interposer_cut_crossings_(get_net_bb_(net_id, /*use_ts=*/false));
            if (num_h_cuts > 0 || num_v_cuts > 0) {
                num_nets_crossing_interposer_cuts++;
            }
        }
    }

    return num_nets_crossing_interposer_cuts;
}

double InterposerCostHandler::compute_interposer_est_cong(bool compute_congestion_cost) {
    // Predict interposer channel utilization along each cut and store it for fast range queries, then optionally
    // refresh per-net interposer congestion for all nets.
    //
    // (1) For every net, spread a unit of wire demand across coordinates along each cut that lies inside the net BB.
    // (2) Divide by precomputed interposer capacity at each coordinate to get demand/capacity ratio.
    // (3) Convert to prefix sums along the cut line so get_net_cube_interposer_cong_cost_ can average
    // utilization over a net span in O(1).
    // (4) If requested, fill net_interposer_cong_cost_ per net and return the total across the design.
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const ClusteredNetlist& clb_nlist = cluster_ctx.clb_nlist;

    const size_t num_layers = grid.get_num_layers();
    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();

    horz_interposer_est_cong_.fill(0.);
    vert_interposer_est_cong_.fill(0.);

    // (1) Consider one unit of wire demand per net for each cut; split it evenly along the BB extent parallel to each crossed cut
    for (ClusterNetId net_id : clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        const t_bb& bb = get_net_bb_(net_id, /*use_ts=*/false);

        for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {

            const std::vector<int>& layer_h_cuts = horizontal_cuts[layer];
            for (size_t i_cut = 0; i_cut < layer_h_cuts.size(); i_cut++) {
                int cut_y = layer_h_cuts[i_cut];
                if (cut_y >= bb.ymin && cut_y < bb.ymax) {
                    const double contribution = 1.0 / (bb.xmax - bb.xmin + 1);
                    for (int x = bb.xmin; x <= bb.xmax; x++) {
                        horz_interposer_est_cong_[layer][i_cut][x] += contribution;
                    }
                }
            }

            const std::vector<int>& layer_v_cuts = vertical_cuts[layer];
            for (size_t i_cut = 0; i_cut < layer_v_cuts.size(); i_cut++) {
                int cut_x = layer_v_cuts[i_cut];
                if (cut_x >= bb.xmin && cut_x < bb.xmax) {
                    const double contribution = 1.0 / (bb.ymax - bb.ymin + 1);
                    for (int y = bb.ymin; y <= bb.ymax; y++) {
                        vert_interposer_est_cong_[layer][i_cut][y] += contribution;
                    }
                }
            }
        }
    }

    const vtr::NdMatrix<int, 3>& horz_interposer_capacity = device_ctx.horz_interposer_capacity_;
    const vtr::NdMatrix<int, 3>& vert_interposer_capacity = device_ctx.vert_interposer_capacity_;
    // (2) Turn accumulated demand into utilization: divide by per-coordinate capacity from the RR graph
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t i_cut = 0; i_cut < horizontal_cuts[layer].size(); i_cut++) {
            for (size_t x = 0; x < grid_width; x++) {
                int avail = horz_interposer_capacity[layer][i_cut][x];
                if (avail > 0) {
                    horz_interposer_est_cong_[layer][i_cut][x] /= avail;
                } else {
                    horz_interposer_est_cong_[layer][i_cut][x] = 1.;
                }
            }
        }
    }
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t i_cut = 0; i_cut < vertical_cuts[layer].size(); i_cut++) {
            for (size_t y = 0; y < grid_height; y++) {
                int avail = vert_interposer_capacity[layer][i_cut][y];
                if (avail > 0) {
                    vert_interposer_est_cong_[layer][i_cut][y] /= avail;
                } else {
                    vert_interposer_est_cong_[layer][i_cut][y] = 1.;
                }
            }
        }
    }

    // (3) Prefix sums along each cut: after this, position i holds sum(util[0..i-1]) and index N holds total;
    // range sum over [a,b] is prefix[b+1] - prefix[a] (see get_net_cube_interposer_cong_cost_).
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t i_cut = 0; i_cut < horizontal_cuts[layer].size(); i_cut++) {
            double running_sum = 0.;
            for (size_t x = 0; x < grid_width; ++x) {
                double val = horz_interposer_est_cong_[layer][i_cut][x];
                horz_interposer_est_cong_[layer][i_cut][x] = running_sum;
                running_sum += val;
            }

            horz_interposer_est_cong_[layer][i_cut][grid_width] = running_sum;
        }
    }

    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t i_cut = 0; i_cut < vertical_cuts[layer].size(); i_cut++) {
            double running_sum = 0.;
            for (size_t y = 0; y < grid_height; ++y) {
                double val = vert_interposer_est_cong_[layer][i_cut][y];
                vert_interposer_est_cong_[layer][i_cut][y] = running_sum;
                running_sum += val;
            }

            vert_interposer_est_cong_[layer][i_cut][grid_height] = running_sum;
        }
    }

    interposer_cong_modeling_started_ = true;

    double total_interposer_cong_cost = 0.;
    // (4) Optionally compute each net's congestion penalty and return the total.
    if (compute_congestion_cost) {
        for (ClusterNetId net_id : clb_nlist.nets()) {
            if (clb_nlist.net_is_ignored(net_id)) {
                continue;
            }

            net_interposer_cong_cost_[net_id] = get_net_cube_interposer_cong_cost_(net_id, /*use_ts=*/false);
            total_interposer_cong_cost += net_interposer_cong_cost_[net_id];
        }
    }
    return total_interposer_cong_cost;
}
