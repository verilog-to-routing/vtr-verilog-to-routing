#include "simulationinterface.h"

simulationInterface::simulationInterface()
{
    setUpVariables();
    num_waves = 0;
    simulationStarted = false;
}

void simulationInterface::setUpVariables()
{
    netlist = verilog_netlist;

    //create lines
    input_lines = create_lines(netlist, INPUT);
    if(!verify_lines(input_lines))
    {
        fprintf(stderr, "Input lines error. \n");
    }
    lines_t* output_lines = create_lines(netlist, OUTPUT);
    if(!verify_lines(output_lines))
    {
        fprintf(stderr, "Output lines error.\n");
    }
    //Open Files and check for errors
    //logs ouputs which are produced during simulation
    out = fopen(OUTPUT_VECTOR_FILE_NAME, "w");
    if(!out)
    {
        fprintf(stderr, "Could not open output file.\n");
    }
    //logs the inputs which are used during simulation
    in_out = fopen(INPUT_VECTOR_FILE_NAME, "w");
    if(!in_out)
    {
        fprintf(stderr, "Could not open input file. \n");
    }
    //used to read in specified input values
    in = NULL;

    //if input vector file is given, open it. Otherwise inputs
    // will be generated during simulation
    input_vector_file = global_args.sim_vector_input_file;
    if(input_vector_file)
    {
        in = fopen(input_vector_file, "r");
        if(!in)
        {
            fprintf(stderr, "Could not open vector input file");
        }
        //if inputs are given, count the number of vectors to be simulated
        num_vectors = count_test_vectors(in);
        fprintf(stderr, "Simulating %d new vectors.\n", num_vectors);
    }

    //determine which edges to output into the file
    if(global_args.sim_output_rising_edge)
    {
        output_edge = -1;
    } else if(global_args.sim_output_rising_edge)
    {
        output_edge = 1;
    } else
    {
        output_edge = 0;
    }
// remember hold high and hold low pins
    hold_high = parse_pin_name_list(global_args.sim_hold_high);
    hold_low = parse_pin_name_list(global_args.sim_hold_low);
    hold_high_index = index_pin_name_list(hold_high);
    hold_low_index = index_pin_name_list(hold_low);
    stgs = 0;
}

void simulationInterface::simulateWave()
{
    /*Simulate a wave, which will be copied into the visualization
      If more than one wave is needed, simulate next wave.*/
    num_vectors = SIM_WAVE_LENGTH;


    if(!num_vectors)
    {
        fprintf(stderr, "No vectors to simulate.\n");
    } else{

        double total_time = 0;
        double simulation_time = 0;


        num_cycles = num_vectors*2;
        num_waves++;

        double wave_start_time = wall_time();
        int wave_length = SIM_WAVE_LENGTH;
        int cycle_offset = wave*wave_length;

        //Read in vectors or generate them for each cycle
        for(cycle = 0;cycle <wave_length;cycle++)
        {
            if(is_even_cycle(cycle))
            {
                if(input_vector_file)
                {
                    char buffer[BUFFER_MAX_SIZE];
                    if(!get_next_vector(in,buffer))
                    {
                        error_message(SIMULATION_ERROR,0,-1,(char*)"Could not read next vector.");
                    }

                    tvector = parse_test_vector(buffer);
                } else {
                    tvector = generate_random_test_vector(input_lines,\
                                                          cycle,
                                                          hold_high_index,
                                                          hold_low_index);

                }

            }
            add_test_vector_to_lines(tvector, input_lines, cycle);
            if(!is_even_cycle(cycle))
            {
                free_test_vector(tvector);
            }
        }
        //log the inputs
        write_wave_to_file(input_lines,in_out,cycle_offset,wave_length,1);

        double simulation_start_time = wall_time();

        //Simulate
        for(cycle = 0; cycle<wave_length; cycle++)
        {
            if(simulationStarted)
            {
                simulate_cycle(cycle+cycle_offset,stgs);
            } else
            {
                /*The first cycle produces the stages and adds additional
                lines as specified by the user*/
                pin_names* pinlist = parse_pin_name_list(global_args.sim_additional_pins);
                stgs = simulate_first_cycle(netlist, cycle, pinlist, output_lines);
                free_pin_name_list(pinlist);
                //Make sure the output lines are still OK after adding custom lines.
//                if(!verify_lines(output_lines))
//                {
//                    error_message(SIMULATION_ERROR,0,-1,
//                                  (char*)"Problem detected with the output lines after the first cycle.");
//                }
                simulationStarted = true;
            }
        }
        simulation_time += wall_time()-simulation_start_time;

        //write the result of the wave to output vector file
        write_wave_to_file(output_lines, out,cycle_offset, wave_length, output_edge);
        fflush(out);

        //measure total time
        total_time += wall_time() - wave_start_time;

        //print statistics
        print_netlist_stats(stgs,num_vectors);
        fflush(stdout);
    }


}

void simulationInterface::endSimulation(){
    free_pin_name_list(hold_high);
    free_pin_name_list(hold_low);
    hold_high_index->destroy_free_items(hold_high_index);
    hold_low_index->destroy_free_items(hold_low_index);


    free_stages(stgs);
    free_lines(output_lines);
    free_lines(input_lines);

    fclose(in_out);
    fclose(out);
    if(input_vector_file)
    {
        fclose(in);
    }
}
