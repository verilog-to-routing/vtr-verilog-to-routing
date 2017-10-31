#ifndef TATUM_ANALYZER_FACTORY_HPP
#define TATUM_ANALYZER_FACTORY_HPP
#include <memory>

#include "analyzer_factory_fwd.hpp"

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"

#include "tatum/graph_walkers.hpp"
#include "tatum/timing_analyzers.hpp"
#include "tatum/analyzers/full_timing_analyzers.hpp"

namespace tatum {

/** \file
 * This file defines the AnalyzerFactory class used to construct timing analyzers.
 *
 * We assume that the user has already defined the timing graph, constraints and
 * thier own delay calculator: 
 *
 *      TimingGraph timing_graph;
 *      TimingConstraints timing_constraints;
 *      MyDelayCalculator delay_calculator;
 *
 *      //... code that initializes the graph, constraints and delay calculator
 *      
 * We can now create the analyzer. For example, if we wanted to perform a setup
 * analysis:
 *
 *      auto setup_analyzer = AnalyzerFactory<SetupAnalysis>::make(timing_graph,
 *                                                                 timing_constraints,
 *                                                                 delay_calculator);
 *
 * We can similarily generate analyzers for other types of analysis, for instance Hold:
 *
 *      auto hold_analyzer = AnalyzerFactory<SetupAnalysis>::make(timing_graph,
 *                                                                timing_constraints,
 *                                                                delay_calculator);
 *
 * We can also build a parallel analyzer (instead of the serial default):
 *
 *      auto parallel_setup_analyzer = AnalyzerFactory<SetupAnalysis,ParallelWalker>::make(timing_graph,
 *                                                                                         timing_constraints,
 *                                                                                         delay_calculator);
 *
 * The AnalzyerFactory returns a std::unique_ptr to the appropriate TimingAnalyzer sub-class:
 *
 *      SetupAnalysis       =>  SetupTimingAnalyzer
 *      HoldAnalysis        =>  HoldTimingAnalyzer
 *      SetupHoldAnalysis   =>  SetupHoldTimingAnalyzer
 */

///Factor class to construct timing analyzers
///
///\tparam Visitor The analysis type visitor (e.g. SetupAnalysis)
///\tparam GraphWalker The graph walker to use (defaults to serial traversals)
template<class Visitor,
         class GraphWalker>
struct AnalyzerFactory {

    //We use the dependent_false template to detect if the un-specialized AnalyzerFactor 
    //template is being instantiated
    template<typename T>
    struct dependent_false : std::false_type {};

    //Error if the unspecialized template is instantiated
    static_assert(dependent_false<Visitor>::value,
                  "Must specify a specialized analysis visitor type (e.g. SetupAnalysis, HoldAnalysis, SetupHoldAnalysis)");

    //We provide the function definition to avoid spurious errors when the
    //unspecialized template is instantiated

    ///Builds a timing analyzer
    ///
    ///\param timing_graph The timing graph to associate with the analyzer
    ///\param timing_constraints The timing constraints to associate with the analyzer
    ///\param delay_calc The edge delay calculator to use. Note that this is a custom user defined type,
    ///                  but it must satisfy the the interface defined by DelayCalculator (\see DelayCalculator)
    ///
    ///\returns std::unique_ptr to the analyzer
    static std::unique_ptr<TimingAnalyzer> make(const TimingGraph& timing_graph,
                                                const TimingConstraints& timing_constraints,
                                                const DelayCalculator& delay_calc);
};

//Specialize for setup
template<class GraphWalker>
struct AnalyzerFactory<SetupAnalysis,GraphWalker> {

    static std::unique_ptr<SetupTimingAnalyzer> make(const TimingGraph& timing_graph,
                                                     const TimingConstraints& timing_constraints,
                                                     const DelayCalculator& delay_calc) {
        return std::unique_ptr<SetupTimingAnalyzer>(
                new detail::FullSetupTimingAnalyzer<GraphWalker>(timing_graph, 
                                                                 timing_constraints, 
                                                                 delay_calc)
                );
    }
};

//Specialize for hold
template<class GraphWalker>
struct AnalyzerFactory<HoldAnalysis,GraphWalker> {

    static std::unique_ptr<HoldTimingAnalyzer> make(const TimingGraph& timing_graph,
                                                    const TimingConstraints& timing_constraints,
                                                    const DelayCalculator& delay_calc) {
        return std::unique_ptr<HoldTimingAnalyzer>(
                new detail::FullHoldTimingAnalyzer<GraphWalker>(timing_graph,
                                                                timing_constraints, 
                                                                delay_calc)
                );
    }
};

//Specialize for combined setup and hold
template<class GraphWalker>
struct AnalyzerFactory<SetupHoldAnalysis,GraphWalker> {

    static std::unique_ptr<SetupHoldTimingAnalyzer> make(const TimingGraph& timing_graph,
                                                         const TimingConstraints& timing_constraints,
                                                         const DelayCalculator& delay_calc) {
        return std::unique_ptr<SetupHoldTimingAnalyzer>(
                new detail::FullSetupHoldTimingAnalyzer<GraphWalker>(timing_graph, 
                                                                     timing_constraints, 
                                                                     delay_calc)
                );
    }
};

} //namepsace

#endif
