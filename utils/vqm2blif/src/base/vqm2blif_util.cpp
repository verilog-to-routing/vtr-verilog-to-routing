#include "vqm2blif_util.h"
#include "logic_types.h"
#include "vtr_util.h"
#include "vtr_assert.h"

#define dsp_clock_count 15
//============================================================================================
//============================================================================================

void print_usage (t_boolean terminate){
//  Prints a standard usage reminder to the user, terminates the program if directed.
	cout << "********\n" ;
	cout << "USAGE:\n" ;
	cout << "\tvqm2blif -vqm <VQM file>.vqm -arch <ARCH file>.xml\n" ;
	cout << "OPTIONAL FLAGS:\n" ;
	cout << "\t-out <OUT file>\n" ;
	cout << "\t-elab [none | modes | modes_timing*]\n" ;
	cout << "\t-clean [none | buffers | all*]\n" ;
	cout << "\t-buffouts\n" ;
	cout << "\t-luts [vqm* | blif]\n" ;
    cout << "\t-multiclock_primitives\n";
    cout << "\t-include_unused_subckt_pins\n";
    cout << "\t-remove_const_nets\n";
    cout << "\t-eblif_format\n";
    cout << "\t-insert_custom_hard_blocks <hard_block_1_module_name> <hard_block_2_module_name> ...\n";
    cout << "\t-device <device name> (if not provided then default is 'stratixiv')\n";
    //Hide experimental options by default
    //cout << "\t-split_multiclock_blocks\n";
    //cout << "\t-split_carry_chain_logic\n";
    //cout << "\t-fixglobals\n";
	cout << "\t-debug\n" ;
	cout << "\t-verbose\n" ;
	cout << "\nNote: Default values indicated with * . All flags are order-independant. For more information, see README.txt\n\n";
	
	if (terminate)
		exit(1);
}

//============================================================================================
//============================================================================================

void verify_format (string* filename, string extension){
//  Verifies that a filename string ends with the desired extension.

	size_t cptr = filename->find_last_of(".");
	if ( filename->substr(cptr+1) != extension ){
		cout << "ERROR: Improper filename " << *filename << ".\n" ;
		print_usage(T_TRUE);
	}
}

//============================================================================================
//============================================================================================



void verify_hard_block_type_name(string curr_hard_block_type_name){
// verifies whether the hard block name (type of hard block) provided by the user meets verilog/VHDL naming rules. VHDL naming rules are a subset of verilog, so we just check verilog. (Using verilog 2001 standard)

    // naming rules have 2 main conditions:
    // Condition 1: the first charatcer must be a lowercase/uppercase alphabetical character. Or the first character can be a underscore.
    // Condition 2: The remaning characters must be a lowercase/uppercase alphabetical character, or a underscore, or a single digit number or the '$' character
    // the rules above are checked with the identifier below 
    std::regex verilog_VHDL_naming_rules_one ("^[a-zA-Z_][a-zA-Z_\\$0-9]*[a-zA-Z_\\$0-9]$");

    // verilog names can also contain any characters, as long as they are escaped with a '\' at the start of the identifer. For example, \reset-
    // we check this using the identifier below
    std::regex verilog_VHDL_naming_rules_two ("[\\](.*)", std::regex_constants::extended); // need std::regex_constants::extended as it supports '\\' character

   if ((!(std::regex_match(curr_hard_block_type_name, verilog_VHDL_naming_rules_one))) && (!(std::regex_match(curr_hard_block_type_name, verilog_VHDL_naming_rules_two))))
    {
        // the hard block type name did not meet the verilog naming rules
        // Display error message to user
        std::cout << "ERROR:The provided Hard Block Type Name '" << curr_hard_block_type_name << "' did not meet the verilog/VHDL naming rules.\n";

        std::cout << "********\n";

        std::cout << "Please ensure the provided Hard Block Type Names follow the conditions below:\n";

        std::cout << "\tCondition 1: The first character of the Hard Block Type Name should either be a-z, A-Z or _\n";

        std::cout << "\tCondition 2: The remaining characters of the Hard Block Type Name should either be a-z, A-Z, 0-9, _ or $\n";

        std::cout << "Alternatively, any character can be used for the hard block type name, as long as the name is escaped using '\\' character. For example, '\\reset-' would be legal.\n";

        exit(1);

    }

    return;

}

//============================================================================================
//============================================================================================

void verify_device(string device_name)
{
    /* 
        The list of devices that this program can support and their parameters are stored within a map structure ('device_parameter_database'). We
        check whether the provided device by the user supported by comparing it
        to the devices within the map. 
    */

    std::map<std::string, DeviceInfo>::const_iterator device_support_status;

    device_support_status = device_parameter_database.find(device_name);

    // checks to see whether we support the given device
    if (device_support_status == device_parameter_database.end())
    {
        // if we are here then we don't support the user supplied device
        std::cout << "ERROR:The provided device is not supported.";
        std::cout << " Only the following devices are supported:\n";

        device_support_status = device_parameter_database.begin();

        while (device_support_status != device_parameter_database.end())
        {
            std::cout << device_support_status->first << "\n";
            device_support_status++;
        }

        exit(1);
    }

    return;

}

//============================================================================================
//============================================================================================

//============================================================================================
//============================================================================================

void cleanup_hard_block_type_name(string* curr_hard_block_type_name){
// if the hard block type name was escaped by '\', we need to remove the '\' character from the name

    // matching token below tries to determine whether the current hard block type name was escaped with '\'
    std::regex verilog_naming_rules ("[\\](.*)", std::regex_constants::extended);

    // stores the hard block type name without the '\' character
    std::smatch matches;

    // store the matched strings and also determine whether
    // the hard block type name is actually escaped. if not escaped, then we do not need to do anything
    if (std::regex_match(*curr_hard_block_type_name, matches, verilog_naming_rules, std::regex_constants::match_default))
    {
        // if we are here, there should be two string matches
        // the first match is the entire hard block type name with the escape character '\'
        // the second match should just be the hard block type name without the escape character '\'

        // below we just store the second match
        curr_hard_block_type_name->assign(matches[matches.size() - 1]);
    }

    return;

}

void construct_filename (char* filename, const char* path, const char* ext){
//Constructs a char* filename from a given path and extension.

	strcpy(filename, path);		//add the path
	strcat(filename, ext);		//add the desired extension
	
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
	if (index < 0){
		cout << "PARSER ERROR: Cannot append " << index << " as index.\n" ;
		exit(1);
	}

	stringstream ss;
	ss << index;

	return (busname + '[' + ss.str() + ']');
}

//============================================================================================
//============================================================================================

string get_wire_name(t_pin_def* net, int index){
/* Constructs a wire name based on its indeces and width.
 * If index == -1, the entire net is used (multiple wires if the net is a bus).
 * net->indexed indicates whether the net is declared as a bus or just a wire.
 */
	if (net->indexed == T_FALSE){
		return (string)net->name;
	} else if (index == -1) {
		//a wire must only be 1-bit wide!! Check right and left indeces.
		VTR_ASSERT(net->left == net->right);
		return append_index_to_str((string)net->name, net->left);
	} else {
		return append_index_to_str((string)net->name, index);
	}
}

//============================================================================================
//============================================================================================

string generate_opname (t_node* vqm_node, const LogicalModels& arch_models, string device){
    //Add support for different architectures here.
    // Currently only support Stratix IV and Stratix 10
    string mode_hash;
    if(device == "stratix10")
        mode_hash = generate_opname_stratix10(vqm_node, arch_models);    
    if(device == "stratixiv")
        mode_hash = generate_opname_stratixiv(vqm_node, arch_models);
    
    //Final sanity check
    LogicalModelId arch_model_id = arch_models.get_model_by_name(mode_hash);
    if (!arch_model_id.is_valid()) {
        cout << "Error: could not find primitive '" << mode_hash << "' in architecture file" << endl;
        exit(1);
    }

    return mode_hash;

}

string generate_opname_stratixiv (t_node* vqm_node, const LogicalModels& arch_models){
/*  Generates a mode-hash string based on a node's name and parameter set
 *
 *	ARGUMENTS
 *  vqm_node:
 *	the particular node in the VQM file to be translated
 */
	string mode_hash;	//temporary container for the mode-hashed block name

	mode_hash = (string)vqm_node->type;	//begin by copying the entire block name

	t_node_parameter* temp_param;	//temporary container for the node's parameters
   
    //The blocks operation mode
    t_node_parameter* operation_mode = NULL;
    
	for (int i = 0; i < vqm_node->number_of_params; i++){
		//Each parameter specifies a configuration of the node in the circuit.
		temp_param = vqm_node->array_of_params[i];

        //Save the operation mode parameter
		if (strcmp (temp_param->name, "operation_mode") == 0){
			VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            operation_mode = temp_param;
            break;
		} 
    }
    
    //Simple opmode name appended
    //    NOTE: this applies to all blocks
    if (operation_mode != NULL) {
        //Copy the string
        char* temp_string_value = vtr::strdup(operation_mode->value.string_value);

        //Remove characters that are invalid in blif
        clean_name(temp_string_value);

        //Add the opmode
        mode_hash.append(".opmode{" + (string)temp_string_value + "}");

        free(temp_string_value);
    }

    /*
     * DSP Block Multipliers
     */
    if(strcmp(vqm_node->type, "stratixiv_mac_mult") == 0) {
        generate_opname_stratixiv_dsp_mult(vqm_node, mode_hash);
    }

    /*
     * DSP Block Output (MAC)
     */
    if(strcmp(vqm_node->type, "stratixiv_mac_out") == 0) {
        generate_opname_stratixiv_dsp_out(vqm_node, mode_hash);
    }

    /*
     * RAM Blocks
     */
    if(strcmp(vqm_node->type, "stratixiv_ram_block") == 0) {
        generate_opname_ram(vqm_node, arch_models, mode_hash, "stratixiv");
    }

    return mode_hash;
}

void generate_opname_stratixiv_dsp_mult (t_node* vqm_node, string& mode_hash) {
    //We check for all mac_mult's input ports to see if any use a clock
    // if so, we set ALL input ports to be registered.  While this is an approximation,
    // it would be very unusually to have only some of the ports registered.
    //
    // E.g. data input registered, but associated sign bit not registered.
    //
    // The only exception to this might be if one data input was regisetered, while
    // the other was not - however more detailed modeling like this would bloat the
    // architecture description significantly, so we make the above assumption.
    //
    // The main output of the mac_mult can not be registered, but the scanouta port can be.
    // We assume that this never occurs. Since the scanouta port is internal to the hardened DSP
    // block, it should never end up on the critical path, so this should not impact the accuracy
    // of system timing.
    //
    // We attempt to inform the users of any approximations being made during the conversion
    // process.
    //
    // Provided that an input/outputs clock parameter value is 'none', then it is combinational (doesn't use the register)
    //
    // See QUIP stratixiii_dsp_wys.pdf for details on the mac_mult block and meaning of parameters
    
    VTR_ASSERT(strcmp(vqm_node->type, "stratixiv_mac_mult") == 0);

    if(elab_mode == MODES_TIMING) {
        //Only elaborate registered/combinational behaviour if in timing accurate
        // mode elaboration

        //Assume all possibly registered inputs/outputs are registered
        bool dataa_input_reg = true;
        bool datab_input_reg = true;
        bool signa_input_reg = true;
        bool signb_input_reg = true;
        bool scanouta_output_reg = true;

        for (int i = 0; i < vqm_node->number_of_params; i++){
            //Each parameter specifies a configuration of the node in the circuit.
            t_node_parameter* temp_param = vqm_node->array_of_params[i];

            //Check the clocking related parameters
            if (strcmp (temp_param->name, "dataa_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    dataa_input_reg = false;
                }
                continue;
            }

            if (strcmp (temp_param->name, "datab_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    datab_input_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "signa_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    signa_input_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "signb_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    signb_input_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "scanouta_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    scanouta_output_reg = false;
                }
                continue;
            }
        }

        //If ANY of the input ports are registered, we model all input ports as registered
        if(dataa_input_reg || datab_input_reg || signa_input_reg || signb_input_reg) {

            //In the unsual case of only some inputs being registered, print a warning to the user
            //if(verbose_mode) {
                //if(!dataa_input_reg || !datab_input_reg || !signa_input_reg || !signb_input_reg) {
                    //cout << "Warning: DSP " << vqm_node->type << " '" << vqm_node->name << "' has only some inputs registered.";
                    //cout << " Approximating as all inputs registered." << endl; 
                //}
            //}

            //Mark this pimitive instance as having registered inputs
            //cout << "DSP mac_mult '" << vqm_node->name << "' with REG inputs" << endl;
            mode_hash.append(".input_type{reg}");

        } else {
            //Mark this primitive instance as having unregistered inputs
            //cout << "DSP mac_mult '" << vqm_node->name << "' with COMB inputs" << endl;
            mode_hash.append(".input_type{comb}");
        }

        //Check if we are approximating the registered scanouta port
        if(verbose_mode) {
            if(scanouta_output_reg) {
                //cout << "Warning: DSP " << vqm_node->type << " '" << vqm_node->name << "' has registered 'scanouta' port.";
                //cout << " Approximating as combinational." << endl; 
            }
        }
    }
}

void generate_opname_stratixiv_dsp_out (t_node* vqm_node, string& mode_hash) {
    //It is not practical to model all of the internal registers of the mac_out block, as this
    // would significantly increase the size of the architecture description.  As a result, we
    // only identify whether the input or output registers are used.
    //
    // For the input registers, we check only for the register after the first adder stage.
    // This means we don't check the initial input registers for control signals like:
    //   zerroloopback, zerochainout, zeroacc, signa, signb, rotate, shiftright, round, saturate,
    //   roundchainout, saturatechainout
    // However these (except for signa and signb) directly feed the register after the first stage
    // adder. While this may not be correct from a cycle accurate perspective, from a timing
    // perspective it should make no difference, as they wouldn't be on the critical path anyway.
    //
    // For the output register, we check only for the final register.
    //
    // Provided that an input/outputs clock parameter value is 'none', then it is combinational (doesn't use the register)
    //
    // See QUIP stratixiii_dsp_wys.pdf for DSP output architecture details and port/parameter
    // meanings.
    VTR_ASSERT(strcmp(vqm_node->type, "stratixiv_mac_out") == 0);
    if(elab_mode == MODES_TIMING) {
        //Only elaborate registered/combinational behaviour if in timing accurate
        // mode elaboration
    
        //Assume all registered inputs/outputs are registered
        //  Note that the mac_out has many possibly clocked inputs and outputs.
        //  Most would never be on the critical path, so we do not check those.
        bool first_adder0_reg = true;
        bool first_adder1_reg = true;
        bool second_adder_reg = true; //Note to give a warning
        bool output_reg = true;


        for (int i = 0; i < vqm_node->number_of_params; i++){
            //Each parameter specifies a configuration of the node in the circuit.
            t_node_parameter* temp_param = vqm_node->array_of_params[i];

            //Check the clocking related parameters
            if (strcmp (temp_param->name, "first_adder0_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    first_adder0_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "first_adder1_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    first_adder1_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "second_adder_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    second_adder_reg = false;
                }
                continue;
            }
            if (strcmp (temp_param->name, "output_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    output_reg = false;
                }
                continue;
            }
        }

        //Check for input registers
        if(first_adder0_reg || first_adder1_reg) {

            //In nearly all reasonable usage modes, both adders should be using the same clock,
            // Print a warning if they are not
            //if(verbose_mode) {
                //if(!first_adder0_reg || !first_adder1_reg) {
                    //cout << "Warning: DSP " << vqm_node->type << " '" << vqm_node->name << "' has only some inputs registered.";
                    //cout << " Approximating as all inputs registered." << endl; 
                //}
            //}
            //Mark this pimitive instance as having registered inputs
            //cout << "DSP mac_out '" << vqm_node->name << "' with REG inputs" << endl;
            mode_hash.append(".input_type{reg}");

        } else {
            //Mark this primitive instance as having unregistered inputs
            //cout << "DSP mac_out '" << vqm_node->name << "' with COMB inputs" << endl;
            mode_hash.append(".input_type{comb}");
        }

        //Check for output registers
        if(output_reg) {
            //Mark this pimitive instance as having registered outputs
            //cout << "DSP mac_out '" << vqm_node->name << "' with REG outputs" << endl;
            mode_hash.append(".output_type{reg}");

        } else {
            //Mark this primitive instance as having unregistered outputs
            //cout << "DSP mac_out '" << vqm_node->name << "' with COMB outputs" << endl;
            mode_hash.append(".output_type{comb}");
        }

        //Print a warning if second stage adder reg was not used
        if(verbose_mode) {
            if(!second_adder_reg) {
                //cout << "Warning: DSP " << vqm_node->type << " '" << vqm_node->name << "' does not use second stage register.";
                //cout << " Please check architecture carefully to verify any timing approximations made about the block." << endl; 
            }
        }
    }

}
        
void generate_opname_ram (t_node* vqm_node, const LogicalModels& arch_models, string& mode_hash, string device) {
    
    if(device == "stratixiv")
        VTR_ASSERT(strcmp(vqm_node->type, "stratixiv_ram_block") == 0);
    else
        VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_ram_block") == 0);

    
    RamInfo ram_info = get_ram_info(vqm_node, device);

    /*
     *  The following code attempts to identify RAM bocks which do and do not use
     *  output registers.
     */
    if(elab_mode == MODES_TIMING) {
        //Only elaborate registered/combinational behaviour if in timing accurate
        // mode elaboration
        
        //Mark whether the outputs are registered or not
        bool reg_outputs = false;
        if (ram_info.mode == "single_port" || ram_info.mode == "rom") {
            VTR_ASSERT(!ram_info.port_b_input_clock);
            VTR_ASSERT(!ram_info.port_b_output_clock);

            if(ram_info.port_a_output_clock) {
                reg_outputs = true;
            }
        } else {
            VTR_ASSERT(ram_info.mode == "dual_port" || ram_info.mode == "bidir_dual_port" || ram_info.mode == "quad_port");
            VTR_ASSERT_MSG(ram_info.port_b_input_clock, "RAM inputs always assumed sequential");

            if (ram_info.mode == "dual_port" && ram_info.port_b_output_clock) {
                reg_outputs = true;
            } else if (ram_info.mode == "bidir_dual_port") {
                
                if (ram_info.port_a_output_clock && ram_info.port_b_output_clock) {
                    reg_outputs = true; //Sequential output
                } else if (!ram_info.port_a_output_clock && !ram_info.port_b_output_clock) {
                    reg_outputs = false; //Comb output
                } else {
                    cout << "Unable to resolve whether bi-dir dual port RAM " << vqm_node->name << " outputs are sequential or combinational:\n";
                    cout << "   port A output sequential: " << bool(ram_info.port_a_output_clock) << "\n";
                    cout << "   port B output sequential: " << bool(ram_info.port_b_output_clock) << "\n";
                }
            } else {
                if (ram_info.port_a_output_clock && ram_info.port_b_output_clock) {
                    reg_outputs = true; //Sequential output
                } else if (!ram_info.port_a_output_clock && !ram_info.port_b_output_clock) {
                    reg_outputs = false; //Comb output
                } else {
                    cout << "Unable to resolve whether quad port RAM " << vqm_node->name << " outputs are sequential or combinational:\n";
                    cout << "   port A output sequential: " << bool(ram_info.port_a_output_clock) << "\n";
                    cout << "   port B output sequential: " << bool(ram_info.port_b_output_clock) << "\n";
                }
            }
        }

        if(reg_outputs) {
            mode_hash.append(".output_type{reg}");
        } else {
            mode_hash.append(".output_type{comb}");
        }
    }

    /*
     *  Which parameters to append to the vqm primitive name depends on what
     *  primitives are included in the Architecture file.
     *
     *  The following code attempts to create the most detailed description of
     *  a RAM primitive possible, PROVIDED it exists in the architecture file.
     *
     *  This is done in several steps:
     *      1) If it is a single port memory, just append both the opmode and address_width
     *          e.g. $ram_primitive_name.opmode{single_port}.port_a_address_width{7}
     *
     *      2) If it is a dual_port memory, with two ports of the same width, append the opmode and address_widths
     *          e.g. $ram_primitive_name.opmode{dual_port}.port_a_address_width{5}.port_b_address_width{5}
     *
     *      3) If it is a dual_port memory, with two ports of different width:
     *          a) Use the simplest name (no extra width or address info appended)
     *          b) Unless the most detailed name (opmode + address_widths + data_widths) exists in the arch file
     *
     */
    // 1) A single port memory, only port A params are found
    //if (   (port_a_data_width != NULL) && (port_a_addr_width != NULL)
        //&& (port_b_data_width == NULL) && (port_b_addr_width == NULL)) {
    if(ram_info.mode == "single_port" || ram_info.mode == "rom") {
        VTR_ASSERT(ram_info.port_b_addr_width == 0);
        VTR_ASSERT(ram_info.port_b_addr2_width == 0);
        VTR_ASSERT(ram_info.port_a_addr2_width == 0);

        //Only print the address width, the data widths are handled by the VPR memory class
        mode_hash.append(".port_a_address_width{" + std::to_string(ram_info.port_a_addr_width) + "}");

        LogicalModelId arch_model_id = arch_models.get_model_by_name(mode_hash);
        if (!arch_model_id.is_valid()) {
            cout << "Error: could not find single port memory primitive '" << mode_hash << "' in architecture file" << endl;
            exit(1);
        }

    //A dual port memory, both port A and B params have been found
    } else if (ram_info.mode == "dual_port" || ram_info.mode == "bidir_dual_port"){
        VTR_ASSERT(ram_info.port_b_addr2_width == 0);
        VTR_ASSERT(ram_info.port_a_addr2_width == 0);
        
        //2) Both ports are the same size, so only append the address widths, the data widths are handled by the VPR memory class
        if (   (ram_info.port_a_data_width == ram_info.port_b_data_width)
            && (ram_info.port_a_addr_width == ram_info.port_b_addr_width)) {

            mode_hash.append(".port_a_address_width{" + std::to_string(ram_info.port_a_addr_width) + "}");
            mode_hash.append(".port_b_address_width{" + std::to_string(ram_info.port_b_addr_width) + "}");

            LogicalModelId arch_model_id = arch_models.get_model_by_name(mode_hash);
            if (!arch_model_id.is_valid()) {
                cout << "Error: could not find dual port (non-mixed_width) memory primitive '" << mode_hash << "' in architecture file";
                exit(1);
            }
        //3) Mixed width dual port ram
        } else {
            //Make a temporary copy of the mode hash
            string tmp_mode_hash = mode_hash;

            /*
             * Try to see if the detailed version exists in the architecture,
             * if it does, use it.  Otherwise use the operation mode only.
             */

            tmp_mode_hash.append(".port_a_address_width{" + std::to_string(ram_info.port_a_addr_width) + "}");
            tmp_mode_hash.append(".port_b_address_width{" + std::to_string(ram_info.port_b_addr_width) + "}");

            //Each port has a different size, so print both the address and data widths. Mixed widths are not handled by the VPR memory class
            tmp_mode_hash.append(".port_a_data_width{" + std::to_string(ram_info.port_a_data_width) + "}");
            tmp_mode_hash.append(".port_b_data_width{" + std::to_string(ram_info.port_b_data_width) + "}");

            LogicalModelId arch_model_id = arch_models.get_model_by_name(tmp_mode_hash);
            if (!arch_model_id.is_valid()) {
                //3a) Not found, use the default name (no specific address/data widths)
                ; // do nothing
            } else {
                //3b) Use the more detailed name, since it was found in the architecture
                mode_hash = tmp_mode_hash;
            }
        }
    } else {
        VTR_ASSERT(ram_info.mode == "quad_port");
        
        //2) Both ports are the same size, so only append the address widths, the data widths are handled by the VPR memory class
        if (   (ram_info.port_a_data_width == ram_info.port_b_data_width)
            && (ram_info.port_a_addr_width == ram_info.port_b_addr_width)
            && (ram_info.port_a_addr_width == ram_info.port_b_addr2_width)
            && (ram_info.port_a_addr2_width == ram_info.port_b_addr2_width)) {

            mode_hash.append(".port_a_address_width{" + std::to_string(ram_info.port_a_addr_width) + "}");
            mode_hash.append(".port_a_address2_width{" + std::to_string(ram_info.port_a_addr2_width) + "}");
            mode_hash.append(".port_b_address_width{" + std::to_string(ram_info.port_b_addr_width) + "}");
            mode_hash.append(".port_b_address2_width{" + std::to_string(ram_info.port_b_addr2_width) + "}");

            LogicalModelId arch_model_id = arch_models.get_model_by_name(mode_hash);
            if (!arch_model_id.is_valid()) {
                cout << "Error: could not find dual port (non-mixed_width) memory primitive '" << mode_hash << "' in architecture file";
                exit(1);
            }
        //3) Mixed width dual port ram
        } else {
            //Make a temporary copy of the mode hash
            string tmp_mode_hash = mode_hash;

            /*
             * Try to see if the detailed version exists in the architecture,
             * if it does, use it.  Otherwise use the operation mode only.
             */

            tmp_mode_hash.append(".port_a_address_width{" + std::to_string(ram_info.port_a_addr_width) + "}");
            tmp_mode_hash.append(".port_a_address2_width{" + std::to_string(ram_info.port_a_addr2_width) + "}");
            tmp_mode_hash.append(".port_b_address_width{" + std::to_string(ram_info.port_b_addr_width) + "}");
            tmp_mode_hash.append(".port_b_address2_width{" + std::to_string(ram_info.port_b_addr2_width) + "}");

            //Each port has a different size, so print both the address and data widths. Mixed widths are not handled by the VPR memory class
            tmp_mode_hash.append(".port_a_data_width{" + std::to_string(ram_info.port_a_data_width) + "}");
            tmp_mode_hash.append(".port_b_data_width{" + std::to_string(ram_info.port_b_data_width) + "}");

            LogicalModelId arch_model_id = arch_models.get_model_by_name(tmp_mode_hash);
            if (!arch_model_id.is_valid()) {
                //3a) Not found, use the default name (no specific address/data widths)
                ; // do nothing
            } else {
                //3b) Use the more detailed name, since it was found in the architecture
                mode_hash = tmp_mode_hash;
            }
        }
    }
}

string generate_opname_stratix10 (t_node* vqm_node, const LogicalModels& arch_models){
/*  Generates a mode-hash string based on a node's name and parameter set
 *
 *	ARGUMENTS
 *  vqm_node:
 *	the particular node in the VQM file to be translated
 */
    string mode_hash;	//temporary container for the mode-hashed block name
    mode_hash = (string)vqm_node->type;	//begin by copying the entire block name
    t_node_parameter* temp_param;	//temporary container for the node's parameters

    //The blocks operation mode
    t_node_parameter* operation_mode = NULL;
    
    for (int i = 0; i < vqm_node->number_of_params; i++){
        //Each parameter specifies a configuration of the node in the circuit.
        temp_param = vqm_node->array_of_params[i];

        //Save the operation mode parameter
        if (strcmp (temp_param->name, "operation_mode") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            operation_mode = temp_param;
            break;
	} 
    }

    //Simple opmode name appended
    //    NOTE: this applies to all blocks
    if (operation_mode != NULL) {
        //Copy the string
        char* temp_string_value = vtr::strdup(operation_mode->value.string_value);

        //Remove characters that are invalid in blif
        clean_name(temp_string_value);

        //Add the opmode
        mode_hash.append(".opmode{" + (string)temp_string_value + "}");

        free(temp_string_value);
    }

    /*
     * Fracturable LUT
     */
    if(strcmp(vqm_node->type, "fourteennm_lcell_comb") == 0) {
        generate_opname_stratix10_lut(vqm_node, mode_hash);  
    }

    /*
     * DSP Block - fixed point
     */
    if(strcmp(vqm_node->type, "fourteennm_mac") == 0) {
        generate_opname_stratix10_dsp(vqm_node, mode_hash, 0);
        
    }

    /*
     * DSP Block - floating point
     */
    if(strcmp(vqm_node->type, "fourteennm_fp_mac") == 0) {
        generate_opname_stratix10_dsp(vqm_node, mode_hash, 1);
        
    }

    /*
     * RAM Blocks
     */
    if(strcmp(vqm_node->type, "fourteennm_ram_block") == 0) {
        generate_opname_ram(vqm_node, arch_models, mode_hash, "stratix10");
    }

    return mode_hash;
}

void generate_opname_stratix10_dsp (t_node* vqm_node, string& mode_hash, bool dsp_mode) {
    //
    // It is not practical to model all of the internal registers of the mac block, as this
    // would significantly increase the size of the architecture description.  As a result, we
    // only identify whether the input or output registers are used.
    //
    // We check for all mac's input ports to see if any use a clock
    // if so, we set ALL input ports to be registered 
    //  While this is an approximation,
    // it would be very unusually to have only some of the ports registered.
    //
    // E.g. data input registered, but associated sign bit not registered.
    //
    // The only exception to this might be if one data input was regisetered, while
    // the other was not - however more detailed modeling like this would bloat the
    // architecture description significantly, so we make the above assumption.
    //
    // For the output register, we check only for the final register.
    //
    // We attempt to inform the users of any approximations being made during the conversion
    // process.
    //
    // Provided that an input/outputs clock parameter value is 'none', then it is combinational (doesn't use the register)
    //
    // The boolean variable dsp_mode indicates wether the dsp block is in fixed point mode (dsp_mode = 0) or floating point mode (dsp_mode = 1)
    if(dsp_mode == 0)
        VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_mac") == 0);
    else
        VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_fp_mac") == 0);

    if(elab_mode == MODES_TIMING) {
        //Only elaborate registered/combinational behaviour if in timing accurate
        // mode elaboration
        
        //an array of all the input clocks
        // if one of the following clocks are active then all inputs are assumed to be registered
        const char *input_regs [6] = {"ax_clock", "ay_clock", "az_clock", "bx_clock", "by_clock", "bz_clock"};
        int inputs_regs_size;
        
        // in fixed point mode there are can be up to six data inputs while in floating point mode there are up to three inputs only
        if(dsp_mode == 0)
            inputs_regs_size = 6;
        else
            inputs_regs_size = 3;
        //Variables indicating whether input or output ports are registered
        bool input_reg = false;
        bool output_reg = true;

        // checking the clock related parameters to see if the in/out ports are registered
        for (int i = 0; i < vqm_node->number_of_params; i++) {

            //each parameter specifies a configuration of the node in the circuit.
            t_node_parameter* temp_param = vqm_node->array_of_params[i];
            
            // check if the current parameter is related to an input clock
            for(int j = 0; j < inputs_regs_size; j++) {

                if(strcmp(temp_param->name, input_regs[j]) == 0) {
                    VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                    // if the found input clock is activated set the input_reg to true
                    if(strcmp(temp_param->value.string_value, "none") != 0) {

                        input_reg = true;
                        break;
                    }
                }
            }

            //Check the parameter indicating the source of output clock
            if (strcmp (temp_param->name, "output_clock") == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                // if the output clock source is none set the output reg to false
                if(strcmp(temp_param->value.string_value, "none") == 0) {
                    output_reg = false;
                }
                break;
            }
        }
        
        //Check for input registers
        if(input_reg) {
            //Mark this pimitive instance as having registered inputs
            mode_hash.append(".input_type{reg}");

        } else {
            //Mark this primitive instance as having unregistered inputs
            mode_hash.append(".input_type{comb}");
        }

        //Check for output registers
        if(output_reg) {
            //Mark this pimitive instance as having registered outputs
            mode_hash.append(".output_type{reg}");

        } else {
            //Mark this primitive instance as having unregistered outputs
            mode_hash.append(".output_type{comb}");
        }   
    }
}

//Generates an alphabetically continuous series of LUT input ports,
// e.g. if the logical LUT has the inputs dataa, datac, and datad used
// these inputs get remapped to dataa, datab, and datac ports
// The 5-LUT physical primitive described in S10 arch 
// has input ports dataa to datae.  
// Without this remapping some of the 5-input logical primitives that use ports
// such as dataf or datag cannot be mapped to the physical 5-LUT primitive
// defined in the arch file
void remap_lut_ports(t_node* vqm_node){
    char lut_in_start_char = 'a';
    int lut_input_idx = 0;
    for(int i = 0; i < vqm_node->number_of_ports; ++i) {

        t_node_port_association* vqm_port = vqm_node->array_of_ports[i];

        // If the port name starts with data, then the port is a LUT input
        if(strncmp(vqm_port->port_name, "data", 4) == 0) {
            vqm_port->port_name[4] = lut_in_start_char + lut_input_idx;
            lut_input_idx += 1;
        }
    }

}

void generate_opname_stratix10_lut (t_node* vqm_node, string& mode_hash) {
    // Stratix 10 has a 6-input FLUT that can be configured either as a single 8-input
    // LUT (under certain conditions the 6-input FLUT can implement up to 8-input functions 
    // so in the primitive description there are 8 inputs rather than 6) or two 5-input LUTs with two shared inputs.
    // To enable VPR to distinguish between 5-input LUTs (used in fractured mode) and 6-8 inputs LUTs (used in non-fractured mode)
    // we convert the fourteennm_lcell_comb primitives to one of the fourteennm_lcell_comb5 or fourteennm_lcell_comb6
    // based on the number of inputs

    
    VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_lcell_comb") == 0);

    int num_of_inputs = 0;

    for(int i = 0; i < vqm_node->number_of_ports; ++i) {
        t_node_port_association* vqm_port = vqm_node->array_of_ports[i];

        // If the port name starts with data, then the port is a LUT input
        if(strncmp(vqm_port->port_name, "data", 4) == 0) {
            num_of_inputs += 1;
        }
    }

    if(num_of_inputs < 6) {
        mode_hash = "fourteennm_lcell_comb5";
        // remap the input ports of the logical primitive to the set of fourteennm_lcell_comb5 input ports described in the arch file
        remap_lut_ports(vqm_node);
    } else {
        mode_hash = "fourteennm_lcell_comb6";
    }
}


//============================================================================================
//============================================================================================
void clean_name(char* name) {
    /*
     * Remove invalid characters from blif identifiers
     */
    size_t p;
    for (p = 0; p < strlen(name); p++) {
        switch(name[p]) {
            case ' ': //Currently only spaces have been causing issues
                    name[p] = '_';
                    break;
            default:
                    break;
        }
    }
}

//============================================================================================
//============================================================================================

int get_width (t_pin_def* buspin){
	return (abs(buspin->left - buspin->right) + 1);
}

//============================================================================================
//============================================================================================

void verify_module (t_module* module){
	if (module == NULL){
		cout << "ERROR: VQM File invalid.\n";
		exit(1);
	}

	VTR_ASSERT((module->name != NULL)&&(strlen(module->name) > 0));

	t_pin_def* temp_pin;
	t_assign* temp_assign;
	t_node* temp_node;
	t_node_port_association* temp_port;

	for (int i = 0; i < module->number_of_pins; i++){
		temp_pin = module->array_of_pins[i];
		VTR_ASSERT(temp_pin != NULL);
		VTR_ASSERT((temp_pin->name != NULL)&&(strlen(temp_pin->name) > 0));
	}

	for (int i = 0; i < module->number_of_assignments; i++){

		temp_assign = module->array_of_assignments[i];
		VTR_ASSERT(temp_assign != NULL);

		temp_pin = temp_assign->target;
		VTR_ASSERT(temp_pin != NULL);
		if (temp_assign->target_index >= 0){
			VTR_ASSERT((temp_assign->target_index <= max(temp_pin->left, temp_pin->right))&&
					(temp_assign->target_index >= min(temp_pin->left, temp_pin->right)));
		} else {
			VTR_ASSERT(temp_pin->left == temp_pin->right);
		}
	}

	for (int i = 0; i < module->number_of_nodes; i++){
		temp_node = module->array_of_nodes[i];
		VTR_ASSERT(temp_node != NULL);
		for (int j = 0; j < temp_node->number_of_ports; j++){
			temp_port = temp_node->array_of_ports[j];
			VTR_ASSERT(temp_port != NULL);

			temp_pin = temp_port->associated_net;
			VTR_ASSERT(temp_pin != NULL);
			
			if (temp_port->wire_index >= 0){
				VTR_ASSERT((temp_port->wire_index <= max(temp_pin->left, temp_pin->right))&&
						(temp_port->wire_index >= min(temp_pin->left, temp_pin->right))); 
			} else {
				VTR_ASSERT(temp_pin->left == temp_pin->right);
			}
		}
	}
}

RamInfo get_ram_info(const t_node* vqm_node, string device) {
    if(device=="stratixiv")
        VTR_ASSERT(strcmp(vqm_node->type, "stratixiv_ram_block") == 0);
    if(device=="stratix10")
        VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_ram_block") == 0);

    t_node_parameter* operation_mode = NULL;
    
    //We need to save clock information about the RAM to determine whether output registers are used
    t_node_parameter* port_a_address_clock = NULL;
    t_node_parameter* port_a_data_out_clock = NULL;
    t_node_parameter* port_b_address_clock = NULL;
    t_node_parameter* port_b_data_out_clock = NULL;
    
    //We need to save clock enable information about the RAM to determine whether clock enables for registers are used - For Stratix 10 only
    t_node_parameter* clk0_input_clock_enable = NULL;
    t_node_parameter* clk0_output_clock_enable = NULL;
    t_node_parameter* clk1_input_clock_enable = NULL;
    t_node_parameter* clk1_output_clock_enable = NULL;

    //We need to save clear information about the RAM to determine whether clear signals for output registers are used - For Stratix 10 only
    t_node_parameter* port_a_dataout_clear = NULL;
    t_node_parameter* port_b_dataout_clear = NULL;

    //We need to save the ram data and address widths, to identfy the RAM type (singel port, rom, simple dual port, true dual port)
    t_node_parameter* port_a_data_width = NULL;
    t_node_parameter* port_a_addr_width = NULL;
    t_node_parameter* port_a_addr2_width = NULL;
    t_node_parameter* port_b_data_width = NULL;
    t_node_parameter* port_b_addr_width = NULL;
    t_node_parameter* port_b_addr2_width = NULL;

	for (int i = 0; i < vqm_node->number_of_params; i++){
		//Each parameter specifies a configuration of the node in the circuit.
		t_node_parameter* temp_param = vqm_node->array_of_params[i];

        //Save the ram width/depth related parameters
        if (strcmp (temp_param->name, "operation_mode") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            operation_mode = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_data_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_a_data_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_address_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_a_addr_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_address2_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_a_addr2_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_data_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_b_data_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_address_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_b_addr_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_address2_width") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_INTEGER );
            port_b_addr2_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_address_clock") == 0){ // This parameter doesn't exist for Stratix 10 - clock0 is always used for port address_a
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_a_address_clock = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_data_out_clock") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_a_data_out_clock = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_address_clock") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_b_address_clock = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_data_out_clock") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_b_data_out_clock = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "clk0_input_clock_enable") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            clk0_input_clock_enable = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "clk0_output_clock_enable") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            clk0_output_clock_enable = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "clk1_input_clock_enable") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            clk1_input_clock_enable = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "clk1_output_clock_enable") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            clk1_output_clock_enable = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_data_out_clear") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_a_dataout_clear = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_data_out_clear") == 0){
            VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
            port_b_dataout_clear = temp_param;
            continue;
        }
        
    }

    RamInfo ram_info;

    //Set the mode
    VTR_ASSERT(operation_mode);
    ram_info.mode = operation_mode->value.string_value;

    //Set the widths
    VTR_ASSERT(port_a_addr_width);
    ram_info.port_a_addr_width = port_a_addr_width->value.integer_value;

    VTR_ASSERT(port_a_data_width);
    ram_info.port_a_data_width = port_a_data_width->value.integer_value;

    if (port_a_addr2_width) {
        ram_info.port_a_addr2_width = port_a_addr2_width->value.integer_value;
    }
    if (port_b_addr2_width) {
        ram_info.port_b_addr2_width = port_b_addr2_width->value.integer_value;
    } 
    if (port_b_addr_width) {
        ram_info.port_b_addr_width = port_b_addr_width->value.integer_value;
    }

    if (port_b_data_width) {
        ram_info.port_b_data_width = port_b_data_width->value.integer_value;
    }


    //Find the clock ports
    t_node_port_association* clk0_port = nullptr;
    t_node_port_association* clk1_port = nullptr;

    t_node_port_association* clk_portain = nullptr;
    t_node_port_association* clk_portaout = nullptr;
    t_node_port_association* clk_portbin = nullptr;
    t_node_port_association* clk_portbout = nullptr;

    //find the enable ports
    t_node_port_association* ena0_port = nullptr;
    t_node_port_association* ena1_port = nullptr;

    // find the enables ports correspodning to each register
    t_node_port_association* clk0_enable_in_port = nullptr;
    t_node_port_association* clk0_enable_out_port = nullptr;
    t_node_port_association* clk1_enable_in_port = nullptr;
    t_node_port_association* clk1_enable_out_port = nullptr;

    // find the clear ports
    t_node_port_association* sclr_port = nullptr;
    t_node_port_association* aclr_port = nullptr;


    for(int i = 0; i < vqm_node->number_of_ports; ++i) {
        t_node_port_association* check_port = vqm_node->array_of_ports[i];
        if(check_port->port_name == std::string("clk0")) {
            clk0_port = check_port;
        }
        if(check_port->port_name == std::string("clk1")) {
            clk1_port = check_port;
        }
        if(check_port->port_name == std::string("clk_portain")) {
            clk_portain = check_port;
        }
        if(check_port->port_name == std::string("clk_portaout")) {
            clk_portaout = check_port;
        }
        if(check_port->port_name == std::string("clk_portbin")) {
            clk_portbin = check_port;
        }
        if(check_port->port_name == std::string("clk_portbout")) {
            clk_portbout = check_port;
        }
        if(check_port->port_name == std::string("ena0")) {
            ena0_port = check_port;
        }
        if(check_port->port_name == std::string("ena1")) {
            ena1_port = check_port;
        }
        if(check_port->port_name == std::string("sclr")) {
            sclr_port = check_port;
        }
        if(check_port->port_name == std::string("aclr")) {
            aclr_port = check_port;
        }
    }

    if(clk0_port) {
        //Not pre-processed, so remap the clocks ourselvs
        VTR_ASSERT(!clk_portain);
        VTR_ASSERT(!clk_portaout);
        VTR_ASSERT(!clk_portbin);
        VTR_ASSERT(!clk_portbout);
        if (device == "stratix10"){
        // set the clock enable_in and enable_out ports for clock0 and clock1 
            if (clk0_input_clock_enable) {
                if (clk0_input_clock_enable->value.string_value == std::string("ena0")){
                    VTR_ASSERT(ena0_port);
                    clk0_enable_in_port = ena0_port;
                } else if (clk0_input_clock_enable->value.string_value == std::string("ena1")) {
                    VTR_ASSERT(ena1_port);
                    clk0_enable_in_port = ena1_port;
                } else {
                    VTR_ASSERT(clk0_input_clock_enable->value.string_value == std::string("none"));
                    clk0_enable_in_port = nullptr;
                }
            }
            if (clk0_output_clock_enable) {
                if (clk0_output_clock_enable->value.string_value == std::string("ena0")){
                    VTR_ASSERT(ena0_port);
                    clk0_enable_out_port = ena0_port;
                } else if (clk0_output_clock_enable->value.string_value == std::string("ena1")) {
                    VTR_ASSERT(ena1_port);
                    clk0_enable_out_port = ena1_port;
                } else {
                    VTR_ASSERT(clk0_output_clock_enable->value.string_value == std::string("none"));
                    clk0_enable_out_port = nullptr;
                }
            }
            if (clk1_input_clock_enable) {
                if (clk1_input_clock_enable->value.string_value == std::string("ena0")){
                    VTR_ASSERT(ena0_port);
                    clk1_enable_in_port = ena0_port;
                } else if (clk1_input_clock_enable->value.string_value == std::string("ena1")) {
                    VTR_ASSERT(ena1_port);
                    clk1_enable_in_port = ena1_port;
                } else {
                    VTR_ASSERT(clk1_input_clock_enable->value.string_value == std::string("none"));
                    clk1_enable_in_port = nullptr;
                }
            }
            if (clk1_output_clock_enable) {
                if (clk1_output_clock_enable->value.string_value == std::string("ena0")){
                    VTR_ASSERT(ena0_port);
                    clk1_enable_out_port = ena0_port;
                } else if (clk1_output_clock_enable->value.string_value == std::string("ena1")) {
                    VTR_ASSERT(ena1_port);
                    clk1_enable_out_port = ena1_port;
                } else {
                    VTR_ASSERT(clk1_output_clock_enable->value.string_value == std::string("none"));
                    clk1_enable_out_port = nullptr;
                }
            }
            // set the clear ports for output registers 
            if (port_a_dataout_clear) {
                if (port_a_dataout_clear->value.string_value == std::string("aclear")){
                    VTR_ASSERT(aclr_port);
                    ram_info.port_a_dataout_aclr = aclr_port;
                    ram_info.port_a_dataout_sclr = nullptr;

                } else if (port_a_dataout_clear->value.string_value == std::string("sclear")) {
                    VTR_ASSERT(sclr_port);
                    ram_info.port_a_dataout_aclr = nullptr;
                    ram_info.port_a_dataout_sclr = sclr_port;
                } else {
                    VTR_ASSERT(port_a_dataout_clear->value.string_value == std::string("none"));
                    ram_info.port_a_dataout_aclr = nullptr;
                    ram_info.port_a_dataout_sclr = nullptr;
                }
            }
            if (port_b_dataout_clear) {
                if (port_b_dataout_clear->value.string_value == std::string("aclear")){
                    VTR_ASSERT(aclr_port);
                    ram_info.port_b_dataout_aclr = aclr_port;
                    ram_info.port_b_dataout_sclr = nullptr;
                } else if (port_b_dataout_clear->value.string_value == std::string("sclear")) {
                    VTR_ASSERT(sclr_port);
                    ram_info.port_b_dataout_aclr = nullptr;
                    ram_info.port_b_dataout_sclr = sclr_port;
                } else {
                    VTR_ASSERT(port_b_dataout_clear->value.string_value == std::string("none"));
                    ram_info.port_b_dataout_aclr = nullptr;
                    ram_info.port_b_dataout_sclr = nullptr;
                }
            }
        }

        //Port a input clock and the corresponding enable
        if(!port_a_address_clock) {
            //Assumed that port a input is always controlled by clk0, 
            //none of the VQM files have produced a port_a_address_clock param
            ram_info.port_a_input_clock = clk0_port;
            ram_info.port_a_input_ena = clk0_enable_in_port;

        } else if (port_a_address_clock->value.string_value == std::string("clock0")){
            ram_info.port_a_input_clock = clk0_port;
            ram_info.port_a_input_ena = clk0_enable_in_port;
        } else {
            VTR_ASSERT(port_a_address_clock->value.string_value == std::string("clock1"));
            VTR_ASSERT(clk1_port);
            ram_info.port_a_input_clock = clk1_port;
            ram_info.port_a_input_ena = clk1_enable_in_port;
        }

        //Port a output clock
        VTR_ASSERT(port_a_data_out_clock);
        if (port_a_data_out_clock->value.string_value == std::string("clock0")){
            ram_info.port_a_output_clock = clk0_port;
            ram_info.port_a_output_ena = clk0_enable_out_port;
        } else if (port_a_data_out_clock->value.string_value == std::string("clock1")) {
            VTR_ASSERT(clk1_port);
            ram_info.port_a_output_clock = clk1_port;
            ram_info.port_a_output_ena = clk1_enable_out_port;
        } else {
            VTR_ASSERT(port_a_data_out_clock->value.string_value == std::string("none"));
            ram_info.port_a_output_clock = nullptr;
        }

        //Port b input clock
        if (port_b_address_clock) {
            if (port_b_address_clock->value.string_value == std::string("clock0")){
                ram_info.port_b_input_clock = clk0_port;
                ram_info.port_b_input_ena = clk0_enable_in_port;

            } else if (port_b_address_clock->value.string_value == std::string("clock1")) {
                VTR_ASSERT(clk1_port);
                ram_info.port_b_input_clock = clk1_port;
                ram_info.port_b_input_ena = clk1_enable_in_port;

            } else {
                VTR_ASSERT(port_b_address_clock->value.string_value == std::string("none"));
                ram_info.port_b_input_clock = nullptr;
                ram_info.port_b_input_ena = nullptr;

            }
        }

        //Port b output clock and enable
        if (port_b_data_out_clock) {
            if (port_b_data_out_clock->value.string_value == std::string("clock0")){
                ram_info.port_b_output_clock = clk0_port;
                ram_info.port_b_output_ena = clk0_enable_out_port;
            } else if (port_b_data_out_clock->value.string_value == std::string("clock1")) {
                VTR_ASSERT(clk1_port);
                ram_info.port_b_output_clock = clk1_port;
                ram_info.port_b_output_ena = clk1_enable_out_port;
            } else {
                VTR_ASSERT(port_b_data_out_clock->value.string_value == std::string("none"));
                ram_info.port_b_output_clock = nullptr;
                ram_info.port_b_output_ena = nullptr;

            }
        }
        
    } else {
        VTR_ASSERT(!clk0_port);
        VTR_ASSERT(!clk1_port);
        //Use the pre-processed values
        ram_info.port_a_input_clock = clk_portain;
        ram_info.port_a_output_clock = clk_portaout;
        ram_info.port_b_input_clock = clk_portbin;
        ram_info.port_b_output_clock = clk_portbout;
    }
    return ram_info;
}
//============================================================================================
//============================================================================================

void set_dsp_clk(DSPInfo& dsp_info, t_node_port_association* source_clk, std::string clock_name){
   
        if(clock_name.compare("ax_clock") == 0){ 
            dsp_info.port_ax_clock = source_clk;
        }        
        if(clock_name.compare("ay_clock") == 0){ 
            dsp_info.port_ay_clock = source_clk;
        }        
        if(clock_name.compare("az_clock") == 0){ 
            dsp_info.port_az_clock = source_clk;
        }        
        if(clock_name.compare("bx_clock") == 0){ 
            dsp_info.port_bx_clock = source_clk;
        }        
        if(clock_name.compare("by_clock") == 0){ 
            dsp_info.port_by_clock = source_clk;
        }        
        if(clock_name.compare("bz_clock") == 0){ 
            dsp_info.port_bz_clock = source_clk;
        }        
        if(clock_name.compare("coef_sel_a_clock") == 0){ 
            dsp_info.port_coef_sel_a_clock = source_clk;
        }        
        if(clock_name.compare("coef_sel_b_clock") == 0){ 
            dsp_info.port_coef_sel_b_clock = source_clk;
        }        
        if(clock_name.compare("ay_scan_in_clock") == 0){ 
            dsp_info.port_ay_scan_in_clock = source_clk;
        }        
        if(clock_name.compare("accumulate_clock") == 0){
            dsp_info.port_accumulate_clock = source_clk;
        }        
        if(clock_name.compare("load_const_clock") == 0){
            dsp_info.port_load_const_clock = source_clk;
        }        
        if(clock_name.compare("negate_clock") == 0){
            dsp_info.port_negate_clock = source_clk;
        }        
        if(clock_name.compare("sub_clock") == 0){
            dsp_info.port_sub_clock = source_clk;
        }        
        if(clock_name.compare("chainout_clock") == 0){
            dsp_info.port_chainout_clock = source_clk;
        }        
        if(clock_name.compare("output_clock") == 0){
            dsp_info.port_output_clock = source_clk; 
        }
}


DSPInfo get_dsp_info(const t_node* vqm_node) {

    VTR_ASSERT(strcmp(vqm_node->type, "fourteennm_mac") == 0 || strcmp(vqm_node->type, "fourteennm_fp_mac") == 0);

    const char* clock_param_name[dsp_clock_count] = { 
        "ax_clock", 
        "ay_clock", 
        "az_clock", 
        "bx_clock", 
        "by_clock", 
        "bz_clock", 
        "coef_sel_a_clock", 
        "coef_sel_b_clock", 
        "ay_scan_in_clock", 
        "accumulate_clock",
        "load_const_clock",
        "negate_clock",
        "sub_clock",
        "chainout_clock",
        "output_clock"
    };
    //We need to save clock-related parameters of a dsp block to determine which clock signal is driving which register
    map<std::string, t_node_parameter*> clock_param;


	for (int i = 0; i < vqm_node->number_of_params; i++){

		t_node_parameter* temp_param = vqm_node->array_of_params[i];

        for (int j = 0; j < dsp_clock_count; j++){

            if (strcmp (temp_param->name, clock_param_name[j]) == 0){
                VTR_ASSERT( temp_param->type == NODE_PARAMETER_STRING );
                clock_param.insert({clock_param_name[j],temp_param});
                continue;
            }

        }
    }

    //There are three distinct clock signals at most
    t_node_port_association* clk_port [] = {nullptr, nullptr, nullptr};
    // find the three bit clock port in the vqm_node
    for(int i = 0; i < vqm_node->number_of_ports; ++i) {
        t_node_port_association* check_port = vqm_node->array_of_ports[i];
        if(check_port->port_name == std::string("clk")) {
            clk_port[check_port->port_index] = check_port;
        }
    }

    DSPInfo dsp_info;

    if(clk_port[0] || clk_port[1] || clk_port[2]) {

        for (auto const& param : clock_param){

    //         set the new register-based clock ports to one of the three clock signals based on the corrosponding parameters 
            if (param.second) {
                if (param.second->value.string_value == std::string("0")){
                    VTR_ASSERT(clk_port[0]);
                    set_dsp_clk(dsp_info, clk_port[0], param.first);
                } else if (param.second->value.string_value == std::string("1")) {
                    VTR_ASSERT(clk_port[1]);
                    set_dsp_clk(dsp_info, clk_port[1], param.first);
                } else if (param.second->value.string_value == std::string("2")) {
                    VTR_ASSERT(clk_port[2]);
                    set_dsp_clk(dsp_info, clk_port[2], param.first);
                } else {
                    VTR_ASSERT(param.second->value.string_value == std::string("none"));
                    set_dsp_clk(dsp_info, nullptr, param.first);
                }
            }
        }
    }

    return dsp_info;
}
//============================================================================================
//============================================================================================
