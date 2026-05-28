#include "direct_connection_legality.h"

#include <cstdlib>

#include "clb2clb_directs.h"
#include "globals.h"
#include "vtr_assert.h"

namespace {

/// Returns the pin_count_in_cluster of a physical tile pin on the given
/// logical block, or -1 if the block doesn't expose that pin.
int tile_pin_to_pb_pin_id(t_physical_tile_type_ptr tile,
                          t_logical_block_type_ptr lb,
                          int phys_pin) {
    auto outer = tile->on_tile_pin_num_to_pb_pin.find(phys_pin);
    if (outer == tile->on_tile_pin_num_to_pb_pin.end()) return -1;
    auto inner = outer->second.find(lb);
    if (inner == outer->second.end()) return -1;
    const t_pb_graph_pin* pb_pin = inner->second;
    if (pb_pin == nullptr) return -1;
    return pb_pin->pin_count_in_cluster;
}

} // namespace

DirectConnectionLegality::DirectConnectionLegality(const std::vector<t_direct_inf>& directs) {
    const int delayless_switch = g_vpr_ctx.device().delayless_switch_idx;
    std::vector<t_clb_to_clb_directs> clb_to_clb_directs =
        alloc_and_load_clb_to_clb_directs(directs, delayless_switch);

    forward_by_from_lb_.resize(g_vpr_ctx.device().logical_block_types.size());

    for (const t_clb_to_clb_directs& cd : clb_to_clb_directs) {
        if (cd.from_clb_type == nullptr || cd.to_clb_type == nullptr) continue;

        // Pin ranges can be given in either direction; step = -1 means reversed.
        const int from_size = std::abs(cd.from_clb_pin_end_index - cd.from_clb_pin_start_index) + 1;
        const int to_size = std::abs(cd.to_clb_pin_end_index - cd.to_clb_pin_start_index) + 1;
        if (from_size != to_size) continue;
        const int from_step = (cd.from_clb_pin_start_index <= cd.from_clb_pin_end_index) ? 1 : -1;
        const int to_step = (cd.to_clb_pin_start_index <= cd.to_clb_pin_end_index) ? 1 : -1;

        // LIMITATION: only the first equivalent logical block on each side is
        // considered. Multi-equivalent_sites is not handled.
        if (cd.from_clb_type->sub_tiles.empty() || cd.to_clb_type->sub_tiles.empty()) continue;
        const auto& from_eq_sites = cd.from_clb_type->sub_tiles.front().equivalent_sites;
        const auto& to_eq_sites = cd.to_clb_type->sub_tiles.front().equivalent_sites;
        if (from_eq_sites.empty() || to_eq_sites.empty()) continue;
        t_logical_block_type_ptr from_lb = from_eq_sites.front();
        t_logical_block_type_ptr to_lb = to_eq_sites.front();

        for (int k = 0; k < from_size; ++k) {
            int from_phys = cd.from_clb_pin_start_index + k * from_step;
            int to_phys = cd.to_clb_pin_start_index + k * to_step;

            int from_pin_id = tile_pin_to_pb_pin_id(cd.from_clb_type, from_lb, from_phys);
            if (from_pin_id < 0) continue;
            int to_pin_id = tile_pin_to_pb_pin_id(cd.to_clb_type, to_lb, to_phys);
            if (to_pin_id < 0) continue;

            forward_by_from_lb_[from_lb->index][from_pin_id]
                .emplace(to_lb->index, to_pin_id);
        }
    }
}

bool DirectConnectionLegality::is_direct_legal(t_logical_block_type_ptr from_lb, int from_top_pin, t_logical_block_type_ptr to_lb, int to_top_pin) const {
    if (from_lb == nullptr || from_lb->index < 0
        || static_cast<size_t>(from_lb->index) >= forward_by_from_lb_.size()) {
        return false;
    }
    const auto& pin_map = forward_by_from_lb_[from_lb->index];
    auto pin_it = pin_map.find(from_top_pin);
    if (pin_it == pin_map.end()) return false;
    return pin_it->second.count({to_lb->index, to_top_pin}) != 0;
}
