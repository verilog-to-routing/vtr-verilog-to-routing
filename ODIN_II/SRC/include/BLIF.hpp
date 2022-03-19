/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
 * 
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
 *
 * @file This file includes the definition of the basic structure used 
 * in Odin-II BLIF class to parse a BLIF file. Moreover, it provides
 * the declaration of BLIF class routines
 *
 */

#ifndef __BLIF_H__
#define __BLIF_H__

#include "GenericReader.hpp"
#include "GenericWriter.hpp"

#include "config_t.h"
#include "odin_types.h"
#include "Hashtable.hpp"

#include "vtr_util.h"
#include "vtr_memory.h"

#define TOKENS " \t\n"
#define YOSYS_TOKENS "[]"
#define YOSYS_ID_FIRST_DELIMITER "\\\\"
#define YOSYS_ID_LAST_DELIMITER "\""
#define YOSYS_TOKENS "[]"
#define GND_NAME "gnd"
#define VCC_NAME "vcc"
#define HBPAD_NAME "unconn"

#define READ_BLIF_BUFFER 1048576 // 1MB

// Stores pin names of the form port[pin]
struct hard_block_pins {
    int count;
    char** names;
    // Maps name to index.
    Hashtable* index;
};

// Stores port names, and their sizes.
struct hard_block_ports {
    char* signature;
    int count;
    int* sizes;
    char** names;
    // Maps portname to index.
    Hashtable* index;
};

// Stores all information pertaining to a hard block model. (.model)
struct hard_block_model {
    char* name;

    hard_block_pins* inputs;
    hard_block_pins* outputs;

    hard_block_ports* input_ports;
    hard_block_ports* output_ports;
};

// A cache structure for models.
struct hard_block_models {
    hard_block_model** models;
    int count;
    // Maps name to model
    Hashtable* index;
};

extern int line_count;
extern int num_lines;
extern bool skip_reading_bit_map;
extern bool insert_global_clock;

extern FILE* file;

/**
 *---------------------------------------------------------------------------------------------
 * (function: getbline)
 * 
 * @brief reading blif file lines according to the blif file type.
 * For instance, in yosys blif we must read lines since there is 
 * no '\' usage. For arbitrary long lines fgets does not work due
 * to size limit. However, the scenario in Odin is on the flipside
 * and we ought to use fgets to pass '\' and the line length are 
 * always fix and less than a const value
 * 
 * @param buf buffer pointer
 * @param size buffer size (in the case of using fgets)
 * @param fd file stream
 * 
 * @return null pointer if end of the file, otherwise the read line
 *---------------------------------------------------------------------------------------------*/
inline char* getbline(char*& buf, size_t size, FILE* fd) {
    char* retval = NULL;
    if (buf) {
        vtr::free(buf);
        buf = NULL;
    }
    /* decide whether need to read line or using fgets */
    if (configuration.coarsen) {
        retval = vtr::getline(buf, fd);
    } else {
        buf = (char*)vtr::malloc(READ_BLIF_BUFFER * sizeof(char));
        retval = vtr::fgets(buf, size, fd);
    }

    return (retval);
}

/**
 * @brief A class to provide the general object of an input BLIF file reader
 */
class BLIF {
  public:
    /**
     * @brief Construct the Reader object
     * required by compiler
     */
    BLIF();
    /**
     * @brief Destruct the Reader object
     * to avoid memory leakage
     */
    ~BLIF();

    class Reader : public GenericReader {
      public:
        /**
         * @brief Construct the Reader object
         * required by compiler
         */
        Reader();
        /**
         * @brief Destruct the Reader object
         * to avoid memory leakage
         */
        ~Reader();

        void* _read();

        /* No need to have writer in Generic Reader */
        void _write(const netlist_t* /* netlist */) {
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
         * -------------------------------------------------------------------------------------------
         */
        int read_tokens(char* buffer, hard_block_models* models);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: resolve_signal_name_based_on_blif_type)
         * 
         * @brief to find the name of top module
         * -------------------------------------------------------------------------------------------
         */
        static void find_top_module();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: hook_up_nets)
         * 
         * @brief find the output nets and add the corresponding nets
         * -------------------------------------------------------------------------------------------
         */
        void hook_up_nets();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: hook_up_node)
         * 
         * @brief Connect the given node's input pins to their corresponding nets by
         * looking each one up in the output_nets_sc.
         * 
         * @param node represents one of netlist internal nodes or ff nodes or top output nodes
         * ---------------------------------------------------------------------------------------------
         */
        void hook_up_node(nnode_t* node);
        /**
         *------------------------------------------------------------------------------------------- 
         * (function:create_hard_block_nodes)
         * 
         * @brief to create the hard block nodes
         * 
         * @param models list of hard block models
         * -------------------------------------------------------------------------------------------
         */
        void create_hard_block_nodes(hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function:create_internal_node_and_driver)
         * 
         * @brief to create an internal node and driver from that node
         * 
         * @param name_prefix
         * -------------------------------------------------------------------------------------------
         */
        void create_internal_node_and_driver();
        /**
         *---------------------------------------------------------------------------------------------
         * (function: build_top_input_node)
         * 
         * @brief to build a the top level input
         * 
         * @param name_str representing the name input signal
         * -------------------------------------------------------------------------------------------
         */
        void build_top_input_node(const char* name_str);
        /**
         * (function: add_top_input_nodes)
         * 
         * @brief to add the top level inputs to the netlist
         *
         * -------------------------------------------------------------------------------------------
         */
        void add_top_input_nodes();
        /**
         *---------------------------------------------------------------------------------------------
         * (function: rb_create_top_output_nodes)
         * 
         * @brief to add the top level outputs to the netlist
         * 
         * -------------------------------------------------------------------------------------------
         */
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
         * -------------------------------------------------------------------------------------------
         */
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
         * -------------------------------------------------------------------------------------------
         */
        hard_block_model* read_hard_block_model(char* name_subckt, operation_list type, hard_block_ports* ports);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: rb_create_top_driver_nets)
         *
         * @brief Creates the drivers for the top module
         * Special as all inputs are actually drivers.
         * Also make the 0 and 1 constant nodes at this point.
         *
         * @param instance_name_prefix the prefix of the instance name
         * ---------------------------------------------------------------------------------------------
         */
        static void rb_create_top_driver_nets(const char* instance_name_prefix);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: rb_look_for_clocks)
         *
         * @brief looking for clock nodes in the blif file by 
         * going through tthe clock driver of ff nodes
         * ---------------------------------------------------------------------------------------------
         */
        static void rb_look_for_clocks();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: dum_parse)
         *
         * @brief continue reading file to the end
         * ---------------------------------------------------------------------------------------------
         */
        static void dum_parse(char* buffer);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function:assign_node_type_from_node_name)
         *
         * @brief This function tries to assign the node->type by looking at the name
         * Else return GENERIC. function will decide the node->type of the given node
         *
         * @param output_name the node name
         * ---------------------------------------------------------------------------------------------
         */
        static operation_list assign_node_type_from_node_name(char* output_name);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: read_bit_map_find_unknown_gate)
         * 
         * @brief read the bit map for simulation
         *
         * @param input_count number of inputs
         * @param node pointer to the netlist node
         * @param names list of node inputs
         * ---------------------------------------------------------------------------------------------
         */
        static operation_list read_bit_map_find_unknown_gate(int input_count, nnode_t* node, char** names);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: create_latch_node_and_driver)
         * 
         * @brief to create an ff node and driver from that node format
         * .latch <input> <output> [<type> <control/clock>] <initial val>
         * ---------------------------------------------------------------------------------------------
         */
        static void create_latch_node_and_driver();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: search_clock_name)
         * 
         * @brief to search the clock if the control in the latch
         * is not mentioned
         * ---------------------------------------------------------------------------------------------
         */
        static char* search_clock_name();
        /**
         * ---------------------------------------------------------------------------------------------
         * (funtion: get_hard_block_port_name)
         *
         * @brief Gets the text in the given string which occurs
         * before the first instance of "[". The string is
         * presumably of the form "port[pin_number]"
         *
         * The retuned string is strduped and must be freed.
         * The original string is unaffected.
         *
         * @param name the cstring of the port name
         * ---------------------------------------------------------------------------------------------
         */
        static char* get_hard_block_port_name(char* name);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: get_hard_block_pin_number)
         * 
         * @brief Parses a port name of the form port[pin_number]
         * and returns the pin number as a long. Returns -1
         * if there is no [pin_number] in the name. Throws an
         * error if pin_number is not parsable as a long.
         * The original string is unaffected.
         *
         * @param original_name
         * ---------------------------------------------------------------------------------------------
         */
        static long get_hard_block_pin_number(char* original_name);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: compare_hard_block_pin_names)
         *
         * @brief Callback function for qsort which compares pin names
         * of the form port_name[pin_number] primarily
         * on the port_name, and on the pin_number if the port_names
         * are identical.
         *
         * @param p1 pin name 1
         * @param p2 pin name 2
         * ---------------------------------------------------------------------------------------------
         */
        static int compare_hard_block_pin_names(const void* p1, const void* p2);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: get_hard_block_ports)
         * 
         * @brief Organises the given strings representing pin names on a hard block
         * model into ports, and indexes the ports by name. 
         *
         * @param pins list of hard block pins
         * @param count number of hard block pins
         *
         * @return the organised ports as a hard_block_ports struct.
         * ---------------------------------------------------------------------------------------------
         */
        static hard_block_ports* get_hard_block_ports(char** pins, int count);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: index_names)
         *
         * @brief Creates a hashtable index for an array of strings of
         * the form names[i]=>i.
         *
         * @param names the list of names
         * @param count the number of names
         * ---------------------------------------------------------------------------------------------
         */
        static Hashtable* index_names(char** names, int count);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: associate_names)
         *
         * @brief Create an associative index of names1[i]=>names2[i]
         * NOTE: the size of both lists should be the same
         *
         * @param names1 the first list of names
         * @param names2 the second list of names
         * @param count the number of names
         * ---------------------------------------------------------------------------------------------
         */
        static Hashtable* associate_names(char** names1, char** names2, int count);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: get_hard_block_model)
         *
         * @brief Looks up a hard block model by name. Returns null if the
         * model is not found.
         *
         * @param name hard block model name
         * @param ports list of the hard block's ports
         * @param models list of all hard block models
         * ---------------------------------------------------------------------------------------------
         */
        static hard_block_model* get_hard_block_model(char* name, hard_block_ports* ports, hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: add_hard_block_model)
         *
         * @brief Adds the given model to the hard block model cache.
         *
         * @param m the hard block model
         * @param ports list of the hard block's ports
         * @param models list of all hard block models
         * ---------------------------------------------------------------------------------------------
         */
        static void add_hard_block_model(hard_block_model* m, hard_block_ports* ports, hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: generate_hard_block_ports_signature)
         *
         * @brief Generates string which represents the geometry of the given hard block ports.
         *
         * @param ports list of the hard block's ports
         * ---------------------------------------------------------------------------------------------
         */
        static char* generate_hard_block_ports_signature(hard_block_ports* ports);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: create_hard_block_models)
         * 
         * @brief Creates a new hard block model cache.
         * ---------------------------------------------------------------------------------------------
         */
        static hard_block_models* create_hard_block_models();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: count_blif_lines)
         * 
         * @brief Counts the number of lines in the given blif file
         * before a .end token is hit.
         * ---------------------------------------------------------------------------------------------
         */
        static int count_blif_lines();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: free_hard_block_models)
         *
         * @brief Frees the hard block model cache, freeing
         * all encapsulated hard block models.
         *
         * @param models list of all hard block models
         * ---------------------------------------------------------------------------------------------
         */
        static void free_hard_block_models(hard_block_models* models);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: free_hard_block_model)
         *
         * @brief Frees a hard_block_model.
         * 
         * @param models list of all hard block models
         * ---------------------------------------------------------------------------------------------
         */
        static void free_hard_block_model(hard_block_model* model);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: free_hard_block_pins)
         *
         * @brief Frees hard_block_pins
         *
         * @param p list of hard block pins
         * ---------------------------------------------------------------------------------------------
         */
        static void free_hard_block_pins(hard_block_pins* p);
        /**
         * ---------------------------------------------------------------------------------------------
         * (free_hard_block_ports)
         *
         * @brief Frees hard_block_ports
         *
         * @param p list of hard block ports
         * ---------------------------------------------------------------------------------------------
         */
        static void free_hard_block_ports(hard_block_ports* p);

      private:
        /**
         *---------------------------------------------------------------------------------------------
         * (function: resolve_signal_name_based_on_blif_type)
         * 
         * @brief to make the signal names of an input blif 
         * file compatible with the odin's name convention
         * 
         * @param name_str representing the name input signal
         * -------------------------------------------------------------------------------------------
         */
        static char* resolve_signal_name_based_on_blif_type(const char* name_prefix, const char* name_str);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: create_hard_block)
         * 
         * @brief create a hard block model based on the given hard block port
         * 
         * @param name representing the name of a hard block
         * @param ports list of a hard block ports
         * -------------------------------------------------------------------------------------------
         */
        hard_block_model* create_hard_block_model(const char* name, operation_list type, hard_block_ports* ports);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: create_model)
         * 
         * @brief create a model that has multiple input ports and one output port.
         * port sizes will be specified based on the number of pins in the BLIF file
         * 
         * 
         * @param name representing the name of a hard block
         * @param ports list of a hard block ports
         * @param output_idx_START showing the beginning idx of output ports
         * @param output_idx_END showing the end idx of output ports
         * -------------------------------------------------------------------------------------------
         */
        hard_block_model* create_model(const char* name, hard_block_ports* ports, int output_idx_START, int output_idx_END);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: hard_block_sensitivities)
         * 
         * @brief specify whether a type needs clock sensitivity or not
         * 
         * @param subckt_name hard block name
         * @param new_node pointer to the netlist node
         * -------------------------------------------------------------------------------------------
         */
        void hard_block_sensitivities(const char* subckt_name, nnode_t* new_node);
        /**
         *---------------------------------------------------------------------------------------------
         * (function: need_params)
         * 
         * @brief specify whether a type needs clock sensitivity or not
         * 
         * @param type node type
         * -------------------------------------------------------------------------------------------
         */
        bool need_params(operation_list type);
    };

    class Writer : public GenericWriter {
      public:
        /**
         * @brief Construct the Writer object
         * required by compiler
         */
        Writer();
        /**
         * @brief Destruct the Writer object
         * to avoid memory leakage
         */
        ~Writer();

        /* No need to have reader in Generic Writer */
        void* _read() {
            error_message(UTIL, unknown_location, "%s is not available in Generic Writer\n", __PRETTY_FUNCTION__);
            return NULL;
        }

        void _write(const netlist_t* netlist);
        void _create_file(const char* file_name, const file_type_e file_type = _BLIF);

      protected:
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: warn_undriven)
         * 	to check is a net undriven
         *---------------------------------------------------------------------------------------------
         */
        static bool warn_undriven(nnode_t* node, nnet_t* net);
        // TODO Uncomment this for In Outs
        //static void merge_with_inputs(nnode_t* node, long pin_idx);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: print_net_driver)
         * 	The function that prints the driver of a given net
         *---------------------------------------------------------------------------------------------
         */
        static void print_net_driver(FILE* out, nnode_t* node, nnet_t* net, long driver_idx);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: print_input_single_driver)
         * 	The function that prints the single input pin driver 
         *---------------------------------------------------------------------------------------------
         */
        static void print_input_single_driver(FILE* out, nnode_t* node, long pin_idx);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: print_output_pin)
         * 	The function that prints the output pins of a given node
         *---------------------------------------------------------------------------------------------
         */
        static void print_output_pin(FILE* out, nnode_t* node);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: print_dot_names_header)
         * 	The function that prints .model, .inputs, .outputs, etc.
         *---------------------------------------------------------------------------------------------
         */
        static void print_dot_names_header(FILE* out, nnode_t* node);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: create_blif)
         * 	The function that creates the output blif file
         *---------------------------------------------------------------------------------------------
         */
        FILE* create_blif(const char* file_name);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: output_blif)
         * 	The function that prints out the details for a blif formatted file
         *---------------------------------------------------------------------------------------------
         */
        void output_blif(FILE* out, const netlist_t* netlist);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: depth_first_traversal_to_parital_map()
         * ---------------------------------------------------------------------------------------------
         */
        void depth_first_traversal_to_output(short marker_value, FILE* fp, const netlist_t* netlist);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: depth_first_traverse)
         * ---------------------------------------------------------------------------------------------
         */
        void depth_traverse_output_blif(nnode_t* node, uintptr_t traverse_mark_number, FILE* fp);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: output_node)
         * 	Depending on node type, figures out what to print for this node
         * ---------------------------------------------------------------------------------------------
         */
        void output_node(nnode_t* node, short /*traverse_number*/, FILE* fp);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: define_logical_function)
         * ---------------------------------------------------------------------------------------------
         */
        void define_logical_function(nnode_t* node, FILE* out);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: define_set_input_logical_function)
         * ---------------------------------------------------------------------------------------------
         */
        void define_set_input_logical_function(nnode_t* node, const char* bit_output, FILE* out);
        /** 
         * ---------------------------------------------------------------------------------------------
         * (function: define_clock)
         * ---------------------------------------------------------------------------------------------
         */
        void define_clock(nnode_t* node, FILE* out);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: define_ff)
         * ---------------------------------------------------------------------------------------------
         */
        void define_ff(nnode_t* node, FILE* out);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: define_decoded_mux)
         * ---------------------------------------------------------------------------------------------
         */
        void define_decoded_mux(nnode_t* node, FILE* out);

      private:
        bool haveOutputLatchBlackbox;
    };
};

#endif //__BLIF_H__
