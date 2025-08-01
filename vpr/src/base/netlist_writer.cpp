/**
 * @file
 *
 * This file implements the netlist writer which generates post-implementation (i.e. post-routing) netlists
 * in BLIF and Verilog formats, along with an SDF file which can be used for timing back-annotation (e.g. for
 * timing simulation).
 *
 * The externally facing function from this module is netlist_writer() which will generate the netlist and SDF
 * output files.  netlist_writer() assumes that a post-routing implementation is available and that there is a
 * valid timing graph with real delays annotated (these delays are used to create the SDF).
 *
 * Implementation
 * ==============
 *
 * The netlist writer is primarily driven by the NetlistWriterVisitor class, which walks the netlist using the
 * NetlistWalker (see netlist_walker.h).  The netlist walker calls NetlistWriterVisitor's  visit_atom_impl()
 * method for every atom (i.e. primitive circuit element) in VPR's internal data structures.
 *
 * visit_atom_impl() then dispatches the atom to the appropriate NetlistWriterVisitor member function
 * (e.g. LUTs to make_lut_instance(), Latches to make_latch_instance(), etc.).  Each of the make_*_instance()
 * functions records the external nets the atom connects to (see make_inst_wire()) and constructs a concrete
 * 'Instance' object to represent the primitive. NetlistWriterVisitor saves these representations for later
 * output.
 *
 * 'Instance' is an abstract class representing objects which know how to create thier own representation in
 * BLIF, Verilog and SDF formats.  Most primitives can be represented by the BlackBoxInst class, but special
 * primitives like LUTs and Latchs have thier own implementations (LutInst, LatchInst) to handle some of their
 * unique requirements.
 *
 * Once the entire netlist has been traversed the netlist walker will call NetlistWriterVisitor's finish_impl()
 * method which kicks off the generation of the actual netlists and SDF files.  NetlistWriterVisitor's print_*()
 * methods setting up file-level global information (I/Os, net declarations etc.) and then ask each Instance to
 * print itself in the appropriate format to the appropriate file.
 *
 * Name Escaping
 * =============
 * One of the challenges in generating netlists is producing consistent naming of netlist elements.
 * In particular valid BLIF names, Verilog names and SDF names are not neccessarily the same.
 * As a result we must escape invalid characters/identifiers when producing some formats.
 *
 * All name escaping is handled at the output stage (i.e. in the print_*() functions) since it is
 * very format dependant.
 *
 * VPR stores names internally generally following BLIF conventions.  As a result these names need
 * to be escaped when generating Verilog or SDF.  This is handled with escape_verilog_identifier()
 * and escape_sdf_identifier() functions.
 *
 * Primitives
 * ==========
 * Verilog netlist generation assumes the existance of appropriate primitives for the various
 * atom types (i.e. a LUT_K module, a DFF module etc.).  These are currently defined the file
 * `<vtr>/vtr_flow/primitives.v`, where `<vtr>` is the root of the VTR source tree.
 *
 * You will typically need to link with this file when attempting to simulate a post-implementation netlist.
 * Also see the comments in primitives.v for important notes related to performing SDF back-annotated timing
 * simulation.
 */

#include "netlist_writer.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "clock_modeling.h"
#include "globals.h"
#include "logic_vec.h"
#include "netlist_walker.h"
#include "read_blif.h"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_version.h"

/* Enable for extra output while calculating LUT masks */
//#define DEBUG_LUT_MASK

namespace {

//
//File local type declarations
//

/**
 * @brief A triple of delay values (all delays should be in seconds).
 *
 * For delay values in SDF files, three numbers are specified to describe the
 * minimum, typical, and maximum delays along a timing edge.
 */
struct DelayTriple {
    DelayTriple() = default;
    constexpr DelayTriple(double minimum_sec, double typical_sec, double maximum_sec)
        : minimum(minimum_sec)
        , typical(typical_sec)
        , maximum(maximum_sec) {}

    /// @brief The minimum delay along a timing edge.
    double minimum = std::numeric_limits<double>::quiet_NaN();
    /// @brief The typical delay along a timing edge.
    double typical = std::numeric_limits<double>::quiet_NaN();
    /// @brief The maximum delay along a timing edge.
    double maximum = std::numeric_limits<double>::quiet_NaN();

    /**
     * @brief Returns true if the minimum, typical, and maximum delay values have
     *        been assigned a number.
     *
     * These values are defaulted to NaN, so this checks if the values have changed.
     */
    inline bool has_value() const {
        return !std::isnan(minimum) && !std::isnan(typical) && !std::isnan(maximum);
    }

    /**
     * @brief Convert the triple into a string. This string will be of the form:
     *          (minimum:typical:maximum)
     *
     * This string is expected to be written directly into an SDF file.
     *
     * Since the delays stored in this struct are implied to be in seconds, this
     * print method converts the output into picoseconds.
     */
    inline std::string str() const {
        VTR_ASSERT_MSG(has_value(),
                       "Cannot create a non-initialized delay triple string");

        // Convert the delays to picoseconds for printing.
        double minimum_ps = minimum * 1e12;
        double typical_ps = typical * 1e12;
        double maximum_ps = maximum * 1e12;

        // Create the string.
        std::stringstream delay_ss;
        delay_ss << '(' << minimum_ps << ':' << typical_ps << ':' << maximum_ps << ')';
        return delay_ss.str();
    }
};

// This pair cointains the following values:
//      - double: hold, setup or clock-to-q delays of the port
//      - string: port name of the associated source clock pin of the sequential port
typedef std::pair<DelayTriple, std::string> sequential_port_delay_pair;

/*enum class PortType {
 * IN,
 * OUT,
 * CLOCK
 * };*/

//
// File local function declarations
//

/**
 * @brief Get the tco delay triple for the given pb_graph pin.
 */
DelayTriple get_pin_tco_delay_triple(const t_pb_graph_pin& pin);

/**
 * @brief Get the edge delay triple for the given edge, as found in the given
 *        timing graph.
 */
DelayTriple get_edge_delay_triple(tatum::EdgeId edge_id,
                                  const AnalysisDelayCalculator& delay_calc,
                                  const tatum::TimingGraph& timing_graph);

std::string indent(size_t depth);

void print_blif_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, int depth);
void print_verilog_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, PortType type, int depth, struct t_analysis_opts& opts);

std::string create_unconn_net(size_t& unconn_count);
std::string escape_verilog_identifier(const std::string id);
std::string escape_sdf_identifier(const std::string id);
bool is_special_sdf_char(char c);
std::string join_identifier(std::string lhs, std::string rhs);

//
//
// File local class and function implementations
//
//

// Unconnected net prefix
const std::string unconn_prefix = "__vpr__unconn";

//A combinational timing arc
class Arc {
  public:
    Arc(std::string src_port,  ///<Source of the arc
        int src_ipin,          ///<Source pin index
        std::string snk_port,  ///<Sink of the arc
        int snk_ipin,          ///<Sink pin index
        DelayTriple del,       ///<Delay on this arc
        std::string cond = "") ///<Condition associated with the arc
        : source_name_(src_port)
        , source_ipin_(src_ipin)
        , sink_name_(snk_port)
        , sink_ipin_(snk_ipin)
        , delay_(del)
        , condition_(cond) {}

    //Accessors
    std::string source_name() const { return source_name_; }
    int source_ipin() const { return source_ipin_; }
    std::string sink_name() const { return sink_name_; }
    int sink_ipin() const { return sink_ipin_; }
    const DelayTriple& delay() const { return delay_; }
    std::string condition() const { return condition_; }

  private:
    std::string source_name_;
    int source_ipin_;
    std::string sink_name_;
    int sink_ipin_;
    DelayTriple delay_;
    std::string condition_;
};

/**
 * @brief Instance is an interface used to represent an element instantiated in a netlist
 *
 * Instances know how to describe themselves in BLIF, Verilog and SDF
 *
 * This should be subclassed to implement support for new primitive types (although
 * see BlackBoxInst for a general implementation for a black box primitive)
 */
class Instance {
  public:
    virtual ~Instance() = default;

    /**
     * @brief Print the current instance in blif format
     *
     *   @param os  The output stream to print to
     *   @param unconn_count  The current count of unconnected nets.
     *              BLIF has limitations requiring unconnected nets to be used to
     *              represent unconnected ports. To allow unique naming of these nets
     *              unconn_count is used to uniquify these names. Whenever creating
     *              an unconnected net (and using unconn_count to uniquify its name
     *              in the file) unconn_count should be incremented.
     *
     *   @param depth  Current indentation depth.  This is used to figure-out how much indentation
     *                 should be applied. This is purely for cosmetic formatting.  Use indent() for
     *                 generating consistent indentation.
     */
    virtual void print_blif(std::ostream& os, size_t& unconn_count, int depth = 0) = 0;

    ///@brief Print the current instanse in Verilog, see print_blif() for argument descriptions
    virtual void print_verilog(std::ostream& os, size_t& unconn_count, int depth = 0) = 0;

    ///@brief Print the current instanse in SDF, see print_blif() for argument descriptions
    virtual void print_sdf(std::ostream& os, int depth = 0) = 0;
};

///@brief An instance representing a Look-Up Table
class LutInst : public Instance {
  public:                                                               ///<Public methods
    LutInst(size_t lut_size,                                            ///<The LUT size
            LogicVec lut_mask,                                          ///<The LUT mask representing the logic function
            std::string inst_name,                                      ///<The name of this instance
            std::map<std::string, std::vector<std::string>> port_conns, ///<The port connections of this instance. Key: port name, Value: connected nets
            std::vector<Arc> timing_arc_values,                         ///<The timing arcs of this instance
            struct t_analysis_opts opts)
        : type_(opts.post_synth_netlist_module_parameters ? "LUT_K" : "LUT_" + std::to_string(lut_size))
        , lut_size_(lut_size)
        , lut_mask_(lut_mask)
        , inst_name_(inst_name)
        , port_conns_(port_conns)
        , timing_arcs_(timing_arc_values)
        , opts_(opts) {
    }

    //Accessors
    const std::vector<Arc>& timing_arcs() { return timing_arcs_; }
    std::string instance_name() { return inst_name_; }
    std::string type() { return type_; }

  public: //Instance interface method implementations
    void print_verilog(std::ostream& os, size_t& unconn_count, int depth) override {
        //Instantiate the lut
        os << indent(depth) << type_;

        // If module parameters are on, pass the lut size and the lut mask as parameters.
        if (opts_.post_synth_netlist_module_parameters) {
            os << " #(\n";

            os << indent(depth + 1) << ".K(" << lut_size_ << "),\n";

            std::stringstream param_ss;
            param_ss << lut_mask_;
            os << indent(depth + 1) << ".LUT_MASK(" << param_ss.str() << ")\n";

            os << indent(depth) << ")";
        }

        os << " " << escape_verilog_identifier(inst_name_) << " (\n";

        VTR_ASSERT(port_conns_.count("in"));
        VTR_ASSERT(port_conns_.count("out"));
        VTR_ASSERT(port_conns_.size() == 2);

        // If module parameters are not on, the mask of the lut will be passed
        // as input to the module.
        if (!opts_.post_synth_netlist_module_parameters) {
            os << indent(depth + 1) << ".mask(" << lut_mask_ << "),\n";
        }

        print_verilog_port(os, unconn_count, "in", port_conns_["in"], PortType::INPUT, depth + 1, opts_);
        os << ","
           << "\n";
        print_verilog_port(os, unconn_count, "out", port_conns_["out"], PortType::OUTPUT, depth + 1, opts_);
        os << "\n";

        os << indent(depth) << ");\n\n";
    }

    void print_blif(std::ostream& os, size_t& unconn_count, int depth) override {
        os << indent(depth) << ".names ";

        //Input nets
        for (const auto& net : port_conns_["in"]) {
            if (net == "") {
                //Disconnected
                os << create_unconn_net(unconn_count) << " ";
            } else {
                os << net << " ";
            }
        }

        VTR_ASSERT(port_conns_["out"].size() == 1);

        //Output net
        auto out_net = port_conns_["out"][0];
        if (out_net == "") {
            //Disconnected
            os << create_unconn_net(unconn_count) << " ";
        } else {
            os << out_net << " ";
        }
        os << "\n";

        //Write out the truth table.
        //
        // We can write out the truth table as either a set of minterms (where the function is true),
        // or a set of maxterms (where the function is false). We choose the representation which produces
        // the fewest terms (and is smallest to represent in BLIF).

        //Count the number of set minterms
        size_t num_set_minterms = 0;
        for (size_t i = 0; i < lut_mask_.size(); ++i) {
            if (lut_mask_[i] == vtr::LogicValue::TRUE) {
                ++num_set_minterms;
            }
        }

        vtr::LogicValue output_value;
        if (num_set_minterms < lut_mask_.size() / 2) {
            //Less than half the minterms are true, so
            //output minterms
            output_value = vtr::LogicValue::TRUE;
        } else {
            //Half-or-more minterms are true, so
            //output maxterms
            output_value = vtr::LogicValue::FALSE;
        }

        size_t minterms_set = 0;
        size_t maxterms_set = 0;
        for (size_t minterm = 0; minterm < lut_mask_.size(); minterm++) {
            if (lut_mask_[minterm] == output_value) {
                //Convert the minterm to a string of bits
                std::string bit_str = std::bitset<64>(minterm).to_string();

                //Because BLIF puts the input values in order from LSB (left) to MSB (right), we need
                //to reverse the string
                std::reverse(bit_str.begin(), bit_str.end());

                //Trim to the LUT size
                std::string input_values(bit_str.begin(), bit_str.begin() + (lut_size_));

                //Add the row as true
                os << indent(depth) << input_values;

                //Specify whether this is a minterm (on-set) or max-term (off-set)
                if (output_value == vtr::LogicValue::TRUE) {
                    os << " 1\n";
                    minterms_set++;
                } else {
                    VTR_ASSERT(output_value == vtr::LogicValue::FALSE);
                    os << " 0\n";
                    maxterms_set++;
                }
            }
        }
        if (minterms_set == 0 && maxterms_set == 0) {
            //Handle the always true/false case
            for (size_t i = 0; i < port_conns_["in"].size(); ++i) {
                os << "-"; //Don't care for all inputs
            }

            if (output_value == vtr::LogicValue::TRUE) {
                //Always false
                os << " 0\n";
            } else {
                VTR_ASSERT(output_value == vtr::LogicValue::FALSE);
                //Always true
                os << " 1\n";
            }
        }
        os << "\n";
    }

    void print_sdf(std::ostream& os, int depth) override {
        os << indent(depth) << "(CELL\n";
        os << indent(depth + 1) << "(CELLTYPE \"" << type() << "\")\n";
        os << indent(depth + 1) << "(INSTANCE " << escape_sdf_identifier(instance_name()) << ")\n";

        if (!timing_arcs().empty()) {
            os << indent(depth + 1) << "(DELAY\n";
            os << indent(depth + 2) << "(ABSOLUTE\n";

            for (auto& arc : timing_arcs()) {
                os << indent(depth + 3) << "(IOPATH ";
                //Note we do not escape the last index of multi-bit signals since they are used to
                //match multi-bit ports
                os << escape_sdf_identifier(arc.source_name()) << "[" << arc.source_ipin() << "]"
                   << " ";

                VTR_ASSERT(arc.sink_ipin() == 0); //Should only be one output
                os << escape_sdf_identifier(arc.sink_name()) << " ";
                os << arc.delay().str() << " " << arc.delay().str() << ")\n";
            }
            os << indent(depth + 2) << ")\n";
            os << indent(depth + 1) << ")\n";
        }

        os << indent(depth) << ")\n";
        os << indent(depth) << "\n";
    }

  private:
    std::string type_;
    size_t lut_size_;
    LogicVec lut_mask_;
    std::string inst_name_;
    std::map<std::string, std::vector<std::string>> port_conns_;
    std::vector<Arc> timing_arcs_;
    struct t_analysis_opts opts_;
};

class LatchInst : public Instance {
  public: //Public types
    ///@brief Types of latches (defined by BLIF)
    enum class Type {
        RISING_EDGE,
        FALLING_EDGE,
        ACTIVE_HIGH,
        ACTIVE_LOW,
        ASYNCHRONOUS,
    };
    friend std::ostream& operator<<(std::ostream& os, const Type& type) {
        if (type == Type::RISING_EDGE)
            os << "re";
        else if (type == Type::FALLING_EDGE)
            os << "fe";
        else if (type == Type::ACTIVE_HIGH)
            os << "ah";
        else if (type == Type::ACTIVE_LOW)
            os << "al";
        else if (type == Type::ASYNCHRONOUS)
            os << "as";
        else
            VTR_ASSERT(false);
        return os;
    }
    friend std::istream& operator>>(std::istream& is, Type& type) {
        std::string tok;
        is >> tok;
        if (tok == "re")
            type = Type::RISING_EDGE;
        else if (tok == "fe")
            type = Type::FALLING_EDGE;
        else if (tok == "ah")
            type = Type::ACTIVE_HIGH;
        else if (tok == "al")
            type = Type::ACTIVE_LOW;
        else if (tok == "as")
            type = Type::ASYNCHRONOUS;
        else
            VTR_ASSERT(false);
        return is;
    }

  public:
    LatchInst(std::string inst_name,                         ///<Name of this instance
              std::map<std::string, std::string> port_conns, ///<Instance's port-to-net connections
              Type type,                                     ///<Type of this latch
              vtr::LogicValue init_value,                    ///<Initial value of the latch
              DelayTriple tcq,                               ///<Clock-to-Q delay
              DelayTriple tsu,                               ///<Setup time
              DelayTriple thld,                              ///<Hold time
              t_analysis_opts opts)
        : instance_name_(inst_name)
        , port_connections_(port_conns)
        , type_(type)
        , initial_value_(init_value)
        , tcq_delay_triple_(tcq)
        , tsu_delay_triple_(tsu)
        , thld_delay_triple_(thld)
        , opts_(opts) {}

    void print_blif(std::ostream& os, size_t& /*unconn_count*/, int depth = 0) override {
        os << indent(depth) << ".latch"
           << " ";

        //Input D port
        auto d_port_iter = port_connections_.find("D");
        VTR_ASSERT(d_port_iter != port_connections_.end());
        os << d_port_iter->second << " ";

        //Output Q port
        auto q_port_iter = port_connections_.find("Q");
        VTR_ASSERT(q_port_iter != port_connections_.end());
        os << q_port_iter->second << " ";

        //Latch type
        os << type_ << " "; //Type, i.e. rising-edge

        //Control input
        auto control_port_iter = port_connections_.find("clock");
        VTR_ASSERT(control_port_iter != port_connections_.end());
        os << control_port_iter->second << " "; //e.g. clock
        os << (int)initial_value_ << " ";       //Init value: e.g. 2=don't care
        os << "\n";
    }

    void print_verilog(std::ostream& os, size_t& /*unconn_count*/, int depth = 0) override {
        //Currently assume a standard DFF
        VTR_ASSERT(type_ == Type::RISING_EDGE);

        os << indent(depth) << "DFF";

        // If module parameters are turned on, pass in the initial value of the
        // register as a parameter.
        // Note: This initial value is only used for simulation.
        if (opts_.post_synth_netlist_module_parameters) {
            os << " #(\n";
            os << indent(depth + 1) << ".INITIAL_VALUE(";
            if (initial_value_ == vtr::LogicValue::TRUE)
                os << "1'b1";
            else if (initial_value_ == vtr::LogicValue::FALSE)
                os << "1'b0";
            else if (initial_value_ == vtr::LogicValue::DONT_CARE)
                os << "1'bx";
            else if (initial_value_ == vtr::LogicValue::UNKOWN)
                os << "1'bx";
            else
                VTR_ASSERT(false);
            os << ")\n";
            os << indent(depth) << ")";
        }

        os << " " << escape_verilog_identifier(instance_name_) << " (\n";

        for (auto iter = port_connections_.begin(); iter != port_connections_.end(); ++iter) {
            os << indent(depth + 1) << "." << iter->first << "(" << escape_verilog_identifier(iter->second) << ")";

            if (iter != --port_connections_.end()) {
                os << ", ";
            }
            os << "\n";
        }
        os << indent(depth) << ");";
        os << "\n";
    }

    void print_sdf(std::ostream& os, int depth = 0) override {
        VTR_ASSERT(type_ == Type::RISING_EDGE);

        os << indent(depth) << "(CELL\n";
        os << indent(depth + 1) << "(CELLTYPE \""
           << "DFF"
           << "\")\n";
        os << indent(depth + 1) << "(INSTANCE " << escape_sdf_identifier(instance_name_) << ")\n";

        //Clock to Q
        if (tcq_delay_triple_.has_value()) {
            os << indent(depth + 1) << "(DELAY\n";
            os << indent(depth + 2) << "(ABSOLUTE\n";
            os << indent(depth + 3) << "(IOPATH "
               << "(posedge clock) Q " << tcq_delay_triple_.str() << " " << tcq_delay_triple_.str() << ")\n";
            os << indent(depth + 2) << ")\n";
            os << indent(depth + 1) << ")\n";
        }

        //Setup/Hold
        if (tsu_delay_triple_.has_value() || thld_delay_triple_.has_value()) {
            os << indent(depth + 1) << "(TIMINGCHECK\n";
            if (tsu_delay_triple_.has_value()) {
                os << indent(depth + 2) << "(SETUP D (posedge clock) " << tsu_delay_triple_.str() << ")\n";
            }
            if (thld_delay_triple_.has_value()) {
                os << indent(depth + 2) << "(HOLD D (posedge clock) " << thld_delay_triple_.str() << ")\n";
            }
        }
        os << indent(depth + 1) << ")\n";
        os << indent(depth) << ")\n";
        os << indent(depth) << "\n";
    }

  private:
    std::string instance_name_;
    std::map<std::string, std::string> port_connections_;
    Type type_;
    vtr::LogicValue initial_value_;
    DelayTriple tcq_delay_triple_;  ///<Clock delay + tcq
    DelayTriple tsu_delay_triple_;  ///<Setup time
    DelayTriple thld_delay_triple_; ///<Hold time
    t_analysis_opts opts_;
};

class BlackBoxInst : public Instance {
  public:
    BlackBoxInst(std::string type_name,                                             ///<Instance type
                 std::string inst_name,                                             ///<Instance name
                 std::map<std::string, std::string> params,                         ///<Verilog parameters: Dictonary of <param_name,value>
                 std::map<std::string, std::string> attrs,                          ///<Instance attributes: Dictonary of <attr_name,value>
                 std::map<std::string, std::vector<std::string>> input_port_conns,  ///<Port connections: Dictionary of <port,nets>
                 std::map<std::string, std::vector<std::string>> output_port_conns, ///<Port connections: Dictionary of <port,nets>
                 std::vector<Arc> timing_arcs,                                      ///<Combinational timing arcs
                 std::map<std::string, sequential_port_delay_pair> ports_tsu,       ///<Port setup checks
                 std::map<std::string, sequential_port_delay_pair> ports_thld,      ///<Port hold checks
                 std::vector<Arc> cq_timing_arcs,                                   ///<Port clock-to-q timing arcs
                 struct t_analysis_opts opts)
        : type_name_(type_name)
        , inst_name_(inst_name)
        , params_(params)
        , attrs_(attrs)
        , input_port_conns_(input_port_conns)
        , output_port_conns_(output_port_conns)
        , timing_arcs_(timing_arcs)
        , ports_tsu_(ports_tsu)
        , ports_thld_(ports_thld)
        , cq_timing_arcs_(cq_timing_arcs)
        , opts_(opts) {}

    void print_blif(std::ostream& os, size_t& unconn_count, int depth = 0) override {
        os << indent(depth) << ".subckt " << type_name_ << " \\"
           << "\n";

        //Input ports
        for (auto iter = input_port_conns_.begin(); iter != input_port_conns_.end(); ++iter) {
            const auto& port_name = iter->first;
            const auto& nets = iter->second;
            print_blif_port(os, unconn_count, port_name, nets, depth + 1);

            if (!(iter == --input_port_conns_.end() && output_port_conns_.empty())) {
                os << " \\";
            }
            os << "\n";
        }

        //Output ports
        for (auto iter = output_port_conns_.begin(); iter != output_port_conns_.end(); ++iter) {
            const auto& port_name = iter->first;
            const auto& nets = iter->second;
            print_blif_port(os, unconn_count, port_name, nets, depth + 1);

            if (!(iter == --output_port_conns_.end())) {
                os << " \\";
            }
            os << "\n";
        }

        // Params
        for (auto iter = params_.begin(); iter != params_.end(); ++iter) {
            os << ".param " << iter->first << " " << iter->second << "\n";
        }

        // Attrs
        for (auto iter = attrs_.begin(); iter != attrs_.end(); ++iter) {
            os << ".attr " << iter->first << " " << iter->second << "\n";
        }

        os << "\n";
    }

    void print_verilog(std::ostream& os, size_t& unconn_count, int depth = 0) override {
        //Instance type
        os << indent(depth) << type_name_;

        // Print the parameters if any are provided.
        if (params_.size() > 0) {
            if (opts_.post_synth_netlist_module_parameters) {
                os << " #(\n";

                //Verilog parameters
                for (auto iter = params_.begin(); iter != params_.end(); ++iter) {
                    /* Prepend a prefix if needed */
                    std::stringstream prefix;
                    if (is_binary_param(iter->second)) {
                        prefix << iter->second.length() << "'b";
                    }

                    os << indent(depth + 1) << "." << iter->first << "(" << prefix.str() << iter->second << ")";
                    if (iter != --params_.end()) {
                        os << ",";
                    }
                    os << "\n";
                }
                os << indent(depth) << ")";
            } else {
                // TODO: RAMs are considered black box instructions. These can
                // probably be handled similar to LUTs.
                VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                "Cannot generate black box instruction of type %s "
                                "without using parameters.",
                                type_name_.c_str());
            }
        }

        //Instance name
        os << " " << escape_verilog_identifier(inst_name_) << " (\n";

        //Input Port connections
        for (auto iter = input_port_conns_.begin(); iter != input_port_conns_.end(); ++iter) {
            auto& port_name = iter->first;
            auto& nets = iter->second;
            print_verilog_port(os, unconn_count, port_name, nets, PortType::INPUT, depth + 1, opts_);
            if (!(iter == --input_port_conns_.end() && output_port_conns_.empty())) {
                os << ",";
            }
            os << "\n";
        }

        //Output Port connections
        for (auto iter = output_port_conns_.begin(); iter != output_port_conns_.end(); ++iter) {
            auto& port_name = iter->first;
            auto& nets = iter->second;
            print_verilog_port(os, unconn_count, port_name, nets, PortType::OUTPUT, depth + 1, opts_);
            if (!(iter == --output_port_conns_.end())) {
                os << ",";
            }
            os << "\n";
        }
        os << indent(depth) << ");\n";
        os << "\n";
    }

    void print_sdf(std::ostream& os, int depth = 0) override {
        if (!timing_arcs_.empty() || !cq_timing_arcs_.empty() || !ports_tsu_.empty() || !ports_thld_.empty()) {
            os << indent(depth) << "(CELL\n";
            os << indent(depth + 1) << "(CELLTYPE \"" << type_name_ << "\")\n";
            os << indent(depth + 1) << "(INSTANCE " << escape_sdf_identifier(inst_name_) << ")\n";
            os << indent(depth + 1) << "(DELAY\n";

            if (!timing_arcs_.empty() || !cq_timing_arcs_.empty()) {
                os << indent(depth + 2) << "(ABSOLUTE\n";

                //Combinational paths
                for (const Arc& arc : timing_arcs_) {
                    //Note that we explicitly do not escape the last array indexing so an SDF
                    //reader will treat the ports as multi-bit
                    //
                    //We also only put the last index in if the port has multiple bits
                    os << indent(depth + 3) << "(IOPATH ";
                    os << escape_sdf_identifier(arc.source_name());
                    if (find_port_size(arc.source_name()) > 1) {
                        os << "[" << arc.source_ipin() << "]";
                    }
                    os << " ";
                    os << escape_sdf_identifier(arc.sink_name());
                    if (find_port_size(arc.sink_name()) > 1) {
                        os << "[" << arc.sink_ipin() << "]";
                    }
                    os << " ";
                    os << arc.delay().str();
                    os << ")\n";
                }

                //Clock-to-Q delays
                for (const Arc& cq_arc : cq_timing_arcs_) {
                    os << indent(depth + 3) << "(IOPATH (posedge ";
                    os << escape_sdf_identifier(cq_arc.source_name());
                    if (find_port_size(cq_arc.source_name()) > 1) {
                        os << "[" << cq_arc.source_ipin() << "]";
                    }
                    os << ") ";
                    os << escape_sdf_identifier(cq_arc.sink_name());
                    if (find_port_size(cq_arc.sink_name()) > 1) {
                        os << "[" << cq_arc.sink_ipin() << "]";
                    }
                    os << " ";
                    os << cq_arc.delay().str();
                    os << ")\n";
                }
                os << indent(depth + 2) << ")\n"; //ABSOLUTE
            }
            os << indent(depth + 1) << ")\n"; //DELAY

            if (!ports_tsu_.empty() || !ports_thld_.empty()) {
                //Setup checks
                os << indent(depth + 1) << "(TIMINGCHECK\n";
                for (auto kv : ports_tsu_) {
                    DelayTriple delay_triple = kv.second.first;
                    os << indent(depth + 2) << "(SETUP " << escape_sdf_identifier(kv.first) << " (posedge  " << escape_sdf_identifier(kv.second.second) << ") " << delay_triple.str() << ")\n";
                }
                for (auto kv : ports_thld_) {
                    DelayTriple delay_triple = kv.second.first;
                    os << indent(depth + 2) << "(HOLD " << escape_sdf_identifier(kv.first) << " (posedge " << escape_sdf_identifier(kv.second.second) << ") " << delay_triple.str() << ")\n";
                }
                os << indent(depth + 1) << ")\n"; //TIMINGCHECK
            }
            os << indent(depth) << ")\n"; //CELL
        }
    }

    size_t find_port_size(std::string port_name) {
        auto iter = input_port_conns_.find(port_name);
        if (iter != input_port_conns_.end()) {
            return iter->second.size();
        }

        iter = output_port_conns_.find(port_name);
        if (iter != output_port_conns_.end()) {
            return iter->second.size();
        }
        VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                        "Could not find port %s on %s of type %s\n", port_name.c_str(), inst_name_.c_str(), type_name_.c_str());

        return -1; //Suppress warning
    }

  private:
    std::string type_name_;
    std::string inst_name_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> attrs_;
    std::map<std::string, std::vector<std::string>> input_port_conns_;
    std::map<std::string, std::vector<std::string>> output_port_conns_;
    std::vector<Arc> timing_arcs_;
    std::map<std::string, sequential_port_delay_pair> ports_tsu_;
    std::map<std::string, sequential_port_delay_pair> ports_thld_;
    std::vector<Arc> cq_timing_arcs_;
    struct t_analysis_opts opts_;
};

/**
 * @brief Assignment represents the logical connection between two nets
 *
 * This is synonomous with verilog's 'assign x = y' which connects
 * two nets with logical identity, assigning the value of 'y' to 'x'
 */
class Assignment {
  public:
    Assignment(std::string lval, ///<The left value (assigned to)
               std::string rval) ///<The right value (assigned from)
        : lval_(lval)
        , rval_(rval) {}

    void print_verilog(std::ostream& os, std::string indent) {
        os << indent << "assign " << escape_verilog_identifier(lval_) << " = " << escape_verilog_identifier(rval_) << ";\n";
    }

    void print_merged_verilog(std::ostream& os, std::string indent) {
        os << indent << "assign " << lval_ << " = " << rval_ << ";\n";
    }

    void print_blif(std::ostream& os, std::string indent) {
        os << indent << ".names " << rval_ << " " << lval_ << "\n";
        os << indent << "1 1\n";
    }

  private:
    std::string lval_;
    std::string rval_;
};

/**
 * @brief A class which writes post-synthesis netlists (Verilog and BLIF) and the SDF
 *
 * It implements the NetlistVisitor interface used by NetlistWalker (see netlist_walker.h)
 */
class NetlistWriterVisitor : public NetlistVisitor {
  public:                                          //Public interface
    NetlistWriterVisitor(std::ostream& verilog_os, ///<Output stream for verilog netlist
                         std::ostream& blif_os,    ///<Output stream for blif netlist
                         std::ostream& sdf_os,     ///<Output stream for SDF
                         std::shared_ptr<const AnalysisDelayCalculator> delay_calc,
                         const LogicalModels& models,
                         struct t_analysis_opts opts)
        : verilog_os_(verilog_os)
        , blif_os_(blif_os)
        , sdf_os_(sdf_os)
        , delay_calc_(delay_calc)
        , models_(models)
        , opts_(opts) {
        auto& atom_ctx = g_vpr_ctx.atom();

        //Initialize the pin to tnode look-up
        for (AtomPinId pin : atom_ctx.netlist().pins()) {
            AtomBlockId blk = atom_ctx.netlist().pin_block(pin);
            ClusterBlockId clb_idx = atom_ctx.lookup().atom_clb(blk);

            const t_pb_graph_pin* gpin = atom_ctx.lookup().atom_pin_pb_graph_pin(pin);
            VTR_ASSERT(gpin);
            int pb_pin_idx = gpin->pin_count_in_cluster;

            tatum::NodeId tnode_id = atom_ctx.lookup().atom_pin_tnode(pin);

            auto key = std::make_pair(clb_idx, pb_pin_idx);
            auto value = std::make_pair(key, tnode_id);
            auto ret = pin_id_to_tnode_lookup_.insert(value);
            VTR_ASSERT_MSG(ret.second, "Was inserted");
        }
    }

    //Non copyable/assignable/moveable
    NetlistWriterVisitor(NetlistWriterVisitor& other) = delete;
    NetlistWriterVisitor(NetlistWriterVisitor&& other) = delete;
    NetlistWriterVisitor& operator=(NetlistWriterVisitor& rhs) = delete;
    NetlistWriterVisitor& operator=(NetlistWriterVisitor&& rhs) = delete;

  private: //Internal types
  private: //NetlistVisitor interface functions
    void visit_top_impl(const char* top_level_name) override {
        top_module_name_ = top_level_name;
    }

    void visit_atom_impl(const t_pb* atom) override {
        auto& atom_ctx = g_vpr_ctx.atom();

        auto atom_pb = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
        if (atom_pb == AtomBlockId::INVALID()) {
            return;
        }
        LogicalModelId model_id = atom_ctx.netlist().block_model(atom_pb);
        std::string model_name = models_.model_name(model_id);

        if (model_name == LogicalModels::MODEL_INPUT) {
            inputs_.emplace_back(make_io(atom, PortType::INPUT));
        } else if (model_name == LogicalModels::MODEL_OUTPUT) {
            outputs_.emplace_back(make_io(atom, PortType::OUTPUT));
        } else if (model_name == LogicalModels::MODEL_NAMES) {
            cell_instances_.push_back(make_lut_instance(atom));
        } else if (model_name == LogicalModels::MODEL_LATCH) {
            cell_instances_.push_back(make_latch_instance(atom));
        } else if (model_name == std::string("single_port_ram")) {
            cell_instances_.push_back(make_ram_instance(atom));
        } else if (model_name == std::string("dual_port_ram")) {
            cell_instances_.push_back(make_ram_instance(atom));
        } else if (model_name == std::string("multiply")) {
            cell_instances_.push_back(make_multiply_instance(atom));
        } else if (model_name == std::string("adder")) {
            cell_instances_.push_back(make_adder_instance(atom));
        } else {
            cell_instances_.push_back(make_blackbox_instance(atom));
        }
    }

    void finish_impl() override {
        print_verilog();
        print_blif();
        print_sdf();
    }

  protected:
    virtual void print_primary_io(int depth) {
        //Primary Inputs
        for (auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
            verilog_os_ << indent(depth + 1) << "input " << escape_verilog_identifier(*iter);
            if (iter + 1 != inputs_.end() || outputs_.size() > 0) {
                verilog_os_ << ",";
            }
            verilog_os_ << "\n";
        }
        //Primary Outputs
        for (auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
            verilog_os_ << indent(depth + 1) << "output " << escape_verilog_identifier(*iter);
            if (iter + 1 != outputs_.end()) {
                verilog_os_ << ",";
            }
            verilog_os_ << "\n";
        }
    }

    virtual void print_assignments(int depth) {
        verilog_os_ << "\n";
        verilog_os_ << indent(depth + 1) << "//IO assignments\n";
        for (auto& assign : assignments_) {
            assign.print_verilog(verilog_os_, indent(depth + 1));
        }
    }

    ///@brief Writes out the verilog netlist
    void print_verilog(int depth = 0) {
        verilog_os_ << indent(depth) << "//Verilog generated by VPR " << vtr::VERSION << " from post-place-and-route implementation\n";
        verilog_os_ << indent(depth) << "module " << top_module_name_ << " (\n";

        print_primary_io(depth);
        verilog_os_ << indent(depth) << ");\n";

        //Wire declarations
        verilog_os_ << "\n";
        verilog_os_ << indent(depth + 1) << "//Wires\n";
        for (auto& kv : logical_net_drivers_) {
            verilog_os_ << indent(depth + 1) << "wire " << escape_verilog_identifier(kv.second.first) << ";\n";
        }
        for (auto& kv : logical_net_sinks_) {
            for (auto& wire_tnode_pair : kv.second) {
                verilog_os_ << indent(depth + 1) << "wire " << escape_verilog_identifier(wire_tnode_pair.first) << ";\n";
            }
        }

        //connections between primary I/Os and their internal wires
        print_assignments(depth);

        //Interconnect between cell instances
        verilog_os_ << "\n";
        verilog_os_ << indent(depth + 1) << "//Interconnect\n";
        for (const auto& kv : logical_net_sinks_) {
            auto atom_net_id = kv.first;
            auto driver_iter = logical_net_drivers_.find(atom_net_id);
            VTR_ASSERT(driver_iter != logical_net_drivers_.end());
            const auto& driver_wire = driver_iter->second.first;

            for (auto& sink_wire_tnode_pair : kv.second) {
                std::string inst_name = interconnect_name(driver_wire, sink_wire_tnode_pair.first);
                verilog_os_ << indent(depth + 1) << "fpga_interconnect " << escape_verilog_identifier(inst_name) << " (\n";
                verilog_os_ << indent(depth + 2) << ".datain(" << escape_verilog_identifier(driver_wire) << "),\n";
                verilog_os_ << indent(depth + 2) << ".dataout(" << escape_verilog_identifier(sink_wire_tnode_pair.first) << ")\n";
                verilog_os_ << indent(depth + 1) << ");\n\n";
            }
        }

        //All the cell instances (to an internal buffer for now)
        std::stringstream instances_ss;

        size_t unconn_count = 0;
        for (auto& inst : cell_instances_) {
            inst->print_verilog(instances_ss, unconn_count, depth + 1);
        }

        //Unconnected wires declarations
        if (unconn_count) {
            verilog_os_ << "\n";
            verilog_os_ << indent(depth + 1) << "//Unconnected wires\n";
            for (size_t i = 0; i < unconn_count; ++i) {
                auto name = unconn_prefix + std::to_string(i);
                verilog_os_ << indent(depth + 1) << "wire " << escape_verilog_identifier(name) << ";\n";
            }
        }

        //All the cell instances
        verilog_os_ << "\n";
        verilog_os_ << indent(depth + 1) << "//Cell instances\n";
        verilog_os_ << instances_ss.str();

        verilog_os_ << "\n";
        verilog_os_ << indent(depth) << "endmodule\n";
    }

  private: //Internal Helper functions
    ///@brief Writes out the blif netlist
    void print_blif(int depth = 0) {
        blif_os_ << indent(depth) << "#BLIF generated by VPR " << vtr::VERSION << " from post-place-and-route implementation\n";
        blif_os_ << indent(depth) << ".model " << top_module_name_ << "\n";

        //Primary inputs
        blif_os_ << indent(depth) << ".inputs ";
        for (auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
            blif_os_ << *iter << " ";
        }
        blif_os_ << "\n";

        //Primary outputs
        blif_os_ << indent(depth) << ".outputs ";
        for (auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
            blif_os_ << *iter << " ";
        }
        blif_os_ << "\n";

        //I/O assignments
        blif_os_ << "\n";
        blif_os_ << indent(depth) << "#IO assignments\n";
        for (auto& assign : assignments_) {
            assign.print_blif(blif_os_, indent(depth));
        }

        //All interconnect between elements
        blif_os_ << "\n";
        blif_os_ << indent(depth) << "#Interconnect\n";
        for (const auto& kv : logical_net_sinks_) {
            auto atom_net_id = kv.first;
            auto driver_iter = logical_net_drivers_.find(atom_net_id);
            VTR_ASSERT(driver_iter != logical_net_drivers_.end());
            const auto& driver_wire = driver_iter->second.first;

            for (auto& sink_wire_tnode_pair : kv.second) {
                blif_os_ << indent(depth) << ".names " << driver_wire << " " << sink_wire_tnode_pair.first << "\n";
                blif_os_ << indent(depth) << "1 1\n";
            }
        }

        //The cells
        blif_os_ << "\n";
        blif_os_ << indent(depth) << "#Cell instances\n";
        size_t unconn_count = 0;
        for (auto& inst : cell_instances_) {
            inst->print_blif(blif_os_, unconn_count);
        }

        blif_os_ << "\n";
        blif_os_ << indent(depth) << ".end\n";
    }

    ///@brief Writes out the SDF
    void print_sdf(int depth = 0) {
        sdf_os_ << indent(depth) << "(DELAYFILE\n";
        sdf_os_ << indent(depth + 1) << "(SDFVERSION \"2.1\")\n";
        sdf_os_ << indent(depth + 1) << "(DESIGN \"" << top_module_name_ << "\")\n";
        sdf_os_ << indent(depth + 1) << "(VENDOR \"verilog-to-routing\")\n";
        sdf_os_ << indent(depth + 1) << "(PROGRAM \"vpr\")\n";
        sdf_os_ << indent(depth + 1) << "(VERSION \"" << vtr::VERSION << "\")\n";
        sdf_os_ << indent(depth + 1) << "(DIVIDER /)\n";
        sdf_os_ << indent(depth + 1) << "(TIMESCALE 1 ps)\n";
        sdf_os_ << "\n";

        //Interconnect
        for (const auto& kv : logical_net_sinks_) {
            auto atom_net_id = kv.first;
            auto driver_iter = logical_net_drivers_.find(atom_net_id);
            VTR_ASSERT(driver_iter != logical_net_drivers_.end());
            auto driver_wire = driver_iter->second.first;
            auto driver_tnode = driver_iter->second.second;

            for (auto& sink_wire_tnode_pair : kv.second) {
                auto sink_wire = sink_wire_tnode_pair.first;
                auto sink_tnode = sink_wire_tnode_pair.second;

                sdf_os_ << indent(depth + 1) << "(CELL\n";
                sdf_os_ << indent(depth + 2) << "(CELLTYPE \"fpga_interconnect\")\n";
                sdf_os_ << indent(depth + 2) << "(INSTANCE " << escape_sdf_identifier(interconnect_name(driver_wire, sink_wire)) << ")\n";
                sdf_os_ << indent(depth + 2) << "(DELAY\n";
                sdf_os_ << indent(depth + 3) << "(ABSOLUTE\n";

                DelayTriple delay_triple = get_src_to_sink_delay_triple(driver_tnode, sink_tnode);

                sdf_os_ << indent(depth + 4) << "(IOPATH datain dataout " << delay_triple.str() << " " << delay_triple.str() << ")\n";
                sdf_os_ << indent(depth + 3) << ")\n";
                sdf_os_ << indent(depth + 2) << ")\n";
                sdf_os_ << indent(depth + 1) << ")\n";
                sdf_os_ << indent(depth) << "\n";
            }
        }

        //Cells
        for (const auto& inst : cell_instances_) {
            inst->print_sdf(sdf_os_, depth + 1);
        }

        sdf_os_ << indent(depth) << ")\n";
    }

    /**
     * @brief Returns the name of a circuit-level Input/Output
     *
     * The I/O is recorded and instantiated by the top level output routines
     *   @param atom  The implementation primitive representing the I/O
     *   @param dir   The IO direction
     */
    std::string make_io(const t_pb* atom,
                        PortType dir) {
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;

        std::string io_name;
        int cluster_pin_idx = -1;
        if (dir == PortType::INPUT) {
            VTR_ASSERT(pb_graph_node->num_output_ports == 1);                        //One output port
            VTR_ASSERT(pb_graph_node->num_output_pins[0] == 1);                      //One output pin
            cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

            io_name = atom->name;

        } else {
            VTR_ASSERT(pb_graph_node->num_input_ports == 1);                        //One input port
            VTR_ASSERT(pb_graph_node->num_input_pins[0] == 1);                      //One input pin
            cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

            //Strip off the starting 'out:' that vpr adds to uniqify outputs
            //this makes the port names match the input blif file
            io_name = atom->name + 4;
        }

        const auto& top_pb_route = find_top_pb_route(atom);

        if (top_pb_route.count(cluster_pin_idx)) {
            //Net exists
            auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id; //Connected net in atom netlist

            //Port direction is inverted (inputs drive internal nets, outputs sink internal nets)
            PortType wire_dir = (dir == PortType::INPUT) ? PortType::OUTPUT : PortType::INPUT;

            //Look up the tnode associated with this pin (used for delay calculation)
            tatum::NodeId tnode_id = find_tnode(atom, cluster_pin_idx);

            auto wire_name = make_inst_wire(atom_net_id, tnode_id, io_name, wire_dir, 0, 0);

            //Connect the wires to to I/Os with assign statements
            if (wire_dir == PortType::INPUT) {
                assignments_.emplace_back(io_name, wire_name);
            } else {
                assignments_.emplace_back(wire_name, io_name);
            }
        }

        return io_name;
    }

  protected:
    /**
     * @brief Returns the name of a wire connecting a primitive and global net.
     *
     * The wire is recorded and instantiated by the top level output routines.
     */
    std::string make_inst_wire(AtomNetId atom_net_id,  ///<The id of the net in the atom netlist
                               tatum::NodeId tnode_id, ///<The tnode associated with the primitive pin
                               std::string inst_name,  ///<The name of the instance associated with the pin
                               PortType port_type,     ///<The port direction
                               int port_idx,           ///<The instance port index
                               int pin_idx) {          ///<The instance pin index

        std::string wire_name = inst_name;
        if (port_type == PortType::INPUT) {
            wire_name = join_identifier(wire_name, "input");
        } else if (port_type == PortType::CLOCK) {
            wire_name = join_identifier(wire_name, "clock");
        } else {
            VTR_ASSERT(port_type == PortType::OUTPUT);
            wire_name = join_identifier(wire_name, "output");
        }

        wire_name = join_identifier(wire_name, std::to_string(port_idx));
        wire_name = join_identifier(wire_name, std::to_string(pin_idx));

        auto value = std::make_pair(wire_name, tnode_id);
        if (port_type == PortType::INPUT || port_type == PortType::CLOCK) {
            //Add the sink
            logical_net_sinks_[atom_net_id].push_back(value);

        } else {
            //Add the driver
            VTR_ASSERT(port_type == PortType::OUTPUT);

            auto ret = logical_net_drivers_.insert(std::make_pair(atom_net_id, value));
            VTR_ASSERT(ret.second); //Was inserted, drivers are unique
        }

        return wire_name;
    }

    ///@brief Returns an Instance object representing the LUT
    std::shared_ptr<Instance> make_lut_instance(const t_pb* atom) {
        //Determine what size LUT
        int lut_size = find_num_inputs(atom);

        //Determine the truth table
        auto lut_mask = load_lut_mask(lut_size, atom);

        //Determine the instance name
        auto inst_name = join_identifier("lut", atom->name);

        //Determine the port connections
        std::map<std::string, std::vector<std::string>> port_conns;

        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
        VTR_ASSERT(pb_graph_node->num_input_ports == 1); //LUT has one input port

        const auto& top_pb_route = find_top_pb_route(atom);

        VTR_ASSERT(pb_graph_node->num_output_ports == 1);                                 //LUT has one output port
        VTR_ASSERT(pb_graph_node->num_output_pins[0] == 1);                               //LUT has one output pin
        int sink_cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

        //Add inputs connections
        std::vector<Arc> timing_arcs;
        for (int pin_idx = 0; pin_idx < pb_graph_node->num_input_pins[0]; pin_idx++) {
            int cluster_pin_idx = pb_graph_node->input_pins[0][pin_idx].pin_count_in_cluster; //Unique pin index in cluster

            std::string net;
            if (!top_pb_route.count(cluster_pin_idx)) {
                //Disconnected
                net = "";
            } else {
                //Connected to a net
                auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id; //Connected net in atom netlist
                VTR_ASSERT(atom_net_id);

                //Look up the tnode associated with this pin (used for delay calculation)
                tatum::NodeId src_tnode_id = find_tnode(atom, cluster_pin_idx);
                tatum::NodeId sink_tnode_id = find_tnode(atom, sink_cluster_pin_idx);

                net = make_inst_wire(atom_net_id, src_tnode_id, inst_name, PortType::INPUT, 0, pin_idx);

                //Record the timing arc
                DelayTriple delay_triple = get_src_to_sink_delay_triple(src_tnode_id, sink_tnode_id);

                Arc timing_arc("in", pin_idx, "out", 0, delay_triple);

                timing_arcs.push_back(timing_arc);
            }
            port_conns["in"].push_back(net);
        }

        //Add the single output connection
        {
            /* Check if the output is connected */
            AtomNetId atom_net_id = AtomNetId::INVALID();
            if (top_pb_route.count(sink_cluster_pin_idx))
                atom_net_id = top_pb_route[sink_cluster_pin_idx].atom_net_id; //Connected net in atom netlist

            std::string net;
            if (!atom_net_id) {
                //Disconnected
                net = "";
            } else {
                //Connected to a net

                //Look up the tnode associated with this pin (used for delay calculation)
                tatum::NodeId tnode_id = find_tnode(atom, sink_cluster_pin_idx);

                net = make_inst_wire(atom_net_id, tnode_id, inst_name, PortType::OUTPUT, 0, 0);
            }
            port_conns["out"].push_back(net);
        }

        auto inst = std::make_shared<LutInst>(lut_size, lut_mask, inst_name, port_conns, timing_arcs, opts_);

        return inst;
    }

    ///@brief Returns an Instance object representing the Latch
    std::shared_ptr<Instance> make_latch_instance(const t_pb* atom) {
        std::string inst_name = join_identifier("latch", atom->name);

        const auto& top_pb_route = find_top_pb_route(atom);
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;

        //We expect a single input, output and clock ports
        VTR_ASSERT(pb_graph_node->num_input_ports == 1);
        VTR_ASSERT(pb_graph_node->num_output_ports == 1);
        VTR_ASSERT(pb_graph_node->num_clock_ports == 1);
        VTR_ASSERT(pb_graph_node->num_input_pins[0] == 1);
        VTR_ASSERT(pb_graph_node->num_output_pins[0] == 1);
        VTR_ASSERT(pb_graph_node->num_clock_pins[0] == 1);

        //The connections
        std::map<std::string, std::string> port_conns;

        //Input (D)
        int input_cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
        VTR_ASSERT(top_pb_route.count(input_cluster_pin_idx));
        auto input_atom_net_id = top_pb_route[input_cluster_pin_idx].atom_net_id; //Connected net in atom netlist
        std::string input_net = make_inst_wire(input_atom_net_id, find_tnode(atom, input_cluster_pin_idx), inst_name, PortType::INPUT, 0, 0);
        port_conns["D"] = input_net;

        double tsu = pb_graph_node->input_pins[0][0].tsu;
        DelayTriple tsu_triple(tsu, tsu, tsu);

        double thld = pb_graph_node->input_pins[0][0].thld;
        DelayTriple thld_triple(thld, thld, thld);

        //Output (Q)
        int output_cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
        VTR_ASSERT(top_pb_route.count(output_cluster_pin_idx));
        auto output_atom_net_id = top_pb_route[output_cluster_pin_idx].atom_net_id; //Connected net in atom netlist
        std::string output_net = make_inst_wire(output_atom_net_id, find_tnode(atom, output_cluster_pin_idx), inst_name, PortType::OUTPUT, 0, 0);
        port_conns["Q"] = output_net;

        DelayTriple tcq_triple = get_pin_tco_delay_triple(pb_graph_node->output_pins[0][0]);

        //Clock (control)
        int control_cluster_pin_idx = pb_graph_node->clock_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
        VTR_ASSERT(top_pb_route.count(control_cluster_pin_idx));
        auto control_atom_net_id = top_pb_route[control_cluster_pin_idx].atom_net_id; //Connected net in atom netlist
        std::string control_net = make_inst_wire(control_atom_net_id, find_tnode(atom, control_cluster_pin_idx), inst_name, PortType::CLOCK, 0, 0);
        port_conns["clock"] = control_net;

        //VPR currently doesn't store enough information to determine these attributes,
        //for now assume reasonable defaults.
        LatchInst::Type type = LatchInst::Type::RISING_EDGE;
        vtr::LogicValue init_value = vtr::LogicValue::FALSE;

        return std::make_shared<LatchInst>(inst_name, port_conns, type, init_value, tcq_triple, tsu_triple, thld_triple, opts_);
    }

    /**
     * @brief Returns an Instance object representing the RAM
     * @note  the primtive interface to dual and single port rams is nearly identical,
     *        so we using a single function to handle both
     */
    std::shared_ptr<Instance> make_ram_instance(const t_pb* atom) {
        const auto& top_pb_route = find_top_pb_route(atom);
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
        const t_pb_type* pb_type = pb_graph_node->pb_type;

        VTR_ASSERT(pb_type->class_type == MEMORY_CLASS);

        std::string type = models_.model_name(pb_type->model_id);
        std::string inst_name = join_identifier(type, atom->name);
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> attrs;
        std::map<std::string, std::vector<std::string>> input_port_conns;
        std::map<std::string, std::vector<std::string>> output_port_conns;
        std::vector<Arc> timing_arcs;
        std::map<std::string, sequential_port_delay_pair> ports_tsu;
        std::map<std::string, sequential_port_delay_pair> ports_thld;
        std::vector<Arc> cq_timing_arcs;

        params["ADDR_WIDTH"] = "0";
        params["DATA_WIDTH"] = "0";

        //Process the input ports
        for (int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                const t_port* port = pin->port;
                std::string port_class = port->port_class;

                int cluster_pin_idx = pin->pin_count_in_cluster;
                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);
                    net = make_inst_wire(atom_net_id, find_tnode(atom, cluster_pin_idx), inst_name, PortType::INPUT, iport, ipin);
                }

                //RAMs use a port class specification to identify the purpose of each port
                std::string port_name;
                if (port_class == "address") {
                    port_name = "addr";
                    params["ADDR_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "address1") {
                    port_name = "addr1";
                    params["ADDR_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "address2") {
                    port_name = "addr2";
                    params["ADDR_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "data_in") {
                    port_name = "data";
                    params["DATA_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "data_in1") {
                    port_name = "data1";
                    params["DATA_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "data_in2") {
                    port_name = "data2";
                    params["DATA_WIDTH"] = std::to_string(port->num_pins);
                } else if (port_class == "write_en") {
                    port_name = "we";
                } else if (port_class == "write_en1") {
                    port_name = "we1";
                } else if (port_class == "write_en2") {
                    port_name = "we2";
                } else {
                    VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                }

                input_port_conns[port_name].push_back(net);
                DelayTriple delay_triple(pin->tsu, pin->tsu, pin->tsu);
                ports_tsu[port_name] = std::make_pair(delay_triple, pin->associated_clock_pin->port->name);
            }
        }

        //Process the output ports
        for (int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                const t_port* port = pin->port;
                std::string port_class = port->port_class;

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);
                    net = make_inst_wire(atom_net_id, find_tnode(atom, cluster_pin_idx), inst_name, PortType::OUTPUT, iport, ipin);
                }

                std::string port_name;
                if (port_class == "data_out") {
                    port_name = "out";
                } else if (port_class == "data_out1") {
                    port_name = "out1";
                } else if (port_class == "data_out2") {
                    port_name = "out2";
                } else {
                    VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                }
                output_port_conns[port_name].push_back(net);
                DelayTriple delay_triple = get_pin_tco_delay_triple(*pin);
                cq_timing_arcs.emplace_back(pin->associated_clock_pin->port->name,
                                            pin->associated_clock_pin->pin_number,
                                            port_name,
                                            ipin,
                                            delay_triple);
            }
        }

        //Process the clock ports
        for (int iport = 0; iport < pb_graph_node->num_clock_ports; ++iport) {
            VTR_ASSERT(pb_graph_node->num_clock_pins[iport] == 1); //Expect a single clock port

            for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->clock_pins[iport][ipin];
                const t_port* port = pin->port;
                std::string port_class = port->port_class;

                int cluster_pin_idx = pin->pin_count_in_cluster;
                VTR_ASSERT(top_pb_route.count(cluster_pin_idx));
                auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;

                VTR_ASSERT(atom_net_id); //Must have a clock

                std::string net = make_inst_wire(atom_net_id, find_tnode(atom, cluster_pin_idx), inst_name, PortType::CLOCK, iport, ipin);

                if (port_class == "clock") {
                    VTR_ASSERT(pb_graph_node->num_clock_pins[iport] == 1); //Expect a single clock pin
                    input_port_conns[port->name].push_back(net);
                } else {
                    VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                }
            }
        }

        return std::make_shared<BlackBoxInst>(type, inst_name, params, attrs, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_thld, cq_timing_arcs, opts_);
    }

    ///@brief Returns an Instance object representing a Multiplier
    std::shared_ptr<Instance> make_multiply_instance(const t_pb* atom) {
        auto& timing_ctx = g_vpr_ctx.timing();

        const auto& top_pb_route = find_top_pb_route(atom);
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
        const t_pb_type* pb_type = pb_graph_node->pb_type;

        std::string type_name = models_.model_name(pb_type->model_id);
        std::string inst_name = join_identifier(type_name, atom->name);
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> attrs;
        std::map<std::string, std::vector<std::string>> input_port_conns;
        std::map<std::string, std::vector<std::string>> output_port_conns;
        std::vector<Arc> timing_arcs;
        std::map<std::string, sequential_port_delay_pair> ports_tsu;
        std::map<std::string, sequential_port_delay_pair> ports_thld;
        std::vector<Arc> cq_timing_arcs;

        params["WIDTH"] = "0";

        //Delay matrix[sink_tnode] -> tuple of source_port_name, pin index, delay
        std::map<tatum::NodeId, std::vector<std::tuple<std::string, int, DelayTriple>>> tnode_delay_matrix;

        //Process the input ports
        for (int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                const t_port* port = pin->port;
                params["WIDTH"] = std::to_string(port->num_pins); //Assume same width on all ports

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);
                    auto src_tnode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, src_tnode, inst_name, PortType::INPUT, iport, ipin);

                    //Delays
                    //
                    //We record the source sink tnodes and their delays here
                    for (tatum::EdgeId edge : timing_ctx.graph->node_out_edges(src_tnode)) {
                        DelayTriple delay_triple = get_edge_delay_triple(edge, *delay_calc_, *timing_ctx.graph);
                        auto sink_tnode = timing_ctx.graph->edge_sink_node(edge);
                        tnode_delay_matrix[sink_tnode].emplace_back(port->name, ipin, delay_triple);
                    }
                }

                input_port_conns[port->name].push_back(net);
            }
        }

        //Process the output ports
        for (int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                const t_port* port = pin->port;

                int cluster_pin_idx = pin->pin_count_in_cluster;
                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);

                    auto inode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, inode, inst_name, PortType::OUTPUT, iport, ipin);

                    //Record the timing arcs
                    for (auto& data_tuple : tnode_delay_matrix[inode]) {
                        auto src_name = std::get<0>(data_tuple);
                        auto src_ipin = std::get<1>(data_tuple);
                        auto delay_triple = std::get<2>(data_tuple);
                        timing_arcs.emplace_back(src_name, src_ipin, port->name, ipin, delay_triple);
                    }
                }

                output_port_conns[port->name].push_back(net);
            }
        }

        VTR_ASSERT(pb_graph_node->num_clock_ports == 0); //No clocks

        return std::make_shared<BlackBoxInst>(type_name, inst_name, params, attrs, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_thld, cq_timing_arcs, opts_);
    }

    ///@brief Returns an Instance object representing an Adder
    std::shared_ptr<Instance> make_adder_instance(const t_pb* atom) {
        auto& timing_ctx = g_vpr_ctx.timing();

        const auto& top_pb_route = find_top_pb_route(atom);
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
        const t_pb_type* pb_type = pb_graph_node->pb_type;

        std::string type_name = models_.model_name(pb_type->model_id);
        std::string inst_name = join_identifier(type_name, atom->name);
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> attrs;
        std::map<std::string, std::vector<std::string>> input_port_conns;
        std::map<std::string, std::vector<std::string>> output_port_conns;
        std::vector<Arc> timing_arcs;
        std::map<std::string, sequential_port_delay_pair> ports_tsu;
        std::map<std::string, sequential_port_delay_pair> ports_thld;
        std::vector<Arc> cq_timing_arcs;

        params["WIDTH"] = "0";

        //Delay matrix[sink_tnode] -> tuple of source_port_name, pin index, delay
        std::map<tatum::NodeId, std::vector<std::tuple<std::string, int, DelayTriple>>> tnode_delay_matrix;

        //Process the input ports
        for (int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                const t_port* port = pin->port;

                if (port->name == std::string("a") || port->name == std::string("b")) {
                    params["WIDTH"] = std::to_string(port->num_pins); //Assume same width on all ports
                }

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);

                    //Connected
                    auto src_tnode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, src_tnode, inst_name, PortType::INPUT, iport, ipin);

                    //Delays
                    //
                    //We record the souce sink tnodes and thier delays here
                    for (tatum::EdgeId edge : timing_ctx.graph->node_out_edges(src_tnode)) {
                        DelayTriple delay_triple = get_edge_delay_triple(edge, *delay_calc_, *timing_ctx.graph);
                        auto sink_tnode = timing_ctx.graph->edge_sink_node(edge);
                        tnode_delay_matrix[sink_tnode].emplace_back(port->name, ipin, delay_triple);
                    }
                }

                input_port_conns[port->name].push_back(net);
            }
        }

        //Process the output ports
        for (int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                const t_port* port = pin->port;

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);

                    auto inode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, inode, inst_name, PortType::OUTPUT, iport, ipin);

                    //Record the timing arcs
                    for (auto& data_tuple : tnode_delay_matrix[inode]) {
                        auto src_name = std::get<0>(data_tuple);
                        auto src_ipin = std::get<1>(data_tuple);
                        auto delay = std::get<2>(data_tuple);
                        timing_arcs.emplace_back(src_name, src_ipin, port->name, ipin, delay);
                    }
                }

                output_port_conns[port->name].push_back(net);
            }
        }

        return std::make_shared<BlackBoxInst>(type_name, inst_name, params, attrs, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_thld, cq_timing_arcs, opts_);
    }

    std::shared_ptr<Instance> make_blackbox_instance(const t_pb* atom) {
        const auto& top_pb_route = find_top_pb_route(atom);
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
        const t_pb_type* pb_type = pb_graph_node->pb_type;

        auto& timing_ctx = g_vpr_ctx.timing();
        std::string type_name = models_.model_name(pb_type->model_id);
        std::string inst_name = join_identifier(type_name, atom->name);
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> attrs;
        std::map<std::string, std::vector<std::string>> input_port_conns;
        std::map<std::string, std::vector<std::string>> output_port_conns;
        std::vector<Arc> timing_arcs;

        // Maps to store a sink's port with the corresponding timing edge to that sink
        //  - key   : string corresponding to the port's name
        //  - value : pair with the delay and the associated clock pin port name
        //
        //  tsu : Setup
        //  thld: Hold
        //  tcq : Clock-to-Q
        std::map<std::string, sequential_port_delay_pair> ports_tsu;
        std::map<std::string, sequential_port_delay_pair> ports_thld;
        std::vector<Arc> cq_timing_arcs;

        //Delay matrix[sink_tnode] -> tuple of source_port_name, pin index, delay
        std::map<tatum::NodeId, std::vector<std::tuple<std::string, int, DelayTriple>>> tnode_delay_matrix;

        //Process the input ports
        for (int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                const t_port* port = pin->port;

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";

                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);

                    auto src_tnode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, src_tnode, inst_name, PortType::INPUT, iport, ipin);
                    //Delays
                    //
                    //We record the source's sink tnodes and their delays here
                    for (tatum::EdgeId edge : timing_ctx.graph->node_out_edges(src_tnode)) {
                        DelayTriple delay_triple = get_edge_delay_triple(edge, *delay_calc_, *timing_ctx.graph);
                        auto sink_tnode = timing_ctx.graph->edge_sink_node(edge);
                        tnode_delay_matrix[sink_tnode].emplace_back(port->name, ipin, delay_triple);
                    }
                }

                input_port_conns[port->name].push_back(net);
                if (pin->type == PB_PIN_SEQUENTIAL) {
                    if (!std::isnan(pin->tsu)) {
                        DelayTriple delay_triple(pin->tsu, pin->tsu, pin->tsu);
                        ports_tsu[port->name] = std::make_pair(delay_triple, pin->associated_clock_pin->port->name);
                    }
                    if (!std::isnan(pin->thld)) {
                        DelayTriple delay_triple(pin->thld, pin->thld, pin->thld);
                        ports_thld[port->name] = std::make_pair(delay_triple, pin->associated_clock_pin->port->name);
                    }
                }
            }
        }

        //Process the output ports
        for (int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                const t_port* port = pin->port;

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id);

                    auto inode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, inode, inst_name, PortType::OUTPUT, iport, ipin);
                    //Record the timing arcs
                    for (auto& data_tuple : tnode_delay_matrix[inode]) {
                        auto src_name = std::get<0>(data_tuple);
                        auto src_ipin = std::get<1>(data_tuple);
                        auto delay = std::get<2>(data_tuple);
                        timing_arcs.emplace_back(src_name, src_ipin, port->name, ipin, delay);
                    }
                }

                output_port_conns[port->name].push_back(net);
                if (pin->type == PB_PIN_SEQUENTIAL && !std::isnan(pin->tco_max)) {
                    DelayTriple delay_triple = get_pin_tco_delay_triple(*pin);
                    cq_timing_arcs.emplace_back(pin->associated_clock_pin->port->name,
                                                pin->associated_clock_pin->pin_number,
                                                port->name,
                                                ipin,
                                                delay_triple);
                }
            }
        }

        //Process the clock ports
        for (int iport = 0; iport < pb_graph_node->num_clock_ports; ++iport) {
            for (int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ++ipin) {
                const t_pb_graph_pin* pin = &pb_graph_node->clock_pins[iport][ipin];
                const t_port* port = pin->port;

                int cluster_pin_idx = pin->pin_count_in_cluster;

                std::string net;
                if (!top_pb_route.count(cluster_pin_idx)) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected
                    auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
                    VTR_ASSERT(atom_net_id); //Must have a clock

                    auto src_tnode = find_tnode(atom, cluster_pin_idx);
                    net = make_inst_wire(atom_net_id, src_tnode, inst_name, PortType::CLOCK, iport, ipin);
                }

                input_port_conns[port->name].push_back(net);
            }
        }

        auto& atom_ctx = g_vpr_ctx.atom();
        AtomBlockId blk_id = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
        for (auto param : atom_ctx.netlist().block_params(blk_id)) {
            params[param.first] = param.second;
        }

        for (auto attr : atom_ctx.netlist().block_attrs(blk_id)) {
            attrs[attr.first] = attr.second;
        }

        return std::make_shared<BlackBoxInst>(type_name, inst_name, params, attrs, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_thld, cq_timing_arcs, opts_);
    }

    ///@brief Returns the top level pb_route associated with the given pb
    const t_pb_routes& find_top_pb_route(const t_pb* curr) {
        const t_pb* top_pb = find_top_cb(curr);
        return top_pb->pb_route;
    }

    ///@brief Returns the tnode ID of the given atom's connected cluster pin
    tatum::NodeId find_tnode(const t_pb* atom, int cluster_pin_idx) {
        auto& atom_ctx = g_vpr_ctx.atom();

        AtomBlockId blk_id = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
        ClusterBlockId clb_index = atom_ctx.lookup().atom_clb(blk_id);

        auto key = std::make_pair(clb_index, cluster_pin_idx);
        auto iter = pin_id_to_tnode_lookup_.find(key);
        VTR_ASSERT(iter != pin_id_to_tnode_lookup_.end());

        tatum::NodeId tnode_id = iter->second;
        VTR_ASSERT(tnode_id);

        return tnode_id;
    }

  private:
    ///@brief Returns the top complex block which contains the given pb
    const t_pb* find_top_cb(const t_pb* curr) {
        //Walk up through the pb graph until curr
        //has no parent, at which point it will be the top pb
        const t_pb* parent = curr->parent_pb;
        while (parent != nullptr) {
            curr = parent;
            parent = curr->parent_pb;
        }
        return curr;
    }

    ///@brief Returns a LogicVec representing the LUT mask of the given LUT atom
    LogicVec load_lut_mask(size_t num_inputs,  //LUT size
                           const t_pb* atom) { //LUT primitive
        auto& atom_ctx = g_vpr_ctx.atom();

        LogicalModelId model_id = atom_ctx.netlist().block_model(atom_ctx.lookup().atom_pb_bimap().pb_atom(atom));
        std::string model_name = models_.model_name(model_id);
        VTR_ASSERT(model_name == LogicalModels::MODEL_NAMES);

#ifdef DEBUG_LUT_MASK
        std::cout << "Loading LUT mask for: " << atom->name << std::endl;
#endif

        //Figure out how the inputs were permuted (compared to the input netlist)
        std::vector<int> permute = determine_lut_permutation(num_inputs, atom);

        //Retrieve the truth table
        const auto& truth_table = atom_ctx.netlist().block_truth_table(atom_ctx.lookup().atom_pb_bimap().pb_atom(atom));

        //Apply the permutation
        auto permuted_truth_table = permute_truth_table(truth_table, num_inputs, permute);

        //Convert to lut mask
        LogicVec lut_mask = truth_table_to_lut_mask(permuted_truth_table, num_inputs);

#ifdef DEBUG_LUT_MASK
        std::cout << "\tLUT_MASK: " << lut_mask << std::endl;
#endif

        return lut_mask;
    }

    /**
     * @brief Helper function for load_lut_mask() which determines how the LUT inputs were
     *        permuted compared to the input BLIF
     *
     * Since the LUT inputs may have been rotated from the input blif specification we need to
     * figure out this permutation to reflect the physical implementation connectivity.
     *
     * We return a permutation map (which is a list of swaps from index to index)
     * which is then applied to do the rotation of the lutmask.
     *
     * The net in the atom netlist which was originally connected to pin i, is connected
     * to pin permute[i] in the implementation.
     */
    std::vector<int> determine_lut_permutation(size_t num_inputs, const t_pb* atom_pb) {
        auto& atom_ctx = g_vpr_ctx.atom();

        std::vector<int> permute(num_inputs, OPEN);

#ifdef DEBUG_LUT_MASK
        std::cout << "\tInit Permute: {";
        for (size_t i = 0; i < permute.size(); i++) {
            std::cout << permute[i] << ", ";
        }
        std::cout << "}" << std::endl;
#endif

        //Determine the permutation
        //
        //We walk through the logical inputs to this atom (i.e. in the original truth table/netlist)
        //and find the corresponding input in the implementation atom (i.e. in the current netlist)
        auto ports = atom_ctx.netlist().block_input_ports(atom_ctx.lookup().atom_pb_bimap().pb_atom(atom_pb));
        if (ports.size() == 1) {
            const t_pb_graph_node* gnode = atom_pb->pb_graph_node;
            VTR_ASSERT(gnode->num_input_ports == 1);
            VTR_ASSERT(gnode->num_input_pins[0] >= (int)num_inputs);

            AtomPortId port_id = *ports.begin();

            for (size_t ipin = 0; ipin < num_inputs; ++ipin) {
                AtomNetId impl_input_net_id = find_atom_input_logical_net(atom_pb, ipin); //The net currently connected to input j

                //Find the original pin index
                const t_pb_graph_pin* gpin = &gnode->input_pins[0][ipin];
                BitIndex orig_index = atom_pb->atom_pin_bit_index(gpin);

                if (impl_input_net_id) {
                    //If there is a valid net connected in the implementation
                    AtomNetId logical_net_id = atom_ctx.netlist().port_net(port_id, orig_index);

                    // Fatal error should be flagged when the net marked in implementation
                    // does not match the net marked in input netlist
                    if (impl_input_net_id != logical_net_id) {
                        VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                        "Unmatch:\n\tlogical net is '%s' at pin '%lu'\n\timplmented net is '%s' at pin '%s'\n",
                                        atom_ctx.netlist().net_name(logical_net_id).c_str(),
                                        size_t(orig_index),
                                        atom_ctx.netlist().net_name(impl_input_net_id).c_str(),
                                        gpin->to_string().c_str());
                    }

                    //Mark the permutation.
                    //  The net originally located at orig_index in the atom netlist
                    //  was moved to ipin in the implementation
                    permute[orig_index] = ipin;
                }
            }
        } else {
            //May have no inputs on a constant generator
            VTR_ASSERT(ports.size() == 0);
        }

        //Fill in any missing values in the permutation (i.e. zeros)
        std::set<int> perm_indicies(permute.begin(), permute.end());
        size_t unused_index = 0;
        for (size_t i = 0; i < permute.size(); i++) {
            if (permute[i] == OPEN) {
                while (perm_indicies.count(unused_index)) {
                    unused_index++;
                }
                permute[i] = unused_index;
                perm_indicies.insert(unused_index);
            }
        }

#ifdef DEBUG_LUT_MASK
        std::cout << "\tPermute: {";
        for (size_t k = 0; k < permute.size(); k++) {
            std::cout << permute[k] << ", ";
        }
        std::cout << "}" << std::endl;

        std::cout << "\tBLIF = Input ->  Rotated" << std::endl;
        std::cout << "\t------------------------" << std::endl;
#endif
        return permute;
    }

    /**
     * @brief Helper function for load_lut_mask() which determines if the
     *        names is encodeing the ON (returns true) or OFF (returns false) set.
     */
    bool names_encodes_on_set(vtr::t_linked_vptr* names_row_ptr) {
        //Determine the truth (output value) for this row
        // By default we assume the on-set is encoded to correctly handle
        // constant true/false
        //
        // False output:
        //      .names j
        //
        // True output:
        //      .names j
        //      1
        bool encoding_on_set = true;

        //We may get a nullptr if the output is always false
        if (names_row_ptr) {
            //Determine whether the truth table stores the ON or OFF set
            //
            //  In blif, the 'output values' of a .names must be either '1' or '0', and must be consistent
            //  within a single .names -- that is a single .names can encode either the ON or OFF set
            //  (of which only one will be encoded in a single .names)
            //
            const std::string names_first_row = (const char*)names_row_ptr->data_vptr;
            auto names_first_row_output_iter = names_first_row.end() - 1;

            if (*names_first_row_output_iter == '1') {
                encoding_on_set = true;
            } else if (*names_first_row_output_iter == '0') {
                encoding_on_set = false;
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                "Invalid .names truth-table character '%c'. Must be one of '1', '0' or '-'. \n", *names_first_row_output_iter);
            }
        }

        return encoding_on_set;
    }

    /**
     * @brief Helper function for load_lut_mask()
     *
     * Converts the given names_row string to a LogicVec
     */
    LogicVec names_row_to_logic_vec(const std::string names_row, size_t num_inputs, bool encoding_on_set) {
        //Get an iterator to the last character (i.e. the output value)
        auto output_val_iter = names_row.end() - 1;

        //Sanity-check, the 2nd last character should be a space
        auto space_iter = names_row.end() - 2;
        VTR_ASSERT(*space_iter == ' ');

        //Extract the truth (output value) for this row
        if (*output_val_iter == '1') {
            VTR_ASSERT(encoding_on_set);
        } else if (*output_val_iter == '0') {
            VTR_ASSERT(!encoding_on_set);
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                            "Invalid .names encoding both ON and OFF set\n");
        }

        //Extract the input values for this row
        LogicVec input_values(num_inputs, vtr::LogicValue::FALSE);
        size_t i = 0;
        //Walk through each input in the input cube for this row
        while (names_row[i] != ' ') {
            vtr::LogicValue input_val = vtr::LogicValue::UNKOWN;
            if (names_row[i] == '1') {
                input_val = vtr::LogicValue::TRUE;
            } else if (names_row[i] == '0') {
                input_val = vtr::LogicValue::FALSE;
            } else if (names_row[i] == '-') {
                input_val = vtr::LogicValue::DONT_CARE;
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                                "Invalid .names truth-table character '%c'. Must be one of '1', '0' or '-'. \n", names_row[i]);
            }

            input_values[i] = input_val;
            i++;
        }
        return input_values;
    }

    ///@brief Returns the total number of input pins on the given pb
    int find_num_inputs(const t_pb* pb) {
        int count = 0;
        for (int i = 0; i < pb->pb_graph_node->num_input_ports; i++) {
            count += pb->pb_graph_node->num_input_pins[i];
        }
        return count;
    }
    ///@brief Returns the logical net ID
    AtomNetId find_atom_input_logical_net(const t_pb* atom, int atom_input_idx) {
        const t_pb_graph_node* pb_node = atom->pb_graph_node;

        int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
        const auto& top_pb_route = find_top_pb_route(atom);
        AtomNetId atom_net_id;
        if (top_pb_route.count(cluster_pin_idx)) {
            atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id;
        }
        return atom_net_id;
    }

    ///@brief Returns the name of the routing segment between two wires
    std::string interconnect_name(std::string driver_wire, std::string sink_wire) {
        std::string name = join_identifier("routing_segment", driver_wire);
        name = join_identifier(name, "to");
        name = join_identifier(name, sink_wire);

        return name;
    }

    ///@brief Returns the delay triple from source_tnode to sink_tnode
    DelayTriple get_src_to_sink_delay_triple(tatum::NodeId source_tnode, tatum::NodeId sink_tnode) {
        auto& timing_ctx = g_vpr_ctx.timing();

        tatum::EdgeId edge = timing_ctx.graph->find_edge(source_tnode, sink_tnode);
        VTR_ASSERT(edge);

        return get_edge_delay_triple(edge, *delay_calc_, *timing_ctx.graph);
    }

  private:                        //Data
    std::string top_module_name_; ///<Name of the top level module (i.e. the circuit)
  protected:
    std::vector<std::string> inputs_;                       ///<Name of circuit inputs
    std::vector<std::string> outputs_;                      ///<Name of circuit outputs
    std::vector<Assignment> assignments_;                   ///<Set of assignments (i.e. net-to-net connections)
    std::vector<std::shared_ptr<Instance>> cell_instances_; ///<Set of cell instances

  private:
    //Drivers of logical nets.
    // Key: logic net id, Value: pair of wire_name and tnode_id
    std::map<AtomNetId, std::pair<std::string, tatum::NodeId>> logical_net_drivers_;

    //Sinks of logical nets.
    // Key: logical net id, Value: vector wire_name tnode_id pairs
    std::map<AtomNetId, std::vector<std::pair<std::string, tatum::NodeId>>> logical_net_sinks_;
    std::map<std::string, float> logical_net_sink_delays_;

    //Output streams
  protected:
    std::ostream& verilog_os_;

  private:
    std::ostream& blif_os_;
    std::ostream& sdf_os_;

    //Look-up from pins to tnodes
    std::map<std::pair<ClusterBlockId, int>, tatum::NodeId> pin_id_to_tnode_lookup_;

    std::shared_ptr<const AnalysisDelayCalculator> delay_calc_;

  protected:
    const LogicalModels& models_;

  private:
    struct t_analysis_opts opts_;
};

/**
 * @brief A class which writes post-implementation merged netlists (Verilog)
 *
 * It implements the NetlistVisitor interface used by NetlistWalker (see netlist_walker.h)
 */
class MergedNetlistWriterVisitor : public NetlistWriterVisitor {
  public:                                                //Public interface
    MergedNetlistWriterVisitor(std::ostream& verilog_os, ///<Output stream for verilog netlist
                               std::ostream& blif_os,    ///<Output stream for blif netlist
                               std::ostream& sdf_os,     ///<Output stream for SDF
                               std::shared_ptr<const AnalysisDelayCalculator> delay_calc,
                               const LogicalModels& models,
                               struct t_analysis_opts opts)
        : NetlistWriterVisitor(verilog_os, blif_os, sdf_os, delay_calc, models, opts) {}

    std::map<std::string, int> portmap;

    void visit_atom_impl(const t_pb* atom) override {
        auto& atom_ctx = g_vpr_ctx.atom();

        auto atom_pb = atom_ctx.lookup().atom_pb_bimap().pb_atom(atom);
        if (atom_pb == AtomBlockId::INVALID()) {
            return;
        }
        LogicalModelId model_id = atom_ctx.netlist().block_model(atom_pb);
        std::string model_name = models_.model_name(model_id);

        if (model_name == LogicalModels::MODEL_INPUT) {
            auto merged_io_name = make_io(atom, PortType::INPUT);
            if (merged_io_name != "")
                inputs_.emplace_back(merged_io_name);
        } else if (model_name == LogicalModels::MODEL_OUTPUT) {
            auto merged_io_name = make_io(atom, PortType::OUTPUT);
            if (merged_io_name != "")
                outputs_.emplace_back(merged_io_name);
        } else if (model_name == LogicalModels::MODEL_NAMES) {
            cell_instances_.push_back(make_lut_instance(atom));
        } else if (model_name == LogicalModels::MODEL_LATCH) {
            cell_instances_.push_back(make_latch_instance(atom));
        } else if (model_name == std::string("single_port_ram")) {
            cell_instances_.push_back(make_ram_instance(atom));
        } else if (model_name == std::string("dual_port_ram")) {
            cell_instances_.push_back(make_ram_instance(atom));
        } else if (model_name == std::string("multiply")) {
            cell_instances_.push_back(make_multiply_instance(atom));
        } else if (model_name == std::string("adder")) {
            cell_instances_.push_back(make_adder_instance(atom));
        } else {
            cell_instances_.push_back(make_blackbox_instance(atom));
        }
    }

    /**
     * @brief Returns the name of circuit-level Input/Output ports with multi-bit
     * ports merged into one.
     *
     * The I/O is recorded and instantiated by the top level output routines
     *   @param atom    The implementation primitive representing the I/O
     *   @param dir     The IO direction
     *   @param portmap Map for keeping port names and width
     */
    std::string make_io(const t_pb* atom,
                        PortType dir) {
        const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;

        std::string io_name;
        std::string indexed_io_name;
        int cluster_pin_idx = -1;
        // regex for matching 3 groups:
        // * 'out:' - optional
        // * verilog identifier - mandatory
        // * index - optional
        std::string rgx = "(out:)?([a-zA-Z$_]+[a-zA-Z0-9$_]*)(\\[[0-9]+\\])?$";
        std::string name(atom->name);
        std::regex regex(rgx);
        std::smatch matches;

        if (dir == PortType::INPUT) {
            VTR_ASSERT(pb_graph_node->num_output_ports == 1);                        //One output port
            VTR_ASSERT(pb_graph_node->num_output_pins[0] == 1);                      //One output pin
            cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

            io_name = "";
            indexed_io_name = atom->name;

            if (std::regex_match(name, matches, regex)) {
                if (std::find(inputs_.begin(), inputs_.end(), matches[2]) == inputs_.end()) { //Skip already existing multi-bit port names
                    io_name = matches[2];
                    portmap[matches[2]] = 0;
                } else {
                    portmap[matches[2]]++;
                }
            }

        } else {
            VTR_ASSERT(pb_graph_node->num_input_ports == 1);                        //One input port
            VTR_ASSERT(pb_graph_node->num_input_pins[0] == 1);                      //One input pin
            cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

            //Strip off the starting 'out:' that vpr adds to uniqify outputs
            //this makes the port names match the input blif file

            io_name = "";
            indexed_io_name = atom->name + 4;

            if (std::regex_search(name, matches, regex)) {
                if (std::find(outputs_.begin(), outputs_.end(), matches[2]) == outputs_.end()) { //Skip already existing multi-bit port names
                    portmap[matches[2]] = 0;
                    io_name = matches[2];
                } else {
                    portmap[matches[2]]++;
                }
            }
        }

        const auto& top_pb_route = find_top_pb_route(atom);

        if (top_pb_route.count(cluster_pin_idx)) {
            //Net exists
            auto atom_net_id = top_pb_route[cluster_pin_idx].atom_net_id; //Connected net in atom netlist

            //Port direction is inverted (inputs drive internal nets, outputs sink internal nets)
            PortType wire_dir = (dir == PortType::INPUT) ? PortType::OUTPUT : PortType::INPUT;

            //Look up the tnode associated with this pin (used for delay calculation)
            tatum::NodeId tnode_id = find_tnode(atom, cluster_pin_idx);

            auto wire_name = make_inst_wire(atom_net_id, tnode_id, indexed_io_name, wire_dir, 0, 0);

            //Connect the wires to to I/Os with assign statements
            if (wire_dir == PortType::INPUT) {
                assignments_.emplace_back(indexed_io_name, escape_verilog_identifier(wire_name));
            } else {
                assignments_.emplace_back(escape_verilog_identifier(wire_name), indexed_io_name);
            }
        }

        return io_name;
    }

    void print_primary_io(int depth) override {
        //Primary Inputs
        for (auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
            //verilog_os_ << indent(depth + 1) << "input " << escape_verilog_identifier(*iter);
            std::string range;
            if (portmap[*iter] > 0)
                verilog_os_ << indent(depth + 1) << "input [" << portmap[*iter] << ":0] " << *iter;
            else
                verilog_os_ << indent(depth + 1) << "input " << *iter;
            if (iter + 1 != inputs_.end() || outputs_.size() > 0) {
                verilog_os_ << ",";
            }
            verilog_os_ << "\n";
        }

        //Primary Outputs
        for (auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
            std::string range;
            if (portmap[*iter] > 0)
                verilog_os_ << indent(depth + 1) << "output [" << portmap[*iter] << ":0] " << *iter;
            else
                verilog_os_ << indent(depth + 1) << "output " << *iter;
            if (iter + 1 != outputs_.end()) {
                verilog_os_ << ",";
            }
            verilog_os_ << "\n";
        }
    }

    void print_assignments(int depth) override {
        verilog_os_ << "\n";
        verilog_os_ << indent(depth + 1) << "//IO assignments\n";
        for (auto& assign : assignments_) {
            assign.print_merged_verilog(verilog_os_, indent(depth + 1));
        }
    }

    void finish_impl() override {
        // Don't write to blif and sdf streams
        print_verilog();
    }
};

//
// File-scope function implementations
//

DelayTriple get_pin_tco_delay_triple(const t_pb_graph_pin& pin) {
    DelayTriple delay_triple;
    delay_triple.minimum = pin.tco_min;
    delay_triple.maximum = pin.tco_max;
    // Since Tatum does not provide typical delays, set it to be the average
    // of min and max.
    delay_triple.typical = (pin.tco_min + pin.tco_max) / 2.0;
    return delay_triple;
}

DelayTriple get_edge_delay_triple(tatum::EdgeId edge_id,
                                  const AnalysisDelayCalculator& delay_calc,
                                  const tatum::TimingGraph& timing_graph) {
    double min_edge_delay = delay_calc.min_edge_delay(timing_graph, edge_id);
    double max_edge_delay = delay_calc.max_edge_delay(timing_graph, edge_id);

    DelayTriple delay_triple;
    delay_triple.minimum = min_edge_delay;
    delay_triple.maximum = max_edge_delay;
    // Since Tatum does not provide typical delays, set it to be the average
    // of min and max.
    delay_triple.typical = (min_edge_delay + max_edge_delay) / 2.0;
    return delay_triple;
}

///@brief Returns a blank string for indenting the given depth
std::string indent(size_t depth) {
    std::string indent_ = "    ";
    std::string new_indent;
    for (size_t i = 0; i < depth; ++i) {
        new_indent += indent_;
    }
    return new_indent;
}

///@brief Returns the name of a unique unconnected net
std::string create_unconn_net(size_t& unconn_count) {
    //We increment unconn_count by reference so each
    //call generates a unique name
    return unconn_prefix + std::to_string(unconn_count++);
}

/**
 * @brief Pretty-Prints a blif port to the given output stream
 *
 * Handles special cases like multi-bit and disconnected ports
 */
void print_blif_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, int depth) {
    if (nets.size() == 1) {
        //If only a single bit port, don't include port indexing
        os << indent(depth) << port_name << "=";
        if (nets[0].empty()) {
            os << create_unconn_net(unconn_count);
        } else {
            os << nets[0];
        }
    } else {
        //Include bit indexing for multi-bit ports
        for (int ipin = 0; ipin < (int)nets.size(); ++ipin) {
            os << indent(depth) << port_name << "[" << ipin << "]"
               << "=";

            if (nets[ipin].empty()) {
                os << create_unconn_net(unconn_count);
            } else {
                os << nets[ipin];
            }
            if (ipin != (int)nets.size() - 1) {
                os << " "
                   << "\\"
                   << "\n";
            }
        }
    }
}

/**
 * @brief Pretty-Prints a verilog port to the given output stream
 *
 * Handles special cases like multi-bit and disconnected ports
 */
void print_verilog_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, PortType type, int depth, struct t_analysis_opts& opts) {
    auto unconn_inp_name = [&]() {
        switch (opts.post_synth_netlist_unconn_input_handling) {
            case e_post_synth_netlist_unconn_handling::GND:
                return std::string("1'b0");
            case e_post_synth_netlist_unconn_handling::VCC:
                return std::string("1'b1");
            case e_post_synth_netlist_unconn_handling::NETS:
                return create_unconn_net(unconn_count);
            case e_post_synth_netlist_unconn_handling::UNCONNECTED:
            default:
                return std::string("1'bX");
        }
    };

    auto unconn_out_name = [&]() {
        switch (opts.post_synth_netlist_unconn_output_handling) {
            case e_post_synth_netlist_unconn_handling::NETS:
                return create_unconn_net(unconn_count);
            case e_post_synth_netlist_unconn_handling::UNCONNECTED:
            default:
                return std::string();
        }
    };

    //Port name
    os << indent(depth) << "." << port_name << "(";

    //Pins
    if (nets.size() == 1) {
        //Single-bit port
        if (nets[0].empty()) {
            //Disconnected
            if (type == PortType::INPUT || type == PortType::CLOCK) {
                os << unconn_inp_name();
            } else {
                VTR_ASSERT(type == PortType::OUTPUT);
                os << unconn_out_name();
            }
        } else {
            //Connected
            os << escape_verilog_identifier(nets[0]);
        }
    } else {
        // Check if all pins are unconnected
        bool all_unconnected = true;
        for (size_t i = 0; i < nets.size(); ++i) {
            if (!nets[i].empty()) {
                all_unconnected = false;
                break;
            }
        }

        //A multi-bit port, we explicitly concat the single-bit nets to build the port,
        //taking care to print MSB on left and LSB on right
        if (all_unconnected && type == PortType::OUTPUT && opts.post_synth_netlist_unconn_output_handling == e_post_synth_netlist_unconn_handling::UNCONNECTED) {
            // Empty connection
        } else {
            // Individual bits
            os << "{"
               << "\n";
            for (int ipin = (int)nets.size() - 1; ipin >= 0; --ipin) { //Reverse order to match endianess
                os << indent(depth + 1);
                if (nets[ipin].empty()) {
                    //Disconnected
                    if (type == PortType::INPUT || type == PortType::CLOCK) {
                        os << unconn_inp_name();
                    } else {
                        VTR_ASSERT(type == PortType::OUTPUT);
                        // When concatenating output connection there cannot
                        // be an empty placeholder so we have to create a
                        // dummy net.
                        os << create_unconn_net(unconn_count);
                    }
                } else {
                    //Connected
                    os << escape_verilog_identifier(nets[ipin]);
                }
                if (ipin != 0) {
                    os << ",";
                }
                os << "\n";
            }
            os << indent(depth) + " }";
        }
    }
    os << ")";
}

///@brief Escapes the given identifier to be safe for verilog
std::string escape_verilog_identifier(const std::string identifier) {
    //Verilog allows escaped identifiers
    //
    //The escaped identifiers start with a literal back-slash '\'
    //followed by the identifier and are terminated by white space
    //
    //We pre-pend the escape back-slash and append a space to avoid
    //the identifier gobbling up adjacent characters like commas which
    //are not actually part of the identifier
    std::string prefix = "\\";
    std::string suffix = " ";
    std::string escaped_name = prefix + identifier + suffix;

    return escaped_name;
}

///@brief Returns true if c is categorized as a special character in SDF
bool is_special_sdf_char(char c) {
    //From section 3.2.5 of IEEE1497 Part 3 (i.e. the SDF spec)
    //Special characters run from:
    //    ! to # (ASCII decimal 33-35)
    //    % to / (ASCII decimal 37-47)
    //    : to @ (ASCII decimal 58-64)
    //    [ to ^ (ASCII decimal 91-94)
    //    ` to ` (ASCII decimal 96)
    //    { to ~ (ASCII decimal 123-126)
    //
    //Note that the spec defines _ (decimal code 95) and $ (decimal code 36)
    //as non-special alphanumeric characters.
    //
    //However it inconsistently also lists $ in the list of special characters.
    //Since the spec allows for non-special characters to be escaped (they are treated
    //normally), we treat $ as a special character to be safe.
    //
    //Note that the spec appears to have rendering errors in the PDF availble
    //on IEEE Xplore, listing the 'LEFT-POINTING DOUBLE ANGLE QUOTATION MARK'
    //character (decimal code 171) in place of the APOSTROPHE character '
    //with decimal code 39 in the special character list. We assume code 39.
    if ((c >= 33 && c <= 35) || (c == 36) || // $
        (c >= 37 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 94) || (c == 96) || (c >= 123 && c <= 126)) {
        return true;
    }

    return false;
}

///@brief Escapes the given identifier to be safe for sdf
std::string escape_sdf_identifier(const std::string identifier) {
    //SDF allows escaped characters
    //
    //We look at each character in the string and escape it if it is
    //a special character
    std::string escaped_name;

    for (char c : identifier) {
        if (is_special_sdf_char(c)) {
            //Escape the special character
            escaped_name += '\\';
        }
        escaped_name += c;
    }

    return escaped_name;
}

///@brief Joins two identifier strings
std::string join_identifier(std::string lhs, std::string rhs) {
    return lhs + '_' + rhs;
}

/**
 * @brief Add the original SDC constraints that VPR used during its flow to the
 *        given SDC file.
 *
 *  @param sdc_os
 *      Output stream for the target SDC file. The original SDC file passed into
 *      VPR will be appended to this file.
 *  @param timing_info
 *      Information on the timing within VPR. This is used to get the file path
 *      to the original SDC file.
 */
void add_original_sdc_to_post_implemented_sdc_file(std::ofstream& sdc_os,
                                                   const t_timing_inf& timing_info) {
    // Open the original SDC file provided to VPR.
    std::ifstream original_sdc_file;
    original_sdc_file.open(timing_info.SDCFile);
    if (!original_sdc_file.is_open()) {
        // TODO: VPR automatically creates SDC constraints by default if no SDC
        //       file is provided. These can be replicated here if needed.
        VPR_FATAL_ERROR(VPR_ERROR_IMPL_NETLIST_WRITER,
                        "No SDC files provided to VPR; currently cannot generate "
                        "post-implementation SDC file without it");
    }

    // Write a header to declare where these commands came from.
    sdc_os << "\n";
    sdc_os << "#******************************************************************************#\n";
    sdc_os << "# The following SDC commands were provided to VPR from the given SDC file:\n";
    sdc_os << "# \t" << timing_info.SDCFile << "\n";
    sdc_os << "#******************************************************************************#\n";

    // Append the original SDC file to the post-implementation SDC file.
    sdc_os << original_sdc_file.rdbuf();
}

/**
 * @brief Add propagated clock commands to the given SDC file based on the set
 *        clock modeling.
 *
 * This is necessary since VPR decides if clocks are routed or not, which has
 * affects on how timing analysis is performed on the clocks.
 *
 *  @param sdc_os
 *      The file stream to add the propagated clock commands to.
 */
void add_propagated_clocks_to_sdc_file(std::ofstream& sdc_os) {

    // The timing constraints contain information on all the clocks in the circuit
    // (provided by the user-provided SDC file).
    const auto timing_constraints = g_vpr_ctx.timing().constraints;

    // Collect the non-virtual clocks. Virtual clocks are not routed and
    // do not get propageted.
    std::vector<tatum::DomainId> non_virtual_clocks;
    for (tatum::DomainId clock_domain_id : timing_constraints->clock_domains()) {
        if (!timing_constraints->is_virtual_clock(clock_domain_id)) {
            non_virtual_clocks.push_back(clock_domain_id);
        }
    }

    // If there are no non-virtual clocks, no extra commands needed. Virtual
    // clocks are ideal.
    if (non_virtual_clocks.empty()) {
        return;
    }

    // Append a header to explain why these commands are added.
    sdc_os << "\n";
    sdc_os << "#******************************************************************************#\n";
    sdc_os << "# The following are clock domains in VPR which have delays on their edges.\n";
    sdc_os << "#\n";
    sdc_os << "# Any non-virtual clock has its delay determined and written out as part of a\n";
    sdc_os << "# propagated clock command. If VPR was instructed not to route the clock, this\n";
    sdc_os << "# delay will be an underestimate.\n";
    sdc_os << "#\n";
    sdc_os << "# Note: Virtual clocks do not get routed and are treated as ideal.\n";
    sdc_os << "#******************************************************************************#\n";

    // Add the SDC commands to set the non-virtual clocks as propagated (non-ideal);
    // Note: It was decided that "ideal" (dont route) clock modeling in VPR should still
    //       set the clocks as propagated to allow for the input pad delays of
    //       clocks to be included. The SDF delay annotations on clock signals
    //       should make this safe to do.
    for (tatum::DomainId clock_domain_id : non_virtual_clocks) {
        sdc_os << "set_propagated_clock ";
        sdc_os << timing_constraints->clock_domain_name(clock_domain_id);
        sdc_os << "\n";
    }
}

/**
 * @brief Generates a post-implementation SDC file with the given file name
 *        based on the timing info used for VPR.
 *
 *  @param sdc_filename
 *      The file name of the SDC file to generate.
 *  @param timing_info
 *      Information on the timing used in the VPR flow.
 */
void generate_post_implementation_sdc(const std::string& sdc_filename,
                                      const t_timing_inf& timing_info) {
    if (!timing_info.timing_analysis_enabled) {
        VTR_LOG_WARN("Timing analysis is disabled. Post-implementation SDC file "
                     "will not be generated.\n");
        return;
    }

    // Begin writing the post-implementation SDC file.
    std::ofstream sdc_os(sdc_filename);

    // Print a header declaring that this file is auto-generated and what version
    // of VTR produced it.
    sdc_os << "#******************************************************************************#\n";
    sdc_os << "# SDC automatically generated by VPR from a post-place-and-route implementation.\n";
    sdc_os << "#\tVersion: " << vtr::VERSION << "\n";
    sdc_os << "#******************************************************************************#\n";

    // Add the original SDC that VPR used during its flow.
    add_original_sdc_to_post_implemented_sdc_file(sdc_os, timing_info);

    // Add propagated clocks to SDC file if needed.
    add_propagated_clocks_to_sdc_file(sdc_os);
}

} // namespace

//
// Externally Accessible Functions
//

///@brief Main routine for this file. See netlist_writer.h for details.
void netlist_writer(const std::string basename,
                    std::shared_ptr<const AnalysisDelayCalculator> delay_calc,
                    const LogicalModels& models,
                    const t_timing_inf& timing_info,
                    t_analysis_opts opts) {
    std::string verilog_filename = basename + "_post_synthesis.v";
    std::string blif_filename = basename + "_post_synthesis.blif";
    std::string sdf_filename = basename + "_post_synthesis.sdf";

    VTR_LOG("Writing Implementation Netlist: %s\n", verilog_filename.c_str());
    VTR_LOG("Writing Implementation Netlist: %s\n", blif_filename.c_str());
    VTR_LOG("Writing Implementation SDF    : %s\n", sdf_filename.c_str());
    std::ofstream verilog_os(verilog_filename);
    std::ofstream blif_os(blif_filename);
    std::ofstream sdf_os(sdf_filename);

    NetlistWriterVisitor visitor(verilog_os, blif_os, sdf_os, delay_calc, models, opts);

    NetlistWalker nl_walker(visitor);

    nl_walker.walk();

    if (opts.gen_post_implementation_sdc) {
        std::string sdc_filename = basename + "_post_synthesis.sdc";

        VTR_LOG("Writing Implementation SDC    : %s\n", sdc_filename.c_str());

        generate_post_implementation_sdc(sdc_filename,
                                         timing_info);
    }
}

///@brief Main routine for this file. See netlist_writer.h for details.
void merged_netlist_writer(const std::string basename, std::shared_ptr<const AnalysisDelayCalculator> delay_calc, const LogicalModels& models, t_analysis_opts opts) {
    std::string verilog_filename = basename + "_merged_post_implementation.v";

    VTR_LOG("Writing Merged Implementation Netlist: %s\n", verilog_filename.c_str());

    std::ofstream verilog_os(verilog_filename);
    // Don't write blif and sdf, pass dummy streams
    std::ofstream blif_os;
    std::ofstream sdf_os;

    MergedNetlistWriterVisitor visitor(verilog_os, blif_os, sdf_os, delay_calc, models, opts);

    NetlistWalker nl_walker(visitor);

    nl_walker.walk();
}
