/* VQM Parser DLL Header file
 *
 * This file defines basic macros for the VQM Parser.
 *
 * Description:
 * This header file contains the definitions of each data structure that could be created
 * when parsing the Verilog Quartus Mapping file, based on description provided in
 * vqmx_doc.pdf file. The description below describes how to use this parser and how
 * to interpret data from it.
 *
 * How to use:
 * To use this parser your program needs do perform the following steps:
 * 1.		call the vqm_parse_file(char *) function to parse the file.
 *			Example:
 *				my_module = vqm_parse_file("test.vqm");
 * 2.		Process the data supplied by the parse routine. Basically, manipulate the data in the provided data structures
 *			to meet your needs.
 * 3.		Free memory taken by the VQM data structures by calling the following function:
 *				vqm_data_cleanup();
 *
 * Data structures:
 * The data structure returned by the vqm_parse_file function is a t_module structure. This structure consists of a module name,
 * an array of pins, an array of statements and an array of nodes. The array of pins contains a set of input, output and bidirectional
 * pins as well as all internal wires in the circuit. The array of statements defines statements such as:
 * assign gnd = 'b0;
 * assign a = { b , c };
 * Finally, the array of nodes is an array of all Stratix WYSIWYG cells found in the VQM file. These include an LCELL
 * and RAM cells.
 *
 * Array of pins:
 * The array of pins is an array of t_pin_def structures. The t_pin_def structure contains the name of a pin, or a wire,
 * the number of bits this pin, or wire, uses and a type. To specify the number of bits a pin uses, the left and the right index
 * of a pin are specified. When the wire contains only one bit, the left and right indicies are equal to 0. Examples:
 * a) input a;		-> (name = "a", left = 0, right = 0, type = PIN_INPUT)
 * b) output b[2:1];-> (name = "b", left = 2, right = 1, type = PIN_OUTPUT)
 * c) inout c[1:2];	-> (name = "c", left = 1, right = 2, type = PIN_INOUT)
 *
 * Array of statements:
 * An array of statements is an array of t_assign structures. The t_assign structure contains the source and the target of
 * an assignment in a form of a pointer to a t_pin_def structure. For both the source and the target wire a *_index variable
 * is provided to specify which pin in the source and target pin is being used. When the entire pin, including all bits is being used
 * then the index is equal to min(pin.left, pin.right)-1. Because the assignment may require data to be inverted,
 * the inversion boolean flag is provided. When T_TRUE, then the target is an inversion of the source. An assignment statement can also
 * use a tristate buffer. To signify this, a tri_control pin and an is_tristated boolean flag are provided.
 *
 * Array of nodes:
 * An array of nodes is an array of t_node structures. A t_node structure represents a WYSIWYG cell. It has a name,
 * type, list of parameters and list of ports. The list of parameters and the type of a node specify its functionality
 * as described in the following QUIP documents:
 * - stratix_mac_wys_doc.pdf - for multiply and accumulate blocks 
 * - stratix_ram_wys_doc.pdf - for RAM blocks
 * - stratix_wysuser_doc.pdf - for IO and LCELL blocks
 * A node parameter is a string that identifies an option of a WYSIWYG cell followed by a string or an integer
 * value associated with it. Each parameter is stored in a t_node_parameter structure, which is self-explanatory.
 *
 * Finally, the list of ports defines input and output wires that connect to the node. Each connection to the node
 * is defined by a t_node_port_association structure. This structure associates a port on a node with a particular
 * pin (wire/net) in the design. In this implementation, the port_name and the port_index identify which port on the
 * WYSIWYG cell a wire connects. In a case when all bits of a pin connect to a port, the port_index is equal to -1.
 * As for the pin/wire/net specification, the wire is specified by a pointer to the t_pin_def structure for a particular
 * pin/wire/net. The wire_index identifies which bit of the pin is used for the connection. When the wire index is
 * min(pin.left, pin.right)-1.
 *
 * Author: Tomasz Czajkowski
 * Date: February 28, 2005
 * NOTES/REVISIONS:
 * April 21, 2005 - Modified the t_node_port_association structure to include
 *					an index to the port wire. This allows for direct association
 *					between a wire an a particular port. This change was necessary to accomodate
 *					changes to the VQM file format, specifically to allow concat statements to be used
 *					as parameters in connection lists. (TC)
 * June 1, 2005 - Only one warning may be produced when compiling this DLL. It should be
 *				  warning C4102: 'find_rule' : unreferenced label
 *				  It is due to the way that bison and yacc generate the parser for the specified grammar. (TC)
 * October 6, 2005 - I noticed a few bugs in the code, specifically with the assignment statements. They are now fixed. (TC)
 */

#ifndef _VQM_DLL_H_
#define _VQM_DLL_H_


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VQM_DLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VQM_DLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef VQM_DLL_EXPORTS
#define VQM_DLL_API
//__declspec(dllexport)
#else
#define VQM_DLL_API
//__declspec(dllimport)
#endif


/*****************************************************/
/*** INCLUDE FILES ***********************************/
/*****************************************************/


#include <stdio.h>
#include <malloc.h>


/*****************************************************/
/*** DATA STRUCTURES *********************************/
/*****************************************************/

#define VQM_PARSER_VERSION	0x0103
/* Parser Version 1.3 */

#if !defined(t_boolean)
typedef enum e_boolean {T_FALSE = 0, T_TRUE } t_boolean;
#endif

/* Define two types of configuration bits - STRING and INTEGER. Depending on the
 * type a particular configuration bit can be set as either a string or an integer
 * value. For example, a STRING configuration applies when setting Logic Element mode,
 * to "NORMAL" or "ARITHMETIC", while an INTEGER value applies when specifying
 * a RAM bit width.
 */
typedef enum e_node_parameter_type { NODE_PARAMETER_STRING = 0, NODE_PARAMETER_INTEGER } t_node_parameter_type;


/* Define a node parameter structure to hold different node settings. */
typedef struct s_node_parameter {

	/* Name of a node parameter */
	char *name; 

	/* Type of the node parameter - Integer or String*/
	t_node_parameter_type type; 

	/* Value assigned to a node parameter */
	union u_value {
		char	*string_value; /* Use string value when type is NODE_PARAMETER_STRING */
		int		integer_value; /* Use integer value when type is NODE_PARAMETER_INTEGER */
	} value;
} t_node_parameter;


/* Define a connection between nodes/ports */
typedef enum e_pin_def_type { PIN_INPUT = 1, PIN_OUTPUT, PIN_INOUT, PIN_WIRE } t_pin_def_type;
typedef struct s_pin_def {
	/* Define a connection name */
	char *name;

	/* Define an left and right index bound */
	int left,right;

	/* Is the pin indexed. */
	t_boolean indexed;

	/* Pin type */
	t_pin_def_type type;
} t_pin_def;


/* Define a structure to hold association of a node port and a net name */
typedef struct s_node_port_association {
	/* Specify port name */
	char *port_name;
	
	/* Port index the wire is attached to. -1 means that the entire port is associated with the
	 * net.
	 */
	int port_index;
	
	/* Specify connection */
	t_pin_def *associated_net;

	/* Specify the index of the wire, if associated wire is a bus. If this value is
	 * min(associated_pin->left,associated_pin->right)-1 then entire wire
	 * is associated with this connection.
	 */
	int wire_index;
} t_node_port_association;


/* Define a structure to hold node information */
typedef struct s_node {
	/* Node type and name */
	char* type;
	char* name;

	/* Node parameters */
	t_node_parameter **array_of_params;
	int number_of_params;

	/* Node Port association array */
	t_node_port_association **array_of_ports;
	int number_of_ports;
} t_node;


/* Define an assign statement. The meaning of this structure is the same as
 * assign target[target_index] = source[source_index];
 *
 * If both source and target are 1 bit wide, target_index = source_index = 0.
 */
typedef struct s_assign {
	/* Define Assignment source */
	t_pin_def *source;
	int source_index;

	/* Define assignment target */
	t_pin_def *target;
	int target_index;

	/* Is the source signal tristated? */
	t_boolean is_tristated;
	t_pin_def *tri_control;
	int tri_control_index;

	/* Constant value - if source is NULL */
	int value;

	/* Is the input inverted. */
	t_boolean inversion;
} t_assign;


typedef struct s_module {
	/* Module name */
	char *name;

	/* List pins - This includes I/O ports and wires */
	t_pin_def **array_of_pins;
	int number_of_pins;

	/* Assignment statements */
	t_assign **array_of_assignments;
	int number_of_assignments;

	/* Array of nodes */
	t_node **array_of_nodes;
	int number_of_nodes;
} t_module;


/* Create an array reference to pass within bison. Since bison only passes
 * one reference, and we need both a pointer and array size, this data will be enclosed
 * in this one structure.
 */
typedef struct s_array_ref
{
	void **pointer;
	size_t array_size;
    size_t allocated_size;
} t_array_ref;


typedef struct s_identifier_pass {
	char *name;
	/* Name of the identifier */
	
	t_boolean indexed;
	/* If the identifier is a bus, does this reference address a single wire of the bus? */

	int index;
	/* If so, which wire is it?*/
} t_identifier_pass;
/* Structure used to pass parameter information */


typedef enum e_rvalue_type {	RVALUE_BIT_VALUE = 0, RVALUE_IDENTIFIER,
								RVALUE_IDENTIFIER_BAR, RVALUE_CONCAT } t_rvalue_type;
typedef struct s_rvalue {
	t_rvalue_type type;
	union u_data {
		void *data_structure;
		int bit_value;
	} data;
} t_rvalue;
/* Structure to pass RValue information */

/*
 * Identifies whether the current parsing pass is for counting or actual
 * data structure allocation.
 */
typedef enum e_parsing_pass_type { COUNT_PASS = 0, ALLOCATE_PASS} t_parsing_pass_type;

/*
 * Structure used to count number of elements during counting pass
 */
typedef struct s_parse_info {
    t_parsing_pass_type pass_type;
    int number_of_pins;
    int number_of_assignments;
    int number_of_nodes;
    int number_of_modules;
} t_parse_info;

/*
 * A hash table element
 */
typedef struct s_hash_elem t_hash_elem;
struct s_hash_elem {
    char* key;
    size_t value;
    t_hash_elem* next; //In case of hash collision, make a linked list
};

/*
 * A hash table
 */
typedef struct s_hash_table {
    t_hash_elem* table; //Array of t_hash_elem from [0..size-1]
    size_t size; //Number of entries in table
} t_hash_table;

typedef struct s_index_pass {
    t_boolean found; //The net was found and the index field should be valid
    size_t index; //The index into the array_ref passed to find_position_for_net_in_array_by_hash
} t_index_pass;


/*****************************************************/
/*** FUNCTION DECLARATIONS ***************************/
/*****************************************************/

#if !defined(TESTING_DLL_FUNCTIONS)
VQM_DLL_API t_array_ref *vqm_get_module_list();
VQM_DLL_API t_module *vqm_parse_file(char *filename);
VQM_DLL_API void vqm_data_cleanup();
VQM_DLL_API int vqm_get_error_message(char *message_buffer, int length);
#else
t_array_ref *vqm_get_module_list();
t_module *vqm_parse_file(char *filename);
void vqm_data_cleanup();
#endif

#endif
