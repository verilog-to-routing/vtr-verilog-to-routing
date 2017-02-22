#ifndef TATUM_ANALYZER_FACTORY_FWD_HPP
#define TATUM_ANALYZER_FACTORY_FWD_HPP

namespace tatum {

///Factor class to construct timing analyzers
///
///\tparam Visitor The analysis type visitor (e.g. SetupAnalysis)
///\tparam GraphWalker The graph walker to use (defaults to serial traversals)
template<class Visitor,
         template<class V, class D> class GraphWalker=SerialWalker>
struct AnalyzerFactory;

} //namepsace

#endif
