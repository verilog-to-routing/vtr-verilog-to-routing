
#pragma once

#include "device_grid.h"
#include "rr_types.h"
#include "rr_graph_type.h"
#include "vtr_expr_eval.h"

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
typedef vtr::flat_map<vtr::string_view, t_wire_info> t_wire_type_sizes;

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
                 const t_specified_loc& specified_loc = t_specified_loc());

const t_chan_details& index_into_correct_chan(const t_physical_tile_loc& sb_loc,
                                              e_side src_side,
                                              const t_chan_details& chan_details_x,
                                              const t_chan_details& chan_details_y,
                                              t_physical_tile_loc& chan_loc,
                                              e_rr_type& chan_type);

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

int evaluate_num_conns_formula(vtr::FormulaParser& formula_parser,
                               vtr::t_formula_data& formula_data,
                               const std::string& num_conns_formula,
                               int from_wire_count,
                               int to_wire_count);