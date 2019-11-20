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
#include "simulate_blif.h"

#ifndef max
#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))
#endif

/*
 * Performs simulation. 
 */ 
void simulate_netlist(netlist_t *netlist)
{
	printf("Beginning simulation.\n"); fflush(stdout);

	// Create and verify the lines.
	lines_t *input_lines = create_lines(netlist, INPUT);
	if (!verify_lines(input_lines))
		error_message(SIMULATION_ERROR, 0, -1, "Input lines could not be assigned.");

	lines_t *output_lines = create_lines(netlist, OUTPUT);
	if (!verify_lines(output_lines))
		error_message(SIMULATION_ERROR, 0, -1, "Output lines could not be assigned.");

	// Open the output vector file.
	FILE *out = fopen(OUTPUT_VECTOR_FILE_NAME, "w");
	if (!out)
		error_message(SIMULATION_ERROR, 0, -1, "Could not open output vector file.");

	// Open the input vector file.
	FILE *in_out  = fopen( INPUT_VECTOR_FILE_NAME, "w");
	if (!in_out)
		error_message(SIMULATION_ERROR, 0, -1, "Could not open input vector file.");

	// Open the modelsim vector file.
	FILE *modelsim_out = fopen("test.do", "w");
	if (!modelsim_out)
		error_message(SIMULATION_ERROR, 0, -1, "Could not open modelsim output file.");

	FILE *in  = NULL;
	int num_vectors;
	// Passed via the -t option.
	char *input_vector_file  = global_args.sim_vector_input_file;

	// Input vectors can either come from a file or be randomly generated.
	if (input_vector_file)
	{
		in = fopen(input_vector_file, "r");
		if (!in)
			error_message(SIMULATION_ERROR, 0, -1, "Could not open vector input file: %s", input_vector_file);

		num_vectors = count_test_vectors(in);

		// Read the vector headers and check to make sure they match the lines.
		if (!verify_test_vector_headers(in, input_lines))
			error_message(SIMULATION_ERROR, 0, -1, "Invalid vector header format in %s.", input_vector_file);

		printf("Simulating %d existing vectors from \"%s\".\n", num_vectors, input_vector_file); fflush(stdout);
	}
	else
	{
		// Passed via the -g option.
		num_vectors = global_args.sim_num_test_vectors;
		printf("Simulating %d new vectors.\n", num_vectors); fflush(stdout);
	}

	// Determine which edge(s) we are outputting.
	int output_edge;
	if      (global_args.sim_output_both_edges ) output_edge = -1; // Both edges
	else if (global_args.sim_output_rising_edge) output_edge =  1; // Rising edge only
	else                                         output_edge =  0; // Falling edge only

	if (!num_vectors)
	{
		error_message(SIMULATION_ERROR, 0, -1, "No vectors to simulate.");
	}
	else
	{
		printf("\n");

		int       progress_bar_position = -1;
		const int progress_bar_length   = 50;

		double total_time      = 0; // Includes I/O
		double simulation_time = 0; // Does not include I/O

		stages *stages = 0;

		// Parse -L and -H options containing lists of pins to hold high or low during random vector generation.
		pin_names *hold_high = parse_pin_name_list(global_args.sim_hold_high);
		pin_names *hold_low  = parse_pin_name_list(global_args.sim_hold_low);
		hashtable_t *hold_high_index = index_pin_name_list(hold_high);
		hashtable_t *hold_low_index  = index_pin_name_list(hold_low);

		/*
		 * Simulation is done in "waves" of SIM_WAVE_LENGTH cycles at a time.
		 * Every second cycle gets a input new vector.
		 */
		int  num_cycles = num_vectors * 2;
		int  num_waves = ceil(num_cycles / (double)SIM_WAVE_LENGTH);
		int  wave;
		test_vector *v = 0;
		for (wave = 0; wave < num_waves; wave++)
		{
			double wave_start_time = wall_time();

			int cycle_offset = SIM_WAVE_LENGTH * wave;
			int wave_length  = (wave < (num_waves-1))?SIM_WAVE_LENGTH:(num_cycles - cycle_offset);

			// Assign vectors to lines, either by reading or generating them.
			// Every second cycle gets a new vector.
			int cycle;
			for (cycle = cycle_offset; cycle < cycle_offset + wave_length; cycle++)
			{
				if (is_even_cycle(cycle))
				{
					if (input_vector_file)
					{
						char buffer[BUFFER_MAX_SIZE];

						if (!get_next_vector(in, buffer))
							error_message(SIMULATION_ERROR, 0, -1, "Could not read next vector.");

						v = parse_test_vector(buffer);
					}
					else
					{
						v = generate_random_test_vector(input_lines, cycle, hold_high_index, hold_low_index);
					}
				}

				add_test_vector_to_lines(v, input_lines, cycle);

				if (!is_even_cycle(cycle))
					free_test_vector(v);
			}

			// Record the input vectors we are using.
			write_wave_to_file(input_lines, in_out, cycle_offset, wave_length, 1);
			// Write ModelSim script.
			write_wave_to_modelsim_file(netlist, input_lines, modelsim_out, cycle_offset, wave_length);

			double simulation_start_time = wall_time();

			// Perform simulation
			for (cycle = cycle_offset; cycle < cycle_offset + wave_length; cycle++)
			{
				//original_simulate_cycle(netlist, cycle);

				if (cycle)
				{
					simulate_cycle(cycle, stages);
				}
				else
				{
					// The first cycle produces the stages, and adds additional
					// lines as specified by the -p option.
					pin_names *p = parse_pin_name_list(global_args.sim_additional_pins);
					stages = simulate_first_cycle(netlist, cycle, p, output_lines);
					free_pin_name_list(p);
					// Make sure the output lines are still OK after adding custom lines.
					if (!verify_lines(output_lines))
						error_message(SIMULATION_ERROR, 0, -1,
								"Problem detected with the output lines after the first cycle.");
				}
			}

			simulation_time += wall_time() - simulation_start_time;

			// Write the result of this wave to the output vector file.
			write_wave_to_file(output_lines, out, cycle_offset, wave_length, output_edge);

			total_time += wall_time() - wave_start_time;

			// Print netlist-specific statistics.
			if (!cycle_offset)
				print_netlist_stats(stages, num_vectors);

			// Delay drawing of the progress bar until the second wave to improve the accuracy of the ETA.
			if ((num_waves == 1) || cycle_offset)
				progress_bar_position = print_progress_bar(
						cycle/(double)num_cycles, progress_bar_position, progress_bar_length, total_time);
		}

		free_pin_name_list(hold_high);
		free_pin_name_list(hold_low);
		hold_high_index->destroy_free_items(hold_high_index);
		hold_low_index ->destroy_free_items(hold_low_index);

		fflush(out); 
		fprintf(modelsim_out, "run %d\n", num_vectors*100);

		printf("\n");
		// If a second output vector file was given via the -T option, verify that it matches.
		char *output_vector_file = global_args.sim_vector_output_file;
		if (output_vector_file)
		{
			if (verify_output_vectors(output_vector_file, num_vectors))
				printf("Vector file \"%s\" matches output\n", output_vector_file);
			else
				error_message(SIMULATION_ERROR, 0, -1, "Vector files differ.");
			printf("\n");
		}

		// Print statistics.
		print_simulation_stats(stages, num_vectors, total_time, simulation_time);

		free_stages(stages);
	}

	free_lines(output_lines);
	free_lines(input_lines);

	fclose(modelsim_out);
	fclose(in_out);
	if (input_vector_file)
		fclose(in);
	fclose(out);
}

/*
 * This simulates a single cycle using the stages generated
 * during the first cycle. Simulates in parallel if OpenMP is enabled.
 *
 * OpenMP simulation computes a small number of cycles sequentially and
 * a small number in parallel. The minimum parallel and sequential time is
 * taken for each stage, and that stage is computed in parallel for all subsequent
 * cycles if speedup is observed.
 */
void simulate_cycle(int cycle, stages *s)
{
	#ifdef _OPENMP
	// -1 for cycle 0, -1 for the last cycle in the wave.
	const int test_cycles = SIM_WAVE_LENGTH - 2;

	const int st_length = test_cycles/2;
	const int st_start = 1;
	const int st_end = st_start + (st_length-1);

	const int pt_length = test_cycles - st_length;
	const int pt_start = st_end + 1;
	const int pt_end = pt_start + (pt_length-1);

	if (test_cycles < 2)
		error_message(SIMULATION_ERROR, -1, -1, "SIM_WAVE_LENGTH is too small.");

	// Range of cycles over which to test the sequential run times of each stage.
	char sequential_test = (cycle >= st_start && cycle <= st_end);
	// Range of cycles over which to test the parallel run times of each stage.
	char parallel_test   = (cycle >= pt_start && cycle <= pt_end);
	#endif

	int i;
	for(i = 0; i < s->count; i++)
	{
		int j;
		#ifdef _OPENMP
		double time = 0.0;
		if (sequential_test || parallel_test)
			time = wall_time();

		// Compute in parallel if we are profiling or if this stage is known to be faster in parallel.
		char compute_in_parallel = parallel_test
				|| (!sequential_test && !parallel_test && s->sequential_times[i] > s->parallel_times[i]);

		if (compute_in_parallel)
		{	// Compute the stage in parallel.
			#pragma omp parallel for schedule(static)
			for (j = 0; j < s->counts[i]; j++)
				compute_and_store_value(s->stages[i][j], cycle);
		}
		else
		{
		#endif

			// Compute the stage sequentially.
			for (j = 0; j < s->counts[i]; j++)
				compute_and_store_value(s->stages[i][j], cycle);

		#ifdef _OPENMP
		}

		if (sequential_test)
		{
			// Take the minimum sequential time.
			time = wall_time() - time;
			if (s->sequential_times[i] == 0 || time < s->sequential_times[i])
				s->sequential_times[i] = time;
		}
		else if (parallel_test)
		{
			// Take the minimum parallel time.
			time = wall_time() - time;
			if (s->parallel_times[i] == 0 || time < s->parallel_times[i])
				s->parallel_times[i] = time;
		}

		if (cycle == pt_end + 1)
		{
			//printf("%.10f\t %.10f\t %.10f\t %d\t %d\t %f\n", s->sequential_times[i], s->parallel_times[i], s->sequential_times[i]/s->parallel_times[i], s->counts[i], compute_in_parallel, s->num_children[i]/(double)s->counts[i]);
			//printf("%d %d %d %d %d %d\n", st_length, pt_length, st_start, st_end, pt_start, pt_end);

			// Record the number of nodes in parallelizable stages.
			if (compute_in_parallel)
				s->num_parallel_nodes += s->counts[i];
		}
		#endif
	}
}

/*
 * Simulates the first cycle by traversing the netlist and returns
 * the nodes organised into parallelizable stages. Also adds lines to
 * custom pins and nodes as requested via the -p option.
 */
stages *simulate_first_cycle(netlist_t *netlist, int cycle, pin_names *p, lines_t *l)
{
	queue_t *queue = create_queue();
	// Enqueue top input nodes
	int i;
	for (i = 0; i < netlist->num_top_input_nodes; i++)
		enqueue_node_if_ready(queue,netlist->top_input_nodes[i],cycle);

	// Enqueue constant nodes.
	nnode_t *constant_nodes[] = {netlist->gnd_node, netlist->vcc_node, netlist->pad_node};
	int num_constant_nodes = 3;
	for (i = 0; i < num_constant_nodes; i++)
		enqueue_node_if_ready(queue,constant_nodes[i],cycle);

	nnode_t **ordered_nodes = 0;
	int   num_ordered_nodes = 0;

	nnode_t *node;
	while ((node = queue->remove(queue)))
	{
		compute_and_store_value(node, cycle);

		// Match node for items passed via -p and add to lines if there's a match.
		add_additional_items_to_lines(node, p, l);

		// Enqueue child nodes which are ready, not already queued, and not already complete.
		int num_children = 0;
		nnode_t **children = get_children_of(node, &num_children);

		for (i = 0; i < num_children; i++)
		{
			nnode_t* node = children[i];

			if (!node->in_queue && is_node_ready(node, cycle) && !is_node_complete(node, cycle))
			{
				node->in_queue = TRUE;
				queue->add(queue,node);
			}
		}
		free(children);

		node->in_queue = FALSE;

		// Add the node to the ordered nodes array.
		ordered_nodes = realloc(ordered_nodes, sizeof(nnode_t *) * (num_ordered_nodes + 1));
		ordered_nodes[num_ordered_nodes++] = node;
	}
	queue->destroy(queue);

	// Reorganise the ordered nodes into stages for parallel computation.
	stages *s = stage_ordered_nodes(ordered_nodes, num_ordered_nodes);
	free(ordered_nodes);

	return s;
}

/*
 * Puts the ordered nodes in stages, each of which can be computed in parallel.
 */
stages *stage_ordered_nodes(nnode_t **ordered_nodes, int num_ordered_nodes) {
	stages *s = malloc(sizeof(stages));
	s->stages = calloc(1,sizeof(nnode_t**));
	s->counts = calloc(1,sizeof(int));
	s->num_children = calloc(1,sizeof(int));
	s->count  = 1;
	s->num_connections = 0;
	s->num_nodes = num_ordered_nodes;
	s->num_parallel_nodes = 0;

	const int index_table_size = (num_ordered_nodes/100)+10;

	// Hash tables index the nodes in the current stage, as well as their children.
	hashtable_t *stage_children = create_hashtable(index_table_size);
	hashtable_t *stage_nodes    = create_hashtable(index_table_size);

	int i;
	for (i = 0; i < num_ordered_nodes; i++)
	{
		nnode_t* node = ordered_nodes[i];
		int stage = s->count-1;

		// Get the node's children for dependency checks.
		int num_children;
		nnode_t **children = get_children_of(node, &num_children);

		// Determine if the node is a child of any node in the current stage.
		int is_child_of_stage = stage_children->get(stage_children, node, sizeof(nnode_t*))?1:0;

		// Determine if any node in the current stage is a child of this node.
		int is_stage_child_of = FALSE;
		int j;
		if (!is_child_of_stage)
			for (j = 0; j < num_children; j++)
				if ((is_stage_child_of = stage_nodes->get(stage_nodes, children[j], sizeof(nnode_t*))?1:0))
					break;

		// Start a new stage if this node is related to any node in the current stage.
		if (is_child_of_stage || is_stage_child_of)
		{
			s->stages       = realloc(s->stages, sizeof(nnode_t**) * (s->count+1));
			s->counts       = realloc(s->counts, sizeof(int)       * (s->count+1));
			s->num_children = realloc(s->num_children, sizeof(int) * (s->count+1));
			stage = s->count++;
			s->stages[stage] = 0;
			s->counts[stage] = 0;
			s->num_children[stage] = 0;

			stage_children->destroy(stage_children);
			stage_nodes   ->destroy(stage_nodes);

			stage_children = create_hashtable(index_table_size);
			stage_nodes    = create_hashtable(index_table_size);
		}

		// Add the node to the current stage.
		s->stages[stage] = realloc(s->stages[stage],sizeof(nnode_t*) * (s->counts[stage]+1));
		s->stages[stage][s->counts[stage]++] = node;

		// Index the node.
		stage_nodes->add(stage_nodes, node, sizeof(nnode_t*), node);

		// Index its children.
		for (j = 0; j < num_children; j++)
			stage_children->add(stage_children, children[j], sizeof(nnode_t*), children[j]);

		// Record the number of children for computing the degree.
		s->num_connections += num_children;

		s->num_children[stage] += num_children;

		free(children);
	}
	stage_children->destroy(stage_children);
	stage_nodes   ->destroy(stage_nodes);

	s->sequential_times = calloc(s->count, sizeof(double));
	s->parallel_times   = calloc(s->count, sizeof(double));

	return s;
}

/*
 * Given a node, this function will simulate that node's new outputs,
 * and updates those pins.
 */
void compute_and_store_value(nnode_t *node, int cycle)
{
	update_undriven_input_pins(node,cycle);

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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_input_port_sizes == 3);
			oassert(node->num_output_port_sizes == 1);

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
			oassert(node->num_input_port_sizes == 3);
			oassert(node->num_output_port_sizes == 1);

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
			oassert(node->num_input_port_sizes == 3);
			oassert(node->num_output_port_sizes == 1);

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
			oassert(node->num_input_port_sizes == 3);
			oassert(node->num_output_port_sizes == 1);

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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_output_pins == 1);
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
			oassert(node->num_input_pins == 1);
			oassert(node->num_output_pins == 1);

			signed char pin = get_pin_value(node->input_pins[0], cycle);

			if      (pin  < 0) update_pin_value(node->output_pins[0], -1, cycle);
			else if (pin == 1) update_pin_value(node->output_pins[0],  0, cycle);
			else               update_pin_value(node->output_pins[0],  1, cycle);
			break;
		}
		case CLOCK_NODE:
		{
			int i;
			for (i = 0; i < node->num_output_pins; i++)
				//toggle according to ratio
				update_pin_value(node->output_pins[i], is_even_cycle(cycle/node->ratio)?0:1, cycle);
			break;
		}
		case GND_NODE:
			oassert(node->num_output_pins == 1);
			update_pin_value(node->output_pins[0], 0, cycle);
			break;
		case VCC_NODE:
			oassert(node->num_output_pins == 1);
			update_pin_value(node->output_pins[0], 1, cycle);
			break;
		case PAD_NODE:
			oassert(node->num_output_pins == 1);
			update_pin_value(node->output_pins[0], 0, cycle);
			break;
		case INPUT_NODE:
			break;
		case OUTPUT_NODE:
			oassert(node->num_output_pins == 1);
			oassert(node->num_input_pins  == 1);
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

	// Record coverage on any output pins that have changed.
	{
		int i;
		for (i = 0; i < node->num_output_pins; i++)
			if(get_pin_value(node->output_pins[i],cycle-1) != get_pin_value(node->output_pins[i],cycle))
				node->output_pins[i]->coverage++;
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
void update_undriven_input_pins(nnode_t *node, int cycle)
{
	int i;
	for (i = 0; i < node->num_undriven_pins; i++)
	{
		npin_t *pin = node->undriven_pins[i];
		update_pin_value(pin, global_args.sim_initial_value, cycle);
	}

	// By the third cycle everything in the netlist should have been updated.
	if (cycle == 3)
	{
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];
			if (get_pin_cycle(pin) < cycle-1)
			#ifdef _OPENMP
			// Can't have multiple threads trying to error out at the same time.
			#pragma omp critical
			#endif
			{
				char *node_name              = node->name;
				char *pin_name               = pin->name;

				// Print the trace.
				nnode_t *root = print_update_trace(node, cycle);

				// Throw an error.
				error_message(SIMULATION_ERROR,0,-1,"Odin has detected that an input pin attached to %s isn't being updated.\n"
						"\tPin name: %s\n"
						"\tRoot node: %s\n"
						"\tSee the trace immediately above this message for details.\n",
						node_name, pin_name, root?root->name:"N/A"
				);
			}
		}
	}
}

/*
 * Flags any inputs pins which are undriven and have
 * not already been flagged.
 */
void flag_undriven_input_pins(nnode_t *node)
{
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
				node->undriven_pins = realloc(node->undriven_pins, sizeof(npin_t *) * (node->num_undriven_pins + 1));
				node->undriven_pins[node->num_undriven_pins++] = pin;

				warning_message(SIMULATION_ERROR,0,-1,"A node (%s) has an undriven input pin.", node->name);
			}
		}
	}
}

/*
 * Gets the number of nodes whose output pins have been sufficiently covered.
 */
int get_num_covered_nodes(stages *s)
{
	int covered_nodes = 0;
	int i;
	for(i = 0; i < s->count; i++)
	{
		int j;
		for (j = 0; j < s->counts[i]; j++)
		{	/*
			 * To count as being covered, every pin should resolve, and
			 * make at least one transition from one binary value to another
			 * and back. (That's three transitions total.)
			 */
			nnode_t *node = s->stages[i][j];
			int k;
			int covered = TRUE;
			for (k = 0; k < node->num_output_pins; k++)
			{
				if (node->output_pins[k]->coverage < 3)
				{
					covered = FALSE;
					break;
				}
			}

			if (covered)
				covered_nodes++;
		}
	}
	return covered_nodes;
}

/*
 * Enqueues the node in the given queue if is_node_ready returns TRUE.
 */
int enqueue_node_if_ready(queue_t* queue, nnode_t* node, int cycle)
{
	if (is_node_ready(node, cycle))
	{
		node->in_queue = TRUE;
		queue->add(queue, node);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
 * Determines if the given node has been simulated for the given cycle.
 */
int is_node_complete(nnode_t* node, int cycle)
{
	int i;
	for (i = 0; i < node->num_output_pins; i++)
		if (node->output_pins[i] && (get_pin_cycle(node->output_pins[i]) < cycle))
			return FALSE;

	return TRUE;
}

/*
 * Checks to see if the node is ready to be simulated for the given cycle.
 */
int is_node_ready(nnode_t* node, int cycle)
{
	if (!cycle)
	{
		flag_undriven_input_pins(node);
		update_undriven_input_pins(node, cycle);
	}

	if (node->type == FF_NODE)
	{
		npin_t *D_pin     = node->input_pins[0];
		npin_t *clock_pin = node->input_pins[1];
		// Flip-flops depend on the D input from the previous cycle and the clock from this cycle.
		if
		(
			   (get_pin_cycle(D_pin    ) < cycle-1)
			|| (get_pin_cycle(clock_pin) < cycle  )
		)
			return FALSE;
	}
	else if (node->type == MEMORY)
	{
		int i;
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];
			// The data and write enable inputs rely on the values from the previous cycle.
			if (
					   !strcmp(pin->mapping, "data") || !strcmp(pin->mapping, "data1") || !strcmp(pin->mapping, "data2")
					|| !strcmp(pin->mapping, "we")   || !strcmp(pin->mapping, "we1")   || !strcmp(pin->mapping, "we2")
			)
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
 * Returns the ration of a clock node.
 * */
int get_clock_ratio(nnode_t *node)
{
	return node->ratio;
}

/*
 * Changes the ratio of a clock node
 */
void set_clock_ratio(int rat, nnode_t *node)
{
	//change the value only for clocks
	if(node->type != CLOCK_NODE)
	 return;

	node->ratio = rat;
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

				free(net_name);
				free(pin_name);
				free(node_name);
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
						children = realloc(children, sizeof(nnode_t*) * (count + 1));
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
int *get_children_pinnumber_of(nnode_t *node, int *num_children)
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

				free(net_name);
				free(pin_name);
				free(node_name);
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
						pin_numbers = realloc(pin_numbers, sizeof(int) * (count + 1));
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
		error_message(SIMULATION_ERROR, -1, -1, "Requested pin not available");
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

			free(net_name);
			free(pin_name);
			free(node_name);
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
					children = realloc(children, sizeof(nnode_t*) * (count + 1));
					children[count++] = child_node;
				}
			}
		}
	}

	*num_children = count;
	return children;
}

/*
 * Updates the value of a pin and its cycle. Pins should be updated using
 * only this function.
 *
 * Initializes the pin if need be.
 */
void update_pin_value(npin_t *pin, signed char value, int cycle)
{
	if (!pin->values)
		initialize_pin(pin);

	pin->values[get_values_offset(cycle)] = value;
	set_pin_cycle(pin, cycle);
}

/*
 * Gets the value of a pin. Pins should be checked using this function only.
 */
signed char get_pin_value(npin_t *pin, int cycle)
{
	if (!pin->values || cycle < 0){
		/* If the pin's node has an initial value, then use it. 
		   Otherwise use the global initial value 
		   Need to make sure pin->node isn't NULL (e.g. for a dummy node) */
		if(pin->node && pin->node->has_initial_value) {
			return pin->node->initial_value;
		}
		return global_args.sim_initial_value;
	}
	return pin->values[get_values_offset(cycle)];
}

/*
 * Calculates the index in the values array for the given cycle.
 */
inline int get_values_offset(int cycle)
{
	return (((cycle) + (SIM_WAVE_LENGTH)) % (SIM_WAVE_LENGTH));
}

/*
 * Gets the cycle of the given pin
 */
inline int get_pin_cycle(npin_t *pin)
{
	if (!pin->cycle)
		return -1;
	else
		return *(pin->cycle);
}

/*
 * Sets the cycle of the given pin.
 *
 * Only called from update_pin_value.
 */
inline void set_pin_cycle(npin_t *pin, int cycle)
{
	*(pin->cycle) = cycle;
}

/*
 * Returns FALSE if the cycle is odd.
 */
int is_even_cycle(int cycle)
{
	return !((cycle + 2) % 2);
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
void initialize_pin(npin_t *pin)
{
	// Initialise the driver pin if this pin is not the driver.
	if (pin->net && pin->net->driver_pin && pin->net->driver_pin != pin)
		initialize_pin(pin->net->driver_pin);

	// If initialising the driver initialised this pin, we're OK to return.
	if (pin->cycle || pin->values)
		return;

	if (pin->net)
	{
		pin->values = pin->net->values;
		pin->cycle  = &(pin->net->cycle);

		int i;
		for (i = 0; i < pin->net->num_fanout_pins; i++)
		{
			npin_t *fanout_pin = pin->net->fanout_pins[i];
			if (fanout_pin)
			{
				fanout_pin->values = pin->net->values;
				fanout_pin->cycle  = &(pin->net->cycle);
			}
		}
	}
	else
	{
		pin->values = malloc(SIM_WAVE_LENGTH * sizeof(signed char));
		pin->cycle  = malloc(sizeof(int));
	}

	int i;
	for (i = 0; i < SIM_WAVE_LENGTH; i++){
		/* If the pin's node has an initial value, then use it. 
		   Otherwise use the global initial value 
		   Need to make sure pin->node isn't NULL (e.g. for a dummy node) */
		if(pin->node && pin->node->has_initial_value)
			pin->values[i] = pin->node->initial_value;
		else 
			pin->values[i] = global_args.sim_initial_value;
	}

	set_pin_cycle(pin, -1);
}

/*
 * Returns FALSE if the node is not a clock.
 */
inline int is_clock_node(nnode_t *node)
{
	return (
		   (node->type == CLOCK_NODE)
		|| !strcmp(node->name,"top^clk") // Strictly for memories.
	);
}

/*
 * Returns TRUE if the pin's value for this
 * cycle is 1 and the value for the previous cycle
 * was not 1. Otherwise returns FALSE.
 */
int is_posedge(npin_t *pin, int cycle)
{
	if (get_pin_value(pin,cycle) == 1 && get_pin_value(pin,cycle-1) != 1)
		return TRUE;
	else
		return FALSE;
}

/*
 * Computes a node of type FF_NODE for the given cycle.
 */
void compute_flipflop_node(nnode_t *node, int cycle)
{
	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins  == 2);

	npin_t *D_pin      = node->input_pins[0];
	npin_t *clock_pin  = node->input_pins[1];
	npin_t *output_pin = node->output_pins[0];

	// Rising edge: update the flip-flop from the input value of the previous cycle.
	if (is_posedge(clock_pin, cycle))
		update_pin_value(output_pin, get_pin_value(D_pin, cycle-1), cycle);
	// Falling edge: maintain the flip-flop value.
	else
		update_pin_value(output_pin, get_pin_value(output_pin,cycle-1), cycle);
}

/*
 * Computes a node of type MUX_2 for the given cycle.
 */
void compute_mux_2_node(nnode_t *node, int cycle)
{
	oassert(node->num_output_pins == 1);
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
void compute_hard_ip_node(nnode_t *node, int cycle)
{
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	int *input_pins = malloc(sizeof(int)*node->num_input_pins);
	int *output_pins = malloc(sizeof(int)*node->num_output_pins);

	if (!node->simulate_block_cycle)
	{
		char *filename = malloc(sizeof(char)*strlen(node->name));

		if (!index(node->name, '.'))
			error_message(SIMULATION_ERROR, 0, -1,
					"Couldn't extract the name of a shared library for hard-block simulation");

		snprintf(filename, sizeof(char)*strlen(node->name), "%s.so", index(node->name, '.')+1);

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

		free(filename);
	}

	int i;
	for (i = 0; i < node->num_input_pins; i++)
		input_pins[i] = get_pin_value(node->input_pins[i],cycle);

	(node->simulate_block_cycle)
			(cycle, node->num_input_pins, input_pins, node->num_output_pins, output_pins);

	for (i = 0; i < node->num_output_pins; i++)
		update_pin_value(node->output_pins[i], output_pins[i], cycle);

	free(input_pins);
	free(output_pins);
}

/*
 * Computes the given multiply node for the given cycle.
 */
void compute_multiply_node(nnode_t *node, int cycle)
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
		int *a = malloc(sizeof(int)*node->input_port_sizes[0]);
		int *b = malloc(sizeof(int)*node->input_port_sizes[1]);

		for (i = 0; i < node->input_port_sizes[0]; i++)
			a[i] = get_pin_value(node->input_pins[i],cycle);

		for (i = 0; i < node->input_port_sizes[1]; i++)
			b[i] = get_pin_value(node->input_pins[node->input_port_sizes[0] + i],cycle);

		int *result = multiply_arrays(a, node->input_port_sizes[0], b, node->input_port_sizes[1]);

		for (i = 0; i < node->num_output_pins; i++)
			update_pin_value(node->output_pins[i], result[i], cycle);

		free(result);
		free(a);
		free(b);
	}

}

// TODO: Needs to be verified.
void compute_generic_node(nnode_t *node, int cycle)
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

	if (found) update_pin_value(node->output_pins[0], 1, cycle);
	else       update_pin_value(node->output_pins[0], 0, cycle);
}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their product. The
 * length of the returned array is twice that of the two parameters.
 *
 * This array will need to be freed later!
 */
int *multiply_arrays(int *a, int a_length, int *b, int b_length)
{
	int result_size = a_length + b_length;
	int *result = calloc(sizeof(int), result_size);

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
void compute_add_node(nnode_t *node, int cycle, int type)
{
	oassert(node->num_input_port_sizes == 3);
	oassert(node->num_output_port_sizes == 2);

	int i, num;
	int flag = 0;

	int *a = malloc(sizeof(int)*node->input_port_sizes[0]);
	int *b = malloc(sizeof(int)*node->input_port_sizes[1]);
	int *c = malloc(sizeof(int)*node->input_port_sizes[2]);

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

	free(result);
	free(a);
	free(b);
	free(c);

}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their sum. The
 * length of the returned array is the maximum of the two parameters plus one.
 * add by Sen
 * This array will need to be freed later!
 */
int *add_arrays(int *a, int a_length, int *b, int b_length, int *c, int c_length, int flag)
{
	int result_size = max(a_length , b_length) + 1;
	int *result = calloc(sizeof(int), result_size);

	int i;
	int temp_carry_in;

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

	temp_carry_in = result[1];
	if(result_size > 2){
		for(i = 1; i < min(a_length,b_length); i++)
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
			for(i = b_length; i < a_length; i++)
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
			for(i = a_length; i < b_length; i++)
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
void compute_unary_sub_node(nnode_t *node, int cycle)
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
		int *a = malloc(sizeof(int)*node->input_port_sizes[0]);
		int *c = malloc(sizeof(int)*node->input_port_sizes[1]);

		for (i = 0; i < node->input_port_sizes[0]; i++)
			a[i] = get_pin_value(node->input_pins[i],cycle);

		for (i = 0; i < node->input_port_sizes[1]; i++)
			if((node->input_pins[node->input_port_sizes[0]+ node->input_port_sizes[1] + i]->net->driver_pin->node->type == PAD_NODE))
				c[i] = 1;
			else
				c[i] = get_pin_value(node->input_pins[node->input_port_sizes[0] + i],cycle);

		int *result = unary_sub_arrays(a, node->input_port_sizes[0], c, node->input_port_sizes[1]);


		for (i = 1; i < node->num_output_pins; i++)
			update_pin_value(node->output_pins[i], result[(i - 1)], cycle);

		update_pin_value(node->output_pins[0], result[(node->num_output_pins - 1)], cycle);

		free(result);
		free(a);
		free(c);
	}

}

/*
 * Takes two arrays of integers (1's and 0's) and returns an array
 * of integers (1's and 0's) that represent their sum. The
 * length of the returned array is the maximum of the two parameters plus one.
 * add by Sen
 * This array will need to be freed later!
 */
int *unary_sub_arrays(int *a, int a_length, int *c, int c_length)
{
	int result_size = a_length + 1;
	int *result = calloc(sizeof(int), result_size);

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
void compute_memory_node(nnode_t *node, int cycle)
{
	if (is_sp_ram(node))
		compute_single_port_memory(node, cycle);
	else if (is_dp_ram(node))
		compute_dual_port_memory(node, cycle);
	else
		error_message(SIMULATION_ERROR, 0, -1,
				"Could not resolve memory hard block %s to a valid type.", node->name);
}

/*
 * Computes single port memory.
 */
void compute_single_port_memory(nnode_t *node, int cycle)
{
	sp_ram_signals *signals = get_sp_ram_signals(node);

	int posedge = is_posedge(signals->clk, cycle);

	if (!node->memory_data)
		instantiate_memory(node, signals->data->count, signals->addr->count);

	// On the rising edge, write the memory.
	if (posedge)
	{
		int we = get_pin_value(signals->we, cycle - 1);
		long address = compute_memory_address(signals->addr, cycle - 1);
		char address_ok = (address != -1)?1:0;

		int i;
		for (i = 0; i < signals->data->count; i++)
		{
			// Compute which bit we are addressing.
			long bit_address = address_ok?(i + (address * signals->data->count)):-1;

			// If write is enabled, copy the input to memory.
			if (address_ok && we)
				node->memory_data[bit_address] = get_pin_value(signals->data->pins[i],cycle-1);
		}
	}

	// Read data from the memory.
	{

		long address = compute_memory_address(signals->addr, cycle);
		char address_ok = (address != -1)?1:0;

		int i;
		for (i = 0; i < signals->data->count; i++)
		{
			// Compute which bit we are addressing.
			long bit_address = address_ok?(i + (address * signals->data->count)):-1;

			// Update the output.
			if (address_ok)
				update_pin_value(signals->out->pins[i], node->memory_data[bit_address], cycle);
			else
				update_pin_value(signals->out->pins[i], -1, cycle);
		}
	}

	free_sp_ram_signals(signals);
}

/*
 * Computes dual port memory.
 */
void compute_dual_port_memory(nnode_t *node, int cycle)
{
	dp_ram_signals *signals = get_dp_ram_signals(node);
	int posedge = is_posedge(signals->clk, cycle);

	if (!node->memory_data)
		instantiate_memory(node, signals->data2->count, signals->addr2->count);

	// On the rising edge, we write the memory.
	if (posedge)
	{
		int we1     = get_pin_value(signals->we1, cycle - 1);
		int we2     = get_pin_value(signals->we2, cycle - 1);

		long address1 = compute_memory_address(signals->addr1, cycle - 1);
		long address2 = compute_memory_address(signals->addr2, cycle - 1);

		char address1_ok = (address1 != -1)?1:0;
		char address2_ok = (address2 != -1)?1:0;

		int i;
		for (i = 0; i < signals->data1->count; i++)
		{
			// Compute which bit we are addressing.
			long bit_address1 = address1_ok?(i + (address1 * signals->data1->count)):-1;
			long bit_address2 = address2_ok?(i + (address2 * signals->data2->count)):-1;

			// Write to the memory
			if (address1_ok && we1)
				node->memory_data[bit_address1] = get_pin_value(signals->data1->pins[i], cycle-1);

			if (address2_ok && we2)
				node->memory_data[bit_address2] = get_pin_value(signals->data2->pins[i], cycle-1);
		}
	}

	// Read data from memory.
	{
		long address1 = compute_memory_address(signals->addr1, cycle);
		long address2 = compute_memory_address(signals->addr2, cycle);

		char address1_ok = (address1 != -1)?1:0;
		char address2_ok = (address2 != -1)?1:0;

		int i;
		for (i = 0; i < signals->data1->count; i++)
		{
			// Compute which bit we are addressing.
			long bit_address1 = address1_ok?(i + (address1 * signals->data1->count)):-1;
			long bit_address2 = address2_ok?(i + (address2 * signals->data2->count)):-1;

			// Read the memory bit
			if (address1_ok)
				update_pin_value(signals->out1->pins[i], node->memory_data[bit_address1], cycle);
			else
				update_pin_value(signals->out1->pins[i], -1, cycle);

			if (address2_ok)
				update_pin_value(signals->out2->pins[i], node->memory_data[bit_address2], cycle);
			else
				update_pin_value(signals->out2->pins[i], -1, cycle);
		}
	}

	free_dp_ram_signals(signals);
}

/*
 * Calculates the memory address. Returns -1 if the address is unknown.
 */
long compute_memory_address(signal_list_t *addr, int cycle)
{
	long address = 0;
	int i;
	for (i = 0; i < addr->count; i++)
	{
		// If any address pins are x's, write x's we return -1.
		if (get_pin_value(addr->pins[i],cycle) < 0)
			return -1;

		address += get_pin_value(addr->pins[i],cycle) << (i);
	}

	return address;
}

/*
 * Initialises memory using a memory information file (mif). If not
 * file is found, it is initialised to x's.
 */
void instantiate_memory(nnode_t *node, int data_width, int addr_width)
{
	long max_address = 1 << addr_width;
	node->memory_data = malloc(sizeof(signed char) * max_address * data_width);

	// Initialise the memory to -1.
	int i;
	for (i = 0; i < max_address * data_width; i++)
		node->memory_data[i] = -1;

	char *filename = get_mif_filename(node);

	FILE *mif = fopen(filename, "r");
	if (!mif)
	{
		printf("MIF %s (%dx%d) not found. \n", filename, data_width, addr_width);
	}
	else
	{
		assign_memory_from_mif_file(mif, filename, data_width, addr_width, node->memory_data);
		fclose(mif);
	}
	free(filename);
}

/*
 * Removes white space (except new lines) and comments from
 * the given mif file and returns the resulting temporary file.
 */
FILE *preprocess_mif_file(FILE *source)
{
	FILE *destination = tmpfile();
	destination = freopen(NULL, "r+", destination);
	rewind(source);

	char line[BUFFER_MAX_SIZE];
	int in_multiline_comment = FALSE;
	while (fgets(line, BUFFER_MAX_SIZE, source))
	{
		int i;
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

int parse_mif_radix(char *radix)
{
	if (radix)
	{
		if (!strcmp(radix, "HEX"))
			return 16;
		else if (!strcmp(radix, "DEC"))
			return 10;
		else if (!strcmp(radix, "OCT"))
			return 8;
		else if (!strcmp(radix, "BIN"))
			return 2;
		else
			return 0;
	}
	else
	{
		return 0;
	}
}

void assign_memory_from_mif_file(FILE *mif, char *filename, int width, long depth, signed char *memory)
{
	FILE *file = preprocess_mif_file(mif);
	rewind(file);

	hashtable_t *symbols = create_hashtable(100000);

	char buffer[BUFFER_MAX_SIZE];
	int in_content = FALSE;
	char *last_line  = NULL;
	int line_number = 0;

	int addr_radix = 0;
	int data_radix = 0;
	while (fgets(buffer, BUFFER_MAX_SIZE, file))
	{
		line_number++;
		// Remove the newline.
		trim_string(buffer, "\n");
		// Only process lines which are not empty.
		if (strlen(buffer))
		{
			char *line = strdup(buffer);
			// MIF files are case insensitive
			string_to_upper(line);

			// The content section of the file contains address:value; assignments.
			if (in_content)
			{
				// Parse at the :
				char *token = strtok(line, ":");
				if (strlen(token))
				{
					// END; signifies the end of the file.
					if(!strcmp(buffer, "END;"))
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
							error_message(SIMULATION_ERROR, line_number, -1, "%s: address %s is not a base %d string.", filename, address_string, addr_radix);

						if (!is_string_of_radix(data_string, data_radix))
							error_message(SIMULATION_ERROR, line_number, -1, "%s: data string %s is not a base %d string.", filename, data_string, data_radix);

						char *binary_data = convert_string_of_radix_to_bit_string(data_string, data_radix, width);
						long long address = convert_string_of_radix_to_long_long(address_string, addr_radix);

						if (address >= depth)
							error_message(SIMULATION_ERROR, line_number, -1, "%s: address %s is out of range.", filename, address_string);

						// Calculate the offset of this memory location in bits.
						long long offset = address * width;

						// Write the parsed value string to the memory location.
						long long i;
						for (i = offset; i < offset + width; i++)
							memory[i] = binary_data[i - offset] - '0';
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
				char *token = strtok(line, "=");
				if (strlen(token))
				{
					char *symbol = token;
					token = strtok(NULL, ";");

					// If is something after the equals sign and before the semicolon, add the symbol=value association to the symbol table.
					if (token)
						symbols->add(symbols, symbol, sizeof(char) * strlen(symbol), strdup(token));
					else if(!strcmp(buffer, "CONTENT")) {}
					// We found "CONTENT" followed on the next line by "BEGIN". That means we're at the end of the parameters.
					else if(!strcmp(buffer, "BEGIN") && !strcmp(last_line, "CONTENT"))
					{
						// Sanity check parameters to make sure we have what we need.

						// Verify the width parameter.
						char *width_string = symbols->get(symbols, "WIDTH", sizeof(char) * 5);
						int mif_width = atoi(width_string);
						if (!width_string) error_message(SIMULATION_ERROR, -1, -1, "%s: MIF WIDTH parameter unspecified.", filename);
						if (mif_width != width)
								error_message(SIMULATION_ERROR, -1, -1,
									"%s: MIF width mismatch: must be %d but %d was given", filename, width, mif_width);

						// Verify the depth parameter.
						char *depth_string = symbols->get(symbols, "DEPTH", sizeof(char) * 5);
						int mif_depth = atoi(depth_string);
						if (!depth_string) error_message(SIMULATION_ERROR, -1, -1, "%s: MIF DEPTH parameter unspecified.", filename);
						if (mif_depth != depth)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: MIF depth mismatch: must be %d but %d was given", filename, depth, mif_depth);

						// Parse the radix specifications and make sure they're OK.
						addr_radix = parse_mif_radix(symbols->get(symbols, "ADDRESS_RADIX", sizeof(char) * 13));
						data_radix = parse_mif_radix(symbols->get(symbols, "DATA_RADIX", sizeof(char) * 10));

						if (!addr_radix)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: invalid or missing ADDRESS_RADIX: must specify DEC, HEX, OCT, or BIN", filename);

						if (!data_radix)
							error_message(SIMULATION_ERROR, -1, -1,
									"%s: invalid or missing DATA_RADIX: must specify DEC, HEX, OCT, or BIN", filename);

						// If everything checks out, start reading the values.
						in_content = TRUE;
					}
					else
					{
						error_message(SIMULATION_ERROR, line_number, -1, "%s: MIF syntax error: %s", filename, line);
					}
				}
				else
				{
					error_message(SIMULATION_ERROR, line_number, -1, "%s: MIF syntax error: %s", filename, line);
				}
				free(line);
			}

			if (last_line)
				free(last_line);
			last_line = strdup(buffer);
		}
	}
	if (last_line)
		free(last_line);

	symbols->destroy_free_items(symbols);

	fclose(file);
}

/*
 * Assigns the given node to its corresponding line in the given array of line.
 * Assumes the line has already been created.
 */
void assign_node_to_line(nnode_t *node, lines_t *l, int type, int single_pin)
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
	free(port_name);

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
void insert_pin_into_line(npin_t *pin, int pin_number, line_t *line, int type)
{
	// Allocate memory for the new pin.
	line->pins        = realloc(line->pins,        sizeof(npin_t*) * (line->number_of_pins + 1));
	line->pin_numbers = realloc(line->pin_numbers, sizeof(npin_t*) * (line->number_of_pins + 1));

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
lines_t *create_lines(netlist_t *netlist, int type)
{
	lines_t *l = malloc(sizeof(lines_t));
	l->lines = 0;
	l->count = 0;

	int num_nodes   = (type == INPUT)?netlist->num_top_input_nodes:netlist->num_top_output_nodes;
	nnode_t **nodes = (type == INPUT)?netlist->top_input_nodes    :netlist->top_output_nodes;

	int i;
	for (i = 0; i < num_nodes; i++)
	{
		nnode_t *node = nodes[i];
		char *port_name = get_port_name(node->name);

		if (type == OUTPUT || !is_clock_node(node))
		{
			if (find_portname_in_lines(port_name, l) == -1)
			{
				line_t *line = create_line(port_name);
				l->lines = realloc(l->lines, sizeof(line_t *)*(l->count + 1));
				l->lines[l->count++] = line;
			}
			assign_node_to_line(node, l, type, 0);
		}
		free(port_name);
	}
	return l;
}

/*
 * Creates a vector file header from the given lines,
 * and writes it to the given file.
 */
void write_vector_headers(FILE *file, lines_t *l)
{
	char* headers = generate_vector_header(l);
	fprintf(file, "%s", headers);
	free(headers);
	fflush(file);
}

/*
 * Parses the first line of the given file and compares it to the
 * given lines for identity. If there is any difference, a warning is printed,
 * and FALSE is returned. If there are no differences, the file pointer is left
 * at the start of the second line, and TRUE is returned.
 */
int verify_test_vector_headers(FILE *in, lines_t *l)
{
	int current_line = 0;
	int buffer_length = 0;

	// Read the header from the vector file.
	char read_buffer [BUFFER_MAX_SIZE];
	rewind(in);
	if (!get_next_vector(in, read_buffer))
		error_message(SIMULATION_ERROR, 0, -1, "Failed to read vector headers.");

	// Parse the header, checking each entity against the corresponding line.
	char buffer [BUFFER_MAX_SIZE];
	buffer[0] = '\0';
	int i;
	for (i = 0; i < strlen(read_buffer) && i < BUFFER_MAX_SIZE; i++)
	{
		char next = read_buffer[i];

		if (next == EOF)
		{
			warning_message(SIMULATION_ERROR, 0, -1, "Hit end of file.");
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
					free(expected_header);
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
int verify_lines (lines_t *l)
{
	int i; 
	for (i = 0; i < l->count; i++)
	{
		int j;
		for (j = 0; j < l->lines[i]->number_of_pins; j++)
		{
			if (!l->lines[i]->pins[j])
			{
				warning_message(SIMULATION_ERROR, 0, -1, "A line %d:(%s) has a NULL pin. ", j, l->lines[i]->name);
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
int find_portname_in_lines(char* port_name, lines_t *l)
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
line_t *create_line(char *name)
{
	line_t *line = malloc(sizeof(line_t));

	line->number_of_pins = 0;
	line->pins = 0;
	line->pin_numbers = 0;
	line->type = -1;
	line->name = malloc(sizeof(char)*(strlen(name)+1));

	strcpy(line->name, name);

	return line;
}

/*
 * Generates the appropriate vector headers based on the given lines.
 */
char *generate_vector_header(lines_t *l)
{
	char *header = calloc(BUFFER_MAX_SIZE, sizeof(char));
	if (l->count)
	{
		int j;
		for (j = 0; j < l->count; j++)
		{
			// "+ 2" for null and newline/space.
			if ((strlen(header) + strlen(l->lines[j]->name) + 2) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "Buffer overflow anticipated while generating vector header.");

			strcat(header,l->lines[j]->name);
			strcat(header," ");
		}
		header[strlen(header)-1] = '\n';
	}
	else
	{
		header[0] = '\n';
	}
	header = realloc(header, sizeof(char)*(strlen(header)+1));
	return header;
}

/*
 * Stores the given test vector in the given lines, with some sanity checking to ensure that it
 * has a compatible geometry.
 */
void add_test_vector_to_lines(test_vector *v, lines_t *l, int cycle)
{
	if (l->count < v->count) 
		error_message(SIMULATION_ERROR, 0, -1, "Fewer lines (%d) than values (%d).", l->count, v->count);
	if (l->count > v->count) 
		error_message(SIMULATION_ERROR, 0, -1, "More lines (%d) than values (%d).", l->count, v->count);

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
int compare_test_vectors(test_vector *v1, test_vector *v2)
{
	if (v1->count != v2->count)
	{
		warning_message(SIMULATION_ERROR, 0, -1, "Vector lengths differ.");
		return FALSE;
	}

	int l;
	for (l = 0; l < v1->count; l++)
	{	// Compare bit by bit.
		int i;
		for (i = 0; i < v1->counts[l] && i < v2->counts[l]; i++)
			if (v1->values[l][i] != v2->values[l][i])
				return FALSE;

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
	return TRUE;
}

/*
 * Parses the given line from a test vector file into a
 * test_vector data structure.
 */
test_vector *parse_test_vector(char *buffer)
{
	buffer = strdup(buffer);
	test_vector *v = malloc(sizeof(test_vector));
	v->values = 0;
	v->counts = 0;
	v->count  = 0;

	trim_string(buffer,"\r\n");

	const char *delim = " \t";
	char *token = strtok(buffer, delim);
	while (token)
	{
		v->values = realloc(v->values, sizeof(signed char *) * (v->count + 1));
		v->counts = realloc(v->counts, sizeof(int) * (v->count + 1));
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
						v->values[v->count] = realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));
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

				v->values[v->count] = realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));
				v->values[v->count][v->counts[v->count]++] = value;
			}
		}
		v->count++;
		token = strtok(NULL, delim);
	}
	free(buffer);
	return v;
}

/*
 * Generates a "random" test_vector structure based on the geometry of the given lines.
 *
 * If you want better randomness, call srand at some point.
 */
test_vector *generate_random_test_vector(lines_t *l, int cycle, hashtable_t *hold_high_index, hashtable_t *hold_low_index)
{
	test_vector *v = malloc(sizeof(test_vector));
	v->values = 0;
	v->counts = 0;
	v->count = 0;

	int i;
	for (i = 0; i < l->count; i++)
	{
		v->values = realloc(v->values, sizeof(signed char *) * (v->count + 1));
		v->counts = realloc(v->counts, sizeof(int) * (v->count + 1));
		v->values[v->count] = 0;
		v->counts[v->count] = 0;

		int j;
		for (j = 0; j < l->lines[i]->number_of_pins; j++)
		{
			char *name = l->lines[i]->name;
			signed char value;
			if      (hold_high_index->count && hold_high_index->get(hold_high_index,name,sizeof(char)*strlen(name)))
			{
				if (!cycle) value = 0;
				else        value = 1;
			}
			else if (hold_low_index->count  && hold_low_index->get(hold_low_index,name,sizeof(char)*strlen(name)))
			{
				if (!cycle) value = 1;
				else        value = 0;
			}
			else
			{
				// Passed via the -3 option.
				if (global_args.sim_generate_three_valued_logic)
					value = (rand() % 3) - 1;
				else
					value = (rand() % 2);
			}

			v->values[v->count] = realloc(v->values[v->count], sizeof(signed char) * (v->counts[v->count] + 1));		
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
void write_wave_to_file(lines_t *l, FILE* file, int cycle_offset, int wave_length, int edge)
{
	if (!cycle_offset)
		write_vector_headers(file, l);

	int cycle;
	for (cycle = cycle_offset; cycle < (cycle_offset + wave_length); cycle++)
	{
		if (edge == -1 || (edge ==  0 &&  is_even_cycle(cycle)) || (edge ==  1 && !is_even_cycle(cycle)))
			write_vector_to_file(l, file, cycle);
	}
}

/*
 * Writes all values in the given lines to a line in the given file
 * for the given cycle.
 */
void write_vector_to_file(lines_t *l, FILE *file, int cycle)
{
	char buffer[BUFFER_MAX_SIZE];
	int i; 
	for (i = 0; i < l->count; i++)
	{
		line_t *line = l->lines[i];
		int num_pins = line->number_of_pins;

		if (line_has_unknown_pin(line, cycle) || num_pins == 1)
		{
			if ((num_pins + 1) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "Buffer overflow anticipated while writing vector for line %s.", line->name);

			buffer[0] = 0;

			int j;
			int known_values = 0;
			for (j = num_pins - 1; j >= 0 ; j--)
			{
				signed char value = get_line_pin_value(line, j, cycle);

				if (value > 1){
					error_message(SIMULATION_ERROR, 0, -1, "Invalid logic value of %d read from line %s.", value, line->name);
					}

				if (value < 0)
				{
					strcat(buffer, "x");
				}
				else
				{	known_values++;
					sprintf(buffer, "%s%d", buffer, value);
				}
			}
			// If there are no known values, print a single capital X.
			// (Only for testing. Breaks machine readability.)
			//if (!known_values && num_pins > 1)
			//	sprintf(buffer, "X");
		}
		else
		{	
			// +1 for ceiling, +1 for null, +2 for "OX"
			if ((num_pins/4 + 1 + 1 + 2) > BUFFER_MAX_SIZE)
				error_message(SIMULATION_ERROR, 0, -1, "Buffer overflow anticipated while writing vector for line %s.", line->name);

			sprintf(buffer, "0X");

			int hex_digit = 0;				
			int j;
			for (j = num_pins - 1; j >= 0; j--)
			{
				signed char value = get_line_pin_value(line,j,cycle);

				if (value > 1)
					error_message(SIMULATION_ERROR, 0, -1, "Invalid logic value of %d read from line %s.", value, line->name);

				hex_digit += value << j % 4;
				
				if (!(j % 4))
				{
					sprintf(buffer, "%s%X", buffer, hex_digit);
					hex_digit = 0;
				}
			}
		}

		// Expand the value to fill to space under the header. (Gets ugly sometimes.)
		//while (strlen(buffer) < strlen(l->lines[i]->name))
		//	strcat(buffer," ");

		fprintf(file,"%s ",buffer);
	}
	fprintf(file, "\n");
}

/*
 * Writes a wave of vectors to the given modelsim out file.
 */
void write_wave_to_modelsim_file(netlist_t *netlist, lines_t *l, FILE* modelsim_out, int cycle_offset, int wave_length)
{
	if (!cycle_offset)
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
				free(port_name);
			}
		}
	}

	int cycle;
	for (cycle = cycle_offset; cycle < (cycle_offset + wave_length); cycle ++)
	{
		if (!is_even_cycle(cycle))
			write_vector_to_modelsim_file(l, modelsim_out, cycle);
	}
}

/*
 * Writes a vector to the given modelsim out file.
 */
void write_vector_to_modelsim_file(lines_t *l, FILE *modelsim_out, int cycle)
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
int verify_output_vectors(char* output_vector_file, int num_vectors)
{
	if (global_args.sim_output_both_edges)
		num_vectors *= 2;

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
		FILE *current_out  = fopen(OUTPUT_VECTOR_FILE_NAME, "r");
		if (!current_out) error_message(SIMULATION_ERROR, 0, -1, "Could not open output vector file.");

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
				warning_message(SIMULATION_ERROR, 0, -1,"Simulation produced fewer than %d vectors. \n", num_vectors);
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

				// Compare them and print an appropriate message if they differ.
				if (!compare_test_vectors(v1,v2))
				{
					trim_string(buffer1, "\n\t");
					trim_string(buffer2, "\n\t");
					error = TRUE;
					warning_message(SIMULATION_ERROR, 0, -1, "Vector %d mismatch:\n"
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
			warning_message(SIMULATION_ERROR, 0, -1,"%s contains more than %d vectors.\n", output_vector_file, num_vectors);
		}

		fclose(existing_out);
		fclose(current_out);
	}
	return !error;
}

/*
 * Creates a hastable_t index of the given pin names list
 * of the form pin name hashes to pin name array index.
 */
hashtable_t *index_pin_name_list(pin_names *list)
{
	hashtable_t *index = create_hashtable(list->count * 2+1);
	int i;
	for (i = 0; i < list->count; i++)
	{
		int *id = malloc(sizeof(int));
		*id = i;
		index->add(index, list->pins[i], sizeof(char)*strlen(list->pins[i]), id);
	}
	return index;
}

/*
 * Parses the given comma separated list into a
 * pin_names struct. If the list is empty or null
 * an empty struct is returned.
 */
pin_names *parse_pin_name_list(char *list)
{
	pin_names *p = malloc(sizeof(pin_names));
	p->pins  = 0;
	p->count = 0;

	// Parse the list of additional pins passed via the -p option.
	if (list)
	{
		char *pin_list = strdup(list);
		char *token    = strtok(pin_list, ",");
		while (token)
		{
			p->pins = realloc(p->pins, sizeof(char *) * (p->count + 1));
			p->pins[p->count++] = strdup(token);
			token = strtok(NULL, ",");
		}
		free(pin_list);
	}
	return p;
}

/*
 * If the given node matches one of the additional names (passed via -p),
 * it's added to the lines. (Matches on output pin names, net names, and node names).
 */
void add_additional_items_to_lines(nnode_t *node, pin_names *p, lines_t *l)
{
	if (p->count)
	{
		int add = FALSE;
		int j, k = 0;

		// Search the output pin names for each user-defined item.
		for (j = 0; j < node->num_output_pins; j++)
		{
			npin_t *pin = node->output_pins[j];

			if (pin->name)
			{
				for (k = 0; k < p->count; k++)
				{
					if (strstr(pin->name, p->pins[k]))
					{
						add = TRUE;
						break;
					}
				}
				if (add) break;
			}

			if (pin->net && pin->net->name)
			{
				for (k = 0; k < p->count; k++)
				{
					if (strstr(pin->net->name, p->pins[k]))
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
			for (k = 0; k < p->count; k++)
			{
				if (strstr(node->name, p->pins[k]))
				{
					add = TRUE;
					break;
				}
			}
		}

		if (add)
		{
			int single_pin = strchr(p->pins[k], '~')?1:0;

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
					l->lines = realloc(l->lines, sizeof(line_t *)*((l->count)+1));
					l->lines[l->count++] = line;
				}
				assign_node_to_line(node, l, OUTPUT, single_pin);
				free(port_name);
			}
		}
	}
}

/*
 * Parses the given (memory) node name into a corresponding
 * mif file name.
 */
char *get_mif_filename(nnode_t *node)
{
	char buffer[BUFFER_MAX_SIZE];
	buffer[0] = 0;
	strcat(buffer, node->name);

	char *filename = strrchr(buffer, '+');
	if (filename) filename += 1;
	else          filename = buffer;

	strcat(filename, ".mif");

	filename = strdup(filename);
	return filename;
}

/*
 * Trims characters in the given "chars" string
 * from the end of the given string.
 */
void trim_string(char* string, char *chars)
{
	if (string)
	{
		int length;
		while((length = strlen(string)))
		{	int trimmed = FALSE;
			int i;
			for (i = 0; i < strlen(chars); i++)
			{
				if (string[length-1] == chars[i])
				{
					trimmed = TRUE;
					string[length-1] = '\0';
					break;
				}
			}

			if (!trimmed)
				break;
		}
	}
}

/*
 * Returns TRUE if the given line has a pin for
 * the given cycle whose value is -1.
 */
int line_has_unknown_pin(line_t *line, int cycle)
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
char *vector_value_to_hex(signed char *value, int length)
{
	char *tmp;
	char *string = calloc((length + 1),sizeof(char));
	int j;
	for (j = 0; j < length; j++)
		string[j] = value[j] + '0';

	reverse_string(string,strlen(string));

	char *hex_string = malloc(sizeof(char) * ((length/4 + 1) + 1));

	sprintf(hex_string, "%X ", (unsigned int)strtol(string, &tmp, 2));

	free(string);

	return hex_string;
}

/*
 * Counts the number of vectors in the given file. 
 */
int count_test_vectors(FILE *in)
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
 * A given line is a vector if it contains one or more
 * non-whitespace characters and does not being with a #.
 */
int is_vector(char *buffer)
{
	char *line = strdup(buffer);
	trim_string(line," \t\r\n");

	if (line[0] != '#' && strlen(line))
	{
		free(line);
		return TRUE;
	}
	else
	{
		free(line);
		return FALSE;
	}
}

/*
 * Gets the next line from the given file that
 * passes the is_vector() test and places it in
 * the buffer. Returns TRUE if a vector was found,
 * and FALSE if no vector was found.
 */
int get_next_vector(FILE *file, char *buffer)
{
	while (fgets(buffer, BUFFER_MAX_SIZE, file))
		if (is_vector(buffer))
			return TRUE;
	
	return FALSE;
}

/*
 * Free each element in lines[] and the array itself
 */
void free_lines(lines_t *l)
{
	int i;
	for (i = 0; i < l->count; i++)
	{
		free(l->lines[i]->name);
		free(l->lines[i]->pins);
		free(l->lines[i]);
	}

	free(l->lines);
	free(l);
}

/*
 * Free stages.
 */
void free_stages(stages *s)
{
	while (s->count--)
		free(s->stages[s->count]);
	free(s->stages);
	free(s->counts);
	free(s);
}

/*
 * Free the given test_vector.
 */
void free_test_vector(test_vector* v)
{
	while (v->count--)
			free(v->values[v->count]);
	free(v->values);
	free(v->counts);
	free(v);
}

/*
 * Frees pin_names struct.
 */
void free_pin_name_list(pin_names *p)
{
	while (p->count--)
		free(p->pins[p->count]);

	free(p->pins);
	free(p);
}

/*
 * Prints/updates an ASCII progress bar of length "length" to position length * completion
 * from previous position "position". Updates ETA based on the elapsed time "time".
 * Returns the new position. If the position is unchanged the bar is not redrawn.
 *
 * Call with position = -1 to draw for the first time. Returns the new
 * position, calculated based on completion.
 */
int print_progress_bar(double completion, int position, int length, double time)
{
	if (position == -1 || ((int)(completion * length)) > position)
	{
		printf("%3.0f%%|", completion * (double)100);

		position = completion * length;

		int i;
		for (i = 0; i < position; i++)
			printf("=");

		printf(">");

		for (; i < length; i++)
			printf("-");

		if (completion < 1.0)
		{
			printf("| Remaining: ");
			double remaining_time = time/(double)completion - time;
			print_time(remaining_time);
		}
		else
		{
			printf("| Total time: ");
			print_time(time);
		}

		printf("    \r");

		if (position == length)
			printf("\n");

		fflush(stdout);
	}
	return position;
}

/*
 * Prints information about the netlist we are simulating.
 */
void print_netlist_stats(stages *stages, int num_vectors)
{
	printf("%s:\n", get_circuit_filename());

	printf("  Nodes:           %d\n",    stages->num_nodes);
	printf("  Connections:     %d\n",    stages->num_connections);
	printf("  Degree:          %3.2f\n", stages->num_connections/(float)stages->num_nodes);
	printf("  Stages:          %d\n",    stages->count);
	#ifdef _OPENMP
	printf("  Parallel nodes:  %d (%4.1f%%)\n", stages->num_parallel_nodes, (stages->num_parallel_nodes/(double)stages->num_nodes) * 100);
	#endif
	printf("\n");

}

/*
 * Prints statistics. (Coverage and times.)
 */
void print_simulation_stats(stages *stages, int num_vectors, double total_time, double simulation_time)
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
 * Prints the time in appropriate units.
 */
void print_time(double time)
{
	if      (time > 24*3600) printf("%.1fd",  time/(24*3600.0));
	else if (time > 3600)    printf("%.1fh",  time/3600.0);
	else if (time > 60)      printf("%.1fm",  time/60.0);
	else if (time > 1)       printf("%.1fs",  time);
	else                     printf("%.1fms", time*1000);
}

/*
 * Prints n ancestors of the given node, complete
 * with their parents, ids, etc.
 */
void print_ancestry(nnode_t *bottom_node, int n)
{
	if (!n) n = 10;

	queue_t *queue = create_queue();
	queue->add(queue, bottom_node);
	nnode_t *node;
	printf(  "  ------------\n");
	printf(  "  BACKTRACE\n");
	printf(  "  ------------\n");

	while (n-- && (node = queue->remove(queue)))
	{
		char *name = get_pin_name(node->name);
		printf("  %s (%ld):\n", name, node->unique_id);
		free(name);
		int i;
		for (i = 0; i < node->num_input_pins; i++)
		{
			npin_t *pin = node->input_pins[i];
			nnet_t *net = pin->net;
			nnode_t *node = net->driver_pin->node;
			queue->add(queue, node);
			char *name = get_pin_name(node->name);
			printf("\t%s %s (%ld)\n", pin->mapping, name, node->unique_id);fflush(stdout);
			free(name);
		}

		/*int count = 0;
		{
			printf(  "  ------------\n");
			printf(  "  CHILDREN    \n");
			printf(  "  ------------\n");
			printf(  "  O: %d\n", node->num_output_pins);fflush(stdout);
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
									free(name);
								}
								else
								{
									printf("  Null node %d\n", ++count);fflush(stdout);
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
	queue->destroy(queue);

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
nnode_t *print_update_trace(nnode_t *bottom_node, int cycle)
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
	hashtable_t *index = create_hashtable(max_depth?(max_depth * 2):100000);

	queue_t *queue = create_queue();
	queue->add(queue, bottom_node);
	nnode_t *node;
	int depth = 0;
	// Traverse the netlist in reverse, starting with our current location.
	while ((node = queue->remove(queue)))
	{
		int found_undriven_pin = FALSE;
		int is_duplicate = index->get(index, &node->unique_id, sizeof(long))?1:0;
		if (!is_duplicate)
		{
			depth++;
			index->add(index, &node->unique_id, sizeof(long), (void *)1);
			char *name = get_pin_name(node->name);
			printf("  %s (%ld) %d %d\n", name, node->unique_id, node->num_input_pins, node->num_output_pins);
			free(name);

			int i;
			for (i = 0; i < node->num_input_pins; i++)
			{
				npin_t *pin = node->input_pins[i];
				nnet_t *net = pin->net;
				nnode_t *node = net->driver_pin->node;

				// If an input is found which hasn't been updated since before cycle-1, traverse it.
				int is_undriven = FALSE;
				if (get_pin_cycle(pin) < cycle-1)
				{
					// Only add each node for traversal once.
					if (!found_undriven_pin)
					{
						found_undriven_pin = TRUE;
						queue->add(queue, node);
					}
					is_undriven = TRUE;
				}
				char *name = get_pin_name(node->name);
				printf("\t(%s) %s (%ld) %d %d %s \n", pin->mapping, name, node->unique_id, node->num_input_pins, node->num_output_pins, is_undriven?"*":"");
				free(name);
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
	index->destroy(index);
	queue->destroy(queue);
	return root_node;
}

/*
 * Gets the current time in seconds.
 */
inline double wall_time()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (1000000*tv.tv_sec+tv.tv_usec)/1.0e6;
}

/*
 * Gets the name of the file we are simulating as passed by the -b or -V option.
 */
char *get_circuit_filename()
{
	return global_args.verilog_file?global_args.verilog_file:global_args.blif_file;
}
