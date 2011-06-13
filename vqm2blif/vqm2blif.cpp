/*********************************************************************************************
*	vqm2blif.cpp
* The purpose of this program is to generate a valid .blif netlist by 
* parsing and interpreting a .vqm file. 
*
*				VQM to BLIF Convertor V.1.2
*
* Author:	S. Whitty
*		June 6, 2011
*
*
* There are two file sources that facilitate a VQM->BLIF conversion, and each has a 
* parser associated with it that populate a data structure with the data recovered.
*
* The t_module structure describes the circuit completely as seen in the VQM file, but
* does not contain any information about the blocks instantiated in it. This is especially
* relevant to open ports, as they are omitted in VQM format but must be referenced in BLIF.
*
* The t_model structure is populated by the same XML Parser as VPR, and it contains the data
* specifying the blocks situated on the FPGA, as well as configuration data. BLIF does not allow
* for parameterization of its models, so each distinct configuration of a block in the circuit
* described by the VQM file must be declared as a distinct model in the resulting BLIF file.
*
* The t_blif_model structure declared by this project collects data from each of these structures
* and organizes it in a manner conducive to printing in proper BLIF format.
*
*				t_module Structure
*  Source:	VQM File
*  Author:	Tomasz Czajkowski
*  Utilized Members:
*	- name
*		A simple C-string containing the top-level module's instance name.
*	- array_of_pins (number_of_pins = array size)
*		An array of t_pin_def structures that describe the external pins and wires
*		on the FPGA as declared at the top of a VQM module. (e.g. "input [31:0] a")
*	- array_of_assignments
*		An array of t_assign structures that describe all direct assign statements in the 
*		VQM module. (e.g. "assign a[15] = b[15]")
*	- array_of_nodes
*		An array of t_node structures that describe the circuit elements in the FPGA, their
*		connectivity to the external pins/wires (array_of_ports; t_node_association structures), 
*		and their configuration (array_of_params; t_node_parameter structures).
*  For more, see "vqm_dll.h" and "vqm_dll.cpp".
*
*				t_model Structure
*  Source:	FPGA Architecture (XML) File
*  Author:	Jason Luu
*  Utilized Members:
*	- name 
*		A simple C-string containing the subcircuit model's technical name. 
*	- inputs, outputs
*		A linked list of t_model_ports structures, each containing name and size data
*		describing the port names and widths.
*  For more, see "logic_types.h", "read_xml_arch_file.h" and "read_xml_arch_file.c"
*********************************************************************************************/

#include "vqm2blif.h"
#include <string.h>

//fstream-specific hacks, found online
#ifdef min 
#undef min 
#endif 
     
#ifdef max 
#undef max 
#endif
//can investigate their relevance

#include <fstream>
#include <sstream>

#include <sys/stat.h>

//============================================================================================
//			GLOBAL VARIABLES
//============================================================================================

t_pin_def hbpad;	//VPR convention for designating an open port in BLIF

t_node_port_association open_port;	//container for an open-port assignment

t_boolean* models_used;		//array of flags indicating whether a model read from
					//the architecture was instantiated in the .vqm file.

t_boolean debug_mode;	//user-set flag that indicates more runtime output
				//and the creation of intermediate files with debug information.

t_boolean verbose_debug;	//user-set flag that indicates more verbose runtime output
					//if debug_mode is set. Does nothing if debug_mode is off.

//============================================================================================
//			INTERNAL FUNCTION DECLARATIONS
//============================================================================================

//Setup Functions
void cmd_line_parse (int argc, char** argv, string* sourcefile, string* archfile, 
			   string* outfile);
	void setup_tokens (tokmap* tokens);
	void print_usage (t_boolean terminate);
	void verify_format (string* filename, string extension);

//Execution Functions
void init_blif_models(t_blif_model* my_model, t_module* my_module, t_arch* arch);
	void subckt_housekeeping(t_model* cur_model);
	void init_subckt_map(portmap* map, t_model_ports* to_be_mapped, t_node* node, boolvec* ports_found);
		void find_and_map (portmap* map, t_model_ports* to_be_mapped, t_node* node, 
				t_boolean is_bus, int index, boolvec* ports_found);
void dump_blif (char* blif_file, t_blif_model* main_model, t_arch* arch);
	void dump_main_model(t_blif_model* model, FILE* fp, t_boolean debug);
		void dump_portlist (FILE* fp, pinvec ports, t_boolean debug);
		void dump_assignments(FILE* fp, t_blif_model* model, t_boolean debug);
		void dump_subckts(FILE* fp, scktvec* subckts, t_boolean debug);
		void dump_subckt_map (FILE* fp, portmap* map, t_model_ports* temp_port, 
					const char* inst_name, const char* maptype, t_boolean debug);
		void dump_subckt_portlist(FILE* fp, t_model_ports* port, t_boolean debug);
	void dump_subckt_models(t_model* temp_model, FILE* fp, t_boolean debug);
void all_data_cleanup();

//Utility Functions
string file_replace(string file, string DNA, string VIRUS);
void construct_dbfile (char* filename, string* path, const char* ext);
string append_index_to_str (string busname, int index);

//Debug functions
void echo_module (char* echo_file, const char* vqm_filename, t_module* my_module);
	void echo_module_pins (FILE* fp, t_module* module);
	void echo_module_assigns (FILE* fp, t_module* module);
	void echo_module_nodes (FILE* fp, t_module* module);
void echo_blif_model (char* echo_file, const char* vqm_filename, 
				t_blif_model* my_model, t_model* temp_model);
//============================================================================================
//			FUNCTIONS
//============================================================================================

int main(int argc, char* argv[])
{

	t_blif_model my_model;
	//holds top-level model data for the .blif netlist
	//***The primary goal of the program is to populate and dump this structure appropriately.***
	
	//***PARSER STRUCTURES***
	t_module* my_module;	//holds information about a module from the VQM Parser
	
	struct s_arch arch;
	t_type_descriptor *types;
	int numTypes;	//used to hold information about the architecture as read by VPR's parser
	
	//***ARGUMENTS***
	string source_file;	//source VQM filename
	string arch_file;		//used to read in models from the architecture file specified.
	string out_file;		//output BLIF filename

	//***OTHER VARIABLES***
	string project_path; 
	//used to derive debug output filenames, found by truncating out_file.
	
	char temp_name [MAX_LEN]; 
	//used to construct output filenames from project info, 
	//char* filename necessitated by vqm_parse_file()

//*************************************************************************************************
//	Begin Conversion
//*************************************************************************************************

	printf("********************\nVQM to BLIF Convertor\nS. Whitty 2011\n********************\n");
	printf("This parser reads a .vqm file and converts it to .blif format.\n\n");
	
	//verify command-line is correct, populate input variables and global mode flags.
	cmd_line_parse(argc, argv, &source_file, &arch_file, &out_file);
	
	size_t cptr = out_file.find_last_of(".");
	project_path = out_file.substr(0, cptr);	//truncate extension
	//project_path contains the output path and project_name based on user input.
			
	//Parse .vqm and express it in memory: from vqm_dll.cpp
      printf("\n>>Parsing VQM file %s\n", source_file.c_str());

	strcpy (temp_name, source_file.c_str());
	my_module = vqm_parse_file(temp_name);	//VQM Parser call
			
	if (debug_mode){
		//Print debug info to "<project_path>_module.echo"
		construct_dbfile (	temp_name, 
						&project_path, 
						"_module.echo");
		printf("\n>>Dumping to output file %s\n", temp_name);
		echo_module (temp_name, source_file.c_str(), my_module);
		//file contains data read from the .vqm structures.
	}
		
	//Parse the architecture file to get full information about the models used.
	printf("\n>>Parsing architecture file %s\n", arch_file.c_str());
	XmlReadArch(arch_file.c_str(), FALSE, &arch, &types, &numTypes);	//Architecture (XML) Parser call
			
	if (debug_mode){	
		//Print debug into to <project_path>_arch.echo"
		construct_dbfile (	temp_name, 
						&project_path, 
						"_arch.echo");
		printf("\n>>Dumping to output file %s\n", temp_name);
		EchoArch(temp_name, types, numTypes, &arch);
		//file contains data read from the architecture file.
	}
		
	//Reorganize netlist data into structures conducive to .blif writing.
	if (debug_mode){
		printf("\n>>Initializing BLIF model\n");
	} else {
		printf("\n>>Converting to BLIF format\n");
	}
	init_blif_models(&my_model, my_module, &arch);
			
	if(debug_mode){			
		//Print debug into to "<project_path>_blif.echo"
		construct_dbfile (	temp_name, 
						&project_path, 
						"_blif.echo");
		printf("\n>>Dumping to output file %s\n", temp_name);
		echo_blif_model (temp_name, source_file.c_str(), &my_model, arch.models);
		//file contains data read from BLIF structures (verbose).
	}
		
	//Print the blif netlist to "<out_file>"
	printf("\n>>Dumping to output file %s (pre-hack)\n", out_file.c_str());
	strcpy (temp_name, out_file.c_str());
	dump_blif (temp_name, &my_model, &arch);
	
#ifdef TARGET_VPR
	if(debug_mode){
		printf("\n>>Implementing hack\n");
	}
	//replaces "gnd" and "vcc" assignments with "hbpad" in the .blif
	//VPR cannot handle constant assignments
	out_file = file_replace (out_file, "gnd", "hbpad");
	out_file = file_replace (out_file, "vcc", "hbpad");
#endif
		
	//Free all allocated memory.
	printf("\n>>Cleaning up\n");
	all_data_cleanup();

	printf("\nComplete.\n");
	return 0;
}

//============================================================================================
//			SETUP FUNCTIONS
//============================================================================================

void cmd_line_parse (int argc, char** argv, string* sourcefile, string* archfile, 
			   string* outfile){

	tokmap CmdTokens;
	tokmap::iterator it;

	setup_tokens (&CmdTokens);

	if (argc < 5)
	{
		printf("ERROR: Not enough arguments.\n"); 
		print_usage(T_TRUE);
	}

	//initialize non-mandatory variables to default values
	debug_mode = T_FALSE;
	verbose_debug = T_FALSE;

	//now read the command line to configure input variables.
	for (int i = 1; i < argc; i++){
		//check if the argument is in the registered tokens map
		it = CmdTokens.find((string)argv[i]);
		if (it != CmdTokens.end()){
			switch(it->second){
				case OT_VQM:
					*sourcefile = (string)argv[i+1];
					verify_format(sourcefile, "vqm");
					i++; //the next argument has been stored the source file
					break;
				case OT_ARCH:
					*archfile = (string)argv[i+1];
					verify_format(archfile, "xml");
					i++; //the next argument has been stored as the architecture file
					break;
				case OT_OUT:
					*outfile = (string)argv[i+1];
					verify_format(outfile, "blif");
					i++; //the next argument has been stored as the output file
					break;
				case OT_DEBUG:
					debug_mode = T_TRUE;
					break;
				case OT_VERBOSE:
					verbose_debug = T_TRUE;
					break;
				default:
					;
			}
		}
	}

	if ((sourcefile->empty()) || (archfile->empty())){
		printf("ERROR: Missing file argument.\n"); 
		print_usage (T_TRUE);
	}

	//default to outputting a file in the home directory of the same root name as the sourcefile.
	size_t cptr = sourcefile->find_last_of("\\/");
	if (outfile->empty()){
		*outfile = sourcefile->substr(cptr+1);	//truncate filepath
		cptr = outfile->find_last_of(".");
		outfile->replace(cptr, 4, ".blif");	//replace ".vqm" with ".blif"
	}	

}

//============================================================================================
//============================================================================================

void setup_tokens (tokmap* tokens){
//populates the map with the tokens the program accepts and the enumeration associated with each
//for switch comparison later.
	tokens->insert(tokpair("-vqm", OT_VQM));
	tokens->insert(tokpair("-arch", OT_ARCH));
	tokens->insert(tokpair("-out", OT_OUT));
	tokens->insert(tokpair("-debug", OT_DEBUG));
	tokens->insert(tokpair("-verbose", OT_VERBOSE));
}

//============================================================================================
//============================================================================================

void print_usage (t_boolean terminate){
//prints a standard usage reminder to the user, terminates the program if directed.
	printf("Usage: vqm2blif -vqm <project_name>.vqm -arch <architecture file>.xml");
	printf(" -out <output file>.blif (OPTIONAL: -debug)\n");
	if (terminate)
		exit(1);
}

//============================================================================================
//============================================================================================

void verify_format (string* filename, string extension){
//verifies that a filename string ends with the desired extension

	size_t cptr = filename->find_last_of(".");
	if ( filename->substr(cptr+1) != extension ){
		printf("ERROR: Improper filename.\n");
		print_usage(T_TRUE);
	}
}

//============================================================================================
//			EXECUTION FUNCTIONS
//============================================================================================

void init_blif_models(t_blif_model* my_model, t_module* my_module, t_arch* arch){
/*  populates a model structure with appropriate parser data.
 *
 *	ARGUMENTS
 *  my_model:
 *	model structure that is to be populated with BLIF data
 *  my_module: 
 *	module structure populated with VQM data
 *  arch:
 *	arch structure populated with Architecture data
 */
	
	//initialize general variables	
	my_model->name = (string)my_module->name;
	
	if(debug_mode){
		printf("\t>>Assigned name\n");
	}

	//Initialize input and output pinvecs of the blif_model
	t_pin_def* temp_pin;
	for (int i = 0; i < my_module->number_of_pins; i++){
		//traverse all pins of the module
		temp_pin = my_module->array_of_pins[i];
		switch (temp_pin->type){
			//sort the pins into their appropriate vectors
			case PIN_INPUT:
				my_model->input_ports.push_back(temp_pin);
				break;
			case PIN_OUTPUT:
				my_model->output_ports.push_back(temp_pin);
				break;
			case PIN_INOUT:
				printf("\n\nERROR: PIN_INOUT detected. Pin: %s\n", temp_pin->name);
				exit(1);
				break;
			default:
				//PIN_WIREs don't need to be declared independantly for proper
				//BLIF format, they appear in the array_of_assignments.
				;	
		}
	}
	
	if(debug_mode){
		printf("\t>>Assigned .inputs and .outputs\n");
	}

	//Initialize the blif_model's assignment array
	my_model->num_assignments = my_module->number_of_assignments;
	my_model->array_of_assignments = my_module->array_of_assignments;
	//data from my_module is already organized nicely, so piggyback onto its allocation
	
	if(debug_mode){
		printf("\t>>Assigned logic\n");
	}
	
	//Initialize Subcircuits
	
	//temporary process variables
	t_blif_subckt temp_subckt;	//from vqm2blif.h
	t_model* cur_model;		//from physical_types.h
	t_node* temp_node;			//from vqm_dll.h
		
	boolvec ports_found;		//verification flags indicating that all ports found in the VQM were mapped.

	//housekeeping initializations before 
	//the subcircuits can be assigned.
	subckt_housekeeping(arch->models);
	
	if(debug_mode){
		printf("\t>>Assigning subcircuits\n");
	}

	for (int i = 0; i < my_module->number_of_nodes; i++){
		//each node is a subcircuit instantiation
		temp_node = my_module->array_of_nodes[i];
		temp_subckt.inst_name = (string)temp_node->name;

		//reset cur_model to search for this node in the models of the Architecture
		cur_model = arch->models;
		
		if(debug_mode){
			printf("\t\tProcessing subcircuit %s", temp_node->name);
		}
	
		ports_found.clear();
		for (int i = 0; i < temp_node->number_of_ports; i++){
			//initialize all flags to zero.
			ports_found.push_back(T_FALSE);
		}

		//traverse through the models declared in the Architecture file
		//to find the one corresponding to the node found in the vqm
		while ((cur_model)&&(strcmp(cur_model->name, temp_node->type) != 0)){
			cur_model = cur_model->next;
		}
		
		if (!cur_model){	
			//cur_model == NULL if the end of the list was reached without success
			printf("\n\nERROR: Model name %s was not found in the architecture.\n", 
					temp_node->type);
			exit(1);
		} else {
			temp_subckt.model_type = cur_model;	//initialize model_type
			models_used[cur_model->index] = T_TRUE;	//indicate this model should appear in the BLIF

			temp_subckt.input_cnxns.clear();	//reset the input and output maps
			temp_subckt.output_cnxns.clear();
			
			//map the input ports of the model read from the architecture
			//to the corresponding external pin as per the vqm
			init_subckt_map(	&(temp_subckt.input_cnxns), 
						temp_subckt.model_type->inputs, 
						temp_node, &ports_found);
			
			//now map the output ports
			init_subckt_map(	&(temp_subckt.output_cnxns), 
						temp_subckt.model_type->outputs, 
						temp_node, &ports_found);
			
			if(debug_mode){
				printf("\n\n");
			}

			//Verify that all ports of the node were mapped to.
			if ((debug_mode) && (verbose_debug)){
				printf ("\tPorts found: ");
			}

			for (int i = 0; i < temp_node->number_of_ports; i++){
				/* The mapping process maps all ports of the architecture either to the open port or
				 * a port in the VQM. If a port appeared in the VQM and not in the 
				 * architecture, the association's corresponding entry in ports_found would 
				 * remain T_FALSE through the mapping process.
				 */
				if ((debug_mode) && (verbose_debug)){
					printf (" %d = %d", i, ports_found[i]);
				}

				if (ports_found[i] == T_FALSE){
					printf("\n\nERROR: Port %s not found in architecture for %s.\n", 
							temp_node->array_of_ports[i]->port_name,
							temp_node->type);
					exit(1);
				}
			}

			if ((debug_mode) && (verbose_debug)){
				printf("\n\n");
			}
			//copy the temp_subckt into the subckt vector
			my_model->subckts.push_back(temp_subckt);
		}
	}
	
	if(debug_mode){
		printf(">>BLIF model initialization complete.\n");
	}
}

//============================================================================================
//============================================================================================

void subckt_housekeeping(t_model* cur_model){
//accomplishes three housekeeping tasks before 
//the subcircuits of a blif_model can be initialized.

//TASK 1: Count the number of models in the architecture.

	int num_models;
	num_models = cur_model->index;
	while(cur_model){
		//cycle through the list, save the highest index
		if (cur_model->index > num_models){
			num_models = cur_model->index;
		}
		cur_model = cur_model->next;
	}
	//number of models = highest index + 1
	num_models++;	 

//TASK 2: Allocate and initialize the models_used array (global).
	models_used = (t_boolean*)malloc((num_models)*sizeof(t_boolean));
	for (int i = 0; i < num_models; i++){
		models_used[i] = T_FALSE;
	}
	
//TASK 3: Initialize the hbpad and open_port structures.
	hbpad.name = (char*)"hbpad";
	hbpad.indexed = T_FALSE;
	hbpad.type = PIN_WIRE;
	hbpad.left = hbpad.right = 0;
	
	open_port.port_name = NULL;
	open_port.associated_net = &hbpad;
	open_port.port_index = 0;
	open_port.wire_index = 0;
	//these structures are mapped to by ports of a block
	//that are not assigned to an external pin.
}

//============================================================================================
//============================================================================================

void init_subckt_map(portmap* map, t_model_ports* to_be_mapped, t_node* node, boolvec* ports_found){
/*  initializes the a portmap based on a linked list of ports and node information
 *	
 *  A portmap associates a t_model_ports structure (from the Architecture Parser)
 *  with a t_node_port_association structure (from the VQM Parser). 
 *
 *  t_model_ports:
 *	stored as a linked list of either inputs or outputs, 
 *	these structures hold names and information about the ports
 *	of a model declared in the architecture.
 *
 *  t_node_port_association:
 *	contains information about assigned ports of a subcircuit
 *	with the name and index of the t_pin_def structure describing
 *	the external net, and the name and index of the port on the block.
 *
 *	ARGUMENTS
 *	
 *  map:
 *	portmap to be populated
 *  to_be_mapped:
 *	beginning of the linked list to be mapped
 *  node:
 *	holds all information about associated ports of the subcircuit.
 *	
 *  If a port in to_be_mapped is unlisted in node->array_of_ports, 
 *  it is associated with open_port.
 */

	while (to_be_mapped){
		//to_be_mapped == NULL if the end of the list was reached
		if (to_be_mapped->size > 1){
			//the port is a bus, so cycle through each wire of it.
			for (int i = 0; i < to_be_mapped->size; i++){
				//cycle through the nodes, find the name and index
				//that correspond to the buswire, and map its
				//"name[index]" to the association.
				find_and_map (map, to_be_mapped, node, T_TRUE, i, ports_found);
			}
		} else {
			//otherwise map the "name" to the association 
			//without an index.
			find_and_map (map, to_be_mapped, node, T_FALSE, 0, ports_found);
		}
		//move to the next port in the model
		to_be_mapped = to_be_mapped->next;
	}
}

//============================================================================================
//============================================================================================
void find_and_map (portmap* map, t_model_ports* to_be_mapped, t_node* node, 
				t_boolean is_bus, int index, boolvec* ports_found){

//map a subcircuit port from the architecture to a port association from the vqm.
//A string is generated from the name and index of the port being mapped, and this string
//is mapped to a t_node_port_association structure.
	t_boolean found_in_ports;

	string temp_name;
	temp_name = (string)to_be_mapped->name;	//copy the name of the subcircuit port

	if (is_bus){
		//append the index to the string if it's a bus; "name[index]"
		temp_name = append_index_to_str(temp_name, index);
	}
		
	found_in_ports = T_FALSE;	//reset search flag
	for (int i = 0; i < node->number_of_ports; i++){
		//search through the node's assigned port list to see 
		//if the current port has been specified
		if ((strcmp(to_be_mapped->name, node->array_of_ports[i]->port_name) == 0)
			&& ((is_bus)? (index == node->array_of_ports[i]->port_index):(1))){
			//match the name of the subcircuit port to one mentioned in the association
			//match the index of the subcircuit port as well, if it's a bus.
			found_in_ports = T_TRUE;
			if((debug_mode) && (verbose_debug)){
				printf("\n\t\t-> Mapping %s to %s", 
						temp_name.c_str(), 
						node->array_of_ports[i]->associated_net->name);
			}
			//map the port to its connectivity data
			map->insert(portpair(temp_name, node->array_of_ports[i]));

			//indicate that the port has been successfuly mapped to
			ports_found->at(i) = T_TRUE;
			break;
		}
	}

	if (found_in_ports == T_FALSE){
		//if the port wasn't in node->array_of_ports, map it to the open_port
		if((debug_mode) && (verbose_debug)){	
			printf("\n\t\t-> Mapping %s to %s", 
					temp_name.c_str(), 
					open_port.associated_net->name);
		}
		
		map->insert(portpair(temp_name, &open_port));
	}
}
//============================================================================================
//============================================================================================

void dump_blif (char* blif_file, t_blif_model* main_model, t_arch* arch){
/*  uses a populated model structure to construct a .blif netlist.
 *
 *	ARGUMENTS
 *  blif_file:
 *	name of the target .blif file.
 *  main_model:
 *	pointer to the model structure to be elaborated.
 */
	FILE* fp;
	
	// Open file to dump data.
	fp = fopen(blif_file,"w");
	if (!fp){
		printf("ERROR: Cannot open file %s\n", blif_file);
		exit(1);
	}
	
	//comments in BLIF begin with '#'
	fprintf(fp, "#BLIF OUTPUT: %s\n", blif_file);
	fprintf(fp, "\n#MAIN MODEL\n");
	
	//completely dump the top-level model
	dump_main_model(main_model, fp, T_FALSE);
	
	fprintf(fp, "\n#SUBCKT MODELS\n");
	
	//now dump the subckt models from the architecture
	//that were used in the vqm
	dump_subckt_models(arch->models, fp, T_FALSE);
	
	// Close file.
	fclose(fp);
}

//============================================================================================
//============================================================================================

void dump_main_model(t_blif_model* model, FILE* fp, t_boolean debug){
/*  dumps information stored in a model structure in proper BLIF syntax.
 *
 *	ARGUMENTS
 *  model:
 *	pointer to model structure to be dumped.
 *  fp:
 *	pointer to the target blif file.
 *  debug:
 *	flag to indicate whether to print in BLIF or DEBUG format.
 *
 *  BLIF syntax described in blif_info.pdf
 */
 
	fprintf(fp, (debug)? "\nModel Name: %s\n":"\n.model %s\n", model->name.c_str());

	//series of conditional statements define which sections of a blif netlist
	//to print based on the model data.
	 
	//Dump inputs: ".inputs <name> <name> ..."
	if (model->input_ports.size() > 0){
		fprintf(fp, (debug)? "Inputs:\n":".inputs");
		dump_portlist (fp, model->input_ports, debug);
	} 
	
	//Dump outputs: ".outputs <name> <name> ..."
	if (model->output_ports.size() > 0){
		fprintf(fp, (debug)? "Outputs:\n":".outputs");
		dump_portlist (fp, model->output_ports, debug);		
	} 
	
	//Dump clocks: ".clock <name> <name> ..."
	if (model->clock_ports.size() > 0){
		fprintf(fp, (debug)? "Clocks:\n":".clock");
		dump_portlist (fp, model->clock_ports, debug);	
	}
	
	fprintf(fp, "\n");
	
	//Print Assignment Variable information
	if (model->num_assignments > 0){
		dump_assignments(fp, model, debug);
	}
	
	//Print Subcircuit Variable information
	if (model->subckts.size() > 0){
		dump_subckts(fp, &(model->subckts), debug);
	}
	
	//finished dumping the model.
	fprintf(fp, (debug)?"\n":"\n.end\n");
}

//============================================================================================
//============================================================================================

void dump_portlist (FILE* fp, pinvec ports, t_boolean debug){
/*  Lists all the port names in the pinvec on one line in proper BLIF syntax,
 *  flattening any bussed ports into indexed notation.
 *  Debug format has each port on its own line, BLIF lists them on a single line.
 *
 *  NOTE: In VQM/Verilog syntax, buses are declared as
 *  {input, output, clock, wire} [ left : right ] <name>
 *  where left and right are indeces that describe the
 *  bus width.
 */
	int limit = ports.size();
	t_pin_def* temp_pin;
	string temp_name;
	int ports_dumped = 1;

	for (int i = 0; i < limit; i++, ports_dumped++){
		//cycle through all ports
		temp_pin = ports[i];
		
		for (	int j = min(temp_pin->left, temp_pin->right);
			j <= max(temp_pin->left, temp_pin->right); 
			j++, ports_dumped++){
				//if the pin is non-indexed, iterates once and doesn't append an index.
				//otherwise, flattens a bus into component wires using index convention.

				temp_name = (string)temp_pin->name;

				if (temp_pin->indexed){	
					temp_name = append_index_to_str(temp_name, j);
				}

				fprintf(fp, " %s", temp_name.c_str());

				if (debug){
					fprintf(fp, "\n");	//one port per line in DEBUG file
				} else if (ports_dumped % MAX_PPL == 0){
					fprintf(fp, "\\\n");	//prevents ridiculously long lines in BLIF file
				}
		}
	}	

	fprintf(fp, "\n");
}

//============================================================================================
//============================================================================================

void dump_assignments(FILE* fp, t_blif_model* model, t_boolean debug){
/*  dumps all assignment information from model's array_of_assignments
 *  in BLIF or DEBUG syntax to fp.
 *
 *  DEBUG syntax:
 *	assign a = b	OR	assign a = !b	OR	assign a = 1'b0
 *
 *  BLIF syntax:
 *	.names a b		OR	.names a b		OR	.names a
 *	1 1				0 1				0	
 */
 
	t_assign* temp_assign;
	string temp_name;

	for (int i = 0; i < model->num_assignments; i++){
		temp_assign = model->array_of_assignments[i];
		if (temp_assign->source == NULL){
			//Constant Assignment

			#ifdef TARGET_VPR
				continue;
			#endif

			//VPR errors out on "constant generators" (sourceless assigns),
			//so constant assignments are not printed in BLIF if
			//VPR is the target. 
			fprintf(fp, "\n%s ", (debug)? "assign":".names");
			temp_name = (string)temp_assign->target->name;

			if (temp_assign->target->indexed) {
				temp_name = append_index_to_str(temp_name, temp_assign->target_index);
			} 

			fprintf (fp, "%s", temp_name.c_str());

			if (debug){
				//Print DEBUG syntax
				fprintf(fp, " = %d\n", temp_assign->value);
			} else {
				//Print BLIF syntax
				fprintf(fp, "\n%d\n", temp_assign->value);
			}
		} else {
			//Wire/Pin Assignment

			fprintf(fp, "\n%s ", (debug)? "assign":".names");
			temp_name = (string)temp_assign->source->name;

			//Print source name and index (if applicable)
			if (temp_assign->source->indexed) {
				temp_name = append_index_to_str(temp_name, temp_assign->source_index);					
			} 	

			fprintf(fp, "%s ", temp_name.c_str());
	
			if (debug){
				//Print DEBUG logic
				if (temp_assign->inversion)
					fprintf(fp, "= !");
				else
					fprintf(fp, "= ");
			}

			temp_name = (string)temp_assign->target->name;

			//Print target name and index (if applicable)
			if (temp_assign->target->indexed){
				temp_name = append_index_to_str(temp_name, temp_assign->target_index);
			}
			
			fprintf(fp, "%s\n", temp_name.c_str());

			if (!debug){
				//Print BLIF logic
				if (temp_assign->inversion)
					fprintf(fp, "0 1\n");
				else
					fprintf(fp, "1 1\n");
			}
		}
	}
}

//============================================================================================
//============================================================================================

void dump_subckts(FILE* fp, scktvec* subckts, t_boolean debug){
//traverse the subcircuit vector, dumping out the names and connections
//of each instantiated subcircuit in the main model.

	t_blif_subckt* temp_subckt;
	
	int limit;
	limit = subckts->size(); //doesn't run size() on each iter of the loop
	for(int i = 0; i < limit; i++){
		//print each subcircuit's name, port, and connectivity information
		temp_subckt = &(subckts->at(i));
		fprintf(fp, (debug) ? "\nInstance Name: %s\n Type: %s\n" : "\n#  %s \n.subckt %s \\\n",
				temp_subckt->inst_name.c_str(), 
				temp_subckt->model_type->name);
			
		if (debug)
			fprintf(fp, "Input Map:\n");
		
		//dump the input map containing connectivity data
		dump_subckt_map(fp, 	&(temp_subckt->input_cnxns), 
					temp_subckt->model_type->inputs, 
					temp_subckt->inst_name.c_str(), 
					"input", 
					debug);
			
		fprintf(fp, (debug) ? "\nOutput Map:\n" : "\\\n");
			
		//dump the output map containing connectivity data
		dump_subckt_map(fp, 	&(temp_subckt->output_cnxns), 
					temp_subckt->model_type->outputs, 
					temp_subckt->inst_name.c_str(),
					"output", 
					debug);

		fprintf(fp, "\n");
	}
}

//============================================================================================
//============================================================================================

void dump_subckt_map (FILE* fp, portmap* map, t_model_ports* temp_port, 
			const char* inst_name, const char* maptype, t_boolean debug){
/*  dumps a subcircuit connectivity map, showing all connections
 *  the keys to the map are the names of the ports in the model associated with the subcircuit.
 *
 *  fp:
 *	file to dump the map information to.
 *  map:
 *	portmap containing the linkage between a port and an external pin association.
 *  temp_port:
 *	linked list of ports declared in the model, generates map keys.
 *  inst_name:
 *	name of subcircuit instantiation, for debugging error messages.
 *  maptype:
 *	either "input" or "output", for debugging error messages.
 *  debug:
 *	flag indicating whether to output in DEBUG or BLIF format.
 */
	t_node_port_association* temp_cnxn;
	string port_name;
	string pin_name;

	int ports_dumped = 1;

	while(temp_port){
		for (int i = 0; i < temp_port->size; i++, ports_dumped++){
			if (temp_port->size > 1){
				//construct a string of "name"["index"]
				port_name = append_index_to_str((string)temp_port->name, i);
			} else {
				port_name = (string)temp_port->name;
			}
			temp_cnxn = map->find(port_name)->second;	
			//"second" is a member of portpair,
			//corresponds to the connection the port is mapped to
			if (temp_cnxn == NULL) {
				printf("ERROR: %s not found in the %s map of %s.\n", 
						temp_port->name, 
						maptype, 
						inst_name);
				exit(1);
			}

			pin_name = (string)temp_cnxn->associated_net->name;
			if (temp_cnxn->associated_net->indexed){
				//construct a string of "name[index]"
				pin_name = append_index_to_str (pin_name, temp_cnxn->wire_index);
			}

			if (debug)
				fprintf (fp, "\t");

			fprintf (fp, "%s=%s ", port_name.c_str(), pin_name.c_str());

			if (debug){
				fprintf(fp, "\n");	//create a new line for each mapped port in DEBUG file
			} else if (ports_dumped % MAX_PPL == 0){
				fprintf(fp, "\\\n");	//prevents ridiculously long lines in the BLIF file
			}
		}

		//dump the next port's information
		temp_port = temp_port->next;
	}
}

//============================================================================================
//============================================================================================

void dump_subckt_models(t_model* temp_model, FILE* fp, t_boolean debug){
/*  cycles through all models declared in the architecture
 *  and dumps the ones used by the VQM as blackbox models
 *  at the end of the .blif file
 *
 *  temp_model:
 *	first model in the linked list from the 
 *	Architecture parser
 *  fp:
 *	.blif file to output to
 *  debug:
 *	flag to indicate whether to print in DEBUG or BLIF format.
 */	
	while(temp_model){
		if (models_used[temp_model->index]){
			//dump all .subckt models declared in the architecture 
			//that are flagged as "used" in the models_used array
			fprintf(fp, (debug)? "\n Model: %s\n" : "\n.model %s\n", temp_model->name);
			
			fprintf(fp, (debug)? "Inputs:\n" : ".inputs "); //cycle through all inputs
			dump_subckt_portlist(fp, temp_model->inputs, debug);

			fprintf(fp, (debug)? "Outputs:\n" : "\n.outputs "); //cycle through all outputs
			dump_subckt_portlist(fp, temp_model->outputs, debug);

			fprintf(fp, (debug)? "\nEND MODEL\n" : "\n.blackbox\n.end\n");
		}	
		temp_model = temp_model->next;
	}
}

//============================================================================================
//============================================================================================

void dump_subckt_portlist(FILE* fp, t_model_ports* port, t_boolean debug){
//dumps the portlist of a subcircuit model, flattening busses as necessary.
	string temp_name;
	int ports_dumped = 1;

	while (port){	
		for (int i = 0; i < port->size; i++, ports_dumped++){
			temp_name = (string)port->name;

			if (port->size > 1){	//append the index if necessary using the established convention
				temp_name = append_index_to_str(temp_name, i);
			}

			//dump the name of the port
			fprintf(fp, (debug)? "%s\n":"%s ", temp_name.c_str());
			
			if ((!debug)&&(ports_dumped % MAX_PPL == 0)){
				fprintf(fp, "\\\n");		//prevents ridiculously long lines in BLIF file
			}
		}
		port = port->next;
	}
	if (debug)
		fprintf(fp, "\n");
}

//============================================================================================
//				UTILITY FUNCTIONS
//============================================================================================

void all_data_cleanup(){
/* frees all allocated memory from the parser and convertor.
 */
	vqm_data_cleanup();//found in ../LIB/vqm_dll.h, frees parser-allocated memory

	free(models_used);
	
	return;
}

//============================================================================================
//============================================================================================

string file_replace(string file, string DNA, string VIRUS){
/*  uses ifstream and string functions to parse the file,
 *  replacing a target DNA string with a VIRUS string and outputting to a new file.
 *	
 *  file:
 *	the filepath and name to be altered, kept a string for editing purposes
 *  DNA:
 *	specific character sequence to be targeted within the file.
 *  VIRUS:
 *	specific character sequence to insert within the file in place of DNA.
 */
	string sequence;
	size_t cptr; 
	
	if(debug_mode){
		printf("\t>>Opening file %s\n", file.c_str());
	}
	ifstream in_genome (file.c_str());
	
	cptr = file.find_last_of(".");
	file.replace(cptr, 1, "_no_" + DNA + ".");  //append "_no_<DNA>" to the filename.
  	ofstream out_genome (file.c_str());

	if((debug_mode)&&(!verbose_debug)){
			printf("\t\tReplacing instances of %s with %s...\n", DNA.c_str(), VIRUS.c_str());
	}

	int line_number = 0;
	//read in_genome line-by-line
	while(getline(in_genome, sequence).good()){ //good() indicates a successful line read.
		line_number++;
		cptr = sequence.find(DNA);
		//cptr points to the first instance of the target DNA string, if it exists in the line.
		while (cptr != string::npos){
			//cptr == string::npos if DNA wasn't found in this line

			if((debug_mode)&&(verbose_debug)){
				printf("\t\t\tReplacing %s with %s on line %d\n", 
							DNA.c_str(), 
							VIRUS.c_str(), 
							line_number);
			}
			//replace the next <DNA.length()> chars in the line with VIRUS using string::replace...
			sequence.replace(cptr, DNA.length(), VIRUS);	
			//...and find the next DNA sequence, if it exists.
			cptr = sequence.find(DNA);
		}
		//print the infected line to out_genome
		out_genome << sequence << endl;
	}

	//has been dumping all along, but looks cleaner to output now.
	printf("\n>>Dumping to output file %s (post-hack- no %s)\n", file.c_str(), DNA.c_str()); 
	in_genome.close();
	out_genome.close();

	//return the altered filename
	return file;
}

//============================================================================================
//============================================================================================

void construct_dbfile (char* filename, string* path, const char* ext){
//constructs a char* filename from a given path and extension.

	strcpy(filename, path->c_str());	//add the path
	strcat(filename, ext);			//add the desired extension
	
	return;
}

//============================================================================================
//============================================================================================

string append_index_to_str (string busname, int index){
/*Append a given index onto the end of a bus name. 
 *
 *  busname:
 *	string containing the name of a bus; e.g. "a"
 *  index:
 *	integer representing some index within the bus; e.g. 15
 *
 *  RETURN:
 *	a string appended with the index, according to a convention; e.g. "a[15]" or "a~15"
 */
	stringstream ss;
	ss << index;

	return (busname + '~' + ss.str() );
}

//============================================================================================
//			DEBUG FUNCTIONS
//============================================================================================

void echo_module (char* echo_file, const char* vqm_filename, t_module* my_module){
/*  Prints the raw data parsed from the VQM file in three sections:
*
 * 		PIN INFORMATION
 *	All inputs, outputs, inouts, and wires declared in the circuit
 *	and their indeces, if applicable. Equivalent to the beginning
 *	declarations in the VQM Module. ("input [31:0] ACCout", "wire gnd", etc.)
 *
 *		ASSIGNMENT INFORMATION
 *	All statements in the VQM that declare "assign x = y" have their data
 *	stored here. Constant assignments are also stored. 
 *
 *		NODE INFORMATION
 *	All submodule instantiations with their name, port assignments, and 
 *	parameters are stored here. Note that open ports of a block 
 *	are not stored by the parser as they do not appear in VQM format.
 *
 *  echo_file:
 *	Name of file printing vqm data.
 *  vqm_filename:
 *	Name of vqm file parsed.
 *  my_module:
 *	Module containing vqm data from the VQM Parser.
 */

	FILE *fp;
	
	// Open file to dump data.
	fp = fopen(echo_file,"w");
	if (!fp){
		printf("ERROR: Cannot open file %s\n", echo_file);
		exit(1);
	}

	fprintf(fp,"Input netlist file: %s\n\n", vqm_filename);

	fprintf(fp,"Module name: %s\n\n", my_module->name);
	
	echo_module_pins (fp, my_module);

	echo_module_assigns (fp, my_module);
	
	echo_module_nodes (fp, my_module);

	// Close file.
	fclose(fp);
 
}

//============================================================================================
//============================================================================================

void echo_module_pins (FILE* fp, t_module* module){
/* Echo all pin information from the VQM module.
 * Each pin has:
 *  - a name
 *  - right and left indeces
 *  - a type (direction)
 * 
 *  fp:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */	
	t_pin_def* temp_pin;

	fprintf(fp, "\t\t***PIN INFORMATION***\n");
	fprintf(fp,"Number of pins: %d\n\n", module->number_of_pins);
	for (int i = 0; i < module->number_of_pins; i++){
		//Example pin declaration: "input [31:0] a"
		temp_pin = module->array_of_pins[i];
		
		//name = "a"
		fprintf(fp, "Pin: %d ; Name: %s\n", i, temp_pin->name);
		
		//right = 31, left = 0, indexed = true
		fprintf(fp, "\tLeft: %d ; Right: %d ; Indexed: %s\n", 
					temp_pin->left, 
					temp_pin->right, 
					(temp_pin->indexed)?"true":"false");

		//type = PIN_INPUT
		fprintf(fp, "\tType: ");
		switch (temp_pin->type){
			case PIN_INPUT:
				fprintf (fp, "Input\n\n");
				break;
			case PIN_OUTPUT:
				fprintf (fp, "Output\n\n");
				break;
			case PIN_WIRE:
				fprintf (fp, "Wire\n\n");
				break;
			case PIN_INOUT:
				fprintf(fp, "Inout : ERROR\n\n");
				printf("\n\nERROR: Detected inout pin in t_module.\n");
				exit(1);
			default:
				fprintf(fp, "Unknown: ERROR\n\n");
				printf("\n\nERROR: Detected unknown pintype in t_module.\n");
				exit(1);
		}
	}
}

//============================================================================================
//============================================================================================

void echo_module_assigns (FILE* fp, t_module* module){
/* Echo all assignment information from the VQM module.
 * each assignment has: 
 *  - a target wire or pin (with an index) 
 *  - a value if it's a constant assignment
 *  - a source (with an index) if it's a pin assignment
 *  - flags indicating tristated and inverted assignments
 * 
 *  fp:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */
	t_assign* temp_assign;

	fprintf(fp, "\t\t***ASSIGNMENT INFORMATION***\n");
	fprintf(fp, "Number of assignments: %d\n\n", module->number_of_assignments);

	for (int i = 0; i < module->number_of_assignments; i++){
		temp_assign = module->array_of_assignments[i];

		//Echo Target wire/pin data.
		fprintf(fp, "Assignment: %d ; Target: %s ; Target Index: %d\n", 
					i, 
					temp_assign->target->name, 
					temp_assign->target_index);

		//Echo Source data.
		fprintf(fp, "\tSource: ");
		if (temp_assign->source == NULL){
			//Constant assignment; e.g. assign gnd = 1'b0
			fprintf(fp, "Constant ; Value: %d\n", temp_assign->value);
		} else {
			//Wire/pin assignment; e.g. assign ACCout[10] = ACCout[10]~O
			fprintf(fp, "%s ; Source Index: %d\n", 
					temp_assign->source->name, 
					temp_assign->source_index);
		}

		//Echo tristated and inversion flag data.
		fprintf(fp, "\tis_tristated: ");
		if (temp_assign->is_tristated){
			//Tristated assignments have a third associated wire/pin
			fprintf(fp, "true ; tri_control name: %s ; Index: %d\n", 
					temp_assign->tri_control->name, 
					temp_assign->tri_control_index);
		} else {
			fprintf(fp, "false\n");
		}
		fprintf(fp, "\tinversion: %s\n\n", (temp_assign->inversion)?"true":"false");
	}
}

//============================================================================================
//============================================================================================

void echo_module_nodes (FILE* fp, t_module* module){
/* Echo all node information from the VQM module.
 * Each node has:
 *  - a name
 *  - an array of parameters (string or integer)
 *  - an array of assigned ports with pointers to the nets they're 
 *	assigned to.
 *
 *  fp:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */
	t_node* temp_node;
	t_node_parameter* temp_param;
	t_node_port_association* temp_port;

	fprintf(fp, "\n\t\t***NODE INFORMATION***\n");
	fprintf(fp, "Number of nodes: %d\n\n", module->number_of_nodes);

	for (int i = 0; i < module->number_of_nodes; i++){
		temp_node = module->array_of_nodes[i];

		fprintf(fp, "Node: %d ; Name: %s ; Type: %s\n\n", 
					i, 
					temp_node->name, 
					temp_node->type);

		//Echo all parameter information from the array
		fprintf(fp, "\tNumber of params: %d\n\n", temp_node->number_of_params);
		for (int j = 0; j < temp_node->number_of_params; j++){
			//Each parameter specifies a configuration of the node in the circuit.
			temp_param = temp_node->array_of_params[j];
			fprintf(fp, "\t\tParam: %d ; Name: %s\n", j, temp_param->name);
			
			if (temp_param->type == NODE_PARAMETER_STRING){
				//String Parameter; e.g. operation_mode = "arithmetic"
				fprintf(fp, "\t\t\tType: String ; Value: %s\n\n", 
						temp_param->value.string_value);
			} else {
				//Integer Parameter; e.g. dataa_width = 8
				fprintf(fp, "\t\t\tType: Integer ; Value: %d\n\n", 
						temp_param->value.integer_value);
			}
		}

		//Echo all port information from the array
		fprintf(fp, "\tNumber of ports: %d\n\n", temp_node->number_of_ports);
		for (int j = 0; j < temp_node->number_of_ports; j++){
			//Each port specifies an association with an external pin and
			//an input or output to the node; e.g. ".portname(WIRE_NAME)"
			temp_port = temp_node->array_of_ports[j];
			fprintf(fp, "\t\tPort: %d ; Name: %s ; Port Index: %d\n", 
						j, 
						temp_port->port_name, 
						temp_port->port_index);

			if (temp_port->associated_net == NULL){
				//Open port, should never get here.
				fprintf(fp, "\t\t\tAssociated: open\n\n");
			} else {
				//associated_net is a t_pin_def* structure, just as in
				//echo_module_pins. 
				fprintf(fp, "\t\t\tAssociated: %s ; Wire Index: %d\n\n", 
						temp_port->associated_net->name, 
						temp_port->wire_index);
			}
		}
		fprintf(fp, "\n");
	}
}

//============================================================================================
//============================================================================================

void echo_blif_model (char* echo_file, const char* vqm_filename, 
				t_blif_model* my_model, t_model* temp_model) {
/*  Dumps all model data into a .txt for debugging
 *  Used to ensure correct population of the parser data into the model.
 *
 *	ARGUMENTS
 *  echo_file:
 *	target filename to dump model data.
 *  vqm_filename:
 *	source .vqm filename
 *  my_model:
 *	pointer to the model structure with data to be dumped.
 */

	FILE *fp;		// Points to file printing blif data.
	
	// Open file to dump data.
	fp = fopen(echo_file,"w");
	if (!fp){
		printf("\n\nERROR: Cannot open file %s\n", echo_file);
		exit(1);
	}
	fprintf(fp,"\t\t### DEBUG FILE FOR BLIF MODEL: %s###\n", my_model->name.c_str()); 
	fprintf(fp,"Input netlist file: %s\n\n",vqm_filename);
	
	fprintf(fp, "\n\tMAIN MODEL\n");
	
	//completely dump the top-level model in DEBUG format
	dump_main_model(my_model, fp, T_TRUE);
	
	fprintf(fp, "\n\tSUBCKT MODELS\n");
	
	//now dump the subckt models from the architecture
	//that were used in the VQM file, in DEBUG format.
	dump_subckt_models(temp_model, fp, T_TRUE);
	
	// Close file.
	fclose(fp);
}

//============================================================================================
//============================================================================================