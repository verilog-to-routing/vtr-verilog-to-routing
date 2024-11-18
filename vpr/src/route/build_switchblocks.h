#ifndef BUILD_SWITCHBLOCKS_H
#define BUILD_SWITCHBLOCKS_H

#include <unordered_map>
#include <vector>
#include <random>
#include "physical_types.h"
#include "vpr_types.h"
#include "device_grid.h"

#include "vtr_random.h"

/************ Classes, structs, typedefs ************/

/* Holds the coordinates of a switch block source connection. Used to index into a
 * map which specifies which destination wire segments this source wire should		//TODO: what data structure does this index to?
 * connect to */
class Switchblock_Lookup {
  public:
    int x_coord; /* x coordinate of switchblock connection */ //TODO: redundant comment?? add range
    int y_coord;                                              /* y coordinate of switchblock connection */
    int layer_coord;                                          /* layer number of switchblock */
    e_side from_side;                                         /* source side of switchblock connection */
    e_side to_side;                                           /* destination side of switchblock connection */

    /* Empty constructor initializes everything to 0 */
    Switchblock_Lookup() {
        x_coord = y_coord = layer_coord = -1; //TODO: use set function
    }

    /* Constructor for initializing member variables */
    Switchblock_Lookup(int set_x, int set_y, int set_layer, e_side set_from, e_side set_to) {
        this->set_coords(set_x, set_y, set_layer, set_from, set_to); //TODO: use set function
    }

    /* Constructor for initializing member variables with default layer number (0), used for single die FPGA */
    Switchblock_Lookup(int set_x, int set_y, e_side set_from, e_side set_to) {
        this->set_coords(set_x, set_y, 0, set_from, set_to);
    }

    /* Function for setting the segment coordinates */
    void set_coords(int set_x, int set_y, int set_layer, e_side set_from, e_side set_to) {
        x_coord = set_x;
        y_coord = set_y;
        layer_coord = set_layer;
        from_side = set_from;
        to_side = set_to;
    }

    /* Overload == operator which is used by std::unordered_map */
    bool operator==(const Switchblock_Lookup& obj) const {
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
    size_t operator()(const Switchblock_Lookup& obj) const noexcept {
        std::size_t hash = std::hash<int>{}(obj.x_coord);
        vtr::hash_combine(hash, obj.y_coord);
        vtr::hash_combine(hash, obj.layer_coord);
        vtr::hash_combine(hash, obj.from_side);
        vtr::hash_combine(hash, obj.to_side);
        return hash;
    }
};

/**
 * @brief contains the required information to build an RR graph edge for a switch block connection
 *
 *  @from_wire source wire ptc_num index in a channel
 *  @to_wire destination wire ptc_num index in a channel
 *  @switch_ind RR graph switch index that connects the source wire to the destination wire that connect two tracks in same layer
 *  @switch_ind_between_layers RR graph switch index that connects two tracks in different layers
 *  @from_wire_layer the layer index that the source wire is located at
 *  @to_wire_layer the layer index that the destination wire is located at
 *
 */
struct t_switchblock_edge {
    short from_wire;
    short to_wire;
    short switch_ind;
    short switch_ind_between_layers;
    short from_wire_layer;
    short to_wire_layer;
};

/* Switchblock connections are made as [x][y][from_side][to_side][from_wire_ind].
 * The Switchblock_Lookup class specifies these dimensions.
 * Furthermore, a source_wire at a given 5-d coordinate may connect to multiple destination wires so the value
 * of the map is a vector of destination wires.
 * A matrix specifying connections for all switchblocks in an FPGA would be sparse and possibly very large
 * so we use an unordered map to take advantage of the sparsity. */
typedef std::unordered_map<Switchblock_Lookup, std::vector<t_switchblock_edge>, t_hash_Switchblock_Lookup> t_sb_connection_map;

/************ Functions ************/

/**
 * @brief allocates and builds switch block permutation map
 *
 *   @param chan_details_x channel-x details (length, start and end points, ...)
 *   @param chan_details_y channel-y details (length, start and end points, ...)
 *   @param grid device grid
 *   @param inter_cluster_rr used to check if a certain layer contain inter-cluster programmable routing resources (wires and switch blocks)
 *   @param switchblocks switch block information extracted from the architecture file
 *   @param nodes_per_chan number of track in each channel (x,y)
 *   @param directionality specifies the switch block edges direction (unidirectional or bidirectional)
 *   @param rand_state initialize the random number generator (RNG)
 *
 *   @return creates a map between switch blocks (key) and their corresponding edges (value).
 */
t_sb_connection_map* alloc_and_load_switchblock_permutations(const t_chan_details& chan_details_x,
                                                             const t_chan_details& chan_details_y,
                                                             const DeviceGrid& grid,
                                                             const std::vector<bool>& inter_cluster_rr,
                                                             const std::vector<t_switchblock_inf>& switchblocks,
                                                             t_chan_width* nodes_per_chan,
                                                             enum e_directionality directionality,
                                                             vtr::RngContainer& rng);

/**
 * @brief deallocates switch block connections sparse array
 *
 *  @param sb_conns switch block permutation map
 */
void free_switchblock_permutations(t_sb_connection_map* sb_conns);

#endif
