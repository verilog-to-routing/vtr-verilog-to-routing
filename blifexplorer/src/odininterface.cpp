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
#include "odininterface.h"



//global vars needed for odin
t_arch Arch;
int current_parse_file;

OdinInterface::OdinInterface()
{
    fprintf(stderr,"Creating Odin II object\n");
    wave = -1;
    myCycle = 0;
    //myFilename = "/home/kons/Documents/OdinSVN/odin-ii-read-only/ODIN_II/ODIN_II/default_out.blif";
}

/*---------------------------------------------------------------------------------------------
 * (function: startOdin)
 *-------------------------------------------------------------------------------------------*/
void OdinInterface::startOdin()
{
    int num_types;
    init_options();
    //global_args.verilog_file = "/home/kons/Documents/OdinSVN/odin-ii-read-only/ODIN_II/ODIN_II/REGRESSION_TESTS/BENCHMARKS/MICROBENCHMARKS/bm_mod.v";
    /* read the FPGA architecture file */
    if (global_args.arch_file != NULL)
    {
        fprintf(stderr, "Reading FPGA Architecture file\n");

        XmlReadArch(global_args.arch_file, (boolean)FALSE, &Arch, &type_descriptors, &num_types);
    }

    if (!global_args.blif_file)
    {
            /* High level synthesis tool */
            do_high_level_synthesis();
    }
    else
    {
            read_blif(global_args.blif_file);
    }
}

/*---------------------------------------------------------------------------------------------
* (function: init_options)
*-------------------------------------------------------------------------*/
void OdinInterface::init_options()
{
    //set default odin ii options
    set_default_options();

    //necessary for simulation
    global_args.arch_file = NULL;
    global_args.config_file = NULL;
    global_args.activation_blif_file = NULL;
    global_args.high_level_block = NULL;
    global_args.sim_vector_output_file = NULL;
    global_args.sim_hold_low = NULL;
    global_args.sim_vector_input_file = NULL;
    global_args.sim_hold_high = NULL;
    configuration.output_type = (char*)"blif";
    configuration.split_memory_depth = 15;

    //set BLIF filename for Odin II
    global_args.blif_file = myFilename.trimmed().toLocal8Bit().data();

}



/*---------------------------------------------------------------------------------------------
* (function: do_simulation_of_netlist)
*-------------------------------------------------------------------------------------------*/
void OdinInterface::do_simulation_of_netlist()
{
    if (!global_args.sim_num_test_vectors && !global_args.sim_vector_input_file)
            return;

    fprintf(stderr, "Netlist Simulation Begin\n");
    simulate_netlist(verilog_netlist);

    fprintf(stderr, "--------------------------------------------------------------------\n");
}

/*---------------------------------------------------------------------------------------------
 * (function: getNodeTable)
 *-------------------------------------------------------------------------------------------*/
QHash<QString, nnode_t *> OdinInterface::getNodeTable()
{
    int i, items;
    items = 0;
    for (i = 0; i < verilog_netlist->num_top_input_nodes; i++){
        nodequeue.enqueue(verilog_netlist->top_input_nodes[i]);
        //enqueue_node_if_ready(queue,netlist->top_input_nodes[i],cycle);
    }

    // Enqueue constant nodes.
    nnode_t *constant_nodes[] = {verilog_netlist->gnd_node, verilog_netlist->vcc_node, verilog_netlist->pad_node};
    int num_constant_nodes = 3;
    for (i = 0; i < num_constant_nodes; i++){
        nodequeue.enqueue(constant_nodes[i]);
            //enqueue_node_if_ready(queue,constant_nodes[i],cycle);
    }

    // go through the netlist. While doing so
    // remove nodes from the queue and add followup nodes
    nnode_t *node;
    QHash<QString, nnode_t *> result;
    while(!nodequeue.isEmpty()){
        node = nodequeue.dequeue();
        items++;
        //remember name of the node so it is not processed again
        QString nodeName(node->name);
        //save node in hash, so it will be processed only once
        result[nodeName] = node;
        //Enqueue child nodes which are ready
        int num_children = 0;
        nnode_t **children = get_children_of(node, &num_children);


        //make sure children are not already done or in queue
        for(i=0; i< num_children; i++){
            nnode_t *nodeKid = children[i];
            QString kidName(nodeKid->name);
            //if not in queue and not yet processed

            if(!nodequeue.contains(nodeKid) && !result.contains(kidName)){
                nodequeue.enqueue(nodeKid);

            }
        }
    }
    return result;
}

/*---------------------------------------------------------------------------------------------
 * (function: setFilename)
 *-------------------------------------------------------------------------------------------*/
void OdinInterface::setFilename(QString filename)
{
    myFilename = filename;
}

/*---------------------------------------------------------------------------------------------
 * (function: setUpSimulation)
 *-------------------------------------------------------------------------------------------*/
void OdinInterface::setUpSimulation()
{
    netlist = verilog_netlist;
    input_lines = create_lines(netlist, INPUT);
    if(!verify_lines(input_lines))
    {
        fprintf(stderr, "Input lines error.\n");
    }
    output_lines = create_lines(netlist, OUTPUT);
    if(!verify_lines(output_lines))
    {
        fprintf(stderr, "Output lines error.\n");
    }
    out = fopen(OUTPUT_VECTOR_FILE_NAME, "w");
    if(!out)
    {
        fprintf(stderr, "Could not open output file\n");
    }
    in_out = fopen(INPUT_VECTOR_FILE_NAME, "w");
    if(!in_out)
    {
        fprintf(stderr, "Could not open input file\n");
    }
    modelsim_out = fopen("test.do","w");
    if(!modelsim_out)
    {
        fprintf(stderr, "Could not open modelsim output file\n");
    }
    in = NULL;
    num_vectors = 0;
    input_vector_file = global_args.sim_vector_input_file;
    //The amount of cycles that is being simulated
    global_args.sim_num_test_vectors = 8;
    if(input_vector_file)
    {
        in = fopen(input_vector_file, "r");
        if(!in)
        {
            fprintf(stderr, "Could not open vector input file\n");
        }
        num_vectors = count_test_vectors(in);
        if(!verify_test_vector_headers(in, input_lines))
        {
            fprintf(stderr, "Invalid vector header format.\n");
        }
    }else{
        num_vectors = global_args.sim_num_test_vectors;
        fprintf(stderr, "simulating %d new vectors.\n", num_vectors);
    }

    stgs = 0;

    hold_high = parse_pin_name_list(global_args.sim_hold_high);
    hold_low = parse_pin_name_list(global_args.sim_hold_low);
    hold_high_index = index_pin_name_list(hold_high);
    hold_low_index = index_pin_name_list(hold_low);
}

/*---------------------------------------------------------------------------------------------
 * (function: simulateNextWave)
 *-------------------------------------------------------------------------------------------*/
int OdinInterface::simulateNextWave()
{
    if(!num_vectors){
        fprintf(stderr, "No vectors to simulate.\n");
    } else {

        double total_time = 0;
        double simulation_time = 0;

        num_cycles = num_vectors*2;
        num_waves = 1;
        tvector = 0;


        double wave_start_time = wall_time();
        //create a new wave
        wave++;
        int cycle_offset = SIM_WAVE_LENGTH * wave;
        int wave_length  = SIM_WAVE_LENGTH;

        // Assign vectors to lines, either by reading or generating them.
        // Every second cycle gets a new vector.

        for (cycle = cycle_offset; cycle < cycle_offset + wave_length; cycle++)
        {
            if (is_even_cycle(cycle))
            {
                    if (input_vector_file)
                    {
                            char buffer[BUFFER_MAX_SIZE];

                            if (!get_next_vector(in, buffer))
                                    error_message(SIMULATION_ERROR, 0, -1, (char*)"Could not read next vector.");

                            tvector = parse_test_vector(buffer);
                    }
                    else
                    {
                            tvector = generate_random_test_vector(input_lines, cycle, hold_high_index, hold_low_index);
                    }
            }

            add_test_vector_to_lines(tvector, input_lines, cycle);

            if (!is_even_cycle(cycle))
                    free_test_vector(tvector);
        }

        // Record the input vectors we are using.
        write_wave_to_file(input_lines, in_out, cycle_offset, wave_length, 1);
        // Write ModelSim script.
        write_wave_to_modelsim_file(netlist, input_lines, modelsim_out, cycle_offset, wave_length);

        double simulation_start_time = wall_time();

        // Perform simulation
        for (cycle = cycle_offset; cycle < cycle_offset + wave_length; cycle++)
        {
            if (cycle)
            {
                    simulate_cycle(cycle, stgs);
            }
            else
            {
                    // The first cycle produces the stages, and adds additional
                    // lines as specified by the -p option.
                    pin_names *p = parse_pin_name_list(global_args.sim_additional_pins);
                    stgs = simulate_first_cycle(netlist, cycle, p, output_lines);
                    free_pin_name_list(p);
                    // Make sure the output lines are still OK after adding custom lines.
                    if (!verify_lines(output_lines))
                            error_message(SIMULATION_ERROR, 0, -1,
                                            (char*)"Problem detected with the output lines after the first cycle.");
            }
        }

        simulation_time += wall_time() - simulation_start_time;

        // Write the result of this wave to the output vector file.
        write_wave_to_file(output_lines, out, cycle_offset, wave_length, output_edge);

        total_time += wall_time() - wave_start_time;

        // Print netlist-specific statistics.
        if (!cycle_offset)
        {
                print_netlist_stats(stgs, num_vectors);
                fflush(stdout);
        }

        // Print statistics.
        print_simulation_stats(stgs, num_vectors, total_time, simulation_time);
        myCycle = cycle;
    }


    return myCycle;
}

/*---------------------------------------------------------------------------------------------
 * (function: endSimulation)
 *-------------------------------------------------------------------------------------------*/
void OdinInterface::endSimulation(){
    free_pin_name_list(hold_high);
    free_pin_name_list(hold_low);
    hold_high_index->destroy_free_items(hold_high_index);
    hold_low_index ->destroy_free_items(hold_low_index);

    fflush(out);
    fprintf(modelsim_out, "run %d\n", num_vectors*100);

    printf("\n");


    free_stages(stgs);

    free_lines(output_lines);
    free_lines(input_lines);

    fclose(modelsim_out);
    fclose(in_out);
    if (input_vector_file)
            fclose(in);
    fclose(out);

}

/*---------------------------------------------------------------------------------------------
 * (function: getOutputValue)
 *-------------------------------------------------------------------------------------------*/
int OdinInterface::getOutputValue(nnode_t* node, int outPin, int actstep)
{
    npin_t* pin = node->output_pins[outPin];
    int val = get_pin_value(pin,actstep);

    return val;
}

/*---------------------------------------------------------------------------------------------
 * (function: setEdge)
 *-------------------------------------------------------------------------------------------*/
void OdinInterface::setEdge(int i ){
    if(i==-1){
        global_args.sim_output_both_edges = 1;
        global_args.sim_output_rising_edge = 0;
    }else if(i==0){
        global_args.sim_output_both_edges = 0;
        global_args.sim_output_rising_edge = 1;
    }else{
        global_args.sim_output_both_edges = 0;
        global_args.sim_output_rising_edge = 0;
    }
}


