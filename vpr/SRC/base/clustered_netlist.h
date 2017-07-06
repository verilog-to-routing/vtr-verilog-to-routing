#ifndef CLUSTERED_NETLIST_H
#define CLUSTERED_NETLIST_H
/*
* Summary
* ========
* This file defines the ClusteredNetlist class in the ClusteredContext used during 
* pre-placement stages of the VTR flow (packing & clustering).
*/
#include "base_netlist.h"

class ClusteredNetlist : public BaseNetlist {
	public:
		//Constructs a netlist
		// name: the name of the netlist (e.g. top-level module)
		// id:   a unique identifier for the netlist (e.g. a secure digest of the input file)
		ClusteredNetlist(std::string name="", std::string id="");

	public:
		void set_netlist_id(std::string id);
};

#endif
