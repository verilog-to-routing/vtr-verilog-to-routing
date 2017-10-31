#ifndef TATUMPARSE_H
#define TATUMPARSE_H
/*
 * libtatumparse - Kevin E. Murray 2016
 *
 * Released under MIT License see LICENSE.txt for details.
 *
 * OVERVIEW
 * --------------------------
 * This library provides support for parsing Berkely Logic Interchange Format (TATUM)
 * files. It supporst the features required to handle basic netlists (e.g. .model, 
 * .inputs, .outputs, .subckt, .names, .latch)
 *
 * USAGE
 * --------------------------
 * Define a callback derived from the tatumparse::Callback interface, and pass it
 * to one of the tatumparse::tatum_parse_*() functions.
 *
 * The parser will then call the various callback methods as it encouters the 
 * appropriate parts of the netlist.
 *
 * See main.cpp and tatum_pretty_print.hpp for example usage.
 *
 */
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <functional>

namespace tatumparse {
/*
 * Data structure Forward declarations
 */
enum class NodeType {
    SOURCE,
    SINK,
    IPIN,
    OPIN,
    CPIN
};

enum class EdgeType {
    PRIMITIVE_COMBINATIONAL,
    PRIMITIVE_CLOCK_LAUNCH,
    PRIMITIVE_CLOCK_CAPTURE,
    INTERCONNECT
};

enum class TagType {
    SETUP_DATA_ARRIVAL,
    SETUP_DATA_REQUIRED,
    SETUP_LAUNCH_CLOCK,
    SETUP_CAPTURE_CLOCK,
    SETUP_SLACK,
    HOLD_DATA_ARRIVAL,
    HOLD_DATA_REQUIRED,
    HOLD_LAUNCH_CLOCK,
    HOLD_CAPTURE_CLOCK,
    HOLD_SLACK
};

/*
 * Callback object
 */
class Callback {
    public:
        virtual ~Callback() {};

        //Start of parsing
        virtual void start_parse() = 0;

        //Sets current filename
        virtual void filename(std::string fname) = 0;

        //Sets current line number
        virtual void lineno(int line_num) = 0;

        virtual void start_graph() = 0;
        virtual void add_node(int node_id, NodeType type, std::vector<int> in_edge_ids, std::vector<int> out_edge_ids) = 0;
        virtual void add_edge(int edge_id, EdgeType type, int src_node_id, int sink_node_id, bool disabled=false) = 0;
        virtual void finish_graph() = 0;

        virtual void start_constraints() = 0;
        virtual void add_clock_domain(int domain_id, std::string name) = 0;
        virtual void add_clock_source(int node_id, int domain_id) = 0;
        virtual void add_constant_generator(int node_id) = 0;
        virtual void add_max_input_constraint(int node_id, int domain_id, float constraint) = 0;
        virtual void add_min_input_constraint(int node_id, int domain_id, float constraint) = 0;
        virtual void add_max_output_constraint(int node_id, int domain_id, float constraint) = 0;
        virtual void add_min_output_constraint(int node_id, int domain_id, float constraint) = 0;
        virtual void add_setup_constraint(int src_domain_id, int sink_domain_id, float constraint) = 0;
        virtual void add_hold_constraint(int src_domain_id, int sink_domain_id, float constraint) = 0;
        virtual void add_setup_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) = 0;
        virtual void add_hold_uncertainty(int src_domain_id, int sink_domain_id, float uncertainty) = 0;
        virtual void add_early_source_latency(int domain_id, float latency) = 0;
        virtual void add_late_source_latency(int domain_id, float latency) = 0;
        virtual void finish_constraints() = 0;

        virtual void start_delay_model() = 0;
        virtual void add_edge_delay(int edge_id, float min_delay, float max_delay) = 0;
        virtual void add_edge_setup_hold_time(int edge_id, float min_delay, float max_delay) = 0;
        virtual void finish_delay_model() = 0;

        virtual void start_results() = 0;
        virtual void add_node_tag(TagType type, int node_id, int launch_domain_id, int capture_domain_id, float time) = 0;
        virtual void add_edge_slack(TagType type, int edge_id, int launch_domain_id, int capture_domain_id, float slack) = 0;
        virtual void add_node_slack(TagType type, int node_id, int launch_domain_id, int capture_domain_id, float slack) = 0;
        virtual void finish_results() = 0;

        //End of parsing
        virtual void finish_parse() = 0;

        //Error during parsing
        virtual void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) = 0;
};


/*
 * External functions for loading an SDC file
 */
void tatum_parse_filename(std::string filename, Callback& callback);
void tatum_parse_filename(const char* filename, Callback& callback);

//Loads from 'tatum'. 'filename' only used to pass a filename to callback and can be left unspecified
void tatum_parse_file(FILE* tatum, Callback& callback, const char* filename=""); 

/*
 * Enumerations
 */

} //namespace

#endif
