#pragma once

/**
 * @file crr_connection_builder.h
 * @brief Produces the connections (edges) of each switch-block tile from the
 *        compiled switch templates.
 */

#include <memory>
#include <unordered_map>
#include <vector>

#include "rr_graph_view.h"
#include "physical_types.h"
#include "vpr_types.h"

#include "crr_common.h"
#include "crr_compiled_template.h"
#include "data_frame_processor.h"
#include "crr_switch_block_manager.h"

namespace crrgenerator {

/**
 * @brief Builds connections between routing nodes based on switch block
 * configurations
 */
class CRRConnectionBuilder {
  public:
    CRRConnectionBuilder(const RRGraphView& rr_graph,
                         const SwitchBlockManager& sb_manager,
                         const int verbosity,
                         e_gsb_version gsb_version);

    /**
     * @brief Initialize the connection builder and compile all switch
     *        templates into their numeric form.
     * @param fpga_grid_x  Device grid width
     * @param fpga_grid_y  Device grid height
     * @param is_annotated True when template cells carry integer delays
     */
    void initialize(int fpga_grid_x,
                    int fpga_grid_y,
                    bool is_annotated);

    /**
     * @brief Get connections for a tile
     * @param tile_x Tile x coordinate
     * @param tile_y Tile y coordinate
     * @return Vector of connections
     */
    std::vector<Connection> get_tile_connections(size_t tile_x, size_t tile_y) const;

  private:
    // Info from config
    int fpga_grid_x_;
    int fpga_grid_y_;
    bool is_annotated_;

    // Dependencies
    const RRGraphView& rr_graph_;
    const SwitchBlockManager& sb_manager_;
    int verbosity_;
    e_gsb_version gsb_version_;

    /**
     * @brief Resolve every spec of one template axis to a node at tile (x, y).
     * @param specs       Compiled specs of the axis
     * @param x, y        Tile coordinate
     * @param tile_type   Physical tile type at (x, y), used to resolve pin names
     * @param is_vertical True for the source (row) axis
     * @param nodes       Receives one RRNodeId per spec (INVALID when the spec
     *                    does not resolve at this tile)
     */
    void resolve_axis_nodes(const std::vector<CompiledSegSpec>& specs,
                            int x,
                            int y,
                            t_physical_tile_type_ptr tile_type,
                            bool is_vertical,
                            std::vector<RRNodeId>& nodes) const;

    /**
     * @brief Resolve an IPIN/OPIN spec to a node (fatal when the pin does not
     *        exist in the RR graph).
     * @param name_to_ptc Pin name -> PTC map of the tile's physical type
     */
    RRNodeId resolve_pin_spec(const CompiledSegSpec& spec,
                              int x,
                              int y,
                              const std::unordered_map<std::string, int>& name_to_ptc) const;

    /// Resolve a routing segment spec to a node (INVALID when absent)
    RRNodeId resolve_channel_spec(const CompiledSegSpec& spec, int x, int y, bool is_vertical) const;

    /**
     * @brief Find the routing segment node of the given type whose bounding box
     *        is exactly (x_low, y_low) -> (x_high, y_high) and whose PTC
     *        sequence is exactly (first_ptc, first_ptc + ptc_step, ...,
     *        length terms), using the RR graph's spatial node lookup.
     * @return The node, or RRNodeId::INVALID() when no node matches exactly
     */
    RRNodeId find_channel_node(e_rr_type type,
                               int x_low,
                               int x_high,
                               int y_low,
                               int y_high,
                               int first_ptc,
                               int ptc_step,
                               int length) const;

    /**
     * @brief Find the pin node (IPIN/OPIN) located exactly at (x, y) with the
     *        given pin PTC, using the RR graph's spatial node lookup.
     * @return The node, or RRNodeId::INVALID() when not found
     */
    RRNodeId find_pin_node(e_rr_type type, int x, int y, int pin_ptc) const;

    /**
     * @brief Compute the clamped bounding box of a segment spec at tile (x, y).
     * @param physical_length Receives the number of grid locations spanned
     * @param truncated       Receives the number of locations clipped away by
     *                        the device boundary (used to shift the start PTC)
     */
    void calculate_segment_coordinates(const CompiledSegSpec& spec,
                                       int x,
                                       int y,
                                       int& x_low,
                                       int& x_high,
                                       int& y_low,
                                       int& y_high,
                                       int& physical_length,
                                       int& truncated,
                                       bool is_vertical) const;

    /// Compiled templates, one per unique dataframe
    std::unordered_map<const DataFrame*, std::unique_ptr<CompiledTemplate>> compiled_templates_;

    /// Compiled template per SB_MAPS pattern index (nullptr when the pattern
    /// has no template file, i.e. no connections)
    std::vector<const CompiledTemplate*> template_by_pattern_;

    // Per-tile-type reverse map: pin_name -> pin_ptc.
    // Built once at construction for all physical tile types and indexed by
    // t_physical_tile_type::index.
    std::vector<std::unordered_map<std::string, int>> pin_name_to_ptc_cache_;
};

} // namespace crrgenerator
