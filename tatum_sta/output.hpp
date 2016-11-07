#pragma once
#include <memory>
#include <iostream>
#include "TimingGraph.hpp"
#include "timing_constraints_fwd.hpp"
#include "TimingAnalyzer.hpp"

void write_timing_graph(std::ostream& os, const tatum::TimingGraph& tg);
void write_timing_constraints(std::ostream& os, const tatum::TimingGraph& tg, const tatum::TimingConstraints& tc);
void write_analysis_result(std::ostream& os, const tatum::TimingGraph& tg, const std::shared_ptr<tatum::TimingAnalyzer> analyzer);

template<class DelayCalc>
void write_delay_model(std::ostream& os, const tatum::TimingGraph& tg, const DelayCalc& dc) {
    os << "delay_model:\n";
    for(auto edge_id : tg.edges()) {
        os << " edge: " << size_t(edge_id);
        os << " min_delay: " << dc.min_edge_delay(tg, edge_id).value();
        os << " max_delay: " << dc.max_edge_delay(tg, edge_id).value();
        os << "\n";
    }
    os << "\n";
}
