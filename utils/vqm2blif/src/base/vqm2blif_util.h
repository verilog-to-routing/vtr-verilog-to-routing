#ifndef V2B_UTIL_H
#define V2B_UTIL_H

//============================================================================================
//				DEFINES
//============================================================================================
//#define NO_CONSTS 
//If defined, there will be no constant assignments in the BLIF and "gnd" and "vcc" associations 
//will be stripped and replaced with "unconn."

//#define VQM_BUSES
//If defined, the software will be compatible with bus-based assignments in the VQM. Currently,
//these assignments are not observed. Buses in a VQM File are flattened into 1-bit wide assigns.
//NOTE: The functionality of this option is untested.

#define MAX_LEN 350	//maximum length of a port/net name
#define MAX_PPL 10	//maximum number of ports that can be put into one line of a BLIF

//============================================================================================
//				INCLUDES
//============================================================================================
#include <stdlib.h>
#include <assert.h>
#include <time.h>

//fstream-specific hacks, util (in libvqm) and fstream both define min and max; incompatible!
#ifdef min 
#undef min 
#endif 
     
#ifdef max 
#undef max 
#endif

#include <fstream>
#include <sstream>
#include <bitset>
#include <iostream>

#include <string>
#include <map>
#include <vector>
#include <queue>
#include <set>
#include <string.h>
#include <regex>

#include "vqm_dll.h"	//VQM Parser
#include "hash.h"				//Hash Table Functions

#include "read_xml_arch_file.h"	//Architecture Parser

using namespace std;

//============================================================================================
//				GLOBAL TYPEDEFS & ENUMS
//============================================================================================

typedef vector<t_boolean> boolvec;

typedef vector<t_pin_def*> pinvec;

typedef vector<struct s_blif_subckt> scktvec;

typedef map <string,t_node_port_association*> portmap;
typedef pair<string,t_node_port_association*> portpair;

enum v_OptionBaseToken
{
	OT_VQM,
	OT_ARCH,
	OT_OUT,
	OT_DEBUG,
	OT_VERBOSE,
	OT_ELAB,
	OT_LUTS,
	OT_CLEAN,
	OT_BUFFOUTS,
    OT_FIXGLOBALS,
    OT_ELABORATE_RAM_CLOCKS,
    OT_MULTICLOCK_PRIMITIVES,
    OT_SPLIT_CARRY_CHAIN_LOGIC,
    OT_REMOVE_CONST_NETS,
    OT_INCLUDE_UNUSED_SUBCKT_PINS,
    OT_EBLIF_FORMAT,
	OT_UNKNOWN,
  OT_INSERT_CUSTOM_HARD_BLOCKS,
  OT_DEVICE
};

struct cstrcomp{	//operator structure to compare C-strings within a map class
  bool operator() (const char* lhs,const char* rhs) const
  {return strcmp(lhs, rhs) < 0;}
};

typedef map <const char*, v_OptionBaseToken, cstrcomp> tokmap;
typedef pair <const char*, v_OptionBaseToken> tokpair;

struct RamInfo {
    std::string mode = "";
    int port_a_addr_width = 0;
    int port_a_data_width = 0;
    t_node_port_association* port_a_input_clock = nullptr;
    t_node_port_association* port_a_output_clock = nullptr;

    int port_b_addr_width = 0;
    int port_b_data_width = 0;
    t_node_port_association* port_b_input_clock = nullptr;
    t_node_port_association* port_b_output_clock = nullptr;
};

// stores relevant information for a given FPGA device
// currently, just storing the strings used to idenitify luts and dff primitives and their ports within the vqm netlist
// add additional parameters as needed
struct DeviceInfo {
    std::string lut_type_name;
    std::string lut_output_port;
    int lut_output_port_size;

    std::string dff_type_name;
    std::string dff_output_port;
}; 

//============================================================================================
//				GLOBAL FUNCTIONS
//============================================================================================

//File Handling

void verify_format (string* filename, string extension);	//verifies a given string ends in ".extension"

// verifies whether the hard block type name provided by the user meets verilog naming rules
void verify_hard_block_type_name(string curr_hard_block_name);

// checks to see that the device name supplied matches the devices we can support
void verify_device(string device_name);

// if the hard block type name was escaped by '\', we need to remove the '\' character from the name (refer to function above)
void cleanup_hard_block_type_name(string* curr_hard_block_name);

void construct_filename (char* filename, const char* path, const char* ext);	//constructs a filename based on the path and termination passed

//Naming Conventions

string generate_opname (t_node* vqm_node, t_model* arch_models);	//generates a mode-hashed name for a subcircuit instance

string generate_opname_stratixiv (t_node* vqm_node, t_model* arch_models); //mode-hash for Stratix IV
void generate_opname_stratixiv_ram (t_node* vqm_node, t_model* arch_models, string& mode_hash); //mode-hash for Stratix IV RAM blocks
void generate_opname_stratixiv_dsp_mult (t_node* vqm_node, t_model* arch_models, string& mode_hash); //mode-hash for Stratix IV DSP Multiplers
void generate_opname_stratixiv_dsp_out (t_node* vqm_node, t_model* arch_models, string& mode_hash); //mode-hash for Stratix IV DSP Output (MAC)

t_model* find_arch_model_by_name(string model_name, t_model* arch_models); //returns the pointer to a module from the arch file, searches by name

string get_wire_name(t_pin_def* net, int index);	//returns a string with the appropriate wire name

string append_index_to_str (string busname, int index);	//appends an integer index to the end of a string

RamInfo get_ram_info(const t_node* vqm_node);

//Other

void print_usage (t_boolean terminate);

int get_width (t_pin_def* buspin);

void verify_module (t_module* module);

void clean_name(char* name);

//============================================================================================
//				MODE VARIABLES & ENUMS
//============================================================================================

extern t_boolean debug_mode;	//user-set flag causing the creation of intermediate files 
				//with debug information.

extern t_boolean verbose_mode;	//user-set flag that indicates more verbose runtime output

typedef enum v_Elab_Mode { NONE, MODES, MODES_TIMING, ATOMS, UNKNOWN } e_elab;
extern e_elab elab_mode;		//user-set flag dictating how to elaborate a VQM Primitive

typedef enum v_LUT_Mode { VQM, BLIF, OTHER } e_lut;
extern e_lut lut_mode;		//user-set flag dictating how to treat LUTs (as blackboxes or .names)

typedef enum v_Clean_Mode { CL_NONE, CL_BUFF, CL_ALL } e_clean;
extern e_clean clean_mode;	//user-set flag dictating how to clean away buffers/invertors

extern t_boolean buffd_outs;	//user-set flag that regulates whether to keep buffered outputs

//============================================================================================
//				DEVICE SPECIFIC INFORMATION 
//============================================================================================

/*
  A database that stores parameters for different types of FPGA devices.
  Currently only the stratix 4 device parameters are stored.
*/
const map<std::string, DeviceInfo> device_parameter_database {
    // stratix 4 device
    {"stratixiv", {"stratixiv_lcell_comb", "combout", 1, "dffeas", "q" }},
    {"stratix10", {"fourteennm_lcell_comb", "combout", 1, "fourteennm_ff", "q" }},
    {"agilex", {"tennm_lcell_comb", "combout", 1, "tennm_ff", "q" }}
};

#endif


