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

#include "base_netlist_fwd.h"

class BaseNetlist {
	public: //Public Types
		typedef vtr::vector_map<NetId, NetId>::const_iterator net_iterator;
		typedef vtr::vector_map<PinId, PinId>::const_iterator pin_iterator;

		typedef vtr::Range<net_iterator> net_range;
		typedef vtr::Range<pin_iterator> pin_range;

	public: //Constructor
		BaseNetlist(std::string name="", std::string id="");


	public: //Public Mutators
		//Create an empty, or return an existing net in the netlist
		//  name    : The unique name of the net
		NetId   create_net(const std::string name); //An empty or existing net

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
		* Nets
		*/
		//Returns the name of the specified net
		const std::string&  net_name(const NetId id) const;

		
		/* 
		* Aggregates
		*/
		//Returns a range consisting of all nets in the netlist
		net_range   nets() const;
		//Returns a range consisting of all pins in the netlist
		pin_range	pins() const;

		
		/*
		* Lookups
		*/
		//Returns the AtomNetId of the specified net or AtomNetId::INVALID() if not found
		//  name: The name of the net
		NetId   find_net(const std::string& name) const;


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
		
		//Returns the NetId of the specifed port if it exists or NetId::INVALID() if not
        //  name_id: The string ID of the net name to look for
        NetId find_net(const StringId name_id) const;
			 
		
		/*
         * Mutators
         */
        //Create or return the ID of the specified string
        //  str: The string whose ID is requested
        StringId create_string(const std::string& str);


		//Re-builds fast look-ups
		void rebuild_lookups();

		/*
		* Sanity Checks
		*/
		//Verify the internal data structure sizes match
		bool verify_sizes() const; //All data structures
		bool validate_net_sizes() const;
		bool validate_string_sizes() const;

		//Validates that the specified ID is valid in the current netlist state
		bool valid_net_id(NetId id) const;
		bool valid_string_id(StringId id) const;

	protected: //Protected Data
		std::string netlist_name_;	//Name of the top-level netlist
		std::string netlist_id_;	//Unique identifier for the netlist
		bool dirty_;				//Indicates the netlist has invalid entries from remove_*() functions

		//Net data
        vtr::vector_map<NetId,NetId>              net_ids_;   //Valid net ids
        vtr::vector_map<NetId,StringId>           net_names_; //Name of each net
        vtr::vector_map<NetId,std::vector<PinId>> net_pins_;  //Pins associated with each net

        //String data
        // We store each unique string once, and reference it by an StringId
        // This avoids duplicating the strings in the fast look-ups (i.e. the look-ups
        // only store the Ids)
        vtr::vector_map<StringId,StringId>   string_ids_;    //Valid string ids
        vtr::vector_map<StringId,std::string>    strings_;       //Strings

	protected: //Fast lookups
        vtr::vector_map<StringId,BlockId>       block_name_to_block_id_;
        vtr::vector_map<StringId,NetId>         net_name_to_net_id_;
		std::unordered_map<std::string, StringId>    string_to_string_id_;
};


#endif