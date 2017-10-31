#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/timing_analyzers.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

namespace tatum {

void write_timing_graph(std::ostream& os, const TimingGraph& tg);
void write_timing_constraints(std::ostream& os, const TimingConstraints& tc);
void write_analysis_result(std::ostream& os, const TimingGraph& tg, const std::shared_ptr<const TimingAnalyzer> analyzer);
void write_delay_model(std::ostream& os, const TimingGraph& tg, const DelayCalculator& dc);

void write_echo(std::string filename, const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const std::shared_ptr<const TimingAnalyzer> analyzer);
void write_echo(std::ostream& os, const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const std::shared_ptr<const TimingAnalyzer> analyzer);

}
