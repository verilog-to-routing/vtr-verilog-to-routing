#ifndef GRAPHVIZ_DOT_WRITER_HPP
#define GRAPHVIZ_DOT_WRITER_HPP

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/timing_analyzers.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/report/TimingPathFwd.hpp"
#include "tatum/base/TimingType.hpp"
#include <vector>
#include <fstream>
#include <map>
#include <set>

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

class GraphvizDotWriter {
    public:
        GraphvizDotWriter(const TimingGraph& tg, const DelayCalculator& delay_calc);

        //Specify a subset of nodes to dump
        template<class Container>
        void set_nodes_to_dump(const Container& nodes) {
            nodes_to_dump_ = std::set<NodeId>(nodes.begin(), nodes.end());
        }

        //Write the dot file with no timing tags
        void write_dot_file(std::string filename);
        void write_dot_file(std::ostream& os);

        //Write the dot file with timing tags
        void write_dot_file(std::string filename, const SetupTimingAnalyzer& analyzer);
        void write_dot_file(std::ostream& os, const SetupTimingAnalyzer& analyzer);

        void write_dot_file(std::string filename, const HoldTimingAnalyzer& analyzer);
        void write_dot_file(std::ostream& os, const HoldTimingAnalyzer& analyzer);
    private:
        void write_dot_format(std::ostream& os, 
                              const std::map<NodeId,std::vector<TimingTag>>& node_tags,
                              const std::map<NodeId,std::vector<TimingTag>>& node_slacks,
                              const TimingType timing_type);
        void write_dot_node(std::ostream& os, 
                            const NodeId node, 
                            const std::vector<TimingTag>& tags, 
                            const std::vector<TimingTag>& slacks);
        void write_dot_level(std::ostream& os, 
                             const LevelId level);
        void write_dot_edge(std::ostream& os, 
                            const EdgeId edge,
                            const TimingType timing_type);
        void tag_domain_from_to(std::ostream& os, const TimingTag& tag);

        std::string node_name(NodeId node);
    private:
        constexpr static size_t MAX_DOT_GRAPH_NODES = 1000; //Graphviz can't handle large numbers of nodes
        constexpr static const char* CLOCK_CAPTURE_EDGE_COLOR = "#c45403"; //Orange-red
        constexpr static const char* CLOCK_LAUNCH_EDGE_COLOR = "#10c403"; //Green    
        constexpr static const char* DISABLED_EDGE_COLOR = "#aaaaaa"; //Grey

        const TimingGraph& tg_;
        std::set<NodeId> nodes_to_dump_;
        const DelayCalculator& delay_calc_;
};

GraphvizDotWriter make_graphviz_dot_writer(const TimingGraph& tg, const DelayCalculator& delay_calc);

} //namespace

#endif
