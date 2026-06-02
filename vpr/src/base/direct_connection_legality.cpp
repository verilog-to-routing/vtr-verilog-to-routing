#include "direct_connection_legality.h"

#include <cstdlib>
#include <tuple>

#include "clb2clb_directs.h"
#include "physical_types_util.h"
#include "vpr_context.h"
#include "vpr_utils.h"

namespace {

/** @brief Returns the pin_count_in_cluster of a sub-tile-relative pin on the given
 *  logical block, or -1 if the block doesn't expose that pin. */
int tile_pin_to_pb_pin_id(t_physical_tile_type_ptr tile,
                          t_logical_block_type_ptr lb,
                          const t_sub_tile& sub_tile,
                          int sub_tile_relative_pin) {
    auto lb_it = tile->tile_block_pin_directs_map.find(lb->index);
    if (lb_it == tile->tile_block_pin_directs_map.end()) return -1;
    auto st_it = lb_it->second.find(sub_tile.index);
    if (st_it == lb_it->second.end()) return -1;

    auto find_res = st_it->second.find(t_physical_pin(sub_tile_relative_pin));
    if (find_res == st_it->second.inverse_end()) return -1;

    const t_pb_graph_pin* pb_pin = get_pb_graph_node_pin_from_pb_graph_node(lb->pb_graph_head, find_res->second.pin);
    if (pb_pin == nullptr) return -1;
    return pb_pin->pin_count_in_cluster;
}

} // namespace

DirectConnectionLegality::DirectConnectionLegality(const std::vector<t_direct_inf>& directs, const DeviceContext& device_ctx) {
    const int delayless_switch = device_ctx.delayless_switch_idx;
    std::vector<t_clb_to_clb_directs> clb_to_clb_directs =
        alloc_and_load_clb_to_clb_directs(directs, delayless_switch);

    forward_by_from_lb_.resize(device_ctx.logical_block_types.size());

    for (const t_clb_to_clb_directs& cd : clb_to_clb_directs) {
        if (cd.from_clb_type == nullptr || cd.to_clb_type == nullptr) continue;

        // Pin ranges can be given in either direction; step = -1 means reversed.
        const int from_size = std::abs(cd.from_clb_pin_end_index - cd.from_clb_pin_start_index) + 1;
        const int to_size = std::abs(cd.to_clb_pin_end_index - cd.to_clb_pin_start_index) + 1;
        if (from_size != to_size) continue;
        const int from_step = (cd.from_clb_pin_start_index <= cd.from_clb_pin_end_index) ? 1 : -1;
        const int to_step = (cd.to_clb_pin_start_index <= cd.to_clb_pin_end_index) ? 1 : -1;

        // from_sub_tiles holds the from-side capacity locations; the to-side
        // ones are recovered the same way, keyed by the destination port name.
        // Iterating these covers a logical block mapped to several sub-tiles.
        const std::vector<int>& from_caps = cd.from_sub_tiles;
        const std::vector<int> to_caps = find_sub_tile_indices_by_port_name(cd.to_clb_type, cd.to_port);

        for (int from_cap : from_caps) {
            const t_sub_tile* from_sub_tile = get_sub_tile_from_capacity_location(cd.from_clb_type, from_cap);
            if (from_sub_tile == nullptr) continue;

            for (int to_cap : to_caps) {
                const t_sub_tile* to_sub_tile = get_sub_tile_from_capacity_location(cd.to_clb_type, to_cap);
                if (to_sub_tile == nullptr) continue;

                for (int k = 0; k < from_size; ++k) {
                    int from_rel_pin = cd.from_clb_pin_start_index + k * from_step;
                    int to_rel_pin = cd.to_clb_pin_start_index + k * to_step;

                    // A sub-tile can host several equivalent logical blocks. The
                    // <direct> applies to every (from_lb, to_lb) pairing that
                    // exposes the referenced pins, so record an entry for each.
                    for (t_logical_block_type_ptr from_lb : from_sub_tile->equivalent_sites) {
                        int from_pin_id = tile_pin_to_pb_pin_id(cd.from_clb_type, from_lb, *from_sub_tile, from_rel_pin);
                        if (from_pin_id < 0) continue;

                        for (t_logical_block_type_ptr to_lb : to_sub_tile->equivalent_sites) {
                            int to_pin_id = tile_pin_to_pb_pin_id(cd.to_clb_type, to_lb, *to_sub_tile, to_rel_pin);
                            if (to_pin_id < 0) continue;

                            forward_by_from_lb_[from_lb->index][from_pin_id]
                                .emplace(to_lb->index, to_pin_id);
                        }
                    }
                }
            }
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
    return pin_it->second.contains({to_lb->index, to_top_pin});
}
