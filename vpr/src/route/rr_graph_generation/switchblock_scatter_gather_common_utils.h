
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "device_grid.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "vtr_expr_eval.h"
#include "vtr_hash.h"

#include <vector>

/** Contains info about a wire segment type */
struct t_wire_info {
    int length;    ///< the length of this type of wire segment in tiles
    int num_wires; ///< total number of wires in a channel segment (basically W)
    int start;     ///< the wire index at which this type starts in the channel segment (0..W-1)

    void set(int len, int wires, int st) {
        length = len;
        num_wires = wires;
        start = st;
    }

    t_wire_info() {
        this->set(0, 0, 0);
    }

    t_wire_info(int len, int wires, int st) {
        this->set(len, wires, st);
    }
};

struct t_wire_switchpoint {
    int wire;        ///< Wire index within the channel
    int switchpoint; ///< Switchpoint of the wire
};

/// Used to get info about a given wire type based on the name
typedef vtr::flat_map<std::string_view, t_wire_info> t_wire_type_sizes;

/**
 * @brief check whether a switch block exists in a specified coordinate within the device grid
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param loc Coordinates of the given location to be evaluated.
 *   @param sb switchblock information specified in the architecture file
 *
 * @return true if a switch block exists at the specified location, false otherwise.
 */
bool sb_not_here(const DeviceGrid& grid,
                 const std::vector<bool>& inter_cluster_rr,
                 const t_physical_tile_loc& loc,
                 e_sb_location sb_location,
                 const t_specified_loc& specified_loc);

/**
 * @brief True if switchblock location (loc) matches an architecture XY_SPECIFIED rule.
 *
 * When ``x`` and/or ``y`` are set on ``specified_loc``, the match is an exact coordinate or
 * a row/column wildcard. Otherwise ``reg_x`` / ``reg_y`` describe a
 * tiled rectangular region with optional repeat and stride (incrx/incry).
 *
 * @param grid Device grid (dimensions used when clamping region bounds).
 * @param loc Candidate switchblock location to test.
 * @param specified_loc XY_SPECIFIED rule from the architecture (exact x/y or region + stride).
 */
bool match_sb_xy(const DeviceGrid& grid,
                 const t_physical_tile_loc& loc,
                 const t_specified_loc& specified_loc);

/**
 * @brief finds the correct channel (x or y), and the coordinates to index into it based on the
 * specified tile coordinates (x,y,layer) and the switch block side.
 *
 *  @param sb_loc Coordinates of the switch blocks
 *  @param src_side The side of switch block where the routing channels segment is located at.
 *  @param chan_details_x x-channel segment details (length, start and end points, ...)
 *  @param chan_details_y x-channel segment details (length, start and end points, ...)
 *  @param chan_loc Coordinate of the indexed routing channel segment. To be filled by this function.
 *  @param chan_type The type of the indexed routing channel segment. To be filled by this function.
 *
 * @return returns the type of channel that we are indexing into (ie, CHANX or CHANY) and channel coordinates and type
 */
const t_chan_details& index_into_correct_chan(const t_physical_tile_loc& sb_loc,
                                              e_side src_side,
                                              const t_chan_details& chan_details_x,
                                              const t_chan_details& chan_details_y,
                                              t_physical_tile_loc& chan_loc,
                                              e_rr_type& chan_type);

/**
 * @brief Check whether a specific switch block location is valid within the device grid
 * @param loc Coordinates of the location to be evaluated.
 * @param chan_type Routing channel type (CHANX or CHANY), required since device perimeter does not have certain channels
 *
 * @return True if the given location is outside the device grid, false otherwise.
 */
bool chan_coords_out_of_bounds(const t_physical_tile_loc& loc, e_rr_type chan_type);

std::pair<t_wire_type_sizes, t_wire_type_sizes> count_wire_type_sizes(const t_chan_details& chan_details_x,
                                                                      const t_chan_details& chan_details_y,
                                                                      const t_chan_width& nodes_per_chan);

/**
 * Returns the switchpoint of the wire specified by wire_details at a segment coordinate
 * of seg_coord, and connection to the sb_side of the switchblock
 */
int get_switchpoint_of_wire(e_rr_type chan_type,
                            const t_chan_seg_details& wire_details,
                            int seg_coord,
                            e_side sb_side);

/**
 * @brief Evaluates a connection formula for scatter/gather and custom switchblock patterns.
 *
 * Sets variables "from" and "to" to the given wire counts and evaluates
 * the formula string using the provided parser
 */
int evaluate_num_conns_formula(vtr::FormulaParser& formula_parser,
                               vtr::t_formula_data& formula_data,
                               const std::string& num_conns_formula,
                               size_t from_wire_count,
                               size_t to_wire_count);

/**
 * @brief Memoizes switch block permutation formula results.
 *
 * A permutation formula's result depends only on the formula string and the values
 * of its variables W and t. These repeat across connections and switch block locations,
 * so results are cached to avoid re-parsing the formula for every connection.
 */
class SbFormulaCache {
  public:
    /**
     * @brief Evaluates a permutation formula for the given W and t.
     *
     * Returns the raw (unadjusted) formula result. Parses the formula only on the
     * first call for a given (formula, W, t) and returns the cached result afterwards.
     *
     * @param formula Permutation formula string from the architecture file.
     * @param W       Size of the destination wire set.
     * @param t       Index of the source wire within the source set.
     */
    int evaluate(const std::string& formula, int W, int t);

  private:
    /// Parser used on cache misses
    vtr::FormulaParser formula_parser_;

    /// Variable values passed to the parser
    vtr::t_formula_data formula_data_;

    /// Cached raw formula results, indexed by the formula string as written in the
    /// architecture file, then by the (W, t) pair the formula was evaluated with
    std::unordered_map<std::string, std::unordered_map<std::pair<int, int>, int, vtr::hash_pair>> results_;
};
