#include "router_lookahead_separable.h"

#include "globals.h"
#include "vpr_error.h"
#include "vtr_time.h"

/**
 * @note This lookahead is currently a skeleton: the class compiles and can be
 *       selected via --router_lookahead separable, but none of its methods are
 *       implemented yet.
 */
#define NOT_IMPLEMENTED_ERROR(func_name) \
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "%s is not implemented yet", func_name)

SeparableLookahead::SeparableLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier)
    : det_routing_arch_(det_routing_arch)
    , is_flat_(is_flat)
    , route_verbosity_(route_verbosity)
    , device_model_warnings_(device_model_warnings)
    , interposer_base_cost_multiplier_(interposer_base_cost_multiplier) {
    has_interposer_cuts_ = g_vpr_ctx.device().grid.has_interposer_cuts();
}

float SeparableLookahead::get_expected_cost(RRNodeId /*current_node*/, RRNodeId /*target_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::get_expected_cost");
    return 0.f;
}

float SeparableLookahead::get_expected_cost_flat_router(RRNodeId /*current_node*/, RRNodeId /*target_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::get_expected_cost_flat_router");
    return 0.f;
}

std::pair<float, float> SeparableLookahead::get_expected_delay_and_cong(RRNodeId /*from_node*/, RRNodeId /*to_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::get_expected_delay_and_cong");
    return std::make_pair(0.f, 0.f);
}

void SeparableLookahead::compute(const std::vector<t_segment_inf>& /*segment_inf*/) {

}

void SeparableLookahead::compute_intra_tile() {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::compute_intra_tile");
}

void SeparableLookahead::read(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::read");
}

void SeparableLookahead::read_intra_cluster(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::read_intra_cluster");
}

void SeparableLookahead::write(const std::string& /*file_name*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::write");
}

void SeparableLookahead::write_intra_cluster(const std::string& /*file*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::write_intra_cluster");
}

float SeparableLookahead::get_opin_distance_min_delay(int /*physical_tile_idx*/, int /*from_layer*/, int /*to_layer*/, int /*dx*/, int /*dy*/) const {
    NOT_IMPLEMENTED_ERROR("SeparableLookahead::get_opin_distance_min_delay");
    return 0.f;
}

void read_router_lookahead_separable(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("read_router_lookahead_separable");
}

void write_router_lookahead_separable(const std::string& /*file*/) {
    NOT_IMPLEMENTED_ERROR("write_router_lookahead_separable");
}

#undef NOT_IMPLEMENTED_ERROR
