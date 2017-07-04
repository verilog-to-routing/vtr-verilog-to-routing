/* 
 * Summary
 * =======
 * This file defines the BaseNetlist class, which stores the connectivity information 
 * of the components in a netlist. It is the base class for AtomNetlist and ClusteredNetlist.
 *
*/
#ifndef BASE_NETLIST_H
#define BASE_NETLIST_H

#include <string>
#include <vector>
#include <unordered_map>
#include "vtr_range.h"
#include "vtr_vector_map.h"

#include "logic_types.h"

#include "base_netlist_fwd.h"

//Forward declaration for private methods
template<typename I>
class IdMap;

class BaseNetlist {
	public: //Public Types
		typedef vtr::vector_map<BlockId, BlockId>::const_iterator block_iterator;
		typedef vtr::vector_map<NetId, NetId>::const_iterator net_iterator;
		typedef vtr::vector_map<PinId, PinId>::const_iterator pin_iterator;
		typedef vtr::vector_map<PortId, PortId>::const_iterator port_iterator;
		
		typedef vtr::Range<block_iterator> block_range;
		typedef vtr::Range<net_iterator> net_range;
		typedef vtr::Range<pin_iterator> pin_range;
		typedef vtr::Range<port_iterator> port_range;

	public: //Constructor
		BaseNetlist(std::string name="", std::string id="");

	public: //Public Mutators
		//Create or return an existing pin in the netlist
        //  port_id : The port this pin is associated with
        //  port_bit: The bit index of the pin in the port
        //  net_id  : The net the pin drives/sinks
        //  type    : The type of the pin (driver/sink)
        //  is_const: Indicates whether the pin holds a constant value (e. g. vcc/gnd)
        PinId   create_pin  (const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType type, bool is_const=false);

		//Create an empty, or return an existing net in the netlist
		//  name    : The unique name of the net
		NetId   create_net(const std::string name); //An empty or existing net

		//Create a completely specified net from specified driver and sinks
        //  name    : The name of the net (Note: must not already exist)
        //  driver  : The net's driver pin
        //  sinks   : The net's sink pins
        NetId   add_net     (const std::string name, PinId driver, std::vector<PinId> sinks);

		//Mark a pin as being a constant generator.
		// There are some cases where a pin can not be identified as a is constant until after
		// the full netlist has been built; so we expose a way to mark existing pins as constants.
		//  pin_id  : The pin to be marked
		//  value   : The boolean value to set the pin_is_constant attribute
		void set_pin_is_constant(const PinId pin_id, const bool value);

		//Add the specified pin to the specified net as pin_type. Automatically removes
		//any previous net connection for this pin.
		//  pin      : The pin to add
		//  pin_type : The type of the pin (i.e. driver or sink)
		//  net      : The net to add the pin to
		void set_pin_net(const PinId pin, PinType pin_type, const NetId net);

		//Removes a net from the netlist. 
        //This will mark the net's pins as having no associated.
        //  net_id  : The net to be removed
        void remove_net     (const NetId net_id);


	public: //Public Accessors
		/*
		* Netlist
		*/
		//Retrieve the name of the netlist
		const std::string&  netlist_name() const;

		//Retrieve the unique identifier for this netlist
		//This is typically a secure digest of the input file.
		const std::string&  netlist_id() const;

		//Item counts and container info (for debugging)
		void print_stats() const;

		/*
		* Blocks
		*/
		//Returns the name of the specified block
		const std::string&  block_name(const BlockId id) const;

		//Returns the type of the specified block
		BlockType       block_type(const BlockId id) const;

		//Returns the model associated with the block
		const t_model*      block_model(const BlockId id) const;

		//Returns true if the block is purely combinational (i.e. no input clocks
		//and not a primary input
		bool                block_is_combinational(const BlockId id) const;

		//Returns a range of all pins assoicated with the specified block
		pin_range           block_pins(const BlockId id) const;

		//Returns a range of all input pins assoicated with the specified block
		pin_range           block_input_pins(const BlockId id) const;

		//Returns a range of all output pins assoicated with the specified block
		// Note this is typically only data pins, but some blocks (e.g. PLLs) can produce outputs
		// which are clocks.
		pin_range           block_output_pins(const BlockId id) const;

		//Returns a range of all clock pins assoicated with the specified block
		pin_range           block_clock_pins(const BlockId id) const;
		
		//Returns a range of all ports assoicated with the specified block
		port_range          block_ports(const BlockId id) const;

		//Returns a range consisting of the input ports associated with the specified block
		port_range          block_input_ports(const BlockId id) const;

		//Returns a range consisting of the output ports associated with the specified block
		// Note this is typically only data ports, but some blocks (e.g. PLLs) can produce outputs
		// which are clocks.
		port_range          block_output_ports(const BlockId id) const;

		//Returns a range consisting of the input clock ports associated with the specified block
		port_range          block_clock_ports(const BlockId id) const;

		/*
		* Ports
		*/
		//Returns the name of the specified port
		const std::string&      port_name(const PortId id) const;

		//Returns the width (number of bits) in the specified port
		BitIndex                port_width(const PortId id) const;

		//Returns the block associated with the specified port
		BlockId             port_block(const PortId id) const;

		//Returns the type of the specified port
		PortType            port_type(const PortId id) const;

		//Returns the set of valid pins associated with the port
		pin_range               port_pins(const PortId id) const;

		//Returns the pin (potentially invalid) associated with the specified port and port bit index
		//  port_id : The ID of the associated port
		//  port_bit: The bit index of the pin in the port
		//Note: this function is a synonym for find_pin()
		PinId               port_pin(const PortId port_id, const BitIndex port_bit) const;

		//Returns the net (potentially invalid) associated with the specified port and port bit index
		//  port_id : The ID of the associated port
		//  port_bit: The bit index of the pin in the port
		NetId               port_net(const PortId port_id, const BitIndex port_bit) const;

		//Returns the model port of the specified port or nullptr if not
		//  port_id: The ID of the port to look for
		const t_model_ports*    port_model(const PortId port_id) const;

		/*
		* Pins
		*/
		//Returns the constructed name (derived from block and port) for the specified pin
		std::string  pin_name(const PinId id) const;

		//Returns the net associated with the specified pin
		NetId    pin_net(const PinId id) const;

		//Returns the pin type of the specified pin
		PinType  pin_type(const PinId id) const;

		//Returns the port associated with the specified pin
		PortId   pin_port(const PinId id) const;

		//Returns the port bit index associated with the specified pin
		BitIndex     pin_port_bit(const PinId id) const;

		//Returns the port type associated with the specified pin
		PortType pin_port_type(const PinId id) const;

		//Returns the block associated with the specified pin
		BlockId  pin_block(const PinId id) const;

		//Returns true if the pin is a constant (i.e. its value never changes)
		bool     pin_is_constant(const PinId id) const;

		/*
		* Nets
		*/
		//Returns the name of the specified net
		const std::string&  net_name(const NetId id) const;

		//Returns a range consisting of all the pins in the net (driver and sinks)
		//The first element in the range is the driver (and may be invalid)
		//The remaining elements (potentially none) are the sinks
		pin_range           net_pins(const NetId id) const;

		//Returns the (potentially invalid) net driver pin
		PinId           net_driver(const NetId id) const;

		//Returns the (potentially invalid) net driver block
		BlockId         net_driver_block(const NetId id) const;

		//Returns a (potentially empty) range consisting of net's sink pins
		pin_range           net_sinks(const NetId id) const;

		//Returns true if the net is driven by a constant pin (i.e. its value never changes)
		bool                net_is_constant(const NetId id) const;

		//Removes a connection betwen a net and pin. The pin is removed from the net and the pin
		//will be marked as having no associated net
		//  net_id  : The net from which the pin is to be removed
		//  pin_id  : The pin to be removed from the net
		void remove_net_pin(const NetId net_id, const PinId pin_id);

		/* 
		* Aggregates
		*/
		//Returns a range consisting of all blocks in the netlist
		block_range blocks() const;

		//Returns a range consisting of all nets in the netlist
		net_range   nets() const;
		
		//Returns a range consisting of all pins in the netlist
		pin_range	pins() const;

		/*
		* Lookups
		*/
		//Returns the AtomBlockId of the specified block or AtomBlockId::INVALID() if not found
		//  name: The name of the block
		BlockId find_block(const std::string& name) const;

		//Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
		//Note that this method is typically more efficient than searching by name
		//  blk_id: The ID of the block who's ports will be checked
		//  model_port: The port model to look for
		PortId  find_port(const BlockId blk_id, const t_model_ports* model_port) const;

		//Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
		//Note that this method is typically less efficient than searching by a t_model_port
		//  blk_id: The ID of the block who's ports will be checked
		//  name  : The name of the port to look for
		PortId  find_port(const BlockId blk_id, const std::string& name) const;

		//Returns the AtomNetId of the specified net or AtomNetId::INVALID() if not found
		//  name: The name of the net
		NetId   find_net(const std::string& name) const;

		//Returns the AtomPinId of the specified pin or AtomPinId::INVALID() if not found
		//  port_id : The ID of the associated port
		//  port_bit: The bit index of the pin in the port
		PinId   find_pin(const PortId port_id, BitIndex port_bit) const;


	protected: //Protected Base Types
		struct string_id_tag;

		//A unique identifier for a string in the atom netlist
		typedef vtr::StrongId<string_id_tag> StringId;

	protected: //Protected Base Members
		/*
		 * Lookups
		 */
		//Returns the AtomStringId of the specifed string if it exists or AtomStringId::INVALID() if not
		//  str : The string to look for
		StringId find_string(const std::string& str) const;
		
		//Returns the AtomBlockId of the specifed block if it exists or AtomBlockId::INVALID() if not
		//  name_id : The block name to look for
		BlockId find_block(const StringId name_id) const;

		//Returns the NetId of the specifed port if it exists or NetId::INVALID() if not
        //  name_id: The string ID of the net name to look for
        NetId find_net(const StringId name_id) const;
		
		/*
         * Mutators
         */
        //Create or return the ID of the specified string
        //  str: The string whose ID is requested
        StringId create_string(const std::string& str);

		//Updates net cross-references for the specified pin
		void associate_pin_with_net(const PinId pin_id, const PinType type, const NetId net_id);

		//Updates port cross-references for the specified pin
		void associate_pin_with_port(const PinId pin_id, const PortId port_id);

		//Removes a port from the netlist.
		//The port's pins are also marked invalid and removed from any associated nets
		//  port_id: The ID of the port to be removed
		void remove_port(const PortId port_id);

		//Removes a pin from the netlist.
		//The pin is marked invalid, and removed from any assoicated nets
		//  pin_id: The ID of the pin to be removed
		void remove_pin(const PinId pin_id);

		/*
		* Sanity Checks
		*/
		//Verify the internal data structure sizes match
		bool validate_pin_sizes() const;
		bool validate_net_sizes() const;
		bool validate_string_sizes() const;

		//Validates that the specified ID is valid in the current netlist state
		bool valid_block_id(BlockId id) const;
		bool valid_port_id(PortId id) const;
		bool valid_port_bit(PortId id, BitIndex port_bit) const;
		bool valid_pin_id(PinId id) const;
		bool valid_net_id(NetId id) const;
		bool valid_string_id(StringId id) const;


	protected: //Protected Data
		std::string netlist_name_;	//Name of the top-level netlist
		std::string netlist_id_;	//Unique identifier for the netlist
		bool dirty_;				//Indicates the netlist has invalid entries from remove_*() functions

		//Block data
		vtr::vector_map<BlockId, BlockId>					block_ids_;                //Valid block ids
		vtr::vector_map<BlockId, StringId>					block_names_;              //Name of each block
		vtr::vector_map<BlockId, const t_model*>			block_models_;             //Architecture model of each block

		vtr::vector_map<BlockId, std::vector<PortId>>		block_ports_;              //Ports of each block
		vtr::vector_map<BlockId, unsigned>					block_num_input_ports_;    //Input ports of each block
		vtr::vector_map<BlockId, unsigned>					block_num_output_ports_;   //Output ports of each block
		vtr::vector_map<BlockId, unsigned>					block_num_clock_ports_;    //Clock ports of each block

		vtr::vector_map<BlockId, std::vector<PinId>>		block_pins_;               //Pins of each block
		vtr::vector_map<BlockId, unsigned>					block_num_input_pins_;     //Number of input pins on each block
		vtr::vector_map<BlockId, unsigned>					block_num_output_pins_;    //Number of output pins on each block
		vtr::vector_map<BlockId, unsigned>					block_num_clock_pins_;     //Number of clock pins on each block

		//Port data
        vtr::vector_map<PortId,PortId>					port_ids_;      //Valid port ids
		vtr::vector_map<PortId, StringId>				port_names_;    //Name of each port
		vtr::vector_map<PortId, BlockId>				port_blocks_;   //Block associated with each port
		vtr::vector_map<PortId, const t_model_ports*>   port_models_;   //Architecture port models of each port
		vtr::vector_map<PortId, std::vector<PinId>>		port_pins_;     //Pins associated with each port

		//Pin data
		vtr::vector_map<PinId, PinId>		pin_ids_;           //Valid pin ids
		vtr::vector_map<PinId, PortId>		pin_ports_;         //Type of each pin
		vtr::vector_map<PinId, BitIndex>	pin_port_bits_;     //The ports bit position in the port
		vtr::vector_map<PinId, NetId>		pin_nets_;          //Net associated with each pin
		vtr::vector_map<PinId, bool>		pin_is_constant_;   //Indicates if the pin always keeps a constant value

		//Net data
        vtr::vector_map<NetId,NetId>              net_ids_;   //Valid net ids
        vtr::vector_map<NetId,StringId>           net_names_; //Name of each net
        vtr::vector_map<NetId,std::vector<PinId>> net_pins_;  //Pins associated with each net

        //String data
        // We store each unique string once, and reference it by an StringId
        // This avoids duplicating the strings in the fast look-ups (i.e. the look-ups
        // only store the Ids)
        vtr::vector_map<StringId,StringId>		string_ids_;    //Valid string ids
        vtr::vector_map<StringId,std::string>	strings_;       //Strings

	protected: //Fast lookups
        vtr::vector_map<StringId,BlockId>				block_name_to_block_id_;
        vtr::vector_map<StringId,NetId>					net_name_to_net_id_;
		std::unordered_map<std::string, StringId>		string_to_string_id_;
};


#endif