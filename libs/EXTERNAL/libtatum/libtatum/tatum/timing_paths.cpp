#include "tatum/timing_paths.hpp"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/report/TimingReportTagRetriever.hpp"
#include "tatum/report/timing_path_tracing.hpp"
#include "tatum/error.hpp"

namespace tatum {

//Generic path tracer for setup or hold
TimingPath trace_path(const TimingGraph& timing_graph,
                      const detail::TagRetriever& tag_retriever, 
                      const DomainId launch_domain,
                      const DomainId capture_domain,
                      const NodeId sink_node);


std::vector<TimingPathInfo> find_critical_paths(const TimingGraph& timing_graph, 
                                                const TimingConstraints& timing_constraints, 
                                                const SetupTimingAnalyzer& setup_analyzer) {
    std::vector<TimingPathInfo> cpds;

    //We calculate the critical path delay (CPD) for each pair of clock domains (which are connected to each other)
    //
    //Intuitively, CPD is the smallest period (maximum frequency) we can run the launch clock at while not violating
    //the constraint.
    //
    //We calculate CPD as:
    //
    //      CPD = constarint - slack
    //
    //If slack < 0, this will lengthen the constraint to it's feasible value (i.e. the CPD)
    //If slack > 0, this will tighten the constraint to it's feasible value (i.e. the CPD)
    //
    //Using the slack to calculate CPD implicitly accounts for i/o delays, clock uncertainty, clock latency etc,
    //as they are already included in the slack.

    //To ensure we find the critical path delay, we look at all timing endpoints (i.e. logical_outputs()) and keep
    //the largest for each domain pair
    for(NodeId node : timing_graph.logical_outputs()) {

        //Look at each data arrival
        for(TimingTag slack_tag : setup_analyzer.setup_slacks(node)) {
            Time slack = slack_tag.time();
            if(!slack.valid()) {
                throw Error("slack is not valid", node);
            }

            Time constraint = Time(timing_constraints.setup_constraint(slack_tag.launch_clock_domain(), slack_tag.capture_clock_domain()));
            if(!constraint.valid()) {
                throw Error("constraint is not valid", node);
            }

            Time cpd = Time(constraint) - slack;
            if(!cpd.valid()) {
                throw Error("cpd is not valid", node);
            }

            //Record the path info
            TimingPathInfo path(TimingType::SETUP,
                                cpd, slack,
                                NodeId::INVALID(), //We currently don't trace the path back to the start point, so just mark as invalid
                                node, 
                                slack_tag.launch_clock_domain(), slack_tag.capture_clock_domain());

            //Find any existing path for this domain pair
            auto cmp = [&path](const TimingPathInfo& elem) {
                return    elem.launch_domain() == path.launch_domain()
                       && elem.capture_domain() == path.capture_domain();
            };
            auto iter = std::find_if(cpds.begin(), cpds.end(), cmp);

            if(iter == cpds.end()) {
                //New domain pair
                cpds.push_back(path);
            } else if(iter->delay() < path.delay()) {
                //New max CPD
                *iter = path;
            }
        }
    }

    return cpds;
}

TimingPath trace_setup_path(const TimingGraph& timing_graph, 
                            const SetupTimingAnalyzer& setup_analyzer,
                            const DomainId launch_domain,
                            const DomainId capture_domain,
                            const NodeId sink_node) {

    detail::SetupTagRetriever tag_retriever(setup_analyzer);
    return detail::trace_path(timing_graph, tag_retriever, launch_domain, capture_domain, sink_node);
}

TimingPath trace_hold_path(const TimingGraph& timing_graph, 
                           const HoldTimingAnalyzer& hold_analyzer,
                           const DomainId launch_domain,
                           const DomainId capture_domain,
                           const NodeId sink_node) {

    detail::HoldTagRetriever tag_retriever(hold_analyzer);
    return detail::trace_path(timing_graph, tag_retriever, launch_domain, capture_domain, sink_node);
}


} //namespace
