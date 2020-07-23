#ifndef VERIFY_HPP
#define VERIFY_HPP
#include <memory>

#include "tatum/timing_analyzers_fwd.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "golden_reference.hpp"

std::pair<size_t,bool> verify_analyzer(const tatum::TimingGraph& tg, std::shared_ptr<tatum::TimingAnalyzer> analyzer, GoldenReference& gr);

std::pair<size_t,bool> verify_equivalent_analysis(const tatum::TimingGraph& tg, const tatum::DelayCalculator& dc, std::shared_ptr<tatum::TimingAnalyzer> ref_analyzer,  std::shared_ptr<tatum::TimingAnalyzer> check_analyzer);

#endif
