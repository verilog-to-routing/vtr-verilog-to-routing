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
#ifndef SIMULATE_BLIF_H
#define SIMULATE_BLIF_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unordered_map>

#define SIM_WAVE_LENGTH 16
#define BUFFER_MAX_SIZE 1024

#include <queue>
#include "sim_block.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "odin_types.h"
#include "memories.h"

/*
 * Number of values to store for each pin at one time.
 * Determines how frequently we have to write to disk.
 */
#define INPUT_VECTOR_FILE_NAME "input_vectors"
#define OUTPUT_VECTOR_FILE_NAME "output_vectors"
#define OUTPUT_ACTIVITY_FILE_NAME "output_activity"
#define MODEL_SIM_FILE_NAME "test.do"

#define DEFAULT_CLOCK_NAME "GLOBAL_SIM_BASE_CLK"

struct line_t {
    int number_of_pins;
    int max_number_of_pins;
    npin_t** pins;
    int* pin_numbers;
    char* name;
    int type;
};

struct lines_t {
    line_t** lines;
    int* pin_numbers;
    int count;
};

struct stages_t {
    nnode_t*** stages; // Stages.
    int* counts;       // Number of nodes in each stage.
    int count;         // Number of stages.

    // Statistics.
    int num_nodes;           // The total number of nodes.
    int num_connections;     // The sum of the number of children found under every node.
    int* num_children;       // Number of children per stage.
    double avg_worker_count; // The raio of node to be computed in parallel.

    double times;
    int worker_const;
    int worker_temp;
    bool warned;
};

//maria
struct netlist_subset {
    nnode_t** nodes;
    int number_of_nodes;
};

//maria
struct thread_node_distribution {
    netlist_subset** thread_nodes;
    int number_of_threads;
    netlist_subset* memory_nodes; //pointers to memory nodes
};

struct test_vector {
    BitSpace::bit_value_t** values;
    int* counts;
    int count;
};

struct sim_data_t {
    //maria
    lines_t** input_lines_per_wave;
    lines_t** output_lines_per_wave;

    // Create and verify the lines.
    lines_t* input_lines;
    lines_t* output_lines;
    FILE* out;
    FILE* in_out;
    FILE* act_out;
    FILE* modelsim_out;
    FILE* in = NULL;
    long num_vectors;

    double total_time;      // Includes I/O
    double simulation_time; // Does not include I/O

    stages_t* stages;
    //maria
    thread_node_distribution* thread_distribution; //nodes distributed to threads for parallel calculations

    // Parse -L and -H options containing lists of pins to hold high or low during random vector generation.
    std::unordered_map<std::string, short> held_at;

    int num_waves;

    netlist_t* netlist;
};

void simulate_netlist(netlist_t* netlist);

/*these are called by simulate_netlist*/
sim_data_t* init_simulation(netlist_t* netlist);
sim_data_t* terminate_simulation(sim_data_t* sim_data);
int single_step(sim_data_t* sim_data, int wave);
void simulate_steps(sim_data_t* sim_data, double min_coverage);

nnode_t** get_children_of(nnode_t* node, int* count);
nnode_t** get_children_of_nodepin(nnode_t* node, int* num_children, int output_pin);

BitSpace::bit_value_t get_pin_value(npin_t* pin, int cycle);

int get_clock_ratio(nnode_t* node);
void set_clock_ratio(int rat, nnode_t* node);

#endif
