#ifndef ATOM_NETLIST_H
#define ATOM_NETLIST_H

/**
 * @file
 * @brief This file defines the AtomNetlist class used to store and manipulate
 *        the primitive (or atom) netlist.
 *
 * Overview
 * ========
 * The AtomNetlist is derived from the Netlist class, and contains information on the primitives.
 * This includes basic components (Blocks, Ports, Pins, & Nets), and physical descriptions (t_model)
 * of the primitives.
 *
 * Most of the functionality relevant to components and their accessors/cross-accessors
 * is implemented in the Netlist class. Refer to netlist.(h|tpp) for more information.
 *
 *
 * Components
 * ==========
 * There are 4 components in the Netlist: Blocks, Ports, Pins, and Nets.
 * Each component has a unique ID in the netlist, as well as various associations to their
 * related components (e.g. A pin knows which port it belongs to, and what net it connects to)
 *
 * Blocks
 * ------
 * Blocks refer to the atoms (AKA primitives) that are in the the netlist. Each block contains
 * input/output/clock ports. Blocks have names, and various functionalities (LUTs, FFs, RAMs, ...)
 * Each block has an associated t_model, describing the physical properties.
 *
 * Ports
 * -----
 * Ports are composed of a set of pins that have specific directionality (INPUT, OUTPUT, or CLOCK).
 * The ports in the AtomNetlist are respective to the atoms. (i.e. the AtomNetlist does not contain
 * ports of a Clustered Logic Block). Each port has an associated t_model_port, describing the
 * physical properties.
 *
 * Pins
 * ----
 * Pins are single-wire input/outputs. They are part of a port, and are connected to a single net.
 *
 * Nets
 * ----
 * Nets in the AtomNetlist track the wiring connections between the atoms.
 *
 * Models
 * ------
 * There are two main models, the primitive itself (t_model) and the ports of that primitive (t_model_ports).
 * The models are created from the architecture file, and describe the physical properties of the atom.
 *
 * Truth Table
 * -----------
 * The AtomNetlist also contains a TruthTable for each block, which indicates what the LUTs contain.
 *
 *
 * Implementation
 * ==============
 * For all create_* functions, the AtomNetlist will wrap and call the Netlist's version as it contains
 * additional information that the base Netlist does not know about.
 *
 * All functions with suffix *_impl() follow the Non-Virtual Interface (NVI) idiom.
 * They are called from the base Netlist class to simplify pre/post condition checks and
 * prevent Fragile Base Class (FBC) problems.
 *
 * Refer to netlist.h for more information.
 *
 */
#include <vector>
#include <unordered_map>

#include "vtr_range.h"
#include "vtr_logic.h"
#include "vtr_vector_map.h"

#include "logic_types.h" //For t_model

#include "netlist.h"
#include "atom_netlist_fwd.h"

class AtomNetlist : public Netlist<AtomBlockId, AtomPortId, AtomPinId, AtomNetId> {
  public:
    /**
     * @brief Constructs a netlist
     *
     *   @param name  the name of the netlist (e.g. top-level module)
     *   @param id    a unique identifier for the netlist (e.g. a secure digest of the input file)
     */
    AtomNetlist(std::string name = "", std::string id = "");

    AtomNetlist(const AtomNetlist& rhs) = default;
    AtomNetlist& operator=(const AtomNetlist& rhs) = default;

  public: //Public types
    typedef std::vector<std::vector<vtr::LogicValue>> TruthTable;

  public: //Public Accessors
    /*
     * Blocks
     */
    void set_block_types(const t_model* inpad, const t_model* outpad);

    ///@brief Returns the type of the specified block
    AtomBlockType block_type(const AtomBlockId id) const;

    ///@brief Returns the model associated with the block
    const t_model* block_model(const AtomBlockId id) const;

    /**
     * @brief Returns the truth table associated with the block
     *
     * @note This is only non-empty for LUTs and Flip-Flops/latches.
     *
     * For LUTs the truth table stores the single-output cover representing the
     * logic function.
     *
     * For FF/Latches there is only a single entry representing the initial state
     */
    const TruthTable& block_truth_table(const AtomBlockId id) const;

    /*
     * Ports
     */

    /**
     * @brief Returns the model port of the specified port or nullptr if not
     *
     *   @param id  The ID of the port to look for
     */
    const t_model_ports* port_model(const AtomPortId id) const;

    /*
     * Lookups
     */

    /**
     * @brief Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
     * @note This method is typically more efficient than searching by name
     *
     *   @param blk_id The   ID of the block who's ports will be checked
     *   @param model_port   The port model to look for
     */
    AtomPortId find_atom_port(const AtomBlockId blk_id, const t_model_ports* model_port) const;

    /**
     * @brief Returns the AtomBlockId of the atom driving the specified pin if it exists or AtomBlockId::INVALID() if not
     *
     *   @param blk_id The   ID of the block whose ports will be checked
     *   @param model_port   The port model to look for
     *   @param port_bit     The pin number in this port
     */
    AtomBlockId find_atom_pin_driver(const AtomBlockId blk_id, const t_model_ports* model_port, const BitIndex port_bit) const;

    /**
     * @brief Returns the a set of aliases relative to the net name.
     *
     * If no aliases are found, returns a set with the original net name.
     *
     *   @param net_name   name of the net from which the aliases are extracted
     */
    std::unordered_set<std::string> net_aliases(const std::string net_name) const;

  public: //Public Mutators
    /*
     * Note: all create_*() functions will silently return the appropriate ID if it has already been created
     */

    /**
     * @brief Create or return an existing block in the netlist
     *
     *   @param name          The unique name of the block
     *   @param model         The primitive type of the block
     *   @param truth_table   The single-output cover defining the block's logic function
     *                        The truth_table is optional and only relevant for LUTs (where it describes the logic function)
     *                        and Flip-Flops/latches (where it consists of a single entry defining the initial state).
     */
    AtomBlockId create_block(const std::string name, const t_model* model, const TruthTable truth_table = TruthTable());

    /**
     * @brief Create or return an existing port in the netlist
     *
     *   @param blk_id      The block the port is associated with
     *   @param model_port  The model port the port is associated with
     */
    AtomPortId create_port(const AtomBlockId blk_id, const t_model_ports* model_port);

    /**
     * @brief Create or return an existing pin in the netlist
     *
     *   @param port_id    The port this pin is associated with
     *   @param port_bit   The bit index of the pin in the port
     *   @param net_id     The net the pin drives/sinks
     *   @param pin_type   The type of the pin (driver/sink)
     *   @param is_const   Indicates whether the pin holds a constant value (e. g. vcc/gnd)
     */
    AtomPinId create_pin(const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const PinType pin_type, bool is_const = false);

    /**
     * @brief Create an empty, or return an existing net in the netlist
     *
     *   @param name   The unique name of the net
     */
    AtomNetId create_net(const std::string name); //An empty or existing net

    /**
     * @brief Create a completely specified net from specified driver and sinks
     *
     *   @param name       The name of the net (Note: must not already exist)
     *   @param driver     The net's driver pin
     *   @param sinks      The net's sink pins
     */
    AtomNetId add_net(const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks);

    /**
     * @brief Adds a value to the net aliases set for a given net name in the net_aliases_map.
     *
     * If there is no key/value pair in the net_aliases_map, creates a new set and adds it to the map.
     *
     *   @param net_name        The net to be added to the map
     *   @param alias_net_name  The alias of the assigned clock net id
     */
    void add_net_alias(const std::string net_name, std::string alias_net_name);

  private: //Private members
    /*
     * Component removal
     */
    void remove_block_impl(const AtomBlockId blk_id) override;
    void remove_port_impl(const AtomPortId port_id) override;
    void remove_pin_impl(const AtomPinId pin_id) override;
    void remove_net_impl(const AtomNetId net_id) override;

    /*
     * Netlist compression/optimization
     */

    //Removes invalid components and reorders them
    void clean_blocks_impl(const vtr::vector_map<AtomBlockId, AtomBlockId>& block_id_map) override;
    void clean_ports_impl(const vtr::vector_map<AtomPortId, AtomPortId>& port_id_map) override;
    void clean_pins_impl(const vtr::vector_map<AtomPinId, AtomPinId>& pin_id_map) override;
    void clean_nets_impl(const vtr::vector_map<AtomNetId, AtomNetId>& net_id_map) override;

    void rebuild_block_refs_impl(const vtr::vector_map<AtomPinId, AtomPinId>& pin_id_map, const vtr::vector_map<AtomPortId, AtomPortId>& port_id_map) override;
    void rebuild_port_refs_impl(const vtr::vector_map<AtomBlockId, AtomBlockId>& block_id_map, const vtr::vector_map<AtomPinId, AtomPinId>& pin_id_map) override;
    void rebuild_pin_refs_impl(const vtr::vector_map<AtomPortId, AtomPortId>& port_id_map, const vtr::vector_map<AtomNetId, AtomNetId>& net_id_map) override;
    void rebuild_net_refs_impl(const vtr::vector_map<AtomPinId, AtomPinId>& pin_id_map) override;

    ///@brief Shrinks internal data structures to required size to reduce memory consumption
    void shrink_to_fit_impl() override;

    /*
     * Sanity checks
     */
    //Verify the internal data structure sizes match
    bool validate_block_sizes_impl(size_t num_blocks) const override;
    bool validate_port_sizes_impl(size_t num_ports) const override;
    bool validate_pin_sizes_impl(size_t num_pins) const override;
    bool validate_net_sizes_impl(size_t num_nets) const override;

  private: //Private data
    //Block data
    vtr::vector_map<AtomBlockId, const t_model*> block_models_;   //Architecture model of each block
    vtr::vector_map<AtomBlockId, TruthTable> block_truth_tables_; //Truth tables of each block

    // Input IOs and output IOs always exist and have their own architecture
    // models. While their models are already included in block_models_, we
    // also store direct pointers to them to make checks of whether a block is
    // an INPAD or OUTPAD fast, as such checks are common in some netlist
    // operations (e.g. clean-up of an input netlist).
    const t_model* inpad_model_;
    const t_model* outpad_model_;

    //Port data
    vtr::vector_map<AtomPortId, const t_model_ports*> port_models_; //Architecture port models of each port

    //Net aliases
    std::unordered_map<std::string, std::unordered_set<std::string>> net_aliases_map_;
};

#include "atom_lookup.h"

#endif
