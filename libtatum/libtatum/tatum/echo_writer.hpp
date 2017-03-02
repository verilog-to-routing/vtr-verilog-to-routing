#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/timing_analyzers.hpp"

namespace tatum {

void write_timing_graph(std::ostream& os, const TimingGraph& tg);
void write_timing_constraints(std::ostream& os, const TimingConstraints& tc);
void write_analysis_result(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<const TimingAnalyzer> analyzer);

template<class DelayCalc>
void write_delay_model(std::ostream& os, const TimingGraph& tg, const DelayCalc& dc) {
    os << "delay_model:\n";
    for(auto edge_id : tg.edges()) {
        NodeId src_node = tg.edge_src_node(edge_id);
        NodeId sink_node = tg.edge_sink_node(edge_id);

        os << " edge: " << size_t(edge_id);
        if(tg.node_type(src_node) == NodeType::CPIN && tg.node_type(sink_node) == NodeType::SINK) {
            os << " setup_time: " << dc.setup_time(tg, edge_id).value();
            os << " hold_time: " << dc.hold_time(tg, edge_id).value();
        } else {
            os << " min_delay: " << dc.min_edge_delay(tg, edge_id).value();
            os << " max_delay: " << dc.max_edge_delay(tg, edge_id).value();
        }
        os << "\n";
    }
    os << "\n";
}

template<class DelayCalc>
void write_echo(std::string filename, const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const std::shared_ptr<const TimingAnalyzer> analyzer) {
    std::ofstream os(filename);

    write_echo(os, tg, tc, dc, analyzer);
}

template<class DelayCalc>
void write_echo(std::ostream& os, const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const std::shared_ptr<const TimingAnalyzer> analyzer) {
    write_timing_graph(os, tg);
    write_timing_constraints(os, tc);
    write_delay_model(os, tg, dc);
    write_analysis_result(os, tg, analyzer);
}


}
