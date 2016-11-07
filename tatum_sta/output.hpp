#pragma once
#include <memory>
#include <iosfwd>
#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "TimingAnalyzer.hpp"

void write_timing_graph(std::ostream& os, const tatum::TimingGraph& tg);
void write_timing_constraints(std::ostream& os, const tatum::TimingGraph& tg, const tatum::TimingConstraints& tc);
void write_analysis_result(std::ostream& os, const tatum::TimingGraph& tg, const std::shared_ptr<tatum::TimingAnalyzer> analyzer);
