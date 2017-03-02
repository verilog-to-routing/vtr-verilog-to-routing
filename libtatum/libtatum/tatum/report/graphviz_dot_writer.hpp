#ifndef GRAPHVIZ_DOT_WRITER_HPP
#define GRAPHVIZ_DOT_WRITER_HPP

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/timing_analyzers.hpp"
#include "tatum/delay_calc/FixedDelayCalculator.hpp"
#include <vector>

namespace tatum {
/*
 * These routines dump out the timing graph into a graphviz file (in DOT format)
 *
 * This has proven invaluable for debugging. DOT files can be viewed interactively
 * using the 'xdot' tool.
 *
 * If the delay calculator is provided, edge delayes are included in the output
 * If the analyzer is provided, the calculated timing tags are included in the output
 *
 * If a specific set of nodes is provided, these will be the nodes included in the output
 * (if no nodes are provided, the default, then the whole graph will be included in the output,
 * unless it is too large to be practical).
 */

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_setup(std::string filename,
                          const TimingGraph& tg,
                          const DelayCalc& delay_calc = DelayCalc(),
                          const SetupTimingAnalyzer& analyzer = SetupTimingAnalyzer(),
                          std::vector<NodeId> nodes = std::vector<NodeId>());

template<class DelayCalc=FixedDelayCalculator>
void write_dot_file_hold(std::string filename,
                         const TimingGraph& tg,
                         const DelayCalc& delay_calc = DelayCalc(),
                         const HoldTimingAnalyzer& analyzer = TimingAnalyzer(),
                         std::vector<NodeId> nodes = std::vector<NodeId>());

} //namespace

#include "graphviz_dot_writer.tpp" //implementation

#endif
