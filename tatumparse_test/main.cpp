#include <cassert>
#include "tatumparse.hpp"

namespace tp = tatumparse;

class PrintCallback : public tp::Callback {

    void start_parse() override { fprintf(stderr, "Starting parse\n"); }
    void filename(std::string fname) override { filename_ = fname; }
    void lineno(int line_num) override { line_no_ = line_num; }
    void finish_parse() override { fprintf(stderr, "Finished parse\n"); }

    void start_graph() override { fprintf(stdout, "timing_graph:\n"); }
    void add_node(int node_id, tp::NodeType type, std::vector<int> in_edge_ids, std::vector<int> out_edge_ids) override {
        fprintf(stdout, " node: %d", node_id);
        fprintf(stdout, "\n");

        fprintf(stdout, "  type:");
        switch(type) {
            case tp::NodeType::SOURCE: fprintf(stdout, " SOURCE"); break;
            case tp::NodeType::SINK: fprintf(stdout, " SINK"); break;
            case tp::NodeType::IPIN: fprintf(stdout, " IPIN"); break;
            case tp::NodeType::OPIN: fprintf(stdout, " OPIN"); break;
            default: assert(false);
        }
        fprintf(stdout, "\n");

        fprintf(stdout, "  in_edges: ");
        for(auto edge : in_edge_ids) {
            fprintf(stdout, "%d ", edge);
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "  out_edges: ");
        for(auto edge : out_edge_ids) {
            fprintf(stdout, "%d ", edge);
        }
        fprintf(stdout, "\n");
    }
    void add_edge(int edge_id, tp::EdgeType type, int src_node_id, int sink_node_id, bool disabled) override {
        fprintf(stdout, " edge: %d\n", edge_id);

        fprintf(stdout, "  type:");
        switch(type) {
            case tp::EdgeType::PRIMITIVE_COMBINATIONAL: fprintf(stdout, " PRIMITIVE_COMBINATIONAL"); break;
            case tp::EdgeType::PRIMITIVE_CLOCK_LAUNCH : fprintf(stdout, " PRIMITIVE_CLOCK_LAUNCH"); break;
            case tp::EdgeType::PRIMITIVE_CLOCK_CAPTURE: fprintf(stdout, " PRIMITIVE_CLOCK_CAPTURE"); break;
            case tp::EdgeType::INTERCONNECT           : fprintf(stdout, " INTERCONNECT"); break;
            default: assert(false);
        }
        fprintf(stdout, "\n");

        fprintf(stdout, "  src_node: %d\n", src_node_id);
        fprintf(stdout, "  sink_node: %d\n", sink_node_id);
        if(disabled) {
            fprintf(stdout, "  disabled: true\n");
        }
    }
    void finish_graph() override { fprintf(stdout, "# end timing_graph\n"); }

    void start_constraints() override { fprintf(stdout, "timing_constraints:\n"); }
    void add_clock_domain(int domain_id, std::string name) override {
        fprintf(stdout, " type: CLOCK domain: %d name: \"%s\"\n", domain_id, name.c_str());
    }
    void add_clock_source(int node_id, int domain_id) override {
        fprintf(stdout, " type: CLOCK_SOURCE node: %d domain: %d\n", node_id, domain_id);
    }
    void add_constant_generator(int node_id) override {
        fprintf(stdout, " type: CONSTANT_GENERATOR node: %d\n", node_id);
    }
    void add_max_input_constraint(int node_id, int domain_id, float constraint) override {
        fprintf(stdout, " type: MAX_INPUT_CONSTRAINT node: %d domain: %d constraint: %g\n", node_id, domain_id, constraint);
    }
    void add_min_input_constraint(int node_id, int domain_id, float constraint) override {
        fprintf(stdout, " type: MIN_INPUT_CONSTRAINT node: %d domain: %d constraint: %g\n", node_id, domain_id, constraint);
    }
    void add_max_output_constraint(int node_id, int domain_id,  float constraint) override {
        fprintf(stdout, " type: MAX_OUTPUT_CONSTRAINT node: %d domain: %d constraint: %g\n", node_id, domain_id, constraint);
    }
    void add_min_output_constraint(int node_id, int domain_id,  float constraint) override {
        fprintf(stdout, " type: MIN_OUTPUT_CONSTRAINT node: %d domain: %d constraint: %g\n", node_id, domain_id, constraint);
    }
    void add_setup_constraint(int src_domain_id, int sink_domain_id, float constraint) override {
        fprintf(stdout, " type: SETUP_CONSTRAINT src_domain: %d sink_domain: %d constraint: %g\n", src_domain_id, sink_domain_id, constraint);
    }
    void add_hold_constraint(int src_domain_id, int sink_domain_id, float constraint) override {
        fprintf(stdout, " type: HOLD_CONSTRAINT src_domain: %d sink_domain: %d constraint: %g\n", src_domain_id, sink_domain_id, constraint);
    }
    void add_setup_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) override {
        fprintf(stdout, " type: SETUP_UNCERTAINTY src_domain: %d sink_domain: %d uncertainty: %g\n", src_domain_id, sink_domain_id, uncertainty);
    }
    void add_hold_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) override {
        fprintf(stdout, " type: HOLD_UNCERTAINTY src_domain: %d sink_domain: %d uncertainty: %g\n", src_domain_id, sink_domain_id, uncertainty);
    }
    void add_early_source_latency(int domain_id, float latency) override {
        fprintf(stdout, " type: EARLY_SOURCE_LATENCY domain: %d latency: %g\n", domain_id, latency);
    }
    void add_late_source_latency(int domain_id, float latency) override {
        fprintf(stdout, " type: LATE_SOURCE_LATENCY domain: %d latency: %g\n", domain_id, latency);
    }
    void finish_constraints() override { fprintf(stdout, "# end timing_constraints\n"); }

    void start_delay_model() override { fprintf(stdout, "delay_model:\n"); }
    void add_edge_delay(int edge_id, float min_delay, float max_delay) override { 
        fprintf(stdout, " edge_id: %d min_delay: %g max_delay: %g\n", edge_id, min_delay, max_delay); 
    }
    void add_edge_setup_hold_time(int edge_id, float setup_time, float hold_time) override {
        fprintf(stdout, " edge_id: %d setup_time: %g hold_time: %g\n", edge_id, setup_time, hold_time); 
    }
    void finish_delay_model() override { fprintf(stdout, "# end delay_model\n"); }

    void start_results() override { fprintf(stdout, "analysis_result:\n"); }
    void add_node_tag(tp::TagType type, int node_id, int launch_domain_id, int capture_domain_id, float time) override {
        fprintf(stdout, " type: ");
        switch(type) {
            case tp::TagType::SETUP_LAUNCH_CLOCK: fprintf(stdout, "SETUP_LAUNCH_CLOCK"); break;
            case tp::TagType::SETUP_CAPTURE_CLOCK: fprintf(stdout, "SETUP_CAPTURE_CLOCK"); break;
            case tp::TagType::HOLD_LAUNCH_CLOCK: fprintf(stdout, "HOLD_LAUNCH_CLOCK"); break;
            case tp::TagType::HOLD_CAPTURE_CLOCK: fprintf(stdout, "HOLD_CAPTURE_CLOCK"); break;
            default: assert(false);
        }
        fprintf(stdout, " node: %d launch_domain: %d capture_domain: %d time: %g\n", node_id, launch_domain_id, capture_domain_id, time);
    }
    void add_edge_slack(tp::TagType type, int edge_id, int launch_domain_id, int capture_domain_id, float slack) override {
        fprintf(stdout, " type: ");
        switch(type) {
            case tp::TagType::SETUP_SLACK: fprintf(stdout, "SETUP_SLACK"); break;
            case tp::TagType::HOLD_SLACK: fprintf(stdout, "HOLD_SLACK"); break;
            default: assert(false);
        }
        fprintf(stdout, " edge: %d launch_domain: %d capture_domain: %d time: %g\n", edge_id, launch_domain_id, capture_domain_id, slack);
    }
    void add_node_slack(tp::TagType type, int node_id, int launch_domain_id, int capture_domain_id, float slack) override {
        fprintf(stdout, " type: ");
        switch(type) {
            case tp::TagType::SETUP_SLACK: fprintf(stdout, "SETUP_SLACK"); break;
            case tp::TagType::HOLD_SLACK: fprintf(stdout, "HOLD_SLACK"); break;
            default: assert(false);
        }
        fprintf(stdout, " node: %d launch_domain: %d capture_domain: %d time: %g\n", node_id, launch_domain_id, capture_domain_id, slack);
    }
    void finish_results() override { fprintf(stdout, "# end analysis_results\n"); }

    void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override {
        fprintf(stderr, "%s:%d: Parse error: %s", filename_.c_str(), curr_lineno, msg.c_str());
        if(!near_text.empty()) {
            fprintf(stderr, " near '%s'", near_text.c_str());
            fprintf(stderr, "\n");
        } else {
            if(msg.find('\n') == std::string::npos) {
                fprintf(stderr, "\n");
            }
        }
    }

    private:
        std::string filename_;
        int line_no_;
};

class NopCallback : public tp::Callback {
    public:
        //Start of parsing
        void start_parse() override {}

        //Sets current filename
        void filename(std::string fname) override { filename_ = fname; }

        //Sets current line number
        void lineno(int /*line_num*/) override {}

        void start_graph() override {}
        void add_node(int /*node_id*/, tp::NodeType /*type*/, std::vector<int> /*in_edge_ids*/, std::vector<int> /*out_edge_ids*/) override {}
        void add_edge(int /*edge_id*/, tp::EdgeType /*type*/, int /*src_node_id*/, int /*sink_node_id*/, bool /*disabled*/) override {}
        void finish_graph() override {}

        void start_constraints() override {}
        void add_clock_domain(int /*domain_id*/, std::string /*name*/) override {}
        void add_clock_source(int /*node_id*/, int /*domain_id*/) override {}
        void add_constant_generator(int /*node_id*/) override {}
        void add_max_input_constraint(int /*node_id*/, int /*domain_id*/, float /*constraint*/) override {}
        void add_min_input_constraint(int /*node_id*/, int /*domain_id*/, float /*constraint*/) override {}
        void add_max_output_constraint(int /*node_id*/, int /*domain_id*/, float /*constraint*/) override {}
        void add_min_output_constraint(int /*node_id*/, int /*domain_id*/, float /*constraint*/) override {}
        void add_setup_constraint(int /*src_domain_id*/, int /*sink_domain_id*/, float /*constraint*/) override {}
        void add_hold_constraint(int /*src_domain_id*/, int /*sink_domain_id*/, float /*constraint*/) override {}
        void add_setup_uncertainty(int /*src_domain_id*/, int /*sink_domain_id*/, float /*uncertainty*/) override {}
        void add_hold_uncertainty(int /*src_domain_id*/, int /*sink_domain_id*/, float /*uncertainty*/) override {}
        void add_early_source_latency(int /*domain_id*/, float /*latency*/) override {}
        void add_late_source_latency(int /*domain_id*/, float /*latency*/) override {}
        void finish_constraints() override {}

        void start_delay_model() override {}
        void add_edge_delay(int /*edge_id*/, float /*min_delay*/, float /*max_delay*/) override {}
        void add_edge_setup_hold_time(int /*edge_id*/, float /*setup_time*/, float /*hold_time*/) override {}
        void finish_delay_model() override {}

        void start_results() override {}
        void add_node_tag(tp::TagType /*type*/, int /*node_id*/, int /*launch_domain_id*/, int /*capture_domain_id*/, float /*time*/) override {}
        void add_edge_slack(tp::TagType /*type*/, int /*edge_id*/, int /*launch_domain_id*/, int /*capture_domain_id*/, float /*slack*/) override {}
        void add_node_slack(tp::TagType /*type*/, int /*node_id*/, int /*launch_domain_id*/, int /*capture_domain_id*/, float /*slack*/) override {}
        void finish_results() override {}

        //End of parsing
        void finish_parse() override {}

        //Error during parsing
        void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override {
            fprintf(stderr, "%s:%d: Parse error: %s", filename_.c_str(), curr_lineno, msg.c_str());
            if(!near_text.empty()) {
                fprintf(stderr, " near '%s'", near_text.c_str());
                fprintf(stderr, "\n");
            } else {
                if(msg.find('\n') == std::string::npos) {
                    fprintf(stderr, "\n");
                }
            }
        }
    private:
        std::string filename_;
};

int main(int argc, char** argv) {

    //PrintCallback cb;
    NopCallback cb;
    if(argc == 2) {
        if(std::string(argv[1]) == "-") {
            tp::tatum_parse_file(stdin, cb);  
        } else {
            tp::tatum_parse_filename(argv[1], cb);  
        }
    } else {
        fprintf(stderr, "Invalid command line\n");
        return 1;
    }
}
