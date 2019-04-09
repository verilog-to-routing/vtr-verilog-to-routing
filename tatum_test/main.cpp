#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <numeric>
#include <iomanip>

#include "tatum/util/tatum_assert.hpp"

#include "tatum/timing_analyzers.hpp"
#include "tatum/graph_walkers.hpp"
#include "tatum/analyzer_factory.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/TimingReporter.hpp"
#include "tatum/report/NodeNumNameResolver.hpp"
#include "tatum/timing_paths.hpp"

#include "tatum/delay_calc/FixedDelayCalculator.hpp"

#include "tatum/report/graphviz_dot_writer.hpp"
#include "tatum/base/sta_util.hpp"
#include "tatum/echo_writer.hpp"

#include "golden_reference.hpp"
#include "echo_loader.hpp"
#include "verify.hpp"
#include "util.hpp"
#include "profile.hpp"

#if defined(TATUM_USE_TBB) 
# include <tbb/task_scheduler_init.h>
#endif
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

struct Args {
    //Input file to load
    std::string input_file = "";

    //Concurrency (0 is machine concurrency)
    size_t num_workers = 0;

    //Number of serial runs to perform
    size_t num_serial_runs = 10;

    //Number of parallel runs to perform
    size_t num_parallel_runs = 30;

    //Use unit delays instead of from file?
    float unit_delay = 0;

    //Write an echo file of resutls?
    std::string write_echo;

    //Optimize graph memory layout?
    size_t opt_graph_layout = 0;

    //Print tag size info
    size_t print_sizes = 0;

    //Verify results match reference
    size_t verify = 0;

    //Print reports
    size_t report = 1;

    //Timing graph node whose transitive fanout is included in the 
    //dumped .dot file (useful for debugging). Values < 0 dump the
    //entire graph.
    int debug_dot_node = -1;
};

void usage(std::string prog);
void cmd_error(std::string prog, std::string msg);
Args parse_args(int argc, char** argv);

double median(std::vector<double> values);
double arithmean(std::vector<double> values);


void usage(std::string prog) {
    Args default_args;
    cout << "Usage: " << prog << " [options] tg_file\n";
    cout << "\n";
    cout << "  Positional Arguments:\n";
    cout << "    tg_file:                      The input file (or '-' for stdin)\n";
    cout << "\n";
    cout << "  Options:\n";
    cout << "    --num_workers NUM_WORKERS:        Number of parallel workers.\n";
    cout << "                                      0 implies machine concurrency.\n";
    cout << "                                      (default " << default_args.num_workers << ")\n";
    cout << "    --num_serial NUM_SERIAL_RUNS:     Number of serial runs to perform.\n";
    cout << "                                      (default " << default_args.num_serial_runs << ")\n";
    cout << "    --num_parallel NUM_PARALLEL_RUNS: Number of serial runs to perform.\n";
    cout << "                                      (default " << default_args.num_parallel_runs << ")\n";
    cout << "    --unit_delay UNIT_DELAY:          Use specified unit delay for all edges.\n";
    cout << "                                      0 uses delay model from input.\n";
    cout << "                                      (default " << default_args.unit_delay << ")\n";
    cout << "    --write_echo WRITE_ECHO:          Write an echo file of restuls.\n";
    cout << "                                      empty implies no, non-empty implies write to specified file.\n";
    cout << "                                      (default " << default_args.write_echo << ")\n";
    cout << "    --opt_graph_layout OPT_LAYOUT:    Optimize graph layout.\n";
    cout << "                                      0 implies no, non-zero implies yes.\n";
    cout << "                                      (default " << default_args.opt_graph_layout << ")\n";
    cout << "    --print_sizes PRINT_SIZES:        Print various data structure sizes.\n";
    cout << "                                      0 implies no, non-zero implies yes.\n";
    cout << "                                      (default " << default_args.print_sizes << ")\n";
    cout << "    --report REPORT:                  Generate various reports.\n";
    cout << "                                      0 implies no, non-zero implies yes.\n";
    cout << "                                      (default " << default_args.report << ")\n";
    cout << "    --verify VERIFY:                  Verify calculated results match reference.\n";
    cout << "                                      0 implies no, non-zero implies yes.\n";
    cout << "                                      (default " << default_args.verify << ")\n";
    cout << "    --debug_dot_node NODEID:          Specifies the timing graph node node whose transitive\n";
    cout << "                                      connections are dumped to the .dot file (useful for debugging).\n";
    cout << "                                      Values < -1 dump the entire graph,\n";
    cout << "                                      Values == -1 do not dump dot file,\n";
    cout << "                                      Values >= 0 dump the transitive connections of\n";
    cout << "                                      the matching node.\n";
    cout << "                                      (default " << default_args.debug_dot_node << ")\n";
}

void cmd_error(std::string prog, std::string msg) {
    cout << "Error: " << msg << "\n";
    cout << "\n";
    usage(prog);
    exit(1);
}

Args parse_args(int argc, char** argv) {
    Args args;
    auto prog = argv[0];

    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << " ";
    }
    cout << "\n";

    int i = 1;
    while (i < argc) {

        std::string arg_str(argv[i]);
        if (arg_str == "-h" || arg_str == "--help") {
            usage(prog);
            exit(0);
        } else if (arg_str.size() >= 2 && arg_str[0] == '-' && arg_str[1] == '-') {
            if (arg_str == "--write_echo") {
                args.write_echo = argv[i+1];
            } else {

                std::istringstream ss(argv[i+1]);
                float arg_val;
                ss >> arg_val;
                if (ss.fail() || !ss.eof()) {
                    std::stringstream msg;
                    msg << "Invalid option value '" << argv[i+1] << "'\n";
                    cmd_error(prog, msg.str());
                }

                if (arg_str == "--num_workers") {
                    args.num_workers = arg_val;
                } else if (argv[i] == std::string("--num_serial")) { 
                    args.num_serial_runs = arg_val;
                } else if (argv[i] == std::string("--num_parallel")) { 
                    args.num_parallel_runs = arg_val;
                } else if (argv[i] == std::string("--unit_delay")) { 
                    args.unit_delay = arg_val;
                } else if (argv[i] == std::string("--opt_graph_layout")) { 
                    args.opt_graph_layout = arg_val;
                } else if (argv[i] == std::string("--verify")) { 
                    args.verify = arg_val;
                } else if (argv[i] == std::string("--report")) { 
                    args.report = arg_val;
                } else if (argv[i] == std::string("--debug_dot_node")) { 
                    args.debug_dot_node = arg_val;
                } else {
                    std::stringstream msg;
                    msg << "Invalid option '" << arg_str << "'\n";
                    cmd_error(prog, msg.str());
                }
            }

            i += 2;
        } else { 
            if (i == argc - 1) {
                args.input_file = arg_str;
            } else {
                std::stringstream msg;
                msg << "Unrecognized positional argument '" << arg_str<< "'\n";
                cmd_error(prog, msg.str());
            }
            i++;
        }
    }

    if (args.input_file.empty()) {
        cmd_error(prog, "Missing required positional argument 'tg_file'");
    }

    return args;
}

int main(int argc, char** argv) {

    Args args = parse_args(argc, argv);

    int exit_code = 0;

    struct timespec prog_start, load_start, opt_start, verify_start;
    struct timespec prog_end, load_end, opt_end, verify_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    if (args.print_sizes) {
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
    }

#if defined(TATUM_USE_TBB) 
    size_t actual_num_workers = args.num_workers;
    if (actual_num_workers == 0) {
        actual_num_workers = tbb::task_scheduler_init::default_num_threads();
    }
    auto tbb_scheduler = std::make_unique<tbb::task_scheduler_init>(actual_num_workers);
    cout << "Tatum executing with up to " << actual_num_workers << " workers via TBB\n";
#else //Serial
    cout << "Tatum built with only serial execution support, ignoring --num_workers != 1\n";
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
        if(args.input_file == "-") {
            tatum_parse_file(stdin, loader);
        } else {
            tatum_parse_filename(args.input_file, loader);
        }

        timing_graph = loader.timing_graph();
        timing_graph->set_allow_dangling_combinational_nodes(true);
        timing_constraints = loader.timing_constraints();
        if (args.unit_delay) {
            delay_calculator = std::make_shared<tatum::FixedDelayCalculator>(
                    tatum::util::linear_map<tatum::EdgeId,tatum::Time>(timing_graph->edges().size(), tatum::Time(args.unit_delay)),
                    tatum::util::linear_map<tatum::EdgeId,tatum::Time>(timing_graph->edges().size(), tatum::Time(args.unit_delay)),
                    tatum::util::linear_map<tatum::EdgeId,tatum::Time>(timing_graph->edges().size(), tatum::Time(args.unit_delay)),
                    tatum::util::linear_map<tatum::EdgeId,tatum::Time>(timing_graph->edges().size(), tatum::Time(args.unit_delay)));

        } else {
            delay_calculator = loader.delay_calculator();
        }
        golden_reference = loader.golden_reference();

        clock_gettime(CLOCK_MONOTONIC, &load_end);
        cout << "Loading took: " << tatum::time_sec(load_start, load_end) << " sec" << endl;
        cout << endl;
    }

    timing_constraints->print_constraints();


    timing_graph->levelize();
    timing_graph->validate();

    cout << "Timing Graph Nodes: " << timing_graph->nodes().size() << "\n";
    cout << "Timing Graph Edges: " << timing_graph->edges().size() << "\n";
    cout << "Timing Graph Levels: " << timing_graph->levels().size() << "\n";

    if (args.opt_graph_layout) {
        
        clock_gettime(CLOCK_MONOTONIC, &opt_start);
        auto id_maps = timing_graph->optimize_layout();
        clock_gettime(CLOCK_MONOTONIC, &opt_end);
        cout << "Optimizing graph took: " << tatum::time_sec(opt_start, opt_end) << " sec" << endl;

        remap_delay_calculator(*timing_graph, *delay_calculator, id_maps.edge_id_map);
        timing_constraints->remap_nodes(id_maps.node_id_map);
        golden_reference->remap_nodes(id_maps.node_id_map);
    }

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


    std::ofstream ofs(args.write_echo);
    if (!args.write_echo.empty()) {
        tatum::write_timing_graph(ofs, *timing_graph);
        tatum::write_timing_constraints(ofs, *timing_constraints);
        tatum::write_delay_model(ofs, *timing_graph, *delay_calculator);
        ofs.flush();
    }

    //Make all the analyzer types to test templates
    std::shared_ptr<tatum::TimingAnalyzer> setup_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    std::shared_ptr<tatum::TimingAnalyzer> hold_analyzer = tatum::AnalyzerFactory<tatum::SetupAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    std::shared_ptr<tatum::TimingAnalyzer> setup_hold_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);

    //Create the timing analyzer
    std::shared_ptr<tatum::TimingAnalyzer> serial_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis>::make(*timing_graph, *timing_constraints, *delay_calculator);
    auto serial_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(serial_analyzer);
    auto serial_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(serial_analyzer);

    //Performance variables
    float serial_verify_time = 0.;
    size_t serial_tags_verified = 0;
    std::map<std::string,std::vector<double>> serial_prof_data;
    {
        cout << "Running Serial Analysis " << args.num_serial_runs << " times" << endl;

        serial_prof_data = profile(args.num_serial_runs, serial_analyzer);

        cout << "\n";

        if(serial_analyzer->num_unconstrained_startpoints() > 0) {
            cout << "Warning: " << serial_analyzer->num_unconstrained_startpoints() << " sources are unconstrained\n";
        }
        if(serial_analyzer->num_unconstrained_endpoints() > 0) {
            cout << "Warning: " << serial_analyzer->num_unconstrained_endpoints() << " sinks are unconstrained\n";
        }

        tatum::NodeNumResolver name_resolver(*timing_graph, *delay_calculator, false);
        tatum::TimingReporter timing_reporter(name_resolver, *timing_graph, *timing_constraints);

        tatum::NodeNumResolver detailed_name_resolver(*timing_graph, *delay_calculator, true);
        tatum::TimingReporter detailed_timing_reporter(detailed_name_resolver, *timing_graph, *timing_constraints);

        auto dot_writer = make_graphviz_dot_writer(*timing_graph, *delay_calculator);

        std::vector<NodeId> nodes;
        if (args.debug_dot_node == -1) {
            //Pass
        } else if (args.debug_dot_node < -1) {
            auto tg_nodes = timing_graph->nodes();
            nodes = std::vector<NodeId>(tg_nodes.begin(), tg_nodes.end());
        } else if (args.debug_dot_node >= 0) {
            nodes = find_transitively_connected_nodes(*timing_graph, {NodeId(args.debug_dot_node)});
        }
        dot_writer.set_nodes_to_dump(nodes);

        std::shared_ptr<tatum::SetupTimingAnalyzer> echo_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(serial_analyzer);
        if(args.report && echo_setup_analyzer) {
            //write_dot_file_setup("tg_setup_annotated.dot", *timing_graph, *delay_calculator, *echo_setup_analyzer, nodes);
            dot_writer.write_dot_file("tg_setup_annotated.dot", *echo_setup_analyzer);
            timing_reporter.report_timing_setup("report_timing.setup.rpt", *echo_setup_analyzer);
            timing_reporter.report_skew_setup("report_skew.setup.rpt", *echo_setup_analyzer);
            timing_reporter.report_unconstrained_setup("report_unconstrained_timing.setup.rpt", *echo_setup_analyzer);

            detailed_timing_reporter.report_timing_setup("report_timing_detailed.setup.rpt", *echo_setup_analyzer);
            detailed_timing_reporter.report_skew_setup("report_skew_detailed.setup.rpt", *echo_setup_analyzer);
            detailed_timing_reporter.report_unconstrained_setup("report_unconstrained_timing_detailed.setup.rpt", *echo_setup_analyzer);
        }
        std::shared_ptr<tatum::HoldTimingAnalyzer> echo_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(serial_analyzer);
        if(args.report && echo_hold_analyzer) {
            //write_dot_file_hold("tg_hold_annotated.dot", *timing_graph, *delay_calculator, *echo_hold_analyzer, nodes);
            dot_writer.write_dot_file("tg_hold_annotated.dot", *echo_hold_analyzer);
            timing_reporter.report_timing_hold("report_timing.hold.rpt", *echo_hold_analyzer);
            timing_reporter.report_skew_hold("report_skew.hold.rpt", *echo_hold_analyzer);
            timing_reporter.report_unconstrained_hold("report_unconstrained_timing.hold.rpt", *echo_hold_analyzer);

            detailed_timing_reporter.report_timing_hold("report_timing_detailed.hold.rpt", *echo_hold_analyzer);
            detailed_timing_reporter.report_skew_hold("report_skew_detailed.hold.rpt", *echo_hold_analyzer);
            detailed_timing_reporter.report_unconstrained_hold("report_unconstrained_timing_detailed.hold.rpt", *echo_hold_analyzer);
        }

        //Verify
        clock_gettime(CLOCK_MONOTONIC, &verify_start);

        if (args.verify) {
            auto res = verify_analyzer(*timing_graph, serial_analyzer, *golden_reference);

            serial_tags_verified = res.first;

            if(!res.second) {
                cout << "Verification failed!\n";
                exit_code = 1;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &verify_end);
        serial_verify_time += tatum::time_sec(verify_start, verify_end);


        cout << endl;
        cout << "Serial Analysis took " << std::setprecision(6) << std::setw(6) << arithmean(serial_prof_data["analysis_sec"])*args.num_serial_runs << " sec";
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
        cout << endl << "Net Serial Analysis elapsed time: " << serial_analyzer->get_profiling_data("total_analysis_sec") << " sec over " << serial_analyzer->get_profiling_data("num_full_updates") << " full updates" << endl;
    }

    if (!args.write_echo.empty()) {
        tatum::write_analysis_result(ofs, *timing_graph, serial_analyzer);
        ofs.flush();
    }

    std::cout << endl;

    if (args.num_parallel_runs) {
        std::shared_ptr<tatum::TimingAnalyzer> parallel_analyzer = tatum::AnalyzerFactory<tatum::SetupHoldAnalysis,tatum::ParallelWalker>::make(*timing_graph, *timing_constraints, *delay_calculator);
        auto parallel_setup_analyzer = std::dynamic_pointer_cast<tatum::SetupTimingAnalyzer>(parallel_analyzer);
        auto parallel_hold_analyzer = std::dynamic_pointer_cast<tatum::HoldTimingAnalyzer>(parallel_analyzer);

        float parallel_verify_time = 0;
        size_t parallel_tags_verified = 0;
        std::map<std::string,std::vector<double>> parallel_prof_data;
        {
            cout << "Running Parrallel Analysis " << args.num_parallel_runs << " times" << endl;

            //Analyze
            parallel_prof_data = profile(args.num_parallel_runs, parallel_analyzer);

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

            if (args.verify) {
                cout << "\n";
                auto res = verify_analyzer(*timing_graph, parallel_analyzer, *golden_reference);

                parallel_tags_verified = res.first;

                if(!res.second) {
                    cout << "Verification failed!\n";
                    exit_code = 1;
                }
            }

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            parallel_verify_time += tatum::time_sec(verify_start, verify_end);

            cout << endl;
            cout << "Parallel Analysis took " << std::setprecision(6) << std::setw(6) << arithmean(parallel_prof_data["analysis_sec"])*args.num_parallel_runs << " sec";
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

        cout << endl << "Net Parallel Analysis elapsed time: " << parallel_analyzer->get_profiling_data("total_analysis_sec") << " sec over " << parallel_analyzer->get_profiling_data("num_full_updates") << " full updates" << endl;
    }

    //Tag stats
    if(serial_setup_analyzer) {
        print_setup_tags_histogram(*timing_graph, *serial_setup_analyzer);
    }

    if(serial_hold_analyzer) {
        print_hold_tags_histogram(*timing_graph, *serial_hold_analyzer);
    }

    //Critical paths
    cout << "\nCritical Paths:\n";
    auto cpds = find_critical_paths(*timing_graph, *timing_constraints, *serial_setup_analyzer); 
    for(auto cpd : cpds) {
        cout << "  " << cpd.launch_domain() << " -> " << cpd.capture_domain() << ": " << std::scientific << cpd.delay() << "\n";
    }

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    cout << endl << "Total time: " << tatum::time_sec(prog_start, prog_end) << " sec" << endl;

    return exit_code;
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
