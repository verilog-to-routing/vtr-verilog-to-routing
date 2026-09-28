#include "crr_connection_builder.h"

#include <string>

#include "globals.h"
#include "physical_types_util.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_assert.h"
#include "rr_node_types.h"

namespace crrgenerator {

/**
 * @brief Builds the per-tile-type reverse map (pin_name -> pin_ptc) for every
 *        physical tile type.
 *
 * The returned vector is indexed by t_physical_tile_type::index.
 *
 * Kept as a free helper so it can be invoked directly in the constructor's
 * initializer list. The returned vector is a prvalue, so it is moved (not
 * copied) into the member it initializes.
 */
static std::vector<std::unordered_map<std::string, int>>
create_pin_name_to_ptc_cache(const std::vector<t_physical_tile_type>& physical_tile_types) {
    std::vector<std::unordered_map<std::string, int>> pin_name_to_ptc_cache(physical_tile_types.size());

    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        VTR_ASSERT(tile_type.index >= 0 && tile_type.index < static_cast<int>(physical_tile_types.size()));
        std::unordered_map<std::string, int>& name_to_ptc = pin_name_to_ptc_cache[tile_type.index];
        name_to_ptc.reserve(tile_type.num_pins);
        for (int pin_ptc = 0; pin_ptc < tile_type.num_pins; ++pin_ptc) {
            name_to_ptc.emplace(block_type_pin_index_to_name(&tile_type, pin_ptc, false), pin_ptc);
        }
    }

    return pin_name_to_ptc_cache;
}

CRRConnectionBuilder::CRRConnectionBuilder(const RRGraphView& rr_graph,
                                           const SwitchBlockManager& sb_manager,
                                           const int verbosity,
                                           e_gsb_version gsb_version)
    : rr_graph_(rr_graph)
    , sb_manager_(sb_manager)
    , verbosity_(verbosity)
    , gsb_version_(gsb_version)
    , pin_name_to_ptc_cache_(create_pin_name_to_ptc_cache(g_vpr_ctx.device().physical_tile_types)) {
    if (gsb_version_ != e_gsb_version::GSB_V1 && gsb_version_ != e_gsb_version::GSB_V2) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unrecognized GSB version: %d\n", static_cast<int>(gsb_version_));
    }
}

void CRRConnectionBuilder::initialize(int fpga_grid_x,
                                      int fpga_grid_y,
                                      bool is_annotated) {
    fpga_grid_x_ = fpga_grid_x;
    fpga_grid_y_ = fpga_grid_y;
    is_annotated_ = is_annotated;

    // Compile each unique switch template once and map every SB_MAPS pattern
    // to its compiled template (nullptr for patterns without a template file).
    template_by_pattern_.clear();
    template_by_pattern_.reserve(sb_manager_.num_patterns());
    for (size_t pattern_idx = 0; pattern_idx < sb_manager_.num_patterns(); ++pattern_idx) {
        const DataFrame* df = sb_manager_.get_dataframe_by_index(pattern_idx);
        if (df == nullptr) {
            template_by_pattern_.push_back(nullptr);
            continue;
        }
        std::unique_ptr<CompiledTemplate>& compiled = compiled_templates_[df];
        if (!compiled) {
            compiled = std::make_unique<CompiledTemplate>(*df, is_annotated_);
        }
        template_by_pattern_.push_back(compiled.get());
    }
}

std::vector<Connection> CRRConnectionBuilder::get_tile_connections(size_t tile_x, size_t tile_y) const {
    int pattern_idx = sb_manager_.find_matching_pattern_index(tile_x, tile_y);
    if (pattern_idx < 0) {
        // If no pattern is found, it means that no switch block is defined for this location
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No pattern found for switch block at (%zu, %zu)\n", tile_x, tile_y);
        return {};
    }

    const CompiledTemplate* tpl = template_by_pattern_[pattern_idx];
    if (tpl == nullptr) {
        // The pattern has no switch template file, so there are no connections
        VTR_LOGV(verbosity_ > 1, "No switch block is associated with the pattern at (%zu, %zu)\n", tile_x, tile_y);
        return {};
    }

    int x = static_cast<int>(tile_x);
    int y = static_cast<int>(tile_y);

    // CRR currently only supports 2D architectures, so layer 0 is used. The
    // tile type is only needed (and checked) when pin specs are resolved.
    t_physical_tile_type_ptr tile_type = g_vpr_ctx.device().grid.get_physical_type({x, y, 0});

    // Resolve the source (row) and sink (column) specs to nodes at this tile
    std::vector<RRNodeId> source_nodes;
    std::vector<RRNodeId> sink_nodes;
    resolve_axis_nodes(tpl->sources(), x, y, tile_type, /*is_vertical=*/true, source_nodes);
    resolve_axis_nodes(tpl->sinks(), x, y, tile_type, /*is_vertical=*/false, sink_nodes);

    // Emit the pre-compiled cells whose endpoints both resolved
    std::vector<Connection> tile_connections;
    tile_connections.reserve(tpl->cells().size());
    for (const CompiledCell& cell : tpl->cells()) {
        RRNodeId source_node = source_nodes[cell.source_idx];
        RRNodeId sink_node = sink_nodes[cell.sink_idx];
        if (!source_node.is_valid() || !sink_node.is_valid()) {
            continue;
        }

        // If the source node is an IPIN, then it should be considered as
        // a sink of the connection.
        if (tpl->sources()[cell.source_idx].node_type == e_rr_type::IPIN) {
            tile_connections.emplace_back(source_node, sink_node, cell.delay_ps);
        } else {
            tile_connections.emplace_back(sink_node, source_node, cell.delay_ps);
        }
    }

    // Uniqueify the connections
    vtr::uniquify(tile_connections);
    tile_connections.shrink_to_fit();

    VTR_LOGV(verbosity_ > 1, "Generated %zu connections for location (%zu, %zu)\n",
             tile_connections.size(), tile_x, tile_y);

    return tile_connections;
}

void CRRConnectionBuilder::resolve_axis_nodes(const std::vector<CompiledSegSpec>& specs,
                                              int x,
                                              int y,
                                              t_physical_tile_type_ptr tile_type,
                                              bool is_vertical,
                                              std::vector<RRNodeId>& nodes) const {
    // Pin-name resolution map of this tile's type; fetched once when the first
    // pin spec is seen (a missing tile type only matters for pin specs)
    const std::unordered_map<std::string, int>* name_to_ptc = nullptr;

    nodes.resize(specs.size());
    for (size_t ispec = 0; ispec < specs.size(); ++ispec) {
        const CompiledSegSpec& spec = specs[ispec];
        if (spec.is_pin()) {
            if (name_to_ptc == nullptr) {
                if (tile_type == nullptr) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No tile type found while resolving pin '%s' at (%d,%d)\n",
                                    spec.pin_name.c_str(), x, y);
                }
                name_to_ptc = &pin_name_to_ptc_cache_.at(tile_type->index);
            }
            nodes[ispec] = resolve_pin_spec(spec, x, y, *name_to_ptc);
        } else {
            nodes[ispec] = resolve_channel_spec(spec, x, y, is_vertical);
        }
    }
}

RRNodeId CRRConnectionBuilder::resolve_pin_spec(const CompiledSegSpec& spec,
                                                int x,
                                                int y,
                                                const std::unordered_map<std::string, int>& name_to_ptc) const {
    int pin_ptc;
    auto pin_it = name_to_ptc.find(spec.pin_name);
    if (pin_it != name_to_ptc.end()) {
        pin_ptc = pin_it->second;
    } else {
        VTR_LOGV(verbosity_ > 0,
                 "Failed to resolve %s pin '%s' at (%d,%d); falling back to CSV index %d\n",
                 rr_node_typename[spec.node_type],
                 spec.pin_name.c_str(),
                 x, y, spec.fallback_ptc);
        pin_ptc = spec.fallback_ptc;
    }

    RRNodeId node = find_pin_node(spec.node_type, x, y, pin_ptc);
    if (!node.is_valid()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Failed to find %s node for pin '%s' at (%d,%d)\n",
                        rr_node_typename[spec.node_type], spec.pin_name.c_str(), x, y);
    }
    return node;
}

RRNodeId CRRConnectionBuilder::resolve_channel_spec(const CompiledSegSpec& spec, int x, int y, bool is_vertical) const {
    int x_low, x_high, y_low, y_high;
    int physical_length, truncated;
    calculate_segment_coordinates(spec, x, y, x_low, x_high, y_low, y_high,
                                  physical_length, truncated, is_vertical);

    // Shift the starting PTC by the locations clipped away at the device
    // boundary (only when locations were clipped at the low end)
    int first_ptc = spec.base_ptc;
    if (truncated > 0) {
        first_ptc += (spec.direction == Direction::DEC) ? -PTCS_PER_TRACK_PAIR * truncated
                                                        : PTCS_PER_TRACK_PAIR * truncated;
    }
    int ptc_step = (spec.direction == Direction::DEC) ? -PTCS_PER_TRACK_PAIR : PTCS_PER_TRACK_PAIR;

    RRNodeId node = find_channel_node(spec.node_type, x_low, x_high, y_low, y_high,
                                      first_ptc, ptc_step, physical_length);
    if (!node.is_valid()) {
        VTR_LOGV(verbosity_ > 1,
                 "Node not found: SB(%d,%d) side=%s tap=%d -> %s [start ptc %d] (%d,%d) -> (%d,%d)\n",
                 x, y,
                 template_side_name[spec.side],
                 spec.tap,
                 rr_node_typename[spec.node_type],
                 first_ptc,
                 x_low, y_low, x_high, y_high);
    }
    return node;
}

RRNodeId CRRConnectionBuilder::find_channel_node(e_rr_type type,
                                                 int x_low,
                                                 int x_high,
                                                 int y_low,
                                                 int y_high,
                                                 int first_ptc,
                                                 int ptc_step,
                                                 int length) const {
    if (length <= 0) {
        // A segment fully clipped away by the device boundary matches nothing
        return RRNodeId::INVALID();
    }

    // The spatial lookup registers every CHANX/CHANY node at every grid
    // location it spans, keyed by its per-location track number; a node's key
    // at its own low corner is the first entry of its PTC sequence.
    // find_node() bounds-checks all inputs and returns INVALID out of range.
    RRNodeId node = rr_graph_.node_lookup().find_node(0, x_low, y_low, type, first_ptc);
    if (!node.is_valid()) {
        return RRNodeId::INVALID();
    }

    // Verify the node's bounding box and full PTC sequence are exactly the
    // requested ones, so a hit at the low corner never produces a false match
    // (e.g. a longer wire passing through the queried location).
    if (rr_graph_.node_xlow(node) != x_low || rr_graph_.node_xhigh(node) != x_high
        || rr_graph_.node_ylow(node) != y_low || rr_graph_.node_yhigh(node) != y_high) {
        return RRNodeId::INVALID();
    }

    const t_rr_graph_storage& nodes = rr_graph_.rr_nodes();
    if (nodes.node_contain_multiple_ptc(node)) {
        const std::vector<short>& track_nums = nodes.node_tilable_track_nums(node);
        if (static_cast<int>(track_nums.size()) != length) {
            return RRNodeId::INVALID();
        }
        for (int i = 0; i < length; ++i) {
            if (track_nums[i] != first_ptc + ptc_step * i) {
                return RRNodeId::INVALID();
            }
        }
    } else if (length != 1 || nodes.node_ptc_num(node) != first_ptc) {
        return RRNodeId::INVALID();
    }

    return node;
}

RRNodeId CRRConnectionBuilder::find_pin_node(e_rr_type type, int x, int y, int pin_ptc) const {
    // A pin PTC identifies at most one node per tile; it may be registered on
    // several sides of the tile, so probe the sides until it is found.
    for (const e_side& side : TOTAL_2D_SIDES) {
        RRNodeId node = rr_graph_.node_lookup().find_node(0, x, y, type, pin_ptc, side);
        if (!node.is_valid()) {
            continue;
        }
        // Match only pins located exactly at (x, y) (mirrors the previous
        // exact bounding-box lookup semantics)
        if (rr_graph_.node_xlow(node) != x || rr_graph_.node_xhigh(node) != x
            || rr_graph_.node_ylow(node) != y || rr_graph_.node_yhigh(node) != y) {
            return RRNodeId::INVALID();
        }
        return node;
    }
    return RRNodeId::INVALID();
}

void CRRConnectionBuilder::calculate_segment_coordinates(const CompiledSegSpec& spec,
                                                         int x,
                                                         int y,
                                                         int& x_low,
                                                         int& x_high,
                                                         int& y_low,
                                                         int& y_high,
                                                         int& physical_length,
                                                         int& truncated,
                                                         bool is_vertical) const {
    int seg_length = spec.seg_len;
    int tap = spec.tap;

    // V1 uses 1-indexed segment coordinates; V2 uses 0-indexed.
    int coord_offset = (gsb_version_ == e_gsb_version::GSB_V1) ? 1 : 0;

    if (is_vertical) {
        switch (spec.side) {
            case e_sw_template_dir::LEFT:
                x_high = x + seg_length - tap - 1 + coord_offset;
                x_low = x - tap + coord_offset;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::RIGHT:
                x_high = x + tap;
                x_low = x + tap + 1 - seg_length;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::TOP:
                x_high = x;
                x_low = x;
                y_high = y + tap;
                y_low = y + 1 - seg_length + tap;
                break;
            case e_sw_template_dir::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y + seg_length - tap - 1 + coord_offset;
                y_low = y - tap + coord_offset;
                break;
            default:
                VTR_ASSERT_MSG(false, "Routing segment specs always carry a LEFT/RIGHT/TOP/BOTTOM side");
                x_low = x_high = x;
                y_low = y_high = y;
                break;
        }
    } else {
        switch (spec.side) {
            case e_sw_template_dir::LEFT:
                x_high = x + tap - 1;
                x_low = x - seg_length + tap;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::RIGHT:
                x_high = x + seg_length - 1 + coord_offset;
                x_low = x + coord_offset;
                y_high = y;
                y_low = y;
                break;
            case e_sw_template_dir::TOP:
                x_high = x;
                x_low = x;
                y_high = y + seg_length - 1 + coord_offset;
                y_low = y + coord_offset;
                break;
            case e_sw_template_dir::BOTTOM:
                x_high = x;
                x_low = x;
                y_high = y;
                y_low = y - seg_length + tap;
                break;
            default:
                VTR_ASSERT_MSG(false, "Routing segment specs always carry a LEFT/RIGHT/TOP/BOTTOM side");
                x_low = x_high = x;
                y_low = y_high = y;
                break;
        }
    }

    // VPR does not allow routing tracks on the perimeter of the device; the
    // minimum coordinate of a track depends on the channel type.
    bool is_chanx = spec.side == e_sw_template_dir::LEFT || spec.side == e_sw_template_dir::RIGHT;
    int min_x = (is_chanx ? 1 : 0);
    int min_y = (is_chanx ? 0 : 1);
    int max_x = fpga_grid_x_ - 2;
    int max_y = fpga_grid_y_ - 2;

    // Calculate truncation
    truncated = (std::max(x_low, min_x) - x_low) - (x_high - std::min(x_high, max_x));
    truncated += (std::max(y_low, min_y) - y_low) - (y_high - std::min(y_high, max_y));

    // Apply grid boundaries
    x_low = std::max(x_low, min_x);
    y_low = std::max(y_low, min_y);
    x_high = std::min(x_high, max_x);
    y_high = std::min(y_high, max_y);

    // Calculate physical length
    physical_length = (x_high - x_low) + (y_high - y_low) + 1;
}

} // namespace crrgenerator
