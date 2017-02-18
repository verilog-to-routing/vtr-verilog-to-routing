#ifndef VPR_TIMING_REPORTER_H
#define VPR_TIMING_REPORTER_H
#include <iosfwd>
#include <vector>

#include "vtr_assert.h"

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "timing_analyzers.hpp"

#include "atom_netlist_fwd.h"

class TimingPathElem {
    public:
        TimingPathElem(tatum::TimingTag tag_v,
                       tatum::NodeId node_v,
                       tatum::EdgeId incomming_edge_v)
            : tag(tag_v)
            , node(node_v)
            , incomming_edge(incomming_edge_v) {
            //pass
        }

        tatum::TimingTag tag;
        tatum::NodeId node;
        tatum::EdgeId incomming_edge;
};

class TimingPath {
    public:
        tatum::DomainId launch_domain;
        tatum::DomainId capture_domain;

        tatum::NodeId endpoint() const { 
            VTR_ASSERT(data_launch.size() > 0);
            return data_launch[data_launch.size()-1].node;
        }
        tatum::NodeId startpoint() const { 
            VTR_ASSERT(data_launch.size() > 0);
            return data_launch[0].node; 
        }

        std::vector<TimingPathElem> clock_launch;
        std::vector<TimingPathElem> data_launch;
        std::vector<TimingPathElem> clock_capture;
        tatum::TimingTag slack_tag;
};


class TimingReporter {
    public:
        TimingReporter(const AtomNetlist& netist,
                       const AtomLookup& netlist_lookup,
                       std::shared_ptr<const tatum::TimingGraph> timing_graph, 
                       std::shared_ptr<const tatum::TimingConstraints> timing_constraints, 
                       std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer,
                       float unit_scale=1e-9,
                       size_t precision=3);
    public:

        void report_timing(std::string filename, size_t npaths=10000) const;

        void report_timing(std::ostream& os, size_t npaths=10) const;

    private:
        void report_path(std::ostream& os, const TimingPath& path) const;
        void print_path_line(std::ostream& os, std::string point, tatum::Time incr, tatum::Time path) const;
        void print_path_line(std::ostream& os, std::string point, tatum::Time path) const;
        void print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const;

        std::vector<TimingPath> collect_worst_paths(size_t npaths) const;

        TimingPath trace_path(const tatum::TimingTag& sink_tag, const tatum::NodeId sink_node) const;

        float convert_to_printable_units(float) const;

        std::string to_printable_string(tatum::Time val) const;

    private:
        const AtomNetlist& netlist_;
        const AtomLookup& netlist_lookup_;
        std::shared_ptr<const tatum::TimingGraph> timing_graph_;
        std::shared_ptr<const tatum::TimingConstraints> timing_constraints_;
        std::shared_ptr<const tatum::SetupTimingAnalyzer> setup_analyzer_;
        float unit_scale_;
        size_t precision_;
};

#endif
