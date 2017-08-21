#ifndef TATUMSTA_ECHO_LOADER
#define TATUMSTA_ECHO_LOADER
#include <memory>

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/delay_calc/FixedDelayCalculator.hpp"
#include "golden_reference.hpp"

#include "tatumparse.hpp"

class EchoLoader : public tatumparse::Callback {
public:
    EchoLoader();

    //Start of parsing
    void start_parse() override { }

    //Sets current filename
    void filename(std::string fname) override { filename_ = fname; }

    //Sets current line number
    void lineno(int /*line_num*/) override { }

    void start_graph() override { }
    void add_node(int node_id, tatumparse::NodeType type, std::vector<int> in_edge_ids, std::vector<int> out_edge_ids) override;
    void add_edge(int edge_id, tatumparse::EdgeType type, int src_node_id, int sink_node_id, bool disabled) override;
    void finish_graph() override;

    void start_constraints() override { }
    void add_clock_domain(int domain_id, std::string name) override;
    void add_clock_source(int node_id, int domain_id) override;
    void add_constant_generator(int node_id) override;
    void add_max_input_constraint(int node_id, int domain_id, float constraint) override;
    void add_min_input_constraint(int node_id, int domain_id, float constraint) override;
    void add_min_output_constraint(int node_id, int domain_id, float constraint) override;
    void add_max_output_constraint(int node_id, int domain_id, float constraint) override;
    void add_setup_constraint(int src_domain_id, int sink_domain_id, float constraint) override;
    void add_hold_constraint(int src_domain_id, int sink_domain_id, float constraint) override;
    void add_setup_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) override;
    void add_hold_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) override;
    void add_early_source_latency(int domain_id, float latency) override;
    void add_late_source_latency(int domain_id, float latency) override;
    void finish_constraints() override { }

    void start_delay_model() override { }
    void add_edge_delay(int edge_id, float min_delay, float max_delay) override;
    void add_edge_setup_hold_time(int edge_id, float setup_time, float hold_time) override;
    void finish_delay_model() override { }

    void start_results() override { }
    void add_node_tag(tatumparse::TagType type, int node_id, int launch_domain_id, int capture_domain_id, float time) override;
    void add_edge_slack(tatumparse::TagType type, int edge_id, int launch_domain_id, int capture_domain_id, float slack) override;
    void add_node_slack(tatumparse::TagType type, int node_id, int launch_domain_id, int capture_domain_id, float slack) override;
    void finish_results() override { }

    //End of parsing
    void finish_parse() override { }

    //Error during parsing
    void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override;
public:
    std::unique_ptr<tatum::TimingGraph> timing_graph();
    std::unique_ptr<tatum::TimingConstraints> timing_constraints();
    std::unique_ptr<tatum::FixedDelayCalculator> delay_calculator();
    std::unique_ptr<GoldenReference> golden_reference();

private:
    tatum::NodeType to_tatum_node_type(tatumparse::NodeType type);
    tatum::EdgeType to_tatum_edge_type(tatumparse::EdgeType type);

private: //Data

    std::unique_ptr<tatum::TimingGraph> tg_;
    std::unique_ptr<tatum::TimingConstraints> tc_;
    std::unique_ptr<GoldenReference> gr_;

    std::string filename_;

    tatum::util::linear_map<tatum::EdgeId,tatum::Time> max_delay_edges_;
    tatum::util::linear_map<tatum::EdgeId,tatum::Time> min_delay_edges_;
    tatum::util::linear_map<tatum::EdgeId,tatum::Time> setup_times_;
    tatum::util::linear_map<tatum::EdgeId,tatum::Time> hold_times_;
};


#endif
