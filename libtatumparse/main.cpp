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
    void add_edge(int edge_id, int src_node_id, int sink_node_id) override {
        fprintf(stdout, " edge: %d\n", edge_id);
        fprintf(stdout, "  src_node: %d\n", src_node_id);
        fprintf(stdout, "  sink_node: %d\n", sink_node_id);
    }
    void finish_graph() override { fprintf(stdout, "# end timing_graph\n"); }

    void start_constraints() override { fprintf(stdout, "timing_constraints:\n"); }
    void add_clock_domain(int domain_id, std::string name) override {
        fprintf(stdout, " type: CLOCK domain: %d name: \"%s\"\n", domain_id, name.c_str());
    }
    void add_clock_source(int node_id, int domain_id) override {
        fprintf(stdout, " type: CLOCK_SOURCE node: %d domain: %d\n", node_id, domain_id);
    }
    void add_input_constraint(int node_id, float constraint) override {
        fprintf(stdout, " type: INPUT_CONSTRAINT node: %d constraint: %g\n", node_id, constraint);
    }
    void add_output_constraint(int node_id, float constraint) override {
        fprintf(stdout, " type: OUTPUT_CONSTRAINT node: %d constraint: %g\n", node_id, constraint);
    }
    void add_setup_constraint(int src_domain_id, int sink_domain_id, float constraint) override {
        fprintf(stdout, " type: SETUP_CONSTRAINT src_domain: %d sink_domain: %d constraint: %g\n", src_domain_id, sink_domain_id, constraint);
    }
    void add_hold_constraint(int src_domain_id, int sink_domain_id, float constraint) override {
        fprintf(stdout, " type: HOLD_CONSTRAINT src_domain: %d sink_domain: %d constraint: %g\n", src_domain_id, sink_domain_id, constraint);
    }
    void finish_constraints() override { fprintf(stdout, "# end timing_constraints\n"); }

    void start_delay_model() override { fprintf(stdout, "delay_model:\n"); }
    void add_edge_delay(int edge_id, float delay) override { fprintf(stdout, " edge_id: %d delay: %g", edge_id, delay); }
    void finish_delay_model() override { fprintf(stdout, "# end delay_model\n"); }

    void start_results() override { fprintf(stdout, "analysis_result:\n"); }
    void add_tag(tp::TagType type, int node_id, int domain_id, float arr, float req) override {
        fprintf(stdout, " type: ");
        switch(type) {
            case tp::TagType::SETUP_DATA: fprintf(stdout, "SETUP_DATA"); break;
            case tp::TagType::SETUP_CLOCK: fprintf(stdout, "SETUP_CLOCK"); break;
            case tp::TagType::HOLD_DATA: fprintf(stdout, "HOLD_DATA"); break;
            case tp::TagType::HOLD_CLOCK: fprintf(stdout, "HOLD_CLOCK"); break;
            default: assert(false);
        }
        fprintf(stdout, " node: %d domain: %d arr: %g req: %g\n", node_id, domain_id, arr, req);
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

int main(int argc, char** argv) {

    PrintCallback cb;
    if(argc == 2) {
        tp::tatum_parse_filename(argv[1], cb);  
    } else {
        fprintf(stderr, "Invalid command line\n");
        return 1;
    }
}
