/*	number_of_workers = std::max(1, global_args.parralelized_simulation.value());

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
FILEs (the "Software"), to deal in the Software without
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
#include "simulate_blif.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include "vtr_util.h"
#include "vtr_memory.h"
#include "odin_util.h"
#include <string>
#include <sstream>
#include <chrono>
#include <dlfcn.h>
#include <mutex>
#include <unistd.h>
#include <thread>
//#include <cthreads.h>

//maria
#include <sys/sysinfo.h>

#define CLOCK_INITIAL_VALUE 1
#define MAX_REPEAT_SIM 128

static inline signed char init_value(nnode_t *node)
{
	return (node && node->has_initial_value)? node->initial_value: global_args.sim_initial_value;
}

typedef enum
{
	FALLING,
	RISING,
	HIGH,
	LOW,
	UNK
}edge_eval_e;

inline static edge_eval_e get_edge_type(npin_t *clk, int cycle)
{
	if(!clk)
		return UNK;
	signed char prev = !CLOCK_INITIAL_VALUE;
	signed char cur = CLOCK_INITIAL_VALUE;

	if(cycle > 0)
	{
		prev = get_pin_value(clk, cycle-1);
		cur = get_pin_value(clk, cycle);
	}
	return 	((prev != cur) && (prev == 0 || cur == 1))?	RISING:
			((prev != cur) && (prev == 1 || cur == 0))?	FALLING:
			(cur == 1)?									HIGH:
			(cur == 0)?									LOW:
														UNK;
}

inline static bool ff_trigger(edge_type_e type, npin_t* clk, int cycle)
{
	edge_eval_e clk_e = get_edge_type(clk, cycle);
	// update the flip-flop from the input value of the previous cycle.
	return (
			(type == FALLING_EDGE_SENSITIVITY	&&	clk_e == FALLING)
		||	(type == RISING_EDGE_SENSITIVITY	&&	clk_e == RISING)
		||	(type == ACTIVE_HIGH_SENSITIVITY	&&	clk_e == HIGH)
		||	(type == ACTIVE_LOW_SENSITIVITY		&&	clk_e == LOW)
		||	(type == ASYNCHRONOUS_SENSITIVITY	&&	(clk_e == RISING || clk_e == FALLING))
	);
}

inline static signed char get_D(npin_t* D, int cycle)
{
	return get_pin_value(D,cycle-1);
}

inline static signed char get_Q(npin_t* Q, int cycle)
{
	return get_pin_value(Q,cycle-1);
}

inline static signed char compute_ff(bool trigger, signed char D_val, signed char Q_val, int /*cycle*/)
{
	return (trigger) ? D_val: Q_val;
}
inline static signed char compute_ff(bool trigger, npin_t* D, signed char Q_val, int cycle)
{
	return compute_ff(trigger, get_D(D, cycle), Q_val, cycle);
}

inline static signed char compute_ff(bool trigger, signed char D_val, npin_t* Q, int cycle)
{
	return compute_ff(trigger, D_val, get_Q(Q,cycle), cycle);
}

inline static signed char compute_ff(bool trigger, npin_t* D, npin_t* Q, int cycle)
{
	return compute_ff(trigger, get_D(D,cycle), get_Q(Q,cycle), cycle);
}

static inline bool is_clock_node(nnode_t *node)
{
	return node && (
		(node->type == CLOCK_NODE) 
		|| (std::string(node->name) == "top^clk") // Strictly for memories.
		|| (std::string(node->name) == DEFAULT_CLOCK_NAME)
	);
}

//maria
static thread_node_distribution *calculate_thread_distribution(stages_t *s);
static void compute_and_store_part_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int cycle); //to remove
static void compute_and_store_part_wave_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int from_wave,int to_wave);
static void compute_and_store_part_in_waves_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int from_wave, int to_wave,int offset,bool /*notify_back*/);

static void simulate_cycle_multithreaded(int cycle, thread_node_distribution *thread_distribution);

static void simulate_cycle(int cycle, stages_t *s);
static stages_t *simulate_first_cycle(netlist_t *netlist, int cycle, lines_t *output_lines);

static stages_t *stage_ordered_nodes(nnode_t **ordered_nodes, int num_ordered_nodes);
static void free_stages(stages_t *s);

//maria
static void free_thread_distribution(thread_node_distribution *thread_distribution);

static int get_num_covered_nodes(stages_t *s);
static int *get_children_pinnumber_of(nnode_t *node, int *num_children);

//maria
static nnode_t **get_parents_of(nnode_t *node, int *num_parents);
static int is_node_ready(nnode_t* node, int cycle);
static int is_node_complete(nnode_t* node, int cycle);
static void write_back_memory_nodes(nnode_t **nodes, int num_nodes);

static bool compute_and_store_value(nnode_t *node, int cycle);
static void compute_memory_node(nnode_t *node, int cycle);
static void compute_hard_ip_node(nnode_t *node, int cycle);
static void compute_multiply_node(nnode_t *node, int cycle);
static void compute_generic_node(nnode_t *node, int cycle);
static void compute_add_node(nnode_t *node, int cycle, int type);
static void compute_unary_sub_node(nnode_t *node, int cycle);


static void update_pin_value(npin_t *pin, signed char value, int cycle);
static int get_pin_cycle(npin_t *pin);

signed char get_line_pin_value(line_t *line, int pin_num, int cycle);
static int line_has_unknown_pin(line_t *line, int cycle);

static void compute_flipflop_node(nnode_t *node, int cycle);
static void compute_mux_2_node(nnode_t *node, int cycle);

static int *multiply_arrays(int *a, int a_length, int *b, int b_length);

static int *add_arrays(int *a, int a_length, int *b, int b_length, int *c, int c_length, int flag);

static int *unary_sub_arrays(int *a, int a_length, int *c, int c_length);

static void compute_single_port_memory(nnode_t *node, int cycle);
static void compute_dual_port_memory(nnode_t *node, int cycle);

static long compute_memory_address(signal_list_t *addr, int cycle);

static void instantiate_memory(nnode_t *node, long data_width, long addr_width);
static char *get_mif_filename(nnode_t *node);
static FILE *preprocess_mif_file(FILE *source);
static void assign_memory_from_mif_file(nnode_t *node, FILE *mif, char *filename, int width, long address_width);
static int parse_mif_radix(char *radix);

static int count_test_vectors(FILE *in);
static int count_empty_test_vectors(FILE *in);

static int is_vector(char *buffer);
static int get_next_vector(FILE *file, char *buffer);
static test_vector *parse_test_vector(char *buffer);
static test_vector *generate_random_test_vector(int cycle, sim_data_t *sim_data);
static int compare_test_vectors(test_vector *v1, test_vector *v2);

static int verify_test_vector_headers(FILE *in, lines_t *l);
static void free_test_vector(test_vector* v);

static line_t *create_line(char *name);
static int verify_lines(lines_t *l);
static void free_lines(lines_t *l);

static int find_portname_in_lines(char* port_name, lines_t *l);
static lines_t *create_lines(netlist_t *netlist, int type);

static void add_test_vector_to_lines(test_vector *v, lines_t *l, int cycle);
static void assign_node_to_line(nnode_t *node, lines_t *l, int type, int single_pin);
static void insert_pin_into_line(npin_t *pin, int pin_number, line_t *line, int type);

static char *generate_vector_header(lines_t *l);
static void write_vector_headers(FILE *file, lines_t *l);

static void write_vector_to_file(lines_t *l, FILE *file, int cycle);
static void write_cycle_to_file(lines_t *l, FILE* file, int cycle);

static void write_vector_to_modelsim_file(lines_t *l, FILE *modelsim_out, int cycle);
static void write_cycle_to_modelsim_file(netlist_t *netlist, lines_t *l, FILE* modelsim_out, int cycle);

static int verify_output_vectors(char* output_vector_file, int num_test_vectors);

static void add_additional_items_to_lines(nnode_t *node, lines_t *l);

static char *vector_value_to_hex(signed char *value, int length);

static void print_netlist_stats(stages_t *stages, int num_vectors);
static void print_simulation_stats(stages_t *stages, int num_vectors, double total_time, double simulation_time);

static void flag_undriven_input_pins(nnode_t *node);

static void print_ancestry(nnode_t *node, int generations);
static nnode_t *print_update_trace(nnode_t *bottom_node, int cycle);

double used_time;
int number_of_workers;
bool batch_mode;
bool found_best_time;

int num_of_clock;

//maria TODO maybe not the best place?
pthread_cond_t start_threads,start_output;
pthread_mutex_t threads_mp,output_mp,main_mp;
int threads_done_wave = 0;
int threads_created = 0;
int threads_waves = 0;
int threads_start = 0;
int threads_end = 0;

/*
 * Performs simulation.
 */
void simulate_netlist(netlist_t *netlist)
{
	printf("Simulation starts \n");
	sim_data_t *sim_data = init_simulation(netlist);	
	printf("\n");

	//int       progress_bar_position = -1;
	//const int progress_bar_length   = 50;
	

	double min_coverage =0.0;
	if(global_args.sim_min_coverage)
	{
		min_coverage = global_args.sim_min_coverage/100;
	}
	else if(global_args.sim_achieve_best)
	{
		min_coverage = 0.0001;
	}
	if (number_of_workers>1 && global_args.parralelized_simulation_in_batch)
	{
		int start_cycle = 0;
		int end_cycle = sim_data->num_vectors;
		simulate_steps_in_parallel(sim_data,start_cycle,end_cycle,min_coverage);
	}
	else
	{
		simulate_steps_sequential(sim_data,min_coverage);
	}


	fflush(sim_data->out);
	fprintf(sim_data->modelsim_out, "run %ld\n", sim_data->num_vectors*100);

	printf("\n");
	// If a second output vector file was given via the -T option, verify that it matches.
	char *output_vector_file = global_args.sim_vector_output_file;
	if (output_vector_file)
	{
		if (verify_output_vectors(output_vector_file, sim_data->num_vectors))
			printf("Vector file \"%s\" matches output\n", output_vector_file);
		else
			error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Vector files differ.");
		printf("\n");
	}

	// Print statistics.
	print_simulation_stats(sim_data->stages, sim_data->num_vectors, sim_data->total_time, sim_data->simulation_time);
	// Perform ACE activity calculations
	calculate_activity ( netlist, sim_data->num_vectors, sim_data->act_out );
}



/* 
 * Performs simulation batches of cycles pass through the threads.
 */
// before
/* void simulate_netlist(netlist_t *netlist)
{
	sim_data_t *sim_data = init_simulation(netlist);	
	printf("\n");

	int       progress_bar_position = -1;
	const int progress_bar_length   = 50;
	
	int increment_vector_by = global_args.sim_num_test_vectors;;
	double min_coverage =0.0;
	if(global_args.sim_min_coverage)
	{
		min_coverage = global_args.sim_min_coverage/100;
	}
	else if(global_args.sim_achieve_best)
	{
		min_coverage = 0.0001;
	}	

	double current_coverage =0.0;
	int cycle =0;
	while(cycle < sim_data->num_vectors)
	{
		double wave_start_time = wall_time();

		// if we target a minimum coverage keep generating
		if(min_coverage > 0.0)
		{
			if(cycle+1 == sim_data->num_vectors)
			{
				current_coverage = (((double) get_num_covered_nodes(sim_data->stages) / (double) sim_data->stages->num_nodes));
				if(global_args.sim_achieve_best)
				{
					if(current_coverage > min_coverage)
					{
						increment_vector_by = global_args.sim_num_test_vectors;
						min_coverage = current_coverage;
						sim_data->num_vectors += increment_vector_by;
					}
					else if(increment_vector_by)
					{
						//slowly reduce the search until there is no more possible increment, this prevent building too large of a comparative vector pair
						sim_data->num_vectors += increment_vector_by;
						increment_vector_by /= 2;
					}

				}
				else
				{
					if(current_coverage < min_coverage)
						sim_data->num_vectors += increment_vector_by;
				}
			}
		}
		else
		{
			current_coverage = cycle/(double)sim_data->num_vectors;
		}

		single_step(sim_data, cycle);

		// Print netlist-specific statistics.
		if (!cycle)
			print_netlist_stats(sim_data->stages, sim_data->num_vectors);
		
		sim_data->total_time += wall_time() - wave_start_time;


		// Delay drawing of the progress bar until the second wave to improve the accuracy of the ETA.
		if ((sim_data->num_vectors == 1) || cycle)
			progress_bar_position = print_progress_bar(
					cycle/(double)(sim_data->num_vectors), progress_bar_position, progress_bar_length, sim_data->total_time);
		cycle++;
	}

	fflush(sim_data->out);
	fprintf(sim_data->modelsim_out, "run %ld\n", sim_data->num_vectors*100);

	printf("\n");
	// If a second output vector file was given via the -T option, verify that it matches.
	char *output_vector_file = global_args.sim_vector_output_file;
	if (output_vector_file)
	{
		if (verify_output_vectors(output_vector_file, sim_data->num_vectors))
			printf("Vector file \"%s\" matches output\n", output_vector_file);
		else
			error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Vector files differ.");
		printf("\n");
	}

	// Print statistics.
	print_simulation_stats(sim_data->stages, sim_data->num_vectors, sim_data->total_time, sim_data->simulation_time);
	// Perform ACE activity calculations
	calculate_activity ( netlist, sim_data->num_vectors, sim_data->act_out );
}  */




/**
 * Initialize simulation
 */
sim_data_t *init_simulation(netlist_t *netlist)
{
	//for multithreading
	used_time = std::numeric_limits<double>::max();
	number_of_workers = global_args.parralelized_simulation.value();
	if(number_of_workers >1 )
		warning_message(SIMULATION_ERROR,-1,-1,"Executing simulation with maximum of %ld threads", number_of_workers);
	
	found_best_time = false;

	num_of_clock = 0;

	sim_data_t *sim_data = (sim_data_t *)vtr::malloc(sizeof(sim_data_t));

	sim_data->netlist = netlist;
	printf("Beginning simulation. Output_files located @: %s\n", ((char *)global_args.sim_directory)); 
	fflush(stdout);

	// Open the output vector file.
	char out_vec_file[BUFFER_MAX_SIZE] = { 0 };
	odin_sprintf(out_vec_file,"%s/%s",((char *)global_args.sim_directory),OUTPUT_VECTOR_FILE_NAME);
	sim_data->out = fopen(out_vec_file, "w");
	if (!sim_data->out)
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not create output vector file.");

	// Open the input vector file.
	char in_vec_file[BUFFER_MAX_SIZE] = { 0 };
	odin_sprintf(in_vec_file,"%s/%s",((char *)global_args.sim_directory),INPUT_VECTOR_FILE_NAME);
	sim_data->in_out = fopen(in_vec_file, "w+");
	if (!sim_data->in_out)
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not create input vector file.");

	// Open the activity output file.
	char act_file[BUFFER_MAX_SIZE] = { 0 };
	odin_sprintf(act_file,"%s/%s",((char *)global_args.sim_directory),OUTPUT_ACTIVITY_FILE_NAME);
	sim_data->act_out = fopen(act_file, "w");
	if (!sim_data->act_out)
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not create activity output file.");

	// Open the modelsim vector file.
	char test_file[BUFFER_MAX_SIZE] = { 0 };
	odin_sprintf(test_file,"%s/%s",((char *)global_args.sim_directory),MODEL_SIM_FILE_NAME);
	sim_data->modelsim_out = fopen(test_file, "w");
	if (!sim_data->modelsim_out)
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not create modelsim output file.");

	// Create and verify the lines.
	sim_data->input_lines = create_lines(netlist, INPUT);
	if (!verify_lines(sim_data->input_lines))
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Input lines could not be assigned.");

	sim_data->output_lines = create_lines(netlist, OUTPUT);
	if (!verify_lines(sim_data->output_lines))
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Output lines could not be assigned.");


	sim_data->in = NULL;
	sim_data->input_vector_file = NULL;
	// Passed via the -t option. Input vectors can either come from a file 
	// if we expect no lines on input, then don't read the input file and generate as many input test vector as there are output
	// vectors so that simulation can run
	char *input_vector_file = global_args.sim_vector_input_file;
	if (input_vector_file && sim_data->input_lines->count != 0)
	{
		sim_data->input_vector_file  = input_vector_file;
		sim_data->in = fopen(sim_data->input_vector_file, "r");
		if (!sim_data->in)
			error_message(SIMULATION_ERROR, 0, -1, "Could not open vector input file: %s", sim_data->input_vector_file);

		sim_data->num_vectors = count_test_vectors(sim_data->in);

		// Read the vector headers and check to make sure they match the lines.
		if (!verify_test_vector_headers(sim_data->in, sim_data->input_lines))
			error_message(SIMULATION_ERROR, 0, -1, "Invalid vector header format in %s.", sim_data->input_vector_file);

		printf("Simulating %ld existing vectors from \"%s\".\n", sim_data->num_vectors, sim_data->input_vector_file); fflush(stdout);
	}
	// or be randomly generated. Passed via the -g option. it also serve as a fallback when we have an empty input
	else
	{
		if(input_vector_file)
		{
			sim_data->in = fopen(input_vector_file, "r");
			if (!sim_data->in)
				error_message(SIMULATION_ERROR, 0, -1, "Could not open vector input file: %s", input_vector_file);

			sim_data->num_vectors = count_empty_test_vectors(sim_data->in);
		}
		else
		{
			sim_data->num_vectors = global_args.sim_num_test_vectors;
		}
		printf("Simulating %ld new vectors.\n", sim_data->num_vectors); 
		fflush(stdout);

		srand(global_args.sim_random_seed);
	}

	// Setup data structures for activity estimation
	alloc_and_init_ace_structs(netlist);

	// Determine which edge(s) we are outputting.
	sim_data->total_time      = 0;  // Includes I/O
	sim_data->simulation_time = 0;  // Does not include I/O

	sim_data->stages = 0;	

	//maria	
	sim_data->thread_distribution = 0;

	if (!sim_data->num_vectors)
	{
		terminate_simulation(sim_data);
		error_message(SIMULATION_ERROR, 0, -1, "%s", "No vectors to simulate.");
	}

	return sim_data;
}

sim_data_t *terminate_simulation(sim_data_t *sim_data)
{
	free_stages(sim_data->stages);
	//maria
	free_thread_distribution(sim_data->thread_distribution);

	fclose(sim_data->act_out);

	free_lines(sim_data->output_lines);
	free_lines(sim_data->input_lines);

	fclose(sim_data->modelsim_out);
	fclose(sim_data->in_out);
	if (sim_data->input_vector_file)
		fclose(sim_data->in);
	fclose(sim_data->out);
	vtr::free(sim_data);
	sim_data = NULL;
	return sim_data;
}


void simulate_steps_sequential(sim_data_t *sim_data,double min_coverage)
{
	
	printf("\n");

	int       progress_bar_position = -1;
	const int progress_bar_length   = 50;
	
	int increment_vector_by = global_args.sim_num_test_vectors;;
	

	double current_coverage =0.0;
	int cycle=0;
	while(cycle < sim_data->num_vectors)
	{
		double wave_start_time = wall_time();

		// if we target a minimum coverage keep generating
		if(min_coverage > 0.0)
		{
			if(cycle+1 == sim_data->num_vectors)
			{
				current_coverage = (((double) get_num_covered_nodes(sim_data->stages) / (double) sim_data->stages->num_nodes));
				if(global_args.sim_achieve_best)
				{
					if(current_coverage > min_coverage)
					{
						increment_vector_by = global_args.sim_num_test_vectors;
						min_coverage = current_coverage;
						sim_data->num_vectors += increment_vector_by;
					}
					else if(increment_vector_by)
					{
						//slowly reduce the search until there is no more possible increment, this prevent building too large of a comparative vector pair
						sim_data->num_vectors += increment_vector_by;
						increment_vector_by /= 2;
					}

				}
				else
				{
					if(current_coverage < min_coverage)
						sim_data->num_vectors += increment_vector_by;
				}
			}
		}
		else
		{
			current_coverage = cycle/(double)sim_data->num_vectors;
		}

		single_step(sim_data, cycle);

		// Print netlist-specific statistics.
		if (!cycle)
			print_netlist_stats(sim_data->stages, sim_data->num_vectors);
		
		sim_data->total_time += wall_time() - wave_start_time;


		// Delay drawing of the progress bar until the second wave to improve the accuracy of the ETA.
		if ((sim_data->num_vectors == 1) || cycle)
			progress_bar_position = print_progress_bar(
					(cycle+1)/(double)(sim_data->num_vectors), progress_bar_position, progress_bar_length, sim_data->total_time);
		cycle++;
	}

}



void simulate_steps_in_parallel(sim_data_t *sim_data,int from_wave,int to_wave,double min_coverage)
{
	// produce a wave of values at each iteration

	double start_time = wall_time();
	int progress_bar_position = -1;
	const int progress_bar_length   = 50;
	int increment_vector_by = global_args.sim_num_test_vectors;
	double current_coverage =0.0;

	int offset = BUFFER_SIZE-1; // BUFFER_SIZE- simulation
	threads_waves = (to_wave-from_wave)/offset;
	threads_start = from_wave;
	threads_end = to_wave;

	pthread_cond_init(&start_threads, NULL);
	pthread_cond_init(&start_output, NULL);

	std::vector<std::thread> worker_threads;
	
	bool done = FALSE,restart = FALSE;
	while (!done)	
	{
		
		for (int wave = 0; wave<=threads_waves; wave++)
		{
			double simulation_start_time = wall_time();
			int from_cycle = from_wave + wave*offset;
			int to_cycle = from_cycle+offset;
			if (to_cycle > to_wave)
				to_cycle = to_wave;

			
			test_vector *v=NULL;
			// Assign vectors to lines, either by reading or generating them.
			// Every second cycle gets a new vector.

			
			
			char buffer[BUFFER_MAX_SIZE];
			
			for (int i=from_cycle;i<to_cycle;i++)
			{
				v = NULL;
				if (sim_data->in)
				{
					//buffer = NULL;
					if (!get_next_vector(sim_data->in, buffer))
						error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not read next vector.");
					
					v = parse_test_vector(buffer);
					//printf("Here\n");
				}
				else
				{
					v = generate_random_test_vector(i, sim_data);
				}
				//printf("v=%ld \n",v->values[0][0]);
				add_test_vector_to_lines(v, sim_data->input_lines, i);
				write_cycle_to_file(sim_data->input_lines, sim_data->in_out, i);
				write_cycle_to_modelsim_file(sim_data->netlist, sim_data->input_lines, sim_data->modelsim_out, i);
				
			}
			if (v)
				free_test_vector(v);

			if (wave == 0)
			{
				// lines as specified by the -p option.
				sim_data->stages = simulate_first_cycle(sim_data->netlist, from_cycle, sim_data->output_lines);
				// Make sure the output lines are still OK after adding custom lines.
				if (!verify_lines(sim_data->output_lines))
					error_message(SIMULATION_ERROR, 0, -1,"%s\n", 
							"Problem detected with the output lines after the first cycle.");
				
				//maria
				sim_data->thread_distribution = calculate_thread_distribution(sim_data->stages);
				
				//create_threads_and_let them wait for the signal
				for (int t=0; t<sim_data->thread_distribution->number_of_threads; t++)
				{	
					worker_threads.push_back(std::thread(compute_and_store_part_in_waves_multithreaded,t,sim_data->thread_distribution->thread_nodes[t],from_wave,to_wave,offset,TRUE));
				}

			}

			if (wave !=0 || restart)
			{
				
				pthread_mutex_lock(&output_mp);
				threads_done_wave =0;
				pthread_mutex_unlock(&output_mp);
				if( (errno =pthread_cond_broadcast(&start_threads)) !=0)
					printf("Broadcast Error!");					

			}

			pthread_mutex_lock(&threads_mp);
			while (threads_done_wave != sim_data->thread_distribution->number_of_threads)
			{
				pthread_cond_wait(&start_output,&threads_mp);
			}
			pthread_mutex_unlock(&threads_mp);
	
			
			for (int i=from_cycle;i<to_cycle;i++)
			{
				//if (i!=0)
				write_cycle_to_file(sim_data->output_lines, sim_data->out, i);

			}
			//update memory directories of all memory nodes
			write_back_memory_nodes(sim_data->thread_distribution->memory_nodes->nodes,sim_data->thread_distribution->memory_nodes->number_of_nodes);
			sim_data->simulation_time += wall_time() - simulation_start_time;
			if (wave==threads_waves) //check for coverage in the last cycle
			{
				if(min_coverage > 0.0)
				{
					current_coverage = (((double) get_num_covered_nodes(sim_data->stages) / (double) sim_data->stages->num_nodes));
					if(global_args.sim_achieve_best)
					{
						if(current_coverage > min_coverage)
						{
							increment_vector_by = global_args.sim_num_test_vectors;
							min_coverage = current_coverage;
							sim_data->num_vectors += increment_vector_by;
						}
						else if(increment_vector_by)
						{
							//slowly reduce the search until there is no more possible increment, this prevent building too large of a comparative vector pair
							sim_data->num_vectors += increment_vector_by;
							increment_vector_by /= 2;
						}

					}
					else
					{
						if(current_coverage < min_coverage)
							sim_data->num_vectors += increment_vector_by;
					}
					//update the cycle boundaries to continue calculations
					if (sim_data->num_vectors != to_cycle)
					{
						from_wave = to_cycle+1;
						to_wave = sim_data->num_vectors;
						threads_waves = (to_wave-from_wave)/offset;

						pthread_mutex_lock(&output_mp);
						threads_start = from_cycle;
						threads_end = to_cycle;
						pthread_mutex_unlock(&output_mp);
						restart = TRUE;
						//printf("threads start %ld threads end %ld",threads_start,threads_end);	
					}
					else
						done= TRUE;			
				}
				else
				{
					current_coverage = to_cycle/(double)sim_data->num_vectors;
					done = TRUE;
				}
				

			}
			

			// Print netlist-specific statistics.
			if (wave == 0)
				print_netlist_stats(sim_data->stages, sim_data->num_vectors);
			
			sim_data->total_time = wall_time() - start_time;
			progress_bar_position = print_progress_bar(
						to_cycle/(double)(sim_data->num_vectors), progress_bar_position, progress_bar_length, sim_data->total_time);

		}
		if (done)
		{
			//signal to unblock threads and let them finish
			if( (errno =pthread_cond_broadcast(&start_threads)) !=0)
				printf("Broadcast Error!");

		}

	}

	
	int threadnum = 0;
	for (auto &worker: worker_threads)	
	{
		worker.join();
		threadnum++;
	}
	//wait for them to be done
	pthread_cond_destroy(&start_output);
	pthread_cond_destroy(&start_threads);

	sim_data->total_time = wall_time() - start_time;
			progress_bar_position = print_progress_bar(
						(double)sim_data->num_vectors/(double)(sim_data->num_vectors), progress_bar_position, progress_bar_length, sim_data->total_time);

}

/**
 * single step sim
 */
int single_step(sim_data_t *sim_data, int cycle)
{

	test_vector *v = NULL;
	// Assign vectors to lines, either by reading or generating them.
	// Every second cycle gets a new vector.

	double simulation_start_time = wall_time();

	// Perform simulation
	if (sim_data->in)
	{
		char buffer[BUFFER_MAX_SIZE];

		if (!get_next_vector(sim_data->in, buffer))
			error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Could not read next vector.");

		v = parse_test_vector(buffer);
	}
	else
	{
		v = generate_random_test_vector(cycle, sim_data);
	}

	add_test_vector_to_lines(v, sim_data->input_lines, cycle);
	write_cycle_to_file(sim_data->input_lines, sim_data->in_out, cycle);
	write_cycle_to_modelsim_file(sim_data->netlist, sim_data->input_lines, sim_data->modelsim_out, cycle);
	free_test_vector(v);

	if(!cycle)
	{
		// The first cycle produces the stages, and adds additional
		// lines as specified by the -p option.
		sim_data->stages = simulate_first_cycle(sim_data->netlist, cycle, sim_data->output_lines);

		//split the nodes into threads using the stages agbove for parallel calculations
		//maria
		//sim_data->thread_distribution = calculate_thread_distribution(sim_data->stages);

		// Make sure the output lines are still OK after adding custom lines.
		if (!verify_lines(sim_data->output_lines))
			error_message(SIMULATION_ERROR, 0, -1,"%s\n", 
					"Problem detected with the output lines after the first cycle.");
	}
	else
	{
		simulate_cycle(cycle, sim_data->stages);
		//maria
		//simulate_cycle_multithreaded(cycle, sim_data->thread_distribution); 
	}
	write_cycle_to_file(sim_data->output_lines, sim_data->out, cycle);



	sim_data->simulation_time += wall_time() - simulation_start_time;
		
	return cycle +1;
}

/*
 * This simulates a single cycle using the stages generated
 * during the first cycle.
 *
 * simulation computes a small number of cycles sequentially and
 * a small number in parallel. The minimum parallel and sequential time is
 * taken for each stage, and that stage is computed in parallel for all subsequent
 * cycles if speedup is observed.
 */

static void compute_and_store_part(int start, int end, int current_stage, stages_t *s, int cycle)
{

	for (int j = start;j >= 0 && j <= end && j < s->counts[current_stage]; j++)
		if(s->stages[current_stage][j])
			compute_and_store_value(s->stages[current_stage][j], cycle);

}

static void simulate_cycle(int cycle, stages_t *s)
{
	double total_run_time = 0;
	for(int i = 0; i < s->count; i++)
	{
		double time = wall_time();

		std::vector<std::thread> workers;
		int previous_end = 0;

		int nodes_per_thread = s->counts[i]/number_of_workers;
		int remainder_nodes_per_thread = s->counts[i]%number_of_workers;

		for (int id =0; id < number_of_workers; id++)
		{
			int start = previous_end;
			int end = start + nodes_per_thread + ((remainder_nodes_per_thread > 0)? 1: 0);

			remainder_nodes_per_thread -= 1;
			previous_end = end + 1;

			if( (end-start) > 0 )
			{
				if(id < number_of_workers-1) // if child threads
					workers.push_back(std::thread(compute_and_store_part,start,end,i,s,cycle));
				else
					compute_and_store_part(start,end,i,s,cycle);
			}

		}	

		for (auto &worker: workers)	
			worker.join();

		total_run_time += wall_time()-time;
	}
}


//maria
static void compute_and_store_part_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int cycle)
{

	int *nodes_done = (int*)vtr::calloc(thread_nodes->number_of_nodes,sizeof(int));
	int nodes_counter = thread_nodes->number_of_nodes;
	nnode_t **nodes_in_progress = (nnode_t **)vtr::malloc(sizeof(nnode_t*) *thread_nodes->number_of_nodes );
	
	for (int i=0;i<nodes_counter;i++)
		nodes_in_progress[i] = thread_nodes->nodes[i];
	
	
	while (nodes_counter!=0 )
	{
		for (int j = 0;j < nodes_counter; j++)
		{
			nnode_t *node = nodes_in_progress[j];

			if(node && is_node_ready(node,cycle) && !is_node_complete(node,cycle) )
			{
				compute_and_store_value(node, cycle);
				nodes_done[j]=1;
			}
			else if(!node || is_node_complete(node,cycle) )
				nodes_done[j]=1;
		}

		nnode_t **temp = &(*nodes_in_progress);
		int not_done = 0;
		//number of nodes 
		for (int i=0;i<nodes_counter;i++)
		{
			if (!nodes_done[i])
			{
				nodes_in_progress[not_done] = temp[i];
				not_done++;
			}
			nodes_done[i] = 0;
		}
		nodes_counter = not_done;
	}
	vtr::free(nodes_done);
	vtr::free(nodes_in_progress);
}



//maria
static void compute_and_store_part_in_waves_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int from_wave, int to_wave,int offset,bool /*notify_back*/)
{

	int *nodes_done = (int*)vtr::calloc(thread_nodes->number_of_nodes,sizeof(int));
	int nodes_counter = thread_nodes->number_of_nodes;

	int waves = (to_wave-from_wave)/offset;
	//do while
	for (int wave = 0;wave<=waves; wave++)
	{
		int from_cycle = from_wave + wave*offset;
		int to_cycle = from_cycle+offset;
		if (to_cycle > to_wave)
			to_cycle = to_wave;
		
		for (int cycle = from_cycle; cycle<to_cycle; cycle++)
		{
			nodes_counter = thread_nodes->number_of_nodes;
			for (int i=0;i<nodes_counter;i++) 
				nodes_done[i] = 0;
			
			int done = 0;
			while (nodes_counter!=done )
			{
				for (int j = 0;j < nodes_counter; j++)
				{
					if (!nodes_done[j])
					{
						nnode_t *node = thread_nodes->nodes[j];

						if(node && is_node_ready(node,cycle) && !is_node_complete(node,cycle) )
						{
							compute_and_store_value(node, cycle);
							nodes_done[j]=1;

						}
						else if(!node || is_node_complete(node,cycle) )
						{
							nodes_done[j]=1;
						}
					}
				}

				done = 0;
				//number of nodes 
				for (int i=0;i<nodes_counter;i++)
				{
					if (nodes_done[i])
					{
						done++;
					}
				}
				
			}

		}
		//signal the current wave is done
		pthread_mutex_lock(&threads_mp);
		threads_done_wave++;
		pthread_cond_broadcast(&start_output);
		pthread_cond_wait(&start_threads,&threads_mp);
		pthread_mutex_unlock(&threads_mp);

		if (wave == waves) //check if we need to start again for coverage
		{
			int shared_from_wave,shared_to_wave;
			pthread_mutex_lock(&threads_mp);
			shared_from_wave = threads_start;
			shared_to_wave = threads_end;
			pthread_mutex_unlock(&threads_mp);

			//if the shared variable is changed then copy the other values and restart the loop
			if (shared_from_wave != from_wave)
			{
				from_wave = shared_from_wave;
				to_wave = shared_to_wave;
				waves = (to_wave-from_wave)/offset;
			}

		}

	}
	vtr::free(nodes_done);
}

//maria
static void compute_and_store_part_wave_multithreaded(int /*t_id*/,netlist_subset *thread_nodes,int from_wave, int to_wave)
{

	int *nodes_done = (int*)vtr::calloc(thread_nodes->number_of_nodes,sizeof(int));
	int nodes_counter = thread_nodes->number_of_nodes;


	for (int cycle = from_wave; cycle<to_wave; cycle++)
	{
		nodes_counter = thread_nodes->number_of_nodes;
		for (int i=0;i<nodes_counter;i++) 
			nodes_done[i] = 0;
		
		int done = 0;
		while (nodes_counter!=done )
		{
			for (int j = 0;j < nodes_counter; j++)
			{
				if (!nodes_done[j])
				{
					nnode_t *node = thread_nodes->nodes[j];

					if(node && is_node_ready(node,cycle) && !is_node_complete(node,cycle) )
					{
						compute_and_store_value(node, cycle);
						nodes_done[j]=1;
					}
					else if(!node || is_node_complete(node,cycle) )
						nodes_done[j]=1;
				}
			}

			
			done = 0;
			//number of nodes 
			for (int i=0;i<nodes_counter;i++)
			{
				if (nodes_done[i])
				{
					done++;
				}
			}
			
		}
	}
	
	vtr::free(nodes_done);

}


//maria
static void simulate_cycle_multithreaded(int cycle, thread_node_distribution *thread_distribution)
{	
	std::vector<std::thread> workers;



	for (int t=0; t<thread_distribution->number_of_threads; t++)
	{
		workers.push_back(std::thread(compute_and_store_part_wave_multithreaded,t,thread_distribution->thread_nodes[t],cycle,cycle+1));
	}

	int threadnum = 0;
	for (auto &worker: workers)	
	{
		worker.join();
		threadnum++;
	}
}



/*
 * Updates all pins which have been flagged as undriven
 * to -1 for the given cycle.
 *
 * Also checks that other pins have been updated
 * by cycle 3 and throws an error if they haven't been.
 *
 * This function is called when each node is updated as a
 * safeguard.
 */
static void update_undriven_input_pins(nnode_t *node, int cycle)
{
	int i;
	for (i = 0; i < node->num_undriven_pins; i++)
		update_pin_value(node->undriven_pins[i], init_value(node), cycle);

	// By the third cycle everything in the netlist should have been updated.
	if (cycle == 3)
	{
		for (i = 0; i < node->num_input_pins; i++)
		{
			if (get_pin_cycle( node->input_pins[i]) < cycle-1)
			{
				// Print the trace.
				nnode_t *root = print_update_trace(node, cycle);
				// Throw an error.
				error_message(SIMULATION_ERROR,0,-1,"Odin has detected that an input pin attached to %s isn't being updated.\n"
						"\tPin name: %s\n"
						"\tRoot node: %s\n"
						"\tSee the trace immediately above this message for details.\n",
						node->name, node->input_pins[i]->name, root?root->name:"N/A"
				);
			}
		}
	}
}

/*
 * Checks to see if the node is ready to be simulated for the given cycle.
 */
static int is_node_ready(nnode_t* node, int cycle)
{
	if (!cycle)
	{
		/*
		* Flags any inputs pins which are undriven and have
		* not already been flagged.
		*/
		int i;
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];

			if (!pin->net || !pin->net->driver_pin || !pin->net->driver_pin->node)
			{
				int already_flagged = FALSE;
				int j;
				for (j = 0; j < node->num_undriven_pins; j++)
				{
					if (node->undriven_pins[j] == pin)
						already_flagged = TRUE;
				}

				if (!already_flagged)
				{
					node->undriven_pins = (npin_t **)vtr::realloc(node->undriven_pins, sizeof(npin_t *) * (node->num_undriven_pins + 1));
					node->undriven_pins[node->num_undriven_pins++] = pin;

					warning_message(SIMULATION_ERROR,0,-1,"A node (%s) has an undriven input pin.", node->name);
				}
			}
		}
		update_undriven_input_pins(node, cycle);

	}


	if (node->type == FF_NODE)
	{
		npin_t *D_pin     = node->input_pins[0];
		npin_t *clock_pin = node->input_pins[1];
		// Flip-flops depend on the D input from the previous cycle and the clock from this cycle.

		if (get_pin_cycle(D_pin) < cycle-1)
			return FALSE;

		if (get_pin_cycle(clock_pin) < cycle )
			return FALSE;
	}
	else if (node->type == MEMORY)
	{
		int i;
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];
			// The data and write enable inputs rely on the values from the previous cycle.
			if (!strcmp(pin->mapping, "data") || !strcmp(pin->mapping, "data1") || !strcmp(pin->mapping, "data2"))
			{
				if (get_pin_cycle(pin) < cycle-1)
					return FALSE;
			}
			else
			{
				if (get_pin_cycle(pin) < cycle)
					return FALSE;
			}
		}
	}
	else
	{
		int i;
		for (i = 0; i < node->num_input_pins; i++)
			if (get_pin_cycle(node->input_pins[i]) < cycle)
				return FALSE;
	}
	return TRUE;
}


/*
 * Simulates the first cycle by traversing the netlist and returns
 * the nodes organised into parallelizable stages. Also adds lines to
 * custom pins and nodes as requested via the -p option.
 */
static stages_t *simulate_first_cycle(netlist_t *netlist, int cycle, lines_t *l)
{
	std::queue<nnode_t *> queue = std::queue<nnode_t *>();
	// Enqueue top input nodes
	int i;
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if(is_node_ready(netlist->top_input_nodes[i], cycle))
		{
			netlist->top_input_nodes[i]->in_queue = TRUE;
			queue.push(netlist->top_input_nodes[i]);
		}
	}

	// Enqueue constant nodes.
	nnode_t *constant_nodes[] = {netlist->gnd_node, netlist->vcc_node, netlist->pad_node};
	int num_constant_nodes = 3;
	for (i = 0; i < num_constant_nodes; i++)
	{
		if(is_node_ready(constant_nodes[i], cycle))
		{
			constant_nodes[i]->in_queue = TRUE;
			queue.push(constant_nodes[i]);
		}
	}

	nnode_t **ordered_nodes = 0;
	int   num_ordered_nodes = 0;


	while (! queue.empty())
	{
		nnode_t *node = queue.front();
		queue.pop();
		compute_and_store_value(node, cycle);
		// Match node for items passed via -p and add to lines if there's a match.
		add_additional_items_to_lines(node, l);

		// Enqueue child nodes which are ready, not already queued, and not already complete.
		int num_children = 0;
		nnode_t **children = get_children_of(node, &num_children);

		for (i = 0; i < num_children; i++)
		{
			nnode_t* node2 = children[i];

			if (!node2->in_queue && is_node_ready(node2, cycle) && !is_node_complete(node2, cycle))
			{
				node2->in_queue = TRUE;
				queue.push(node2);
			}

		}
		vtr::free(children);

		node->in_queue = FALSE;

		// Add the node to the ordered nodes array.
		ordered_nodes = (nnode_t **)vtr::realloc(ordered_nodes, sizeof(nnode_t *) * (num_ordered_nodes + 1));
		ordered_nodes[num_ordered_nodes++] = node;
	}

	// Reorganise the ordered nodes into stages for parallel computation.
	stages_t *s = stage_ordered_nodes(ordered_nodes, num_ordered_nodes);
	vtr::free(ordered_nodes);

	return s;
}

/*
 * Puts the ordered nodes in stages, each of which can be computed in parallel.
 */
static stages_t *stage_ordered_nodes(nnode_t **ordered_nodes, int num_ordered_nodes) {
	stages_t *s = (stages_t *)vtr::malloc(sizeof(stages_t));
	s->stages = (nnode_t ***)vtr::calloc(1,sizeof(nnode_t**));
	s->counts = (int *)vtr::calloc(1,sizeof(int));
	s->num_children = (int *)vtr::calloc(1,sizeof(int));
	s->count  = 1;
	s->num_connections = 0;
	s->num_nodes = num_ordered_nodes;
	s->avg_worker_count = 0;
	s->worker_const = 1;
	s->worker_temp = 0;
	s->times =__DBL_MAX__;

	// Hash tables index the nodes in the current stage, as well as their children.
	std::unordered_set<nnode_t *> stage_children = std::unordered_set<nnode_t *>();
	std::unordered_set<nnode_t *> stage_nodes = std::unordered_set<nnode_t *>();

	int i;
	for (i = 0; i < num_ordered_nodes; i++)
	{
		nnode_t* node = ordered_nodes[i];
		int stage = s->count-1;

		// Get the node's children for dependency checks.
		int num_children;
		nnode_t **children = get_children_of(node, &num_children);

		// Determine if the node is a child of any node in the current stage.
		bool is_child_of_stage = (stage_children.find(node) != stage_children.end());

		// Determine if any node in the current stage is a child of this node.
		bool is_stage_child_of = false;
		if (!is_child_of_stage)
			for (int j = 0; j < num_children; j++)
				if ((is_stage_child_of = (stage_nodes.find(node) != stage_nodes.end())))
					break;

		// Start a new stage if this node is related to any node in the current stage.
		if (is_child_of_stage || is_stage_child_of)
		{
			s->stages       = (nnode_t ***)vtr::realloc(s->stages, sizeof(nnode_t**) * (s->count+1));
			s->counts       = (int *)vtr::realloc(s->counts, sizeof(int)       * (s->count+1));
			s->num_children = (int *)vtr::realloc(s->num_children, sizeof(int) * (s->count+1));
			stage = s->count++;
			s->stages[stage] = 0;
			s->counts[stage] = 0;
			s->num_children[stage] = 0;

			stage_children.clear();
			stage_nodes.clear();
		}

		// Add the node to the current stage.
		s->stages[stage] = (nnode_t **)vtr::realloc(s->stages[stage],sizeof(nnode_t*) * (s->counts[stage]+1));
		s->stages[stage][s->counts[stage]++] = node;

		// Index the node.
		stage_nodes.insert(node);
		//printf("NodeID %ld %s typeof %ld at stage %ld\n",node->unique_id,node->name,node->type,stage);
		// Index its children.
		for (int j = 0; j < num_children; j++)
		{
			//printf("  ChildID %ld %s typeof %ld \n ",children[j]->unique_id,children[j]->name,children[j]->type);
			stage_children.insert(children[j]);
		}

		// Record the number of children for computing the degree.
		s->num_connections += num_children;

		s->num_children[stage] += num_children;

		vtr::free(children);
	}
	stage_children.clear();
	stage_nodes.clear();
	return s;
}

//maria
//simulate one cycle to determine the parallelization degree of the circuit
//returns the number of threads
static thread_node_distribution *calculate_thread_distribution(stages_t *s)
{
	//double nodecost = 1;
	//double extranodeoverhead = 1.0*nodecost;
	//double lessnodeoverhead = -0.5*nodecost;
	double nodeoverhead_100 = 100;
	double nodeoverhead_200 = 200;
	double nodeoverhead_300 = 300;
	double nodeoverhead_400 = 400;

	double stagecost = nodeoverhead_300;
	//double threadoverheadcost = 5*nodecost;

	/** not portable
	 * int max_available_threads = get_nprocs();
	 */

	int max_available_threads = number_of_workers;

	//store nodes for each thread
	thread_node_distribution* thread_distribution= (thread_node_distribution *)vtr::malloc(sizeof(thread_node_distribution));

	//for each stage 
	double *stagescost = (double *)vtr::malloc(sizeof(double)* s->count);
	double graphcost = 0.0;
	int all_nodes = get_num_covered_nodes(s);
	std::map<int, int> nodes_inserted;  //nodeId,flag

	thread_distribution->memory_nodes = (netlist_subset *)vtr::malloc(sizeof(netlist_subset));
	int number_of_mem_nodes = 0;
	nnode_t** nodes_mem_sub = 0; 

	//traverse and calculate the graph cost
	for(int i = 0; i < s->count; i++)
	{
		stagescost[i] = 0.0;
		nnode_t** nodes = s->stages[i];		

		//for each node
		for (int j =0; j < s->counts[i]; j++)
		{			
			//stagescost[i]+=nodecost;
			if (nodes[j]->type == GND_NODE || nodes[j]->type == VCC_NODE || nodes[j]->type == OUTPUT_NODE || nodes[j]->type == CLOCK_NODE || nodes[j]->type == PAD_NODE || nodes[j]->type == MUX_2 || nodes[j]->type == LOGICAL_AND)
				stagescost[i]+=nodeoverhead_100;
			else if (nodes[j]->type == HARD_IP || nodes[j]->type == GENERIC || nodes[j]->type == MEMORY)
				stagescost[i]+=nodeoverhead_300;
			else if (nodes[j]->type ==  MULTIPLY)
				stagescost[i]+=nodeoverhead_400;
			else if (nodes[j]->type == INPUT_NODE)
				stagescost[i]+=1;
			else
				stagescost[i]+=nodeoverhead_200;

			nodes_inserted[nodes[j]->unique_id] = 0;
		}
		graphcost += stagecost+stagescost[i];
	}
	
	thread_distribution->memory_nodes->number_of_nodes = number_of_mem_nodes;
	thread_distribution->memory_nodes->nodes = nodes_mem_sub;

	//printf("graphcost: %lf .\n",graphcost);

	double threadworkcost = ceil(graphcost/max_available_threads);
	int num_threads = 0;
	int current_stage = 0;
	int node_in_stage = 0;
	int nodes_assigned = 0;

	//printf("threadworkcost: %lf .\n",threadworkcost);
	//for each stage 
	netlist_subset **circuit_borders = (netlist_subset **)vtr::malloc(sizeof(netlist_subset*) * max_available_threads);

	int threads = ceil(graphcost/threadworkcost);

	for(int i = 0; i < threads; i++)
	{
		circuit_borders[i] = (netlist_subset *)vtr::malloc(sizeof(netlist_subset));
		circuit_borders[i]->nodes = 0;
	}

	for (int t =0;t<threads && nodes_assigned!=all_nodes;t++)
	{
		double threadcost = 0.0;
		//nodes per thread
		int number_of_nodes = 0;
		nnode_t** nodes_sub = 0; //(nnode_t **)vtr::calloc(1,sizeof(nnode_t*));

		while (threadcost< threadworkcost)
		{
			nnode_t* node = s->stages[current_stage][node_in_stage];
			//printf("NodeID %ld assigned to Thread %ld\n",node->unique_id,t);
			if (nodes_inserted[node->unique_id]==0)
			{
				nodes_sub = (nnode_t **)vtr::realloc(nodes_sub,sizeof(nnode_t*) * (number_of_nodes+1) );
				nodes_sub[number_of_nodes++] = node;
				nodes_assigned++;
				nodes_inserted[node->unique_id] = 1;		

				if (node->type == GND_NODE || node->type == VCC_NODE || node->type == OUTPUT_NODE || node->type == CLOCK_NODE || node->type == PAD_NODE || node->type == MUX_2 || node->type == LOGICAL_AND)
					threadcost+=nodeoverhead_100;
				else if (node->type == HARD_IP || node->type == GENERIC || node->type == MEMORY)
					threadcost+=nodeoverhead_300;
				else if (node->type ==  MULTIPLY)
					threadcost+=nodeoverhead_400;
				else if (node->type == INPUT_NODE)
					threadcost+=1;
				else
					threadcost+=nodeoverhead_200;
				//memory family put together if uncomment. not necessary
				/*
				int num_children;
				nnode_t **children = get_children_of(node, &num_children);	
				nnode_t**memory_nodes = 0;
				int num_memory_nodes = 0;
				//find all decendeces and ancestors of evey memory node related
				nnode_t**memory_family = 0;
				int num_memory_family = 0;
				for(int child=0;child<num_children;child++)
				{
					if (children[child]->type == MEMORY || children[child]->type == HARD_IP)
					{
						memory_nodes = (nnode_t **)vtr::realloc(memory_nodes,sizeof(nnode_t*) * (num_memory_nodes+1) );
						memory_nodes[num_memory_nodes++] = children[child];
						nodes_inserted[children[child]->unique_id] = -1; //to be processed

						memory_family = (nnode_t **)vtr::realloc(memory_family,sizeof(nnode_t*) * (num_memory_family+1) );
						memory_family[num_memory_family++] = children[child];
					}
				}
				if (num_memory_nodes !=0 )
				{
					int mem_index = 0;			
					while(mem_index !=num_memory_nodes)
					{
						nnode_t* memnode = memory_nodes[mem_index];
						nodes_inserted[memnode->unique_id] = -1; 
						//printf("memnode tyope of %s\n ",memnode->name);
						int num_parents;
						nnode_t **parents = get_parents_of(memnode, &num_parents);

						for(int parent=0;parent<num_parents;parent++)
						{
							if ( nodes_inserted[parents[parent]->unique_id] != -1 ) //if it is not processed here
							{
								memory_family = (nnode_t **)vtr::realloc(memory_family,sizeof(nnode_t*) * (num_memory_family+1) );
								memory_family[num_memory_family++] = parents[parent];
								
								//printf("NodeP %ld -1\n",parents[parent]->unique_id);								
								if (parents[parent]->type == HARD_IP || parents[parent]->type == MEMORY) //its a memory node add it to the queue
								{
									memory_nodes = (nnode_t **)vtr::realloc(memory_nodes,sizeof(nnode_t*) * (num_memory_nodes+1) );
									memory_nodes[num_memory_nodes++] = parents[parent];

								}
								else
								{
									nodes_inserted[parents[parent]->unique_id] = -1;
								}
							}
						}
						int num_children;
						nnode_t **children = get_children_of(memnode, &num_children);

						for(int child=0;child<num_children;child++)
						{
							if ( nodes_inserted[children[child]->unique_id] != -1 ) //if it is not processed here
							{
								memory_family = (nnode_t **)vtr::realloc(memory_family,sizeof(nnode_t*) * (num_memory_family+1) );
								memory_family[num_memory_family++] = children[child];
								
								
								if (children[child]->type == HARD_IP || children[child]->type == MEMORY) //its a memory node add it to the queue
								{
									memory_nodes = (nnode_t **)vtr::realloc(memory_nodes,sizeof(nnode_t*) * (num_memory_nodes+1) );
									memory_nodes[num_memory_nodes++] = children[child];
									//nodes_inserted[children[child]->unique_id] = -1; 
								}
								else
								{
									nodes_inserted[children[child]->unique_id] = -1;
								}								
							}
						}
						mem_index++;
						//printf("mem_index %ld num_memory_family %ld  num_memory_nodes%ld \n",mem_index,num_memory_family,num_memory_nodes);
					}
					printf("Memory node found \n");
					mem_index = 0;
					for (mem_index=0;mem_index<num_memory_family;mem_index++)
					{
						nnode_t* memnode = memory_family[mem_index];
						//if (!nodes_inserted[memnode->unique_id])
						//{
						nodes_sub = (nnode_t **)vtr::realloc(nodes_sub,sizeof(nnode_t*) * (number_of_nodes+1) );
						nodes_sub[number_of_nodes++] = memnode;
						nodes_assigned++;
						nodes_inserted[memnode->unique_id] = 1;		
						threadcost+=nodecost;
						if (memnode->type == HARD_IP || memnode->type == GENERIC || memnode->type == MEMORY )
							threadcost+=extranodeoverhead;
						if (memnode->type == GND_NODE || memnode->type == VCC_NODE || memnode->type == INPUT_NODE )
							threadcost+=lessnodeoverhead;
						//}
						//printf("NodeP %ld 1\n",memnode->unique_id);
						//printf("Asgnd %ld out of %ld \n",nodes_assigned,all_nodes);
					}
					//vtr::free(memory_nodes);
					//printf(" Node added \n");
				}
				*/
			}
			//printf("Next node %ld  %ld/%ld \n",node->unique_id,nodes_assigned,all_nodes);
			
			if (node_in_stage == s->counts[current_stage]-1) //change stage
			{
				node_in_stage = 0;

				if (current_stage == s->count-1) //last stage
				{
					
					break;
				}
				else
					current_stage++; //next stage
			}
			else //same stage next node
				node_in_stage++;

		}
		// add them to the structure
		circuit_borders[num_threads]->nodes = nodes_sub;
		circuit_borders[num_threads]->number_of_nodes = number_of_nodes;
		++num_threads;
	}

	// Create a map iterator and point to beginning of map
	std::map<int, int>::iterator it = nodes_inserted.begin();
 
	// Iterate over the map using Iterator till end.
	while (it != nodes_inserted.end())
	{
		// Accessing KEY from element pointed by it.
		int node_id = it->first;
 
		// Accessing VALUE from element pointed by it.
		int inserted = it->second;
 
		if (inserted !=1)
		{
			error_message(SIMULATION_ERROR,1475,-1,"Node %ld is not assigned for simulation!",node_id);

		}
 
		// Increment the Iterator to point to next entry
		it++;
	}


	//if (nodes_assigned == FALSE)
	//{
	//	error_message(SIMULATION_ERROR,1475,-1, "%s", "Some nodes are not assigned for simulation!");
	//}
	thread_distribution->thread_nodes = circuit_borders;
	thread_distribution->number_of_threads = num_threads;

	number_of_workers = num_threads;
	
	vtr::free(stagescost);
	return thread_distribution;
}


/*
 * Given a node, this function will simulate that node's new outputs,
 * and updates those pins.
 */
static bool compute_and_store_value(nnode_t *node, int cycle)
{
	//double computation_time = wall_time();
	is_node_ready(node,cycle);
	operation_list type = is_clock_node(node)?CLOCK_NODE:node->type;
	switch(type)
	{
		case MUX_2:
			compute_mux_2_node(node, cycle);
			break;
		case FF_NODE:
			compute_flipflop_node(node, cycle);
			break;
		case MEMORY:
			compute_memory_node(node, cycle);
			break;
		case MULTIPLY:
			compute_multiply_node(node, cycle);
			break;
		case LOGICAL_AND: // &&
		{
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			char zero    = FALSE;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if      (pin <  0) { unknown = TRUE; }
				else if (pin == 0) { zero    = TRUE; break; }
			}
			if      (zero)    update_pin_value(node->output_pins[0],  0, cycle);
			else if (unknown) update_pin_value(node->output_pins[0], -1, cycle);
			else              update_pin_value(node->output_pins[0],  1, cycle);
			break;
		}
		case LOGICAL_OR:
		{	// ||
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			char one     = FALSE;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if      (pin <  0) { unknown = TRUE; }
				else if (pin == 1) { one     = TRUE; break; }
			}
			if      (one)     update_pin_value(node->output_pins[0],  1, cycle);
			else if (unknown) update_pin_value(node->output_pins[0], -1, cycle);
			else              update_pin_value(node->output_pins[0],  0, cycle);
			break;
		}
		case LOGICAL_NAND:
		{	// !&&
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			char one     = FALSE;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if      (pin <  0) { unknown = TRUE; }
				else if (pin == 0) { one     = TRUE; break; }
			}
			if      (one)     update_pin_value(node->output_pins[0],  1, cycle);
			else if (unknown) update_pin_value(node->output_pins[0], -1, cycle);
			else              update_pin_value(node->output_pins[0],  0, cycle);
			break;
		}
		case LOGICAL_NOT: // !
		case LOGICAL_NOR: // !|
		{
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			char zero    = FALSE;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if      (pin <  0) { unknown = TRUE; }
				else if (pin == 1) { zero    = TRUE; break; }
			}
			if      (zero)    update_pin_value(node->output_pins[0],  0, cycle);
			else if (unknown) update_pin_value(node->output_pins[0], -1, cycle);
			else              update_pin_value(node->output_pins[0],  1, cycle);
			break;
		}
		case LT: // < 010 1
		{
			verify_i_o_availabilty(node, 3, 1);

			signed char pin0 = get_pin_value(node->input_pins[0],cycle);
			signed char pin1 = get_pin_value(node->input_pins[1],cycle);
			signed char pin2 = get_pin_value(node->input_pins[2],cycle);

			if      (pin0  < 0 || pin1  < 0 || pin2  < 0)
				update_pin_value(node->output_pins[0], -1, cycle);
			else if (pin0 == 0 && pin1 == 1 && pin2 == 0)
				update_pin_value(node->output_pins[0],  1, cycle);
			else
				update_pin_value(node->output_pins[0],  0, cycle);

			break;
		}
		case GT: // > 100 1
		{
			verify_i_o_availabilty(node, 3, 1);

			signed char pin0 = get_pin_value(node->input_pins[0],cycle);
			signed char pin1 = get_pin_value(node->input_pins[1],cycle);
			signed char pin2 = get_pin_value(node->input_pins[2],cycle);

			if      (pin0  < 0 || pin1  < 0 || pin2  < 0)
				update_pin_value(node->output_pins[0], -1, cycle);
			else if (pin0 == 1 && pin1 == 0 && pin2 == 0)
				update_pin_value(node->output_pins[0],  1, cycle);
			else
				update_pin_value(node->output_pins[0],  0, cycle);

			break;
		}
		case ADDER_FUNC: // 001 1\n010 1\n100 1\n111 1
		{
			verify_i_o_availabilty(node, 3, 1);

			signed char pin0 = get_pin_value(node->input_pins[0],cycle);
			signed char pin1 = get_pin_value(node->input_pins[1],cycle);
			signed char pin2 = get_pin_value(node->input_pins[2],cycle);

			if (pin0 < 0 || pin1 < 0 || pin2 < 0)
				update_pin_value(node->output_pins[0], -1, cycle);
			else if (
					   (pin0 == 0 && pin1 == 0 && pin2 == 1)
					|| (pin0 == 0 && pin1 == 1 && pin2 == 0)
					|| (pin0 == 1 && pin1 == 0 && pin2 == 0)
					|| (pin0 == 1 && pin1 == 1 && pin2 == 1)
			)
				update_pin_value(node->output_pins[0], 1, cycle);
			else
				update_pin_value(node->output_pins[0], 0, cycle);

			break;
		}
		case CARRY_FUNC: // 011 1\n100 1\n110 1\n111 1
		{
			verify_i_o_availabilty(node, 3, 1);

			signed char pin0 = get_pin_value(node->input_pins[0],cycle);
			signed char pin1 = get_pin_value(node->input_pins[1],cycle);
			signed char pin2 = get_pin_value(node->input_pins[2],cycle);

			if (pin0 < 0 || pin1 < 0 || pin2 < 0)
				update_pin_value(node->output_pins[0], -1, cycle);
			else if (
				   (pin0 == 1 && (pin1 == 1 || pin2 == 1))
				|| (pin1 == 1 && pin2 == 1)
			)
				update_pin_value(node->output_pins[0], 1, cycle);
			else
				update_pin_value(node->output_pins[0], 0, cycle);

			break;
		}
		case NOT_EQUAL:	  // !=
		case LOGICAL_XOR: // ^
		{
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			int ones     = 0;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if      (pin <  0) { unknown = TRUE; break; }
				else if (pin == 1) { ones++; }
			}
			if      (unknown)         update_pin_value(node->output_pins[0], -1, cycle);
			else if ((ones % 2) == 1) update_pin_value(node->output_pins[0],  1, cycle);
			else                      update_pin_value(node->output_pins[0],  0, cycle);
			break;
		}
		case LOGICAL_EQUAL:	// ==
		case LOGICAL_XNOR:  // !^
		{
			verify_i_o_availabilty(node, -1, 1);
			char unknown = FALSE;
			int ones = 0;
			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				signed char pin = get_pin_value(node->input_pins[i], cycle);

				if (pin <  0) { unknown = TRUE; break; }
				if (pin == 1) { ones++; }
			}
			if      (unknown)         update_pin_value(node->output_pins[0], -1, cycle);
			else if ((ones % 2) == 1) update_pin_value(node->output_pins[0],  0, cycle);
			else                      update_pin_value(node->output_pins[0],  1, cycle);
			break;
		}
		case BITWISE_NOT:
		{
			verify_i_o_availabilty(node, 1, 1);

			signed char pin = get_pin_value(node->input_pins[0], cycle);

			if      (pin  < 0) update_pin_value(node->output_pins[0], -1, cycle);
			else if (pin == 1) update_pin_value(node->output_pins[0],  0, cycle);
			else               update_pin_value(node->output_pins[0],  1, cycle);
			break;
		}
		case CLOCK_NODE:
		{
			if(node->num_input_pins == 0) // driven by file or internally
			{
				verify_i_o_availabilty(node, -1, 1);

				/* if the pin is not an input.. find a clock to drive it.*/
				int pin_cycle = get_pin_cycle(node->output_pins[0]);
				if(pin_cycle != cycle)
				{
					if(!node->internal_clk_warn)
					{
						node->internal_clk_warn = true;
						warning_message(SIMULATION_ERROR,-1,-1,"clock(%s) is internally driven, verify your circuit", node->name);
					}
					//toggle according to ratio
					signed char prev_value = !CLOCK_INITIAL_VALUE;
					if(cycle)
						prev_value = get_pin_value(node->output_pins[0], cycle-1);

					if(prev_value < 0)
						prev_value = !CLOCK_INITIAL_VALUE;

					signed char cur_value = (cycle % get_clock_ratio(node)) ? prev_value : !prev_value;
					update_pin_value(node->output_pins[0], cur_value, cycle);
				}
			}
			else // driven by another node
			{
				verify_i_o_availabilty(node, 1, 1);

				int pin_cycle = get_pin_cycle(node->input_pins[0]);
				if(pin_cycle == cycle)
				{
					update_pin_value(node->output_pins[0], get_pin_value(node->input_pins[0],cycle), cycle);
				}
				else
				{
					if(!node->internal_clk_warn)
					{
						node->internal_clk_warn = true;
						warning_message(SIMULATION_ERROR,-1,-1,"node used as clock (%s) is itself driven by a clock, verify your circuit", node->name);
					}
					update_pin_value(node->output_pins[0], get_pin_value(node->input_pins[0],cycle-1), cycle);
				}
			}
			break;
		}
		case GND_NODE:
			verify_i_o_availabilty(node, -1, 1);
			update_pin_value(node->output_pins[0], 0, cycle);
			break;
		case VCC_NODE:
			verify_i_o_availabilty(node, -1, 1);
			update_pin_value(node->output_pins[0], 1, cycle);
			break;
		case PAD_NODE:
			verify_i_o_availabilty(node, -1, 1);
			update_pin_value(node->output_pins[0], 0, cycle);
			break;
		case INPUT_NODE:
			break;
		case OUTPUT_NODE:
			verify_i_o_availabilty(node, 1, 1);
			update_pin_value(node->output_pins[0], get_pin_value(node->input_pins[0],cycle), cycle);
			break;
		case HARD_IP:
			compute_hard_ip_node(node,cycle);
			break;
		case GENERIC :
			compute_generic_node(node,cycle);
			break;
		//case FULLADDER:
		case ADD:
			compute_add_node(node, cycle, 0);
			break;
		case MINUS:
			if(node->num_input_port_sizes == 3)
				compute_add_node(node, cycle, 1);
			else
				compute_unary_sub_node(node, cycle);
			break;
		/* These should have already been converted to softer versions. */
		case BITWISE_AND:
		case BITWISE_NAND:
		case BITWISE_NOR:
		case BITWISE_XNOR:
		case BITWISE_XOR:
		case BITWISE_OR:
		case BUF_NODE:
		case MULTI_PORT_MUX:
		case SL:
		case SR:
        case ASR:
		case CASE_EQUAL:
		case CASE_NOT_EQUAL:
		case DIVIDE:
		case MODULO:
		case GTE:
		case LTE:
		//case ADD:
		//case MINUS:
		default:
			error_message(SIMULATION_ERROR, 0, -1, "Node should have been converted to softer version: %s", node->name);
			break;
	}

	// Count number of ones and toggles for activity estimation
	bool covered = true;
	bool skip_node_from_coverage = (
		type == INPUT_NODE ||
		type == CLOCK_NODE ||
		type == GND_NODE ||
		type == VCC_NODE ||
		type == PAD_NODE
	);

	if(!skip_node_from_coverage)
	{
		for (int i = 0; i < node->num_output_pins; i++) {
			if ( node->output_pins[i]->ace_info != NULL ) {

				signed char pin_value = get_pin_value(node->output_pins[i],cycle);
				// last_pin_value = get_pin_value(node->output_pins[i],cycle-1);
				// Pin values for cycle-1 were not correct on Wave boundaries. Needed to store it in ace object.
				signed char last_pin_value = node->output_pins[i]->ace_info->value;

				// # of ones
				if ( pin_value == 1 ) 
				{
					node->output_pins[i]->ace_info->num_ones += pin_value;
				}

				// # of toggles
				if ( ( pin_value != last_pin_value ) && (last_pin_value != -1 ) ) 
				{
					node->output_pins[i]->ace_info->num_toggles++;
					node->output_pins[i]->coverage++;
					if(node->output_pins[i]->coverage < 2)
						covered = false;
				}

				node->output_pins[i]->ace_info->value = pin_value;
			}
		}
	}
	if(covered || skip_node_from_coverage)
		node->covered = true;

	//computation_time = wall_time() - computation_time;

	//printf("Node %s typeof %ld spent %lf\n",node->name,type,computation_time);
	return true;
}



/*
 * Gets the number of nodes whose output pins have been sufficiently covered.
 */
static int get_num_covered_nodes(stages_t *s)
{
	int covered_nodes = 0;
	for(int i = 0; i < s->count; i++)
		for (int j = 0; j < s->counts[i]; j++)
			covered_nodes += (s->stages[i][j]->covered)? 1: 0;

	return covered_nodes;
}

/*
 * Determines if the given node has been simulated for the given cycle.
 */
static int is_node_complete(nnode_t* node, int cycle)
{
	int i;
	for (i = 0; i < node->num_output_pins; i++)
		if (node->output_pins[i] && (get_pin_cycle(node->output_pins[i]) < cycle))
			return FALSE;

	return TRUE;
}

/*
 * Changes the ratio of a clock node
 */
void set_clock_ratio(int rat, nnode_t *node)
{
	//change the value only for clocks
	if(!is_clock_node(node))
	 return;

	node->ratio = rat;

}

/*
 * get the ratio of a clock node
 */
int get_clock_ratio(nnode_t *node)
{
	//change the value only for clocks
	if(!is_clock_node(node))
		return 0;

	return node->ratio;
}


/*Gets the parents of the given node. Return the number of
* parents via the num_parents parameter.*/
//maria
nnode_t **get_parents_of(nnode_t *node, int *num_parents)
{
	nnode_t **parents = 0;
	int count = 0;
	int i;

	for (i = 0; i < node->num_input_pins; i++)
	{
		npin_t *pin = node->input_pins[i];
		nnet_t *net = pin->net;

		if (pin && net && net->driver_pin->node)
		{
			nnode_t *parent_node = net->driver_pin->node;
			//char *parent_node_name = get_pin_name(parent_node->name);

			parents = (nnode_t **)vtr::realloc(parents, sizeof(nnode_t*) * (count + 1));
			parents[count++] = parent_node;
		}
	}
	*num_parents = count;
	return parents;
}

/*
 * Gets the children of the given node. Returns the number of
 * children via the num_children parameter. Throws warnings
 * or errors if invalid connection patterns are detected.
 */
nnode_t **get_children_of(nnode_t *node, int *num_children)
{
	nnode_t **children = 0;
	int count = 0;
	int i;
	for (i = 0; i < node->num_output_pins; i++)
	{
		npin_t *pin = node->output_pins[i];
		nnet_t *net = pin->net;
		if (net)
		{
			/*
			 *  Detects a net that may be being driven by two
			 *  or more pins or has an incorrect driver pin assignment.
			 */
			if (net->driver_pin != pin && global_args.all_warnings)
			{
				char *pin_name  = get_pin_name(pin->name);
				char *node_name = get_pin_name(node->name);
				char *net_name  = get_pin_name(net->name);

				warning_message(SIMULATION_ERROR, -1, -1,
						"Found output pin \"%s\" (%ld) on node \"%s\" (%ld)\n"
						"             which is mapped to a net \"%s\" (%ld) whose driver pin is \"%s\" (%ld) \n",
						pin_name,
						pin->unique_id,
						node_name,
						node->unique_id,
						net_name,
						net->unique_id,
						net->driver_pin->name,
						net->driver_pin->unique_id
				);

				vtr::free(net_name);
				vtr::free(pin_name);
				vtr::free(node_name);
			}

			int j;
			for (j = 0; j < net->num_fanout_pins; j++)
			{
				npin_t *fanout_pin = net->fanout_pins[j];
				if (fanout_pin && fanout_pin->type == INPUT && fanout_pin->node)
				{
					nnode_t *child_node = fanout_pin->node;

					// Check linkage for inconsistencies.
					if (fanout_pin->net != net)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}
					else if (fanout_pin->net->driver_pin->net != net)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}

					else if (fanout_pin->net->driver_pin->node != node)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}
					else
					{
						// Add child.
						children = (nnode_t **)vtr::realloc(children, sizeof(nnode_t*) * (count + 1));
						children[count++] = child_node;
					}
				}
			}
		}
	}
	*num_children = count;
	return children;
}

/*
 * Gets the children of the given node. Returns the number of
 * children via the num_children parameter. Throws warnings
 * or errors if invalid connection patterns are detected.
 */
static int *get_children_pinnumber_of(nnode_t *node, int *num_children)
{
	int *pin_numbers = 0;
	int count = 0;
	int i;
	for (i = 0; i < node->num_output_pins; i++)
	{
		npin_t *pin = node->output_pins[i];
		nnet_t *net = pin->net;
		if (net)
		{
			/*
			 *  Detects a net that may be being driven by two
			 *  or more pins or has an incorrect driver pin assignment.
			 */
			if (net->driver_pin != pin && global_args.all_warnings)
			{
				char *pin_name  = get_pin_name(pin->name);
				char *node_name = get_pin_name(node->name);
				char *net_name  = get_pin_name(net->name);

				warning_message(SIMULATION_ERROR, -1, -1,
						"Found output pin \"%s\" (%ld) on node \"%s\" (%ld)\n"
						"             which is mapped to a net \"%s\" (%ld) whose driver pin is \"%s\" (%ld) \n",
						pin_name,
						pin->unique_id,
						node_name,
						node->unique_id,
						net_name,
						net->unique_id,
						net->driver_pin->name,
						net->driver_pin->unique_id
				);

				vtr::free(net_name);
				vtr::free(pin_name);
				vtr::free(node_name);
			}

			int j;
			for (j = 0; j < net->num_fanout_pins; j++)
			{
				npin_t *fanout_pin = net->fanout_pins[j];
				if (fanout_pin && fanout_pin->type == INPUT && fanout_pin->node)
				{
					nnode_t *child_node = fanout_pin->node;

					// Check linkage for inconsistencies.
					if (fanout_pin->net != net)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}
					else if (fanout_pin->net->driver_pin->net != net)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}

					else if (fanout_pin->net->driver_pin->node != node)
					{
						print_ancestry(child_node, 0);
						error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
					}
					else
					{
						// Add child.
						pin_numbers = (int *)vtr::realloc(pin_numbers, sizeof(int) * (count + 1));
						pin_numbers[count++] = i;
					}
				}
			}
		}
	}
	*num_children = count;
	return pin_numbers;
}

/*
 * Gets the children of a specific output pin of the given node. Returns the number of
 * children via the num_children parameter. Throws warnings
 * or errors if invalid connection patterns are detected.
 */
nnode_t **get_children_of_nodepin(nnode_t *node, int *num_children, int output_pin)
{
	nnode_t **children = 0;
	int count = 0;
	int output_pin_number = node->num_output_pins;
	if(output_pin < 0 || output_pin > output_pin_number)
	{
		error_message(SIMULATION_ERROR, -1, -1, "%s", "Requested pin not available");
		return children;
	}

	npin_t *pin = node->output_pins[output_pin];
	nnet_t *net = pin->net;
	if (net)
	{
		/*
		 *  Detects a net that may be being driven by two
		 *  or more pins or has an incorrect driver pin assignment.
		 */
		if (net->driver_pin != pin && global_args.all_warnings)
		{
			char *pin_name  = get_pin_name(pin->name);
			char *node_name = get_pin_name(node->name);
			char *net_name  = get_pin_name(net->name);

			warning_message(SIMULATION_ERROR, -1, -1,
					"Found output pin \"%s\" (%ld) on node \"%s\" (%ld)\n"
					"             which is mapped to a net \"%s\" (%ld) whose driver pin is \"%s\" (%ld) \n",
					pin_name,
					pin->unique_id,
					node_name,
					node->unique_id,
					net_name,
					net->unique_id,
					net->driver_pin->name,
					net->driver_pin->unique_id
			);

			vtr::free(net_name);
			vtr::free(pin_name);
			vtr::free(node_name);
		}

		int j;
		for (j = 0; j < net->num_fanout_pins; j++)
		{
			npin_t *fanout_pin = net->fanout_pins[j];
			if (fanout_pin && fanout_pin->type == INPUT && fanout_pin->node)
			{
				nnode_t *child_node = fanout_pin->node;

				// Check linkage for inconsistencies.
				if (fanout_pin->net != net)
				{
					print_ancestry(child_node, 0);
					error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
				}
				else if (fanout_pin->net->driver_pin->net != net)
				{
					print_ancestry(child_node, 0);
					error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
				}

				else if (fanout_pin->net->driver_pin->node != node)
				{
					print_ancestry(child_node, 0);
					error_message(SIMULATION_ERROR, -1, -1, "Found mismapped node %s", node->name);
				}
				else
				{
					// Add child.
					children = (nnode_t **)vtr::realloc(children, sizeof(nnode_t*) * (count + 1));
					children[count++] = child_node;
				}
			}
		}
	}

	*num_children = count;
	return children;
}

/*
 * Allocates memory for the pin's value and cycle.
 *
 * Checks to see if this pin's net has a different driver, and
 * initialises that pin too.
 *
 * Fanout pins will share the same memory locations for cycle
 * and values so that the values don't have to be propagated
 * through the net.
 */
static void initialize_pin(npin_t *pin)
{
	// Initialise the driver pin if this pin is not the driver.
	if (pin->net && pin->net->driver_pin && pin->net->driver_pin != pin)
		initialize_pin(pin->net->driver_pin);

	// If initialising the driver initialised this pin, we're OK to return.
	if (pin->values)
		return;

	if (pin->net)
	{
		if(!pin->net->values)
		{
			pin->net->values = std::make_shared<AtomicBuffer>(init_value(pin->node));
		}

		pin->values = pin->net->values;

		for (int i = 0; i < pin->net->num_fanout_pins; i++)
			if (pin->net->fanout_pins[i])
				pin->net->fanout_pins[i]->values = pin->net->values;
	}
	else
	{
		pin->values = std::make_shared<AtomicBuffer>(init_value(pin->node));
	}
}

/*
 * Updates the value of a pin and its cycle. Pins should be updated using
 * only this function.
 *
 * Initializes the pin if need be.
 */
static void update_pin_value(npin_t *pin, signed char value, int cycle)
{
	if (pin->values == NULL)
		initialize_pin(pin);
	
	pin->values->update_value(value, cycle);
}

/*
 * Gets the value of a pin. Pins should be checked using this function only.
 */
signed char get_pin_value(npin_t *pin, int cycle)
{
	if (pin->values == NULL)
	{
		initialize_pin(pin);
	}
	return pin->values->get_value(cycle);
}

/*
 * Gets the cycle of the given pin
 */
static int get_pin_cycle(npin_t *pin)
{
	if (pin->values == NULL)
		initialize_pin(pin);

	return pin->values->get_cycle();
}

/*
 * Computes a node of type FF_NODE for the given cycle.
 */
static void compute_flipflop_node(nnode_t *node, int cycle)
{
	verify_i_o_availabilty(node, 2, 1);

	npin_t *D      	= 	node->input_pins[0];
	npin_t *Q		=	node->output_pins[0];
	npin_t *clock_pin 	=	node->input_pins[1];
	npin_t *output_pin	=	node->output_pins[0];
	bool trigger = ff_trigger(node->edge_type, clock_pin, cycle);

	signed char new_value = compute_ff(trigger, D, Q, cycle);
	update_pin_value(output_pin, new_value, cycle);
}

/*
 * Computes a node of type MUX_2 for the given cycle.
 */
static void compute_mux_2_node(nnode_t *node, int cycle)
{
	verify_i_o_availabilty(node, -1, 1);
	oassert(node->num_input_port_sizes >= 2);
	oassert(node->input_port_sizes[0] == node->input_port_sizes[1]);

	ast_node_t *ast_node = node->related_ast_node;

	// Figure out which pin is being selected.
	char unknown = FALSE;
	int select = -1;
	int default_select = -1;
	int i;
	for (i = 0; i < node->input_port_sizes[0]; i++)
	{
		npin_t *pin = node->input_pins[i];
		signed char value = get_pin_value(pin, cycle);

		if      (value  < 0)
			unknown = TRUE;
		else if (value == 1 && select == -1) // Take the first selection only.
			select = i;

		/*
		 *  If the pin comes from an "else" condition or a case "default" condition,
		 *  we favour it in the case where there are unknowns.
		 */
		if (ast_node && pin->is_default && (ast_node->type == IF || ast_node->type == CASE))
			default_select = i;
	}

	// If there are unknowns and there is a default clause, select it.
	if (unknown && default_select >= 0)
	{
		unknown = FALSE;
		select = default_select;
	}

	npin_t *output_pin = node->output_pins[0];

	// If any select pin is unknown (and we don't have a default), we take the value from the previous cycle.
	if (unknown)
	{
		/*
		 *  Conform to ModelSim's behaviour where in-line ifs are concerned. If the
		 *  condition is unknown, the inline if's output is unknown.
		 */
		if (ast_node && ast_node->type == IF_Q)
			update_pin_value(output_pin, -1, cycle);
		else
			update_pin_value(output_pin, get_pin_value(output_pin, cycle-1), cycle);
	}
	// If no selection is made (all 0) we output x.
	else if (select < 0)
	{
		update_pin_value(output_pin, -1, cycle);
	}
	else
	{
		npin_t *pin = node->input_pins[select + node->input_port_sizes[0]];
		signed char value = get_pin_value(pin,cycle);

		// Drive implied drivers to unknown value.
		/*if (pin->is_implied && ast_node && (ast_node->type == CASE))
			update_pin_value(output_pin, -1, cycle);
		else*/
			update_pin_value(output_pin, value, cycle);
	}
}



// TODO: Needs to be verified.
static void compute_hard_ip_node(nnode_t *node, int cycle)
{
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

#ifndef _WIN32
	int *input_pins = (int *)vtr::malloc(sizeof(int)*node->num_input_pins);
	int *output_pins = (int *)vtr::malloc(sizeof(int)*node->num_output_pins);

	if (!node->simulate_block_cycle)
	{
		char *filename = (char *)vtr::malloc(sizeof(char)*strlen(node->name));

		if (!strchr(node->name, '.'))
			error_message(SIMULATION_ERROR, 0, -1, "%s\n", 
					"Couldn't extract the name of a shared library for hard-block simulation");

		snprintf(filename, sizeof(char)*strlen(node->name), "%s.so", strchr(node->name, '.')+1);

		void *handle = dlopen(filename, RTLD_LAZY);

		if (!handle)
			error_message(SIMULATION_ERROR, 0, -1,
					"Couldn't open a shared library for hard-block simulation: %s", dlerror());

		dlerror();

		void (*func_pointer)(int, int, int*, int, int*) =
				(void(*)(int, int, int*, int, int*))dlsym(handle, "simulate_block_cycle");

		char *error = dlerror();
		if (error)
			error_message(SIMULATION_ERROR, 0, -1,
					"Couldn't load a shared library method for hard-block simulation: %s", error);

		node->simulate_block_cycle = func_pointer;

		vtr::free(filename);
	}

	int i;
	for (i = 0; i < node->num_input_pins; i++)
		input_pins[i] = get_pin_value(node->input_pins[i],cycle);

	(node->simulate_block_cycle)
			(cycle, node->num_input_pins, input_pins, node->num_output_pins, output_pins);

	for (i = 0; i < node->num_output_pins; i++)
		update_pin_value(node->output_pins[i], output_pins[i], cycle);

	vtr::free(input_pins);
	vtr::free(output_pins);

#else
	//Not supported
	oassert(false);
#endif
}

/*
 * Computes the given multiply node for the given cycle.
 */
static void compute_multiply_node(nnode_t *node, int cycle)
{
	oassert(node->num_input_port_sizes == 2);
	oassert(node->num_output_port_sizes == 1);

	int i;
	char unknown = FALSE;
	for (i = 0; i < node->input_port_sizes[0] + node->input_port_sizes[1]; i++)
	{
		signed char pin = get_pin_value(node->input_pins[i],cycle);
		if (pin < 0)
		{
			unknown = TRUE;
			break;
		}
	}

	if (unknown)
	{
		for (i = 0; i < node->num_output_pins; i++)
			update_pin_value(node->output_pins[i], -1, cycle);
	}
	else
	{
		int *a = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[0]);
		int *b = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[1]);

		for (i = 0; i < node->input_port_sizes[0]; i++)
			a[i] = get_pin_value(node->input_pins[i],cycle);

		for (i = 0; i < node->input_port_sizes[1]; i++)
			b[i] = get_pin_value(node->input_pins[node->input_port_sizes[0] + i],cycle);

		int *result = multiply_arrays(a, node->input_port_sizes[0], b, node->input_port_sizes[1]);

		for (i = 0; i < node->num_output_pins; i++)
			update_pin_value(node->output_pins[i], result[i], cycle);

		vtr::free(result);
		vtr::free(a);
		vtr::free(b);
	}

}

// TODO: Needs to be verified.
static void compute_generic_node(nnode_t *node, int cycle)
{
	int line_count_bitmap = node->bit_map_line_count;
	char **bit_map = node->bit_map;

	int lut_size  = 0;
	while (bit_map[0][lut_size] != 0)
		lut_size++;

	int found = 0;
	int i;
	for (i = 0; i < line_count_bitmap && (!found); i++)
	{
		int j;
		for (j = 0; j < lut_size; j++)
		{
			if (get_pin_value(node->input_pins[j],cycle) < 0)
			{
				update_pin_value(node->output_pins[0], -1, cycle);
				return;
			}

			if ((bit_map[i][j] != '-') && (bit_map[i][j]-'0' != get_pin_value(node->input_pins[j],cycle)))
				break;
		}

		if (j == lut_size) found = TRUE;
	}

	if (node->generic_output == 1){
		if (found) update_pin_value(node->output_pins[0], 1, cycle);
		else       update_pin_value(node->output_pins[0], 0, cycle);
	} else {
		if (found) update_pin_value(node->output_pins[0], 0, cycle);
		else       update_pin_value(node->output_pins[0], 1, cycle);
	}
}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their product. The
 * length of the returned array is twice that of the two parameters.
 *
 * This array will need to be freed later!
 */
static int *multiply_arrays(int *a, int a_length, int *b, int b_length)
{
	int result_size = a_length + b_length;
	int *result = (int *)vtr::calloc(sizeof(int), result_size);

	int i;
	for (i = 0; i < a_length; i++)
	{
		if (a[i] == 1)
		{
			int j;
			for (j = 0; j < b_length; j++)
				result[i+j] += b[j];
		}
	}
	for (i = 0; i < result_size; i++)
	{
		while (result[i] > 1)
		{
			result[i] -= 2;
			result[i+1]++;
		}
	}
	return result;
}

/*
 * Computes the given add node for the given cycle.
 * add by Sen
 */
static void compute_add_node(nnode_t *node, int cycle, int type)
{
	oassert(node->num_input_port_sizes == 3);
	oassert(node->num_output_port_sizes == 2);

	int i, num;
	int flag = 0;

	int *a = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[0]);
	int *b = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[1]);
	int *c = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[2]);

	num = node->input_port_sizes[0]+ node->input_port_sizes[1];
	//if cin connect to unconn(PAD_NODE), a[0] connect to ground(GND_NODE) and b[0] connect to ground, flag = 0 the initial adder for addition
	//if cin connect to unconn(PAD_NODE), a[0] connect to ground(GND_NODE) and b[0] connect to vcc, flag = 1 the initial adder for subtraction
	if(node->input_pins[num]->net->driver_pin->node->type == PAD_NODE)
	{
		if(node->input_pins[0]->net->driver_pin->node->type == GND_NODE && node->input_pins[node->input_port_sizes[0]]->net->driver_pin->node->type == GND_NODE)
			flag = 0;
		else if(node->input_pins[0]->net->driver_pin->node->type == GND_NODE && node->input_pins[node->input_port_sizes[0]]->net->driver_pin->node->type == VCC_NODE)
			flag = 1;
	}
	else
		flag = 2;

	for (i = 0; i < node->input_port_sizes[0]; i++)
		a[i] = get_pin_value(node->input_pins[i],cycle);
	for (i = 0; i < node->input_port_sizes[1]; i++)
		b[i] = get_pin_value(node->input_pins[node->input_port_sizes[0] + i],cycle);

	for (i = 0; i < node->input_port_sizes[2]; i++)
		//the initial cin of carry chain subtractor should be 1
		if(flag == 1)
			c[i] = 1;
		//the initial cin of carry chain adder should be 0
		else if(flag == 0)
			c[i] = 0;
		else
			c[i] = get_pin_value(node->input_pins[node->input_port_sizes[0]+ node->input_port_sizes[1] + i],cycle);

	int *result = add_arrays(a, node->input_port_sizes[0], b, node->input_port_sizes[1], c, node->input_port_sizes[2],type);

	//update the pin value of output
	for (i = 1; i < node->num_output_pins; i++)
		update_pin_value(node->output_pins[i], result[(i - 1)], cycle);

	update_pin_value(node->output_pins[0], result[(node->num_output_pins - 1)], cycle);

	vtr::free(result);
	vtr::free(a);
	vtr::free(b);
	vtr::free(c);

}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their sum. The
 * length of the returned array is the maximum of the two parameters plus one.
 * add by Sen
 * This array will need to be freed later!
 */
static int *add_arrays(int *a, int a_length, int *b, int b_length, int *c, int /*c_length*/, int /*flag*/)
{
	int result_size = std::max(a_length , b_length) + 1;
	int *result = (int *)vtr::calloc(sizeof(int), result_size);

	//least significant bit would use the input carryIn, the other bits would use the compute value
	//if one of the number is unknown, then the answer should be unknown(same as ModelSim)
	if(a[0] == -1 || b[0] == -1 || c[0] == -1)
	{
		result[0] = -1;
		result[1] = -1;
	}
	else
	{
		result[0] = a[0] ^ b[0] ^ c[0];
		result[1] = (a[0] & b[0]) | (c[0] & b[0]) | (a[0] & c[0]);
	}

	int temp_carry_in = result[1];
	if(result_size > 2){
		for(int i = 1; i < std::min(a_length,b_length); i++)
		{
			if(a[i] == -1 || b[i] == -1 || temp_carry_in == -1)
			{
				result[i] = -1;
				result[i+1] = -1;
			}
			else
			{
				result[i] = a[i] ^ b[i] ^ temp_carry_in;
				result[i+1] = (a[i] & b[i]) | (a[i] & temp_carry_in) | (temp_carry_in & b[i]);
			}
			temp_carry_in = result[i+1];
		}
		if(a_length >= b_length)
		{
			for(int i = b_length; i < a_length; i++)
			{
				if(a[i] == -1 || temp_carry_in == -1)
				{
					result[i] = -1;
					result[i+1] = -1;
				}
				else
				{
					result[i] = a[i] ^ temp_carry_in;
					result[i+1] = a[i] & temp_carry_in;
				}
				temp_carry_in = result[i+1];
			}
		}
		else
		{
			for(int i = a_length; i < b_length; i++)
			{
				if(b[i] == -1 || temp_carry_in == -1)
				{
					result[i] = -1;
					result[i+1] = -1;
				}else
				{
					result[i] = b[i] ^ temp_carry_in;
					result[i+1] = b[i] & temp_carry_in;
				}
				temp_carry_in = result[i+1];
			}
		}
	}
	return result;
}

/*
 * Computes the given add node for the given cycle.
 * add by Sen
 */
static void compute_unary_sub_node(nnode_t *node, int cycle)
{
	oassert(node->num_input_port_sizes == 2);
	oassert(node->num_output_port_sizes == 2);

	int i;
	char unknown = FALSE;
	for (i = 0; i < (node->input_port_sizes[0] + node->input_port_sizes[1]); i++)
	{
		signed char pin = get_pin_value(node->input_pins[i],cycle);
		if (pin < 0)
		{
			unknown = TRUE;
			break;
		}
	}

	if (unknown)
	{
		for (i = 0; i < (node->output_port_sizes[0] + node->output_port_sizes[1]); i++)
			update_pin_value(node->output_pins[i], -1, cycle);
	}
	else
	{
		int *a = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[0]);
		int *c = (int *)vtr::malloc(sizeof(int)*node->input_port_sizes[1]);

		for (i = 0; i < node->input_port_sizes[0]; i++)
			a[i] = get_pin_value(node->input_pins[i],cycle);

		for (i = 0; i < node->input_port_sizes[1]; i++)
			if(node->input_pins[node->input_port_sizes[0]+ node->input_port_sizes[1] + i]->net->driver_pin->node->type == PAD_NODE)
				c[i] = 1;
			else
				c[i] = get_pin_value(node->input_pins[node->input_port_sizes[0] + i],cycle);

		int *result = unary_sub_arrays(a, node->input_port_sizes[0], c, node->input_port_sizes[1]);


		for (i = 1; i < node->num_output_pins; i++)
			update_pin_value(node->output_pins[i], result[(i - 1)], cycle);

		update_pin_value(node->output_pins[0], result[(node->num_output_pins - 1)], cycle);

		vtr::free(result);
		vtr::free(a);
		vtr::free(c);
	}

}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their sum. The
 * length of the returned array is the maximum of the two parameters plus one.
 * add by Sen
 * This array will need to be freed later!
 */
static int *unary_sub_arrays(int *a, int a_length, int *c, int /*c_length*/)
{
	int result_size = a_length + 1;
	int *result = (int *)vtr::calloc(sizeof(int), result_size);

	int i;
	int temp_carry_in;

	c[0] = 1;
	result[0] = (!a[0]) ^ c[0] ^ 0;
	result[1] = ((!a[0]) & 0) | (c[0] & 0) | ((!a[0]) & c[0]);

	temp_carry_in = result[1];
	if(result_size > 2){
		for(i = 1; i < a_length; i++)
		{
			result[i] = (!a[i]) ^ 0 ^ temp_carry_in;
			result[i+1] = ((!a[i]) & 0) | ((!a[i]) & temp_carry_in) | (temp_carry_in & 0);
			temp_carry_in = result[i+1];
		}
	}
	return result;
}

/*
 * Computes the given memory node.
 */
static void compute_memory_node(nnode_t *node, int cycle)
{
	if (is_sp_ram(node))
		compute_single_port_memory(node, cycle);
	else if (is_dp_ram(node))
		compute_dual_port_memory(node, cycle);
	else
		error_message(SIMULATION_ERROR, 0, -1,
				"Could not resolve memory hard block %s to a valid type.", node->name);
}

/**
 * compute the address 
 */
static long compute_address(signal_list_t *input_address, int cycle)
{
	long address = 0;
	for (long i = 0; i < input_address->count && address >= 0; i++)
	{
		// If any address pins are x's, write x's we return -1.
		signed char value = get_pin_value(input_address->pins[i],cycle);
		if (value != 1 && value != 0)
			address = -1;
		else
			address += shift_left_value_with_overflow_check(value, i);
	}
	return address;
}

static void read_write_to_memory(nnode_t *node , signal_list_t *input_address, signal_list_t *data_out, signal_list_t *data_in, bool trigger, npin_t *write_enabled, int cycle)
{

	long address = compute_address(input_address, cycle);
	
	 //make a single trigger out of write_enable pin and if it was a positive edge
	 
	bool write = (trigger && 1 == get_pin_value(write_enabled, cycle));
	bool address_is_valid = (address >= 0 && address < node->memory_data.size());

	/* init the vector with all -1 */
	std::vector<signed char> new_values(data_out->count, -1);
	if(address_is_valid)
	{
		// init from memory pins first from previous value
		new_values = node->memory_data[address];

		// if it is a valid write, grap the input pin and store those in a vector
		if (write)
		{
			for (long i = 0; i < data_out->count; i++)
			{
				new_values[i]= get_pin_value(data_in->pins[i],cycle-1);
			}
		}


		if (false == global_args.parralelized_simulation_in_batch)
		{
			node->memory_data[address] = new_values;
		}
		/**
		 * use dictionnary when there are multiple workers
		 */
		else
		{
			/********* Critical section */
			/* LOCK */node->memory_mtx.lock();
			if(write)
			{
				if ( node->memory_directory.find(cycle) == node->memory_directory.end() ) 
				{
					node->memory_directory[cycle] = {};
				}
				node->memory_directory[cycle][address]= new_values;
			}
			/**
			 * read from the dictionary if exist otherwise use current mem_data value
			 */
			else /* READ */
			{
				for ( auto it = node->memory_directory.begin(); it != node->memory_directory.end(); it++ )
				{
					int recorded_cycle = it->first;
					if (recorded_cycle <= cycle
					&& ( node->memory_directory[recorded_cycle].find(address) != node->memory_directory[recorded_cycle].end() ))
					{
						new_values = node->memory_directory[recorded_cycle][address];
					}
				}
			}
			/* UNLOCK */node->memory_mtx.unlock();
		}
	}

	/**
	 * now we update the output pins
	 */
	for (long i = 0; i < data_out->count; i++)
	{
		update_pin_value(data_out->pins[i], new_values[i], cycle);
	}
}


static void write_back_memory_nodes(nnode_t **nodes, int num_nodes)
{
 	int num;
	//printf("here\n");
 	for(num=0;num<num_nodes;num++)
 	{
 		nnode_t* node = nodes[num];
 		if (!node->memory_directory.empty())
 		{
			std::map<int,std::map<long,std::vector<signed char>>>::iterator it;
			for ( it = node->memory_directory.begin(); it != node->memory_directory.end(); it++ )
			{
				int recorded_cycle = it->first;
				std::map<long,std::vector<signed char>>::iterator cit;
				for ( cit = node->memory_directory[recorded_cycle].begin(); cit != node->memory_directory[recorded_cycle].end(); cit++ )
				{
					long address = cit->first;
					std::vector<signed char> new_values = cit->second;
					node->memory_data[address] = new_values;
				}
				node->memory_directory[recorded_cycle] = {};
			}
			node->memory_directory={};
		}
	}
}

/*
 * Computes single port memory.
 */
static void compute_single_port_memory(nnode_t *node, int cycle)
{
	sp_ram_signals *signals = get_sp_ram_signals(node);

	bool trigger = ff_trigger(RISING_EDGE_SENSITIVITY, signals->clk, cycle);
	
	if (node->memory_data.empty())
		instantiate_memory(node, signals->data->count, signals->addr->count);


	read_write_to_memory(node, signals->addr, signals->out, signals->data, trigger, signals->we, cycle);

	free_sp_ram_signals(signals);
}

/*
 * Computes dual port memory.
 */
static void compute_dual_port_memory(nnode_t *node, int cycle)
{
	dp_ram_signals *signals = get_dp_ram_signals(node);
	bool trigger = ff_trigger(RISING_EDGE_SENSITIVITY, signals->clk, cycle);

	if (node->memory_data.empty())
		instantiate_memory(node, 
			std::max(signals->data1->count, signals->data2->count), 
			std::max(signals->addr1->count,signals->addr2->count)
		);


	read_write_to_memory(node, signals->addr1, signals->out1, signals->data1, trigger, signals->we1, cycle);
	read_write_to_memory(node, signals->addr2, signals->out2, signals->data2, trigger, signals->we2, cycle);

	free_dp_ram_signals(signals);
}

/*
 * Initialises memory using a memory information file (mif). If not
 * file is found, it is initialised to x's.
 */
static void instantiate_memory(nnode_t *node, long data_width, long addr_width)
{
	long max_address = shift_left_value_with_overflow_check(0x1, addr_width);
	node->memory_data = std::vector<std::vector<signed char>>(max_address, std::vector<signed char>(data_width, init_value(node)));
	if(global_args.read_mif_input)
	{
		char *filename = get_mif_filename(node);
		FILE *mif = fopen(filename, "r");
		if (!mif)
		{
			printf("MIF %s (%ldx%ld) not found. \n", filename, data_width, addr_width);
		}
		else
		{
			assign_memory_from_mif_file(node, mif, filename, data_width, addr_width);
			fclose(mif);
		}
		vtr::free(filename);
	}
}

/*
 * Removes white space (except new lines) and comments from
 * the given mif file and returns the resulting temporary file.
 */
static FILE *preprocess_mif_file(FILE *source)
{
	FILE *destination = tmpfile();
	destination = freopen(NULL, "r+", destination);
	rewind(source);

	char line[BUFFER_MAX_SIZE];
	int in_multiline_comment = FALSE;
	while (fgets(line, BUFFER_MAX_SIZE, source))
	{
		unsigned int i;
		for (i = 0; i < strlen(line); i++)
		{
			if (!in_multiline_comment)
			{
				// For a single line comment, skip the rest of the line.
				if (line[i] == '-' && line[i+1] == '-')
					break;
				// Start of a multiline comment
				else if (line[i] == '%')
					in_multiline_comment = TRUE;
				// Don't copy any white space over.
				else if (line[i] != '\n' && line[i] != ' ' && line[i] != '\r' && line[i] != '\t' )
					fputc(line[i], destination);
			}
			else
			{
				// If we're in a multi-line comment, search for the %
				if (line[i] == '%')
					in_multiline_comment = FALSE;
			}
		}
		fputc('\n', destination);
	}
	rewind(destination);
	return destination;
}

static int parse_mif_radix(std::string radix)
{
		return 	(radix == "HEX")	?	16:
				(radix == "DEC")	?	10:
				(radix == "OCT")	?	8:
				(radix == "BIN")	?	2:
										0;
}

static void assign_memory_from_mif_file(nnode_t *node, FILE *mif, char *filename, int width, long address_width)
{
	FILE *file = preprocess_mif_file(mif);
	rewind(file);

	std::unordered_map<std::string, std::string> symbols = std::unordered_map<std::string, std::string>();

	char buffer_in[BUFFER_MAX_SIZE];
	bool in_content = false;
	std::string last_line;
	int line_number = 0;

	int addr_radix = 0;
	int data_radix = 0;
	while (fgets(buffer_in, BUFFER_MAX_SIZE, file))
	{
		line_number++;
		// Remove the newline.
		trim_string(buffer_in, "\n");
		std::string buffer = buffer_in;
		// Only process lines which are not empty.
		if (buffer.size())
		{
			// MIF files are case insensitive
			string_to_upper(buffer_in);

			// The content section of the file contains address:value; assignments.
			if (in_content)
			{
				// Parse at the :
				char *token = strtok(buffer_in, ":");
				if (strlen(token))
				{
					// END; signifies the end of the file.
					if(buffer ==  "END;")
						break;

					// The part before the : is the address.
					char *address_string = token;
					token = strtok(NULL, ";");
					// The reset (before the ;) is the data_value.
					char *data_string = token;

					if (token)
					{
						// Make sure the address and value are valid strings of the specified radix.
						if (!is_string_of_radix(address_string, addr_radix))
							error_message(SIMULATION_ERROR, line_number, -1, "%s: address %s is not a base %ld string.", filename, address_string, addr_radix);

						if (!is_string_of_radix(data_string, data_radix))
							error_message(SIMULATION_ERROR, line_number, -1, "%s: data string %s is not a base %ld string.", filename, data_string, data_radix);

						char *binary_data = convert_string_of_radix_to_bit_string(data_string, data_radix, width);
						long address = convert_string_of_radix_to_long(address_string, addr_radix);

						if (address > address_width)
							error_message(SIMULATION_ERROR, line_number, -1, "%s: address %s is out of range.", filename, address_string);

						// Write the parsed value string to the memory location.
						for(int i=0; i<width; i++)
							node->memory_data[address][i] = binary_data[i] - '0';
					}
					else
					{
						error_message(SIMULATION_ERROR, line_number, -1,
								"%s: MIF syntax error.", filename);
					}
				}

			}
			// The header section of the file contains parameters given as PARAMETER=value;
			else
			{
				// Grab the bit before the = sign.
				char *token = strtok(buffer_in, "=");
				if (strlen(token))
				{
					char *symbol = token;
					token = strtok(NULL, ";");

					// If is something after the equals sign and before the semicolon, add the symbol=value association to the symbol table.
					if (token)
						symbols.insert({symbol, token});
					else if(buffer == "CONTENT") {}
					// We found "CONTENT" followed on the next line by "BEGIN". That means we're at the end of the parameters.
					else if(buffer == "BEGIN" && last_line == "CONTENT")
					{
						// Sanity check parameters to make sure we have what we need.

						std::unordered_map<std::string,std::string>::const_iterator item_in;

						// Verify the width parameter.
						item_in = symbols.find("WIDTH");
						if ( item_in == symbols.end() ) 
							error_message(SIMULATION_ERROR, -1, -1, "%s: MIF WIDTH parameter unspecified.", filename);

						long mif_width = std::strtol(item_in->second.c_str(),NULL,10);
						if (mif_width != width)
							error_message(SIMULATION_ERROR, -1, -1, "%s: MIF width mismatch: must be %ld but %ld was given", filename, width, mif_width);


						// Verify the depth parameter.
						item_in = symbols.find("DEPTH");
						if ( item_in == symbols.end() ) 
							error_message(SIMULATION_ERROR, -1, -1, "%s: MIF DEPTH parameter unspecified.", filename);
						
						long mif_depth = std::strtol(item_in->second.c_str(),NULL,10);
						long expected_depth = shift_left_value_with_overflow_check(0x1, address_width);
						if (mif_depth != expected_depth)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: MIF depth mismatch: must be %ld but %ld was given", filename, expected_depth, mif_depth);


						// Parse the radix specifications and make sure they're OK.
						item_in = symbols.find("ADDRESS_RADIX");
						if ( item_in == symbols.end() ) 
							error_message(SIMULATION_ERROR, -1, -1, "%s: ADDRESS_RADIX parameter unspecified.", filename);
						addr_radix = parse_mif_radix(item_in->second);
						if (!addr_radix)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: invalid or missing ADDRESS_RADIX: must specify DEC, HEX, OCT, or BIN", filename);


						item_in = symbols.find("DATA_RADIX");
						if ( item_in == symbols.end() ) 
							error_message(SIMULATION_ERROR, -1, -1, "%s: DATA_RADIX parameter unspecified.", filename);
						data_radix = parse_mif_radix(item_in->second);
						if (!data_radix)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: invalid or missing DATA_RADIX: must specify DEC, HEX, OCT, or BIN", filename);

						// If everything checks out, start reading the values.
						in_content = true;
					}
					else
					{
						error_message(SIMULATION_ERROR, line_number, -1, "%s: MIF syntax error: %s", filename, buffer_in);
					}
				}
				else
				{
					error_message(SIMULATION_ERROR, line_number, -1, "%s: MIF syntax error: %s", filename, buffer_in);
				}
			}

			last_line = buffer;
		}
	}

	fclose(file);
}

/*
 * Assigns the given node to its corresponding line in the given array of line.
 * Assumes the line has already been created.
 */
static void assign_node_to_line(nnode_t *node, lines_t *l, int type, int single_pin)
{
	// Make sure the node has an output pin.
	if (!node->num_output_pins)
	{
		npin_t *pin = allocate_npin();
		allocate_more_output_pins(node, 1);
		add_output_pin_to_node(node, pin, 0);
	}

	// Parse the node name into a pin number and a port name.
	int pin_number = get_pin_number(node->name);
	char *port_name;
	if (pin_number != -1 && !single_pin) {
		port_name = get_port_name(node->name);
	}
	else {
		port_name = get_pin_name(node->name);
		single_pin = TRUE;
	}
	// Search the lines for the port name.
	int j = find_portname_in_lines(port_name, l);
	vtr::free(port_name);

	if (single_pin)
	{
		if (j == -1)
		{
			warning_message(SIMULATION_ERROR, 0, -1,
					"Could not map single-bit node '%s' line", node->name);
		}
		else
		{
			pin_number = (pin_number == -1)?0:pin_number;
			int already_added = l->lines[j]->number_of_pins >= 1;
			if (!already_added)
				insert_pin_into_line(node->output_pins[0], pin_number, l->lines[j], type);
		}
	}
	else
	{
		if (j == -1)
			warning_message(SIMULATION_ERROR, 0, -1,
					"Could not map multi-bit node '%s' to line", node->name);
		else
			insert_pin_into_line(node->output_pins[0], pin_number, l->lines[j], type);
	}
}

/*
 * Inserts the given pin according to its pin number into the given line.
 */
static void insert_pin_into_line(npin_t *pin, int pin_number, line_t *line, int type)
{
	// Allocate memory for the new pin.
	line->pins        = (npin_t **)vtr::realloc(line->pins,        sizeof(npin_t*) * (line->number_of_pins + 1));
	line->pin_numbers = (int *)vtr::realloc(line->pin_numbers, sizeof(npin_t*) * (line->number_of_pins + 1));

	// Find the proper place to insert this pin, and make room for it.
	int i;
	for (i = 0; i < line->number_of_pins; i++)
	{
		if (line->pin_numbers[i] > pin_number)
		{
			// Move other pins to the right to make room.
			int j;
			for (j = line->number_of_pins; j > i; j--)
			{
				line->pins[j] = line->pins[j-1];
				line->pin_numbers[j] = line->pin_numbers[j-1];
			}
			break;
		}
	}
	// Add the new pin.
	line->pins[i] = pin;
	line->pin_numbers[i] = pin_number;
	line->type = type;
	line->number_of_pins++;
}

/*
 * Given a netlist, this function maps the top_input_nodes
 * or top_output_nodes depending on the value of type
 * (INPUT or OUTPUT) to a line_t each. It stores them in a
 * lines_t struct and returns a pointer to it.
 */
static lines_t *create_lines(netlist_t *netlist, int type)
{
	lines_t *l = (lines_t *)vtr::malloc(sizeof(lines_t));
	l->lines = 0;
	l->count = 0;

	int num_nodes   = (type == INPUT)?netlist->num_top_input_nodes:netlist->num_top_output_nodes;
	nnode_t **nodes = (type == INPUT)?netlist->top_input_nodes    :netlist->top_output_nodes;

	int i;
	for (i = 0; i < num_nodes; i++)
	{
		nnode_t *node = nodes[i];
		char *port_name = get_port_name(node->name);

		if (find_portname_in_lines(port_name, l) == -1)
		{
			line_t *line = create_line(port_name);
			l->lines = (line_t **)vtr::realloc(l->lines, sizeof(line_t *)*(l->count + 1));
			l->lines[l->count++] = line;
		}
		assign_node_to_line(node, l, type, 0);
		/**
		 * TODO: implicit memories with multiclock input (one for read and one for write)
		 * is broken, need fixing
		 */
		if(is_clock_node(node))
			set_clock_ratio(++num_of_clock,node);

		vtr::free(port_name);
	}
	return l;
}

/*
 * Creates a vector file header from the given lines,
 * and writes it to the given file.
 */
static void write_vector_headers(FILE *file, lines_t *l)
{
	char* headers = generate_vector_header(l);
	fprintf(file, "%s", headers);
	vtr::free(headers);
	fflush(file);
}

/*
 * Parses the first line of the given file and compares it to the
 * given lines for identity. If there is any difference, a warning is printed,
 * and FALSE is returned. If there are no differences, the file pointer is left
 * at the start of the second line, and TRUE is returned.
 */
static int verify_test_vector_headers(FILE *in, lines_t *l)
{
	int current_line = 0;
	int buffer_length = 0;

	// Read the header from the vector file.
	char read_buffer [BUFFER_MAX_SIZE];
	rewind(in);
	if (!get_next_vector(in, read_buffer))
		error_message(SIMULATION_ERROR, 0, -1, "%s\n", "Failed to read vector headers.");

	// Parse the header, checking each entity against the corresponding line.
	char buffer [BUFFER_MAX_SIZE];
	buffer[0] = '\0';
	unsigned int i;
	for (i = 0; i < strlen(read_buffer) && i < BUFFER_MAX_SIZE; i++)
	{
		char next = read_buffer[i];

		if (next == EOF)
		{
			warning_message(SIMULATION_ERROR, 0, -1, "%s", "Hit end of file.");
			return FALSE;
		}
		else if (next == ' ' || next == '\t' || next == '\n')
		{
			if (buffer_length)
			{
				if(strcmp(l->lines[current_line]->name,buffer))
				{
					char *expected_header = generate_vector_header(l);
					warning_message(SIMULATION_ERROR, 0, -1,
							"Vector header mismatch: \n "
							"  Found:    %s "
							"  Expected: %s", read_buffer, expected_header);
					vtr::free(expected_header);
					return FALSE;
				}
				else
				{
					buffer_length = 0;
					current_line++;
				}
			}

			if (next == '\n')
				break;
		}
		else
		{
			buffer[buffer_length++] = next;
			buffer[buffer_length] = '\0';
		}
	}
	return TRUE;
}

/*
 * Verifies that no lines have null pins.
 */
static int verify_lines (lines_t *l)
{
	int i;
	for (i = 0; i < l->count; i++)
	{
		int j;
		for (j = 0; j < l->lines[i]->number_of_pins; j++)
		{
			if (!l->lines[i]->pins[j])
			{
				warning_message(SIMULATION_ERROR, 0, -1, "A line %ld:(%s) has a NULL pin. ", j, l->lines[i]->name);
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*
 * Searches for a line with the given name in the lines. Returns the index
 * or -1 if no such line was found.
 */
static int find_portname_in_lines(char* port_name, lines_t *l)
{
	int j;
	for (j = 0; j < l->count; j++)
		if (!strcmp(l->lines[j]->name, port_name))
			return  j;

	return -1;
}

/*
 * allocates memory for and initialises a line_t struct
 */
static line_t *create_line(char *name)
{
	line_t *line = (line_t *)vtr::malloc(sizeof(line_t));

	line->number_of_pins = 0;
	line->pins = 0;
	line->pin_numbers = 0;
	line->type = -1;
	line->name = (char *)vtr::malloc(sizeof(char)*(strlen(name)+1));

	strcpy(line->name, name);

	return line;
}

/*
 * Generates the appropriate vector headers based on the given lines.
 */
static char *generate_vector_header(lines_t *l)
{
	char *header = (char *)vtr::calloc(BUFFER_MAX_SIZE, sizeof(char));
	if (l->count)
	{
		int j;
		for (j = 0; j < l->count; j++)
		{
			// "+ 2" for null and newline/space.
			if ((strlen(header) + strlen(l->lines[j]->name) + 2) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "%s", "Buffer overflow anticipated while generating vector header.");

			strcat(header,l->lines[j]->name);
			strcat(header," ");
		}
		header[strlen(header)-1] = '\n';
	}
	else
	{
		header[0] = '\n';
	}
	header = (char *)vtr::realloc(header, sizeof(char)*(strlen(header)+1));
	return header;
}

/*
 * Stores the given test vector in the given lines, with some sanity checking to ensure that it
 * has a compatible geometry.
 */
static void add_test_vector_to_lines(test_vector *v, lines_t *l, int cycle)
{
	if (l->count < v->count)
		error_message(SIMULATION_ERROR, 0, -1, "Fewer lines (%ld) than values (%ld).", l->count, v->count);
	if (l->count > v->count)
		error_message(SIMULATION_ERROR, 0, -1, "More lines (%ld) than values (%ld).", l->count, v->count);

	int i;
	for (i = 0; i < v->count; i++)
	{
		line_t *line = l->lines[i];

		if (line->number_of_pins < 1)
			error_message(SIMULATION_ERROR, 0, -1, "Found a line '%s' with no pins.", line->name);

		int j;
		for (j = 0; j < line->number_of_pins; j++)
		{
			if (j < v->counts[i]) update_pin_value(line->pins[j], v->values[i][j], cycle);
			else                  update_pin_value(line->pins[j], 0, cycle);
		}
	}
}

/*
 * Compares two test vectors for numerical and geometric identity. Returns FALSE if
 * they are found to be different, and TRUE otherwise.
 */
static int compare_test_vectors(test_vector *v1, test_vector *v2)
{
	int equivalent = TRUE;
	if (v1->count != v2->count)
	{
		warning_message(SIMULATION_ERROR, 0, -1, "%s", "Vector lengths differ.");
		return FALSE;
	}

	int l;
	for (l = 0; l < v1->count; l++)
	{	// Compare bit by bit.
		int i;
		for (i = 0; i < v1->counts[l] && i < v2->counts[l]; i++)
		{
			if (v1->values[l][i] != v2->values[l][i])
			{
				if (v1->values[l][i] == -1)
					equivalent = -1;
				else
					return FALSE;
			}
		}

		/*
		 *  If one value has more bits than the other, they are still
		 *  equivalent as long as the higher order bits of the longer
		 *  one are zero.
		 */
		if (v1->counts[l] != v2->counts[l])
		{
			test_vector *v = v1->counts[l] < v2->counts[l] ? v2 : v1;
			int j;
			for (j = i; j < v->counts[l]; j++)
				if (v->values[l][j] != 0)
					return FALSE;
		}
	}
	return equivalent;
}

/*
 * Parses the given line from a test vector file into a
 * test_vector data structure.
 */
static test_vector *parse_test_vector(char *buffer)
{
	buffer = vtr::strdup(buffer);
	test_vector *v = (test_vector *)vtr::malloc(sizeof(test_vector));
	v->values = 0;
	v->counts = 0;
	v->count  = 0;

	trim_string(buffer,"\r\n");

	const char *delim = " \t";
	char *token = strtok(buffer, delim);
	while (token)
	{
		v->values = (signed char **)vtr::realloc(v->values, sizeof(signed char *) * (v->count + 1));
		v->counts = (int *)vtr::realloc(v->counts, sizeof(int) * (v->count + 1));
		v->values[v->count] = 0;
		v->counts[v->count] = 0;

		if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
		{	// Value is hex.
			token += 2;
			int token_length = strlen(token);
			reverse_string(token, token_length);
			int i;
			for (i = 0; i < token_length; i++)
			{
				char temp[] = {token[i],'\0'};

				int value = strtol(temp, NULL, 16);
				int k;
				for (k = 0; k < 4; k++)
				{
						signed char bit = 0;
						if (value > 0)
						{
							bit = value % 2;
							value /= 2;
						}
						v->values[v->count] = (signed char *)vtr::realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));
						v->values[v->count][v->counts[v->count]++] = bit;
				}
			}
		}
		else
		{	// Value is binary.
			int i;
			for (i = strlen(token) - 1; i >= 0; i--)
			{
				signed char value = -1;
				if      (token[i] == '0') value = 0;
				else if (token[i] == '1') value = 1;

				v->values[v->count] = (signed char *)vtr::realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));
				v->values[v->count][v->counts[v->count]++] = value;
			}
		}
		v->count++;
		token = strtok(NULL, delim);
	}
	vtr::free(buffer);
	return v;
}

/*
 * Generates a "random" test_vector structure based on the geometry of the given lines.
 *
 * If you want better randomness, call srand at some point.
 */
static bool contains_a_substr_of_name(std::vector<std::string> held, const char *name_in)
{
	if(!name_in)
		return false;
	
	if(held.empty())
		return false;

	std::string name = name_in;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	for(std::string sub_str: held)
	{
		std::transform(sub_str.begin(), sub_str.end(), sub_str.begin(), ::tolower);
		if(name.find(sub_str) != std::string::npos)
			return true;
	}
	return false;
}

static test_vector *generate_random_test_vector(int cycle, sim_data_t *sim_data)
{
	/**
	 * generate test vectors
	 */
	test_vector *v = (test_vector *)vtr::malloc(sizeof(test_vector));
	v->values = 0;
	v->counts = 0;
	v->count = 0;

	for (int i = 0; i < sim_data->input_lines->count; i++)
	{
		v->values = (signed char **)vtr::realloc(v->values, sizeof(signed char *) * (v->count + 1));
		v->counts = (int *)vtr::realloc(v->counts, sizeof(int) * (v->count + 1));
		v->values[v->count] = 0;
		v->counts[v->count] = 0;

		line_t *line = sim_data->input_lines->lines[i];
		for (int j = 0; j < line->number_of_pins; j++)
		{
			//default
			signed char value = (rand() % 2);

			npin_t *pin = line->pins[j];
			signed char clock_ratio = get_clock_ratio(pin->node);

			/********************************************************
			 * if it is a clock node, use it's ratio to generate a cycle
			 */
			if(clock_ratio > 0)
			{
				if(!cycle)
					value = CLOCK_INITIAL_VALUE;
				else
				{
					signed char previous_cycle_clock_value = get_pin_value(pin, cycle-1);
					if((cycle%(clock_ratio)) == 0)
					{
						if(previous_cycle_clock_value == 0)
							value = 1;
						else
							value = 0;
					}
					else
						value = previous_cycle_clock_value;
				}
			}
			/********************************************************
			 * use input override to set the pin value to hold high if requested
			 */
			else if(contains_a_substr_of_name(global_args.sim_hold_high.value(),line->name))
			{
				if (cycle < (num_of_clock*3)) 	value =	0;	// start with reverse value
				else        	value =	1;	// then hold to requested value				
			}
			/********************************************************
			 * use input override to set the pin value to hold low if requested
			 */
			else if(contains_a_substr_of_name(global_args.sim_hold_low.value(),line->name))
			{
				if (cycle < (num_of_clock*3)) 	value = 1;	// start with reverse value
				else       		value = 0;		// then hold to requested value
			}
			/********************************************************
			 * set the value via the -3 option
			 */
			else if( global_args.sim_generate_three_valued_logic)
			{
				value = (rand() % 3) - 1;
			}
			
			v->values[v->count] = (signed char *)vtr::realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));
			v->values[v->count][v->counts[v->count]++] = value;
		}
		v->count++;
	}
	return v;
}





/*
 * Writes a wave of vectors to the given file. Writes the headers
 * prior to cycle 0.
 *
 * When edge is -1, both edges of the clock are written. When edge is 0,
 * the falling edge is written. When edge is 1, the rising edge is written.
 */
static void write_cycle_to_file(lines_t *l, FILE* file, int cycle)
{
	if (!cycle)
		write_vector_headers(file, l);

	write_vector_to_file(l, file, cycle);
}

/*
 * Writes all values in the given lines to a line in the given file
 * for the given cycle.
 */
static void write_vector_to_file(lines_t *l, FILE *file, int cycle)
{
	std::stringstream buffer;
	int i;
	
	for (i = 0; i < l->count; i++)
	{
		buffer.str(std::string());
		line_t *line = l->lines[i];
		int num_pins = line->number_of_pins;

		if (line_has_unknown_pin(line, cycle) || num_pins == 1)
		{
			if ((num_pins + 1) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "Buffer overflow anticipated while writing vector for line %s.", line->name);

			int j;
			for (j = num_pins - 1; j >= 0 ; j--)
			{
				signed char value = get_line_pin_value(line, j, cycle);
				
				if (value > 1){
					error_message(SIMULATION_ERROR, 0, -1, "Invalid logic value of %ld read from line %s.", value, line->name);
				}else if(value < 0){
					buffer << "x";
				}else{
					buffer << std::dec <<(int)value;
				}
			}
			// If there are no known values, print a single capital X.
			// (Only for testing. Breaks machine readability.)
			//if (!known_values && num_pins > 1)
			//	odin_sprintf(buffer, "X");
		}
		else
		{
			// +1 for ceiling, +1 for null, +2 for "OX"
			if ((num_pins/4 + 1 + 1 + 2) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "Buffer overflow anticipated while writing vector for line %s.", line->name);
			buffer << "0X";

			int hex_digit = 0;
			int j;
			for (j = num_pins - 1; j >= 0; j--)
			{
				signed char value = get_line_pin_value(line,j,cycle);

				if (value > 1)
					error_message(SIMULATION_ERROR, 0, -1, "Invalid logic value of %ld read from line %s.", value, line->name);

				hex_digit += value << j % 4;

				if (!(j % 4))
				{
					buffer << std::hex << hex_digit;
					hex_digit = 0;
				}
			}
		}
		buffer << " ";
		// Expand the value to fill to space under the header. (Gets ugly sometimes.)
		//while (strlen(buffer) < strlen(l->lines[i]->name))
		//	strcat(buffer," ");

		fprintf(file,"%s",buffer.str().c_str());
	}
	fprintf(file, "\n");
}

/*
 * Writes a wave of vectors to the given modelsim out file.
 */
static void write_cycle_to_modelsim_file(netlist_t *netlist, lines_t *l, FILE* modelsim_out, int cycle)
{
	if (!cycle)
	{
		fprintf(modelsim_out, "add wave *\n");

		// Add clocks to the output file.
		int i;
		for (i = 0; i < netlist->num_top_input_nodes; i++)
		{
			nnode_t *node = netlist->top_input_nodes[i];
			if (is_clock_node(node))
			{
				char *port_name = get_port_name(node->name);
				fprintf(modelsim_out, "force %s 0 0, 1 50 -repeat 100\n", port_name);
				vtr::free(port_name);
			}
		}
	}

	write_vector_to_modelsim_file(l, modelsim_out, cycle);
}

/*
 * Writes a vector to the given modelsim out file.
 */
static void write_vector_to_modelsim_file(lines_t *l, FILE *modelsim_out, int cycle)
{
	int i;
	for (i = 0; i < l->count; i++)
	{
		if (line_has_unknown_pin(l->lines[i], cycle) || l->lines[i]->number_of_pins == 1)
		{
			fprintf(modelsim_out, "force %s ",l->lines[i]->name);
			int j;

			for (j = l->lines[i]->number_of_pins - 1; j >= 0 ; j--)
			{
				int value = get_line_pin_value(l->lines[i],j,cycle);

				if (value < 0)  fprintf(modelsim_out, "%s", "x");
				else 		fprintf(modelsim_out, "%d", value);
			}
			fprintf(modelsim_out, " %d\n", cycle/2 * 100);
		}
		else
		{
			int value = 0;
			fprintf(modelsim_out, "force %s 16#", l->lines[i]->name);

			int j;
			for (j = l->lines[i]->number_of_pins - 1; j >= 0; j--)
			{
				if (get_line_pin_value(l->lines[i],j,cycle) > 0)
					value += my_power(2, j % 4);

				if (j % 4 == 0)
				{
					fprintf(modelsim_out, "%X", value);
					value = 0;
				}
			}
			fprintf(modelsim_out, " %d\n", cycle/2 * 100);
		}

	}
}

/*
 * Verify that the given output vector file is identical (numerically)
 * to the one written by the current simulation. This is done by parsing each
 * file vector by vector and comparing them. Also verifies that the headers are identical.
 *
 * Prints appropriate warning messages when differences are found.
 *
 * Returns false if the files differ and true if they are identical, with the exception of
 * number format.
 */
static int verify_output_vectors(char* output_vector_file, int num_vectors)
{
	int error = FALSE;

	// The filename cannot be the same as our default output file.
	if (!strcmp(output_vector_file,OUTPUT_VECTOR_FILE_NAME))
	{
		error = TRUE;
		warning_message(SIMULATION_ERROR,0,-1,
				"Vector file \"%s\" given for verification "
				"is the same as the default output file \"%s\". "
				"Ignoring.", output_vector_file, OUTPUT_VECTOR_FILE_NAME);
	}
	else
	{
		// The file being verified against.
		FILE *existing_out = fopen(output_vector_file, "r");
		if (!existing_out) error_message(SIMULATION_ERROR, 0, -1, "Could not open vector output file: %s", output_vector_file);

		// Our current output vectors. (Just produced.)
		char out_vec_file[BUFFER_MAX_SIZE] = { 0 };
		odin_sprintf(out_vec_file,"%s/%s",((char *)global_args.sim_directory),OUTPUT_VECTOR_FILE_NAME);
		FILE *current_out  = fopen(out_vec_file, "r");
		if (!current_out)
			error_message(SIMULATION_ERROR, 0, -1, "Could not open output vector file: %s", out_vec_file);

		int cycle;
		char buffer1[BUFFER_MAX_SIZE];
		char buffer2[BUFFER_MAX_SIZE];
		// Start at cycle -1 to check the headers.
		for (cycle = -1; cycle < num_vectors; cycle++)
		{
			if (!get_next_vector(existing_out, buffer1))
			{
				error = TRUE;
				warning_message(SIMULATION_ERROR, 0, -1,"Too few vectors in %s \n", output_vector_file);
				break;
			}
			else if (!get_next_vector(current_out, buffer2))
			{
				error = TRUE;
				warning_message(SIMULATION_ERROR, 0, -1,"Simulation produced fewer than %ld vectors. \n", num_vectors);
				break;
			}
			// The headers differ.
			else if ((cycle == -1) && strcmp(buffer1,buffer2))
			{
				error = TRUE;
				warning_message(SIMULATION_ERROR, 0, -1, "Vector headers do not match: \n"
						"\t%s"
						"in %s does not match\n"
						"\t%s"
						"in %s.\n\n",
						buffer2, OUTPUT_VECTOR_FILE_NAME, buffer1, output_vector_file
				);
				break;
			}
			else
			{
				// Parse both vectors.
				test_vector *v1 = parse_test_vector(buffer1);
				test_vector *v2 = parse_test_vector(buffer2);

				int equivalent = compare_test_vectors(v1,v2);
				// Compare them and print an appropriate message if they differ.

				if (!equivalent)
				{
					trim_string(buffer1, "\n\t");
					trim_string(buffer2, "\n\t");
					error = TRUE;
					warning_message(SIMULATION_ERROR, 0, -1, "Vector %ld mismatch:\n"
							"\t%s in %s\n"
							"\t%s in %s\n",
							cycle, buffer2, OUTPUT_VECTOR_FILE_NAME, buffer1, output_vector_file
					);
				}
				else if (equivalent == -1)
				{
					trim_string(buffer1, "\n\t");
					trim_string(buffer2, "\n\t");
					warning_message(SIMULATION_ERROR, 0, -1, "Vector %ld equivalent but output vector has bits set when expecting don't care :\n"
							"\t%s in %s\n"
							"\t%s in %s\n",
							cycle, buffer2, OUTPUT_VECTOR_FILE_NAME, buffer1, output_vector_file
					);
				}

				free_test_vector(v1);
				free_test_vector(v2);
			}
		}

		// If the file we're checking against is longer than the current output, print an appropriate warning.
		if (!error && get_next_vector(existing_out, buffer1))
		{
			error = TRUE;
			warning_message(SIMULATION_ERROR, 0, -1,"%s contains more than %ld vectors.\n", output_vector_file, num_vectors);
		}

		fclose(existing_out);
		fclose(current_out);
	}
	return !error;
}

/*
 * If the given node matches one of the additional names (passed via -p),
 * it's added to the lines. (Matches on output pin names, net names, and node names).
 */
static void add_additional_items_to_lines(nnode_t *node, lines_t *l)
{
	std::vector<std::string> p = global_args.sim_additional_pins.value();
	if (!p.empty())
	{
		int add = FALSE;
		int j, k = 0;

		// Search the output pin names for each user-defined item.
		for (j = 0; j < node->num_output_pins; j++)
		{
			npin_t *pin = node->output_pins[j];

			if (pin->name)
			{
				for (k = 0; k < p.size(); k++)
				{
					if (strstr(pin->name, p[k].c_str()))
					{
						add = TRUE;
						break;
					}
				}
				if (add) break;
			}

			if (pin->net && pin->net->name)
			{
				for (k = 0; k < p.size(); k++)
				{
					if (strstr(pin->net->name, p[k].c_str()))
					{
						add = TRUE;
						break;
					}
				}
				if (add) break;
			}
		}

		// Search the node name for each user defined item.
		if (!add && node->name && strlen(node->name) && strchr(node->name, '^'))
		{
			for (k = 0; k < p.size(); k++)
			{
				if (strstr(node->name, p[k].c_str()))
				{
					add = TRUE;
					break;
				}
			}
		}

		if (add)
		{
			int single_pin = strchr(p[k].c_str(), '~')?1:0;

			if (strchr(node->name, '^'))
			{
				char *port_name;
				if (single_pin)
					port_name = get_pin_name(node->name);
				else
					port_name = get_port_name(node->name);

				if (find_portname_in_lines(port_name, l) == -1)
				{
					line_t *line = create_line(port_name);
					l->lines = (line_t **)vtr::realloc(l->lines, sizeof(line_t *)*((l->count)+1));
					l->lines[l->count++] = line;
				}
				assign_node_to_line(node, l, OUTPUT, single_pin);
				vtr::free(port_name);
			}
		}
	}
}

/*
 * Parses the given (memory) node name into a corresponding
 * mif file name.
 */
static char *get_mif_filename(nnode_t *node)
{
	char buffer[BUFFER_MAX_SIZE];
	buffer[0] = 0;
	strcat(buffer, node->name);

	char *filename = strrchr(buffer, '+');
	if (filename) filename += 1;
	else          filename = buffer;

	strcat(filename, ".mif");

	filename = vtr::strdup(filename);
	return filename;
}

/*
 * Returns TRUE if the given line has a pin for
 * the given cycle whose value is -1.
 */
static int line_has_unknown_pin(line_t *line, int cycle)
{
	int unknown = FALSE;
	int j;
	for (j = line->number_of_pins - 1; j >= 0; j--)
	{
		if (get_line_pin_value(line, j, cycle) < 0)
		{
			unknown = TRUE;
			break;
		}
	}
	return unknown;
}

/*
 * Gets the value of the pin given pin within the given line
 * for the given cycle.
 */
signed char get_line_pin_value(line_t *line, int pin_num, int cycle)
{
	return get_pin_value(line->pins[pin_num],cycle);
}

/*
 * Returns a value from a test_vectors struct in hex. Works
 * for the values arrays in pins as well.
 */
static char *vector_value_to_hex(signed char *value, int length)
{
	char *tmp;
	char *string = (char *)vtr::calloc((length + 1),sizeof(char));
	int j;
	for (j = 0; j < length; j++)
		string[j] = value[j] + '0';

	reverse_string(string,strlen(string));

	char *hex_string = (char *)vtr::malloc(sizeof(char) * ((length/4 + 1) + 1));

	odin_sprintf(hex_string, "%X ", (unsigned int)strtol(string, &tmp, 2));

	vtr::free(string);

	return hex_string;
}

/*
 * Counts the number of vectors in the given file.
 */
static int count_test_vectors(FILE *in)
{
	rewind(in);

	int count = 0;
	char buffer[BUFFER_MAX_SIZE];
	while (get_next_vector(in, buffer))
		count++;

	if (count) // Don't count the headers.
		count--;

	rewind(in);

	return count;
}

/*
 * Counts the number of vectors in the given file.
 */
static int count_empty_test_vectors(FILE *in)
{
	rewind(in);

	int count = 0;
	int buffer;
	while ( (buffer = getc(in)) != EOF )
		if(((char)buffer) == '\n')
			count++;

	if (count) // Don't count the headers.
		count--;

	rewind(in);

	return count;
}

/*
 * A given line is a vector if it contains one or more
 * non-whitespace characters and does not being with a #.
 */
static int is_vector(char *buffer)
{
	char *line = vtr::strdup(buffer);
	trim_string(line," \t\r\n");

	if (line[0] != '#' && strlen(line))
	{
		vtr::free(line);
		return TRUE;
	}
	else
	{
		vtr::free(line);
		return FALSE;
	}
}

/*
 * Gets the next line from the given file that
 * passes the is_vector() test and places it in
 * the buffer. Returns TRUE if a vector was found,
 * and FALSE if no vector was found.
 */
static int get_next_vector(FILE *file, char *buffer)
{
	oassert(file != NULL
		&& "unable to retrieve file for next test vector");
	oassert(buffer != NULL
		&& "unable to use buffer for next test vector as it is not initialized");

	while (fgets(buffer, BUFFER_MAX_SIZE, file))
		if (is_vector(buffer))
			return TRUE;

	return FALSE;
}

/*
 * Free each element in lines[] and the array itself
 */
static void free_lines(lines_t *l)
{
	if(l)
	{
		if(l->lines)
		{
			while (l->count--)
			{
				if(l->lines[l->count])
				{
					if(l->lines[l->count]->name)
						vtr::free(l->lines[l->count]->name);
					if(l->lines[l->count]->pins)
						vtr::free(l->lines[l->count]->pins);
					vtr::free(l->lines[l->count]);
				}
			}

			vtr::free(l->lines);
		}
		vtr::free(l);
	}
}

/*
 * Free stages.
 */
static void free_stages(stages_t *s)
{
	if(s)
	{
		if(s->stages)
		{
			while (s->count--)
			{
				if(s->stages[s->count])
					vtr::free(s->stages[s->count]);
			}
			vtr::free(s->stages);
		}

		if(s->counts)
		{
			vtr::free(s->counts);
		}
		vtr::free(s);
	}
}

//maria
//Free thread distribution

static void free_thread_distribution(thread_node_distribution *thread_distribution)
{
	if(thread_distribution)
	{
		if(thread_distribution->thread_nodes)
		{
			for(int i = 0; i < thread_distribution->number_of_threads; i++)
			{
				if(thread_distribution->thread_nodes[i])
				{
					for (int j=0;j<thread_distribution->thread_nodes[i]->number_of_nodes;j++)
					{
						if(thread_distribution->thread_nodes[i]->nodes[j])
							vtr::free(thread_distribution->thread_nodes[i]->nodes[j]);
					}

					if(thread_distribution->thread_nodes[i]->nodes)
						vtr::free(thread_distribution->thread_nodes[i]->nodes);

					vtr::free(thread_distribution->thread_nodes[i]);
				}
			}
			vtr::free(thread_distribution->thread_nodes);
		}
		vtr::free(thread_distribution);
	}
}


/*
 * Free the given test_vector.
 */
static void free_test_vector(test_vector* v)
{
	if(v)
	{
		if(v->values)
		{
			while (v->count--)
			{
				if(v->values[v->count])
					vtr::free(v->values[v->count]);
			}
			vtr::free(v->values);
		}

		if(v->counts)
			vtr::free(v->counts);
		vtr::free(v);
	}
}

/*
 * Prints information about the netlist we are simulating.
 */
static void print_netlist_stats(stages_t *stages, int /*num_vectors*/)
{
	if(configuration.list_of_file_names.size() == 0)
		printf("%s:\n", (char*)global_args.blif_file);
	else
		for(long i=0; i < configuration.list_of_file_names.size(); i++)
			printf("%s:\n", configuration.list_of_file_names[i].c_str());

	printf("  Nodes:           %d\n",    stages->num_nodes);
	printf("  Connections:     %d\n",    stages->num_connections);
	printf("  Threads:         %d\n",   number_of_workers);
	printf("  Degree:          %3.2f\n", stages->num_connections/(float)stages->num_nodes);
	printf("  Stages:          %d\n",    stages->count);
	printf("  Nodes/thread:    %d(%4.2f%%)\n", (stages->num_nodes/number_of_workers), 100.0/(double)number_of_workers);
	printf("\n");

}

/*
 * Prints statistics. (Coverage and times.)
 */
static void print_simulation_stats(stages_t *stages, int /*num_vectors*/, double total_time, double simulation_time)
{
	int covered_nodes = get_num_covered_nodes(stages);
	printf("Simulation time:   ");
	print_time(simulation_time);
	printf("\n");
	printf("Elapsed time:      ");
	print_time(total_time);
	printf("\n");
	printf("Coverage:          "
			"%d (%4.1f%%)\n", covered_nodes, (covered_nodes/(double)stages->num_nodes) * 100);
}

/*
 * Prints n ancestors of the given node, complete
 * with their parents, ids, etc.
 */
static void print_ancestry(nnode_t *bottom_node, int n)
{
	if (!n) n = 10;

	std::queue<nnode_t *> queue = std::queue<nnode_t *>();
	queue.push(bottom_node);
	printf(  "  ------------\n");
	printf(  "  BACKTRACE\n");
	printf(  "  ------------\n");

	while (n-- && !queue.empty())
	{
		nnode_t *node = queue.front();
		queue.pop();
		char *name = get_pin_name(node->name);
		printf("  %s (%ld):\n", name, node->unique_id);
		vtr::free(name);
		int i;
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];
			nnet_t *net = pin->net;
			nnode_t *node2 = net->driver_pin->node;
			queue.push(node2);
			char *name2 = get_pin_name(node2->name);
			printf("\t%s %s (%ld)\n", pin->mapping, name2, node2->unique_id);fflush(stdout);
			vtr::free(name2);
		}

		/*int count = 0;
		{
			printf(  "  ------------\n");
			printf(  "  CHILDREN    \n");
			printf(  "  ------------\n");
			printf(  "  O: %ld\n", node->num_output_pins);fflush(stdout);
			for (i = 0; i < node->num_output_pins; i++)
			{
				npin_t *pin = node->output_pins[i];
				if (pin)
				{
					nnet_t *net = pin->net;
					if (net)
					{
						int j;
						for (j = 0; j < net->num_fanout_pins; j++)
						{
							npin_t  *pin  = net->fanout_pins[j];
							if (pin)
							{
								nnode_t *node = pin->node;
								if (node)
								{
									char *name = get_pin_name(node->name);
									printf("\t%s %s (%ld)\n", pin->mapping, name, node->unique_id);fflush(stdout);
									vtr::free(name);
								}
								else
								{
									printf("  Null node %ld\n", ++count);fflush(stdout);
								}
							}
							else
							{
								printf("  Null fanout pin\n");fflush(stdout);
							}
						}
					}
					else
					{
						printf("  Null net\n");fflush(stdout);
					}
				}
			}
		}*/
		printf(  "  ------------\n");

	}

	printf(  "  END OF TRACE\n");
	printf(  "  ------------\n");
}

/*
 * Traces an node which is failing to update back to parent of
 * the earliest node in the net list with an unupdated pin.
 * (Pin whose cycle is less than cycle-1). Prints all nodes
 * traversed during the trace with the original node printed
 * first, and the root of the issue printed last.
 *
 * Returns the root node for inspection.
 */
static nnode_t *print_update_trace(nnode_t *bottom_node, int cycle)
{
	// Limit the length of the trace. 0 for unlimited.
	const int max_depth = 0;
	printf(
			"  --------------------------------------------------------------------------\n"
			"  --------------------------------------------------------------------------\n"
			"  BACKTRACE:\n"
			"\tFormat: Each node is listed followed by its parents. The parent \n"
			"\twhich is not updating is indicated with an astrisk. (*)\n"
			"\tEach node is listed in the form:\n"
			"\t\t(<mapping>) <Name> (<Unique ID>) <# of Parents> <# of Children> \n"
			"  --------------------------------------------------------------------------\n"
	);

	nnode_t *root_node = NULL;

	// Used to detect cycles. Based table size of max depth. If unlimited, set to something big.
	std::unordered_set<long> index;

	std::queue<nnode_t *> queue = std::queue<nnode_t *>();
	queue.push(bottom_node);
	int depth = 0;
	// Traverse the netlist in reverse, starting with our current location.
	while (!queue.empty())
	{
		nnode_t *node = queue.front();
		queue.pop();
		int found_undriven_pin = FALSE;
		if (index.find(node->unique_id) == index.end())
		{
			depth++;
			index.insert(node->unique_id);
			char *name = get_pin_name(node->name);
			printf("  %s (%ld) %ld %ld\n", name, node->unique_id, node->num_input_pins, node->num_output_pins);
			vtr::free(name);

			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				npin_t *pin = node->input_pins[i];
				nnet_t *net = pin->net;
				nnode_t *node2 = net->driver_pin->node;

				// If an input is found which hasn't been updated since before cycle-1, traverse it.
				int is_undriven = FALSE;
				if (get_pin_cycle(pin) < cycle-1)
				{
					// Only add each node for traversal once.
					if (!found_undriven_pin)
					{
						found_undriven_pin = TRUE;
						queue.push(node2);
					}
					is_undriven = TRUE;
				}
				char *name2 = get_pin_name(node2->name);
				printf("\t(%s) %s (%ld) %ld %ld %s \n", pin->mapping, name2, node2->unique_id, node2->num_input_pins, node2->num_output_pins, is_undriven?"*":"");
				vtr::free(name2);
			}
			printf("  ------------\n");
		}
		else {
			printf("  CYCLE DETECTED AFTER %d NODES.\n", depth);
			printf("  ------------\n");
			break;
		}

		if (!found_undriven_pin)
		{
			printf("  TOP OF TRACE\n");
			printf("  ------------\n");
			root_node = node;
			break;
		}
		else if (max_depth && depth >=max_depth)
		{
			printf("  TRACE TRUNCATED AT %d NODES.\n", max_depth);
			printf("  ------------\n");
			break;
		}
	}
	return root_node;
}
