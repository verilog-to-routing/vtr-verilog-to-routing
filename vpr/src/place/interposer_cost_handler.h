#pragma once
/**
 * @file interposer_cost_handler.h
 * @brief Tracks placement cost terms associated with interposer.
 */

#include "place_util.h"
#include "vpr_types.h"
#include "vtr_circular_buffer.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"

#include <functional>
#include <utility>
#include <vector>

enum class e_interposer_cost_stage {
  FIRST,
  SECOND
};

class InterposerCostHandler {
  public:
    InterposerCostHandler() = delete;
    InterposerCostHandler(const InterposerCostHandler&) = delete;
    InterposerCostHandler& operator=(const InterposerCostHandler&) = delete;
    InterposerCostHandler(InterposerCostHandler&&) = delete;
    InterposerCostHandler& operator=(InterposerCostHandler&&) = delete;

    InterposerCostHandler(bool interposer_cost_enabled,
                          double interposer_cong_threshold,
                          e_interposer_net_cost_type interposer_net_cost_type,
                          e_interposer_net_cost_type two_stage_interposer_net_cost_first_stage_type,
                          e_interposer_net_cost_type two_stage_interposer_net_cost_second_stage_type,
                          double interposer_net_cost_change_threshold,
                          std::function<const t_bb&(ClusterNetId net_id, bool use_ts)> get_net_bb);

    /// @brief Returns true when at least one interposer cost term is activated.
    bool has_active_cost_terms() const;

    /// @brief Recomputes and stores interposer cost terms across all non-ignored nets.
    /// @return {interposer crossing cost, interposer congestion cost}; disabled terms are zero.
    std::pair<double, double> recompute_costs();
    /// @brief Returns total committed interposer costs across all non-ignored nets.
    /// @return {interposer crossing cost, interposer congestion cost}.
    std::pair<double, double> total_committed_cost() const;
    /// @brief Computes total proposed interposer cost deltas for a set of affected nets.
    /// @return {interposer crossing cost delta, interposer congestion cost delta}.
    std::pair<double, double> total_proposed_cost(const std::vector<ClusterNetId>& net_ids);
    /// @brief Commits proposed interposer costs for a set of affected nets.
    void commit_costs(const std::vector<ClusterNetId>& net_ids);

    /// @brief Get the number of nets crossing interposer cuts.
    int get_num_nets_crossing_interposer_cuts() const;

    /// @brief Compute estimated interposer congestion and optionally the total interposer congestion cost.
    /// @param compute_congestion_cost When true, compute total interposer congestion cost (sum over all nets).
    /// @return Total interposer congestion cost when compute_congestion_cost is true, otherwise 0.
    double compute_interposer_est_cong(bool compute_congestion_cost = true);

    /// @brief Try switching to a more detailed interposer net cost model based on recent costs.
    /// @return True if the interposer net cost model was changed.
    bool try_change_interposer_cost_model(double current_cost);

    e_interposer_net_cost_type get_net_cost_type() const { return cost_type_; }

  private:
    double get_net_interposer_cost_(ClusterNetId net_id, bool use_ts) const;
    double get_net_cube_interposer_cong_cost_(ClusterNetId net_id, bool use_ts) const;
    std::pair<int, int> count_bb_interposer_cut_crossings_(const t_bb& bb) const;
    e_interposer_net_cost_type get_active_net_cost_type_() const;

  private:
    /// Enables interposer crossing cost term.
    bool interposer_cost_enabled_;
    /// Indicates whether interposer congestion modeling has been initialized/activated.
    bool interposer_cong_modeling_started_;
    double interposer_cong_threshold_;
    /// Reciprocals of device grid width and height for normalizing bounding-box spans.
    double inv_device_grid_width_;
    double inv_device_grid_height_;
    /// Fetches committed/proposed BB state owned by NetCostHandler.
    std::function<const t_bb&(ClusterNetId net_id, bool use_ts)> get_net_bb_;

    /// @brief Estimated interposer cut utilization, stored as 1D prefix sums for O(1) queries.
    /// Stores (estimated demand / capacity) per cut segment, then converted to prefix sums.
    /// Indexing is [layer][i_cut][coord_prefix]; the last dimension has length (N+1) (prefix[0]=0),
    /// so sum over ([a,b]) is prefix[b+1] - prefix[a].
    vtr::NdMatrix<double, 3> horz_interposer_est_cong_;
    vtr::NdMatrix<double, 3> vert_interposer_est_cong_;

    /// Per-net interposer crossing cost (and temporary value during move evaluation).
    vtr::vector<ClusterNetId, double> net_interposer_cost_, proposed_net_interposer_cost_;
    /// Per-net interposer congestion cost (and temporary value during move evaluation).
    vtr::vector<ClusterNetId, double> net_interposer_cong_cost_, proposed_net_interposer_cong_cost_;

    e_interposer_net_cost_type cost_type_ = e_interposer_net_cost_type::MINIMIZE_INTERPOSER_CROSSING_BB;
    
    /// Current stage of interposet net cost when using two-stage cost mode
    e_interposer_cost_stage cost_stage_ = e_interposer_cost_stage::FIRST;
    /// Concrete interposer net cost type used during the first stage of two-stage mode.
    const e_interposer_net_cost_type two_stage_interposer_net_cost_first_stage_type_;
    /// Concrete interposer net cost type used during the second stage of two-stage mode.
    const e_interposer_net_cost_type two_stage_interposer_net_cost_second_stage_type_;
    /// Threshold used to switch from first to second stage in two-stage mode.
    double interposer_net_cost_change_threshold_;
    /// Recent interposer net costs used to decide when to switch cost models.
    vtr::circular_buffer<double> interposer_net_cost_history_ = vtr::circular_buffer<double>(10);
};
