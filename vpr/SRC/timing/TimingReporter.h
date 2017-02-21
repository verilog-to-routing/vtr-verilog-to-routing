#ifndef VPR_TIMING_REPORTER_H
#define VPR_TIMING_REPORTER_H
#include <iosfwd>
#include <vector>
#include <string>

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

enum class TimingPathType {
    SETUP,
    HOLD,
    UNKOWN
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
        TimingPathType type;
};

class TimingGraphNameResolver {
    public:
        virtual ~TimingGraphNameResolver() = default;

        virtual std::string node_name(tatum::NodeId node) const = 0;
        virtual std::string node_block_type_name(tatum::NodeId node) const = 0;
};

class VprTimingGraphNameResolver : public TimingGraphNameResolver {
    public:
        std::string node_name(tatum::NodeId node) const override;
        std::string node_block_type_name(tatum::NodeId node) const override;
};

class TimingReporter {
    public:
        TimingReporter(const TimingGraphNameResolver& name_resolver,
                       const tatum::TimingGraph& timing_graph, 
                       const tatum::TimingConstraints& timing_constraints, 
                       float unit_scale=1e-9,
                       size_t precision=3);
    public:
        void report_timing_setup(std::string filename, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=100) const;
        void report_timing_setup(std::ostream& os, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths=100) const;

        //TODO: implement report_timing_hold
    private:
        void report_timing(std::ostream& os, const std::vector<TimingPath>& paths, size_t npaths) const;
        void report_path(std::ostream& os, const TimingPath& path) const;
        void print_path_line(std::ostream& os, std::string point, tatum::Time incr, tatum::Time path) const;
        void print_path_line(std::ostream& os, std::string point, tatum::Time path) const;
        void print_path_line(std::ostream& os, std::string point, std::string incr, std::string path) const;

        std::vector<TimingPath> collect_worst_setup_paths(const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const;

        TimingPath trace_setup_path(const tatum::SetupTimingAnalyzer& setup_analyzer, const tatum::TimingTag& sink_tag, const tatum::NodeId sink_node) const;

        float convert_to_printable_units(float) const;

        std::string to_printable_string(tatum::Time val) const;

    private:
        const TimingGraphNameResolver& name_resolver_;
        const tatum::TimingGraph& timing_graph_;
        const tatum::TimingConstraints& timing_constraints_;
        float unit_scale_;
        size_t precision_;
};

#endif
