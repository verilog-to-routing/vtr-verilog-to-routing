/*********************************************************************************************
*		lut_recog.cpp
*
*	S. Whitty
*	October 27th, 2011
*
* This file determines the ability of the vqm2blif convertor to recognize and subsequently
* elaborate a VQM Primitive that represents a LUT into a corresponding BLIF ".names" structure.
*
*		BACKGROUND
*	A primitive in a VQM file represents a block of hardware on an Altera® FPGA device that
* has been instantiated, connected and configured to perform a certain function within the 
* circuit. Each family of devices has a corresponding set of primitives that represent blocks
* such as RAMs, DSP blocks, PLLs, or combinational logic elements. Additionally, each primitive 
* in a VQM file has a certain Configuration Profile (CP) that describes its lower-level 
* functionality. A CP is made up of the parameter values associated with the primitive as well 
* as its connectivity (i.e. which ports are connected/unconnected). Primitives with the same name 
* but different CPs will have fundamentally different lower-level functionalities in the circuit.
*
*	LUTs are the simplest combinational logic circuit element, and have a unique representation
* in a BLIF file (See: DOCS/blif_info.pdf for more). In order to determine whether a given VQM
* primitive is functioning as a LUT and can be elaborated, it is necessary to assess both of the
* following:
*
*	- The Primitive's Type (e.g. "stratixiv_lcell_comb")
*	- The Primitive's CP
*
*		IMPLEMENTATION
*	The current implementation supports both the Stratix® III and IV representation of a LUT
* in a VQM file. In order to expand this recognition, a user must do the following:
*
*	I.	Create a Configuration Profile Function that, given a t_node structure, can determine
*		if a primitive has the proper CP
*	II.	Insert a new pair in the supported_luts structure that maps the new primitive's name
*		to its CP Function
*
*	Step II. must be performed in the setup_lut_support_map() function declared here. Example
* pair insertions for the Stratix I and IV cases appear there. 
*
*	Determining the CP of a different Device Family's LUT can be done through Altera's QUIP 
* documentation or device handbooks. Alternatively, since the CP is unique to a certain functionality, 
* a user can simply generate a VQM file mapped to the family in question and locate primitives that 
* are known to be LUTs, then record their CP.
*
*	The following is a deconstruction of the t_node structure and its components, to give an
* understanding of how a CP is expressed in memory. Example CP determination is shown in the
* CP functions included here. All of these structures are declared in vqm_dll.h
*
*---------------------------------------------------------------------------------------------
*	Each t_node structure  has the following members:
*
* char* type 
*	The primitive type, Family-specific (e.g. "stratixiv_lcell_comb")
* char* name
*	The instantiation name of a primitive, arbitrary and unique to each instance
* t_node_parameter **array_of_params, int number_of_params
*	An array containing parameter structures and its size.
* t_node_port_association **array_of_ports, int number_of_ports
*	An array containing port structures and its size.
*---------------------------------------------------------------------------------------------
*	Each t_node_parameter structure has the following members:
*	
* char *name
*	The parameter name (e.g. "operation_mode")
* t_node_parameter_type type
* 	Datatype of the parameter, either NODE_PARAMETER_STRING or NODE_PARAMETER_INTEGER
* value
* 	The value of the parameter, the union of value.string_value and value.integer_value,
* 	as determined by type
*---------------------------------------------------------------------------------------------
*	Each t_node_port_association structure has the following members:
*	
* char *port_name, int port_index
*	The name and index of the port that has been connected in the VQM
* t_pin_def *associated_net, int wire_index
*	The external net structure and index within it that is connected to the port
*--------------------------------------------------------------------------------------------
*	NOTE: If a port appears in the t_node structure, it was specified in the VQM. At time
* of publication, only the ports that are connected to external nets are specified in each primitive.
* Therefore, it is only necessary to check for the existence of a specific port name within the 
* structure to verify that it is connected to an external net. For more information on the 
* t_pin_def structure, see vqm_dll.h.
*
*	To see how the VQM primitive is elaborated once marked as a LUT, see lutmask.h and the 
* push_lut() function in vqm2blif.cpp.
*********************************************************************************************/
#include "lut_recog.h"

//============================================================================================
//				CP FUNCTIONS
//============================================================================================
bool stratixiv_lut_CP (t_node* vqm_node);
bool stratixiii_lut_CP (t_node* vqm_node);
/* bool your_lut_CP (t_node* vqm_node); */

//============================================================================================
//============================================================================================

void setup_lut_support_map (){
/* Initializes the global lut_support_map by associating a VQM primitive's name with its
 * Configuration Profile function. 
 *
 * The lut_support_map structure maps a string to a function pointer that can
 * further verify a primitive's CP. A non-combinational primitive won't appear in the
 * map, and an incorrectly-configured combinational primitive will fail its CP test.
 */
	if (verbose_mode){
		cout << ">> Initializing LUT support" << endl;
	}
	supported_luts.insert(lut_support_pair("stratixiv_lcell_comb", &stratixiv_lut_CP));
	supported_luts.insert(lut_support_pair("stratixiii_lcell_comb", &stratixiii_lut_CP));
	/* supported_luts.insert(lut_support_pair("your_lut_primitive", &your_lut_CP)); */

}

//============================================================================================
//============================================================================================

bool is_lut (t_node* vqm_node){
/* Identifies if a given node in the VQM File can be elaborated into a BLIF-Format LUT (".names").
 * 
 *	ARGUMENTS
 *  vqm_node:
 *	Node read from the VQM File to be analyzed. 
 *	
 */
	lut_support_map::const_iterator lut_type = supported_luts.find(vqm_node->type);
	if (lut_type == supported_luts.end()){
		return 0;	//lut_type not supported
	} else{
		return lut_type->second(vqm_node);	//lut type supported, verify its CP
	}
}

//============================================================================================
//============================================================================================
bool stratixiv_lut_CP (t_node* vqm_node){
/*		Stratix IV LCELL Configuration Profile Function
 * Assesses the connectivity and parameter values for a given node against the known CP
 * of a Stratix IV LUT.
 *
 *	Example LUT CP:
 *
 *	stratixiv_lcell_comb my_lcell (
 *			.dataa( A ),
 *			.datab( B ),
 *			.datac( C ),
 *			.combout( OUT ));
 * 		defparam my_lcell .shared_arith = "off";
 *		defparam my_lcell .extended_lut = "off";
 *		defparam my_lcell .lut_mask = "0202020202020202";
 * 
 *	Where lut_mask can be any 16-character string.
 */
	bool isLut = true;
	bool found_lutmask = false;
	int data_ports = 0;

	for (int i = 0; i < vqm_node->number_of_params; i++){
		if (strcmp(vqm_node->array_of_params[i]->name, "shared_arith") == 0){
			VTR_ASSERT(vqm_node->array_of_params[i]->type == NODE_PARAMETER_STRING);
			if (strcmp(vqm_node->array_of_params[i]->value.string_value, "off") != 0){
				isLut = false;
				break;
			}
		} else if (strcmp(vqm_node->array_of_params[i]->name, "extended_lut") == 0){
			VTR_ASSERT(vqm_node->array_of_params[i]->type == NODE_PARAMETER_STRING);
			if (strcmp(vqm_node->array_of_params[i]->value.string_value, "off") != 0){
				isLut = false;
				break;
			}
		} else if (strcmp(vqm_node->array_of_params[i]->name, "lut_mask") == 0){
			VTR_ASSERT(vqm_node->array_of_params[i]->type == NODE_PARAMETER_STRING);
			if (strlen(vqm_node->array_of_params[i]->value.string_value) != 16){
				isLut = false;
				break;
			}
			found_lutmask = true; //The lut_mask parameter is necessary for BLIF-elaboration
		}
	}
	if ((isLut)&&(found_lutmask)){
		for (int i = 0; i < vqm_node->number_of_ports; i++){
			if (		(strcmp(vqm_node->array_of_ports[i]->port_name, "sumout") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "cout") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "shareout") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "sharein") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "cin") == 0)){
					//Ports mentioned in the VQM are connected; simple LUTs don't use these ports.
					isLut = false;
					break;
			}
			if ((data_ports<2)&&
				(	(strcmp(vqm_node->array_of_ports[i]->port_name, "dataa") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "datab") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "datac") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "datad") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "datae") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "dataf") == 0)
				||	(strcmp(vqm_node->array_of_ports[i]->port_name, "datag") == 0))){
					 //At least two data ports must be used in a simple LUT
					data_ports++;
			}
		}
		if (data_ports < 2)
			isLut = false;	
	}
	
	return isLut;
}

//============================================================================================
//============================================================================================

bool stratixiii_lut_CP (t_node* vqm_node){
/*		Stratix III LCELL Configuration Profile Function
 *
 * Assesses the connectivity and parameter values for a given node against the known CP
 * of a Stratix III LUT. Since the Stratix III LCELL has the same CP as a Stratix IV LCELL
 * when in LUT operation, the Stratix IV CP Function can be used to verify both families. 
 */
	return stratixiv_lut_CP(vqm_node);	
}

//============================================================================================
//============================================================================================
