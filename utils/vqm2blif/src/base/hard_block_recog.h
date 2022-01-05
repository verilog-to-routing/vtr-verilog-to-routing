#ifndef HARD_BLOCK_RECOG_H
#define HARD_BLOCK_RECOG_H

/* VQM new primitive hard block recognition header file
*
*  The purpose of this file is to identify and instantiate
*  user defined hard blocks, which will later be writtent to a .blif file.
*  For more information, refer to 'hard_block_recog.cpp'.
*   
*  This file contains the definition of the main data structures 
*  that are instantiated when trying to identify and create user
*  created hard blocks. 
* 
*  This file also contains all the declarations of functions that can be
*  used to interact with and manipulate the main data structures. Additionally,
*  some utility functions are added to help identify whether a hard block was
*  included within the design.
*  For more information, refer to 'hard_block_recog.cpp'.
*
*/


// need to use vtr::malloc for memory allocation (only in the files here)
// no need to rcheck for failed malloc (NULL pointer), this is already checked in vtr::malloc


// user VTR libraries
#include "vqm_dll.h"
#include "physical_types.h"
#include "logic_types.h"
#include "vtr_error.h"
#include "vtr_log.h"
#include "vqm_common.h"
#include "vtr_memory.h"
#include "logic_types.h"

// user vqm2blif libraries
#include "vqm2blif_util.h"
#include "cleanup.h"

// standard libraries
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>

// useful spacing definitions
#define TOP_LEVEL 0
#define SECOND_LEVEL 1

#define PORT_WIRE_NOT_INDEXED -1

#define INPUT_PORTS "input"
#define OUTPUT_PORTS "output"

#define HARD_BLOCK_WITH_NO_PORTS 0
#define DEFAULT_PORT_INDEX 0

// unique identifier that seperates a hard block type name (module name), from the specific instance name
#define HARD_BLOCK_TYPE_NAME_SEPERATOR ":"

// a node name in reference to a hard block port (dffeas and stratic_lcell blocks) in the vqm netlist file consists of multiple hierarchy levels.
// for example we can have the following name: \router_interconnect:test_router_interconnect|router:noc_router|id[2]~QIC_DANGLING_PORT_I (this has 3 hierarchy levels)
// each level of hierarchy (module within module) is seperated by the delimiter character defined below. The last level of hierarchy is the output net of the block 
#define VQM_NODE_NAME_DELIMITER "|"

// a port that is a bus is a vectored port. So a single port name with multiple indices.
// We use the size of the port to determine whether the port is a bus or not.
// A port size of 1 represents a port that is not a bus, whereas a port size greater than 1 represents a bus. 
// So a non-bus port size is represented below 
#define PORT_NOT_BUS 1

// We use regex to extract the port name and index from the vqm netlist.
// For example, given the string payload[1]~QIC_DANGLING_PORT_I, we would extract 'payload' and '1'.
// For the case where a port is not a bus, we would just extract the port name.
// Now the extracted information is stored in an array of strings, and the indices of where the port name and index are stored is found below. 
#define PORT_NAME 1
#define PORT_INDEX 2

// used to identify the case where a hard block instance is not found within the netlist
#define HARD_BLOCK_INSTANCE_DOES_NOT_EXIST -1

/* Structure Declarations */ 


/*
*   The port information (in an array) for an arbritary user defined
*   hard block in the design is stored in s_hard_block_port_info. Then a 
*   mapping is provided which can       
*   help identify the specific location within the port array 
*   that a given port name begins at. This data structure is created
*   for each type of hard block.
*/
typedef struct s_hard_block_port_info
{
    // mapping structure to quickly identify where a specific port begins
    std::unordered_map<std::string,int> port_name_to_port_start_index;

    // mapping structure to determine the size of each port
    std::unordered_map<std::string,int> port_name_to_port_size;
    
    // An array of all the ports within the hard block is stored here
    // refer to 'vqm_dll.h' for more information
    t_array_ref hard_block_ports;

} t_hard_block_port_info;


/*
*   Each hard block instance that will eventually be written to the blif file
*   will need to be stored as a 't_node' type (refer to 'vqm_dll.h').
*   This node will then be stored within a node array and enclosed
*   within a 't_module' variable (refer to 'vqm_dll.h').
*
*   A reference to a single hard block node (one instance of a hard block
*   within the design) is stored inside s_hard_block, so it essentially
*   represents a hard block instance.
*   This way, the node can be accessed quickly whenever changes need to be 
*   applied. Additional information about the hard block is also stored as well.
*   
*   The node itself is stored internally within the a node array. And
*   located inside a module.
*/
typedef struct s_hard_block
{
 
    // helps keep track of the number of hard block ports we have left to assign a net to
    int hard_block_ports_not_assigned = 0;

    // a reference to the corresponding hard block node that represents this 
    // particular hard block instance
    t_node* hard_block_instance_node_reference;

}t_hard_block;

/*
*   Below is the main data structure used for hard block
*   identification and creation. This data strcuture contains
*   the names and port info of every type of user defined hard blocks. It
*   also stores all hard blocks that were instantiated within
*   the user design and an accompanying data strcuture to quickly identify
*   a specific hard block instance. All functions will primarily interact
*   with this data structure.
*/
typedef struct s_hard_block_recog
{
    
    // store the port information of each and every type of
    // user defined hard blocks. All ports connected to each
    // hard block type are stored.
    std::unordered_map<std::string, t_hard_block_port_info> hard_block_type_name_to_port_info;  

    // store each hard block instantiated within a 
    // user design
    std::vector<t_hard_block> hard_block_instances;

    /* 
       Given a specific hard block instance name, we need to quickly
       identify the corresponding hard block structure instance.
       We do this by using the map data structure below,
       where a hard block instance name is associated with an index
       within the hard block vector above (stores all instances within the design).
    */
    std::unordered_map<std::string, int> hard_block_instance_name_to_index;

    /* 
       The luts and dffeas used to model ports are all stored as nodes and
       we will need to remove them from the netlist after adding all the
       hard blocks to the netlist.The ports that are modelled by the luts and dffeas 
       aren't needed once all the hard blocks within the design are added to the netlist.
       The luts and dffeas are simply repeating the port information, so we can remove them.
        
       The nodelist is simply an array. Now deleting lut/dffeas nodes while still creating additional 
       hard block nodes could cause some problems, so we will delete all these nodes at the end. 
	   Therefore we need to keep a reference for all the luts and dffeas nodes that
       represent hard block ports here, so we can them remove later on.
    */
    std::vector<t_node*> luts_dffeas_nodes_to_remove; // look into using array index instead

    // variable to store parameters specific to the fpga device used
    DeviceInfo target_device_info;

}t_hard_block_recog;

/*
*   When we go through the .vqm file, the ports of any user defined hard block 
*   will be represented as a LUT (stratix_lcell), or flip flop (dffeas) 
*   (for more info refer to 'hard_block_recog.cpp'). The generated names found  
*   in the .vqm file for the two previous blocks contain a lot of information 
*   about the hard block. The structure below is used to store the information, 
*   which includes the hard block name, hard block type, the specfic hard 
*   block port and if the port is a bus, then the specific index.
*/
typedef struct s_parsed_hard_block_port_info
{
    // name used to identify the unique hard block the current port belongs to
    // multiple ports can belong to the same hard block and there can be multiple hard blocks in the design
    std::string hard_block_name = "";

    // the specific type of hard block the current port belongs to
    // the user can define multiple hard block types
    std::string hard_block_type = "";

    // the port name defined in the current block (LUT or dffeas)
    std::string hard_block_port_name = "";

    // index of the port defined in the current block (LUT or dffeas)
    // initialized to an index equivalent to what a port that is not a bus would have
    int hard_block_port_index = DEFAULT_PORT_INDEX;

}t_parsed_hard_block_port_info;


/*  Global Function Declarations 
*
*   For more information about functions, refer to
*   'hard_block_recog.cpp'
*/

/*
*	Function: add_hard_blocks_to_netlist
*	
*	Goes through a netlist and removes luts/dff that represent
*   specific hard block ports and then the corresponding hard block
*   is added to the netlist.
*
*	Parameters:
*		t_module*  - A reference to the netlist of the design
*		t_arch* - a pointer to the FPGA architecture that design will
*                 target
*		std::vector<std::string>* - a reference to a list of hard block names
                                    that need to be identified within the
                                    netlist.
*		
*	    std::string - the name of the architecture file (".xml")
        std::string - the name of the Quartus generated netlist file (".vqm")
*
*/
void add_hard_blocks_to_netlist(t_module* main_module, t_arch* main_arch, std::vector<std::string>* list_hard_block_type_names, std::string arch_file_name, std::string vqm_file_name, std::string device);

#endif
