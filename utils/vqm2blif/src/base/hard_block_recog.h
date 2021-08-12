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
*  some utility functions are added to help identify whether a hard block was *  included within the design. For more information, 
*  refer to 'hard_block_recog.cpp'.
*
*/


// need to use vtr::malloc for memory allocation (only in the files here)
// no need to run VTR assert, after memory allocation, this is checked in the function already


#ifndef HARD_BLOCK_RECOG_H
#define HARD_BLOCK_RECOG_H

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

// standard libraries
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>

// useful spacing definitions
#define TOP_LEVEL 0
#define SECOND_LEVEL 1

#define WIRE_NOT_INDEXED -1

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

#define PORT_NAME 1
#define PORT_INDEX 2



/* Structure Declarations */ 


/*
*   The purpose of this data structure is to
*   store the port information (in an array) for an arbritary user defined
*   hard block design. Then a mapping is provided which can 
*   help identify the specific location within the port array 
*   that a given port name begins at.
*/
typedef struct s_hard_block_port_info
{
    // mapping structure to quickly identify where a specific port begins
    std::unordered_map<std::string, int> port_name_to_port_start_index;
    
    // mapping structure to quickly identify the direction of a port
    std::unordered_map<std::string, PORTS> port_name_to_port_dir;

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
*
*   The purpose of the data structure below is to keep a reference to a 
*   single hard block node (one instance of a hard block within the design). *   This way, the node can be accessed quickly whenever changes need to be 
*   applied. Additional information about the hard block is also stored as well.
*   
*   THe node itself is stored internally within the a node array. And
*   located inside a module.
*/
typedef struct s_hard_block
{
    // indicates the name of the user defined hard block
    std::string hard_block_name = "";

    // helps keep track of the number of hard block ports we have assigned
    // a net to 
    int hard_block_ports_assigned = 0;

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
*   a specific hard block instance. All functiions will primarily interact
*   with this data structure.
*/
typedef struct s_hard_block_recog
{
    // names of all the user defined hard blocks
    //std::vector<std::string>* hard_block_type_names; // hb_type_names (probably do not need)
    
    // store the port information of each and every type of
    // user defined hard blocks. All ports connected to each
    // hard block type are stored.
    std::unordered_map<std::string, t_hard_block_port_info> hard_block_type_name_to_port_info;  

    // store each hard blocks instantiated within a 
    // user design
    std::vector<t_hard_block> hard_block_instances;

    /* given a specific hard block instance name, we need to quickly
       identify the corresponding hard block structure instance.
       We do this by using the map data structure below,
       where a hard block instance name is associated with an index
       within the hard block instances above. 
    */
    std::unordered_map<std::string, int> hard_block_instance_name_to_index;

    /* The lcells and dffeas used to model ports are all stored as nodes.
       The nodelist is simply an array. Now deleting lcell nodes while still
       creating additional nodes could cause some problems, so we will
       delete all these nodes at the end. Therefore we need to keep a 
       reference for all the lcell and dffeas nodes here.*/
    std::vector<t_node*> luts_dffeas_nodes_to_remove; // look into using array index instead

}t_hard_block_recog;

/*
*   When we go through the .vqm file, the ports of any user defined hard block  *   will be represented as a LUT (stratix_lcell), or flip flop (dffeas) (for *   more info refer to 'hard_block_recog.cpp'). The generated names found in *   the .vqm file for the two previous blocks contain a lot of information     *   about the hard block. The structure below is used to store the information, 
*   which includes the hard block name, hard block type, the specfic hard    *   block port and if the port is a bus, then the specific index.
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
    // initialized to an index equivalent to what a port thas is not a bus would have
    int hard_block_port_index = DEFAULT_PORT_INDEX;

}t_parsed_hard_block_port_info;


/*  Function Declarations 
*
*   For more information about functions, refer to
*   'hard_block_recog.cpp'
*/

void filter_and_create_hard_blocks(t_module*, t_arch*, std::vector<std::string>*, std::string, std::string);

void initialize_hard_block_models(t_arch*, std::vector<std::string>*, t_hard_block_recog*);

bool create_and_initialize_all_hard_block_ports(t_model*, t_hard_block_recog*);

void create_hard_block_port_info_structure(t_hard_block_recog*, std::string);

int extract_and_store_hard_block_model_ports(t_hard_block_recog*, t_model_ports*, std::string, int, std::string);

t_array_ref* convert_hard_block_model_port_to_hard_block_node_port(t_model_ports*);

t_node_port_association* create_unconnected_node_port_association(char*, int ,int);

void store_hard_block_port_info(t_hard_block_recog*, std::string, std::string,PORTS, t_array_ref**, int*);

void copy_array_ref(t_array_ref*, t_array_ref*);

void delete_hard_block_port_info(std::unordered_map<std::string, t_hard_block_port_info>*);

t_parsed_hard_block_port_info extract_hard_block_port_info_from_module_node(t_node*, std::vector<std::string>*);

std::string identify_hard_block_type(std::vector<std::string>*, std::string);

void identify_hard_block_port_name_and_index (t_parsed_hard_block_port_info*, std::string);

void split_node_name(std::string, std::vector<std::string>*, std::string);

std::string construct_hard_block_name(std::vector<std::string>*, std::string);

// utility functions

void store_hard_block_names(char**, int, std::vector<std::string>*);

#endif