#ifndef CLEAN_H
#define CLEAN_H


//============================================================================================
//				INCLUDES
//============================================================================================

#include "vqm2blif_util.h"
#include "lut_recog.h"
#include "vqm_common.h"

//============================================================================================
//				GLOBALS
//============================================================================================

extern int buffer_count, invert_count, onelut_count;
extern int buffers_elim, inverts_elim, oneluts_elim;

void netlist_cleanup (t_module* module);

/*
*	Function: remove_node
*	
*	Removes a specific node within a modules node list (node array)
*
*	Parameters:
*		node  - a pointer to the node to remove from the module node list (node array)
*		nodes - The module node list containing all nodes (node array) 
*		original_num_nodes - an integer that represents the total number of nodes within the module node list (node array)
*		
*	returns: If successful, then nothing is returned. 
*		     If the node to remove was not found, and assertion is raised. 
*
*/
void remove_node ( t_node* node, t_node** nodes, int original_num_nodes );

/*
*	Function: reorganize_module_node_list
*	
*	This function fixes a module node list (node array) that has elements
*   within it removed. THe removed elements create gaps within the array and
*	this function fills in those gaps so that the array is continuous.
*
*	Parameters:
*		module - the module that contains a node list with elemets within it deleted 
*		
*/
void reorganize_module_node_list(t_module* module);

//============================================================================================
//				STRUCTURES & TYPEDEFS
//============================================================================================
typedef enum d_type {NODRIVE, BUFFER, INVERT, BLACKBOX, CONST} drive_type;

typedef struct s_net{

	t_pin_def* pin;
	int bus_index;
	int wire_index;

	void* source;
	drive_type driver;

	t_node* block_src;

	int num_children;

} t_net;

typedef vector<t_net> netvec;
typedef vector<netvec> busvec;

//============================================================================================
//				CLEANUP DEFINES
//============================================================================================

//#define CLEAN_DEBUG	//dumps intermediate output files detailing the netlist as it is cleaned

#endif


