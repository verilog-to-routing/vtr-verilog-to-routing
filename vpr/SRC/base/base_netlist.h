#ifndef BASE_NETLIST_H
#define BASE_NETLIST_H

#include <string>
#include "vtr_vector_map.h"

/* 
 * Summary
 * =======
 * This file defines the BaseNetlist class, which stores the connectivity information 
 * of the components in a netlist. It is the base class for AtomNetlist and ClusteredNetlist.
 *
*/
class BaseNetlist {
	public:
		BaseNetlist(std::string name="", std::string id="");

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

	private:
		std::string netlist_name_;	//Name of the top-level netlist
		std::string netlist_id_;	//Unique identifier for the netlist
};


#endif
