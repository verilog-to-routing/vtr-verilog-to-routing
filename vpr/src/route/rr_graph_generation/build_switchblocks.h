#pragma once

#include <unordered_map>
#include <vector>
#include "physical_types.h"
#include "device_grid.h"
#include "rr_graph_type.h"
#include "vtr_random.h"
#include "rr_types.h"

/************ Classes, structs, typedefs ************/

/**
 * @brief Holds the coordinates of a switch block source connection.
 *
 * Used to index into a map which specifies which destination wire segments this source wire should
 * connect to.
 */
class SwitchblockLookupKey {
  public:
    int x_coord;      ///< x coordinate of switchblock connection
    int y_coord;      ///< y coordinate of switchblock connection
    int layer_coord;  ///< layer number of switchblock
    e_side from_side; ///< source side of switchblock connection
    e_side to_side;   ///< destination side of switchblock connection

    /// @brief Empty constructor initializes everything to 0
    SwitchblockLookupKey() {
        x_coord = y_coord = layer_coord = -1; //TODO: use set function
    }

    /// @brief Constructor for initializing member variables
    SwitchblockLookupKey(int set_x, int set_y, int set_layer, e_side set_from, e_side set_to) {
        this->set_coords(set_x, set_y, set_layer, set_from, set_to);
    }

    SwitchblockLookupKey(const t_physical_tile_loc& set_loc, e_side set_from, e_side set_to) {
        this->set_coords(set_loc.x, set_loc.y, set_loc.layer_num, set_from, set_to);
    }

    /// @brief Constructor for initializing member variables with default layer number (0), used for single die FPGA
    SwitchblockLookupKey(int set_x, int set_y, e_side set_from, e_side set_to) {
        this->set_coords(set_x, set_y, 0, set_from, set_to);
    }

    /// @brief Sets the coordinates
    void set_coords(int set_x, int set_y, int set_layer, e_side set_from, e_side set_to) {
        x_coord = set_x;
        y_coord = set_y;
        layer_coord = set_layer;
        from_side = set_from;
        to_side = set_to;
    }

    /// @brief Overload == operator which is used by std::unordered_map
    bool operator==(const SwitchblockLookupKey& obj) const {
        bool result;
        if (x_coord == obj.x_coord && y_coord == obj.y_coord
            && from_side == obj.from_side && to_side == obj.to_side
            && layer_coord == obj.layer_coord) {
            result = true;
        } else {
            result = false;
        }
        return result;
    }
};

struct t_hash_Switchblock_Lookup {
    size_t operator()(const SwitchblockLookupKey& obj) const noexcept {
        std::size_t hash = std::hash<int>{}(obj.x_coord);
        vtr::hash_combine(hash, obj.y_coord);
        vtr::hash_combine(hash, obj.layer_coord);
        vtr::hash_combine(hash, obj.from_side);
        vtr::hash_combine(hash, obj.to_side);
        return hash;
    }
};

/// @brief Contains the required information to build an RR graph edge for a switch block connection.
struct t_switchblock_edge {
    /// Source wire ptc_num index in a channel
    short from_wire;

    /// Destination wire ptc_num index in a channel
    short to_wire;

    /// RR graph switch index that connects the source wire to the destination wire that connect two tracks
    short switch_ind;
};

/**
 * @brief Switchblock connections are made as [x][y][from_layer][from_side][to_side][from_wire_idx].
 *
 * The Switchblock_Lookup class specifies these dimensions.
 * Furthermore, a source_wire at a given 6-d coordinate may connect to multiple destination wires
 * so the value of the map is a vector of destination wires.
 *
 * A matrix specifying connections for all switchblocks in an FPGA would be sparse and possibly very large,
 * so we use an unordered map to take advantage of the sparsity.
 */
typedef std::unordered_map<SwitchblockLookupKey, std::vector<t_switchblock_edge>, t_hash_Switchblock_Lookup> t_sb_connection_map;

/************ Functions ************/

/**
 * @brief Allocates and builds switch block permutation map
 *
 *   @param chan_details_x channel-x details (length, start and end points, ...)
 *   @param chan_details_y channel-y details (length, start and end points, ...)
 *   @param grid device grid
 *   @param inter_cluster_rr used to check if a certain layer contain inter-cluster programmable routing resources (wires and switch blocks)
 *   @param switchblocks switch block information extracted from the architecture file
 *   @param nodes_per_chan number of track in each channel (x,y)
 *   @param directionality specifies the switch block edges direction (unidirectional or bidirectional)
 *   @param rng the random number generator (RNG)
 *
 *   @return creates a map between switch blocks (key) and their corresponding edges (value).
 */
t_sb_connection_map* alloc_and_load_switchblock_permutations(const t_chan_details& chan_details_x,
                                                             const t_chan_details& chan_details_y,
                                                             const DeviceGrid& grid,
                                                             const std::vector<bool>& inter_cluster_rr,
                                                             const std::vector<t_switchblock_inf>& switchblocks,
                                                             const t_chan_width& nodes_per_chan,
                                                             e_directionality directionality,
                                                             vtr::RngContainer& rng);

/**
 * @brief deallocates switch block connections sparse array
 *
 *  @param sb_conns switch block permutation map
 */
void free_switchblock_permutations(t_sb_connection_map* sb_conns);
