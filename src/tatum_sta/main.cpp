#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <array>
#include <iostream>

#include <valgrind/callgrind.h>

#include "assert.hpp"

#include "sta_util.hpp"

#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingNode.hpp"

#include "SerialTimingAnalyzer.hpp"

//Cilk variants
#include "ParallelLevelizedCilkTimingAnalyzer.hpp"
#include "ParallelDynamicCilkTimingAnalyzer.hpp"


//Illegal versions used for upper-bound speed-up estimates
#include "ParallelNoDependancyCilkTimingAnalyzer.hpp"

#include "vpr_timing_graph_common.hpp"

#define NUM_SERIAL_RUNS 5
#define NUM_PARALLEL_RUNS 100 //NUM_SERIAL_RUNS
#define OPTIMIZE_NODE_EDGE_ORDER

//Currently don't check for differences in the other direction (from us to VPR),
//since we do a single traversal we generate extra ancillary timing tags which
//will not match VPR
//#define CHECK_TATUM_TO_VPR_DIFFERENCES
using std::cout;
using std::endl;

int verify_analyzer(const TimingGraph& tg, const TimingAnalyzer& analyzer, const VprArrReqTimes& expected_arr_req_times, const std::set<NodeId>& const_gen_fanout_nodes, const std::set<NodeId>& clock_gen_fanout_nodes);
bool verify_arr_tag(float arr_time, float vpr_arr_time, NodeId node_id, int domain, const std::set<NodeId>& clock_gen_fanout_nodes, std::streamsize num_width);
bool verify_req_tag(float req_time, float vpr_req_time, NodeId node_id, int domain, const std::set<NodeId>& const_gen_fanout_nodes, std::streamsize num_width);


int main(int argc, char** argv) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " tg_echo_file" << endl;
        return 1;
    }

    struct timespec prog_start, load_start, analyze_start, verify_start;
    struct timespec prog_end, load_end, analyze_end, verify_end;

    clock_gettime(CLOCK_MONOTONIC, &prog_start);

    cout << "Time class sizeof  = " << sizeof(Time) << " bytes. Time Vec Width: " << TIME_VEC_WIDTH << endl;
    cout << "Time class alignof = " << alignof(Time) << endl;

    cout << "TimingTag class sizeof  = " << sizeof(TimingTag) << " bytes." << endl;
    cout << "TimingTag class alignof = " << alignof(TimingTag) << " bytes." << endl;

    cout << "TimingTags class sizeof  = " << sizeof(TimingTags) << " bytes." << endl;
    cout << "TimingTags class alignof = " << alignof(TimingTags) << " bytes." << endl;

    TimingGraph timing_graph;
    TimingConstraints timing_constraints;
    VprArrReqTimes orig_expected_arr_req_times;
    VprArrReqTimes expected_arr_req_times;
    std::set<NodeId> const_gen_fanout_nodes;
    std::set<NodeId> clock_gen_fanout_nodes;

    SerialTimingAnalyzer serial_analyzer;
    //ParallelLevelizedCilkTimingAnalyzer parallel_analyzer = ParallelLevelizedCilkTimingAnalyzer();

    //ParallelNoDependancyCilkTimingAnalyzer parallel_analyzer = ParallelNoDependancyCilkTimingAnalyzer();
    //ParallelDynamicCilkTimingAnalyzer parallel_analyzer = ParallelDynamicCilkTimingAnalyzer();
    //ParallelLevelizedOpenMPTimingAnalyzer parallel_analyzer = ParallelLevelizedOpenMPTimingAnalyzer();

    {
        clock_gettime(CLOCK_MONOTONIC, &load_start);

        yyin = fopen(argv[1], "r");
        if(yyin != NULL) {
            int error = yyparse(timing_graph, orig_expected_arr_req_times, timing_constraints);
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



        cout << "Timing Graph Stats:" << endl;
        cout << "  Nodes : " << timing_graph.num_nodes() << endl;
        cout << "  Levels: " << timing_graph.num_levels() << endl;
        cout << "Num Clocks: " << orig_expected_arr_req_times.get_num_clocks() << endl;

        cout << endl;

#ifdef OPTIMIZE_NODE_EDGE_ORDER
        timing_graph.contiguize_level_edges();
        std::vector<NodeId> vpr_node_map = timing_graph.contiguize_level_nodes();

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

    float serial_analysis_time = 0.;
    float serial_pretraverse_time = 0.;
    float serial_fwdtraverse_time = 0.;
    float serial_bcktraverse_time = 0.;
    float serial_analysis_time_avg = 0.;
    float serial_pretraverse_time_avg = 0.;
    float serial_fwdtraverse_time_avg = 0.;
    float serial_bcktraverse_time_avg = 0.;
    float serial_verify_time = 0.;
    int serial_arr_req_verified = 0;
    {
        cout << "Running Serial Analysis " << NUM_SERIAL_RUNS << " times" << endl;

        CALLGRIND_START_INSTRUMENTATION;
        for(int i = 0; i < NUM_SERIAL_RUNS; i++) {
            //Analyze

            clock_gettime(CLOCK_MONOTONIC, &analyze_start);

            CALLGRIND_TOGGLE_COLLECT;

            ta_runtime traversal_times = serial_analyzer.calculate_timing(timing_graph, timing_constraints);

            CALLGRIND_TOGGLE_COLLECT;

            clock_gettime(CLOCK_MONOTONIC, &analyze_end);

            serial_analysis_time += time_sec(analyze_start, analyze_end);
            serial_pretraverse_time += traversal_times.pre_traversal;
            serial_fwdtraverse_time += traversal_times.fwd_traversal;
            serial_bcktraverse_time += traversal_times.bck_traversal;

            cout << ".";
            cout.flush();

            //print_timing_tags(timing_graph, serial_analyzer);

            //Verify
            clock_gettime(CLOCK_MONOTONIC, &verify_start);

            if(serial_analyzer.is_correct()) {
                serial_arr_req_verified = verify_analyzer(timing_graph, serial_analyzer,
                                                          expected_arr_req_times, const_gen_fanout_nodes,
                                                          clock_gen_fanout_nodes );
            }

            clock_gettime(CLOCK_MONOTONIC, &verify_end);
            serial_verify_time += time_sec(verify_start, verify_end);

            if(i == NUM_SERIAL_RUNS-1) {
                serial_analyzer.save_level_times(timing_graph, "serial_level_times.csv");
            } else {
                serial_analyzer.reset_timing();
            }
        }
        CALLGRIND_STOP_INSTRUMENTATION;

        serial_analysis_time_avg = serial_analysis_time / NUM_SERIAL_RUNS;
        serial_pretraverse_time_avg = serial_pretraverse_time / NUM_SERIAL_RUNS;
        serial_fwdtraverse_time_avg = serial_fwdtraverse_time / NUM_SERIAL_RUNS;
        serial_bcktraverse_time_avg = serial_bcktraverse_time / NUM_SERIAL_RUNS;

        cout << endl;
        cout << "Serial Analysis took " << serial_analysis_time << " sec, AVG: " << serial_analysis_time_avg << " s" << endl;
        cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_pretraverse_time_avg << " s";
        cout << " (" << std::setprecision(2) << serial_pretraverse_time_avg/serial_analysis_time_avg << ")" << endl;
        cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_fwdtraverse_time_avg << " s";
        cout << " (" << std::setprecision(2) << serial_fwdtraverse_time_avg/serial_analysis_time_avg << ")" << endl;
        cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << serial_bcktraverse_time_avg << " s";
        cout << " (" << std::setprecision(2) << serial_bcktraverse_time_avg/serial_analysis_time_avg << ")" << endl;
        cout << "Verifying Serial Analysis took: " << time_sec(verify_start, verify_end) << " sec" << endl;
        if(serial_arr_req_verified != 2*timing_graph.num_nodes()*expected_arr_req_times.get_num_clocks()) { //2x for arr and req
            cout << "WARNING: Expected arr/req times differ from number of nodes. Verification may not have occured!" << endl;
        } else {
            cout << "\tVerified " << serial_arr_req_verified << " arr/req times accross " << timing_graph.num_nodes() << " nodes and " << expected_arr_req_times.get_num_clocks() << " clocks" << endl;
        }
        cout << endl;

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
 *        cout << "Running Parrallel Analysis " << NUM_PARALLEL_RUNS << " times" << endl;
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
 *            cout << ".";
 *            cout.flush();
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
 *        cout << endl;
 *        if(!parallel_analyzer.is_correct()) {
 *            cout << "Skipped correctness verification" << endl;
 *        }
 *        cout << "Parallel Analysis took " << parallel_analysis_time << " sec, AVG: " << std::setprecision(6) << std::setw(6) << parallel_analysis_time_avg << " s" << endl;
 *        cout << "\tPre-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_pretraverse_time_avg << " s";
 *        cout << " (" << std::setprecision(2) << parallel_pretraverse_time_avg/parallel_analysis_time_avg << ")" << endl;
 *        cout << "\tFwd-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_fwdtraverse_time_avg << " s";
 *        cout << " (" << std::setprecision(2) << parallel_fwdtraverse_time_avg/parallel_analysis_time_avg << ")" << endl;
 *        cout << "\tBck-traversal Avg: " << std::setprecision(6) << std::setw(6) << parallel_bcktraverse_time_avg << " s";
 *        cout << " (" << std::setprecision(2) << parallel_bcktraverse_time_avg/parallel_analysis_time_avg << ")" << endl;
 *        cout << "Verifying Parallel Analysis took: " << time_sec(verify_start, verify_end) << " sec" << endl;
 *        cout << endl;
 *    }
 *
 *
 *
 *    cout << "Parallel Speed-Up: " << std::fixed << serial_analysis_time_avg / parallel_analysis_time_avg << "x" << endl;
 *    cout << "\tPre-traversal: " << std::fixed << serial_pretraverse_time_avg / parallel_pretraverse_time_avg << "x" << endl;
 *    cout << "\tFwd-traversal: " << std::fixed << serial_fwdtraverse_time_avg / parallel_fwdtraverse_time_avg << "x" << endl;
 *    cout << "\tBck-traversal: " << std::fixed << serial_bcktraverse_time_avg / parallel_bcktraverse_time_avg << "x" << endl;
 *    cout << endl;
 */

    clock_gettime(CLOCK_MONOTONIC, &prog_end);

    cout << endl << "Total time: " << time_sec(prog_start, prog_end) << " sec" << endl;

    return 0;
}

#define RELATIVE_EPSILON 1.e-5
#define ABSOLUTE_EPSILON 1.e-13

int verify_analyzer(const TimingGraph& tg, const TimingAnalyzer& analyzer, const VprArrReqTimes& expected_arr_req_times, const std::set<NodeId>& const_gen_fanout_nodes, const std::set<NodeId>& clock_gen_fanout_nodes) {
    //expected_arr_req_times.print();

    //cout << "Verifying Calculated Timing Against VPR" << endl;
    std::ios_base::fmtflags saved_flags = cout.flags();
    std::streamsize prec = cout.precision();
    std::streamsize width = cout.width();

    std::streamsize num_width = 10;
    cout.precision(3);
    cout << std::scientific;
    cout << std::setw(10);
    int arr_reqs_verified = 0;
    bool error = false;

    /*
     * Check from VPR to Tatum results
     */
    for(DomainId domain : expected_arr_req_times.clocks()) {

        int arrival_nodes_checked = 0; //Count number of nodes checked
        int required_nodes_checked = 0; //Count number of nodes checked

        //Arrival check by level
        for(int ilevel = 0; ilevel <tg.num_levels(); ilevel++) {
            //cout << "LEVEL " << ilevel << endl;
            for(NodeId node_id : tg.level(ilevel)) {
                //cout << "Verifying node: " << node_id << " Launch: " << src_domain << " Capture: " << sink_domain << endl;
                const TimingTags& node_data_tags = analyzer.data_tags(node_id);
                float vpr_arr_time = expected_arr_req_times.get_arr_time(domain, node_id);

                //Check arrival
                TimingTagConstIterator data_tag_iter = node_data_tags.find_tag_by_clock_domain(domain);
                if(data_tag_iter == node_data_tags.end()) {
                    //Did not find a matching data tag

                    //See if there is an associated clock tag.
                    //Note that VPR doesn't handle seperate clock tags, but we place
                    //clock arrivals on FF_SINK and FF_SOURCE nodes from the clock network,
                    //so even if such a clock tag exists we don't want to compare it to VPR
                    if(tg.node_type(node_id) != TN_Type::FF_SINK && tg.node_type(node_id) != TN_Type::FF_SOURCE) {
                        const TimingTags& node_clock_tags = analyzer.clock_tags(node_id);
                        TimingTagConstIterator clock_tag_iter = node_clock_tags.find_tag_by_clock_domain(domain);
                        if(clock_tag_iter != node_data_tags.end()) {
                            error |= verify_arr_tag(clock_tag_iter->arr_time().value(), vpr_arr_time, node_id, domain, clock_gen_fanout_nodes, num_width);

                        } else if(!isnan(vpr_arr_time)) {
                            error = true;
                            cout << "Node: " << node_id << " Clk: " << domain << endl;
                            cout << "\tERROR Found no arrival-time tag, but VPR arrival time was ";
                            cout << std::setw(num_width) << vpr_arr_time << " (expected NAN)" << endl;
                        } else {
                            VERIFY(isnan(vpr_arr_time));
                        }
                    }
                } else {
                    error |= verify_arr_tag(data_tag_iter->arr_time().value(), vpr_arr_time, node_id, domain, clock_gen_fanout_nodes, num_width);
                }
                arr_reqs_verified ++;
                arrival_nodes_checked++;
            }
        }
        //Since we walk our version of the graph make sure we see the same number of nodes as VPR
        VERIFY(arrival_nodes_checked == (int) expected_arr_req_times.get_num_nodes());

        //Required check by level (in reverse)
        for(int ilevel = tg.num_levels() - 1; ilevel >= 0; ilevel--) {
            for(NodeId node_id : tg.level(ilevel)) {

                const TimingTags& node_data_tags = analyzer.data_tags(node_id);
                float vpr_req_time = expected_arr_req_times.get_req_time(domain, node_id);
                //Check Required time
                TimingTagConstIterator data_tag_iter = node_data_tags.find_tag_by_clock_domain(domain);
                if(data_tag_iter == node_data_tags.end()) {
                    //See if there is an associated clock tag.
                    //Note that VPR doesn't handle seperate clock tags, but we place
                    //clock arrivals on FF_SINK and FF_SOURCE nodes from the clock network,
                    //so even if such a clock tag exists we don't want to compare it to VPR
                    if(tg.node_type(node_id) != TN_Type::FF_SINK && tg.node_type(node_id) != TN_Type::FF_SOURCE) {
                        const TimingTags& node_clock_tags = analyzer.clock_tags(node_id);
                        TimingTagConstIterator clock_tag_iter = node_clock_tags.find_tag_by_clock_domain(domain);
                        if(clock_tag_iter != node_data_tags.end()) {
                            error |= verify_req_tag(clock_tag_iter->req_time().value(), vpr_req_time, node_id, domain, const_gen_fanout_nodes, num_width);
                        } else if(!isnan(vpr_req_time)) {
                            error = true;
                            cout << "Node: " << node_id << " Clk: " << domain  << endl;
                            cout << "\tERROR Found no required-time tag, but VPR required time was " << std::setw(num_width);
                            cout << vpr_req_time << " (expected NAN)" << endl;
                        }
                    }
                } else {
                    error |= verify_req_tag(data_tag_iter->req_time().value(), vpr_req_time, node_id, domain, const_gen_fanout_nodes, num_width);

                }
                arr_reqs_verified++;
                required_nodes_checked++;
            }
        }
        VERIFY(required_nodes_checked == (int) expected_arr_req_times.get_num_nodes());
    }

    /*
     * Check from Tatum to VPR
     */

    //Check by level
//#ifdef CHECK_TATUM_TO_VPR_DIFFERENCES
#if 0
    for(int ilevel = 0; ilevel <tg.num_levels(); ilevel++) {
        //cout << "LEVEL " << ilevel << endl;
        for(NodeId node_id : tg.level(ilevel)) {
            for(const TimingTag& data_tag : analyzer.data_tags(node_id)) {
                //Arrival
                float arr_time = data_tag.arr_time().value();
                float vpr_arr_time = expected_arr_req_times.get_arr_time(data_tag.clock_domain(), node_id);
                verify_arr_tag(arr_time, vpr_arr_time, node_id, data_tag.clock_domain(), clock_gen_fanout_nodes, num_width);

                //Required
                float req_time = data_tag.req_time().value();
                float vpr_req_time = expected_arr_req_times.get_req_time(data_tag.clock_domain(), node_id);
                verify_req_tag(req_time, vpr_req_time, node_id, data_tag.clock_domain(), const_gen_fanout_nodes, num_width);
            }
        }
    }
#endif

    if(error) {
        cout << "Timing verification FAILED!" << endl;
        exit(1);
    } else {
        //cout << "Timing verification SUCCEEDED" << endl;
    }

    cout.flags(saved_flags);
    cout.precision(prec);
    cout.width(width);
    return arr_reqs_verified;
}


bool verify_arr_tag(float arr_time, float vpr_arr_time, NodeId node_id, int domain, const std::set<NodeId>& clock_gen_fanout_nodes, std::streamsize num_width) {
    bool error = false;
    float arr_abs_err = fabs(arr_time - vpr_arr_time);
    float arr_rel_err = relative_error(arr_time, vpr_arr_time);
    if(isnan(arr_time) && isnan(arr_time) != isnan(vpr_arr_time)) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
        cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
        cout << "\tERROR Calculated arrival time was nan and didn't match VPR." << endl;
    } else if (!isnan(arr_time) && isnan(vpr_arr_time)) {
        if(clock_gen_fanout_nodes.count(node_id)) {
            //Pass, clock gen fanout can be NAN in VPR but have a value here,
            //since (unlike VPR) we explictly track clock arrivals as tags
        } else {
            error = true;
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
            cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
            cout << "\tERROR Calculated arrival time was not nan but VPR expected nan." << endl;
        }
    } else if (isnan(arr_time) && isnan(vpr_arr_time)) {
        //They agree, pass
    } else if(arr_rel_err > RELATIVE_EPSILON && arr_abs_err > ABSOLUTE_EPSILON) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Arr: " << std::setw(num_width) << arr_time;
        cout << " VPR_Arr: " << std::setw(num_width) << vpr_arr_time << endl;
        cout << "\tERROR arrival time abs, rel errs: " << std::setw(num_width) << arr_abs_err;
        cout << ", " << std::setw(num_width) << arr_rel_err << endl;
    } else {
        VERIFY(!isnan(arr_rel_err) && !isnan(arr_abs_err));
        VERIFY(arr_rel_err < RELATIVE_EPSILON || arr_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}

bool verify_req_tag(float req_time, float vpr_req_time, NodeId node_id, int domain, const std::set<NodeId>& const_gen_fanout_nodes, std::streamsize num_width) {
    bool error = false;
    float req_abs_err = fabs(req_time - vpr_req_time);
    float req_rel_err = relative_error(req_time, vpr_req_time);
    if(isnan(req_time) && isnan(req_time) != isnan(vpr_req_time)) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Req: " << std::setw(num_width) << req_time;
        cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
        cout << "\tERROR Calculated required time was nan and didn't match VPR." << endl;
    } else if (!isnan(req_time) && isnan(vpr_req_time)) {
        if (const_gen_fanout_nodes.count(node_id)) {
            //VPR doesn't propagate required times along paths sourced by constant generators
            //but we do, so ignore such errors
#if 0
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Req: " << std::setw(num_width) << req_time;
            cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
            cout << "\tOK since " << node_id << " in fanout of Constant Generator" << endl;
#endif
        } else {
            error = true;
            cout << "Node: " << node_id << " Clk: " << domain;
            cout << " Calc_Req: " << std::setw(num_width) << req_time;
            cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
            cout << "\tERROR Calculated required time was not nan but VPR expected nan." << endl;
        }

    } else if (isnan(req_time) && isnan(vpr_req_time)) {
        //They agree, pass
    } else if(req_rel_err > RELATIVE_EPSILON && req_abs_err > ABSOLUTE_EPSILON) {
        error = true;
        cout << "Node: " << node_id << " Clk: " << domain;
        cout << " Calc_Req: " << std::setw(num_width) << req_time;
        cout << " VPR_Req: " << std::setw(num_width) << vpr_req_time << endl;
        cout << "\tERROR required time abs, rel errs: " << std::setw(num_width) << req_abs_err;
        cout << ", " << std::setw(num_width) << req_rel_err << endl;
    } else {
        VERIFY(!isnan(req_rel_err) && !isnan(req_abs_err));
        VERIFY(req_rel_err < RELATIVE_EPSILON || req_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}
