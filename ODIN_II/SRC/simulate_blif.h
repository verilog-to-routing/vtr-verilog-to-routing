/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef SIMULATE_BLIF_H
#define SIMULATE_BLIF_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/time.h>

#define SIM_WAVE_LENGTH 16
#define BUFFER_MAX_SIZE 1024

#include "queue.h"
#include "hashtable.h"
#include "sim_block.h"
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "outputs.h"
#include "util.h"
#include "multipliers.h"
#include "hard_blocks.h"
#include "types.h"
#include "memories.h"

/*
 * Number of values to store for each pin at one time.
 * Determines how frequently we have to write to disk.
 */
#define INPUT_VECTOR_FILE_NAME "input_vectors"
#define OUTPUT_VECTOR_FILE_NAME "output_vectors"

#define SINGLE_PORT_MEMORY_NAME "single_port_ram"
#define DUAL_PORT_MEMORY_NAME "dual_port_ram"

typedef struct {
	char **pins;
	int   count;
} pin_names;

typedef struct {
	int number_of_pins;
	int max_number_of_pins;
	npin_t **pins;
	int  *pin_numbers;
	char *name;
	int type;
} line_t;

typedef struct {
	line_t **lines;
	int    *pin_numbers;
	int    count;
} lines_t;

typedef struct {
	nnode_t ***stages; // Stages.
	int       *counts; // Number of nodes in each stage.
	int 	   count;  // Number of stages.
	double *sequential_times; // Sequential execution time values for each stage for tuning.
	double *parallel_times;   // Parallel execution time values for each stage for tuning.

	// Statistics.
	int    num_nodes;          // The total number of nodes.
	int    num_connections;    // The sum of the number of children found under every node.
	int    *num_children;      // Number of children per stage.
	int    num_parallel_nodes; // The number of nodes while will be computed in parallel.
} stages;

typedef struct {
	signed char  **values;
	int           *counts;
	int            count;
} test_vector;

void simulate_netlist(netlist_t *netlist);
void simulate_cycle(int cycle, stages *s);
stages *simulate_first_cycle(netlist_t *netlist, int cycle, pin_names *p, lines_t *output_lines);

stages *stage_ordered_nodes(nnode_t **ordered_nodes, int num_ordered_nodes);
void free_stages(stages *s);

int get_num_covered_nodes(stages *s);
int get_clock_ratio(nnode_t *node);
void set_clock_ratio(int rat, nnode_t *node);
nnode_t **get_children_of(nnode_t *node, int *count);
int *get_children_pinnumber_of(nnode_t *node, int *num_children);
nnode_t **get_children_of_nodepin(nnode_t *node, int *count, int output_pin);
int is_node_ready(nnode_t* node, int cycle);
int is_node_complete(nnode_t* node, int cycle);
int enqueue_node_if_ready(queue_t* queue, nnode_t* node, int cycle);

void compute_and_store_value(nnode_t *node, int cycle);
void compute_memory_node(nnode_t *node, int cycle);
void compute_hard_ip_node(nnode_t *node, int cycle);
void compute_multiply_node(nnode_t *node, int cycle);
void compute_generic_node(nnode_t *node, int cycle);
void compute_add_node(nnode_t *node, int cycle, int type);
void compute_unary_sub_node(nnode_t *node, int cycle);


void update_pin_value(npin_t *pin, signed char value, int cycle);
signed char get_pin_value(npin_t *pin, int cycle);

inline int get_values_offset(int cycle);

inline int get_pin_cycle(npin_t *pin);
inline void set_pin_cycle(npin_t *pin, int cycle);
void initialize_pin(npin_t *pin);

int is_even_cycle(int cycle);
inline int is_clock_node(nnode_t *node);

signed char get_line_pin_value(line_t *line, int pin_num, int cycle);
int line_has_unknown_pin(line_t *line, int cycle);

void compute_flipflop_node(nnode_t *node, int cycle);
void compute_mux_2_node(nnode_t *node, int cycle);

int *multiply_arrays(int *a, int a_length, int *b, int b_length);

int *add_arrays(int *a, int a_length, int *b, int b_length, int *c, int c_length, int flag);

int *unary_sub_arrays(int *a, int a_length, int *c, int c_length);

void compute_single_port_memory(nnode_t *node, int cycle);
void compute_dual_port_memory(nnode_t *node, int cycle);

long compute_memory_address(signal_list_t *addr, int cycle);

void instantiate_memory(nnode_t *node, int data_width, int addr_width);
char *get_mif_filename(nnode_t *node);
FILE *preprocess_mif_file(FILE *source);
void assign_memory_from_mif_file(FILE *file, char *filename, int width, long depth, signed char *memory);
int parse_mif_radix(char *radix);

int count_test_vectors(FILE *in);
int is_vector(char *buffer);
int get_next_vector(FILE *file, char *buffer);
test_vector *parse_test_vector(char *buffer);
test_vector *generate_random_test_vector(lines_t *l, int cycle, hashtable_t *hold_high_index, hashtable_t *hold_low_index);
int compare_test_vectors(test_vector *v1, test_vector *v2);

int verify_test_vector_headers(FILE *in, lines_t *l);
void free_test_vector(test_vector* v);

line_t *create_line(char *name);
int verify_lines(lines_t *l);
void free_lines(lines_t *l);

int find_portname_in_lines(char* port_name, lines_t *l);
lines_t *create_lines(netlist_t *netlist, int type);

void add_test_vector_to_lines(test_vector *v, lines_t *l, int cycle);
void assign_node_to_line(nnode_t *node, lines_t *l, int type, int single_pin);
void insert_pin_into_line(npin_t *pin, int pin_number, line_t *line, int type);

char *generate_vector_header(lines_t *l);
void write_vector_headers(FILE *file, lines_t *l);

void write_vector_to_file(lines_t *l, FILE *file, int cycle);
void write_wave_to_file(lines_t *l, FILE* file, int cycle_offset, int wave_length, int both_edges);

void write_vector_to_modelsim_file(lines_t *l, FILE *modelsim_out, int cycle);
void write_wave_to_modelsim_file(netlist_t *netlist, lines_t *l, FILE* modelsim_out, int cycle_offset, int wave_length);

int verify_output_vectors(char* output_vector_file, int num_test_vectors);

void add_additional_items_to_lines(nnode_t *node, pin_names *p, lines_t *l);
pin_names *parse_pin_name_list(char *list);
void free_pin_name_list(pin_names *p);
hashtable_t *index_pin_name_list(pin_names *list);

void trim_string(char* string, char *chars);
char *vector_value_to_hex(signed char *value, int length);

int  print_progress_bar(double completion, int position, int length, double time);
void print_netlist_stats(stages *stages, int num_vectors);
void print_simulation_stats(stages *stages, int num_vectors, double total_time, double simulation_time);
void print_time(double time);

double wall_time();

char *get_circuit_filename();

void update_undriven_input_pins(nnode_t *node, int cycle);
void flag_undriven_input_pins(nnode_t *node);

void print_ancestry(nnode_t *node, int generations);
nnode_t *print_update_trace(nnode_t *bottom_node, int cycle);

int is_posedge(npin_t *pin, int cycle);


#endif

