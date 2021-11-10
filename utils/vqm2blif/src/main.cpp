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
* Copyright (C)  2011 by 	Scott Whitty, Jason Luu, Jonathan Rose and Vaughn Betz
*					The Edward S. Rogers Sr. Department of Electrical and Computer Engineering
*					University of Toronto
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
*			THEORY
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
*  Data Source:	VQM File
*  Author:	Tomasz Czajkowski
*  Utilized Members:
*	- name
*		A simple C-string containing the top-level module's instance name.
*	- array_of_pins (number_of_pins = array size)
*		An array of t_pin_def structures that describe the external pins and wires
*		on the FPGA as declared at the top of a VQM module. (e.g. "input [31:0] a" or "wire b")
*	- array_of_assignments
*		An array of t_assign structures that describe all direct assign statements in the 
*		VQM module. (e.g. "assign a[15] = b[15]" or "assign a_bus = b_bus")
*	- array_of_nodes
*		An array of t_node structures that describe the circuit elements in the FPGA, their
*		connectivity to the external pins/wires (array_of_ports; t_node_association structures), 
*		and their configuration (array_of_params; t_node_parameter structures).
*  For more, see "vqm_dll.h" and "vqm_dll.cpp" in libvqm
*
*				t_model Structure
*  Data Source:	FPGA Architecture (XML) File
*  Author:	Jason Luu
*  Utilized Members:
*	- name 
*		A simple C-string containing the subcircuit model's technical name. 
*	- inputs, outputs
*		A linked list of t_model_ports structures, each containing name and size data
*		describing the port names and widths.
*  For more, see "logic_types.h", "read_xml_arch_file.h" and "read_xml_arch_file.c" in libvpr
*********************************************************************************************/

#include "vqm2blif.h"
#include "lut_stats.h"
#include "vtr_error.h"
#include "physical_types.h"
#include "hard_block_recog.h"

#include <sys/stat.h>

//============================================================================================
//			GLOBAL VARIABLES
//============================================================================================

t_pin_def unconn;	//VPR convention for designating an open port in BLIF

t_node_port_association open_port;	//container for an open-port assignment

lut_support_map supported_luts;	//map structure for verifying a VQM primitive against supported
					//LUT configurations (see lut_regoc.h & lut_recog.cpp)


int* model_count;		//array of flags indicating whether a model read from
					//the architecture was instantiated in the .vqm file.

t_boolean debug_mode;	//user-set flag causing the creation of intermediate files 
				//with debug information.

t_boolean verbose_mode;	//user-set flag that indicates more verbose runtime output

e_elab elab_mode;		//user-set flag dictating how to elaborate a VQM Primitive

e_lut lut_mode;		//user-set flag dictating how to treat LUTs (as blackboxes or .names)


int buffer_count, invert_count, onelut_count;
int buffers_elim, inverts_elim, oneluts_elim;

e_clean clean_mode;

t_boolean buffd_outs;	//user-set flag that regulates whether to keep buffered outputs

t_boolean fix_global_nets;  //user-set flag which controls whether to insert fake buffers in 
                            //an attempt to make VPR treat some signals (i.e. clocks) as 
                            //globals

t_boolean elaborate_ram_clocks; //user-set flag which controls whether rams have their clocks
                                //explicitly elaborated so the clock associated with in/out data
                                //for each port are explicit

t_boolean single_clock_primitives; //user-set flag which controls whether multiclock blocks have
                                   // their extra clocks dropped.  This is a work around for
                                   // VPR's limitaiton of one clock per primitive.

t_boolean split_carry_chain_logic; //user-set flag which controls whether to decompose carry chain
                                   //logic cells into their constituent logic (LUT) and arithmetic 
                                   //(ADDER) components.  May be useful for people working on logic
                                   //re-synthesis who want to re-synthesize the logic feeding the
                                   //adder in a Stratix IV like architecture
t_boolean remove_const_nets; //user-set flag which controls whether to sweep away constant nets
t_boolean print_unused_subckt_pins; //user-set flag which controls whether subckts included unused pins (if true)
                                    //or if they are omitted (if false). Some BLIF readers require the unused pins
                                    //to be listed (and connected to nets with no drivers/sinks), which would require
                                    //this option to be true.
t_boolean eblif_format;             //If true, writes circuit in extended BLIF (.eblif) format (supported by YOSYS & VPR)

t_boolean insert_custom_hard_blocks; // user-set flag. Which if true, helps find and filter user-defined hard blocks within the netlist (based on the supplied hard block names) and then instantiates the hard blocks within the blif file. 
									// or if false, then the netlist is processed without identifying any hard blocks.
									// this option should only be used if the original user-design contained custom hard-blocks.
									// The user-defined hard block names need to be provided

//============================================================================================
//			FUNCTION DECLARATIONS
//============================================================================================

//Setup Functions
void cmd_line_parse (int argc, char** argv, string* sourcefile, string* archfile, string* outfile,
			   string* device, std::vector<std::string>* hard_block_list);
	void setup_tokens (tokmap* tokens);

//Execution Functions
void init_blif_models(t_blif_model* my_model, t_module* my_module, t_arch* arch);
	void subckt_prep(t_model* cur_model);
	void init_blif_subckts (t_node **vqm_nodes, int number_of_vqm_nodes, 
						t_model *arch_models, t_blif_model* my_model);

	//1-to-1 Subcircuit Function
	//Translates one VQM Node into one BLIF Subcircuit, appends a mode-hash based on 
	//parameters if elab_mode is MODES or MODES_TIMING.
	void push_node_1_to_1 (t_node* vqm_node, t_model* arch_models, scktvec* blif_subckts);

	//Atomize Subcircuit Function
	//Uses select parameters and a dictionary to atomize each block and
	//name it as the dictionary says.
	void push_node_atomize (t_node* vqm_node, t_model* arch_models, scktvec* blif_subckts);

	//General Subcircuit Initialization
	//No matter the translation, models from the architecture get turned into
	//subcircuits and they are connected appropriately.
	void init_subckt_map(portmap* map, t_model_ports* to_be_mapped, 
				t_node* node, boolvec* vqm_ports_found);
	void find_and_map (portmap* map, t_model_ports* to_be_mapped, t_node* node, 
				t_boolean is_bus, int index, boolvec* vqm_ports_found);

	//LUT Elaboration Functions
	// - Verifies that a node is a LUT, and expands a blif_model's LUT-vector accordingly
	void push_lut (t_node* vqm_node, lutvec* blif_luts);

//Output Functions

void dump_blif (char* blif_file, t_blif_model* main_model, t_arch* arch, t_boolean print_unused_subckt_pins);

void dump_main_model(t_blif_model* model, ofstream& outfile, t_boolean print_unused_subckt_pins, t_boolean eblif_format, t_boolean debug);

void dump_portlist (ofstream& outfile, pinvec ports, t_boolean debug);
void dump_assignments(ofstream& outfile, t_blif_model* model, t_boolean eblif_format, t_boolean debug);

#ifdef VQM_BUSES
void dump_bus_assign(ofstream& outfile, string target_name, 
        int target_left, int target_right, int target_dir,
        t_boolean is_constant, 
            string source_name, int source_left, int source_right, int source_dir,
            int value, 
        t_boolean debug, t_boolean inversion);

string file_replace(string file, string strip, string replace);
#endif
void dump_wire_assign(ofstream& outfile, string target_name, 
                t_boolean is_constant, string source_name, int value, 
                t_boolean eblif_format, t_boolean debug, t_boolean inversion);

void dump_luts (ofstream& outfile, lutvec* blif_luts, t_boolean eblif_format, t_boolean debug);

void dump_subckts(ofstream& outfile, scktvec* subckts, t_boolean print_unused_pins, t_boolean eblif_format, t_boolean debug);

void dump_subckt_map (ofstream& outfile, portmap* map, t_model_ports* temp_port, 
                const char* inst_name, const char* maptype, int s_index, t_boolean print_unused_pins, t_boolean debug, bool last);
size_t count_print_pins(t_model_ports* temp_port, portmap* map, t_boolean print_unused);

void dump_subckt_portlist(ofstream& outfile, t_model_ports* port, string indent, t_boolean debug);

void dump_subckt_models(t_model* temp_model, ofstream& outfile, t_boolean debug);

//Debug functions
void echo_module (char* echo_file, const char* vqm_filename, t_module* my_module);
void echo_module_pins (ofstream& outfile, t_module* module);
void echo_module_assigns (ofstream& outfile, t_module* module);
void echo_module_nodes (ofstream& outfile, t_module* module);
void echo_blif_model (char* echo_file, const char* vqm_filename, 
				t_blif_model* my_model, t_model* temp_model);


//Other Functions

	void all_data_cleanup();	//frees all allocated memory

//============================================================================================
//			FUNCTION IMPLEMENTATIONS
//============================================================================================

int main(int argc, char* argv[])
{

	t_blif_model my_model;
	//holds top-level model data for the .blif netlist
	//***The primary goal of the program is to populate and dump this structure appropriately.***
	
	//***PARSER STRUCTURES***
	t_module* my_module;	//holds information about a module from the VQM Parser
	
	t_arch arch;
    arch.power = NULL; //Must explicitly set power to null, so that libarchfpga 
                       //doesn't attempt to read in power info if it doesn't exist

    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

	t_logical_block_type *types;
	int numTypes;	//used to hold information about the architecture as read by VPR's parser
	
	//***COMMAND-LINE ARGUMENTS***
	string source_file;	//source VQM filename
	string arch_file;		//source ARCH filename
	string out_file;		//output BLIF filename
	//debug_mode and verbose_mode are global flags

	//***OTHER VARIABLES***
	string project_path; 
	//found by truncating out_file, used to derive debug filenames.
	
	char temp_name [MAX_LEN]; 
	//used to construct output filenames from project name 
	//char* filename necessitated by vqm_parse_file()

	// a list which stores all the user supplied custom hard block type names
	std::vector<std::string> hard_block_type_name_list;

	// indicates the type of device the circuit is targetting
	// we set the default value to the stratix 4 device
	string device = "stratixiv";

//*************************************************************************************************
//	Begin Conversion
//*************************************************************************************************

	cout << "********************\nVQM to BLIF Convertor\nS. Whitty 2011\n********************\n" ;
	cout << "This parser reads a .vqm file and converts it to .blif format.\n\n" ;
	
	//verify command-line is correct, populate input variables and global mode flags.
	cmd_line_parse(argc, argv, &source_file, &arch_file, &out_file, &device,&hard_block_type_name_list);

	setup_lut_support_map ();	//initialize LUT support for cleanup and elaborate functions

	size_t cptr = out_file.find_last_of(".");
	project_path = out_file.substr(0, cptr);	//truncate extension
	//project_path is derived from the out_file from command line, used to derive other output filenames.
			
	//Parse the .vqm and express it in memory: from vqm_dll.cpp
    cout << "\n>> Parsing VQM file " << source_file << endl ;

	unsigned long flowStart = clock();
	unsigned long processStart = clock();

	VTR_ASSERT(source_file.length() < MAX_LEN );
	strcpy ( temp_name, source_file.c_str() );

    //VQM Parser Call, requires char* filename
    my_module = vqm_parse_file(temp_name);

	unsigned long processEnd = clock();
	cout << "\n>> VQM Parsing took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;

    cout << "\n>> Verifying module";
	verify_module (my_module);

	if (debug_mode){
		//Print debug info to "<project_path>_module.echo"
		construct_filename ( temp_name, project_path.c_str(), "_module.echo" );
		cout << "\n>> Dumping to output file " << temp_name << endl ;
		echo_module ( temp_name, source_file.c_str(), my_module );
		//file contains data read from the .vqm structures.
	}

	//Parse the architecture file to get full information about the models used.
    //  Note: the architecture file needs to be loaded first, so that it can be used
    //        to derive the port directionality of black box primitives.  This is 
    //        required when decomposing INOUT pins into input and output pins
	cout << "\n>> Parsing architecture file " << arch_file << endl ;
    try {
        XmlReadArch( arch_file.c_str(), false, &arch, physical_tile_types, logical_block_types);	//Architecture (XML) Parser call
    } catch (const vtr::VtrError& e) {
        cout << "Error at line " << e.line() << " in " << e.filename() << ": " << e.what() << endl;
        exit(1);
    }

    //Really should update all code to use these... but this works for now
    types = logical_block_types.data();
    numTypes = logical_block_types.size();

	VTR_ASSERT((types) && (numTypes > 0));
	VTR_ASSERT(arch.models != NULL);
			
    //Pre-process the netlist
    //  Currently this just 'cleans up' bi-directional inout pins
    cout << "\n>> Preprocessing Netlist...\n";
    processStart = clock();

    preprocess_netlist(my_module, &arch, types, numTypes, fix_global_nets, elaborate_ram_clocks,
                       single_clock_primitives, split_carry_chain_logic, remove_const_nets);
	
    if (debug_mode){
		//Print debug info to "<project_path>_module.echo"
		construct_filename ( temp_name, project_path.c_str(), "_module_preprocess.echo" );
		cout << "\n>> Dumping to output file " << temp_name << endl ;
		echo_module ( temp_name, source_file.c_str(), my_module );
		//file contains data read from the .vqm structures.
	}

    processEnd = clock();
	cout << "\n>> Preprocessing Netlist took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;
    
    cout << "\n>> LUT Stats...\n";
    processStart = clock();

    //Print LUT Stats
    print_lut_stats(my_module);

    processEnd = clock();
    cout << "\n>> LUT Stats took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;

    //Clean the netlist
	if (clean_mode != CL_NONE){
		cout << "\n>> Cleaning up netlist...\n" ;
		processStart = clock();

		netlist_cleanup (my_module);

		processEnd = clock();
		cout << "\n>> Netlist cleaning took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;

		if (debug_mode){
			construct_filename ( temp_name, project_path.c_str(), "_module_postclean.echo" );
			cout << "\n>> Dumping to output file " << temp_name << endl ;
			echo_module ( temp_name, source_file.c_str(), my_module );
			//file contains cleaned data read from the .vqm structures.
		}
	}

	// only process the netlist for any custom hard blocks if the user provided valid hard block names
	if (insert_custom_hard_blocks)
	{	
		cout << "\n>> Identifying and instantiating custom hard blocks within the netlist.\n";
		
		processStart = clock();
		
		try
		{
			add_hard_blocks_to_netlist(my_module,&arch,&hard_block_type_name_list, arch_file, source_file, device);
		}
		catch(const vtr::VtrError& error)
		{
			VTR_LOG_ERROR("%s\n", ((std::string)error.what()).c_str());
			exit(1);
		}

		processEnd = clock();

		cout << "\n>> Custom hard block instantiations took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;
	}
	
	//Reorganize netlist data into structures conducive to .blif writing.
	if (verbose_mode){
		cout << "\n>> Initializing BLIF model\n" ;
	} else {
		cout << "\n>> Converting to BLIF format\n" ;
	}

	processStart = clock();

	init_blif_models( &my_model, my_module, &arch );
			
	if(debug_mode){			
		//Print debug info to "<project_path>_blif.echo"
		construct_filename ( temp_name, project_path.c_str(), "_blif.echo" );
		cout << "\n>> Dumping to output file " << temp_name << endl ;
		echo_blif_model ( temp_name, source_file.c_str(), &my_model, arch.models );
		//file contains data read from BLIF structures (verbose).
	}
		
	//Print the blif netlist to "<out_file>"
#ifdef NO_CONSTS
	cout << "\n>> Dumping to output file "<< out_file << " (pre-hack)\n";
#else
	cout << "\n>> Dumping to output file "<< out_file << endl;
#endif

	strcpy ( temp_name, out_file.c_str() );
	dump_blif ( temp_name, &my_model, &arch, print_unused_subckt_pins );

	processEnd = clock();
	cout << "\n>> BLIF Conversion took " << (float)(processEnd - processStart)/CLOCKS_PER_SEC << " seconds.\n" ;
	
#ifdef NO_CONSTS
	if(verbose_mode){
		cout << "\n>> Implementing constants hack\n";
	}
	//replaces "gnd" and "vcc" assignments with "unconn" in the .blif
	out_file = file_replace ( out_file, "gnd", "unconn" );
	out_file = file_replace ( out_file, "vcc", "unconn" );
#endif

		
	//Free all allocated memory.
	cout << "\n>> Cleaning up\n";
	all_data_cleanup();
	
	unsigned long flowEnd = clock();
	cout << "\n>> VQM->BLIF Total Runtime: " << (float)(flowEnd - flowStart)/CLOCKS_PER_SEC << " seconds.\n" ;

	cout << "\nComplete.\n";
	return 0;
}

//============================================================================================
//			SETUP FUNCTIONS
//============================================================================================

void cmd_line_parse (int argc, char** argv, string* sourcefile, string* archfile, string* outfile,
			   string* device, std::vector<std::string>* hard_block_type_name_list){
/*  Interpret the command-line arguments, accepting the input files, output file, and various
 *  mode settings from the user. 
 *
 *	ARGUMENTS
 *  argc, argv:
 *	Command-line argument data, just as in main().
 *  sourcefile:
 *	String to contain the name of the VQM File to be converted.
 *  archfile:
 *	String to contain the name of the Architecture (XML) File with the relevant subckt models.
 *  outfile:
 *	String to contain the name of the BLIF File to be output.
 */

	tokmap CmdTokens;
	tokmap::iterator it;
	size_t cptr;

	if (argc < 5)
	{
		cout << "ERROR: Not enough arguments."; 
		print_usage(T_TRUE);
	}

	setup_tokens (&CmdTokens);

	//Initialize non-mandatory variables to default values
    // Be sure to update print_usage() if this is changed
	debug_mode = T_FALSE;
	verbose_mode = T_FALSE;
	elab_mode = MODES_TIMING;
	lut_mode = VQM;
	clean_mode = CL_ALL;
	buffd_outs = T_FALSE;
    fix_global_nets = T_FALSE;
    elaborate_ram_clocks = T_TRUE;
    single_clock_primitives = T_FALSE;
    split_carry_chain_logic = T_FALSE;
    remove_const_nets = T_FALSE;
    print_unused_subckt_pins = T_FALSE;
    eblif_format = T_FALSE;
	insert_custom_hard_blocks = T_FALSE;

	// temporary storage to hold hard block names from the argument list
	std::string curr_hard_block_name; 

	//Now read the command line to configure input variables.
	for (int i = 1; i < argc; i++){
		//Check if the argument is in the registered tokens map
		it = CmdTokens.find(argv[i]);
		if (it != CmdTokens.end()){
			//Token was found in the map.
			switch(it->second){
				case OT_VQM:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing sourcefile name.\n" ;
						print_usage(T_TRUE);
					}
					//Store the next argument as the source file
					*sourcefile = (string)argv[i+1]; 
					verify_format(sourcefile, "vqm"); //verifies proper filename, quits if wrong
					i++; //Increment past the next argument
					break;
				case OT_ARCH:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing archfile name.\n" ;
						print_usage(T_TRUE);
					}
					//Store the next argument as the architecture file
					*archfile = (string)argv[i+1]; 
					verify_format(archfile, "xml"); //verifies proper filename, quits if wrong
					i++; //Increment past the next argument
					break;
				case OT_OUT:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing outfile name.\n" ;
						exit(1);
					}
					//Store the next argument as the output blif file
					*outfile = (string)argv[i+1];
					i++; //Increment past the next argument
					break;
				case OT_DEBUG:
					debug_mode = T_TRUE;
					break;
				case OT_VERBOSE:
					verbose_mode = T_TRUE;
					break;
				case OT_BUFFOUTS:
					buffd_outs = T_TRUE;
					break;
				case OT_ELAB:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing elaboration setting.\n" ;
						print_usage(T_TRUE);
					}
					//Check next argument for valid setting
					if ((strcmp(argv[i+1], "none")) == 0){
						elab_mode = NONE;
					} else if ((strcmp(argv[i+1], "modes")) == 0){
						elab_mode = MODES;
					} else if ((strcmp(argv[i+1], "modes_timing")) == 0){
						elab_mode = MODES_TIMING;
					} else if ((strcmp(argv[i+1], "atoms")) == 0){
						elab_mode = ATOMS;	//NOTE: ATOMIZING ALGORITHM INCOMPLETE.
					} else {
						cout << "\nERROR: Invalid elaboration setting " << argv[i+1] << endl ;
						print_usage(T_TRUE);
					}
					i++;	//Increment past the next argument
					break;
				case OT_LUTS:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing LUT setting.\n" ;
						print_usage(T_TRUE);
					}
					//Check next argument for valid setting
					if ((strcmp(argv[i+1], "vqm")) == 0){
						lut_mode = VQM;
					} else if ((strcmp(argv[i+1], "blif")) == 0){
						lut_mode = BLIF;
					} else {
						cout << "\nERROR: Invalid LUT setting " << argv[i+1] << endl ;
						print_usage(T_TRUE);
					}
					i++;	//Increment past the next argument
					break;
				case OT_CLEAN:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing Cleanup Setting.\n" ;
						print_usage (T_TRUE);
					}
					if ((strcmp(argv[i+1], "none")) == 0){
						clean_mode = CL_NONE;
					} else if ((strcmp(argv[i+1], "buffers")) == 0){
						clean_mode = CL_BUFF;
					} else if ((strcmp(argv[i+1], "all")) == 0){
						clean_mode = CL_ALL;
					} else {
						cout << "\nERROR: Invalid Cleanup Setting.\n" ;
						print_usage(T_TRUE);
					}
					i++;
					break;
                case OT_MULTICLOCK_PRIMITIVES:
                    single_clock_primitives = T_FALSE;
                    break;
                case OT_FIXGLOBALS:
                    fix_global_nets = T_TRUE;
                    break;
                case OT_ELABORATE_RAM_CLOCKS:
                    elaborate_ram_clocks = T_TRUE;
                    break;
                case OT_SPLIT_CARRY_CHAIN_LOGIC:
                    split_carry_chain_logic = T_TRUE;
                    break;
                case OT_REMOVE_CONST_NETS:
                    remove_const_nets = T_TRUE;
                    break;
                case OT_INCLUDE_UNUSED_SUBCKT_PINS:
                    print_unused_subckt_pins = T_TRUE;
                    break;
                case OT_EBLIF_FORMAT:
                    eblif_format = T_TRUE;
                    break;
				case OT_INSERT_CUSTOM_HARD_BLOCKS:
					// first check whether an accompanying hard block type name was supplied (user provides the names of all the various types of custom hard blocks within the design, essentially this is the module names in their HDL design)
					if ( i+1 == argc ){
						// no hard block type name was supplied so throw an error
						cout << "ERROR: Missing Hard Block Module Names.\n" ;
						print_usage (T_TRUE);
					}

					// now we loop through and process the following arguments
					while (T_TRUE)
					{
						// first check whether the next argument is a new option or a supplied hard block type name
						it = CmdTokens.find(argv[i+1]);

						// case where the next argument is a command line option
						if (it != CmdTokens.end())
						{
							// When we come here, we need to finish processing further command line arguments for hard block type names. Since we have a different command-line option.

							// case one below is when the user didnt provide any hard block type names and just provided the next command-line option
							if (!insert_custom_hard_blocks)
							{
								// since no hard block type names were supplied, we throw an error
								cout << "ERROR: Missing Hard Block Module Names.\n"; 
								print_usage (T_TRUE);
							}
							else
							{
								// the user already provided a legal hard block type name, so if we come here then the user simply just wanted to use another command line option, so we just leave this case statement.
								break;
							}
						}
						else
						{
							// when we are here, the user provided an argument which is potentially a valid hard block type name
							curr_hard_block_name.assign(argv[i+1]);

							// check if the provided name is valid
							verify_hard_block_type_name(curr_hard_block_name);

							// if the hard block type name was escaped by '\', then we need to remove the '\' character from the string (look at 'verify_hard_block_type_name' function for more info)
							cleanup_hard_block_type_name(&curr_hard_block_name);

							// if we are here then the provided name was valid.
							insert_custom_hard_blocks = T_TRUE;
							
							// check to see whether the user repeated a hard block type name
							if ((std::find(hard_block_type_name_list->begin(), hard_block_type_name_list->end(), curr_hard_block_name)) == hard_block_type_name_list->end())
							{
								hard_block_type_name_list->push_back(curr_hard_block_name); // add the hard block type name to the list
							}
							else
							{
								cout << "ERROR: Hard Block Module name '" << curr_hard_block_name << "' was repeated.\n";
								print_usage (T_TRUE);
							}

							// update argument index to show that we have processed the current hard block type name successfully
							i++;
						}

						// we handle the case where the next argument is empty (if we continued then an exception will be thrown in the find function above)
						if (i+1 == argc)
						{
							// at this point we would have read a vlid input.
							// Safe to assume that the user just finished providing hard block names
							break;
						}
					}
					break;
				case OT_DEVICE:
					if ( i+1 == argc ){
						cout << "\nERROR: Missing device type.\n" ;
						print_usage(T_TRUE);
					}
					//Store the next argument as the device type
					device->assign((string)argv[i+1]);
					verify_device(*device); // make sure we can support the provided device
					i++; //Increment past the next argument
					break;
				default:
					//Should never get here; unknown tokens aren't mapped.
					cout << "\nERROR: Token " << argv[i] << " mishandled.\n" ;
					exit(1);
			}
		} else {
			cout << "\nERROR: Unknown Argument "<< argv[i] << " [" << i << "]\n" ;
			print_usage(T_TRUE);
		}
	}

	if ((sourcefile->empty()) || (archfile->empty())){
		cout << "ERROR: Missing Mandatory File.\n" ; 
		print_usage (T_TRUE);
	}

	//Default to outputting a file in the home directory of the same root name as the sourcefile.
	if (outfile->empty()){
		cptr = sourcefile->find_last_of("\\/");	//finds last \ or / in the filename
		*outfile = sourcefile->substr(cptr+1);	//copy into outfile, without the filepath 
		cptr = outfile->find_last_of(".");
		outfile->replace(cptr, 4, ".blif");	//replace ".vqm" with ".blif"
	}	
}

//============================================================================================
//============================================================================================

void setup_tokens (tokmap* tokens){
/*  Defines the set of accepted command-line tokens, mapping them to an enum for switch-case
 *  comparison. 
 *
 *	ARGUMENTS
 *  tokens:
 *	Map to be populated with keys (token strings) that map to enumerated values.
 */
	tokens->insert(tokpair("-vqm", OT_VQM));
	tokens->insert(tokpair("-arch", OT_ARCH));
	tokens->insert(tokpair("-out", OT_OUT));
	tokens->insert(tokpair("-debug", OT_DEBUG));
	tokens->insert(tokpair("-verbose", OT_VERBOSE));
	tokens->insert(tokpair("-elab", OT_ELAB));
	tokens->insert(tokpair("-luts", OT_LUTS));
	tokens->insert(tokpair("-clean", OT_CLEAN));
	tokens->insert(tokpair("-buffouts", OT_BUFFOUTS));
	tokens->insert(tokpair("-multiclock_primitives", OT_MULTICLOCK_PRIMITIVES));
	tokens->insert(tokpair("-fixglobals", OT_FIXGLOBALS));
	tokens->insert(tokpair("-elaborate_ram_clocks", OT_ELABORATE_RAM_CLOCKS));
	tokens->insert(tokpair("-split_carry_chain_logic", OT_SPLIT_CARRY_CHAIN_LOGIC));
	tokens->insert(tokpair("-remove_const_nets", OT_REMOVE_CONST_NETS));
	tokens->insert(tokpair("-include_unused_subckt_pins", OT_INCLUDE_UNUSED_SUBCKT_PINS));
	tokens->insert(tokpair("-eblif_format", OT_EBLIF_FORMAT));
	tokens->insert(tokpair("-insert_custom_hard_blocks", OT_INSERT_CUSTOM_HARD_BLOCKS));
	tokens->insert(tokpair("-device", OT_DEVICE));
}

//============================================================================================
//			EXECUTION FUNCTIONS
//============================================================================================

void init_blif_models(t_blif_model* my_model, t_module* my_module, t_arch* arch){
/*  Populates a BLIF model structure with appropriate parser data.
 *
 *	ARGUMENTS
 *  my_model:
 *	Model structure that is to be populated with BLIF data.
 *  my_module: 
 *	Module structure already populated with VQM data.
 *  arch:
 *	Arch structure already populated with Architecture data.
 */
	
	//Initialize general variables
	if(verbose_mode){
		cout << "\t>> Assigning general variables\n" ;
	}

	my_model->name = (string)my_module->name;
	VTR_ASSERT(!my_model->name.empty());
	
	//Initialize input and output pinvecs of the blif_model
	//NOTE: All circuits must have inputs and outputs.
	VTR_ASSERT( my_module->number_of_pins >  0 );
	if(verbose_mode){
		cout << "\t>> Assigning .inputs and .outputs\n" ;
	}

	t_pin_def* temp_pin;
	for (int i = 0; i < my_module->number_of_pins; i++){
		//Traverse all pins of the module
		temp_pin = my_module->array_of_pins[i];
		switch (temp_pin->type){
			//Sort the pins into their appropriate vectors
			case PIN_INPUT:
				my_model->input_ports.push_back(temp_pin);
				break;
			case PIN_OUTPUT:
				my_model->output_ports.push_back(temp_pin);
				break;
			case PIN_INOUT:
                //INOUT pins are not supported by BLIF format
				cout << "\n\nERROR: PIN_INOUT detected. Pin: " << temp_pin->name << endl ;
				exit(1);
				break;
			default:
				//PIN_WIREs don't need to be declared independantly for proper
				//BLIF format, they'll appear again in the array_of_assignments.
				;	
		}
	}
	
	//Verify that both inputs and outputs exist.
	VTR_ASSERT((my_model->input_ports.size() > 0) && (my_model->output_ports.size() > 0));

	//Initialize the blif_model's assignment array
	//Possible to have a circuit without any assign statements.
	if (my_module->number_of_assignments > 0){
		if(verbose_mode){
			cout << "\t>> Assigning buffer logic\n" ;
		}
		
		my_model->num_assignments = my_module->number_of_assignments;
		my_model->array_of_assignments = my_module->array_of_assignments;
		VTR_ASSERT((my_model->num_assignments > 0) && (my_model->array_of_assignments != NULL));
		//data from my_module is already organized nicely, so piggyback onto its allocation
	}
	
	//Initialize Subcircuits
	//Possible to have a circuit without any nodes.
	if (my_module->number_of_nodes > 0){
		if(verbose_mode){
			cout << "\t>> Assigning subcircuits\n" ;
		}

		init_blif_subckts (	my_module->array_of_nodes, 
						my_module->number_of_nodes,
						arch->models,
						my_model	);
	}
	
	if(verbose_mode){
		cout << ">> BLIF model initialization complete.\n" ;
	}
}

//============================================================================================
//============================================================================================
void 	init_blif_subckts (	t_node **vqm_nodes, 
					int number_of_vqm_nodes,
					t_model *arch_models,
					t_blif_model* my_model	){
/*  Initializes the subcircuits vector within a t_blif_model based on a t_node array.
 *
 *	ARGUMENTS
 *  vqm_nodes:
 *	Array of nodes containing all subcircuit data parsed from the VQM.
 *  number_of_vqm_nodes:
 *	Size of the vqm_nodes array.
 *  arch_models:
 *	Linked list of models found in the Architecture file that each subcircuit corresponds to.
 *  my_model:
 *	t_blif_model to be populated with subcircuit information.
 */
					
	t_node* temp_node;		//from vqm_dll.h

	//Preparations before the subcircuits can be assigned.
	VTR_ASSERT(arch_models != NULL);
	subckt_prep(arch_models);

	VTR_ASSERT((vqm_nodes != NULL)&&(vqm_nodes[0] != NULL)&&(number_of_vqm_nodes >= 0));

	for (int i = 0; i < number_of_vqm_nodes; i++){
		//Each "node" from the VQM is a subcircuit instantiation
		temp_node = vqm_nodes[i];
		VTR_ASSERT(temp_node != NULL);

		if(verbose_mode){
			VTR_ASSERT(temp_node->name != NULL);
			cout << "\n\t\tProcessing VQM Node " << temp_node->name  << endl;
		}

		//Based on the specified LUT and elaboration mode, translate the current node
		//into a blif subckt or LUT(or group of subckts)
		if ((lut_mode == BLIF)&&(is_lut(temp_node))){
			push_lut ( temp_node, &(my_model->luts) );
		} else if ((elab_mode == NONE)||(elab_mode == MODES)||(elab_mode == MODES_TIMING)){
			push_node_1_to_1 (temp_node, arch_models, &(my_model->subckts));
		} else if (elab_mode == ATOMS){
			push_node_atomize (temp_node, arch_models, &(my_model->subckts));
		} else {
			//Should never get here; a bad elab_mode should be rejected at parse-time
			cout << "\nERROR: Corrupted Elaboration Mode.\n" ;
			exit(1);
		}
	}
}

//============================================================================================
//============================================================================================

void subckt_prep(t_model* cur_model){
/*  Accomplishes preparatory tasks before the subcircuits of a blif_model can be initialized.
 *
 *	ARGUMENTS
 *  cur_model:
 *	Linked list containing all model information from the Architecture file.
 */

//TASK 1: Find the maximum index among the models in the Architecture.
	int max_model_index;
	max_model_index = cur_model->index;
	while(cur_model){
		//Cycle through the list, save the highest index.
		if (cur_model->index > max_model_index){
			max_model_index = cur_model->index;
		}
		cur_model = cur_model->next;
	}

//TASK 2: Allocate and initialize the model_count array (global).
	model_count = (int*)malloc((max_model_index + 1)*sizeof(int));
	for (int i = 0; i <= max_model_index; i++){
		model_count[i] = 0;
	}
	
//TASK 3: Initialize the unconn and open_port structures.
	unconn.name = strdup("unconn");
	unconn.indexed = T_FALSE;
	unconn.type = PIN_WIRE;
	unconn.left = unconn.right = 0;
	
	open_port.port_name = NULL;
	open_port.associated_net = &unconn;
	open_port.port_index = 0;
	open_port.wire_index = 0;
	//These structures are mapped to by ports of a block
	//that are not assigned to an external pin.
}

//============================================================================================
//============================================================================================
void push_node_1_to_1 (t_node* vqm_node, t_model* arch_models, scktvec* blif_subckts){
/*  Interprets each VQM block as a single instance of a BLIF subcircuit. Depending on 
 *  the global elab_mode, the Architecture will be searched either for the VQM block name as-is
 *  or a name appended with a special mode-hash, generated from parameters of the VQM block.
 *
 *	ARGUMENTS	
 *  vqm_node:
 *	The particular node in the VQM file to be translated.
 *  arch_models:
 *	Linked list of all BLIF models made available in the Architecture.
 *  blif_subckts:
 *	Vector as part of a t_blif_model structure describing the subcircuits in the model.
 */

	//temporary process variables
	t_blif_subckt temp_subckt;	//from vqm2blif.h
	t_model* cur_model;		//from physical_types.h

	boolvec vqm_ports_found;	//verification flags indicating that all ports 
						//found in the VQM were mapped.

	string search;		//temporary variable to construct a desired block name within.

	cur_model = arch_models; //search for the corresponding model in the list from the architecture.
	VTR_ASSERT(cur_model != NULL);

	temp_subckt.inst_name = (string)vqm_node->name;	//copy the instance name
	VTR_ASSERT( !temp_subckt.inst_name.empty() );
	
	if (elab_mode == NONE){
		//search for an Architecture model solely based on block name
		VTR_ASSERT(vqm_node->type != NULL);
		search = vqm_node->type;	

	} else if (elab_mode == MODES || elab_mode == MODES_TIMING){
		//search for an Architecture model based on the block name and the parameters
		search = generate_opname(vqm_node, arch_models);  //generate the simple mode-hashed name based on parameters.

	} else {
		//should never get here, based on condition in init_blif_subckts()
		cout << "\n\nERROR: Corrupted Elaboration Mode.\n" ;
		exit(1);
	}

	vqm_ports_found.clear();
	for (int i = 0; i < vqm_node->number_of_ports; i++){
		//initialize all ports_found flags to zero.
		vqm_ports_found.push_back(T_FALSE);
	}


	//traverse through the models declared in the Architecture file
	//to find the one corresponding directly to the node found in the vqm
	if (verbose_mode){
		cout << "\n\t\t>> Blackbox Identified." ;
		cout << "\n\t\t>> Searching Architecture for Model Data \""<< search << "\"" ;
	}
	while ((cur_model)&&(strcmp(cur_model->name, search.c_str()) != 0)){
		cur_model = cur_model->next;
	}

	//cur_model now points to either NULL or the appropriate Architecture model.
	if (!cur_model){	
		//cur_model == NULL if the end of the model list was reached without success.
		cout << "\n\nERROR: Model name " << search << " was not found in the architecture.\n" ;
		exit(1);
	} else {
		if (verbose_mode){
			cout << "\n\t\t>> Model " << search << " identified.\n" ;
		}
		temp_subckt.model_type = cur_model;	//initialize model_type
		model_count[cur_model->index]++;	//increment the instance count of the model
		temp_subckt.input_cnxns.clear();	//reset the input and output maps
		temp_subckt.output_cnxns.clear();
		
		//map the input ports of the model read from the architecture
		//to the corresponding external pin as per the vqm
		init_subckt_map(	&(temp_subckt.input_cnxns), 
					temp_subckt.model_type->inputs, 
					vqm_node, &vqm_ports_found);	
		//vqm_ports_found entries are set as ports are mapped

		VTR_ASSERT(!temp_subckt.input_cnxns.empty());	//all blocks must have an input

		//now map the output ports
		init_subckt_map(	&(temp_subckt.output_cnxns), 
					temp_subckt.model_type->outputs, 
					vqm_node, &vqm_ports_found);
		//vqm_ports_found entries are set as ports are mapped

		VTR_ASSERT(!temp_subckt.output_cnxns.empty());	//all blocks must have an output

        //Pass through parameters
        for (int iparam = 0; iparam < vqm_node->number_of_params; ++iparam) {
            t_subckt_param_attr param;
            t_node_parameter* vqm_param = vqm_node->array_of_params[iparam];
            param.name = vqm_param->name;

            if (vqm_param->type == NODE_PARAMETER_STRING) {
                param.value = vqm_param->value.string_value;
            } else {
                VTR_ASSERT(vqm_param->type == NODE_PARAMETER_INTEGER);
                param.value = std::to_string(vqm_param->value.integer_value);
            }
            temp_subckt.params.push_back(param);
        }

		//Verify that all ports specified in the VQM node were successfully mapped before 
		//committing the subcircuit to the BLIF structure.			
		if (verbose_mode){
			cout << "\t\tVQM Port Verification:\n" ;
		}
		for (int i = 0; i < vqm_node->number_of_ports; i++){
			/* The mapping process maps all ports of the architecture either to the open port or
			 * a port in the VQM. If a port appeared in the VQM and not in the 
			 * architecture, the association's corresponding entry in vqm_ports_found would 
			 * remain T_FALSE through the mapping process.
			 */
			if (verbose_mode){
				//Print whether the port was mapped explicitly
				//Prints "Port (port)[index] = [ mapped | unmapped ]"
				cout << "\t\t\tPort " << vqm_node->array_of_ports[i]->port_name ;

				if (vqm_node->array_of_ports[i]->port_index >= 0){
					cout << "[" << vqm_node->array_of_ports[i]->port_index << "]" ;
				}

				cout <<"= " << ((vqm_ports_found[i])? "mapped":"unmapped") << endl;
			}

			if (vqm_ports_found[i] == T_FALSE){
				cout << "\n\nERROR: Port " << vqm_node->array_of_ports[i]->port_name ;
				if (vqm_node->array_of_ports[i]->port_index >= 0){
					cout << "[" << vqm_node->array_of_ports[i]->port_index << "]" ;
				}
				cout << " not found in architecture for " << search << endl ; 
				exit(1);
			}
		}
		if (verbose_mode){
			cout << endl ;
		}
		//push the temp_subckt into the subckt vector
		blif_subckts->push_back(temp_subckt);
	}
}

//============================================================================================
//============================================================================================

void push_node_atomize (t_node* /*vqm_node*/, t_model* /*arch_models*/, scktvec* /*blif_subckts*/ /*, FILE* dict*/){
/*  Interprets each VQM block and its parameter set, then expands that block into its smaller 
 *  atomic constituents based on a "Dictionary" document.
 *
 *	ARGUMENTS
 *  vqm_node:
 *	The particular node in the VQM file to be atomized.
 *  arch_models:
 *	Linked list of all BLIF models made available in the Architecture.
 *  blif_subckts:
 *	Vector as part of a t_blif_model structure describing the subcircuits in the model.
 */
	cout << "\nERROR: Atom Translation Unsupported.\n" ;
	exit(1);
}

//============================================================================================
//============================================================================================

void init_subckt_map(portmap* map, t_model_ports* to_be_mapped, t_node* node, boolvec* vqm_ports_found){
/*  Initializes the portmap based on a linked list of ports and node information
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
 *  map:
 *	portmap to be populated
 *  to_be_mapped:
 *	Linked list of ports to be mapped
 *  node:
 *	Structure containing all information about associated ports of the subcircuit.
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
				find_and_map (map, to_be_mapped, node, T_TRUE, i, vqm_ports_found);
			}
		} else {
			//otherwise map the "name" to the association 
			//without an index.
			find_and_map (map, to_be_mapped, node, T_FALSE, 0, vqm_ports_found);
		}
		//move to the next port in the model
		to_be_mapped = to_be_mapped->next;
	}
}

//============================================================================================
//============================================================================================
void find_and_map (portmap* map, t_model_ports* to_be_mapped, t_node* node, 
				t_boolean is_bus, int index, boolvec* vqm_ports_found){
/*  Maps a subcircuit port from the architecture to a port association from the vqm.
 *  A string is generated from the name and index of the port being mapped, and this string
 *  is mapped to a t_node_port_association structure.
 *
 *	ARGUMENTS
 *  map:
 *	Structure containing the association data.
 *  to_be_mapped:
 *	The subcircuit port given from the architecture file.
 *  node:
 *	VQM Node whose ports list will be searched for the architecure's port.
 *  is_bus:
 *	Flag indicating whether to index the map key or not.
 *  index:
 *	If the key is mapped, this is the index to use.
 *  vqm_ports_found:
 *	Vector of flags keeping track of which ports mentioned in the VQM have been mapped so far.
 */

	t_boolean found_in_ports;

	string temp_name;
	temp_name = (string)to_be_mapped->name;	//copy the name of the subcircuit port
	VTR_ASSERT(!temp_name.empty());

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
			if(verbose_mode){
				cout << "\t\t\t-> Mapping " << temp_name << " to " << node->array_of_ports[i]->associated_net->name << endl ;
			}
			//map the port to its connectivity data
			map->insert(portpair(temp_name, node->array_of_ports[i]));

			//indicate that the port has been successfuly mapped to
			vqm_ports_found->at(i) = T_TRUE;
			break;
		}
	}

	if (found_in_ports == T_FALSE){
		//if the port wasn't in node->array_of_ports, map it to the open_port
		if(verbose_mode){	
			cout << "\t\t\t-> Mapping " << temp_name << " to " << open_port.associated_net->name << endl;
		}
		
		map->insert(portpair(temp_name, &open_port));
	}
}

//============================================================================================
//============================================================================================
void push_lut (t_node* vqm_node, lutvec* blif_luts){
/* Takes a node that's been flagged as a LUT and elaborates it into the main model in a BLIF-
 * compatible way.
 *
 *	ARGUMENTS
 *  vqm_node:
 *	Node from the VQM File to be elaborated into a LUT.
 *  blif_luts:
 *	A vector from a BLIF model containing instances of BLIF-compatible LUTs.
 */

	char* lutmask = nullptr;
	signed int which_port;
	string portName;

	BlifLut temp_lut;	//Custom Class from lutmask.h

	for (int i = 0; i < vqm_node->number_of_params; i++ ){
		if (strcmp(vqm_node->array_of_params[i]->name, "lut_mask") == 0){
			VTR_ASSERT(vqm_node->array_of_params[i]->type == NODE_PARAMETER_STRING);
			lutmask = vqm_node->array_of_params[i]->value.string_value;
			break;
		}
	}

	if (lutmask == NULL){
		cout <<  "ERROR: Cannot find LUTmask for node " << vqm_node->name << " of type " << vqm_node->type << ".\n";
	} else {
		if (verbose_mode){
			cout << "\n\t\t>> Elaborating LUT " << vqm_node->name << endl ;
			cout << "\t\t  Lutmask: " << lutmask << endl ;
		}
		temp_lut.inst_name = vqm_node->name;
		temp_lut.set_lutmask(lutmask);
		for (int i = 0; i < vqm_node->number_of_ports; i++){
			which_port = -1;

			if (strcmp(vqm_node->array_of_ports[i]->port_name, "dataa") == 0){
				which_port = 0;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "datab") == 0){
				which_port = 1;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "datac") == 0){
				which_port = 2;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "datad") == 0){
				which_port = 3;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "datae") == 0){
				which_port = 4;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "dataf") == 0){
				which_port = 5;
			} else if (strcmp(vqm_node->array_of_ports[i]->port_name, "combout") == 0){
				which_port = 6;
			}

			if (which_port >= 0){
				portName = get_wire_name(vqm_node->array_of_ports[i]->associated_net, vqm_node->array_of_ports[i]->wire_index);
				if (which_port < 6){
					if (verbose_mode) {
						cout << "\t\t  Assigning inport " << i << " to " << portName << endl;
					}
					temp_lut.set_inport ( which_port, portName.c_str() );
				} else {
					if (verbose_mode) {
						cout << "\t\t  Assigning outport to " << portName << endl;
					}
					temp_lut.set_outport( get_wire_name(vqm_node->array_of_ports[i]->associated_net, vqm_node->array_of_ports[i]->wire_index).c_str() );
				}
			}
		}
		
		blif_luts->push_back( temp_lut );
	}

}

//============================================================================================
//============================================================================================

void dump_blif (char* blif_file, t_blif_model* main_model, t_arch* arch, t_boolean print_unused_subckt_pins_local){
/*  Uses a populated model structure to construct a .blif netlist.
 *
 *	ARGUMENTS
 *  blif_file:
 *	Name of the target .blif file.
 *  main_model:
 *	Structure containing all BLIF-model data to be printed.
 *  arch:
 *	Structure containing data from the architecture file.
 */
	ofstream blif_out (blif_file);
	
	// Open file to dump data.
	if (!blif_out.is_open()){
		cout << "ERROR: Cannot open file " << blif_file << endl ;
		exit(1);
	}
	
	//comments in BLIF begin with '#'
	blif_out << "#BLIF OUTPUT: " << blif_file << endl ;
	blif_out << "\n#MAIN MODEL\n" ;
	
	//completely dump the top-level model
	dump_main_model(main_model, blif_out, print_unused_subckt_pins_local, eblif_format, T_FALSE);
	
	//now dump the subckt models from the architecture
	//that were used in the vqm
	if (main_model->subckts.size() > 0){
		blif_out << "\n#SUBCKT MODELS\n" ;
		dump_subckt_models(arch->models, blif_out, T_FALSE);
	}
	
	// Close file.
	blif_out.close();
}

//============================================================================================
//============================================================================================

void dump_main_model(t_blif_model* model, ofstream& outfile, t_boolean print_unused_subckt_pins_local, t_boolean eblif_format_local, t_boolean debug){
/*  Dumps information stored in a model structure in proper BLIF syntax.
 *
 *	ARGUMENTS
 *  model:
 *	Pointer to model structure to be dumped.
 *  outfile:
 *	Filestream to output to.
 *  debug:
 *	Flag to indicate whether to print in BLIF or DEBUG format.
 *
 *  BLIF syntax described in blif_info.pdf
 */
 
	outfile << ((debug)? "\nModel Name: ":"\n.model ") <<  model->name << endl;

	//series of conditional statements define which sections of a blif netlist
	//to print based on the model data.
	 
	//Dump inputs: ".inputs <name> <name> ..."
	if (model->input_ports.size() > 0){
		outfile << ((debug)? "Inputs:\n":".inputs \\\n");
		dump_portlist (outfile, model->input_ports, debug);
	} 
	
	//Dump outputs: ".outputs <name> <name> ..."
	if (model->output_ports.size() > 0){
		outfile << ((debug)? "Outputs:\n":".outputs \\\n");
		dump_portlist (outfile, model->output_ports, debug);		
	} 
	
	//Dump clocks: ".clock <name> <name> ..."
	if (model->clock_ports.size() > 0){
		outfile << ((debug)? "Clocks:\n":".clock ");
		dump_portlist (outfile, model->clock_ports, debug);	
	}

    outfile << "\n";
	
	//ABC needs a dummy net called "unconn"; VPR ignores this.
    if (print_unused_subckt_pins_local) {
        outfile << "\n.names unconn\n\n";
    }
	
	//Print Assignment Variable information
	if (model->num_assignments > 0){
		dump_assignments(outfile, model, eblif_format_local, debug);
	}

	//Print LUT information
	if (model->luts.size() > 0){
		dump_luts (outfile, &(model->luts), eblif_format_local, debug);
	}
	
	//Print Subcircuit Variable information
	if (model->subckts.size() > 0){
		dump_subckts(outfile, &(model->subckts), print_unused_subckt_pins_local, eblif_format_local, debug);
	}
	
	//Printing the model data is complete.
	outfile << ((debug)?"\n":"\n.end\n");
}

//============================================================================================
//============================================================================================

void dump_portlist (ofstream& outfile, pinvec ports, t_boolean /*debug*/){
/*  Prints all the port names of a model, flattening any bussed ports into indexed notation.
 *  Debug format has each port on its own line, BLIF lists them on a single line.
 *
 *  NOTE: In VQM/Verilog syntax, buses are declared as
 *			{input, output, clock, wire} [ left : right ] <name>
 *  where left and right are indeces that describe the
 *  bus width.
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to print the port data to.
 *  ports:
 *	Vector containing all related ports (either inputs, outputs, or clocks.)
 *  debug:
 *	Flag indicating whether to print in DEBUG or BLIF format.
 */
	int limit = ports.size();	//don't evaluate .size() each iteration of the loop.
	t_pin_def* temp_pin;
	string temp_name;
    string indent = "    ";

	for (int i = 0; i < limit; i++){
		//cycle through all ports
		temp_pin = ports[i];
		
        int j_min = min(temp_pin->left, temp_pin->right);
        int j_max = max(temp_pin->left, temp_pin->right); 
		for (int j = j_min; j <= j_max; j++){
            //if the pin is non-indexed, iterates once and doesn't append an index.
            //otherwise, flattens a bus into component wires using index convention.

            temp_name = (string)temp_pin->name;

            if (temp_pin->indexed){	
                temp_name = append_index_to_str(temp_name, j);
            }

            outfile << indent << temp_name;

            bool last = (i == limit - 1 && j == j_max);

            if(!last) {
                outfile << " \\";
            }
            outfile << "\n";

		}
	}	
}

//============================================================================================
//============================================================================================

void dump_assignments(ofstream& outfile, t_blif_model* model, t_boolean eblif_format_local, t_boolean debug){
/*  Prints all assignment information from model's array_of_assignments
 *  in BLIF or DEBUG syntax.
 *
 *  DEBUG syntax:
 *	assign a = b	OR	assign a = !b	OR	assign a = 1'b0
 *
 *  BLIF syntax:
 *	.names a b		OR	.names a b		OR	.names a
 *	1 1				0 1				0	
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to output the assignment data to.
 *  model:
 *	Model containing the array_of_assignments and number_of_assignments to print.
 *  debug:
 *	Flag indicating whether to print in DEBUG or BLIF syntax.
 */
 
	t_assign* temp_assign;
	string target_name, source_name;

	for (int i = 0; i < model->num_assignments; i++){
		temp_assign = model->array_of_assignments[i];

		if (temp_assign == NULL){
			continue;
		}

		if (temp_assign->source == NULL){
			//Constant Assignment

#ifdef NO_CONSTS
			//No Constant assignments in the BLIF (including gnd and vcc).
			continue;
#endif

#ifdef VQM_BUSES
			if ((temp_assign->target->indexed)&&(temp_assign->target_index == -1)) {
				//VQM Parser returns an index of -1 on whole-bus assignments
				dump_bus_assign(outfile, 
							(string)temp_assign->target->name, 
							temp_assign->target->left,
							temp_assign->target->right,
							(temp_assign->target->left > temp_assign->target->right)? -1:1,
							T_TRUE, 
							NULL,
							temp_assign->target->left,
							temp_assign->target->right,
							(temp_assign->target->left > temp_assign->target->right)? -1:1,
							temp_assign->value,
							debug, temp_assign->inversion);
				//dump_bus_assign() calls dump_wire_assign() repeatedly based on the indeces
				//of the target and whether the right or left index is larger. 
			} else {
#else
				VTR_ASSERT((!(temp_assign->target->indexed))||(temp_assign->target_index != -1));
#endif
				//a specific wire is being assigned, so append the index to the bus name.
				target_name = get_wire_name(temp_assign->target, temp_assign->target_index);

				dump_wire_assign (outfile, target_name, T_TRUE, target_name, temp_assign->value, 
							eblif_format_local, debug, temp_assign->inversion);
#ifdef VQM_BUSES
			}
#endif

		} else {
			//Wire/Pin Assignment
#ifdef VQM_BUSES
			if ((temp_assign->target_index == -1) && (temp_assign->target->indexed == T_TRUE)
				&& (temp_assign->source_index == -1) && (temp_assign->source->indexed == T_TRUE)){
				//Bus-to-bus assignment

				//Buses must be of the same width
				VTR_ASSERT(abs(temp_assign->target->left - temp_assign->target->right) 
						== abs(temp_assign->source->left - temp_assign->source->right));
				
				dump_bus_assign(outfile, 
							(string)temp_assign->target->name, 
							temp_assign->target->left,
							temp_assign->target->right,
							(temp_assign->target->left > temp_assign->target->right)? -1:1,
							T_FALSE, 
							(string)temp_assign->source->name,
							temp_assign->source->left,
							temp_assign->source->right,
							(temp_assign->source->left > temp_assign->target->right)? -1:1,
							0,
							debug, temp_assign->inversion);
			} else {
#else
			VTR_ASSERT((temp_assign->target_index != -1) || (temp_assign->target->indexed != T_TRUE)
				|| (temp_assign->source_index != -1) || (temp_assign->source->indexed != T_TRUE));
#endif
				target_name = get_wire_name(temp_assign->target, temp_assign->target_index);
				source_name = get_wire_name(temp_assign->source, temp_assign->source_index);

				dump_wire_assign (outfile, 
							target_name,
							T_FALSE,
							source_name,
							0,
							eblif_format_local, debug, temp_assign->inversion);
#ifdef VQM_BUSES
			}
#endif
		}
	}
}

//============================================================================================
//============================================================================================
#ifdef VQM_BUSES

void dump_bus_assign(ofstream& outfile, string target_name, int target_left, int target_right, int target_dir,
					t_boolean is_constant, 
						string source_name, int source_left, int source_right, int source_dir,
						int value, 
					t_boolean debug, t_boolean inversion){
/*  Flattens bus assignments into wire-by-wire assigns
 *  A bus assignment only occurs when the following is in the VQM:
 *   "	wire [X:Y] a;
 *  		wire [X:Y] b;
 *		assign a = b; " <-- NOTE: no indeces in the assign statement.
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to output the assignment data to.
 *  target_name:
 *	Name of the wire being driven by the assign (on the left of the assign). 
 *  target_left, target_right:
 *	Left and right indeces of the target bus, as declared (Left = X, Right = Y)
 *  target_dir:
 *	Directionality of the indeces. +1 if Y > X, -1 if Y < X.
 *  is_constant:
 *	Flag indicating whether the assignment is being driven by a constant generator.
 *  source_name:
 *	Name of the driver wire (on the right of the assign).
 *  source_left, source_right:
 *	Left and right indeces of the driver bus, as declared (Left = X, Right = Y)
 *  source_dir:
 *	Directionality of the indeces. +1 if Y > X, -1 if Y < X.
 *  value:
 *	If the assign is constant, holds the value of the generator.
 *  debug:
 *	Flag commanding the function to run in debug mode or not.
 *  inversion:
 *	Flag indicating whether the assignment is straight or inverted.
 */
	int i, j;
	for (i = target_left, j = source_left; 
			(i != target_right + target_dir) && (j != source_right + source_dir); 
			i += target_dir, j += source_dir){

		dump_wire_assign(outfile,
				append_index_to_str(target_name, i),
				is_constant,
				append_index_to_str(source_name, j),
				value,
				debug,
				inversion);

	}
}

#endif
//============================================================================================
//============================================================================================

void dump_wire_assign(ofstream& outfile, string target_name, 
				t_boolean is_constant, string source_name, int value, 
				t_boolean eblif_format_local, t_boolean /*debug*/, t_boolean inversion){
/*  Outputs a 1-bit wide assignment to a file in either DEBUG or BLIF format.
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to output the assignment data to.
 *  target_name:
 *	Name of the wire being driven by the assign (on the left of the assign). 
 *  is_constant:
 *	Flag indicating whether the assignment is being driven by a constant generator.
 *  source_name:
 *	Name of the driver wire (on the right of the assign).
 *  value:
 *	If the assign is constant, holds the value of the generator.
 *  debug:
 *	Flag commanding the function to run in debug mode or not.
 *  inversion:
 *	Flag indicating whether the assignment is straight or inverted.
 */
	if (is_constant){
        outfile << ".names " << target_name << endl;
        outfile << value << endl ;
	} else {
        if (inversion) {
            outfile << ".names " << source_name << " " << target_name << endl;
            outfile << "0 1\n" ;
        } else {
            if (eblif_format_local) {
                outfile << ".conn " << source_name << " " << target_name << endl;
            } else {
                outfile << ".names " << source_name << " " << target_name << endl;
                outfile << "1 1\n" ;
            }
        }
	}
    outfile << "\n";
}

//============================================================================================
//============================================================================================

void dump_luts (ofstream& outfile, lutvec* blif_luts, t_boolean eblif_format_local, t_boolean debug){
/*  Prints the data for all BLIF-format LUTs (.names) in the model.
 *  Calls the print member of a BlifLut class. See "lutmask.h" for algorithm.
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to print the LUT data to.
 *  blif_luts:
 *	Vector containing all instances of BLIF-compatible LUTs.
 *  debug:
 *	Flag indicating whether to print in DEBUG or BLIF mode.
 */ 
	lutvec::iterator lut;
	lp_mode printMode;

	//lp_mode is the internal flag for printing format of the BlifLut class.
	if (debug){
		printMode = VERBOSE_PRINT;
	} else {
		printMode = BLIF_PRINT;
	}

	cout << "\t>> Introduced " << blif_luts->size() << " BLIF-LUTs (.names)\n" ; 
	for (lut = blif_luts->begin(); lut != blif_luts->end(); lut++){
		//traverse entire luts vector, printing them all.
		lut->print( printMode, outfile, eblif_format_local );
	}
}

//============================================================================================
//============================================================================================

void dump_subckts(ofstream& outfile, scktvec* subckts, t_boolean print_unused_pins, t_boolean eblif_format_local, t_boolean debug){
/*  Traverse the subcircuit vector, printing the names and connections
 *  of each instantiated subcircuit in the main model.
 *
 *	ARGUMENTS
 *  outfile:
 *	Filestream to print the subcircuit instantiations to.
 *  subckts:
 *	Vector containing information about all subcircuit instantiations.
 *  debug:
 *	Flag indicating whether to print in DEBUG or BLIF mode.
 */

	t_blif_subckt* temp_subckt;
	
	int limit;
	limit = subckts->size(); //doesn't run size() on each iter of the loop
	for(int i = 0; i < limit; i++){
		//print each subcircuit's name, port, and connectivity information
		temp_subckt = &(subckts->at(i));
		if (debug){
			outfile << "\nSubcircuit Number: " << i << endl;
			outfile << "Instance Name: " << temp_subckt->inst_name << endl;
			outfile << "Type: " << temp_subckt->model_type->name << endl ;
		} else {
            if (!eblif_format_local) {
                outfile << "\n# Subckt " << i << ": " << temp_subckt->inst_name << " \n";
            }
            outfile << ".subckt " << temp_subckt->model_type->name << " \\\n" ;
		}

			
		if (debug){
			outfile << "Input Map:\n" ;
		}

        size_t num_print_output_pins = count_print_pins(temp_subckt->model_type->outputs, &(temp_subckt->output_cnxns), print_unused_pins);
		
		//dump the input map containing connectivity data
        bool last = (num_print_output_pins == 0);
		dump_subckt_map(outfile,
					&(temp_subckt->input_cnxns), 
					temp_subckt->model_type->inputs, 
					temp_subckt->inst_name.c_str(), 
					"input", i,
                    print_unused_pins,
					debug,
                    last);
			
		if (debug){
			outfile << "\nOutput Map:\n" ;
		}
			
		//dump the output map containing connectivity data
        last = true;
		dump_subckt_map(outfile,
					&(temp_subckt->output_cnxns), 
					temp_subckt->model_type->outputs, 
					temp_subckt->inst_name.c_str(),
					"output", i,
                    print_unused_pins,
					debug,
                    last);
		if (!debug && eblif_format_local) {
			outfile << ".cname " << temp_subckt->inst_name << "\n" ;
            for (auto& param : temp_subckt->params) {
                outfile << ".param " << param.name << " " << param.value << "\n";
            }
            for (auto& attr : temp_subckt->attrs) {
                outfile << ".attr " << attr.name << " " << attr.value << "\n";
            }
			outfile << "\n" ;
        }
	}
}

//============================================================================================
//============================================================================================

void dump_subckt_map (ofstream& outfile, portmap* map, t_model_ports* temp_port, 
			const char* inst_name, const char* maptype, int s_index, t_boolean print_unused, t_boolean debug, bool last){
/*  Prints a subcircuit connectivity map, showing all connections.
 *  The keys to the map are the names of the ports in subcircuit's architecture model.
 *
 *	ARGUMENTS
 *  outfile:
 *	filestream to print the map information to.
 *  map:
 *	portmap containing the linkage between a port and an external pin association (Source: VQM File).
 *  temp_port:
 *	linked list of ports declared in the model, generates map keys (Source: Architecture File).
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
    string indent = "    ";

	int ports_dumped = 1;

    std::vector<std::string> port_strings;

	while(temp_port){
		for (int i = 0; i < temp_port->size; i++, ports_dumped++){
			//loop through a port's size
			if (temp_port->size > 1){
				//rename the port "name[i]"
				port_name = append_index_to_str((string)temp_port->name, i);
			} else {
				//otherwise just copy the port name without an index
				port_name = (string)temp_port->name;
			}
			temp_cnxn = map->find(port_name)->second;	
			//"second" is a member of portpair,
			//corresponds to the connection the port is mapped to
			if (temp_cnxn == NULL) {
				cout << "ERROR: " << temp_port->name << " not found in the " << maptype << " map of " << inst_name << ".\n" ; 
				exit(1);
			}

			pin_name = (string)temp_cnxn->associated_net->name;
			if (debug)
				outfile << "\t" ;

            bool is_input = (std::string(maptype) == "input");
            VTR_ASSERT(is_input || std::string(maptype) == "output");


            bool pin_used = (pin_name != "unconn");

            if (pin_used) {
				if (temp_cnxn->associated_net->indexed){
					//construct a string of "name[index]"
					pin_name = append_index_to_str (pin_name, temp_cnxn->wire_index);
				}

                port_strings.push_back(port_name + "=" + pin_name);

            } else {
                //Pin is unused
                if (print_unused) {
                    if(is_input) {
                        //i.e. connect unconnected inputs to the 'unconn' net
                        port_strings.push_back(port_name + "=" + pin_name);
                    } else {
                        //i.e. connect disconnected outputs to a uniquely named net
                        port_strings.push_back(port_name + "=subckt" + std::to_string(s_index) + "~" + port_name + "~unconn");
                    }
                }
            }
		}

		//print the next port's information
		temp_port = temp_port->next;
	}

    for (size_t i = 0; i < port_strings.size(); ++i) {
        outfile << indent << port_strings[i];

        if (!(last && i == port_strings.size() - 1)) {
            outfile << " \\";
        }
        outfile << "\n";
    }
}

size_t count_print_pins(t_model_ports* temp_port, portmap* map, t_boolean print_unused) {
    size_t count = 0;
	while(temp_port){
		for (int i = 0; i < temp_port->size; i++){
			//loop through a port's size
            string port_name;
			if (temp_port->size > 1){
				//rename the port "name[i]"
				port_name = append_index_to_str((string)temp_port->name, i);
			} else {
				//otherwise just copy the port name without an index
				port_name = (string)temp_port->name;
			}
			t_node_port_association* temp_cnxn = map->find(port_name)->second;	
			//"second" is a member of portpair,
			//corresponds to the connection the port is mapped to
			VTR_ASSERT(temp_cnxn != NULL);

			string pin_name = (string)temp_cnxn->associated_net->name;

            bool pin_used = (pin_name != "unconn");

            if(pin_used || print_unused) {
                ++count;
            }

        }
		temp_port = temp_port->next;
    }

    return count;
}

//============================================================================================
//============================================================================================

void dump_subckt_models(t_model* temp_model, ofstream& outfile, t_boolean debug){
/*  Cycles through all models declared in the architecture
 *  and dumps the ones used by the VQM as blackbox models
 *  at the end of the .blif file
 *
 *  temp_model:
 *	Linked list of models from the Architecture parser
 *  outfile:
 *	.blif file to output to
 *  debug:
 *	flag to indicate whether to print in DEBUG or BLIF format.
 */	
    unsigned long total_block_count = 0;
	while(temp_model){
		if (model_count[temp_model->index] > 0){
            //Count the number of blocks
            total_block_count += model_count[temp_model->index];

			//dump all .subckt models declared in the architecture 
			//Use the model_count array to only output the models that were used.
			if (!debug){
				cout << "\t>> Introduced " << model_count[temp_model->index] << " instances of blackbox " << temp_model->name << endl;
			}
			outfile << ((debug)? "\n Model: " : "\n.model ") << temp_model->name << endl ;
			
			outfile << ((debug)? "Inputs:\n" : ".inputs \\\n") ; //cycle through all inputs
			dump_subckt_portlist(outfile, temp_model->inputs, "    ", debug);

			outfile << ((debug)? "Outputs:\n" : ".outputs \\\n") ; //cycle through all outputs
			dump_subckt_portlist(outfile, temp_model->outputs, "    ", debug);

			outfile << ((debug)? "\nEND MODEL\n" : ".blackbox\n.end\n") ;
		}	
		temp_model = temp_model->next;
	}
    cout << "\t>> Total Block Count: " << total_block_count;
}

//============================================================================================
//============================================================================================

void dump_subckt_portlist(ofstream& outfile, t_model_ports* port, std::string indent, t_boolean debug){
/*  Dumps the portlist of a subcircuit model at the end of a BLIF, flattening busses as necessary.
 *
 *	ARGUMENTS
 *  outfile:
 *	.blif file to output to
 *  port:
 *	Linked list containing the ports of a subcircuit.
 *  debug:
 *	flag to indicate whether to print in DEBUG or BLIF format.
 */	
	string temp_name;
	int ports_dumped = 1;

	while (port){	
		for (int i = 0; i < port->size; i++, ports_dumped++){
			temp_name = (string)port->name;

			if (port->size > 1){	//append the index if necessary using the established convention
				temp_name = append_index_to_str(temp_name, i);
			}

			//dump the name of the port
			outfile << indent << temp_name ;

            bool last = (port->next == NULL && i == port->size - 1);

            if(!last) {
                outfile << " \\";
            }
            outfile << "\n";
		}
		port = port->next;
	}
	if (debug)
		outfile << endl ;
}

//============================================================================================
//				UTILITY FUNCTIONS
//============================================================================================

void all_data_cleanup(){
/* Frees all allocated memory from the parser and convertor.
 */
	vqm_data_cleanup();//found in ../LIB/vqm_dll.h, frees parser-allocated memory

	free(model_count);
	
	return;
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
 *	ARGUMENTS
 *  echo_file:
 *	Name of file printing vqm data.
 *  vqm_filename:
 *	Name of vqm file parsed.
 *  my_module:
 *	Module containing vqm data from the VQM Parser.
 */

	ofstream module_out (echo_file);
	if (!module_out.is_open()){
		cout << "ERROR: Cannot open file " << echo_file << endl ;
		exit(1);
	}

	module_out << "Input netlist file: " << vqm_filename << "\n\n" ;

	module_out << "Module name: " << my_module->name << "\n\n" ;
	
	echo_module_pins (module_out, my_module);

	echo_module_assigns (module_out, my_module);
	
	echo_module_nodes (module_out, my_module);

	// Close file.
	module_out.close();
 
}

//============================================================================================
//============================================================================================

void echo_module_pins (ofstream& outfile, t_module* module){
/* Print all pin information from the VQM module.
 *	Each pin has:
 *	- a name
 *	- right and left indeces
 *	- a type (direction)
 * 
 *	ARGUMENTS
 *  outfile:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */	
	t_pin_def* temp_pin;

	outfile << "\t\t***PIN INFORMATION***\n" ;
	outfile << "Number of pins: " << module->number_of_pins << "\n\n" ;
	for (int i = 0; i < module->number_of_pins; i++){
		//Example pin declaration: "input [31:0] a"
		temp_pin = module->array_of_pins[i];
		
		//name = "a"
		outfile << "Pin: " << i << " Name: " << temp_pin->name << endl;
		
		//right = 31, left = 0, indexed = true
		outfile << "\tLeft: " << temp_pin->left << "; Right: " << temp_pin->right << "; Indexed: " << ((temp_pin->indexed)?"true":"false") << endl ;

		//type = PIN_INPUT
		outfile << "\tType: ";
		switch (temp_pin->type){
			case PIN_INPUT:
				outfile << "Input\n\n" ;
				break;
			case PIN_OUTPUT:
				outfile << "Output\n\n" ;
				break;
			case PIN_WIRE:
				outfile << "Wire\n\n" ;
				break;
			case PIN_INOUT:
				outfile << "Inout : ERROR\n\n" ;
				outfile << "\n\nERROR: Detected inout pin in t_module.\n";
				exit(1);
			default:
				outfile << "Unknown: ERROR\n\n" ;
				outfile << "\n\nERROR: Detected unknown pintype in t_module.\n" ;
				exit(1);
		}
	}
}

//============================================================================================
//============================================================================================

void echo_module_assigns (ofstream& outfile, t_module* module){
/* Print all assignment information from the VQM module.
 *	Each assignment has: 
 *	- a target wire or pin (with an index) 
 *	- a value if it's a constant assignment
 *	- a source (with an index) if it's a pin assignment
 *	- flags indicating tristated and inverted assignments
 * 
 *	ARGUMENTS
 *  outfile:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */
	t_assign* temp_assign;

	outfile << "\t\t***ASSIGNMENT INFORMATION***\n" ;
	outfile << "Number of assignments: " << module->number_of_assignments << "\n\n" ;

	for (int i = 0; i < module->number_of_assignments; i++){
		temp_assign = module->array_of_assignments[i];

		if (temp_assign == NULL){
			continue;
		}

		//Echo Target wire/pin data.
		outfile << "\tAssignment: " << i << endl ;
		outfile << "Target: " << temp_assign->target->name << endl ;
		outfile << "\tIndexed: " << temp_assign->target->indexed << ", Target Index: " << temp_assign->target_index << endl ;
		outfile << "\tLeft: " << temp_assign->target->left << " Right: " << temp_assign->target->right << endl ;

		//Echo Source data.
		outfile << "Source: " ;
		if (temp_assign->source == NULL){
			//Constant assignment; e.g. assign gnd = 1'b0
			outfile << "Constant\n\tValue: " << temp_assign->value << endl ;
		} else {
			//Wire/pin assignment; e.g. assign a[10] = a[10]~O
			outfile << temp_assign->source->name << endl;
			outfile << "\tIndexed: " << temp_assign->source->indexed <<", Source Index: " << temp_assign->source_index << endl ;
			outfile << "\tLeft: " << temp_assign->source->left << " Right: " << temp_assign->source->right << endl ;
		}

		//Echo tristated and inversion flag data.
		outfile << "is_tristated: ";
		if (temp_assign->is_tristated){
			//Tristated assignments have a third associated wire/pin
			outfile << "true ; tri_control name: " << temp_assign->tri_control->name ; 
			outfile << " Index: " << temp_assign->tri_control_index << endl;
		} else {
			outfile << "false\n";
		}
		outfile << "inversion: " << ((temp_assign->inversion)?"true":"false") << "\n\n" ;
	}
}

//============================================================================================
//============================================================================================

void echo_module_nodes (ofstream& outfile, t_module* module){
/* Print all node information from the VQM module.
 *	Each node has:
 *	- a name
 *	- an array of parameters (string or integer)
 *	- an array of assigned ports with pointers to the nets they're 
 *	  assigned to.
 *
 *	ARGUMENTS
 *  outfile:
 *	file to echo information into
 *  module:
 *	module containing relevant data
 */
	t_node* temp_node;
	t_node_parameter* temp_param;
	t_node_port_association* temp_port;

	outfile << "\n\t\t***NODE INFORMATION***\n" ;
	outfile << "Number of nodes: " << module->number_of_nodes << "\n\n" ;

	for (int i = 0; i < module->number_of_nodes; i++){
		temp_node = module->array_of_nodes[i];

		outfile << "Node: " << i << "; Name: " << temp_node->name << "; Type: " << temp_node->type << endl ; 

		//Echo all parameter information from the array
		outfile << "\tNumber of params: " << temp_node->number_of_params << endl ;
		for (int j = 0; j < temp_node->number_of_params; j++){
			//Each parameter specifies a configuration of the node in the circuit.
			temp_param = temp_node->array_of_params[j];
			outfile << "\t\tParam: " << j << "; Name: " << temp_param->name << endl ;
			
			if (temp_param->type == NODE_PARAMETER_STRING){
				//String Parameter; e.g. operation_mode = "arithmetic"
				outfile << "\t\t\tType: String ; Value: " << temp_param->value.string_value << "\n\n" ;
			} else {
				//Integer Parameter; e.g. dataa_width = 8
				outfile << "\t\t\tType: Integer ; Value: "<< temp_param->value.integer_value << "\n\n" ;
			}
		}

		//Echo all port information from the array
		outfile << "\tNumber of ports: " << temp_node->number_of_ports << "\n\n" ;
		for (int j = 0; j < temp_node->number_of_ports; j++){
			//Each port specifies an association with an external pin and
			//an input or output to the node; e.g. ".portname(WIRE_NAME)"
			temp_port = temp_node->array_of_ports[j];
			outfile << "\t\tPort: " << j << "; Name: " << temp_port->port_name << "; Port Index: " << temp_port->port_index << endl ;

			if (temp_port->associated_net == NULL){
				//Open port, should never get here. Open ports don't appear in a VQM file.
				outfile << "\t\t\tAssociated: open\n\n";
			} else {
				//associated_net is a t_pin_def* structure, just as in
				//echo_module_pins(). 
				outfile << "\t\t\tAssociated: " << temp_port->associated_net->name << "; Wire Index: " << temp_port->wire_index << "\n\n" ;
			}
		}
		outfile << endl;
	}
}

//============================================================================================
//============================================================================================

void echo_blif_model (char* echo_file, const char* vqm_filename, 
				t_blif_model* my_model, t_model* temp_model) {
/*  Prints all model data into a .txt for debugging
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

	ofstream model_out (echo_file);		// Points to file printing blif data.
	if (!model_out.is_open()){
		cout << "\n\nERROR: Cannot open file " << echo_file << endl ;
		exit(1);
	}
	
	model_out << "\t\t### DEBUG FILE FOR BLIF MODEL: " << my_model->name << "###\n" ; 
	model_out << "Input netlist file: " << vqm_filename << "\n\n" ;
	
	model_out << "\n\tMAIN MODEL\n" ;
	
	//completely dump the top-level model in DEBUG format
	dump_main_model(my_model, model_out, T_TRUE, T_TRUE, T_TRUE);
	
	model_out << "\n\tSUBCKT MODELS\n";
	
	//now dump the subckt models from the architecture
	//that were used in the VQM file, in DEBUG format.
	if (my_model->subckts.size() > 0){
		dump_subckt_models(temp_model, model_out, T_TRUE);
	}
	
	// Close file.
	model_out.close();
}

//============================================================================================
//============================================================================================
#ifdef NO_CONSTS

string file_replace(string file, string strip, string replace){
/*  Uses ifstream and string functions to parse the file,
 *  replacing a target strip string with a replace string and outputting to a new file.
 *	
 *	ARGUMENTS
 *  file:
 *	the filepath and name to be altered, kept a string for editing purposes
 *  strip:
 *	specific character sequence to be targeted within the file.
 *  replace:
 *	specific character sequence to insert within the file in place of strip.
 */
	string sequence;
	size_t cptr; 
	
	if(verbose_mode){
		cout << "\t>> Opening file " << file << endl ;
	}
	ifstream in_genome (file.c_str());
	VTR_ASSERT(in_genome.is_open());
	
	cptr = file.find_last_of(".");
	file.replace(cptr, 1, "_no_" + strip + ".");  //append "_no_<strip>" to the filename.

  	ofstream out_genome (file.c_str());
	VTR_ASSERT(out_genome.is_open());

	if(verbose_mode){
			cout << "\n\tReplacing instances of " << strip << " with " << replace << "...\n" ;
	}

	int line_number = 0;
	//Read in_genome line-by-line
	while(getline(in_genome, sequence).good()){ //good() indicates a successful line read.
		line_number++;
		cptr = sequence.find(strip);
		//cptr points to the first instance of the target strip string, if it exists in the line.
		while (cptr != string::npos){
			//cptr == string::npos if strip wasn't found in this line

			if(verbose_mode){
				cout << "\t\tReplacing " << strip << " with " << replace << " on line " << line_number << ".\n" ;
			}
			//Replace the next <strip.length()> chars in the line with replace using string::replace...
			sequence.replace(cptr, strip.length(), replace);	
			//...and find the next strip sequence, if it exists.
			cptr = sequence.find(strip);
		}
		//Print the infected line to out_genome
		out_genome << sequence << endl;
	}

	//Has been dumping all along, but looks cleaner to output now.
	cout << "\n>> Dumping to output file " << file << "(post-hack- no " << strip << ")\n" ; 
	in_genome.close();
	out_genome.close();

	//Return the altered filename.
	return file;
}

//============================================================================================
//============================================================================================

#endif
