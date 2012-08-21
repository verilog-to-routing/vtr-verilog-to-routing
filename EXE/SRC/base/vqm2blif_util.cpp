#include "../include/vqm2blif_util.h"

//============================================================================================
//============================================================================================

void print_usage (t_boolean terminate){
//  Prints a standard usage reminder to the user, terminates the program if directed.
	cout << "********\n" ;
	cout << "USAGE:\n" ;
	cout << "\tvqm2blif -vqm <VQM file>.vqm -arch <ARCH file>.xml\n" ;
	cout << "OPTIONAL FLAGS:\n" ;
	cout << "\t-out <OUT file>.blif\n" ;
	cout << "\t-elab [none | modes | modes_detailed]\n" ;
	cout << "\t-clean [none | buffers | all]\n" ;
	cout << "\t-buffouts\n" ;
	cout << "\t-luts [vqm | blif]\n" ;
	cout << "\t-debug\n" ;
	cout << "\t-verbose\n" ;
	cout << "\nNote: All flags are order-independant. For more information, see README.txt\n\n";
	
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
		assert (net->left == net->right);
		return append_index_to_str((string)net->name, net->left);
	} else {
		return append_index_to_str((string)net->name, index);
	}
}

//============================================================================================
//============================================================================================

string generate_opname (t_node* vqm_node, t_boolean detailed_elaboration){
/*  Generates a mode-hash string based on a node's name and parameter set
 *
 *	ARGUMENTS
 *  vqm_node:
 *	the particular node in the VQM file to be translated
 */
	string mode_hash;	//temporary container for the mode-hashed block name

	mode_hash = (string)vqm_node->type;	//begin by copying the entire block name

	t_node_parameter* temp_param;	//temporary container for the node's parameters
   
    //We need to save the ram data and address widths, we can only make decisiosn based on all the parameters
    t_node_parameter* port_a_data_width = NULL;
    t_node_parameter* port_a_addr_width = NULL;
    t_node_parameter* port_b_data_width = NULL;
    t_node_parameter* port_b_addr_width = NULL;

    char buffer[128]; //For integer to string conversion, use with snprintf which checks for overflow

	for (size_t i = 0; i < vqm_node->number_of_params; i++){
		//Each parameter specifies a configuration of the node in the circuit.
		temp_param = vqm_node->array_of_params[i];

		if (strcmp (temp_param->name, "operation_mode") == 0){
			assert( temp_param->type == NODE_PARAMETER_STRING );

            //Copy the string
            char* temp_string_value = my_strdup(temp_param->value.string_value);

            //Remove characters that are invalid in blif
            clean_name(temp_string_value);

            //Add the opmode
			mode_hash.append(".opmode{" + (string)temp_string_value + "}");

            free(temp_string_value);
            continue;
		} 

        //Save the ram width/depth related parameters
        if (strcmp (temp_param->name, "port_a_data_width") == 0){
            assert( temp_param->type == NODE_PARAMETER_INTEGER );
            port_a_data_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_a_address_width") == 0){
            assert( temp_param->type == NODE_PARAMETER_INTEGER );
            port_a_addr_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_data_width") == 0){
            assert( temp_param->type == NODE_PARAMETER_INTEGER );
            port_b_data_width = temp_param;
            continue;
        }
        if (strcmp (temp_param->name, "port_b_address_width") == 0){
            assert( temp_param->type == NODE_PARAMETER_INTEGER );
            port_b_addr_width = temp_param;
            continue;
        }
    }

    //Detailed memory modes
    if (detailed_elaboration && (port_a_data_width != NULL) && (port_a_addr_width != NULL)
                             && (port_b_data_width == NULL) && (port_b_addr_width == NULL)) {
            //A single port memory, only port A params are found

            //Only print the address width, the data widths are handled by the VPR memory class
            snprintf(buffer, sizeof(buffer), "%d", port_a_addr_width->value.integer_value);
            mode_hash.append(".port_a_address_width{" + (string)buffer + "}");

    } else if (detailed_elaboration && (port_a_data_width != NULL) && (port_a_addr_width != NULL) 
                                    && (port_b_data_width != NULL) && (port_b_addr_width != NULL)) {
        //A dual port memory, both port A and B params have been found
      
        if (   (port_a_data_width->value.integer_value == port_b_data_width->value.integer_value)
            && (port_a_addr_width->value.integer_value == port_b_addr_width->value.integer_value)) {
            //Both ports are the same size, so only append the address widths, the data widths are handled by the VPR memory class

            snprintf(buffer, sizeof(buffer), "%d", port_a_addr_width->value.integer_value);
            mode_hash.append(".port_a_address_width{" + (string)buffer + "}");

            snprintf(buffer, sizeof(buffer), "%d", port_b_addr_width->value.integer_value);
            mode_hash.append(".port_b_address_width{" + (string)buffer + "}");

        } else {
            //Each port has a different size, so print both the address and data widths. Mixed widths are not handled by the VPR memory class
            snprintf(buffer, sizeof(buffer), "%d", port_a_data_width->value.integer_value);
            mode_hash.append(".port_a_data_width{" + (string)buffer + "}");

            snprintf(buffer, sizeof(buffer), "%d", port_a_addr_width->value.integer_value);
            mode_hash.append(".port_a_address_width{" + (string)buffer + "}");

            snprintf(buffer, sizeof(buffer), "%d", port_b_data_width->value.integer_value);
            mode_hash.append(".port_b_data_width{" + (string)buffer + "}");

            snprintf(buffer, sizeof(buffer), "%d", port_b_addr_width->value.integer_value);
            mode_hash.append(".port_b_address_width{" + (string)buffer + "}");

        }
	} else {
        ; //Not a memory, do nothing
    }

	return mode_hash;
}

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

	assert ((module->name != NULL)&&(strlen(module->name) > 0));

	t_pin_def* temp_pin;
	t_assign* temp_assign;
	t_node* temp_node;
	t_node_port_association* temp_port;

	for (int i = 0; i < module->number_of_pins; i++){
		temp_pin = module->array_of_pins[i];
		assert (temp_pin != NULL);
		assert ((temp_pin->name != NULL)&&(strlen(temp_pin->name) > 0));
	}

	for (int i = 0; i < module->number_of_assignments; i++){

		temp_assign = module->array_of_assignments[i];
		assert (temp_assign != NULL);

		temp_pin = temp_assign->target;
		assert (temp_pin != NULL);
		if (temp_assign->target_index >= 0){
			assert ((temp_assign->target_index <= max(temp_pin->left, temp_pin->right))&&
					(temp_assign->target_index >= min(temp_pin->left, temp_pin->right)));
		} else {
			assert (temp_pin->left == temp_pin->right);
		}
	}

	for (int i = 0; i < module->number_of_nodes; i++){
		temp_node = module->array_of_nodes[i];
		assert (temp_node != NULL);
		for (int j = 0; j < temp_node->number_of_ports; j++){
			temp_port = temp_node->array_of_ports[j];
			assert (temp_port != NULL);

			temp_pin = temp_port->associated_net;
			assert (temp_pin != NULL);
			
			if (temp_port->wire_index >= 0){
				assert ((temp_port->wire_index <= max(temp_pin->left, temp_pin->right))&&
						(temp_port->wire_index >= min(temp_pin->left, temp_pin->right))); 
			} else {
				assert (temp_pin->left == temp_pin->right);
			}
		}
	}
}

//============================================================================================
//============================================================================================
