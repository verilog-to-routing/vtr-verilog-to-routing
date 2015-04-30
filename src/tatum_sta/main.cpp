#include <stdio.h>
#include <ctime>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <map>
#include <array>

#include "sta_util.hpp"

#include "TimingGraph.hpp"
#include "TimingNode.hpp"

#include "SerialTimingAnalyzer.hpp"

//Cilk variants
#include "ParallelLevelizedCilkTimingAnalyzer.hpp"
#include "ParallelDynamicCilkTimingAnalyzer.hpp"


//Illegal versions used for upper-bound speed-up estimates
#include "ParallelNoDependancyCilkTimingAnalyzer.hpp"

#include "vpr_timing_graph_common.hpp"

#define NUM_SERIAL_RUNS 100
#define NUM_PARALLEL_RUNS 100 //NUM_SERIAL_RUNS
#define OPTIMIZE_NODE_EDGE_ORDER

void verify_analyzer(const TimingAnalyzer& analyzer, std::vector<node_arr_req_t>& expected_arr_req_times);


int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " tg_echo_file" << std::endl;
        return 1;
    }


    struct timespec prog_start, load_start, analyze_start, verify_start;
    struct timespec prog_end, load_end, analyze_end, verify_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    std::cout << "Time class sizeof  = " << sizeof(Time) << " bytes. Time Vec Width: " << TIME_VEC_WIDTH << std::endl;
    std::cout << "Time class alignof = " << alignof(Time) << std::endl;

    std::cout << "TimingTag class sizeof  = " << sizeof(TimingTag) << " bytes." << std::endl;
    std::cout << "TimingTag class alignof = " << alignof(TimingTag) << " bytes." << std::endl;

    std::cout << "TimingTags class sizeof  = " << sizeof(TimingTags) << " bytes." << std::endl;
    std::cout << "TimingTags class alignof = " << alignof(TimingTags) << " bytes." << std::endl;

    TimingGraph timing_graph;
    std::vector<node_arr_req_t> orig_expected_arr_req_times;
    std::vector<node_arr_req_t> expected_arr_req_times;

    SerialTimingAnalyzer serial_analyzer;
    //ParallelLevelizedCilkTimingAnalyzer parallel_analyzer = ParallelLevelizedCilkTimingAnalyzer(); 

    //ParallelNoDependancyCilkTimingAnalyzer parallel_analyzer = ParallelNoDependancyCilkTimingAnalyzer(); 
    //ParallelDynamicCilkTimingAnalyzer parallel_analyzer = ParallelDynamicCilkTimingAnalyzer(); 
    //ParallelLevelizedOpenMPTimingAnalyzer parallel_analyzer = ParallelLevelizedOpenMPTimingAnalyzer(); 

    {
        clock_gettime(CLOCK_MONOTONIC, &load_start);

        yyin = fopen(argv[1], "r");
        if(yyin != NULL) {
            int error = yyparse(timing_graph, orig_expected_arr_req_times);
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

#ifdef OPTIMIZE_NODE_EDGE_ORDER
        timing_graph.contiguize_level_edges();
        std::map<NodeId,NodeId> vpr_node_map = timing_graph.contiguize_level_nodes();

        //Re-build the expected_arr_req_times to reflect the new node orderings
        expected_arr_req_times = std::vector<node_arr_req_t>(orig_expected_arr_req_times.size());
        for(size_t i = 0; i < orig_expected_arr_req_times.size(); i++) {
            NodeId new_id = vpr_node_map[i];
            expected_arr_req_times[new_id] = orig_expected_arr_req_times[i];
        }
#else
        expected_arr_req_times = orig_expected_arr_req_times;
#endif

        clock_gettime(CLOCK_MONOTONIC, &load_end);
    }
    std::cout << "Loading took: " << time_sec(load_start, load_end) << " sec" << std::endl;
    std::cout << std::endl;

    int n_histo_bins = 20;
    print_level_histogram(timing_graph, n_histo_bins);
    print_node_fanin_histogram(timing_graph, n_histo_bins);
    print_node_fanout_histogram(timing_graph, n_histo_bins);
    std::cout << std::endl;

    //print_timing_graph(timing_graph);

    float serial_analysis_time = 0.;
    float serial_pretraverse_time = 0.;
    float serial_fwdtraverse_time = 0.;
    float serial_bcktraverse_time = 0.;
    float serial_analysis_time_avg = 0.;
    float serial_pretraverse_time_avg = 0.;
    float serial_fwdtraverse_time_avg = 0.;
    float serial_bcktraverse_time_avg = 0.;
    float serial_verify_time = 0.;
    {
        std::cout << "Running Serial Analysis " << NUM_SERIAL_RUNS << " times" << std::endl;
        
        for(int i = 0; i < NUM_SERIAL_RUNS; i++) {
            //Analyze

            clock_gettime(CLOCK_MONOTONIC, &analyze_start);

            ta_runtime traversal_times = serial_analyzer.calculate_timing(timing_graph);

            clock_gettime(CLOCK_MONOTONIC, &analyze_end);

            serial_analysis_time += time_sec(analyze_start, analyze_end);
            serial_pretraverse_time += traversal_times.pre_traversal;
            serial_fwdtraverse_time += traversal_times.fwd_traversal;
            serial_bcktraverse_time += traversal_times.bck_traversal;

            std::cout << ".";
            std::cout.flush();

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

            if(serial_analyzer.is_correct()) {
                verify_analyzer(serial_analyzer, expected_arr_req_times);
            }

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            serial_verify_time += time_sec(verify_start, verify_end);

            if(i == NUM_SERIAL_RUNS-1) {
                serial_analyzer.save_level_times(timing_graph, "serial_level_times.csv");
            } else {
                serial_analyzer.reset_timing();
            }
        }
        serial_analysis_time_avg = serial_analysis_time / NUM_SERIAL_RUNS;
        serial_pretraverse_time_avg = serial_pretraverse_time / NUM_SERIAL_RUNS;
        serial_fwdtraverse_time_avg = serial_fwdtraverse_time / NUM_SERIAL_RUNS;
        serial_bcktraverse_time_avg = serial_bcktraverse_time / NUM_SERIAL_RUNS;

        std::cout << std::endl;
        std::cout << "Serial Analysis took " << serial_analysis_time << " sec, AVG: " << serial_analysis_time_avg << " s" << std::endl;
        std::cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_pretraverse_time_avg << " s";
        std::cout << " (" << std::setprecision(2) << serial_pretraverse_time_avg/serial_analysis_time_avg << ")" << std::endl;
        std::cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_fwdtraverse_time_avg << " s";
        std::cout << " (" << std::setprecision(2) << serial_fwdtraverse_time_avg/serial_analysis_time_avg << ")" << std::endl;
        std::cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_bcktraverse_time_avg << " s";
        std::cout << " (" << std::setprecision(2) << serial_bcktraverse_time_avg/serial_analysis_time_avg << ")" << std::endl;
        std::cout << "Verifying Serial Analysis took: " << time_sec(verify_start, verify_end) << " sec" << std::endl;
        std::cout << std::endl;

        //Tag stats
        print_timing_tags_histogram(timing_graph, serial_analyzer, 10);
    }

/*
 *    float parallel_analysis_time = 0;
 *    float parallel_pretraverse_time = 0.;
 *    float parallel_fwdtraverse_time = 0.;
 *    float parallel_bcktraverse_time = 0.;
 *    float parallel_analysis_time_avg = 0;
 *    float parallel_pretraverse_time_avg = 0.;
 *    float parallel_fwdtraverse_time_avg = 0.;
 *    float parallel_bcktraverse_time_avg = 0.;
 *    float parallel_verify_time = 0;
 *    {
 *        std::cout << "Running Parrallel Analysis " << NUM_PARALLEL_RUNS << " times" << std::endl;
 *        
 *        for(int i = 0; i < NUM_PARALLEL_RUNS; i++) {
 *            //Analyze
 *            clock_gettime(CLOCK_MONOTONIC, &analyze_start);
 *
 *            ta_runtime traversal_times = parallel_analyzer.calculate_timing(timing_graph);
 *
 *            clock_gettime(CLOCK_MONOTONIC, &analyze_end);
 *            parallel_pretraverse_time += traversal_times.pre_traversal;
 *            parallel_fwdtraverse_time += traversal_times.fwd_traversal;
 *            parallel_bcktraverse_time += traversal_times.bck_traversal;
 *            parallel_analysis_time += time_sec(analyze_start, analyze_end);
 *
 *            std::cout << ".";
 *            std::cout.flush();
 *
 *            //Verify
 *            clock_gettime(CLOCK_MONOTONIC, &verify_start);
 *
 *            if (parallel_analyzer.is_correct()) {
 *                verify_timing_graph(timing_graph, expected_arr_req_times);
 *            }
 *
 *            clock_gettime(CLOCK_MONOTONIC, &verify_end);
 *            parallel_verify_time += time_sec(verify_start, verify_end);
 *            
 *            if(i == NUM_PARALLEL_RUNS-1) {
 *                parallel_analyzer.save_level_times(timing_graph, "parallel_level_times.csv");
 *            }
 *            //Reset
 *            parallel_analyzer.reset_timing(timing_graph);
 *        }
 *        parallel_analysis_time_avg = parallel_analysis_time / NUM_PARALLEL_RUNS;
 *        parallel_pretraverse_time_avg = parallel_pretraverse_time / NUM_PARALLEL_RUNS;
 *        parallel_fwdtraverse_time_avg = parallel_fwdtraverse_time / NUM_PARALLEL_RUNS;
 *        parallel_bcktraverse_time_avg = parallel_bcktraverse_time / NUM_PARALLEL_RUNS;
 *        std::cout << std::endl;
 *        if(!parallel_analyzer.is_correct()) {
 *            std::cout << "Skipped correctness verification" << std::endl; 
 *        }
 *        std::cout << "Parallel Analysis took " << parallel_analysis_time << " sec, AVG: " << std::setprecision(6) << std::setw(6) << parallel_analysis_time_avg << " s" << std::endl;
 *        std::cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_pretraverse_time_avg << " s";
 *        std::cout << " (" << std::setprecision(2) << parallel_pretraverse_time_avg/parallel_analysis_time_avg << ")" << std::endl;
 *        std::cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_fwdtraverse_time_avg << " s";
 *        std::cout << " (" << std::setprecision(2) << parallel_fwdtraverse_time_avg/parallel_analysis_time_avg << ")" << std::endl;
 *        std::cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_bcktraverse_time_avg << " s";
 *        std::cout << " (" << std::setprecision(2) << parallel_bcktraverse_time_avg/parallel_analysis_time_avg << ")" << std::endl;
 *        std::cout << "Verifying Parallel Analysis took: " << time_sec(verify_start, verify_end) << " sec" << std::endl;
 *        std::cout << std::endl;
 *    }
 *
 *
 *
 *    std::cout << "Parallel Speed-Up: " << std::fixed << serial_analysis_time_avg / parallel_analysis_time_avg << "x" << std::endl;
 *    std::cout << "\tPre-traversal: " << std::fixed << serial_pretraverse_time_avg / parallel_pretraverse_time_avg << "x" << std::endl;
 *    std::cout << "\tFwd-traversal: " << std::fixed << serial_fwdtraverse_time_avg / parallel_fwdtraverse_time_avg << "x" << std::endl;
 *    std::cout << "\tBck-traversal: " << std::fixed << serial_bcktraverse_time_avg / parallel_bcktraverse_time_avg << "x" << std::endl;
 *    std::cout << std::endl;
 */

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    std::cout << "Total time: " << time_sec(prog_start, prog_end) << " sec" << std::endl;

    return 0;
}

#define RELATIVE_EPSILON 1.e-5
#define ABSOLUTE_EPSILON 1.e-13

void verify_analyzer(const TimingAnalyzer& analyzer, std::vector<node_arr_req_t>& expected_arr_req_times) {
    assert(expected_arr_req_times.size() > 0);

    //std::cout << "Verifying Calculated Timing Against VPR" << std::endl;
    std::ios_base::fmtflags saved_flags = std::cout.flags();
    std::streamsize prec = std::cout.precision();
    std::streamsize width = std::cout.width();

    std::streamsize num_width = 10;
    std::cout.precision(3);
    std::cout << std::scientific;
    std::cout << std::setw(10);

    bool error = false;
    for(int node_id = 0; node_id < (int) expected_arr_req_times.size(); node_id++) {
        const TimingTags& arr_tags = analyzer.arrival_tags(node_id);
        const TimingTags& req_tags = analyzer.required_tags(node_id);
        float vpr_arr_time = expected_arr_req_times[node_id].T_arr;
        float vpr_req_time = expected_arr_req_times[node_id].T_req;

        //Check arrival
        if(arr_tags.num_tags() == 0) {
            if(!isnan(vpr_arr_time)) {
                error = true;
                std::cout << "Node: " << node_id << std::endl;
                std::cout << "\tERROR Found no arrival-time tag, but VPR arrival time was " << std::setw(num_width) << vpr_arr_time << " (expected NAN)" << std::endl;
            }
        } else {
            float arr_time = arr_tags.begin()->time().value();
            float arr_abs_err = fabs(arr_time - vpr_arr_time);
            float arr_rel_err = relative_error(arr_time, vpr_arr_time);
            if(isnan(arr_time) && isnan(arr_time) != isnan(vpr_arr_time)) {
                error = true;
                std::cout << "Node: " << node_id << " Calc_Arr: " << std::setw(num_width) << arr_time << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << std::endl;
                std::cout << "\tERROR Calculated arrival time was nan and didn't match VPR." << std::endl;

            } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
                std::cout << "Node: " << node_id << " Calc_Arr: " << std::setw(num_width) << arr_time << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << std::endl;
                std::cout << "\tERROR arrival time abs, rel errs: " << std::setw(num_width) << arr_abs_err << ", " << std::setw(num_width) << arr_rel_err << std::endl;
                error = true;
            }
        }
        
        if(req_tags.num_tags() == 0) {
            if(!isnan(vpr_req_time)) {
                error = true;
                std::cout << "Node: " << node_id << std::endl;
                std::cout << "\tERROR Found no required-time tag, but VPR required time was " << std::setw(num_width) << vpr_req_time << " (expected NAN)" << std::endl;
            }
        } else {
            float req_time = req_tags.begin()->time().value();
            float req_abs_err = fabs(req_time - vpr_req_time);
            float req_rel_err = relative_error(req_time, vpr_req_time);
            if(isnan(req_time) && isnan(req_time) != isnan(vpr_req_time)) {
                error = true;
                std::cout << "Node: " << node_id << " Calc_Req: " << std::setw(num_width) << req_time << " VPR_Req: " << std::setw(num_width) << vpr_req_time << std::endl;
                std::cout << "\tERROR Calculated required time was nan and didn't match VPR." << std::endl;
            } else if(req_rel_err > RELATIVE_EPSILON && req_abs_err > ABSOLUTE_EPSILON) {
                std::cout << "Node: " << node_id << " Calc_Req: " << std::setw(num_width) << req_time << " VPR_Req: " << std::setw(num_width) << vpr_req_time << std::endl;
                std::cout << "\tERROR required time abs, rel errs: " << std::setw(num_width) << req_abs_err << ", " << std::setw(num_width) << req_rel_err << std::endl;
                error = true;
            }
        }
    }

    if(error) {
        std::cout << "Timing verification FAILED!" << std::endl;
        exit(1);
    } else {
        //std::cout << "Timing verification SUCCEEDED" << std::endl;
    }

    std::cout.flags(saved_flags);
    std::cout.precision(prec);
    std::cout.width(width);
}


