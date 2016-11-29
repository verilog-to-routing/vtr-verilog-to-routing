#ifndef VERIFY_HPP
#define VERIFY_HPP
#include <memory>

#include "TimingAnalyzer.hpp"
#include "TimingGraph.hpp"
#include "golden_reference.hpp"

std::pair<size_t,bool> verify_analyzer(const tatum::TimingGraph& tg, std::shared_ptr<tatum::TimingAnalyzer> analyzer, GoldenReference& gr);

#endif
