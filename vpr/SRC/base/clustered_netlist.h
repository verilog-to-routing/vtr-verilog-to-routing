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

		//Sets the netlist id based on a file digest's string
		void set_netlist_id(std::string id);

	public: //Public Accessors
		//Returns the physical block
		t_pb* block_pb(const BlockId id) const;

		//Returns the mode of the pb
		int block_mode(const BlockId id) const;

		//Returns the type of CLB (Logic block, RAM, DSP, etc.)
		t_type_ptr block_type(const BlockId id) const;

	private: //Private Members
		bool validate_block_sizes() const;

	private: //Private Data
		vtr::vector_map<BlockId, t_pb*>			block_pbs_;             //Physical block representing the clustering of this CLB
		vtr::vector_map<BlockId, t_type_ptr>	block_types_;			//The type of physical block this user circuit block can map into

};

#endif
