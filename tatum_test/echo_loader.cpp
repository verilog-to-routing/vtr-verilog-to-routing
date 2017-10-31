#include "echo_loader.hpp"

EchoLoader::EchoLoader() {
    tg_ = std::unique_ptr<tatum::TimingGraph>(new tatum::TimingGraph);
    tc_ = std::unique_ptr<tatum::TimingConstraints>(new tatum::TimingConstraints);
    gr_ = std::unique_ptr<GoldenReference>(new GoldenReference);
}

void EchoLoader::add_node(int node_id, tatumparse::NodeType type, std::vector<int> /*in_edge_ids*/, std::vector<int> /*out_edge_ids*/) {
    tatum::NodeId created_node_id = tg_->add_node(to_tatum_node_type(type));
    TATUM_ASSERT(created_node_id == tatum::NodeId(node_id));
}

void EchoLoader::add_edge(int edge_id, tatumparse::EdgeType type, int src_node_id, int sink_node_id, bool disabled) {
    tatum::EdgeId created_edge_id = tg_->add_edge(to_tatum_edge_type(type), tatum::NodeId(src_node_id), tatum::NodeId(sink_node_id));
    TATUM_ASSERT(created_edge_id == tatum::EdgeId(edge_id));

    tg_->disable_edge(created_edge_id, disabled);
}

void EchoLoader::finish_graph() { 
    max_delay_edges_.resize(tg_->edges().size()); 
    min_delay_edges_.resize(tg_->edges().size()); 
    setup_times_.resize(tg_->edges().size()); 
    hold_times_.resize(tg_->edges().size()); 
}

void EchoLoader::add_clock_domain(int domain_id, std::string name) {
    tatum::DomainId created_domain_id = tc_->create_clock_domain(name);
    TATUM_ASSERT(created_domain_id == tatum::DomainId(domain_id));
}

void EchoLoader::add_clock_source(int node_id, int domain_id) {
    tc_->set_clock_domain_source(tatum::NodeId(node_id), tatum::DomainId(domain_id));
}

void EchoLoader::add_constant_generator(int node_id) {
    tc_->set_constant_generator(tatum::NodeId(node_id));
}

void EchoLoader::add_max_input_constraint(int node_id, int domain_id, float constraint) {
    tc_->set_input_constraint(tatum::NodeId(node_id), tatum::DomainId(domain_id), tatum::DelayType::MAX, tatum::Time(constraint));
}

void EchoLoader::add_min_input_constraint(int node_id, int domain_id, float constraint) {
    tc_->set_input_constraint(tatum::NodeId(node_id), tatum::DomainId(domain_id), tatum::DelayType::MIN, tatum::Time(constraint));
}

void EchoLoader::add_max_output_constraint(int node_id, int domain_id, float constraint) {
    tc_->set_output_constraint(tatum::NodeId(node_id), tatum::DomainId(domain_id), tatum::DelayType::MAX, tatum::Time(constraint));
}

void EchoLoader::add_min_output_constraint(int node_id, int domain_id, float constraint) {
    tc_->set_output_constraint(tatum::NodeId(node_id), tatum::DomainId(domain_id), tatum::DelayType::MIN, tatum::Time(constraint));
}

void EchoLoader::add_setup_constraint(int src_domain_id, int sink_domain_id, float constraint) {
    tc_->set_setup_constraint(tatum::DomainId(src_domain_id), tatum::DomainId(sink_domain_id), tatum::Time(constraint));
}

void EchoLoader::add_hold_constraint(int src_domain_id, int sink_domain_id, float constraint) {
    tc_->set_hold_constraint(tatum::DomainId(src_domain_id), tatum::DomainId(sink_domain_id), tatum::Time(constraint));
}

void EchoLoader::add_setup_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) {
    tc_->set_setup_clock_uncertainty(tatum::DomainId(src_domain_id), tatum::DomainId(sink_domain_id), tatum::Time(uncertainty));
}

void EchoLoader::add_hold_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) {
    tc_->set_hold_clock_uncertainty(tatum::DomainId(src_domain_id), tatum::DomainId(sink_domain_id), tatum::Time(uncertainty));
}

void EchoLoader::add_early_source_latency(int domain_id, float latency) {
    tc_->set_source_latency(tatum::DomainId(domain_id), tatum::ArrivalType::EARLY, tatum::Time(latency));
}

void EchoLoader::add_late_source_latency(int domain_id, float latency) {
    tc_->set_source_latency(tatum::DomainId(domain_id), tatum::ArrivalType::LATE, tatum::Time(latency));
}

void EchoLoader::add_edge_setup_hold_time(int edge_id, float setup_time, float hold_time) {
    setup_times_[tatum::EdgeId(edge_id)] = tatum::Time(setup_time);
    hold_times_[tatum::EdgeId(edge_id)] = tatum::Time(hold_time);
}

void EchoLoader::add_edge_delay(int edge_id, float min_delay, float max_delay) {
    min_delay_edges_[tatum::EdgeId(edge_id)] = tatum::Time(min_delay);
    max_delay_edges_[tatum::EdgeId(edge_id)] = tatum::Time(max_delay);
}

void EchoLoader::add_node_tag(tatumparse::TagType type, int node_id, int launch_domain_id, int capture_domain_id, float time) {
    gr_->set_result(tatum::NodeId(node_id), type, tatum::DomainId(launch_domain_id), tatum::DomainId(capture_domain_id), time);
}

void EchoLoader::add_edge_slack(tatumparse::TagType /*type*/, int /*edge_id*/, int /*launch_domain_id*/, int /*capture_domain_id*/, float /*slack*/) {
    //nop
}
void EchoLoader::add_node_slack(tatumparse::TagType /*type*/, int /*node_id*/, int /*launch_domain_id*/, int /*capture_domain_id*/, float /*slack*/) {
    //nop
}

std::unique_ptr<tatum::TimingGraph> EchoLoader::timing_graph() {
    return std::move(tg_);
}

std::unique_ptr<tatum::TimingConstraints> EchoLoader::timing_constraints() {
    return std::move(tc_);
}

std::unique_ptr<tatum::FixedDelayCalculator> EchoLoader::delay_calculator() {
    return std::unique_ptr<tatum::FixedDelayCalculator>(new tatum::FixedDelayCalculator(max_delay_edges_, setup_times_, min_delay_edges_, hold_times_));
}

std::unique_ptr<GoldenReference> EchoLoader::golden_reference() {
    return std::move(gr_);
}

tatum::NodeType EchoLoader::to_tatum_node_type(tatumparse::NodeType type) {
    if(type == tatumparse::NodeType::SOURCE) {
        return tatum::NodeType::SOURCE;
    } else if(type == tatumparse::NodeType::SINK) {
        return tatum::NodeType::SINK;
    } else if(type == tatumparse::NodeType::IPIN) {
        return tatum::NodeType::IPIN;
    } else if(type == tatumparse::NodeType::CPIN) {
        return tatum::NodeType::CPIN;
    } else {
        TATUM_ASSERT(type == tatumparse::NodeType::OPIN);
        return tatum::NodeType::OPIN;
    }
}

tatum::EdgeType EchoLoader::to_tatum_edge_type(tatumparse::EdgeType type) {
    if(type == tatumparse::EdgeType::PRIMITIVE_COMBINATIONAL) {
        return tatum::EdgeType::PRIMITIVE_COMBINATIONAL;
    } else if(type == tatumparse::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
        return tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH;
    } else if(type == tatumparse::EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
        return tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE;
    } else {
        TATUM_ASSERT(type == tatumparse::EdgeType::INTERCONNECT);
        return tatum::EdgeType::INTERCONNECT;
    }
}

void EchoLoader::parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) {
    fprintf(stderr, "%s:%d Failed to parse echo file: %s near '%s'\n", filename_.c_str(), curr_lineno, msg.c_str(), near_text.c_str());
    std::exit(1);
}
