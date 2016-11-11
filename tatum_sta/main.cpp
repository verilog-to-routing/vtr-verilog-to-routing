#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

#include <valgrind/callgrind.h>

#include "tatum_assert.hpp"

#include "timing_analyzers.hpp"
#include "full_timing_analyzers.hpp"
#include "graph_walkers.hpp"
#include "analyzer_factory.hpp"

#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

#include "FixedDelayCalculator.hpp"
#include "ConstantDelayCalculator.hpp"

#include "sta_util.hpp"

#include "golden_reference.hpp"
#include "echo_load.hpp"
#include "verify.hpp"
#include "util.hpp"
#include "output.hpp"

#define NUM_SERIAL_RUNS 1
#define NUM_PARALLEL_RUNS (1*NUM_SERIAL_RUNS)
//#define NUM_SERIAL_RUNS 20
//#define NUM_PARALLEL_RUNS (3*NUM_SERIAL_RUNS)

//Should we optimize the timing graph memory layout?
#define OPTIMIZE_GRAPH_LAYOUT

//Should we print out tag related object size info
//#define PRINT_TAG_SIZES

//Do we dump an echo file?
#define ECHO

typedef std::chrono::duration<double> dsec;
typedef std::chrono::high_resolution_clock Clock;

using std::cout;
using std::endl;

using tatum::Time;
using tatum::TimingTag;
using tatum::TimingTags;
using tatum::TimingGraph;
using tatum::TimingConstraints;
using tatum::NodeId;
using tatum::EdgeId;

int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " tg_echo_file" << endl;
        return 1;
    }

    struct timespec prog_start, load_start, verify_start, reset_start;
    struct timespec prog_end, load_end, verify_end, reset_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

#ifdef PRINT_TAG_SIZES
    cout << "Time class sizeof  = " << sizeof(Time) << " bytes. Time Vec Width: " << TIME_VEC_WIDTH << endl;
    cout << "Time class alignof = " << alignof(Time) << endl;

    cout << "TimingTag class sizeof  = " << sizeof(TimingTag) << " bytes." << endl;
    cout << "TimingTag class alignof = " << alignof(TimingTag) << " bytes." << endl;

    cout << "TimingTags class sizeof  = " << sizeof(TimingTags) << " bytes." << endl;
    cout << "TimingTags class alignof = " << alignof(TimingTags) << " bytes." << endl;
#endif

    //Raw outputs of parser
    std::shared_ptr<TimingGraph> timing_graph;
    std::shared_ptr<TimingConstraints> timing_constraints;
    std::shared_ptr<tatum::FixedDelayCalculator> delay_calculator;
    std::shared_ptr<GoldenReference> golden_reference;

    {
        clock_gettime(CLOCK_MONOTONIC, &load_start);

        //Load the echo file
        EchoLoader loader;
        if(argv[1] == std::string("-")) {
            tatum_parse_file(stdin, loader);
        } else {
            tatum_parse_filename(argv[1], loader);
        }

        timing_graph = loader.timing_graph();
        timing_constraints = loader.timing_constraints();
        delay_calculator = loader.delay_calculator();
        golden_reference = loader.golden_reference();

        clock_gettime(CLOCK_MONOTONIC, &load_end);
        cout << "Loading took: " << tatum::time_sec(load_start, load_end) << " sec" << endl;
        cout << endl;
    }

    timing_graph->levelize();

#ifdef OPTIMIZE_GRAPH_LAYOUT
    
    auto id_maps = timing_graph->optimize_layout();

    remap_delay_calculator(*timing_graph, *delay_calculator, id_maps.edge_id_map);

#endif

    /*
     *timing_constraints->print();
     */

    int n_histo_bins = 10;
    tatum::print_level_histogram(*timing_graph, n_histo_bins);
    tatum::print_node_fanin_histogram(*timing_graph, n_histo_bins);
    tatum::print_node_fanout_histogram(*timing_graph, n_histo_bins);
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


#ifdef ECHO
    std::ofstream ofs("timing_graph.echo");
    write_timing_graph(ofs, *timing_graph);
    write_timing_constraints(ofs, *timing_constraints);
    write_delay_model(ofs, *timing_graph, *delay_calculator);
    ofs.flush();
#endif

    //Create the timing analyzer
    std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    auto serial_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(serial_analyzer);
    auto serial_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(serial_analyzer);

    //Performance variables
    float serial_verify_time = 0.;
    float serial_reset_time = 0.;
    size_t serial_tags_verified = 0;
    std::map<std::string,float> serial_prof_data;
    {
        cout << "Running Serial Analysis " << NUM_SERIAL_RUNS << " times" << endl;

        //To selectively profile using callgrind:
        //  valgrind --tool=callgrind --collect-atstart=no --instr-atstart=no --cache-sim=yes --cacheuse=yes ./command
        CALLGRIND_START_INSTRUMENTATION;
        for(int i = 0; i < NUM_SERIAL_RUNS; i++) {
            //Analyze

            {
                auto start = Clock::now();

                CALLGRIND_TOGGLE_COLLECT;
                serial_analyzer->update_timing();
                CALLGRIND_TOGGLE_COLLECT;

                serial_prof_data["analysis_sec"] += std::chrono::duration_cast<dsec>(Clock::now() - start).count();

            }

            for(auto key : {"arrival_pre_traversal_sec", "arrival_traversal_sec", "required_pre_traversal_sec", "required_traversal_sec"}) {
                serial_prof_data[key] += serial_analyzer->get_profiling_data(key);
            }

            cout << ".";
            cout.flush();

            //print_setup_tags(timing_graph, serial_analyzer);
            //print_hold_tags(timing_graph, serial_analyzer);

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

            if(i == NUM_SERIAL_RUNS - 1) {

                write_dot_file_setup("tg_setup_annotated.dot", *timing_graph, serial_analyzer, delay_calculator);
                write_dot_file_hold("tg_hold_annotated.dot", *timing_graph, serial_analyzer, delay_calculator);

                serial_tags_verified = verify_analyzer(*timing_graph, serial_analyzer, *golden_reference);
            }

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            serial_verify_time += tatum::time_sec(verify_start, verify_end);

            if(i < NUM_SERIAL_RUNS-1) {
                clock_gettime(CLOCK_MONOTONIC, &reset_start);
                serial_analyzer->reset_timing();
                clock_gettime(CLOCK_MONOTONIC, &reset_end);
                serial_reset_time += tatum::time_sec(reset_start, reset_end);
            }
        }
        CALLGRIND_STOP_INSTRUMENTATION;

        for(auto kv : serial_prof_data) {
            serial_prof_data[kv.first] /= NUM_SERIAL_RUNS;
        }

        cout << endl;
        cout << "Serial Analysis took " << std::setprecision(6) << std::setw(6) << serial_prof_data["analysis_sec"]*NUM_SERIAL_RUNS << " sec, AVG: " << serial_prof_data["analysis_sec"] << " s" << endl;

        cout << "\tArr Pre-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["arrival_pre_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["arrival_pre_traversal_sec"]/serial_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tReq Pre-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["required_pre_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["required_pre_traversal_sec"]/serial_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tArr     traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["arrival_traversal_sec"]<< " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["arrival_traversal_sec"]/serial_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tReq     traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_prof_data["required_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << serial_prof_data["required_traversal_sec"]/serial_prof_data["analysis_sec"] << ")" << endl;

        cout << "Verifying Serial Analysis took: " << serial_verify_time << " sec" << endl;
        if(serial_tags_verified != golden_reference->num_tags()) {
            cout << "WARNING: Expected tags differs from tags checked, verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_tags_verified << " tags (expected " << golden_reference->num_tags() << ") accross " << timing_graph->nodes().size() << " nodes" << endl;
        }
        cout << "Resetting Serial Analysis took: " << serial_reset_time << " sec" << endl;
        cout << endl;
    }

    cout << endl;

#if NUM_PARALLEL_RUNS > 0
    std::shared_ptr<tatum::TimingAnalyzer> parallel_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis,tatum::ParallelWalker>::make(*timing_graph, *timing_constraints, *delay_calculator);
    auto parallel_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(parallel_analyzer);
    auto parallel_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(parallel_analyzer);

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
    size_t parallel_tags_verified = 0;
    std::map<std::string,float> parallel_prof_data;
    {
        cout << "Running Parrallel Analysis " << NUM_PARALLEL_RUNS << " times" << endl;

        for(int i = 0; i < NUM_PARALLEL_RUNS; i++) {
            //Analyze
            {
                auto start = Clock::now();

                parallel_analyzer->update_timing();

                parallel_prof_data["analysis_sec"] += std::chrono::duration_cast<dsec>(Clock::now() - start).count();
            }

            for(auto key : {"arrival_pre_traversal_sec", "arrival_traversal_sec", "required_pre_traversal_sec", "required_traversal_sec"}) {
                parallel_prof_data[key] += parallel_analyzer->get_profiling_data(key);
            }

            cout << ".";
            cout.flush();

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

            if(i == NUM_PARALLEL_RUNS - 1) {
                parallel_tags_verified = verify_analyzer(*timing_graph, parallel_analyzer, *golden_reference);
            }

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            parallel_verify_time += tatum::time_sec(verify_start, verify_end);

            if(i < NUM_PARALLEL_RUNS-1) {
                clock_gettime(CLOCK_MONOTONIC, &reset_start);
                parallel_analyzer->reset_timing();
                clock_gettime(CLOCK_MONOTONIC, &reset_end);
                parallel_reset_time += tatum::time_sec(reset_start, reset_end);
            }
        }
        for(auto kv : parallel_prof_data) {
            parallel_prof_data[kv.first] /= NUM_PARALLEL_RUNS;
        }
        cout << endl;

        cout << "Parallel Analysis took " << std::setprecision(6) << std::setw(6) << parallel_prof_data["analysis_sec"]*NUM_PARALLEL_RUNS << " sec, AVG: " << parallel_prof_data["analysis_sec"] << " s" << endl;

        cout << "\tArr Pre-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["arrival_pre_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["arrival_pre_traversal_sec"]/parallel_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tReq Pre-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["required_pre_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["required_pre_traversal_sec"]/parallel_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tArr     traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["arrival_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["arrival_traversal_sec"]/parallel_prof_data["analysis_sec"] << ")" << endl;

        cout << "\tReq     traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_prof_data["required_traversal_sec"] << " s";
        cout << " (" << std::setprecision(2) << parallel_prof_data["required_traversal_sec"]/parallel_prof_data["analysis_sec"] << ")" << endl;

        cout << "Verifying Parallel Analysis took: " <<  parallel_verify_time<< " sec" << endl;
        if(parallel_tags_verified != golden_reference->num_tags()) {
            cout << "WARNING: Expected " << golden_reference->num_tags() << " tags but checked only " << parallel_tags_verified << ", verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << parallel_tags_verified << " tags (expected " << golden_reference->num_tags() << ") accross " << timing_graph->nodes().size() << " nodes" << endl;
        }
        cout << "Resetting Parallel Analysis took: " << parallel_reset_time << " sec" << endl;
    }
    cout << endl;

#ifdef ECHO
    write_analysis_result(ofs, *timing_graph, serial_analyzer);
#endif


    cout << "Parallel Speed-Up: " << std::fixed << serial_prof_data["analysis_sec"] / parallel_prof_data["analysis_sec"] << "x" << endl;
    cout << "\tArr Pre-traversal: " << std::fixed << serial_prof_data["arrival_pre_traversal_sec"] / parallel_prof_data["arrival_pre_traversal_sec"] << "x" << endl;
    cout << "\tReq Pre-traversal: " << std::fixed << serial_prof_data["required_pre_traversal_sec"] / parallel_prof_data["required_pre_traversal_sec"] << "x" << endl;
    cout << "\t    Arr-traversal: " << std::fixed << serial_prof_data["arrival_traversal_sec"] / parallel_prof_data["arrival_traversal_sec"] << "x" << endl;
    cout << "\t    Req-traversal: " << std::fixed << serial_prof_data["required_traversal_sec"] / parallel_prof_data["required_traversal_sec"] << "x" << endl;
    cout << endl;

    //Per-level speed-up
    tatum::dump_level_times("level_times.csv", *timing_graph, serial_prof_data, parallel_prof_data);
#endif //NUM_PARALLEL_RUNS


    //Tag stats
    if(serial_setup_analyzer) {
        print_setup_tags_histogram(*timing_graph, *serial_setup_analyzer);
    }

    if(serial_hold_analyzer) {
        print_hold_tags_histogram(*timing_graph, *serial_hold_analyzer);
    }

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    cout << endl << "Total time: " << tatum::time_sec(prog_start, prog_end) << " sec" << endl;

    return 0;
}
