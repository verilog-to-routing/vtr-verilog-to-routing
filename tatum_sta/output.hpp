#pragma once
#include <memory>
#include <iostream>
#include "TimingGraph.hpp"
#include "timing_constraints_fwd.hpp"
#include "TimingAnalyzer.hpp"

void write_timing_graph(std::ostream& os, const tatum::TimingGraph& tg);
void write_timing_constraints(std::ostream& os, const tatum::TimingConstraints& tc);
void write_analysis_result(std::ostream& os, const tatum::TimingGraph& tg, const std::shared_ptr<tatum::TimingAnalyzer> analyzer);

template<class DelayCalc>
void write_delay_model(std::ostream& os, const tatum::TimingGraph& tg, const DelayCalc& dc) {
    os << "delay_model:\n";
    for(auto edge_id : tg.edges()) {
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        os << " edge: " << size_t(edge_id);
        if(tg.node_type(src_node) == tatum::NodeType::CPIN && tg.node_type(sink_node) == tatum::NodeType::SINK) {
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
