#include <stdio.h>
#include <ctime>

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"
#include "ParallelTimingAnalyzer.hpp"
#include "vpr_timing_graph_common.hpp"

#define NUM_RUNS 10

void verify_timing_graph(const TimingGraph& tg, std::vector<node_arr_req_t>& expected_arr_req_times);

float time_sec(struct timespec start, struct timespec end);

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " tg_echo_file" << std::endl;
        return 1;
    }

    struct timespec prog_start, load_start, analyze_start, verify_start;
    struct timespec prog_end, load_end, analyze_end, verify_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    TimingGraph timing_graph;
    std::vector<node_arr_req_t> expected_arr_req_times;

    SerialTimingAnalyzer serial_analyzer = SerialTimingAnalyzer();
    ParallelTimingAnalyzer parallel_analyzer = ParallelTimingAnalyzer();

    {
        clock_gettime(CLOCK_MONOTONIC, &load_start);

        yyin = fopen(argv[1], "r");
        if(yyin != NULL) {
            int error = yyparse(timing_graph, expected_arr_req_times);
            if(error) {
                std::cout << "Parse Error" << std::endl;
                fclose(yyin);
                return 1;
            }
            fclose(yyin);
        } else {
            std::cout << "Could not open file " << argv[1] << std::endl;
            return 1;
        }



        std::cout << "Timing Graph Stats:" << std::endl;
        std::cout << "  Nodes : " << timing_graph.num_nodes() << std::endl;
        std::cout << "  Levels: " << timing_graph.num_levels() << std::endl;
        std::cout << std::endl;

        clock_gettime(CLOCK_MONOTONIC, &load_end);
    }
    std::cout << "Loading took: " << time_sec(load_start, load_end) << " sec" << std::endl;
    std::cout << std::endl;

    float serial_analysis_time_avg;
    {
        std::cout << "Running Serial Analysis " << NUM_RUNS << " times" << std::endl;
        TimingAnalyzer& timing_analyzer = serial_analyzer;

        clock_gettime(CLOCK_MONOTONIC, &analyze_start);
        
        for(int i = 0; i < NUM_RUNS; i++) {
            timing_analyzer.calculate_timing(timing_graph);
        }

        clock_gettime(CLOCK_MONOTONIC, &analyze_end);

        float serial_analysis_time = time_sec(analyze_start, analyze_end);
        serial_analysis_time_avg = serial_analysis_time / NUM_RUNS;
        std::cout << "Serial Analysis took " << serial_analysis_time << " sec, AVG: " << serial_analysis_time_avg << "s" << std::endl;
    }

    {
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        timing_graph.print();

        verify_timing_graph(timing_graph, expected_arr_req_times);

        clock_gettime(CLOCK_MONOTONIC, &verify_end);
    }
    std::cout << "Verifying Serial Analysis took: " << time_sec(verify_start, verify_end) << " sec" << std::endl;
    std::cout << std::endl;

    float parallel_analysis_time_avg;
    {
        std::cout << "Running Parrallel Analysis " << NUM_RUNS << " times" << std::endl;
        TimingAnalyzer& timing_analyzer = parallel_analyzer;

        clock_gettime(CLOCK_MONOTONIC, &analyze_start);
        
        for(int i = 0; i < NUM_RUNS; i++) {
            timing_analyzer.calculate_timing(timing_graph);
        }

        clock_gettime(CLOCK_MONOTONIC, &analyze_end);

        float parallel_analysis_time = time_sec(analyze_start, analyze_end);
        parallel_analysis_time_avg = parallel_analysis_time / NUM_RUNS;
        std::cout << "Parallel Analysis took " << parallel_analysis_time << " sec, AVG: " << parallel_analysis_time_avg << "s" << std::endl;
    }


    {
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        timing_graph.print();

        verify_timing_graph(timing_graph, expected_arr_req_times);

        clock_gettime(CLOCK_MONOTONIC, &verify_end);
    }
    std::cout << "Verifying Parallel Analysis took: " << time_sec(verify_start, verify_end) << " sec" << std::endl;
    std::cout << std::endl;

    std::cout << "Parallel Speed-Up: " << std::fixed << serial_analysis_time_avg / parallel_analysis_time_avg << "x" << std::endl;
    std::cout << std::endl;

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    std::cout << "Total time: " << time_sec(prog_start, prog_end) << " sec" << std::endl;

    return 0;
}

float time_sec(struct timespec start, struct timespec end) {
    float time = end.tv_sec - start.tv_sec;

    time += (end.tv_nsec - start.tv_nsec) * 1e-9;
    return time;
}

#define RELATIVE_EPSILON 1.e-5
#define ABSOLUTE_EPSILON 1.e-13

float relative_error(float A, float B) {
    if (A == B) {
        return 0.;
    }

    if (fabs(B) > fabs(A)) {
        return fabs((A - B) / B);
    } else {
        return fabs((A - B) / A);
    }
}

void verify_timing_graph(const TimingGraph& tg, std::vector<node_arr_req_t>& expected_arr_req_times) {
    //std::cout << "Verifying Calculated Timing Against VPR" << std::endl;

    std::streamsize num_width = 10;
    std::cout.precision(3);
    std::cout << std::scientific;
    std::cout << std::setw(10);

    bool error = false;
    for(int node_id = 0; node_id < (int) expected_arr_req_times.size(); node_id++) {
        const TimingNode& node = tg.node(node_id);
        
        float arr_time = node.arrival_time().value();
        float req_time = node.required_time().value();

        //Check arrival
        float arr_abs_err = fabs(arr_time - expected_arr_req_times[node_id].T_arr);
        float arr_rel_err = relative_error(arr_time, expected_arr_req_times[node_id].T_arr);
        if(isnan(arr_time) && isnan(arr_time) != isnan(expected_arr_req_times[node_id].T_arr)) {
            error = true;
            std::cout << "Node: " << node_id << " Calc_Arr: " << std::setw(num_width) << arr_time << " VPR_Arr: " << std::setw(num_width) << expected_arr_req_times[node_id].T_arr << std::endl;
            std::cout << "\tERROR Calculated arrival time was nan and didn't match VPR." << std::endl;

        } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
            std::cout << "Node: " << node_id << " Calc_Arr: " << std::setw(num_width) << arr_time << " VPR_Arr: " << std::setw(num_width) << expected_arr_req_times[node_id].T_arr << std::endl;
            std::cout << "\tERROR arrival time abs, rel errs: " << std::setw(num_width) << arr_abs_err << ", " << std::setw(num_width) << arr_rel_err << std::endl;
            error = true;
        }
        
        float req_abs_err = fabs(req_time - expected_arr_req_times[node_id].T_req);
        float req_rel_err = relative_error(req_time, expected_arr_req_times[node_id].T_req);
        if(isnan(req_time) && isnan(req_time) != isnan(expected_arr_req_times[node_id].T_req)) {
            error = true;
            std::cout << "Node: " << node_id << " Calc_Req: " << std::setw(num_width) << req_time << " VPR_Arr: " << std::setw(num_width) << expected_arr_req_times[node_id].T_req << std::endl;
            std::cout << "\tERROR Calculated required time was nan and didn't match VPR." << std::endl;
        } else if(req_rel_err > RELATIVE_EPSILON && req_abs_err > ABSOLUTE_EPSILON) {
            std::cout << "Node: " << node_id << " Calc_Req: " << std::setw(num_width) << req_time << " VPR_Arr: " << std::setw(num_width) << expected_arr_req_times[node_id].T_req << std::endl;
            std::cout << "\tERROR required time abs, rel errs: " << std::setw(num_width) << req_abs_err << ", " << std::setw(num_width) << req_rel_err << std::endl;
            error = true;
        }
    }

    if(error) {
        std::cout << "Timing verification FAILED!" << std::endl;
        exit(1);
    } else {
        //std::cout << "Timing verification SUCCEEDED" << std::endl;
    }

}
