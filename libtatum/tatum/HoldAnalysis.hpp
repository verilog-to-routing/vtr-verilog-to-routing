#pragma once
#include <memory>
#include <vector>

#include "tatum/tags/TimingTags.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"

#include "tatum/graph_visitors/CommonAnalysisVisitor.hpp"
#include "tatum/graph_visitors/HoldAnalysisOps.hpp"

#include "tatum/util/tatum_assert.hpp"


namespace tatum {

/** \file
 * The 'HoldAnalysis' class defines the operations needed by a timing analyzer class
 * to perform a hold (min/shortest path) analysis.
 *
 * \see SetupAnalysis
 * \see TimingAnalyzer
 *
 * Hold Analysis Principles
 * ==========================
 *
 * In addition to satisfying setup constraints, data arriving at a Flip-Flop (FF) must stay (i.e.
 * remain stable) for some amount of time *after* the capturing clock edge arrives. This time is
 * referred to as the 'Hold Time' of the Flip-Flop, \f$ t_h \f$.  If the data changes during the 
 * hold window (i.e. less than \f$ t_h \f$ after the capturing clock edge) then the FF may go 
 * meta-stable failing to capture the data. This will put the circuit in an invalid state (this 
 * is bad).
 *
 * More formally, for correct operation at every cycle we require the following to be satisfied
 * for every path in the circuit:
 *
 * \f[
 *      t_{clk\_insrt}^{(launch)} + t_{cq}^{(min)} + t_{comb}^{(min)} \geq t_{clk\_insrt}^{(capture)} + t_h   (1)
 * \f]
 *
 * where \f$ t_{clk\_insrt}^{(launch)}, t_{clk\_insrt}^{(capture)} \f$ are the up/downstream FF clock insertion
 * delays,  \f$ t_{cq}^{(min)} \f$ is the minimum clock-to-q delay of the upstream FF, \f$ t_{comb}^{(min)} \f$ is
 * the minimum combinational path delay from the upstream to downstream FFs, and \f$ t_h \f$ is the hold
 * constraint of the downstream FF.
 *
 * Note that unlike in setup analysis this behaviour is indepenant of clock period.
 * Intuitively, hold analysis can be viewed as data from the upstream FF trampling the data launched
 * on the previous cycle before it can be captured by the downstream FF.
 */

/** \class HoldAnalysis
 *
 * The 'HoldAnalysis' class defines the operations needed by a timing analyzer
 * to perform a hold (min/shortest path) analysis.
 *
 * \see SetupAnalysis
 * \see TimingAnalyzer
 * \see CommonAnalysisVisitor
 */
class HoldAnalysis : public detail::CommonAnalysisVisitor<detail::HoldAnalysisOps> {

    public:
        HoldAnalysis(size_t num_tags, size_t num_slacks)
            : detail::CommonAnalysisVisitor<detail::HoldAnalysisOps>(num_tags, num_slacks) {}

        TimingTags::tag_range hold_tags(const NodeId node) const { return ops_.get_tags(node); }
        TimingTags::tag_range hold_tags(const NodeId node, TagType type) const { return ops_.get_tags(node, type); }
        TimingTags::tag_range hold_edge_slacks(const EdgeId edge) const { return ops_.get_edge_slacks(edge); }
        TimingTags::tag_range hold_node_slacks(const NodeId node) const { return ops_.get_node_slacks(node); }
};

} //namepsace
