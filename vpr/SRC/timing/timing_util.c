#include "timing_util.h"
#include "vtr_assert.h"

#include "globals.h"

float find_critical_path_delay(const tatum::SetupTimingAnalyzer& setup_analyzer) {

    //Look at all the edges to sink nodes in the graph and find the one with least slack
    tatum::NodeId least_slack_node;
    tatum::TimingTag least_slack_tag;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) { //All sink nodes
        for(tatum::EdgeId edge : g_timing_graph.node_in_edges(node)) { //All incoming edges
            for(tatum::TimingTag tag : setup_analyzer.setup_slacks(edge)) { //All slacks on edge
                if(!least_slack_node //Uninitialized
                   || least_slack_tag.time() > tag.time()) //Smaller
                {
                    least_slack_node = node;
                    least_slack_tag = tag;
                }
            }
        }
    }

    //Find the associated least-slack tag on the least-slack node
    float cpd = NAN;
    for(tatum::TimingTag tag : setup_analyzer.setup_tags(least_slack_node, tatum::TagType::DATA_ARRIVAL)) {
        if(tag.launch_clock_domain() == least_slack_tag.launch_clock_domain()) {
            cpd = tag.time().value();
            break;
        }
    }
    VTR_ASSERT(!std::isnan(cpd));

    return cpd;
}

float find_setup_total_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    float tns = 0.;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            tns += tag.time().value();
        }
    }
    return tns;
}

float find_setup_worst_negative_slack(const tatum::SetupTimingAnalyzer& setup_analyzer) {
    float wns = 0.;
    for(tatum::NodeId node : g_timing_graph.logical_outputs()) {
        for(tatum::TimingTag tag : setup_analyzer.setup_slacks(node)) {
            wns = std::min(wns, tag.time().value());
        }
    }
    return wns;
}
