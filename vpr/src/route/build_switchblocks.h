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
    e_side from_side;                                         /* source side of switchblock connection */
    e_side to_side;                                           /* destination side of switchblock connection */

    /* Empty constructor initializes everything to 0 */
    Switchblock_Lookup() {
        x_coord = y_coord = -1; //TODO: use set function
    }

    /* Constructor for initializing member variables */
    Switchblock_Lookup(int set_x, int set_y, e_side set_from, e_side set_to) {
        this->set_coords(set_x, set_y, set_from, set_to); //TODO: use set function
    }

    /* Function for setting the segment coordinates */
    void set_coords(int set_x, int set_y, e_side set_from, e_side set_to) {
        x_coord = set_x;
        y_coord = set_y;
        from_side = set_from;
        to_side = set_to;
    }

    /* Overload == operator which is used by std::unordered_map */
    bool operator==(const Switchblock_Lookup& obj) const {
        bool result;
        if (x_coord == obj.x_coord && y_coord == obj.y_coord
            && from_side == obj.from_side && to_side == obj.to_side) {
            result = true;
        } else {
            result = false;
        }
        return result;
    }
};

struct t_hash_Switchblock_Lookup {
    size_t operator()(const Switchblock_Lookup& obj) const noexcept {
        //TODO: use vtr::hash_combine
        size_t result;
        result = ((((std::hash<int>()(obj.x_coord)
                     ^ std::hash<int>()(obj.y_coord) << 10)
                    ^ std::hash<int>()((int)obj.from_side) << 20)
                   ^ std::hash<int>()((int)obj.to_side) << 30));
        return result;
    }
};

/* contains the index of the destination wire segment within a channel
 * and the index of the switch used to connect to it */
struct t_switchblock_edge {
    short from_wire;
    short to_wire;
    short switch_ind;
};

/* Switchblock connections are made as [x][y][from_side][to_side][from_wire_ind].
 * The Switchblock_Lookup class specifies these dimensions.
 * Furthermore, a source_wire at a given 5-d coordinate may connect to multiple destination wires so the value
 * of the map is a vector of destination wires.
 * A matrix specifying connections for all switchblocks in an FPGA would be sparse and possibly very large
 * so we use an unordered map to take advantage of the sparsity. */
typedef std::unordered_map<Switchblock_Lookup, std::vector<t_switchblock_edge>, t_hash_Switchblock_Lookup> t_sb_connection_map;

/************ Functions ************/

/* allocate and build switch block permutation map */
t_sb_connection_map* alloc_and_load_switchblock_permutations(const t_chan_details& chan_details_x, const t_chan_details& chan_details_y, const DeviceGrid& grid, std::vector<t_switchblock_inf> switchblocks, t_chan_width* nodes_per_chan, enum e_directionality directionality, vtr::RandState& rand_state);

/* deallocates switch block connections sparse array */
void free_switchblock_permutations(t_sb_connection_map* sb_conns);

#endif
