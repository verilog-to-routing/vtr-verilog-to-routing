/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __SUBCKT_BLIF_READER_H__
#define __SUBCKT_BLIF_READER_H__

#include "BLIF.hh"

/**
 * @brief A class to provide the general object of an input BLIF file reader
*/
class SubcktBLIFReader : public BLIF::Reader {

    public:
        /**
         * @brief Construct the SubcktBLIFReader object
         * required by compiler
         */
        SubcktBLIFReader();
        /**
         * @brief Destruct the SubcktBLIFReader object
         * to avoid memory leakage
         */
        ~SubcktBLIFReader();

        /**
         * @brief Reads a blif file with the given filename and produces
         * a netlist which is referred to by the global variable
         * "blif_netlist".
         * 
         * @return the generated netlist file 
         */
        void* __read();

        /* No need to have writer in Generic Reader */
        void __write(const netlist_t* /* netlist */) {
            error_message(UTIL, unknown_location, "%s is not available in Generic Reader\n", __PRETTY_FUNCTION__);
        }

    protected:
        /**
         *---------------------------------------------------------------------------------------------
         * (function: read_tokens)
         *
         * @brief Parses the given line from the blif file. 
         * Returns true if there are more lines to read.
         * 
         * @param buffer a global buffer for tokenizing
         * @param models list of hard block models
         *-------------------------------------------------------------------------------------------*/
        int read_tokens(char* buffer, hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: hook_up_nets)
         * 
         * @brief find the output nets and add the corresponding nets
         *-------------------------------------------------------------------------------------------*/
        void hook_up_nets();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: hook_up_node)
         * 
         * @brief Connect the given node's input pins to their corresponding nets by
         * looking each one up in the output_nets_sc.
         * 
         * @param node represents one of netlist internal nodes or ff nodes or top output nodes
         * ---------------------------------------------------------------------------------------------*/
        void hook_up_node(nnode_t* node);
        /**
         *------------------------------------------------------------------------------------------- 
        * (function:create_hard_block_nodes)
        * 
        * @brief to create the hard block nodes
        * 
        * @param name_prefix
        * @param models list of hard block models
        *-------------------------------------------------------------------------------------------*/
        void create_hard_block_nodes(const char* name_prefix, hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function:create_internal_node_and_driver)
         * 
         * @brief to create an internal node and driver from that node
         * 
         * @param name_prefix
         *-------------------------------------------------------------------------------------------*/
        void create_internal_node_and_driver(const char* name_prefix);
        /**
         *---------------------------------------------------------------------------------------------
        * (function: build_top_input_node)
        * 
        * @brief to build a the top level input
        * 
        * @param name_str representing the name input signal
        *-------------------------------------------------------------------------------------------*/
        void build_top_input_node(const char* name_prefix, const char* name_str);
        /**
         * (function: add_top_input_nodes)
         * 
         * @brief to add the top level inputs to the netlist
         *
         * @param model hard block model
         *-------------------------------------------------------------------------------------------*/
        void add_top_input_nodes(const char* name_prefix, hard_block_model* model);
        /**
         *---------------------------------------------------------------------------------------------
        * (function: rb_create_top_output_nodes)
        * 
        * @brief to add the top level outputs to the netlist
        * 
        * @param model hard block model
        *-------------------------------------------------------------------------------------------*/
        void rb_create_top_output_nodes(const char* name_prefix, hard_block_model* model);
        /**
         *---------------------------------------------------------------------------------------------
        * (function: verify_hard_block_ports_against_model)
        * 
        * @brief Check for inconsistencies between the hard block model and the ports found
        * in the hard block instance. Returns false if differences are found.
        * 
        * @param ports list of a hard block ports
        * @param model hard block model
        * 
        * @return the hard is verified against model or no.
        *-------------------------------------------------------------------------------------------*/
        bool verify_hard_block_ports_against_model(hard_block_ports* ports, hard_block_model* model);
        /**
         *---------------------------------------------------------------------------------------------
        * (function: read_hard_block_model)
        * 
        * @brief Scans ahead in the given file to find the
        * model for the hard block by the given name.
        * 
        * @param name_subckt representing the name of the sub-circuit 
        * @param ports list of a hard block ports
        * 
        * @return the file to its original position when finished.
        *-------------------------------------------------------------------------------------------*/
        hard_block_model* read_hard_block_model(char* name_prefix, char* name_subckt, hard_block_ports* ports, hard_block_models* models);


    private:
        /**
         *---------------------------------------------------------------------------------------------
         * (function: build_internal_input_node)
         * 
         * @brief to build a the top level input
         * 
         * @param name_str representing the name input signal
         *-------------------------------------------------------------------------------------------*/
        void build_internal_input_node(const char* name_prefix, const char* name_str);
        /**
         * (function: add_internal_input_nodes)
         * 
         * @brief to add the top level inputs to the netlist
         *
         * @param name_prefix
         * @param model hard block model
         *-------------------------------------------------------------------------------------------*/
        void add_internal_input_nodes(const char* name_prefix, hard_block_model* model);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: rb_create_internal_output_nodes)
         * 
         * @brief to add the top level outputs to the netlist
         * 
         * @param name_prefix
         * 
         *-------------------------------------------------------------------------------------------*/
        void rb_create_internal_output_nodes(const char* name_prefix, hard_block_model* model);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: model_parse)
         * 
         * @brief in case of having other tool's blif file like yosys odin needs to read all .model
         * however the odin generated blif file includes only one .model representing the top module
         * 
         * @param buffer a global buffer for tokenizing
         *-------------------------------------------------------------------------------------------*/
        void model_parse(char* buffer);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: resolve_signal_name_based_on_blif_type)
         * 
         * @brief to make the signal names of an input blif 
         * file compatible with the odin's name convention
         * 
         * @param name_str representing the name input signal
         *-------------------------------------------------------------------------------------------*/
        char* resolve_signal_name_based_on_blif_type(const char* name_prefix, const char* name_str);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: rename_port_name)
         * 
         * @brief changing the mapping name of a port to make it compatible 
         * with its model in the following of chaning its name
         * 
         * @param ports list of a hard block ports
         * @param port_index the index of a port that is to be changed
         *-------------------------------------------------------------------------------------------*/
        void rename_port_name(hard_block_ports* ports, char* const new_name, int port_index);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: create_hard_block)
         * 
         * @brief create a hard block model based on the given hard block port
         * 
         * @param name representing the name of a hard block
         * @param ports list of a hard block ports
         *-------------------------------------------------------------------------------------------*/
        hard_block_model* create_hard_block_model(const char* name, hard_block_ports* ports);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: create_multiple_inputs_one_output_port_model)
         * 
         * @brief create a model that has multiple input ports and one output port.
         * port sizes will be specified based on the number of pins in the BLIF file
         * 
         * @param name representing the name of a hard block
         * @param ports list of a hard block ports
         *-------------------------------------------------------------------------------------------*/
        hard_block_model* create_multiple_inputs_one_output_port_model(const char* name, hard_block_ports* ports);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: harmonize_mappings_with_model_pin_names)
         * 
         * @brief makes the mappings compatible with the actual model port names
         * 
         * @param model hard block model
         * @param mappings instance ports names read from the blif file
         *-------------------------------------------------------------------------------------------*/
        void harmonize_mappings_with_model_pin_names(hard_block_model* model, char** mappings);

};

#endif