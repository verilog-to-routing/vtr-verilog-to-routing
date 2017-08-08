#ifndef ATOM_NETLIST_H
#define ATOM_NETLIST_H
/*
* Summary
* ========
* This file defines the AtomNetlist class used to store and manipulate the primitive (or atom) netlist.
*
*/
#include <vector>
#include <unordered_map>

#include "vtr_range.h"
#include "vtr_logic.h"
#include "vtr_vector_map.h"

#include "logic_types.h" //For t_model

#include "atom_netlist_fwd.h"

#include "netlist.h"

//Forward declaration for private methods
template<typename I>
class IdMap;

class AtomNetlist : public Netlist<AtomBlockId,AtomPortId,AtomPinId,AtomNetId> {
	public:
		//Constructs a netlist
		// name: the name of the netlist (e.g. top-level module)
		// id:   a unique identifier for the netlist (e.g. a secure digest of the input file)
		AtomNetlist(std::string name = "", std::string id = "");

    public: //Public types
        typedef std::vector<std::vector<vtr::LogicValue>> TruthTable;

    public: //Public Accessors
        /*
         * Blocks
         */
        //Returns the type of the specified block
        AtomBlockType       block_type          (const AtomBlockId id) const;

		//Returns the model associated with the block
		const t_model*      block_model(const AtomBlockId id) const;

        //Returns the truth table associated with the block
        // Note that this is only non-empty for LUTs and Flip-Flops/latches.
        //
        // For LUTs the truth table stores the single-output cover representing the
        // logic function.
        //
        // For FF/Latches there is only a single entry representing the initial state
        const TruthTable&   block_truth_table(const AtomBlockId id) const; 

        /*
         * Ports
         */
        //Returns the type of the specified port
        AtomPortType			port_type(const AtomPortId id) const; 

		//Returns the model port of the specified port or nullptr if not
		//  id: The ID of the port to look for
		const t_model_ports*    port_model(const AtomPortId id) const;

        /*
         * Pins
         */
        //Returns the pin type of the specified pin
        AtomPinType			pin_type(const AtomPinId id) const; 

        //Returns the port type associated with the specified pin
        AtomPortType		pin_port_type(const AtomPinId id) const;

		/*
		* Lookups
		*/
		//Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
		//Note that this method is typically more efficient than searching by name
		//  blk_id: The ID of the block who's ports will be checked
		//  model_port: The port model to look for
		AtomPortId  find_port(const AtomBlockId blk_id, const t_model_ports* model_port) const;

        /*
         * Utility
         */
        //Sanity check for internal consistency (throws an exception on failure)
        bool verify() const;

    public: //Public Mutators
        /*
         * Note: all create_*() functions will silently return the appropriate ID if it has already been created
         */

        //Create or return an existing block in the netlist
        //  name        : The unique name of the block
        //  model       : The primitive type of the block
        //  truth_table : The single-output cover defining the block's logic function
        //                The truth_table is optional and only relevant for LUTs (where it describes the logic function)
        //                and Flip-Flops/latches (where it consists of a single entry defining the initial state).
        AtomBlockId create_block(const std::string name, const t_model* model, const TruthTable truth_table=TruthTable());

        //Create or return an existing port in the netlist
        //  blk_id      : The block the port is associated with
        //  name        : The name of the port (must match the name of a port in the block's model)
        AtomPortId  create_port (const AtomBlockId blk_id, const t_model_ports* model_port);

        //Create or return an existing pin in the netlist
        //  port_id    : The port this pin is associated with
        //  port_bit   : The bit index of the pin in the port
        //  net_id     : The net the pin drives/sinks
        //  pin_type   : The type of the pin (driver/sink)
		//  port_type  : The type of the port (input/output/clock)
        //  is_const   : Indicates whether the pin holds a constant value (e. g. vcc/gnd)
        AtomPinId   create_pin  (const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const AtomPinType pin_type, const AtomPortType port_type, bool is_const=false);

        //Create an empty, or return an existing net in the netlist
        //  name    : The unique name of the net
        AtomNetId   create_net  (const std::string name); //An empty or existing net

        //Create a completely specified net from specified driver and sinks
        //  name    : The name of the net (Note: must not already exist)
        //  driver  : The net's driver pin
        //  sinks   : The net's sink pins
        AtomNetId   add_net     (const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks);

        //Mark a pin as being a constant generator.
        // There are some cases where a pin can not be identified as a is constant until after
        // the full netlist has been built; so we expose a way to mark existing pins as constants.
        //  pin_id  : The pin to be marked
        //  value   : The boolean value to set the pin_is_constant attribute
        void set_pin_is_constant(const AtomPinId pin_id, const bool value);

        //Add the specified pin to the specified net as pin_type. Automatically removes
        //any previous net connection for this pin.
        //  pin      : The pin to add
        //  pin_type : The type of the pin (i.e. driver or sink)
        //  net      : The net to add the pin to
        void set_pin_net(const AtomPinId pin, AtomPinType pin_type, const AtomNetId net);

        //Compresses the netlist, removing any invalid and/or unreferenced
        //blocks/ports/pins/nets.
        //
        //This should be called after completing a series of netlist modifications 
        //(e.g. removing blocks/ports/pins/nets).
        //
        //NOTE: this invalidates all existing IDs!
        void compress();

    private: //Private types
        //A unique identifier for a string in the atom netlist
        typedef vtr::StrongId<Netlist<AtomBlockId, AtomPortId, AtomPinId, AtomNetId>::string_id_tag> AtomStringId;

    private: //Private members
        /*
         * Netlist compression/optimization
         */
        //Removes invalid and reorders blocks
        void clean_blocks(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map);

        //Removes invalid and reorders ports
        void clean_ports(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map);

        //Shrinks internal data structures to required size to reduce memory consumption
        void shrink_to_fit();

        /*
         * Sanity Checks
         */
        //Verify the internal data structure sizes match
        bool verify_sizes() const; //All data structures
        bool validate_block_sizes() const;
		bool validate_port_sizes() const;

		//Verify that internal data structure cross-references are consistent
		bool verify_refs() const; //All cross-references
		bool validate_block_pin_refs() const;
		bool validate_port_pin_refs() const;

		bool valid_port_bit(AtomPortId id, BitIndex port_bit) const;

        //Verify that block invariants hold (i.e. logical consistency)
        bool verify_block_invariants() const;

    private: //Private data
        //Block data
		vtr::vector_map<AtomBlockId, const t_model*>		block_models_;             //Architecture model of each block
		vtr::vector_map<AtomBlockId,TruthTable>             block_truth_tables_;       //Truth tables of each block

		//Port data
		vtr::vector_map<AtomPortId, const t_model_ports*>   port_models_;   //Architecture port models of each port
};

#include "atom_lookup.h"

#endif
