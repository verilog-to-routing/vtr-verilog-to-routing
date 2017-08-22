#ifndef TATUM_ANALYZER_FACTORY_FWD_HPP
#define TATUM_ANALYZER_FACTORY_FWD_HPP

#include "graph_walkers_fwd.hpp"

namespace tatum {

///Factor class to construct timing analyzers
///
///\tparam Visitor The analysis type visitor (e.g. SetupAnalysis)
///\tparam GraphWalker The graph walker to use (defaults to serial traversals)
template<class Visitor,
         class GraphWalker=SerialWalker>
struct AnalyzerFactory;

} //namepsace

#endif
