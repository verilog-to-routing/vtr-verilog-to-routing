#include "crr_compiled_template.h"

#include <algorithm>
#include <charconv>

#include "vpr_error.h"

namespace crrgenerator {

/**
 * @brief Direction of a routing segment given its switch-block side and the
 *        axis it appears on in the template (rows = vertical, columns =
 *        horizontal).
 */
static Direction direction_for_side(e_sw_template_dir side, bool is_vertical) {
    if (is_vertical) {
        return (side == e_sw_template_dir::RIGHT || side == e_sw_template_dir::TOP) ? Direction::DEC
                                                                                    : Direction::INC;
    } else {
        return (side == e_sw_template_dir::RIGHT || side == e_sw_template_dir::TOP) ? Direction::INC
                                                                                    : Direction::DEC;
    }
}

/// RR node type of a routing segment on the given switch-block side
static e_rr_type chan_type_for_side(e_sw_template_dir side) {
    return (side == e_sw_template_dir::LEFT || side == e_sw_template_dir::RIGHT) ? e_rr_type::CHANX
                                                                                 : e_rr_type::CHANY;
}

CompiledTemplate::CompiledTemplate(const DataFrame& df, bool is_annotated) {
    std::vector<int32_t> source_index; // dataframe row -> index into sources_
    std::vector<int32_t> sink_index;   // dataframe col -> index into sinks_
    compile_axis(df, /*is_vertical=*/true, sources_, source_index);
    compile_axis(df, /*is_vertical=*/false, sinks_, sink_index);

    // Compile the non-empty cells whose row and column both carry a valid spec
    for (size_t row = NUM_EMPTY_ROWS; row < df.rows(); ++row) {
        if (source_index[row] < 0) {
            continue;
        }
        for (size_t col = NUM_EMPTY_COLS; col < df.cols(); ++col) {
            if (sink_index[col] < 0) {
                continue;
            }
            const Cell& cell = df.at(row, col);
            if (cell.is_empty()) {
                continue;
            }
            cells_.push_back({source_index[row], sink_index[col], parse_cell_delay(cell, is_annotated)});
        }
    }
    cells_.shrink_to_fit();
}

void CompiledTemplate::compile_axis(const DataFrame& df,
                                    bool is_vertical,
                                    std::vector<CompiledSegSpec>& specs,
                                    std::vector<int32_t>& spec_index) {
    size_t axis_size = is_vertical ? df.rows() : df.cols();
    size_t axis_start = is_vertical ? NUM_EMPTY_ROWS : NUM_EMPTY_COLS;
    spec_index.assign(axis_size, -1);

    // The PTC assignment of a routing segment depends on the segments that
    // precede it on the same axis. This state chain replicates the original
    // per-tile parser exactly; it is tile-independent, which is what makes the
    // one-time compilation possible.
    std::string prev_seg_type;
    int prev_seg_index = -1;
    e_sw_template_dir prev_side = e_sw_template_dir::NUM_SIDES;
    int prev_ptc_number = 0;

    for (size_t i = axis_start; i < axis_size; ++i) {
        // Cell positions of side/type/index/tap differ between the two axes
        const Cell& side_cell = is_vertical ? df.at(i, 0) : df.at(0, i);
        const Cell& type_cell = is_vertical ? df.at(i, 1) : df.at(1, i);
        const Cell& index_cell = is_vertical ? df.at(i, 2) : df.at(3, i);
        const Cell& tap_cell = is_vertical ? df.at(i, 3) : df.at(4, i);

        CompiledSegSpec spec;
        if (!side_cell.is_empty()) {
            std::string side_str = side_cell.as_string();
            std::transform(side_str.begin(), side_str.end(), side_str.begin(), ::toupper);
            spec.side = get_sw_template_dir(side_str);
        }
        if (spec.side == e_sw_template_dir::NUM_SIDES) {
            // No side -> not a spec; the original parser skips the row/column
            // without touching the PTC state chain.
            continue;
        }

        std::string seg_type = type_cell.is_empty() ? std::string() : type_cell.as_string();
        int seg_index = index_cell.is_empty() ? -1 : static_cast<int>(index_cell.as_int());
        // The horizontal axis defaults a missing tap to 1; the vertical axis
        // leaves it unset (mirroring the original parser).
        spec.tap = is_vertical ? -1 : 1;
        if (!tap_cell.is_empty()) {
            if (tap_cell.is_number()) {
                spec.tap = static_cast<int>(tap_cell.as_int());
            } else {
                spec.pin_name = tap_cell.as_string();
            }
        }

        // An OPIN spec on the sink (column) axis is not supported by the
        // template format and can never resolve to a node: no spec is emitted
        // for it (the PTC state chain updates below still apply), so all cells
        // in its column are dropped at compile time.
        bool emit_spec = true;
        if (spec.is_pin()) {
            spec.node_type = (spec.side == e_sw_template_dir::OPIN) ? e_rr_type::OPIN : e_rr_type::IPIN;
            spec.fallback_ptc = seg_index;
            emit_spec = is_vertical || spec.side != e_sw_template_dir::OPIN;
            if (emit_spec && spec.pin_name.empty()) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "No pin name is specified for segment index %d in switch template '%s'\n",
                                seg_index, df.source_file.c_str());
            }
        } else {
            // Routing segment: seg_type is "L<len>"
            spec.node_type = chan_type_for_side(spec.side);
            spec.seg_len = std::stoi(seg_type.substr(1));
            spec.direction = direction_for_side(spec.side, is_vertical);

            // PTC state chain (identical to the original per-tile computation)
            if (prev_seg_type != seg_type) {
                prev_ptc_number = prev_seg_index;
            }
            if (prev_side != spec.side) {
                prev_ptc_number = 0;
                prev_seg_index = 0;
            }
            int ptc = (seg_index - 1) * spec.seg_len * PTCS_PER_TRACK_PAIR + prev_ptc_number;
            prev_seg_index = std::max(prev_seg_index, ptc + PTCS_PER_TRACK_PAIR * spec.seg_len);
            ptc += (spec.direction == Direction::INC) ? 0 : 1;
            ptc += (spec.direction == Direction::DEC) ? PTCS_PER_TRACK_PAIR * (spec.seg_len - 1) : 0;
            spec.base_ptc = ptc;
        }

        prev_seg_type = seg_type;
        prev_side = spec.side;

        if (emit_spec) {
            spec_index[i] = static_cast<int32_t>(specs.size());
            specs.push_back(std::move(spec));
        }
    }
    specs.shrink_to_fit();
}

int CompiledTemplate::parse_cell_delay(const Cell& cell, bool is_annotated) {
    if (!is_annotated) {
        return -1;
    }
    std::string value = cell.as_string();
    int delay_ps = 0;
    const char* first = value.data();
    const char* last = value.data() + value.size();
    auto [ptr, ec] = std::from_chars(first, last, delay_ps);
    if (ec != std::errc() || ptr != last) {
        // Not an integer -> delay is not annotated for this connection
        return -1;
    }
    return delay_ps;
}

} // namespace crrgenerator
