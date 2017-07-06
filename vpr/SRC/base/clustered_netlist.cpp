#include "clustered_netlist.h"

/*
*
*
* ClusteredNetlist Class Implementation
*
*
*/
ClusteredNetlist::ClusteredNetlist(std::string name, std::string id)
	: BaseNetlist(name, id) {}


// Mutator
void ClusteredNetlist::set_netlist_id(std::string id) {
	netlist_id_ = id;
}