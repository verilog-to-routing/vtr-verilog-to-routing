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
#include <vector>
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
#include <string.h>

#include "libvqm/vqm_dll.h"	//VQM Parser
#include "hash.h"				//Hash Table Functions

extern "C" {
	#include "libvpr/include/read_xml_arch_file.h"	//Architecture Parser
}

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
	OT_UNKNOWN
};

struct cstrcomp{	//operator structure to compare C-strings within a map class
  bool operator() (const char* lhs,const char* rhs) const
  {return strcmp(lhs, rhs) < 0;}
};

typedef map <const char*, v_OptionBaseToken, cstrcomp> tokmap;
typedef pair <const char*, v_OptionBaseToken> tokpair;

//============================================================================================
//				GLOBAL FUNCTIONS
//============================================================================================

//File Handling

void verify_format (string* filename, string extension);	//verifies a given string ends in ".extension"

void construct_filename (char* filename, const char* path, const char* ext);	//constructs a filename based on the path and termination passed

//Naming Conventions

string generate_opname (t_node* vqm_node, t_boolean detailed_elaboration);	//generates a mode-hashed name for a subcircuit instance

string get_wire_name(t_pin_def* net, int index);	//returns a string with the appropriate wire name

string append_index_to_str (string busname, int index);	//appends an integer index to the end of a string

//Other

void print_usage (t_boolean terminate);

int get_width (t_pin_def* buspin);

void verify_module (t_module* module);

//============================================================================================
//				MODE VARIABLES & ENUMS
//============================================================================================

extern t_boolean debug_mode;	//user-set flag causing the creation of intermediate files 
				//with debug information.

extern t_boolean verbose_mode;	//user-set flag that indicates more verbose runtime output

typedef enum v_Elab_Mode { NONE, MODES, MODES_DETAILED, ATOMS, UNKNOWN } e_elab;
extern e_elab elab_mode;		//user-set flag dictating how to elaborate a VQM Primitive

typedef enum v_LUT_Mode { VQM, BLIF, OTHER } e_lut;
extern e_lut lut_mode;		//user-set flag dictating how to treat LUTs (as blackboxes or .names)

typedef enum v_Clean_Mode { CL_NONE, CL_BUFF, CL_ALL } e_clean;
extern e_clean clean_mode;	//user-set flag dictating how to clean away buffers/invertors

extern t_boolean buffd_outs;	//user-set flag that regulates whether to keep buffered outputs

#endif


