#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include <valgrind/callgrind.h>

#include "assert.hpp"
#include "sta_util.hpp"
#include "verify.hpp"

#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

#include "SerialTimingAnalyzer.hpp"
#include "analysis_types.hpp"
#include "TimingTag.hpp"
#include "PreCalcDelayCalculator.hpp"
#include "ConstantDelayCalculator.hpp"

//Cilk variants
#include "ParallelLevelizedTimingAnalyzer.hpp"
//#include "ParallelDynamicCilkTimingAnalyzer.hpp"


//Illegal versions used for upper-bound speed-up estimates
#include "ParallelNoDependancyTimingAnalyzer.hpp"

#include "vpr_timing_graph_common.hpp"

#define NUM_SERIAL_RUNS 20
#define NUM_PARALLEL_RUNS (3*NUM_SERIAL_RUNS)
#define OPTIMIZE_NODE_EDGE_ORDER

//Do we perform verification checks?
#define VERIFY_VPR_TO_TATUM

//Currently don't check for differences in the other direction (from us to VPR),
//since we do a single traversal we generate extra ancillary timing tags which
//will not match VPR
//#define CHECK_TATUM_TO_VPR_DIFFERENCES

using std::cout;
using std::endl;

int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " tg_echo_file" << endl;
        return 1;
    }

    struct timespec prog_start, load_start, analyze_start, verify_start, reset_start;
    struct timespec prog_end, load_end, analyze_end, verify_end, reset_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    cout << "Time class sizeof  = " << sizeof(Time) << " bytes. Time Vec Width: " << TIME_VEC_WIDTH << endl;
    cout << "Time class alignof = " << alignof(Time) << endl;

    cout << "TimingTag class sizeof  = " << sizeof(TimingTag) << " bytes." << endl;
    cout << "TimingTag class alignof = " << alignof(TimingTag) << " bytes." << endl;

    cout << "TimingTags class sizeof  = " << sizeof(TimingTags) << " bytes." << endl;
    cout << "TimingTags class alignof = " << alignof(TimingTags) << " bytes." << endl;

    //Raw outputs of parser
    TimingGraph timing_graph;
    TimingConstraints timing_constraints;
    VprArrReqTimes orig_expected_arr_req_times;
    std::vector<float> orig_edge_delays;

    //Potentially modified based on parser output
    VprArrReqTimes expected_arr_req_times;
    std::vector<float> edge_delays;

    std::set<NodeId> const_gen_fanout_nodes;
    std::set<NodeId> clock_gen_fanout_nodes;

    //ParallelLevelizedCilkTimingAnalyzer parallel_analyzer = ParallelLevelizedCilkTimingAnalyzer();

    //ParallelNoDependancyCilkTimingAnalyzer parallel_analyzer = ParallelNoDependancyCilkTimingAnalyzer();
    //ParallelDynamicCilkTimingAnalyzer parallel_analyzer = ParallelDynamicCilkTimingAnalyzer();
    //ParallelLevelizedOpenMPTimingAnalyzer parallel_analyzer = ParallelLevelizedOpenMPTimingAnalyzer();

    {
        clock_gettime(CLOCK_MONOTONIC, &load_start);

        yyin = fopen(argv[1], "r");
        if(yyin != NULL) {
            int error = yyparse(timing_graph, orig_expected_arr_req_times, timing_constraints, orig_edge_delays);
            if(error) {
                cout << "Parse Error" << endl;
                fclose(yyin);
                return 1;
            }
            fclose(yyin);
        } else {
            cout << "Could not open file " << argv[1] << endl;
            return 1;
        }

        //Fix up the timing graph.
        //VPR doesn't have edges from FF_CLOCKs to FF_SOURCEs and FF_SINKs,
        //but we require them. So post-process the timing graph here to add them.
        add_ff_clock_to_source_sink_edges(timing_graph, orig_edge_delays);
        //We then need to re-levelize the graph
        timing_graph.levelize();

        cout << "Timing Graph Stats:" << endl;
        cout << "  Nodes : " << timing_graph.num_nodes() << endl;
        cout << "  Levels: " << timing_graph.num_levels() << endl;
        cout << "Num Clocks: " << orig_expected_arr_req_times.get_num_clocks() << endl;

        cout << endl;

#ifdef OPTIMIZE_NODE_EDGE_ORDER
        //Re-order edges
        std::vector<EdgeId> vpr_edge_map = timing_graph.optimize_edge_layout();

        //Adjust the edge delays to reflect the new ordering
        edge_delays = std::vector<float>(orig_edge_delays.size());
        for(EdgeId i = 0; i < (EdgeId) orig_edge_delays.size(); i++) {
            EdgeId new_id = vpr_edge_map[i];
            edge_delays[new_id] = orig_edge_delays[i];
        }

        //Re-order nodes
        std::vector<NodeId> vpr_node_map = timing_graph.optimize_node_layout();

        //Re-build the expected_arr_req_times to reflect the new node orderings
        expected_arr_req_times = VprArrReqTimes();
        expected_arr_req_times.set_num_nodes(orig_expected_arr_req_times.get_num_nodes());

        for(int src_domain = 0; src_domain < (int) orig_expected_arr_req_times.get_num_clocks(); src_domain++) {
            //For every clock domain pair
            for(int i = 0; i < orig_expected_arr_req_times.get_num_nodes(); i++) {
                NodeId new_id = vpr_node_map[i];
                expected_arr_req_times.add_arr_time(src_domain, new_id, orig_expected_arr_req_times.get_arr_time(src_domain, i));
                expected_arr_req_times.add_req_time(src_domain, new_id, orig_expected_arr_req_times.get_req_time(src_domain, i));
            }
        }

        //Adjust the timing constraints
        timing_constraints.remap_nodes(vpr_node_map);

#else
        expected_arr_req_times = orig_expected_arr_req_times;
        edge_delays = orig_edge_delays;
#endif

        clock_gettime(CLOCK_MONOTONIC, &load_end);

        const_gen_fanout_nodes = identify_constant_gen_fanout(timing_graph);
        clock_gen_fanout_nodes = identify_clock_gen_fanout(timing_graph);
    }
    cout << "Loading took: " << time_sec(load_start, load_end) << " sec" << endl;
    cout << endl;

    /*
     *timing_constraints.print();
     */

    int n_histo_bins = 10;
    print_level_histogram(timing_graph, n_histo_bins);
    print_node_fanin_histogram(timing_graph, n_histo_bins);
    print_node_fanout_histogram(timing_graph, n_histo_bins);
    cout << endl;

    /*
     *cout << "Timing Graph" << endl;
     *print_timing_graph(timing_graph);
     *cout << endl;
     */

    /*
     *cout << "Levelization" << endl;
     *print_levelization(timing_graph);
     *cout << endl;
     */


    //Create the delay calculator
    PreCalcDelayCalculator delay_calculator(edge_delays);

    //Create the timing analyzer
    auto serial_analyzer = std::make_shared<SerialTimingAnalyzer<SetupAnalysis, PreCalcDelayCalculator>>(timing_graph, timing_constraints, delay_calculator);

    //Performance variables
    float serial_verify_time = 0.;
    float serial_reset_time = 0.;
    int serial_arr_req_verified = 0;
    std::map<std::string,float> serial_prof_data;
    {
        cout << "Running Serial Analysis " << NUM_SERIAL_RUNS << " times" << endl;

        //To selectively profile using callgrind:
        //  valgrind --tool=callgrind --collect-atstart=no --instr-atstart=no --cache-sim=yes --cacheuse=yes ./command
        CALLGRIND_START_INSTRUMENTATION;
        for(int i = 0; i < NUM_SERIAL_RUNS; i++) {
            //Analyze

            CALLGRIND_TOGGLE_COLLECT;

            serial_analyzer->calculate_timing();

            CALLGRIND_TOGGLE_COLLECT;

            auto prof_data = serial_analyzer->profiling_data();
            for(auto kv : prof_data) {
                serial_prof_data[kv.first] += kv.second;
            }

            cout << ".";
            cout.flush();

            //print_setup_tags(timing_graph, serial_analyzer);
            //print_hold_tags(timing_graph, serial_analyzer);

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

#ifdef VERIFY_VPR_TO_TATUM
            serial_arr_req_verified = verify_analyzer(timing_graph, serial_analyzer,
                                                      expected_arr_req_times, const_gen_fanout_nodes,
                                                      clock_gen_fanout_nodes );
#endif

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            serial_verify_time += time_sec(verify_start, verify_end);

            if(i < NUM_SERIAL_RUNS-1) {
                clock_gettime(CLOCK_MONOTONIC, &reset_start);
                serial_analyzer->reset_timing();
                clock_gettime(CLOCK_MONOTONIC, &reset_end);
                serial_reset_time += time_sec(reset_start, reset_end);
            }
        }
        CALLGRIND_STOP_INSTRUMENTATION;

        for(auto kv : serial_prof_data) {
            serial_prof_data[kv.first] /= NUM_SERIAL_RUNS;
        }

        cout << endl;
        cout << "Serial Analysis took " << serial_prof_data["analysis"]*NUM_SERIAL_RUNS << " sec, AVG: " << serial_prof_data["analysis"]<< " s" << endl;
        cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["pre_traversal"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["pre_traversal"]/serial_prof_data["analysis"] << ")" << endl;
        cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["fwd_traversal"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["fwd_traversal"]/serial_prof_data["analysis"] << ")" << endl;
        cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["bck_traversal"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["bck_traversal"]/serial_prof_data["analysis"] << ")" << endl;
        cout << "Verifying Serial Analysis took: " << serial_verify_time << " sec" << endl;
        if(serial_arr_req_verified != 2*timing_graph.num_nodes()*expected_arr_req_times.get_num_clocks()) { //2x for arr and req
            cout << "WARNING: Expected arr/req times differ from number of nodes. Verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_arr_req_verified << " arr/req times accross " << timing_graph.num_nodes() << " nodes and " << expected_arr_req_times.get_num_clocks() << " clocks" << endl;
        }
        cout << "Resetting Serial Analysis took: " << serial_reset_time << " sec" << endl;
        cout << endl;
    }

    if(timing_graph.num_nodes() < 1000) {
        cout << "Writing Anotated Timing Graph Dot File" << endl;
        std::ofstream tg_setup_dot_file("tg_setup_annotated.dot");
        write_dot_file_setup(tg_setup_dot_file, timing_graph, serial_analyzer);

        //std::ofstream tg_hold_dot_file("tg_hold_annotated.dot");
        //write_dot_file_hold(tg_hold_dot_file, timing_graph, serial_analyzer);
    } else {
        cout << "Skipping writting dot file due to large graph size" << endl;
    }
    cout << endl;

#if NUM_PARALLEL_RUNS > 0
    auto parallel_analyzer = std::make_shared<ParallelLevelizedTimingAnalyzer<SetupAnalysis, PreCalcDelayCalculator>>(timing_graph, timing_constraints, delay_calculator);
    //auto parallel_analyzer = std::make_shared<ParallelNoDependancyTimingAnalyzer<SetupAnalysis, PreCalcDelayCalculator>>(timing_graph, timing_constraints, delay_calculator);

    //float parallel_analysis_time = 0;
    //float parallel_pretraverse_time = 0.;
    //float parallel_fwdtraverse_time = 0.;
    //float parallel_bcktraverse_time = 0.;
    //float parallel_analysis_time_avg = 0;
    //float parallel_pretraverse_time_avg = 0.;
    //float parallel_fwdtraverse_time_avg = 0.;
    //float parallel_bcktraverse_time_avg = 0.;
    float parallel_verify_time = 0;
    float parallel_reset_time = 0;
    int parallel_arr_req_verified = 0;
    std::map<std::string,float> parallel_prof_data;
    {
        cout << "Running Parrallel Analysis " << NUM_PARALLEL_RUNS << " times" << endl;

        for(int i = 0; i < NUM_PARALLEL_RUNS; i++) {
            //Analyze
            clock_gettime(CLOCK_MONOTONIC, &analyze_start);

            parallel_analyzer->calculate_timing();

            clock_gettime(CLOCK_MONOTONIC, &analyze_end);
            auto prof_data = parallel_analyzer->profiling_data();
            for(auto kv : prof_data) {
                parallel_prof_data[kv.first] += kv.second;
            }

            cout << ".";
            cout.flush();

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

#ifdef VERIFY_VPR_TO_TATUM
            parallel_arr_req_verified = verify_analyzer(timing_graph, parallel_analyzer,
                                                      expected_arr_req_times, const_gen_fanout_nodes,
                                                      clock_gen_fanout_nodes );
#endif

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            parallel_verify_time += time_sec(verify_start, verify_end);

            if(i < NUM_PARALLEL_RUNS-1) {
                clock_gettime(CLOCK_MONOTONIC, &reset_start);
                parallel_analyzer->reset_timing();
                clock_gettime(CLOCK_MONOTONIC, &reset_end);
                parallel_reset_time += time_sec(reset_start, reset_end);
            }
        }
        for(auto kv : parallel_prof_data) {
            parallel_prof_data[kv.first] /= NUM_PARALLEL_RUNS;
        }
        cout << endl;

        cout << "Parallel Analysis took " << parallel_prof_data["analysis"]*NUM_PARALLEL_RUNS << " sec, AVG: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["analysis"] << " s" << endl;
        cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["pre_traversal"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["pre_traversal"]/parallel_prof_data["analysis"] << ")" << endl;
        cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["fwd_traversal"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["fwd_traversal"]/parallel_prof_data["analysis"] << ")" << endl;
        cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["bck_traversal"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["bck_traversal"]/parallel_prof_data["analysis"] << ")" << endl;
        cout << "Verifying Parallel Analysis took: " <<  parallel_verify_time<< " sec" << endl;
        if(parallel_arr_req_verified != 2*timing_graph.num_nodes()*expected_arr_req_times.get_num_clocks()) { //2x for arr and req
            cout << "WARNING: Expected arr/req times differ from number of nodes. Verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_arr_req_verified << " arr/req times accross " << timing_graph.num_nodes() << " nodes and " << expected_arr_req_times.get_num_clocks() << " clocks" << endl;
        }
        cout << "Resetting Parallel Analysis took: " << parallel_reset_time << " sec" << endl;
    }
    cout << endl;



    cout << "Parallel Speed-Up: " << std::fixed << serial_prof_data["analysis"] / parallel_prof_data["analysis"] << "x" << endl;
    cout << "\tPre-traversal: " << std::fixed << serial_prof_data["pre_traversal"] / parallel_prof_data["pre_traversal"] << "x" << endl;
    cout << "\tFwd-traversal: " << std::fixed << serial_prof_data["fwd_traversal"] / parallel_prof_data["fwd_traversal"] << "x" << endl;
    cout << "\tBck-traversal: " << std::fixed << serial_prof_data["bck_traversal"] / parallel_prof_data["bck_traversal"] << "x" << endl;
    cout << endl;

    //Per-level speed-up
    dump_level_times("level_times.csv", timing_graph, serial_prof_data, parallel_prof_data);
#endif //NUM_PARALLEL_RUNS


    //Tag stats
    print_setup_tags_histogram(timing_graph, serial_analyzer);
    //print_hold_tags_histogram(timing_graph, serial_analyzer);

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    cout << endl << "Total time: " << time_sec(prog_start, prog_end) << " sec" << endl;

    return 0;
}
