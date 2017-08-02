#ifndef CLUSTERED_NETLIST_H
#define CLUSTERED_NETLIST_H
/*
* Summary
* ========
* This file defines the ClusteredNetlist class in the ClusteredContext used during 
* pre-placement stages of the VTR flow (packing & clustering).
*/
#include "base_netlist.h"
#include "vpr_types.h"
#include "vtr_util.h"

class ClusteredNetlist : public BaseNetlist {
	public:
		//Constructs a netlist
		// name: the name of the netlist (e.g. top-level module)
		// id:   a unique identifier for the netlist (e.g. a secure digest of the input file)
		ClusteredNetlist(std::string name="", std::string id="");

	public: //Public Mutators
		//Create or return an existing block in the netlist
		//  name        : The unique name of the block
		//  pb			: The physical representation of the block
		//  t_type_ptr	: The type of the CLB
		BlockId create_block(const char *name, t_pb* pb, t_type_ptr type);

		//Create or return an existing port in the netlist
		//  blk_id      : The block the port is associated with
		//  name        : The name of the port (must match the name of a port in the block's model)
		//  width		: The width (number of bits) of the port
		//  type		: The type of the port (INPUT, OUTPUT, or CLOCK)
		PortId create_port(const BlockId blk_id, const std::string name, BitIndex width, PortType port_type);

		//Create or return an existing pin in the netlist
		//  port_id    : The port this pin is associated with
		//  port_bit   : The bit index of the pin in the port
		//  net_id     : The net the pin drives/sinks
		//  pin_type   : The type of the pin (driver/sink)
		//  port_type  : The type of the port (input/output/clock)
		//  pin_index  : The index of the pin relative to its block
		//  is_const   : Indicates whether the pin holds a constant value (e. g. vcc/gnd)
		PinId   create_pin(const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType pin_type, const PortType port_type, int pin_index, bool is_const=false);

		//Sets the pin's index in a block
		//  pin_id   : The pin to be set
		//  index    : The new index to set the pin to
		void	set_pin_index(const PinId pin_id, const int index);

		//Create an empty, or return an existing net in the netlist
		//  name    : The unique name of the net
		NetId   create_net(const std::string name); //An empty or existing net

		//Sets the netlist id based on a file digest's string
		void set_netlist_id(std::string id);
		
		//Sets the flag in net_global_ = state
		void set_global(NetId net_id, bool state);

		//Sets the flag in net_routed_ = state
		void set_routed(NetId net_id, bool state);

		//Sets the flag in net_fixed_ = state
		void set_fixed(NetId net_id, bool state);

	public: //Public Accessors
		/*
		* Blocks
		*/
		//Returns the physical block
		t_pb* block_pb(const BlockId id) const;

		//Returns the type of CLB (Logic block, RAM, DSP, etc.)
		t_type_ptr block_type(const BlockId id) const;

		//Returns the net of the block attached to the specific pin index
		NetId block_net(const BlockId blk_id, const int pin_index) const;

		/*
		* Pins
		*/
		//Returns the index of the block which the pin belongs to
		int pin_index(const PinId id) const;

		//Returns the index of the block which the pin belongs to
		//  net_id     : The net to iterate through
		//  count      : The index of the pin in the net
		int pin_index(NetId net_id, int count) const;

		/*
		* Nets
		*/
		//Returns the block of the net & pin which it's attached to
		BlockId net_pin_block(const NetId net_id, const int pin_index) const;

		//Returns the pin's index in the net
		int net_pin_index(NetId net_id, PinId pin_id) const;

		//Returns whether the net is global or fixed
		bool net_global(const NetId id) const;
		bool net_routed(const NetId id) const;
		bool net_fixed(const NetId id) const;

	private: //Private Members
		bool validate_block_sizes() const;
		bool validate_pin_sizes() const;
		bool validate_net_sizes() const;

	private: //Private Data
		
		//Blocks
		vtr::vector_map<BlockId, t_pb*>			block_pbs_;         //Physical block representing the clustering & internal hierarchy of each CLB
		vtr::vector_map<BlockId, t_type_ptr>	block_types_;		//The type of physical block this user circuit block is mapped to

		//Pins
		vtr::vector_map<PinId, int>				pin_index_;			//The index of the pins relative to its block
			
		//Nets
		vtr::vector_map<NetId, bool>			net_global_;		//Boolean mapping indicating if the net is
		vtr::vector_map<NetId, bool>			net_routed_;		//Global, routed, or fixed (mutually exclusive).
		vtr::vector_map<NetId, bool>			net_fixed_;			//TODO: transfer net routing state to RoutingContext
};

#endif
