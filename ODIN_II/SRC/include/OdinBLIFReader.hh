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

#ifndef __ODIN_BLIF_READER_H__
#define __ODIN_BLIF_READER_H__

#include "BLIF.hh"

/**
 * @brief A class to provide the general object of an input BLIF file reader
*/
class OdinBLIFReader : public BLIF::Reader {

    public:
        /**
         * @brief Construct the BLIFReader object
         * required by compiler
         */
        OdinBLIFReader();
        /**
         * @brief Destruct the BLIFReader object
         * to avoid memory leakage
         */
        ~OdinBLIFReader();

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
        * @param models list of hard block models
        *-------------------------------------------------------------------------------------------*/
        void create_hard_block_nodes(hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function:create_internal_node_and_driver)
         * 
         * @brief to create an internal node and driver from that node
         * 
         * @param name_prefix
         *-------------------------------------------------------------------------------------------*/
        void create_internal_node_and_driver();
        /**
         *---------------------------------------------------------------------------------------------
        * (function: build_top_input_node)
        * 
        * @brief to build a the top level input
        * 
        * @param name_str representing the name input signal
        *-------------------------------------------------------------------------------------------*/
        void build_top_input_node(const char* name_str);
        /**
         * (function: add_top_input_nodes)
         * 
         * @brief to add the top level inputs to the netlist
         *
         *-------------------------------------------------------------------------------------------*/
        void add_top_input_nodes();
        /**
         *---------------------------------------------------------------------------------------------
        * (function: rb_create_top_output_nodes)
        * 
        * @brief to add the top level outputs to the netlist
        * 
        *-------------------------------------------------------------------------------------------*/
        void rb_create_top_output_nodes();
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
        hard_block_model* read_hard_block_model(char* name_subckt, hard_block_ports* ports);

};

#endif