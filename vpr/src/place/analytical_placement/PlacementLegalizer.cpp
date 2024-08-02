
#include "PlacementLegalizer.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "PartialPlacement.h"
#include "device_grid.h"
#include "globals.h"
#include "physical_types.h"
#include "vpr_context.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

namespace {

// A new strong ID for the placement tiles.
struct place_tile_id_tag {};
typedef vtr::StrongId<place_tile_id_tag, size_t> PlaceTileId;

// Struct to contain sub-tile information
struct PlaceSubTile {
    // Atoms currently placed in this sub_tile
    std::unordered_set<size_t> atoms_in_site;
};

// Struct to contain tile information
struct PlaceTile {
    // Tile Information
    //  - tile_index: index into tiles array, unique per tile.
    //  - physical_tile_type: physical tile type of this tile.
    //  - x: horizontal coordinate (far left corner)
    //  - y: vertical coordinate (far bottom corner)
    //  - layer_num: The layer of the tile (0 for single-die)
    // NOTE: There is no is_empty flag in here since we leave this up to the
    //       model to handle. There may be a reason to put things in an empty
    //       tile. This means that there may be 0 sub-tiles!
    PlaceTileId tile_index;
    const t_physical_tile_type& physical_tile_type;
    int x;
    int y;
    int layer_num;
    // Atom placement information
    //  - sub_tiles: the different sub_tiles on the tile that contain atoms (may
    //               be empty)
    //  - overflow_sub_tile: a "fake" sub-tile where unplaceable atoms go.
    //      - an unplaceable atom is one that cannot fit into the real sub-tiles.
    // TODO: Maybe combine these so that these become contiguous (N). -VB
    std::vector<PlaceSubTile> sub_tiles;
    PlaceSubTile overflow_sub_tile;
    // TODO: It may be a good idea to use a strong ID for the sub_tile.
    static constexpr size_t overflow_sub_tile_idx = std::numeric_limits<size_t>::max();
    // Neighbors of this tile (i.e. tiles directly adjacent to this tile).
    //  - These are other tiles that directly share a side with this tile.
    //  - Does not include diagonals (TODO: should it?)
    // FIXME: This should probably be left to the architecture model.
    std::vector<PlaceTileId> neighbors;

    PlaceTile(int tile_x, int tile_y, int tile_layer_num, size_t index,
              const t_physical_tile_type &tile_type)
                : tile_index(index), physical_tile_type(tile_type), x(tile_x),
                  y(tile_y), layer_num(tile_layer_num) {
        sub_tiles.resize(tile_type.capacity);
        // Safety check; We use the largest representable unsigned integer
        // as the overflow_sub_tile_idx. Therefore, we need to make sure
        // the number of sub_tiles never gets that large!
        // TODO: Make this a weaker assert level
        VTR_ASSERT(sub_tiles.size() <= overflow_sub_tile_idx);
    }

    // Static method to detect if the sub_tile_idx is representing the overflow
    // sub_tile.
    static inline bool is_overflow_sub_tile(size_t sub_tile_idx) {
        return sub_tile_idx == overflow_sub_tile_idx;
    }

    // Get the sub_tile at the given index.
    std::reference_wrapper<PlaceSubTile> get_sub_tile(size_t sub_tile_idx) {
        // If this is the overflow index, return the overflow tile.
        if (is_overflow_sub_tile(sub_tile_idx))
            return overflow_sub_tile;
        // TODO: Definately make these ones debug.
        VTR_ASSERT(!sub_tiles.empty());
        VTR_ASSERT(sub_tile_idx < sub_tiles.size());
        // Else, return the subtile.
        return sub_tiles[sub_tile_idx];
    }

    // Add an atom to a given sub_tile.
    void add_atom_to_sub_tile(size_t node_id, size_t sub_tile_idx) {
        PlaceSubTile& sub_tile = get_sub_tile(sub_tile_idx);
        sub_tile.atoms_in_site.insert(node_id);
    }

    // Remove an atom from the given sub_tile.
    void remove_atom_from_sub_tile(size_t node_id, size_t sub_tile_idx) {
        PlaceSubTile& sub_tile = get_sub_tile(sub_tile_idx);
        sub_tile.atoms_in_site.erase(node_id);
    }

    // Clear all of the atoms from the tile.
    void clear() {
        for (PlaceSubTile &sub_tile : sub_tiles) {
            sub_tile.atoms_in_site.clear();
        }
        overflow_sub_tile.atoms_in_site.clear();
    }
};

// A graph used for storing the atoms in valid tile locations.
class PlaceTileGraph {
    // A vector of all the tiles. This is where the real tile information is
    // stored.
    vtr::vector<PlaceTileId, PlaceTile> tiles;
    // Grid of indices for each tile. Used only for using grid position to
    // find the tile. Will return the SAME tile if it covers different locations.
    vtr::NdMatrix<PlaceTileId, 2> tile_grid;
    // Position information within the graph for a given atom.
    struct PlaceGraphPos {
        PlaceTileId tile_id;
        size_t sub_tile_index;
    };
    // Lookup table to quickly get the location of the given atom.
    // TODO: Maybe this can be made a vector.
    std::unordered_map<size_t, PlaceGraphPos> atom_node_id_pos;
    // Set of tiles with any nodes in their overfilled sub_tile. This is used
    // to make finding all the overfilled tiles much faster.
    std::unordered_set<PlaceTileId> overfilled_tile_ids;
public:
    PlaceTileGraph() = delete;
    PlaceTileGraph(const DeviceContext& device) : tile_grid({device.grid.width(), device.grid.height()}) {
        // Construct each of the tiles.
        size_t grid_width = device.grid.width();
        size_t grid_height = device.grid.height();
        for (size_t x = 0; x < grid_width; x++) {
            for (size_t y = 0; y < grid_height; y++) {
                // Ignoring 3D placement for now. Will likely require modification to
                // the solver and legalizer.
                t_physical_tile_loc tile_loc = {(int)x, (int)y, 0};
                // Is this the root location? Only create tiles for roots.
                size_t width_offset = device.grid.get_width_offset(tile_loc);
                size_t height_offset = device.grid.get_height_offset(tile_loc);
                if (width_offset != 0 || height_offset != 0) {
                    // If this is not a root, point the tile_grid to the root
                    // tile location.
                    tile_grid[x][y] = tile_grid[x - width_offset][y - height_offset];
                    continue;
                }
                // Create the PlaceTile and insert into tiles array.
                t_physical_tile_type_ptr physical_type = device.grid.get_physical_type(tile_loc);
                size_t tile_index = tiles.size();
                const PlaceTile& place_tile = tiles.emplace_back(tile_loc.x, tile_loc.y, tile_loc.layer_num, tile_index, *physical_type);
                tile_grid[x][y] = place_tile.tile_index;
            }
        }
        // Connect the tiles through neighbors.
        for (size_t x = 0; x < grid_width; x++) {
            for (size_t y = 0; y < grid_height; y++) {
                // Ignoring 3D placement for now. Will likely require modification to
                // the solver and legalizer.
                t_physical_tile_loc tile_loc = {(int)x, (int)y, 0};
                // Is this the root location?
                if (device.grid.get_width_offset(tile_loc) != 0 ||
                    device.grid.get_height_offset(tile_loc) != 0) {
                    continue;
                }
                PlaceTile &place_tile = tiles[tile_grid[x][y]];
                size_t tile_width = place_tile.physical_tile_type.width;
                size_t tile_height = place_tile.physical_tile_type.height;
                // Add the neighbors.
                std::unordered_set<PlaceTileId> neighbor_tile_ids;
                // Add unique tiles on left and right sides
                for (size_t ty = y; ty < y + tile_height; ty++) {
                    if (x >= 1)
                        neighbor_tile_ids.insert(tile_grid[x - 1][ty]);
                    if (x <= grid_width - tile_width - 1)
                        neighbor_tile_ids.insert(tile_grid[x + tile_width][ty]);
                }
                // Add unique tiles on the top and bottom
                for (size_t tx = x; tx < x + tile_width; tx++) {
                    if (y >= 1)
                        neighbor_tile_ids.insert(tile_grid[tx][y - 1]);
                    if (y <= grid_height - tile_height - 1)
                        neighbor_tile_ids.insert(tile_grid[tx][y + tile_height]);
                }
                // Insert the neighbors into the tile
                place_tile.neighbors.assign(neighbor_tile_ids.begin(), neighbor_tile_ids.end());
            }
        }
    }

    // Helper method for getting the number of tiles in the graph.
    size_t get_num_tiles() const {
        return tiles.size();
    }

    // Get a reference to the tile with the given ID.
    std::reference_wrapper<const PlaceTile> get_place_tile(PlaceTileId tile_id) const {
        return tiles[tile_id];
    }

    // Get the tile ID for the tile at the given x and y location.
    PlaceTileId get_place_tile_id(int x, int y) const {
        // TODO: Make assert debug
        VTR_ASSERT(x >= 0 && static_cast<size_t>(x) < tile_grid.dim_size(0));
        VTR_ASSERT(y >= 0 && static_cast<size_t>(y) < tile_grid.dim_size(1));
        return tile_grid[x][y];
    }

    // Get the tile that contains the given node_id
    PlaceTileId get_containing_tile(size_t node_id) {
        // TODO: Make assert debug.
        VTR_ASSERT(atom_node_id_pos.find(node_id) != atom_node_id_pos.end());
        return atom_node_id_pos[node_id].tile_id;
    }

    // Get the sub_tile_idx that contains the given node_id
    size_t get_containing_sub_tile_idx(size_t node_id) {
        // TODO: Make assert debug.
        VTR_ASSERT(atom_node_id_pos.find(node_id) != atom_node_id_pos.end());
        return atom_node_id_pos[node_id].sub_tile_index;
    }

    // Helper method to add an atom to a sub_tile.
    void add_atom_to_sub_tile(size_t node_id, PlaceTileId tile_id, size_t sub_tile_idx) {
        // TODO: Make assert debug.
        VTR_ASSERT(atom_node_id_pos.find(node_id) == atom_node_id_pos.end());
        // Add the atom to the sub_tile and update the position information.
        PlaceTile& place_tile = tiles[tile_id];
        place_tile.add_atom_to_sub_tile(node_id, sub_tile_idx);
        atom_node_id_pos[node_id] = PlaceGraphPos({place_tile.tile_index, sub_tile_idx});

        // If anything is ever inserted into the overflow sub-tile, the tile
        // is now overfilled.
        if (place_tile.is_overflow_sub_tile(sub_tile_idx))
            overfilled_tile_ids.insert(place_tile.tile_index);
    }

    // Helper method to add an atom to the overflow sub_tile of a tile.
    void add_atom_to_overflow(size_t node_id, PlaceTileId tile_id) {
        add_atom_to_sub_tile(node_id, tile_id, PlaceTile::overflow_sub_tile_idx);
    }

    // Helper method to move an atom to the target sub_tile.
    // NOTE: It is assumed that the atom is already somewhere in the graph.
    void move_atom_to_sub_tile(size_t node_id, PlaceTileId target_tile_id, size_t target_sub_tile_idx) {
        // TODO: Make assert debug.
        VTR_ASSERT(atom_node_id_pos.find(node_id) != atom_node_id_pos.end());
        // Remove the atom from its original tile,sub-tile
        PlaceGraphPos& atom_pos = atom_node_id_pos[node_id];
        PlaceTile& from_tile = tiles[atom_pos.tile_id];
        from_tile.remove_atom_from_sub_tile(node_id, atom_pos.sub_tile_index);
        // Add the atom to its new tile,sub-tile
        PlaceTile& target_tile = tiles[target_tile_id];
        target_tile.add_atom_to_sub_tile(node_id, target_sub_tile_idx);
        // Update the storage
        atom_pos.tile_id = target_tile.tile_index;
        atom_pos.sub_tile_index = target_sub_tile_idx;

        // Check if the source tile is now not overfilled
        if (from_tile.overflow_sub_tile.atoms_in_site.empty())
            overfilled_tile_ids.erase(from_tile.tile_index);
        // If anything is ever inserted into the overflow sub-tile, the tile
        // is now overfilled.
        if (target_tile.is_overflow_sub_tile(target_sub_tile_idx))
            overfilled_tile_ids.insert(target_tile.tile_index);
    }

    // Helper metod to move an atom to the overflow sub_tile of the target tile.
    void move_atom_to_overflow(size_t node_id, PlaceTileId tile_id) {
        move_atom_to_sub_tile(node_id, tile_id, PlaceTile::overflow_sub_tile_idx);
    }

    // Get a list of overfilled tiles (tiles which have any atoms in the
    // overflow sub_tile).
    std::reference_wrapper<const std::unordered_set<PlaceTileId>> get_overfilled_tiles() const {
        return overfilled_tile_ids;
    }

    // Helper method to clear all of the atom information from the graph.
    // NOTE: The tile information will remain.
    void clear() {
        for (PlaceTile& tile : tiles) {
            tile.clear();
        }
        atom_node_id_pos.clear();
    }
};

/*
 * A simple architecture model
 *
 * This model treats each tile as a bin which can contain a certain number of
 * atoms. It does not look at their type, so as such it is homogenous.
 */
class SimpleArchModel {
    // The tile graph.
    // TODO: Perhaps the SimpleArchModel should inherit from the PlaceTileGraph
    //       class.
    PlaceTileGraph tile_graph;
    // This is a terrible way to do this. But doing this for now to integrate
    // with the old algorithm. This should be a dynamic value calculated per
    // tile.
    // FIXME:
    static constexpr size_t BIN_CAPACITY = 8;

    // Helper function to return the amount of "valid" space in the tile.
    // In other words how many more atoms can fit in this tile legally.
    size_t get_space_in_tile(PlaceTileId tile_id) const {
        const PlaceTile& place_tile = tile_graph.get_place_tile(tile_id);
        // If there are no sub_tiles, then there is no space in the tile.
        if (place_tile.sub_tiles.size() == 0)
            return 0;
        // If there are sub_tiles, the simple model only puts things in the
        // first sub_tile.
        size_t num_atoms_in_st = place_tile.sub_tiles[0].atoms_in_site.size();
        // TODO: Make this assert debug.
        VTR_ASSERT(num_atoms_in_st <= BIN_CAPACITY);
        return BIN_CAPACITY - num_atoms_in_st;
    }

    // Helper method to add an atom to a given tile.
    // NOTE: This was made private since removing / adding an atom from/to the
    //       tile graph does not make sense during operation of the legalizer.
    void add_atom_to_tile(size_t node_id, PlaceTileId tile_id) {
        if (get_space_in_tile(tile_id) > 0)
            tile_graph.add_atom_to_sub_tile(node_id, tile_id, 0);
        else
            tile_graph.add_atom_to_overflow(node_id, tile_id);
    }

public:
    SimpleArchModel(const DeviceContext& device_ctx) : tile_graph(device_ctx) {}

    // Getter methods to get information on the tile graph.
    // FIXME: If the ArchModel inherited from the tile graph it could have
    //        these already.
    size_t get_num_tiles() const {
        return tile_graph.get_num_tiles();
    }

    std::reference_wrapper<const PlaceTile> get_place_tile(PlaceTileId tile_id) const {
        return tile_graph.get_place_tile(tile_id);
    }

    std::reference_wrapper<const std::unordered_set<PlaceTileId>> get_overfilled_tiles() const {
        return tile_graph.get_overfilled_tiles();
    }

    // Gets all of the moveable nodes in a given tile (regardless of if they are
    // in the overflow or not.
    std::vector<size_t> get_all_moveable_nodes(const PlaceTileId tile_id, const PartialPlacement& p_placement) {
        // FIXME: If this becomes a bottleneck, it may be a good idea to store
        //        this information in the class.
        // TODO: Vaughns comment on the overflow subtile being part of the subtiles
        //       would make this cleaner.
        const PlaceTile& place_tile = tile_graph.get_place_tile(tile_id);
        std::vector<size_t> all_nodes;
        for (const PlaceSubTile& sub_tile : place_tile.sub_tiles) {
            const auto& atoms_in_site = sub_tile.atoms_in_site;
            for (size_t node_id : atoms_in_site) {
                if (!p_placement.is_moveable_node(node_id))
                    continue;
                all_nodes.push_back(node_id);
            }
        }
        const auto& overflow_atoms = place_tile.overflow_sub_tile.atoms_in_site;
        for (size_t node_id : overflow_atoms) {
            if (!p_placement.is_moveable_node(node_id))
                continue;
            all_nodes.push_back(node_id);
        }
        return all_nodes;
    }

    // Gets the number of atoms in the overflow sub_tile.
    size_t get_num_overflowing_nodes(const PlaceTileId tile_id) const {
        const PlaceTile& tile = tile_graph.get_place_tile(tile_id);
        return tile.overflow_sub_tile.atoms_in_site.size();
    }

    // Move an atom to the given tile.
    void move_atom_to_tile(size_t node_id, PlaceTileId target_tile_id) {
        // Simple model just moves nodes.
        // Get the old position.
        PlaceTileId source_tile_id = tile_graph.get_containing_tile(node_id);
        size_t old_sub_tile_idx = tile_graph.get_containing_sub_tile_idx(node_id);
        // Move the atom
        if (get_space_in_tile(target_tile_id) > 0)
            tile_graph.move_atom_to_sub_tile(node_id, target_tile_id, 0);
        else
            tile_graph.move_atom_to_overflow(node_id, target_tile_id);
        // Fix-up the from tile:
        //  - if the atom was not from the overflow, move something from the
        //    overflow (if anything) into the sub_tile
        const PlaceTile& source_tile = tile_graph.get_place_tile(source_tile_id);
        if (!PlaceTile::is_overflow_sub_tile(old_sub_tile_idx) &&
            source_tile.overflow_sub_tile.atoms_in_site.size() > 0) {
            size_t atom_to_move = *source_tile.overflow_sub_tile.atoms_in_site.begin();
            tile_graph.move_atom_to_sub_tile(atom_to_move, source_tile_id, 0);
        }
    }

    // Gets how much the tile is being overused by nodes of the same type as node_id
    float get_tile_overuse(const PlaceTileId tile_id, size_t node_id, const PartialPlacement& p_placement) const {
        // For the simple, homogenous model, all nodes have the same type.
        // Other models may use this information.
        (void)node_id;
        (void)p_placement;
        // For the simple model, by definition, the overuse of a tile is the
        // number of nodes in the overflow sub-tile.
        return static_cast<float>(get_num_overflowing_nodes(tile_id));
    }

    // Gets how much the tile is being underused by nodes of the same type as node_id.
    // In other words, what is the largest number of additional nodes of this
    // type that this tile can support.
    float get_tile_underuse(const PlaceTileId tile_id, size_t node_id, const PartialPlacement& p_placement) const {
        // For the simple, homogenous model, all nodes have the same type.
        // Other models may use this information.
        (void)node_id;
        (void)p_placement;
        // For the simple model, if there is anything in the overflow sub-tile
        // then this tile is not underused.
        const PlaceTile& place_tile = tile_graph.get_place_tile(tile_id);
        if (!place_tile.overflow_sub_tile.atoms_in_site.empty())
            return 0.f;
        // Else return the amount of space left in the sub-tile
        return get_space_in_tile(tile_id);
    }

    // Import the atom locations from the partial placement into the tile graph.
    void import_node_locations(const PartialPlacement& p_placement) {
        // Insert the fixed nodes first. It is the task of the architecture model
        // to ensure that fixed nodes do not get moved beyond where they are allowed.
        // The reason for this design decision is that the fixed_blocks may not
        // specify which sub-tile to fix the block, leaving some room for the
        // model to optimize.
        // For the simple model, this just means keeping them in the same tile.
        // For other models, they may have felixibility with which sub-tile to put
        // them on.
        for (size_t node_id = p_placement.num_moveable_nodes; node_id < p_placement.num_nodes; node_id++) {
            int tile_x = std::floor(p_placement.node_loc_x[node_id]);
            int tile_y = std::floor(p_placement.node_loc_y[node_id]);
            PlaceTileId tile_id = tile_graph.get_place_tile_id(tile_x, tile_y);
            add_atom_to_tile(node_id, tile_id);
            // Cannot allow any of the fixed nodes to go into the overflow
            // sub tile. This would imply that the fixed nodes cannot fit in
            // this tile.
            // This may need to be made into an if condition to gracefully error.
            VTR_ASSERT(!PlaceTile::is_overflow_sub_tile(tile_graph.get_containing_sub_tile_idx(node_id)));
        }
        // Insert the moveable nodes.
        for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
            int tile_x = std::floor(p_placement.node_loc_x[node_id]);
            int tile_y = std::floor(p_placement.node_loc_y[node_id]);
            PlaceTileId tile_id = tile_graph.get_place_tile_id(tile_x, tile_y);
            add_atom_to_tile(node_id, tile_id);
        }
    }

    // Export the atom locations from the tile graph into the partial placement.
    void export_node_locations(PartialPlacement& p_placement) {
        // TODO: Look into only updating the nodes that actually moved.
        for (size_t node_id = 0; node_id < p_placement.num_moveable_nodes; node_id++) {
            const PlaceTileId tile_id = tile_graph.get_containing_tile(node_id);
            const PlaceTile& place_tile = tile_graph.get_place_tile(tile_id);
            double node_x = p_placement.node_loc_x[node_id];
            double node_y = p_placement.node_loc_y[node_id];
            double offset_x = node_x - std::floor(node_x);
            double offset_y = node_y - std::floor(node_y);
            // FIXME: The solver will likely put many nodes right on top of one
            //        another. The legalizer should spready them out a bit to
            //        help it converge faster.
            p_placement.node_loc_x[node_id] = static_cast<double>(place_tile.x) + offset_x;
            p_placement.node_loc_y[node_id] = static_cast<double>(place_tile.y) + offset_y;
            // FIXME: The sub-tile information should also be stored in the
            //        PartialPlacement object.
            // tile_graph.get_containing_sub_tile_idx(node_id);
        }
    }

    // Clear all modified information.
    void clear() {
        tile_graph.clear();
    }
};

} // namespace

// Helper method to compute the phi term in the durav algorithm.
static inline double computeMaxMovement(size_t iter) {
    return 100 * (iter + 1) * (iter + 1);
}

// Helper method to get the supply of a tile.
static inline size_t getSupply(PlaceTileId tile_id, const SimpleArchModel &arch_model, const PartialPlacement &p_placement) {
    // FIXME: This is not following the paper, but a guess on my part. Will need
    //        to verify before we make these functions more complex.
    return arch_model.get_tile_overuse(tile_id, 0, p_placement);
}

// Helper method to get the demand of a tile.
static inline size_t getDemand(PlaceTileId tile_id, const SimpleArchModel &arch_model, const PartialPlacement &p_placement) {
    // FIXME: This is not following the paper, but a guess on my part. Will need
    //        to verify before we make these functions more complex.
    return arch_model.get_tile_underuse(tile_id, 0, p_placement);
}

// Helper method to get the closest node in the src tile to the sink tile.
// NOTE: This is from the original solved position of the node in the src tile.
static inline size_t getClosestNode(const PartialPlacement& p_placement,
                                    SimpleArchModel& arch_model,
                                    PlaceTileId src_tile_id,
                                    PlaceTileId sink_tile_id,
                                    double &smallest_block_dist) {
    const PlaceTile& sink_tile = arch_model.get_place_tile(sink_tile_id);
    double sink_tile_x = sink_tile.x;
    double sink_tile_y = sink_tile.y;

    std::vector<size_t> all_moveable_nodes = arch_model.get_all_moveable_nodes(src_tile_id, p_placement);
    VTR_ASSERT(!all_moveable_nodes.empty());
    size_t closest_node_id = 0;
    smallest_block_dist = std::numeric_limits<double>::infinity();
    for (size_t node_id : all_moveable_nodes) {
        // NOTE: Slight change from original implementation. Not getting tile pos.
        // FIXME: This distance calculation is a bit odd.
        double orig_node_x = p_placement.node_loc_x[node_id];
        double orig_node_y = p_placement.node_loc_y[node_id];
        double dx = orig_node_x - sink_tile_x;
        double dy = orig_node_y - sink_tile_y;
        double block_dist = (dx * dx) + (dy * dy);
        if (block_dist < smallest_block_dist) {
            closest_node_id = node_id;
            smallest_block_dist = block_dist;
        }
    }
    return closest_node_id;
}

// Helper method to compute the cost of moving objects from the src tile to the
// sink tile.
static inline double computeCost(const PartialPlacement& p_placement,
                                 SimpleArchModel& arch_model,
                                 double psi,
                                 PlaceTileId src_tile_id,
                                 PlaceTileId sink_tile_id) {
    // If the src tile is empty, then there is nothing to move.
    // FIXME: This can also be made WAY more efficient!
    if (arch_model.get_all_moveable_nodes(src_tile_id, p_placement).empty())
        return std::numeric_limits<double>::infinity();

    // Get the weight, which is proportional to the size of the set that
    // contains the src.
    // FIXME: What happens when this is zero?
    size_t src_supply = getSupply(src_tile_id, arch_model, p_placement);
    double wt = static_cast<double>(src_supply);

    // Get the minimum quadratic movement to move a cell from the src tile to
    // the sink tile.
    // NOTE: This assumes no diagonal movements.
    double quad_mvmt;
    getClosestNode(p_placement, arch_model, src_tile_id, sink_tile_id, quad_mvmt);

    // If the movement is larger than psi, return infinity
    if (quad_mvmt >= psi)
        return std::numeric_limits<double>::infinity();

    // Following the equation from the Darav paper.
    return wt * quad_mvmt;
}

// Helper method to get the paths to flow nodes between tiles.
static inline void getPaths(const PartialPlacement& p_placement,
                            SimpleArchModel& arch_model,
                            double psi,
                            PlaceTileId starting_tile_id,
                            std::vector<std::vector<PlaceTileId>>& paths) {
    // Create a visited vector.
    vtr::vector<PlaceTileId, bool> tile_visited(arch_model.get_num_tiles());
    std::fill(tile_visited.begin(), tile_visited.end(), false);
    tile_visited[starting_tile_id] = true;
    // Create a cost array. A path can be uniquely identified by its tail tile
    // cost.
    vtr::vector<PlaceTileId, double> tile_cost(arch_model.get_num_tiles());
    std::fill(tile_cost.begin(), tile_cost.end(), 0.0);
    // Create a starting path.
    std::vector<PlaceTileId> starting_path;
    starting_path.push_back(starting_tile_id);
    // Create a FIFO queue.
    std::queue<std::vector<PlaceTileId>> queue;
    queue.push(std::move(starting_path));

    size_t demand = 0;
    size_t starting_tile_supply = getSupply(starting_tile_id, arch_model, p_placement);
    while (!queue.empty() && demand < starting_tile_supply) {
        std::vector<PlaceTileId> &p = queue.front();
        PlaceTileId tail_tile_id = p.back();
        // Get the neighbors of the tail tile.
        const PlaceTile& tail_tile = arch_model.get_place_tile(tail_tile_id);
        const std::vector<PlaceTileId> &neighbor_tile_ids = tail_tile.neighbors;
        for (PlaceTileId neighbor_tile_id : neighbor_tile_ids) {
            if (tile_visited[neighbor_tile_id])
                continue;
            double cost = computeCost(p_placement, arch_model, psi, tail_tile_id, neighbor_tile_id);
            if (cost < std::numeric_limits<double>::infinity()) {
                std::vector<PlaceTileId> p_copy(p);
                tile_cost[neighbor_tile_id] = tile_cost[tail_tile_id] + cost;
                p_copy.push_back(neighbor_tile_id);
                tile_visited[neighbor_tile_id] = true;
                // NOTE: Needed to change this from the algorithm since it was
                //       skipping partially filled tiles. Maybe indicative of a
                //       bug somewhere.
                // if (arch_model.get_all_moveable_nodes(neighbor_tile_id, p_placement).empty())
                size_t neighbor_demand = getDemand(neighbor_tile_id, arch_model, p_placement);
                if (neighbor_demand > 0) {
                    paths.push_back(std::move(p_copy));
                    demand += neighbor_demand;
                } else {
                    queue.push(std::move(p_copy));
                }
            }
        }
        queue.pop();
    }

    // Sort the paths in increasing order of cost.
    std::sort(paths.begin(), paths.end(), [&](std::vector<PlaceTileId>& a, std::vector<PlaceTileId>& b) {
        return tile_cost[a.back()] < tile_cost[b.back()];
    });
}

// Helper method to move cells along a given path.
static inline void moveCells(const PartialPlacement& p_placement,
                             SimpleArchModel& arch_model,
                             double psi,
                             std::vector<PlaceTileId>& path) {
    VTR_ASSERT(!path.empty());
    PlaceTileId src_tile_id = path[0];
    std::stack<PlaceTileId> s;
    s.push(src_tile_id);
    size_t path_size = path.size();
    for (size_t path_index = 1; path_index < path_size; path_index++) {
        PlaceTileId sink_tile_id = path[path_index];
        double cost = computeCost(p_placement, arch_model, psi, src_tile_id, sink_tile_id);
        if (cost == std::numeric_limits<double>::infinity())
            return;
        //  continue;
        src_tile_id = sink_tile_id;
        s.push(sink_tile_id);
    }
    PlaceTileId sink_tile_id = s.top();
    s.pop();
    while (!s.empty()) {
        src_tile_id = s.top();
        s.pop();
        // Minor change to the algorithm proposed by Darav et al., find the
        // closest point in src to sink and move it to sink (instead of sorting
        // the whole list which is wasteful).
        // TODO: Verify this.
        double smallest_block_dist;
        size_t closest_node_id = getClosestNode(p_placement, arch_model, src_tile_id, sink_tile_id, smallest_block_dist);
        arch_model.move_atom_to_tile(closest_node_id, sink_tile_id);

        sink_tile_id = src_tile_id;
    }
}

// Flow-Based Legalizer based off work by Darav et al.
// https://dl.acm.org/doi/10.1145/3289602.3293896
// FIXME: This currently is not working since it treats LUTs and FFs as the same
//        block. Needs to be updated to handle heterogenous blocks.
void FlowBasedLegalizer::legalize(PartialPlacement &p_placement) {
    VTR_LOG("Running Flow-Based Legalizer\n");

    // Build the architecture model and the tile graph
    // FIXME: This should be moved within the leglalizer itself and reset after
    //        each iteration.
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    SimpleArchModel arch_model(device_ctx);

    // Import the node locations into the Tile Graph according to the architecture
    // model.
    arch_model.import_node_locations(p_placement);

    // Run the flow-based spreader.
    size_t flowBasedIter = 0;
    while (true) {
        if (flowBasedIter > 1000) {
            VTR_LOG("HIT MAX ITERATION!!!\n");
            break;
        }
        // Compute the max movement.
        double psi = computeMaxMovement(flowBasedIter);

        // Get the overfilled tiles and sort them in increasing order of supply.
        const auto& overfilled_tiles_set = arch_model.get_overfilled_tiles().get();
        std::vector<PlaceTileId> overfilled_tiles(overfilled_tiles_set.begin(), overfilled_tiles_set.end());
        if (overfilled_tiles.empty()) {
            // VTR_LOG("No overfilled tiles! No spreading needed!\n");
            break;
        }
        std::sort(overfilled_tiles.begin(), overfilled_tiles.end(), [&](PlaceTileId a, PlaceTileId b) {
            return getSupply(a, arch_model, p_placement) < getSupply(b, arch_model, p_placement);
        });
        // VTR_LOG("Num overfilled tile: %zu\n", overfilled_tiles.size());
        // VTR_LOG("\tLargest overfilled tile supply: %zu\n", getSupply(overfilled_tiles.back(), arch_model, p_placement));
        // VTR_LOG("\tpsi = %f\n", psi);

        for (PlaceTileId tile_id : overfilled_tiles) {
            // Get the list of candidate paths based on psi.
            //  NOTE: The paths are sorted by increasing cost within the
            //        getPaths method.
            std::vector<std::vector<PlaceTileId>> paths;
            getPaths(p_placement, arch_model, psi, tile_id, paths);

            // VTR_LOG("\tNum paths: %zu\n", paths.size());
            for (std::vector<PlaceTileId>& path : paths) {
                if (getSupply(tile_id, arch_model, p_placement) == 0)
                    continue;
                // Move cells over the paths.
                //  NOTE: This will modify the tiles. (actual block positions
                //        will not change (yet)).
                moveCells(p_placement, arch_model, psi, path);
            }
        }

        // TODO: Get the total cell displacement for debugging.

        flowBasedIter++;
    }
    VTR_LOG("Flow-Based Legalizer finished in %zu iterations.\n", flowBasedIter + 1);

    // Export the legalized placement to the partial placement.
    arch_model.export_node_locations(p_placement);
}

void FullLegalizer::legalize(PartialPlacement& p_placement) {
    (void)p_placement;
    VTR_LOG("Running Full Legalizer\n");
    VTR_ASSERT(false && "Full legalizer not implemented yet.");
}

