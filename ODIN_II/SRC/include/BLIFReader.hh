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

#ifndef __BLIF_READER_H__
#define __BLIF_READER_H__

#include "GenericReader.hh"


#include "odin_types.h"
#include "Hashtable.hpp"

#define TOKENS " \t\n"
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

class OdinBLIFReader;
class SubcktBLIFReader;

extern int line_count;
extern int num_lines;
extern bool skip_reading_bit_map;
extern bool insert_global_clock;

extern FILE* file;

/**
 * @brief A class to provide the general object of an input BLIF file reader
*/
class BLIFReader : public GenericReader {

    public:
        /**
         * @brief Construct the BLIFReader object
         * required by compiler
         */
        BLIFReader();
        /**
         * @brief Destruct the BLIFReader object
         * to avoid memory leakage
         */
        ~BLIFReader();

        void* read();

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
        void build_top_input_node(const char* name_prefix, const char* name_str);
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
        int verify_hard_block_ports_against_model(hard_block_ports* ports, hard_block_model* model);
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
        /*
         * ---------------------------------------------------------------------------------------------
         * function: Creates the drivers for the top module
         * Top module is :
         * Special as all inputs are actually drivers.
         * Also make the 0 and 1 constant nodes at this point.
         * ---------------------------------------------------------------------------------------------
         */
        static void rb_create_top_driver_nets(const char* instance_name_prefix);
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: look_for_clocks)
         * ---------------------------------------------------------------------------------------------
         */
        static void rb_look_for_clocks();
        /**
         * ---------------------------------------------------------------------------------------------
         * (function: dum_parse)
         * ---------------------------------------------------------------------------------------------
         */
        static void dum_parse(char* buffer);
        /**
         * ---------------------------------------------------------------------------------------------
         * function:assign_node_type_from_node_name(char *)
         * This function tries to assign the node->type by looking at the name
         * Else return GENERIC
         * function will decide the node->type of the given node
         * ---------------------------------------------------------------------------------------------
         */
        static operation_list assign_node_type_from_node_name(char* output_name);
        /**
         * ---------------------------------------------------------------------------------------------
         * function: read_bit_map_find_unknown_gate
         * read the bit map for simulation
         * ---------------------------------------------------------------------------------------------
         */
        static operation_list read_bit_map_find_unknown_gate(int input_count, nnode_t* node);
        /**
         * ---------------------------------------------------------------------------------------------
         * function:create_latch_node_and_driver
         * to create an ff node and driver from that node
         * format .latch <input> <output> [<type> <control/clock>] <initial val>
         * ---------------------------------------------------------------------------------------------
         */
        static void create_latch_node_and_driver();
        /**
         * ---------------------------------------------------------------------------------------------
         * function: search_clock_name
         * to search the clock if the control in the latch
         * is not mentioned
         * ---------------------------------------------------------------------------------------------
         */
        static char* search_clock_name();
        /**
         * ---------------------------------------------------------------------------------------------
         * Gets the text in the given string which occurs
         * before the first instance of "[". The string is
         * presumably of the form "port[pin_number]"
         *
         * The retuned string is strduped and must be freed.
         * The original string is unaffected.
         * ---------------------------------------------------------------------------------------------
         */
        static char* get_hard_block_port_name(char* name);
        /**
         * ---------------------------------------------------------------------------------------------
         * Parses a port name of the form port[pin_number]
         * and returns the pin number as a long. Returns -1
         * if there is no [pin_number] in the name. Throws an
         * error if pin_number is not parsable as a long.
         *
         * The original string is unaffected.
         * ---------------------------------------------------------------------------------------------
         */
        static long get_hard_block_pin_number(char* original_name);
        /**
         * ---------------------------------------------------------------------------------------------
         * Callback function for qsort which compares pin names
         * of the form port_name[pin_number] primarily
         * on the port_name, and on the pin_number if the port_names
         * are identical.
         * ---------------------------------------------------------------------------------------------
         */
        static int compare_hard_block_pin_names(const void* p1, const void* p2);
        /**
        * ---------------------------------------------------------------------------------------------
        * Organises the given strings representing pin names on a hard block
        * model into ports, and indexes the ports by name. Returns the organised
        * ports as a hard_block_ports struct.
        * ---------------------------------------------------------------------------------------------
        */
        static hard_block_ports* get_hard_block_ports(char** pins, int count);
        /**
        * ---------------------------------------------------------------------------------------------
        * Creates a hashtable index for an array of strings of
        * the form names[i]=>i.
        * ---------------------------------------------------------------------------------------------
        */
        static Hashtable* index_names(char** names, int count);
        /**
        * ---------------------------------------------------------------------------------------------
        * Create an associative index of names1[i]=>names2[i]
        * ---------------------------------------------------------------------------------------------
        */
        static Hashtable* associate_names(char** names1, char** names2, int count);
        /**
        * ---------------------------------------------------------------------------------------------
        * Looks up a hard block model by name. Returns null if the
        * model is not found.
        * ---------------------------------------------------------------------------------------------
        */
        static hard_block_model* get_hard_block_model(char* name, hard_block_ports* ports, hard_block_models* models);
        /**
        * ---------------------------------------------------------------------------------------------
        * Adds the given model to the hard block model cache.
        * ---------------------------------------------------------------------------------------------
        */
        static void add_hard_block_model(hard_block_model* m, hard_block_ports* ports, hard_block_models* models);
        /**
        * ---------------------------------------------------------------------------------------------
        * Generates string which represents the geometry of the given hard block ports.
        * ---------------------------------------------------------------------------------------------
        */
        static char* generate_hard_block_ports_signature(hard_block_ports* ports);
        /**
        * ---------------------------------------------------------------------------------------------
        * Creates a new hard block model cache.
        * ---------------------------------------------------------------------------------------------
        */
        static hard_block_models* create_hard_block_models();
        /**
        * ---------------------------------------------------------------------------------------------
        * Counts the number of lines in the given blif file
        * before a .end token is hit.
        * ---------------------------------------------------------------------------------------------
        */
        static int count_blif_lines();
        /**
        * ---------------------------------------------------------------------------------------------
        * Frees the hard block model cache, freeing
        * all encapsulated hard block models.
        * ---------------------------------------------------------------------------------------------
        */
        static void free_hard_block_models(hard_block_models* models);
        /**
        * ---------------------------------------------------------------------------------------------
        * Frees a hard_block_model.
        * ---------------------------------------------------------------------------------------------
        */
        static void free_hard_block_model(hard_block_model* model);
        /**
        * ---------------------------------------------------------------------------------------------
        * Frees hard_block_pins
        * ---------------------------------------------------------------------------------------------
        */
        static void free_hard_block_pins(hard_block_pins* p);
        /**
        * ---------------------------------------------------------------------------------------------
        * Frees hard_block_ports
        * ---------------------------------------------------------------------------------------------
        */
        static void free_hard_block_ports(hard_block_ports* p);

    private:
        OdinBLIFReader* odin_blif_reader;
        SubcktBLIFReader* subckt_blif_reader;
        /**
         * [TODO]
         * EBLIFReader* eblif_reader();
        */
};

#endif