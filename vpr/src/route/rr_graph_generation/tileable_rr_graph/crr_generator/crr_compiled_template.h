#pragma once

/**
 * @file crr_compiled_template.h
 * @brief Numeric, pre-parsed form of a switch-block template dataframe.
 *
 * A switch template CSV describes which routing
 * segments / pins (the dataframe rows are connection sources, the columns are
 * connection sinks) are connected and with what delay. The raw DataFrame holds
 * strings, and the original implementation re-parsed those strings for every
 * switch-block tile on the device. A CompiledTemplate is built once per
 * template and captures everything that is tile-independent:
 *
 *  - each valid row/column parsed into a CompiledSegSpec (side, segment
 *    length, starting PTC including the stateful PTC-assignment chain,
 *    direction, or pin name for IPIN/OPIN specs),
 *  - each non-empty cell reduced to (source spec, sink spec, delay).
 *
 * Resolving a spec to a concrete RRNodeId (which depends on the tile
 * coordinate) is done per tile by the CRRConnectionBuilder using pure integer
 * arithmetic and a numeric node lookup.
 */

#include <string>
#include <vector>

#include "rr_node_types.h"
#include "crr_common.h"
#include "data_frame_processor.h"

namespace crrgenerator {

/// Number of PTCs per track position of a unidirectional channel: each
/// position carries an INC/DEC track pair, so consecutive same-direction
/// tracks — and each grid-location step along a wire's PTC sequence — are
/// two PTCs apart. All template PTC arithmetic is in units of this stride.
constexpr int PTCS_PER_TRACK_PAIR = 2;

/**
 * @brief One template row (connection source) or column (connection sink) in
 *        compiled numeric form.
 */
struct CompiledSegSpec {
    /// Side of the switch block this spec refers to (LEFT/RIGHT/TOP/BOTTOM for
    /// routing segments, IPIN/OPIN for pins)
    e_sw_template_dir side = e_sw_template_dir::NUM_SIDES;

    /// RR node type this spec resolves to (CHANX/CHANY/IPIN/OPIN)
    e_rr_type node_type = e_rr_type::NUM_RR_TYPES;

    // --- Routing segment (CHANX/CHANY) fields ---

    /// Segment length in tiles (parsed from the "L<len>" segment type)
    int seg_len = 0;

    /// Starting PTC of the segment's track group at this switch block,
    /// including the cumulative PTC-assignment chain across template rows and
    /// the direction offsets, but before boundary truncation is applied
    int base_ptc = 0;

    /// Direction of the segment (INC/DEC), derived from the side and axis
    Direction direction = Direction::INC;

    /// Tap position of the segment relative to the switch block
    int tap = 1;

    // --- Pin (IPIN/OPIN) fields ---

    /// Pin name; resolved to a PTC through the physical tile type at the tile
    std::string pin_name;

    /// CSV segment index, used as the PTC when the pin name does not resolve
    int fallback_ptc = -1;

    bool is_pin() const {
        return side == e_sw_template_dir::IPIN || side == e_sw_template_dir::OPIN;
    }
};

/**
 * @brief One non-empty template cell: a connection between a source spec and a
 *        sink spec with a pre-parsed delay.
 */
struct CompiledCell {
    int32_t source_idx; ///< index into CompiledTemplate::sources()
    int32_t sink_idx;   ///< index into CompiledTemplate::sinks()
    /// Connection delay in ps; -1 means the delay is not annotated and the
    /// sink node's architecture-defined driver switch is used instead
    int delay_ps;
};

/**
 * @brief Tile-independent compiled form of one switch template dataframe.
 */
class CompiledTemplate {
  public:
    /**
     * @brief Compile a dataframe.
     * @param df           The parsed switch template
     * @param is_annotated True when the cells carry integer delays
     *                     (--annotated_rr_graph); otherwise all delays are -1
     */
    CompiledTemplate(const DataFrame& df, bool is_annotated);

    const std::vector<CompiledSegSpec>& sources() const { return sources_; }
    const std::vector<CompiledSegSpec>& sinks() const { return sinks_; }
    const std::vector<CompiledCell>& cells() const { return cells_; }

  private:
    /**
     * @brief Parse one axis of the dataframe (rows when is_vertical, columns
     *        otherwise) into compiled specs, replicating the stateful PTC
     *        assignment chain of the original per-tile parser.
     * @param df          The dataframe
     * @param is_vertical True for the row (source) axis
     * @param specs       Receives the compiled specs of valid rows/columns
     * @param spec_index  Receives, per dataframe row/column, the index into
     *                    specs or -1 when the row/column has no valid spec
     */
    static void compile_axis(const DataFrame& df,
                             bool is_vertical,
                             std::vector<CompiledSegSpec>& specs,
                             std::vector<int32_t>& spec_index);

    /**
     * @brief Parse a delay cell. Returns the integer delay when is_annotated
     *        and the cell holds an integer, -1 otherwise (mirroring the
     *        original get_connection_delay_ps()).
     */
    static int parse_cell_delay(const Cell& cell, bool is_annotated);

    std::vector<CompiledSegSpec> sources_; ///< compiled specs of valid rows
    std::vector<CompiledSegSpec> sinks_;   ///< compiled specs of valid columns
    std::vector<CompiledCell> cells_;      ///< compiled non-empty cells
};

} // namespace crrgenerator
