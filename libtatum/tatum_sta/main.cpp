#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <numeric>

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
#include "echo_loader.hpp"
#include "verify.hpp"
#include "util.hpp"
#include "echo_writer.hpp"
#include "profile.hpp"

#define NUM_SERIAL_RUNS 10
#define NUM_PARALLEL_RUNS (1*NUM_SERIAL_RUNS)

//Should we optimize the timing graph memory layout?
//#define OPTIMIZE_GRAPH_LAYOUT

//Should we print out tag related object size info
#define PRINT_TAG_SIZES

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
using tatum::DomainId;

double median(std::vector<double> values);
double arithmean(std::vector<double> values);

int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " tg_echo_file" << endl;
        return 1;
    }

    struct timespec prog_start, load_start, verify_start;
    struct timespec prog_end, load_end, verify_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

#ifdef PRINT_TAG_SIZES
    cout << "Time class sizeof  = " << sizeof(Time) << " bytes. Time Vec Width: " << TIME_VEC_WIDTH << endl;
    cout << "Time class alignof = " << alignof(Time) << endl;

    cout << "TimingTag class sizeof  = " << sizeof(TimingTag) << " bytes." << endl;
    cout << "TimingTag class alignof = " << alignof(TimingTag) << " bytes." << endl;

    cout << "TimingTags class sizeof  = " << sizeof(TimingTags) << " bytes." << endl;
    cout << "TimingTags class alignof = " << alignof(TimingTags) << " bytes." << endl;

    cout << "NodeId class sizeof  = " << sizeof(tatum::NodeId) << " bytes." << endl;
    cout << "NodeId class alignof = " << alignof(tatum::NodeId) << " bytes." << endl;

    cout << "EdgeId class sizeof  = " << sizeof(tatum::EdgeId) << " bytes." << endl;
    cout << "EdgeId class alignof = " << alignof(tatum::EdgeId) << " bytes." << endl;

    cout << "DomainId class sizeof  = " << sizeof(tatum::DomainId) << " bytes." << endl;
    cout << "DomainId class alignof = " << alignof(tatum::DomainId) << " bytes." << endl;

    cout << "TagType class sizeof  = " << sizeof(tatum::TagType) << " bytes." << endl;
    cout << "TagType class alignof = " << alignof(tatum::TagType) << " bytes." << endl;

    cout << "NodeType class sizeof  = " << sizeof(tatum::NodeType) << " bytes." << endl;
    cout << "NodeType class alignof = " << alignof(tatum::NodeType) << " bytes." << endl;
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
    timing_graph->validate();

#ifdef OPTIMIZE_GRAPH_LAYOUT
    
    auto id_maps = timing_graph->optimize_layout();

    remap_delay_calculator(*timing_graph, *delay_calculator, id_maps.edge_id_map);
    timing_constraints->remap_nodes(id_maps.node_id_map);
    golden_reference->remap_nodes(id_maps.node_id_map);

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
    tatum::write_timing_graph(ofs, *timing_graph);
    tatum::write_timing_constraints(ofs, *timing_constraints);
    tatum::write_delay_model(ofs, *timing_graph, *delay_calculator);
    ofs.flush();
#endif

    //Make all the analyzer types to test templates
    std::shared_ptr<tatum::TimingAnalyzer> setup_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    std::shared_ptr<tatum::TimingAnalyzer> hold_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    std::shared_ptr<tatum::TimingAnalyzer> setup_hold_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);

    //Create the timing analyzer
    std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    auto serial_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(serial_analyzer);
    auto serial_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(serial_analyzer);

    //Performance variables
    float serial_verify_time = 0.;
    size_t serial_tags_verified = 0;
    std::map<std::string,std::vector<double>> serial_prof_data;
    {
        cout << "Running Serial Analysis " << NUM_SERIAL_RUNS << " times" << endl;

        serial_prof_data = profile(NUM_SERIAL_RUNS, serial_analyzer);

        cout << "\n";

        //Verify
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        auto res = verify_analyzer(*timing_graph, serial_analyzer, *golden_reference);

        serial_tags_verified = res.first;

        if(!res.second) {
            cout << "Verification failed!\n";
            std::exit(1);
        }

        clock_gettime(CLOCK_MONOTONIC, &verify_end);
        serial_verify_time += tatum::time_sec(verify_start, verify_end);

        //std::vector<NodeId> nodes = find_related_nodes(*timing_graph, {NodeId(152625)});
        std::vector<NodeId> nodes;
        std::shared_ptr<tatum::SetupTimingAnalyzer> echo_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(serial_analyzer);
        if(echo_setup_analyzer) {
            write_dot_file_setup("tg_setup_annotated.dot", *timing_graph, *delay_calculator, *echo_setup_analyzer, nodes);
        }
        std::shared_ptr<tatum::HoldTimingAnalyzer> echo_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(serial_analyzer);
        if(echo_hold_analyzer) {
            write_dot_file_hold("tg_hold_annotated.dot", *timing_graph, *delay_calculator, *echo_hold_analyzer, nodes);
        }

        cout << endl;
        cout << "Serial Analysis took " << std::setprecision(6) << std::setw(6) << arithmean(serial_prof_data["analysis_sec"])*NUM_SERIAL_RUNS << " sec";
        if(serial_prof_data["analysis_sec"].size() > 0) {
            cout << " AVG: " << arithmean(serial_prof_data["analysis_sec"]);
            cout << " Median: " << median(serial_prof_data["analysis_sec"]);
            cout << " Min: " << *std::min_element(serial_prof_data["analysis_sec"].begin(), serial_prof_data["analysis_sec"].end());
            cout << " Max: " << *std::max_element(serial_prof_data["analysis_sec"].begin(), serial_prof_data["analysis_sec"].end());
        }
        cout << endl;

        cout << "\tReset             Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["reset_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["reset_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tArr Pre-traversal Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["arrival_pre_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["arrival_pre_traversal_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tReq Pre-traversal Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["required_pre_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["required_pre_traversal_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tArr     traversal Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["arrival_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["arrival_traversal_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tReq     traversal Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["required_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["required_traversal_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tUpdate slack      Median: " << std::setprecision(6) << std::setw(6) << median(serial_prof_data["update_slack_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(serial_prof_data["update_slack_sec"])/median(serial_prof_data["analysis_sec"]) << ")" << endl;

        cout << "Verifying Serial Analysis took: " << serial_verify_time << " sec" << endl;
        if(serial_tags_verified != golden_reference->num_tags() && serial_tags_verified != golden_reference->num_tags() / 2) {
            //Potentially alow / 2 for setup only analysis from setup/hold golden
            cout << "WARNING: Expected tags (" << golden_reference->num_tags() << ") differs from tags checked (" << serial_tags_verified << ") , verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_tags_verified << " tags (expected " << golden_reference->num_tags() << " or " << golden_reference->num_tags()/2 << ") accross " << timing_graph->nodes().size() << " nodes" << endl;
        }
        cout << endl;
    }

#ifdef ECHO
    tatum::write_analysis_result(ofs, *timing_graph, serial_analyzer);
    ofs.flush();
#endif

    cout << endl;

#if NUM_PARALLEL_RUNS > 0
    std::shared_ptr<tatum::TimingAnalyzer> parallel_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis,tatum::ParallelWalker>::make(*timing_graph, *timing_constraints, *delay_calculator);
    auto parallel_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(parallel_analyzer);
    auto parallel_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(parallel_analyzer);

    float parallel_verify_time = 0;
    size_t parallel_tags_verified = 0;
    std::map<std::string,std::vector<double>> parallel_prof_data;
    {
        cout << "Running Parrallel Analysis " << NUM_PARALLEL_RUNS << " times" << endl;

        //Analyze
        parallel_prof_data = profile(NUM_PARALLEL_RUNS, parallel_analyzer);

        //Verify
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        cout << "\n";
        auto res = verify_analyzer(*timing_graph, parallel_analyzer, *golden_reference);

        parallel_tags_verified = res.first;

        if(!res.second) {
            cout << "Verification failed!\n";
            std::exit(1);
        }

        clock_gettime(CLOCK_MONOTONIC, &verify_end);
        parallel_verify_time += tatum::time_sec(verify_start, verify_end);

        cout << endl;
        cout << "Parallel Analysis took " << std::setprecision(6) << std::setw(6) << arithmean(parallel_prof_data["analysis_sec"])*NUM_SERIAL_RUNS << " sec";
        if(parallel_prof_data["analysis_sec"].size() > 0) {
            cout << " AVG: " << arithmean(parallel_prof_data["analysis_sec"]);
            cout << " Median: " << median(parallel_prof_data["analysis_sec"]);
            cout << " Min: " << *std::min_element(parallel_prof_data["analysis_sec"].begin(), parallel_prof_data["analysis_sec"].end());
            cout << " Max: " << *std::max_element(parallel_prof_data["analysis_sec"].begin(), parallel_prof_data["analysis_sec"].end());
        }
        cout << endl;

        cout << "\tReset             Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["reset_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["reset_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tArr Pre-traversal Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["arrival_pre_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["arrival_pre_traversal_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tReq Pre-traversal Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["required_pre_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["required_pre_traversal_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tArr     traversal Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["arrival_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["arrival_traversal_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tReq     traversal Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["required_traversal_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["required_traversal_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "\tUpdate slack      Median: " << std::setprecision(6) << std::setw(6) << median(parallel_prof_data["update_slack_sec"]) << " s";
        cout << " (" << std::setprecision(2) << median(parallel_prof_data["update_slack_sec"])/median(parallel_prof_data["analysis_sec"]) << ")" << endl;

        cout << "Verifying Parallel Analysis took: " <<  parallel_verify_time<< " sec" << endl;
        if(parallel_tags_verified != golden_reference->num_tags() && parallel_tags_verified != golden_reference->num_tags()/2) {
            //Potentially alow / 2 for setup only analysis from setup/hold golden
            cout << "WARNING: Expected tags (" << golden_reference->num_tags() << ") differs from tags checked (" << serial_tags_verified << ") , verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_tags_verified << " tags (expected " << golden_reference->num_tags() << " or " << golden_reference->num_tags()/2 << ") accross " << timing_graph->nodes().size() << " nodes" << endl;
        }
    }
    cout << endl;


    cout << "Parallel Speed-Up: " << std::fixed << median(serial_prof_data["analysis_sec"]) / median(parallel_prof_data["analysis_sec"]) << "x" << endl;
    cout << "\t            Reset: " << std::fixed << median(serial_prof_data["reset_sec"]) / median(parallel_prof_data["reset_sec"]) << "x" << endl;
    cout << "\tArr Pre-traversal: " << std::fixed << median(serial_prof_data["arrival_pre_traversal_sec"]) / median(parallel_prof_data["arrival_pre_traversal_sec"]) << "x" << endl;
    cout << "\tReq Pre-traversal: " << std::fixed << median(serial_prof_data["required_pre_traversal_sec"]) / median(parallel_prof_data["required_pre_traversal_sec"]) << "x" << endl;
    cout << "\t    Arr-traversal: " << std::fixed << median(serial_prof_data["arrival_traversal_sec"]) / median(parallel_prof_data["arrival_traversal_sec"]) << "x" << endl;
    cout << "\t    Req-traversal: " << std::fixed << median(serial_prof_data["required_traversal_sec"]) / median(parallel_prof_data["required_traversal_sec"]) << "x" << endl;
    cout << "\t     Update-slack: " << std::fixed << median(serial_prof_data["update_slack_sec"]) / median(parallel_prof_data["update_slack_sec"]) << "x" << endl;
    cout << endl;
#endif //NUM_PARALLEL_RUNS


    //Tag stats
    if(serial_setup_analyzer) {
        print_setup_tags_histogram(*timing_graph, *serial_setup_analyzer);
    }

    if(serial_hold_analyzer) {
        print_hold_tags_histogram(*timing_graph, *serial_hold_analyzer);
    }

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    cout << endl << "Net Serial Analysis elapsed time: " << serial_analyzer->get_profiling_data("total_analysis_sec") << " sec over " << serial_analyzer->get_profiling_data("num_full_updates") << " full updates" << endl;
    cout << endl << "Net Parallel Analysis elapsed time: " << parallel_analyzer->get_profiling_data("total_analysis_sec") << " sec over " << parallel_analyzer->get_profiling_data("num_full_updates") << " full updates" << endl;
    cout << endl << "Total time: " << tatum::time_sec(prog_start, prog_end) << " sec" << endl;

    return 0;
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());

    if(values.size() % 2 == 0) {
        return(values[values.size() / 2 - 1] + values[values.size() / 2]) / 2;
    } else {
        return values[values.size() / 2];
    }
}

double arithmean(std::vector<double> values) {
    return std::accumulate(values.begin(), values.end(), 0.) / values.size();
}
