#ifndef READ_BLIF_H
#define READ_BLIF_H

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

netlist_t* read_blif();
extern int line_count;

extern bool skip_reading_bit_map;
extern bool insert_global_clock;

void rb_create_top_driver_nets(const char* instance_name_prefix, Hashtable* output_nets_hash);
void rb_look_for_clocks(); // not sure if this is needed
void add_top_input_nodes(FILE* file, Hashtable* output_nets_hash);
void rb_create_top_output_nodes(FILE* file);
int read_tokens(char* buffer, hard_block_models* models, FILE* file, Hashtable* output_nets_hash);
void dum_parse(char* buffer, FILE* file);
void create_internal_node_and_driver(FILE* file, Hashtable* output_nets_hash);
operation_list assign_node_type_from_node_name(char* output_name); // function will decide the node->type of the given node
operation_list read_bit_map_find_unknown_gate(int input_count, nnode_t* node, FILE* file);
void create_latch_node_and_driver(FILE* file, Hashtable* output_nets_hash);
void create_hard_block_nodes(hard_block_models* models, FILE* file, Hashtable* output_nets_hash);
void hook_up_nets(Hashtable* output_nets_hash);
void hook_up_node(nnode_t* node, Hashtable* output_nets_hash);
char* search_clock_name(FILE* file);
void free_hard_block_model(hard_block_model* model);
char* get_hard_block_port_name(char* name);
long get_hard_block_pin_number(char* original_name);
int compare_hard_block_pin_names(const void* p1, const void* p2);
hard_block_ports* get_hard_block_ports(char** pins, int count);
Hashtable* index_names(char** names, int count);
Hashtable* associate_names(char** names1, char** names2, int count);
void free_hard_block_pins(hard_block_pins* p);
void free_hard_block_ports(hard_block_ports* p);

hard_block_model* get_hard_block_model(char* name, hard_block_ports* ports, hard_block_models* models);
void add_hard_block_model(hard_block_model* m, hard_block_ports* ports, hard_block_models* models);
char* generate_hard_block_ports_signature(hard_block_ports* ports);
int verify_hard_block_ports_against_model(hard_block_ports* ports, hard_block_model* model);
hard_block_model* read_hard_block_model(char* name_subckt, hard_block_ports* ports, FILE* file);

void free_hard_block_models(hard_block_models* models);

hard_block_models* create_hard_block_models();
int count_blif_lines(FILE* file);

#endif
