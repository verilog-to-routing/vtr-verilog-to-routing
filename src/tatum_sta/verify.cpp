#include "verify.hpp"
#include <iostream>
#include "sta_util.hpp"
#include "util.hpp"

#define RELATIVE_EPSILON 1.e-5
#define ABSOLUTE_EPSILON 1.e-13

using std::cout;
using std::endl;

using tatum::NodeId;
using tatum::DomainId;
using tatum::LevelId;
using tatum::TimingGraph;
using tatum::NodeType;

bool verify_arr_tag(float arr_time, float vpr_arr_time, NodeId node_id, DomainId domain, const std::set<NodeId>& clock_gen_fanout_nodes, std::streamsize num_width);

bool verify_req_tag(float req_time, float vpr_req_time, NodeId node_id, DomainId domain, const std::set<NodeId>& const_gen_fanout_nodes, std::streamsize num_width);


int verify_analyzer(const TimingGraph& tg, const tatum::SetupTimingAnalyzer& analyzer, const VprArrReqTimes& expected_arr_req_times, const std::set<NodeId>& const_gen_fanout_nodes, const std::set<NodeId>& clock_gen_fanout_nodes) {
    //expected_arr_req_times.print();

    //std::cout << "Verifying Calculated Timing Against VPR" << std::endl;
    std::ios_base::fmtflags saved_flags = std::cout.flags();
    std::streamsize prec = std::cout.precision();
    std::streamsize width = std::cout.width();

    std::streamsize num_width = 10;
    std::cout.precision(3);
    std::cout << std::scientific;
    std::cout << std::setw(10);
    int arr_reqs_verified = 0;
    bool error = false;

    /*
     * Check from VPR to Tatum results
     */
    for(const DomainId domain : expected_arr_req_times.clocks()) {

        int arrival_nodes_checked = 0; //Count number of nodes checked
        int required_nodes_checked = 0; //Count number of nodes checked

        //Arrival check by level
        for(const LevelId ilevel : tg.levels()){
            //std::cout << "LEVEL " << ilevel << std::endl;
            for(const NodeId node_id : tg.level_nodes(ilevel)) {
                //std::cout << "Verifying node: " << node_id << " Launch: " << src_domain << " Capture: " << sink_domain << std::endl;
                const auto& node_data_tags = analyzer.get_setup_data_tags(node_id);
                float vpr_arr_time = expected_arr_req_times.get_arr_time(domain, node_id);

                //Check arrival
                auto data_tag_iter = node_data_tags.find_tag_by_clock_domain(domain);
                if(data_tag_iter == node_data_tags.end()) {
                    //Did not find a matching data tag

                    //See if there is an associated clock tag.
                    //Note that VPR doesn't handle seperate clock tags, but we place
                    //clock arrivals on FF_SINK and FF_SOURCE nodes from the clock network,
                    //so even if such a clock tag exists we don't want to compare it to VPR
                    if(tg.node_type(node_id) != NodeType::FF_SINK && tg.node_type(node_id) != NodeType::FF_SOURCE) {
                        const auto& node_clock_tags = analyzer.get_setup_clock_tags(node_id);
                        auto clock_tag_iter = node_clock_tags.find_tag_by_clock_domain(domain);
                        if(clock_tag_iter != node_clock_tags.end()) {
                            error |= verify_arr_tag(clock_tag_iter->arr_time().value(), vpr_arr_time, node_id, domain, clock_gen_fanout_nodes, num_width);

                        } else if(!isnan(vpr_arr_time)) {
                            error = true;
                            std::cout << "Node: " << node_id << " Clk: " << domain << std::endl;
                            std::cout << "\tERROR Found no arrival-time tag, but VPR arrival time was ";
                            std::cout << std::setw(num_width) << vpr_arr_time << " (expected NAN)" << std::endl;
                        } else {
                            TATUM_ASSERT(isnan(vpr_arr_time));
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
        TATUM_ASSERT(arrival_nodes_checked == (int) expected_arr_req_times.get_num_nodes());

        //Required check by level (in reverse)
        for(const LevelId level_id : tg.levels()) {
            for(const NodeId node_id : tg.level_nodes(level_id)) {

                const auto& node_data_tags = analyzer.get_setup_data_tags(node_id);
                float vpr_req_time = expected_arr_req_times.get_req_time(domain, node_id);
                //Check Required time
                auto data_tag_iter = node_data_tags.find_tag_by_clock_domain(domain);
                if(data_tag_iter == node_data_tags.end()) {
                    //See if there is an associated clock tag.
                    //Note that VPR doesn't handle seperate clock tags, but we place
                    //clock arrivals on FF_SINK and FF_SOURCE nodes from the clock network,
                    //so even if such a clock tag exists we don't want to compare it to VPR
                    if(tg.node_type(node_id) != NodeType::FF_SINK && tg.node_type(node_id) != NodeType::FF_SOURCE) {
                        const auto& node_clock_tags = analyzer.get_setup_clock_tags(node_id);
                        auto clock_tag_iter = node_clock_tags.find_tag_by_clock_domain(domain);
                        if(clock_tag_iter != node_clock_tags.end()) {
                            error |= verify_req_tag(clock_tag_iter->req_time().value(), vpr_req_time, node_id, domain, const_gen_fanout_nodes, num_width);
                        } else if(!isnan(vpr_req_time)) {
                            error = true;
                            std::cout << "Node: " << node_id << " Clk: " << domain  << std::endl;
                            std::cout << "\tERROR Found no required-time tag, but VPR required time was " << std::setw(num_width);
                            std::cout << vpr_req_time << " (expected NAN)" << std::endl;
                        }
                    }
                } else {
                    error |= verify_req_tag(data_tag_iter->req_time().value(), vpr_req_time, node_id, domain, const_gen_fanout_nodes, num_width);

                }
                arr_reqs_verified++;
                required_nodes_checked++;
            }
        }
        TATUM_ASSERT(required_nodes_checked == (int) expected_arr_req_times.get_num_nodes());
    }

    /*
     * Check from Tatum to VPR
     */

    //Check by level
//#ifdef CHECK_TATUM_TO_VPR_DIFFERENCES
#if 0
    for(int ilevel = 0; ilevel <tg.num_levels(); ilevel++) {
        //std::cout << "LEVEL " << ilevel << std::endl;
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
        std::cout << "Timing verification FAILED!" << std::endl;
        exit(1);
    } else {
        //std::cout << "Timing verification SUCCEEDED" << std::endl;
    }

    std::cout.flags(saved_flags);
    std::cout.precision(prec);
    std::cout.width(width);
    return arr_reqs_verified;
}


bool verify_arr_tag(float arr_time, float vpr_arr_time, NodeId node_id, DomainId domain, const std::set<NodeId>& clock_gen_fanout_nodes, std::streamsize num_width) {
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
        TATUM_ASSERT(!isnan(arr_rel_err) && !isnan(arr_abs_err));
        TATUM_ASSERT(arr_rel_err < RELATIVE_EPSILON || arr_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}

bool verify_req_tag(float req_time, float vpr_req_time, NodeId node_id, DomainId domain, const std::set<NodeId>& const_gen_fanout_nodes, std::streamsize num_width) {
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
        TATUM_ASSERT(!isnan(req_rel_err) && !isnan(req_abs_err));
        TATUM_ASSERT(req_rel_err < RELATIVE_EPSILON || req_abs_err < ABSOLUTE_EPSILON);
    }
    return error;
}
