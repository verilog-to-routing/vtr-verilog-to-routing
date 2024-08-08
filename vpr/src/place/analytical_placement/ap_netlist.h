/**
 * @file
 * @brief This file defines the APNetlist class used to store and manipulate
 *        primitives in the Analytical Placement context.
 *
 * TODO: Add an overview
 *
 */

#pragma once

#include <string>
#include "netlist.h"
#include "ap_netlist_fwd.h"

class t_pack_molecule;

// Struct to store fixed block location information.
// Currently assumes that blocks are fixed to single locations (not ranges).
// TODO: This assumption could be relaxed and allow fixing a range of locations.
// -1 implies that the block is not fixed in that dimension.
struct APFixedBlockLoc {
    int x = -1;
    int y = -1;
    int layer_num = -1;
    int sub_tile = -1;
};

// The type of a block in the APNetlist
// TODO: It would be nice if the netlist contained lists of moveable and fixed
//       block ids.
enum class APBlockType : bool {
    MOVEABLE,   // The block is not constrained in any dimension.
    FIXED       // The block is fixed.
};

class APNetlist : public Netlist<APBlockId, APPortId, APPinId, APNetId> {
public:
    /**
     * @brief Constructs a netlist
     *
     *  @param name The name of the netlist (e.g. top-level module)
     *  @param id   A unique identifier for the netlist (e.g. a secure digest of the input file)
     */
    APNetlist(std::string name = "", std::string id = "") : Netlist(name, id) {}

    APNetlist(const APNetlist& rhs) = default;
    APNetlist& operator=(const APNetlist& rhs) = default;

public: // Public Accessors
    /*
     * Blocks
     */

    ///@brief Returns the molecule that this block represents.
    const t_pack_molecule* block_molecule(const APBlockId id) const;

    ///@biref Returns the type of this block.
    APBlockType block_type(const APBlockId id) const;

    ///@brief Returns the location of this block, if the block is fixed.
    ///       This method should not be used if the block is moveable.
    const APFixedBlockLoc& block_loc(const APBlockId id) const;

public: // Public Mutators
    /*
     * Note: all create_*() functions will silently return the appropriate ID if it has already been created.
     */

    /**
     * @brief Create or return an existing block in the netlist
     *
     *  @param name The unique name of the block
     *  @param mol  The molecule the block represents
     */
    APBlockId create_block(const std::string& name, const t_pack_molecule* mol);

    /**
     * @brief Fixes a block at the given location
     *
     *  @param id  The block to fix
     *  @param loc The location to fix the block to
     */
    void set_block_loc(const APBlockId id, const APFixedBlockLoc& loc);

    /**
     * @brief Create or return an existing port in the netlist
     *
     *  @param blk_id The block the port is associated with
     *  @param name   The name of the port
     *  @param width  The width (number of bits) of the port
     *  @param type   The type of the port (INPUT, OUTPUT, or CLOCK)
     */
    APPortId create_port(const APBlockId blk_id, const std::string& name, BitIndex width, PortType type);

    /**
     * @brief Create or return an existing pin in the netlist
     *
     *  @param port_id  The port this pin is associated with
     *  @param port_bit The bit index of the pin in the port
     *  @param net_id   The net the pin drives/sinks
     *  @param pin_type The type of the pin (driver/sink)
     *  @param is_const Indicates whether the pin holds a constant value (e.g. vcc/gnd)
     */
    APPinId create_pin(const APPortId port_id, BitIndex port_bit, const APNetId net_id, const PinType pin_type, bool is_const = false);

    /**
     * @brief Create an empty, or return an existing net in the netlist
     *
     *  @param name The unique name of the net
     */
    APNetId create_net(const std::string& name);

private: // Private Members
    /*
     * Netlist compression / optimization
     */

    ///@brief Removes invalid components and reorders them
    void clean_blocks_impl(const vtr::vector_map<APBlockId, APBlockId>& block_id_map) override;
    void clean_ports_impl(const vtr::vector_map<APPortId, APPortId>& port_id_map) override;
    void clean_pins_impl(const vtr::vector_map<APPinId, APPinId>& pin_id_map) override;
    void clean_nets_impl(const vtr::vector_map<APNetId, APNetId>& net_id_map) override;

    void rebuild_block_refs_impl(const vtr::vector_map<APPinId, APPinId>& pin_id_map, const vtr::vector_map<APPortId, APPortId>& port_id_map) override;
    void rebuild_port_refs_impl(const vtr::vector_map<APBlockId, APBlockId>& block_id_map, const vtr::vector_map<APPinId, APPinId>& pin_id_map) override;
    void rebuild_pin_refs_impl(const vtr::vector_map<APPortId, APPortId>& port_id_map, const vtr::vector_map<APNetId, APNetId>& net_id_map) override;
    void rebuild_net_refs_impl(const vtr::vector_map<APPinId, APPinId>& pin_id_map) override;

    ///@brief Shrinks internal data structures to required size to reduce memory consumption
    void shrink_to_fit_impl() override;

    /*
     * Component removal
     */
    void remove_block_impl(const APBlockId blk_id) override;
    void remove_port_impl(const APPortId port_id) override;
    void remove_pin_impl(const APPinId pin_id) override;
    void remove_net_impl(const APNetId net_id) override;

    /*
     * Sanity checks
     */
    //Verify the internal data structure sizes match
    bool validate_block_sizes_impl(size_t num_blocks) const override;
    bool validate_port_sizes_impl(size_t num_ports) const override;
    bool validate_pin_sizes_impl(size_t num_pins) const override;
    bool validate_net_sizes_impl(size_t num_nets) const override;

private: // Private Data
    // Block data
    vtr::vector_map<APBlockId, const t_pack_molecule*> block_molecules_;    // Molecule of each block
    vtr::vector_map<APBlockId, APBlockType> block_types_;
    // NOTE: This vector will likely be quite sparse; however, it will
    vtr::vector_map<APBlockId, APFixedBlockLoc> block_locs_;
};

