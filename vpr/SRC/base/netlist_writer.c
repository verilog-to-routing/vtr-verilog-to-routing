#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <bitset>
#include <memory>
#include <unordered_set>

#include "netlist_walker.h"
#include "netlist_writer.h"

#include "globals.h"
#include "path_delay.h"

//Overview
//========
//
//This file implements the netlist writer which generates post-implementation (i.e. post-routing) netlists
//in BLIF and Verilog formats, along with an SDF file which can be used for timing back-annotation (e.g. for
//timing simulation).
//
//The externally facing function from this module is netlist_writer() which will generate the netlist and SDF
//output files.  netlist_writer() assumes that a post-routing implementation is available and that there is a
//valid timing graph with real delays annotated (these delays are used to create the SDF).
//
//Implementation
//==============
//
//The netlist writer is primarily driven by the NetlistWriterVisitor class, which walks the netlist using the
//NetlistWalker (see netlist_walker.h).  The netlist walker calls NetlistWriterVisitor's  visit_atom_impl() 
//method for every atom (i.e. primitive circuit element) in VPR's internal data structures.
//
//visit_atom_impl() then dispatches the atom to the appropriate NetlistWriterVisitor member function 
//(e.g. LUTs to make_lut_instance(), Latches to make_latch_instance(), etc.).  Each of the make_*_instance()
//functions records the external nets the atom connects to (see make_inst_wire()) and constructs a concrete
//'Instance' object to represent the primitive. NetlistWriterVisitor saves these representations for later
//output.
//
//'Instance' is an abstract class representing objects which know how to create thier own representation in
//BLIF, Verilog and SDF formats.  Most primitives can be represented by the BlackBoxInst class, but special 
//primitives like LUTs and Latchs have thier own implementations (LutInst, LatchInst) to handle some of their
//unique requirements.
//
//Once the entire netlist has been traversed the netlist walker will call NetlistWriterVisitor's finish_impl()
//method which kicks off the generation of the actual netlists and SDF files.  NetlistWriterVisitor's print_*() 
//methods setting up file-level global information (I/Os, net declarations etc.) and then ask each Instance to
//print itself in the appropriate format to the appropriate file.
//
//Name Escaping
//=============
//One of the challenges in generating netlists is producing consistent naming of netlist elements.
//In particular valid BLIF names, Verilog names and SDF names are not neccessarily the same.
//As a result we must escape invalid characters/identifiers when producing some formats.
//
//All name escaping is handled at the output stage (i.e. in the print_*() functions) since it is
//very format dependant.
//
//VPR stores names internally generally following BLIF conventions.  As a result these names need
//to be escaped when generating Verilog or SDF.  This is handled with escape_verilog_identifier()
//and escape_sdf_identifier() functions.
//
//Primitives
//==========
//Verilog netlist generation assumes the existance of appropriate primitives for the various
//atom types (i.e. a LUT_K module, a DFF module etc.).  These are currently defined the file 
//<vtr>/vtr_flow/primtives.v, where <vtr> is the root of the VTR source tree.
//
//You will typically need to link with this file when attempting to simulate a post-implementation netlist.
//Also see the comments in primtives.v for important notes related to performing SDF back-annotated timing
//simulation.


//Enable for extra output while calculating LUT masks
/*#define DEBUG_LUT_MASK*/

//
//File local type delcarations
//
enum class LogicVal {
    FALSE=0,
    TRUE=1,
    DONTCARE=2,
    UNKOWN=3,
    HIGHZ
};
std::ostream& operator<<(std::ostream& os, LogicVal val);


enum class PortType {
    IN,
    OUT,
    CLOCK
};

//
// File local function delcarations
//
std::string indent(size_t depth);
double get_delay_ps(double delay_sec);
double get_delay_ps(int source_tnode, int sink_tnode);

void print_blif_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, int depth);
void print_verilog_port(std::ostream& os, const std::string& port_name, const std::vector<std::string>& nets, PortType type, int depth);

std::string create_unconn_net(size_t& unconn_count);
std::string escape_verilog_identifier(const std::string id);
std::string escape_sdf_identifier(const std::string id);
std::string join_identifier(std::string lhs, std::string rhs);

//
//
// File local class and function implementations
//
//

//A vector-like object containing logic values.
//It includes some useful additional operations such as input rotation (rotate()) and
//expansion of Don't Cares into minterm numbers (minterms()) used during while calculating
//LUT masks.
class LogicVec {
    public:
        LogicVec() = default;
        LogicVec(size_t size_val, //Number of logic values
                 LogicVal init_value) //Default value
            : values_(size_val, init_value)
            {}

        //Array indexing operator
        LogicVal& operator[](size_t i) { return values_[i]; }

        //Size accessor
        size_t size() { return values_.size(); }


        //Output operator which writes the logic vector in verilog format
        friend std::ostream& operator<<(std::ostream& os, LogicVec logic_vec) {
            os << logic_vec.values_.size() << "'b";
            //Print in reverse since th convention is MSB on the left, LSB on the right
            //but we store things in array order (putting LSB on left, MSB on right)
            for(auto iter = logic_vec.begin(); iter != logic_vec.end(); iter++) {
                os << *iter;
            }
            return os;
        }

        //Rotates the logic values using permute as a mapping
        //
        //e.g. the new value at index i is the value currently
        //     at index permute[i]
        void rotate(std::vector<int> permute) {
            assert(permute.size() == values_.size());

            auto orig_values = values_;

            for(size_t i = 0; i < values_.size(); i++) {
                values_[i] = orig_values[permute[i]];
            }
        }

        //Returns a vector of minterms (denoted by their number), handling expansion of any
        //don't cares
        std::vector<size_t> minterms() {
            std::vector<size_t> minterms_vec;

            minterms_recur(minterms_vec, *this);

            return minterms_vec;
        }

        //Standard iterators
        std::vector<LogicVal>::reverse_iterator begin() { return values_.rbegin(); }
        std::vector<LogicVal>::reverse_iterator end() { return values_.rend(); }
        std::vector<LogicVal>::const_reverse_iterator begin() const { return values_.crbegin(); }
        std::vector<LogicVal>::const_reverse_iterator end() const { return values_.crend(); }

    private:

        //Recursive helper function to handle minterm expansion in the case of don't
        //cares
        void minterms_recur(std::vector<size_t>& minterms_vec, LogicVec logic_vec) {

            //Find the first don't care
            auto iter = std::find(logic_vec.begin(), logic_vec.end(), LogicVal::DONTCARE);

            if(iter == logic_vec.end()) {
                //Base case (only TRUE/FALSE) caluclate minterm number
                size_t minterm_number = 0;
                for(size_t i = 0; i < values_.size(); i++) {
                    if(logic_vec.values_[i] == LogicVal::TRUE) {
                        size_t index_power = (1 << i);
                        minterm_number += index_power;
                    } else if(logic_vec.values_[i] == LogicVal::FALSE) {
                        //pass
                    } else {
                        assert(false); //Unsupported values
                    }
                }
                minterms_vec.push_back(minterm_number);
            } else {
                //Recurse

                //Expand with Don't Care set true
                *iter = LogicVal::TRUE;
                minterms_recur(minterms_vec, logic_vec);

                //Expand with Don't Care set false
                *iter = LogicVal::FALSE;
                minterms_recur(minterms_vec, logic_vec);
            }
        }

    private:
        std::vector<LogicVal> values_; //The logic values
};

//A combinational timing arc
class Arc {
    public:
        Arc(std::string src_port, //Source of the arc 
            int src_ipin, //Source pin index
            std::string snk_port, //Sink of the arc
            int snk_ipin, //Sink pin index
            float del, //Delay on this arc
            std::string cond="") //Condition associated with the arc
            : source_name_(src_port)
            , source_ipin_(src_ipin)
            , sink_name_(snk_port)
            , sink_ipin_(snk_ipin)
            , delay_(del)
            , condition_(cond)
            {}

        //Accessors
        std::string source_name() const { return source_name_; }
        int source_ipin() const { return source_ipin_; }
        std::string sink_name() const { return sink_name_; }
        int sink_ipin() const { return sink_ipin_; }
        double delay() const { return delay_; }
        std::string condition() const { return condition_; }

    private:
        std::string source_name_;
        int source_ipin_;
        std::string sink_name_;
        int sink_ipin_;
        double delay_;
        std::string condition_;
};

//Instance is an interface used to represent an element insantiated in a netlist
//
//Instances know how to describe themselves in BLIF, Verilog and SDF
//
//This should be subclassed to implement support for new primitive types (although
//see BlackBoxInst for a general implementation for a black box primitive)
class Instance {
    public:
        //Print the current instance in blif format
        //
        //  os - The output stream to print to
        //  unconn_count - The current count of unconnected nets.  BLIF has limitations requiring
        //                 unconnected nets to be used to represent unconnected ports.  To allow
        //                 unique naming of these nets unconn_count is used to uniquify these names.
        //                 Whenever creating an unconnected net (and using unconn_count to uniquify
        //                 its name in the file) unconn_count should be incremented.
        //  depth - Current indentation depth.  This is used to figure-out how much indentation
        //          should be applied.  This is purely for cosmetic formatting.  Use indent() for
        //          generating consistent indentation.
        virtual void print_blif(std::ostream& os, size_t& unconn_count, int depth=0) = 0;

        //Print the current instanse in Verilog, see print_blif() for argument descriptions
        virtual void print_verilog(std::ostream& os, int depth=0) = 0;

        //Print the current instanse in SDF, see print_blif() for argument descriptions
        virtual void print_sdf(std::ostream& os, int depth=0) = 0;
};

//An instance representing a Look-Up Table
class LutInst : public Instance {
    public: //Public methods

        LutInst(size_t lut_size, //The LUT size
                LogicVec lut_mask, //The LUT mask representing the logic function
                std::string inst_name, //The name of this instance
                std::map<std::string,std::vector<std::string>> port_conns, //The port connections of this instance. Key: port name, Value: connected nets 
                std::vector<Arc> timing_arc_values) //The timing arcs of this instance
            : type_("LUT_K")
            , lut_size_(lut_size)
            , lut_mask_(lut_mask)
            , inst_name_(inst_name)
            , port_conns_(port_conns)
            , timing_arcs_(timing_arc_values)
            {
        }

        //Accessors
        const std::vector<Arc>& timing_arcs() { return timing_arcs_; }
        std::string instance_name() { return inst_name_; }
        std::string type() { return type_; }


    public: //Instance interface method implementations

        void print_verilog(std::ostream& os, int depth) override {
            //Instantiate the lut
            os << indent(depth) << type_ << " #(\n";

            os << indent(depth+1) << ".K(" << lut_size_ << "),\n";

            std::stringstream param_ss;
            param_ss << lut_mask_;
            os << indent(depth+1) << ".LUT_MASK(" << param_ss.str() << ")\n";

            os << indent(depth) << ") " << escape_verilog_identifier(inst_name_) << " (\n";

            assert(port_conns_.count("in"));
            assert(port_conns_.count("out"));
            assert(port_conns_.size() == 2);

            print_verilog_port(os, "in", port_conns_["in"], PortType::IN, depth+1);
            os << "," << "\n";
            print_verilog_port(os, "out", port_conns_["out"], PortType::OUT, depth+1);
            os << "\n";

            os << indent(depth) << ");\n\n";
        }

        void print_blif(std::ostream& os, size_t& unconn_count, int depth) override {
            os << indent(depth) << ".names ";

            //Input nets
            for(auto net : port_conns_["in"]) {
                if(net == "") {
                    //Disconnected
                    os << create_unconn_net(unconn_count) << " ";
                } else {
                    os << net << " ";
                }
            }

            assert(port_conns_["out"].size() == 1);

            //Output net
            auto out_net = port_conns_["out"][0];
            if(out_net == "") {
                //Disconnected
                os << create_unconn_net(unconn_count) << " ";
            } else {
                os << out_net << " ";
            }
            os << "\n";

            //Write out the truth table.
            //
            //For simplicity we always output the ON set
            // Using the OFF set for functions that are mostly 'on' 
            // would reduce the size of the output blif, 
            // but would add complexity
            size_t minterms_set = 0;
            for(size_t minterm = 0; minterm < lut_mask_.size(); minterm++) {
                if(lut_mask_[minterm] == LogicVal::TRUE) {
                    //Convert the minterm to a string of bits
                    std::string bit_str = std::bitset<64>(minterm).to_string();

                    //Because BLIF puts the input values in order from LSB (left) to MSB (right), we need
                    //to reverse the string
                    std::reverse(bit_str.begin(), bit_str.end());

                    //Trim to the LUT size
                    std::string input_values(bit_str.begin(), bit_str.begin() + (lut_size_));

                    //Add the row as true
                    os << indent(depth) << input_values << " 1\n";

                    minterms_set++;
                }
            }
            if(minterms_set == 0) {
                //To make ABC happy (i.e. avoid complaints about mismatching cover size and fanin)
                //put in a false value for luts that are always false
                os << std::string(lut_size_, '-') << " 0\n";
            }
        }

        void print_sdf(std::ostream& os, int depth) override {
            os << indent(depth) << "(CELL\n";
            os << indent(depth+1) << "(CELLTYPE \"" << type() << "\")\n";
            os << indent(depth+1) << "(INSTANCE " << escape_sdf_identifier(instance_name()) << ")\n";

            if(!timing_arcs().empty()) {
                os << indent(depth+1) << "(DELAY\n";
                os << indent(depth+2) << "(ABSOLUTE\n";
                
                for(auto& arc : timing_arcs()) {
                    double delay_ps = get_delay_ps(arc.delay());

                    std::stringstream delay_triple;
                    delay_triple << "(" << delay_ps << ":" << delay_ps << ":" << delay_ps << ")";

                    os << indent(depth+3) << "(IOPATH ";
                    //Note we do not escape the last index of multi-bit signals since they are used to
                    //match multi-bit ports
                    os << escape_sdf_identifier(arc.source_name()) << "[" << arc.source_ipin() << "]" << " ";

                    assert(arc.sink_ipin() == 0); //Should only be one output
                    os << escape_sdf_identifier(arc.sink_name()) << " ";
                    os << delay_triple.str() << " " << delay_triple.str() << ")\n";
                }
                os << indent(depth+2) << ")\n";
                os << indent(depth+1) << ")\n";
            }

            os << indent(depth) << ")\n";
            os << indent(depth) << "\n";
        }



    private:
        std::string type_;
        size_t lut_size_;
        LogicVec lut_mask_;
        std::string inst_name_;
        std::map<std::string,std::vector<std::string>> port_conns_;
        std::vector<Arc> timing_arcs_;
};

class LatchInst : public Instance {
    public: //Public types
        //Types of latches (defined by BLIF)
        enum class Type {
            RISING_EDGE,
            FALLING_EDGE,
            ACTIVE_HIGH,
            ACTIVE_LOW,
            ASYNCHRONOUS,
        };
        friend std::ostream& operator<<(std::ostream& os, const Type& type) {
            if     (type == Type::RISING_EDGE)  os << "re";
            else if(type == Type::FALLING_EDGE) os << "fe";
            else if(type == Type::ACTIVE_HIGH)  os << "ah";
            else if(type == Type::ACTIVE_LOW)   os << "al";
            else if(type == Type::ASYNCHRONOUS) os << "as";
            else assert(false);
            return os;
        }
        friend std::istream& operator>>(std::istream& is, Type& type) {
            std::string tok;
            is >> tok;
            if     (tok == "re") type = Type::RISING_EDGE;
            else if(tok == "fe") type = Type::FALLING_EDGE;
            else if(tok == "ah") type = Type::ACTIVE_HIGH;
            else if(tok == "al") type = Type::ACTIVE_LOW;
            else if(tok == "as") type = Type::ASYNCHRONOUS;
            else assert(false);
            return is;
        }
    public:
        LatchInst(std::string inst_name, //Name of this instance
                  std::map<std::string,std::string> port_conns, //Instance's port-to-net connections
                  Type type, //Type of this latch
                  LogicVal init_value, //Initial value of the latch
                  double tcq=std::numeric_limits<double>::quiet_NaN(), //Clock-to-Q delay
                  double tsu=std::numeric_limits<double>::quiet_NaN(), //Setup time
                  double thld=std::numeric_limits<double>::quiet_NaN()) //Hold time
            : instance_name_(inst_name)
            , port_connections_(port_conns)
            , type_(type)
            , initial_value_(init_value)
            , tcq_(tcq)
            , tsu_(tsu)
            , thld_(thld)
            {}

        void print_blif(std::ostream& os, size_t& unconn_count, int depth=0) override {
            os << indent(depth) << ".latch" << " ";

            //Input D port
            auto d_port_iter = port_connections_.find("D");
            assert(d_port_iter != port_connections_.end());
            os << d_port_iter->second << " ";

            //Output Q port
            auto q_port_iter = port_connections_.find("Q");
            assert(q_port_iter != port_connections_.end());
            os << q_port_iter->second << " ";

            //Latch type
            os << type_ << " "; //Type, i.e. rising-edge

            //Control input
            auto control_port_iter = port_connections_.find("clock");
            assert(control_port_iter != port_connections_.end());
            os << control_port_iter->second << " "; //e.g. clock
            os << (int) initial_value_ << " "; //Init value: e.g. 2=don't care
            os << "\n";
        }

        void print_verilog(std::ostream& os, int depth=0) override {
            //Currently assume a standard DFF
            assert(type_ == Type::RISING_EDGE);

            os << indent(depth) << "DFF" << " #(\n";
            os << indent(depth+1) << ".INITIAL_VALUE(";
            if     (initial_value_ == LogicVal::TRUE)     os << "1'b1";
            else if(initial_value_ == LogicVal::FALSE)    os << "1'b0";
            else if(initial_value_ == LogicVal::DONTCARE) os << "1'bx";
            else if(initial_value_ == LogicVal::UNKOWN)   os << "1'bx";
            else assert(false);
            os << ")\n";
            os << indent(depth) << ") " << escape_verilog_identifier(instance_name_) << " (\n";

            for(auto iter = port_connections_.begin(); iter != port_connections_.end(); ++iter) {
                os << indent(depth+1) << "." << iter->first << "(" << escape_verilog_identifier(iter->second) << ")";

                if(iter != --port_connections_.end()) {
                    os << ", ";
                }
                os << "\n";
            }
            os << indent(depth) << ");";
            os << "\n";
        }

        void print_sdf(std::ostream& os, int depth=0) override {
            assert(type_ == Type::RISING_EDGE);

            os << indent(depth) << "(CELL\n";
            os << indent(depth+1) << "(CELLTYPE \"" << "DFF" << "\")\n";
            os << indent(depth+1) << "(INSTANCE " << escape_sdf_identifier(instance_name_) << ")\n";

            //Clock to Q
            if(!std::isnan(tcq_)) {
                os << indent(depth+1) << "(DELAY\n";
                os << indent(depth+2) << "(ABSOLUTE\n";
                    double delay_ps = get_delay_ps(tcq_);

                    std::stringstream delay_triple;
                    delay_triple << "(" << delay_ps << ":" << delay_ps << ":" << delay_ps << ")";

                    os << indent(depth+3) << "(IOPATH " << "(posedge clock) Q " << delay_triple.str() << " " << delay_triple.str() << ")\n";
                os << indent(depth+2) << ")\n";
                os << indent(depth+1) << ")\n";
            }

            //Setup/Hold
            if(!std::isnan(tsu_) || !std::isnan(thld_)) {
                os << indent(depth+1) << "(TIMINGCHECK\n";
                if(!std::isnan(tsu_)) {
                    std::stringstream setup_triple;
                    double setup_ps = get_delay_ps(tsu_);
                    setup_triple << "(" << setup_ps << ":" << setup_ps << ":" << setup_ps << ")";
                    os << indent(depth+2) << "(SETUP D (posedge clock) " << setup_triple.str() << ")\n";
                }
                if(!std::isnan(thld_)) {
                    std::stringstream hold_triple;
                    double hold_ps = get_delay_ps(thld_);
                    hold_triple << "(" << hold_ps << ":" << hold_ps << ":" << hold_ps << ")";
                    os << indent(depth+2) << "(HOLD D (posedge clock) " << hold_triple.str() << ")\n";
                }
            }
            os << indent(depth+1) << ")\n";
            os << indent(depth) << ")\n";
            os << indent(depth) << "\n";
        }
    private:
        std::string instance_name_;
        std::map<std::string,std::string> port_connections_;
        Type type_;
        LogicVal initial_value_;
        double tcq_; //Clock delay + tcq
        double tsu_; //Setup time
        double thld_; //Hold time
};

class BlackBoxInst : public Instance {
    public:
        BlackBoxInst(std::string type_name, //Instance type
                     std::string inst_name, //Instance name
                     std::map<std::string,std::string> params, //Verilog parameters: Dictonary of <param_name,value>
                     std::map<std::string,std::vector<std::string>> input_port_conns, //Port connections: Dictionary of <port,nets>
                     std::map<std::string,std::vector<std::string>> output_port_conns, //Port connections: Dictionary of <port,nets>
                     std::vector<Arc> timing_arcs, //Combinational timing arcs
                     std::map<std::string,double> ports_tsu, //Port setup checks
                     std::map<std::string,double> ports_tcq) //Port clock-to-q delays
            : type_name_(type_name)
            , inst_name_(inst_name)
            , params_(params)
            , input_port_conns_(input_port_conns)
            , output_port_conns_(output_port_conns)
            , timing_arcs_(timing_arcs)
            , ports_tsu_(ports_tsu)
            , ports_tcq_(ports_tcq)
        {}

        void print_blif(std::ostream& os, size_t& unconn_count, int depth=0) override {
            os << indent(depth) << ".subckt " << type_name_ << " \\" << "\n";

            //Input ports
            for(auto iter = input_port_conns_.begin(); iter != input_port_conns_.end(); ++iter) {
                const auto& port_name = iter->first;
                const auto& nets = iter->second;
                print_blif_port(os, unconn_count, port_name, nets, depth+1);

                if(!(iter == --input_port_conns_.end() && output_port_conns_.empty())) {
                    os << " \\";
                }
                os << "\n";
            }

            //Output ports
            for(auto iter = output_port_conns_.begin(); iter != output_port_conns_.end(); ++iter) {
                const auto& port_name = iter->first;
                const auto& nets = iter->second;
                print_blif_port(os, unconn_count, port_name, nets, depth+1);

                if(!(iter == --output_port_conns_.end())) {
                    os << " \\";
                }
                os << "\n";
            }
            os << "\n";
        }

        void print_verilog(std::ostream& os, int depth=0) override {
            //Instance type
            os << indent(depth) << type_name_ << " #(\n";

            //Verilog parameters
            for(auto iter = params_.begin(); iter != params_.end(); ++iter) {
                os << indent(depth+1) << "." << iter->first << "(" << iter->second << ")";
                if(iter != --params_.end()) {
                    os << ",";
                }
                os << "\n";
            }

            //Instance name
            os << indent(depth) << ") " << escape_verilog_identifier(inst_name_) << " (\n";

            //Input Port connections
            for(auto iter = input_port_conns_.begin(); iter != input_port_conns_.end(); ++iter) {
                auto& port_name = iter->first;
                auto& nets = iter->second;
                print_verilog_port(os, port_name, nets, PortType::IN, depth+1);
                if(!(iter == --input_port_conns_.end() && output_port_conns_.empty())) {
                    os << ",";
                }
                os << "\n";
            }

            //Output Port connections
            for(auto iter = output_port_conns_.begin(); iter != output_port_conns_.end(); ++iter) {
                auto& port_name = iter->first;
                auto& nets = iter->second;
                print_verilog_port(os, port_name, nets, PortType::OUT, depth+1);
                if(!(iter == --output_port_conns_.end())) {
                    os << ",";
                }
                os << "\n";
            }
            os << indent(depth) << ");\n";
            os << "\n";
        }

        void print_sdf(std::ostream& os, int depth=0) override {
            os << indent(depth) << "(CELL\n";
            os << indent(depth+1) << "(CELLTYPE \"" << type_name_ << "\")\n";
            os << indent(depth+1) << "(INSTANCE " << escape_sdf_identifier(inst_name_) << ")\n";
            os << indent(depth+1) << "(DELAY\n";

            if(!timing_arcs_.empty() || !ports_tcq_.empty()) {
                os << indent(depth+2) << "(ABSOLUTE\n";

                //Combinational paths
                for(auto arc : timing_arcs_) {
                    double delay_ps = get_delay_ps(arc.delay());

                    std::stringstream delay_triple;
                    delay_triple << "(" << delay_ps << ":" << delay_ps << ":" << delay_ps << ")";

                    //Note that we explicitly do not escape the last array indexing so an SDF
                    //reader will treat the ports as multi-bit
                    //
                    //We also only put the last index in if the port has multiple bits
                    os << indent(depth+3) << "(IOPATH ";
                    os << escape_sdf_identifier(arc.source_name());
                    if(find_port_size(arc.source_name()) > 1) { 
                        os << "[" << arc.source_ipin() << "]";
                    }
                    os << " ";
                    os << escape_sdf_identifier(arc.sink_name());
                    if(find_port_size(arc.sink_name()) > 1) { 
                        os << "[" << arc.sink_ipin() << "]";
                    }
                    os << " ";
                    os << delay_triple.str();
                    os << ")\n";
                }

                //Clock-to-Q delays
                for(auto kv : ports_tcq_) {
                    double clock_to_q_ps = get_delay_ps(kv.second);

                    std::stringstream delay_triple;
                    delay_triple << "(" << clock_to_q_ps << ":" << clock_to_q_ps << ":" << clock_to_q_ps << ")";

                    os << indent(depth+3) << "(IOPATH (posedge clock) " << escape_sdf_identifier(kv.first) << " " << delay_triple.str() << " " << delay_triple.str() << ")\n";
                }
                os << indent(depth+2) << ")\n"; //ABSOLUTE
            }
            os << indent(depth+1) << ")\n"; //DELAY

            if(!ports_tsu_.empty() || !ports_thld_.empty()) {
                //Setup checks
                os << indent(depth+1) << "(TIMINGCHECK\n";
                for(auto kv : ports_tsu_) {
                    double setup_ps = get_delay_ps(kv.second);

                    std::stringstream delay_triple;
                    delay_triple << "(" << setup_ps << ":" << setup_ps << ":" << setup_ps << ")";

                    os << indent(depth+2) << "(SETUP " << escape_sdf_identifier(kv.first) << " (posedge clock) " << delay_triple.str() << ")\n";
                }
                for(auto kv : ports_thld_) {
                    double hold_ps = get_delay_ps(kv.second);

                    std::stringstream delay_triple;
                    delay_triple << "(" << hold_ps << ":" << hold_ps << ":" << hold_ps << ")";

                    os << indent(depth+2) << "(HOLD " << escape_sdf_identifier(kv.first) << " (posedge clock) " << delay_triple.str() << ")\n";
                }
                os << indent(depth+1) << ")\n"; //TIMINGCHECK
            }
            os << indent(depth) << ")\n"; //CELL
        }

        size_t find_port_size(std::string port_name) {
            auto iter = input_port_conns_.find(port_name);
            if(iter != input_port_conns_.end()) {
                return iter->second.size();
            }

            iter = output_port_conns_.find(port_name);
            if(iter != output_port_conns_.end()) {
                return iter->second.size();
            }
            vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                        "Could not find port %s on %s of type %s\n", port_name.c_str(), inst_name_.c_str(), type_name_.c_str());

            return -1; //Suppress warning
        }

    private:
        std::string type_name_;
        std::string inst_name_;
        std::map<std::string,std::string> params_;
        std::map<std::string,std::vector<std::string>> input_port_conns_;
        std::map<std::string,std::vector<std::string>> output_port_conns_;
        std::vector<Arc> timing_arcs_;
        std::map<std::string,double> ports_tsu_;
        std::map<std::string,double> ports_thld_;
        std::map<std::string,double> ports_tcq_;
};

//Assignment represents the logical connection between two nets
//
//  This is synonomous with verilog's 'assign x = y' which connects
//  two nets with logical identity, assigning the value of 'y' to 'x'
class Assignment {
    public:
        Assignment(std::string lval, //The left value (assigned to)
                   std::string rval) //The right value (assigned from)
            : lval_(lval)
            , rval_(rval)
            {}

        void print_verilog(std::ostream& os, std::string indent) {
            os << indent << "assign " << escape_verilog_identifier(lval_) << " = " << escape_verilog_identifier(rval_) << ";\n";
        }
        void print_blif(std::ostream& os, std::string indent) {
            os << indent << ".names " << rval_ << " " << lval_ << "\n";
            os << indent << "1 1\n";
        }
    private:
        std::string lval_;
        std::string rval_;
};


//A class which writes post-synthesis netlists (Verilog and BLIF) and the SDF
//
//It implements the NetlistVisitor interface used by NetlistWalker (see netlist_walker.h)
class NetlistWriterVisitor : public NetlistVisitor {
    public: //Public interface
        NetlistWriterVisitor(std::ostream& verilog_os, //Output stream for verilog netlist
                                std::ostream& blif_os, //Output stream for blif netlist
                                std::ostream& sdf_os) //Output stream for SDF
            : verilog_os_(verilog_os)
            , blif_os_(blif_os)
            , sdf_os_(sdf_os) {
            
            //Need to look-up pins from tnode's
            pin_id_to_tnode_lookup_ = alloc_and_load_tnode_lookup_from_pin_id();
        }

        //Non copyable/assignable/moveable
        NetlistWriterVisitor(NetlistWriterVisitor& other) = delete;
        NetlistWriterVisitor(NetlistWriterVisitor&& other) = delete;
        NetlistWriterVisitor& operator=(NetlistWriterVisitor& rhs) = delete;
        NetlistWriterVisitor& operator=(NetlistWriterVisitor&& rhs) = delete;

        ~NetlistWriterVisitor() {
            free(pin_id_to_tnode_lookup_);
        }

    private: //Internal types
    private: //NetlistVisitor interface functions

        void visit_top_impl(const char* top_level_name) { 
            top_module_name_ = top_level_name;
        }

        void visit_atom_impl(const t_pb* atom) { 
            const t_model* model = logical_block[atom->logical_block].model;

            if(model->name == std::string("input")) {
                inputs_.emplace_back(make_io(atom, PortType::IN));
            } else if(model->name == std::string("output")) {
                outputs_.emplace_back(make_io(atom, PortType::OUT));
            } else if(model->name == std::string("names")) {
                cell_instances_.push_back(make_lut_instance(atom));
            } else if(model->name == std::string("latch")) {
                cell_instances_.push_back(make_latch_instance(atom));
            } else if(model->name == std::string("single_port_ram")) {
                cell_instances_.push_back(make_ram_instance(atom));
            } else if(model->name == std::string("dual_port_ram")) {
                cell_instances_.push_back(make_ram_instance(atom));
            } else if(model->name == std::string("multiply")) {
                cell_instances_.push_back(make_multiply_instance(atom));
            } else if(model->name == std::string("adder")) {
                cell_instances_.push_back(make_adder_instance(atom));
            } else {
                vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                            "Primitive '%s' not recognized by netlist writer\n", model->name);
            }
        }

        void finish_impl() {
            print_verilog();
            print_blif();
            print_sdf();
        }

    private: //Internal Helper functions

        //Writes out the verilog netlist
        void print_verilog(int depth=0) {
            verilog_os_ << indent(depth) << "//Verilog generated by VPR " << BUILD_VERSION << " from post-place-and-route implementation\n";
            verilog_os_ << indent(depth) << "module " << top_module_name_ << " (\n";

            //Primary Inputs
            for(auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
                verilog_os_ << indent(depth+1) << "input " << escape_verilog_identifier(*iter);
                if(iter + 1 != inputs_.end() || outputs_.size() > 0) {
                   verilog_os_ << ",";
                }
                verilog_os_ << "\n";
            }

            //Primary Outputs
            for(auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
                verilog_os_ << indent(depth+1) << "output " << escape_verilog_identifier(*iter);
                if(iter + 1 != outputs_.end()) {
                   verilog_os_ << ",";
                }
                verilog_os_ << "\n";
            }
            verilog_os_ << indent(depth) << ");\n" ;

            //Wire delcarations
            verilog_os_ << "\n";
            verilog_os_ << indent(depth+1) << "//Wires\n";
            for(auto& kv : logical_net_drivers_) {
                verilog_os_ << indent(depth+1) << "wire " << escape_verilog_identifier(kv.second.first) << ";\n";
            }
            for(auto& kv : logical_net_sinks_) {
                for(auto& wire_tnode_pair : kv.second) {
                    verilog_os_ << indent(depth+1) << "wire " << escape_verilog_identifier(wire_tnode_pair.first) << ";\n";
                }
            }

            //connections between primary I/Os and their internal wires
            verilog_os_ << "\n";
            verilog_os_ << indent(depth+1) << "//IO assignments\n";
            for(auto& assign : assignments_) {
                assign.print_verilog(verilog_os_, indent(depth+1));
            }

            //Interconnect between cell instances
            verilog_os_ << "\n";
            verilog_os_ << indent(depth+1) << "//Interconnect\n";
            for(const auto& kv : logical_net_sinks_) {
                int atom_net_idx = kv.first;
                auto driver_iter = logical_net_drivers_.find(atom_net_idx);
                assert(driver_iter != logical_net_drivers_.end());
                const auto& driver_wire = driver_iter->second.first;

                for(auto& sink_wire_tnode_pair : kv.second) {
                    std::string inst_name = interconnect_name(driver_wire, sink_wire_tnode_pair.first);
                    verilog_os_ << indent(depth+1) << "fpga_interconnect " << escape_verilog_identifier(inst_name) << " (\n";
                    verilog_os_ << indent(depth+2) << ".datain(" << escape_verilog_identifier(driver_wire) << "),\n";
                    verilog_os_ << indent(depth+2) << ".dataout(" << escape_verilog_identifier(sink_wire_tnode_pair.first) << ")\n";
                    verilog_os_ << indent(depth+1) << ");\n\n";
                }
            }

            //All the cell instances
            verilog_os_ << "\n";
            verilog_os_ << indent(depth+1) << "//Cell instances\n";
            for(auto& inst : cell_instances_) {
                inst->print_verilog(verilog_os_, depth+1);
            }

            verilog_os_ << "\n";
            verilog_os_ << indent(depth) << "endmodule\n";
        }

        //Writes out the blif netlist
        void print_blif(int depth=0) {
            blif_os_ << indent(depth) << "#BLIF generated by VPR " << BUILD_VERSION << " from post-place-and-route implementation\n";
            blif_os_ << indent(depth) << ".model " << top_module_name_ << "\n";

            //Primary inputs
            blif_os_ << indent(depth) << ".inputs ";
            for(auto iter = inputs_.begin(); iter != inputs_.end(); ++iter) {
                blif_os_ << *iter << " ";
            }
            blif_os_ << "\n";

            //Primary outputs
            blif_os_ << indent(depth) << ".outputs ";
            for(auto iter = outputs_.begin(); iter != outputs_.end(); ++iter) {
                blif_os_ << *iter << " ";
            }
            blif_os_ << "\n";

            //I/O assignments
            blif_os_ << "\n";
            blif_os_ << indent(depth) << "#IO assignments\n";
            for(auto& assign : assignments_) {
                assign.print_blif(blif_os_, indent(depth));
            }

            //All interconnect between elements
            blif_os_ << "\n";
            blif_os_ << indent(depth) << "#Interconnect\n";
            for(const auto& kv : logical_net_sinks_) {
                int atom_net_idx = kv.first;
                auto driver_iter = logical_net_drivers_.find(atom_net_idx);
                assert(driver_iter != logical_net_drivers_.end());
                const auto& driver_wire = driver_iter->second.first;

                for(auto& sink_wire_tnode_pair : kv.second) {
                    blif_os_ << indent(depth) << ".names " << driver_wire << " " << sink_wire_tnode_pair.first << "\n";
                    blif_os_ << indent(depth) << "1 1\n";
                }
            }

            //The cells
            blif_os_ << "\n";
            blif_os_ << indent(depth) << "#Cell instances\n";
            size_t unconn_count = 0;
            for(auto& inst : cell_instances_) {
                inst->print_blif(blif_os_, unconn_count);
            }

            blif_os_ << "\n";
            blif_os_ << indent(depth) << ".end\n";
        }

        //Writes out the SDF
        void print_sdf(int depth=0) {
            sdf_os_ << indent(depth) << "(DELAYFILE\n";
            sdf_os_ << indent(depth+1) << "(SDFVERSION \"2.1\")\n";
            sdf_os_ << indent(depth+1) << "(DESIGN \""<< top_module_name_ << "\")\n";
            sdf_os_ << indent(depth+1) << "(VENDOR \"verilog-to-routing\")\n";
            sdf_os_ << indent(depth+1) << "(PROGRAM \"vpr\")\n";
            sdf_os_ << indent(depth+1) << "(VERSION \"" << BUILD_VERSION << "\")\n";
            sdf_os_ << indent(depth+1) << "(DIVIDER /)\n";
            sdf_os_ << indent(depth+1) << "(TIMESCALE 1 ps)\n";
            sdf_os_ << "\n";

            //Interconnect
            for(const auto& kv : logical_net_sinks_) {
                int atom_net_idx = kv.first;
                auto driver_iter = logical_net_drivers_.find(atom_net_idx);
                assert(driver_iter != logical_net_drivers_.end());
                auto driver_wire = driver_iter->second.first;
                auto driver_tnode = driver_iter->second.second;

                for(auto& sink_wire_tnode_pair : kv.second) {
                    auto sink_wire = sink_wire_tnode_pair.first;
                    auto sink_tnode = sink_wire_tnode_pair.second;

                    sdf_os_ << indent(depth+1) << "(CELL\n";
                    sdf_os_ << indent(depth+2) << "(CELLTYPE \"fpga_interconnect\")\n";
                    sdf_os_ << indent(depth+2) << "(INSTANCE " << escape_sdf_identifier(interconnect_name(driver_wire, sink_wire)) << ")\n";
                    sdf_os_ << indent(depth+2) << "(DELAY\n";
                    sdf_os_ << indent(depth+3) << "(ABSOLUTE\n";

                    double delay = get_delay_ps(driver_tnode, sink_tnode);

                    std::stringstream delay_triple;
                    delay_triple << "(" << delay << ":" << delay << ":" << delay << ")";

                    sdf_os_ << indent(depth+4) << "(IOPATH datain dataout " << delay_triple.str() << " " << delay_triple.str() << ")\n";
                    sdf_os_ << indent(depth+3) << ")\n";
                    sdf_os_ << indent(depth+2) << ")\n";
                    sdf_os_ << indent(depth+1) << ")\n";
                    sdf_os_ << indent(depth) << "\n";
                }
            }

            //Cells
            for(auto inst : cell_instances_) {
                inst->print_sdf(sdf_os_, depth+1);
            }

            sdf_os_ << indent(depth) << ")\n";
        }

        //Returns the name of a wire connecting a primitive and global net.  
        //The wire is recorded and instantiated by the top level output routines.
        std::string make_inst_wire(int atom_net_idx, //The id of the net in the atom netlist
                                   int tnode_id,  //The tnode associated with the primitive pin
                                   std::string inst_name, //The name of the instance associated with the pin
                                   PortType port_type, //The port direction
                                   int port_idx, //The instance port index
                                   int pin_idx) { //The instance pin index

            std::string wire_name = inst_name;
            if(port_type == PortType::IN) {
                wire_name = join_identifier(wire_name, "input");
            } else if(port_type == PortType::CLOCK) {
                wire_name = join_identifier(wire_name, "clock");
            } else {
                assert(port_type == PortType::OUT);
                wire_name = join_identifier(wire_name, "output");
            }

            wire_name = join_identifier(wire_name, std::to_string(port_idx));
            wire_name = join_identifier(wire_name, std::to_string(pin_idx));

            auto value = std::make_pair(wire_name, tnode_id);
            if(port_type == PortType::IN || port_type == PortType::CLOCK) {
                //Add the sink
                logical_net_sinks_[atom_net_idx].push_back(value);

            } else {
                //Add the driver
                assert(port_type == PortType::OUT);

                auto ret = logical_net_drivers_.insert(std::make_pair(atom_net_idx, value));
                assert(ret.second); //Was inserted, drivers are unique
            }

            return wire_name;
        }

        //Returns the name of a circuit-level Input/Output
        //The I/O is recorded and instantiated by the top level output routines
        std::string make_io(const t_pb* atom, //The implementation primitive representing the I/O
                            PortType dir) { //The IO direction
            

            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;

            std::string io_name;
            int cluster_pin_idx = -1;
            if(dir == PortType::IN) {
                assert(pb_graph_node->num_output_ports == 1); //One output port
                assert(pb_graph_node->num_output_pins[0] == 1); //One output pin
                cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
                
                io_name = atom->name;  

            } else {
                assert(pb_graph_node->num_input_ports == 1); //One input port
                assert(pb_graph_node->num_input_pins[0] == 1); //One input pin
                cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster

                //Strip off the starting 'out:' that vpr adds to uniqify outputs
                //this makes the port names match the input blif file
                io_name = atom->name + 4;  
            }

            const t_block* top_block = find_top_block(atom);

            int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

            //Port direction is inverted (inputs drive internal nets, outputs sink internal nets)
            PortType wire_dir = (dir == PortType::IN) ? PortType::OUT : PortType::IN;

            //Look up the tnode associated with this pin (used for delay calculation)
            int tnode_id = find_tnode(atom, cluster_pin_idx);

            auto wire_name = make_inst_wire(atom_net_idx, tnode_id, io_name, wire_dir, 0, 0);

            //Connect the wires to to I/Os with assign statements
            if(wire_dir == PortType::IN) {
                assignments_.emplace_back(io_name, wire_name);
            } else {
                assignments_.emplace_back(wire_name, io_name);
            }
            
            return io_name;
        }

        //Returns an Instance object representing the LUT
        std::shared_ptr<Instance> make_lut_instance(const t_pb* atom)  {
            //Determine what size LUT
            int lut_size = find_num_inputs(atom);

            //Determine the truth table
            auto lut_mask = load_lut_mask(lut_size, atom);

            //Determine the instance name
            auto inst_name = join_identifier("lut", atom->name);

            //Determine the port connections
            std::map<std::string,std::vector<std::string>> port_conns;

            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
            assert(pb_graph_node->num_input_ports == 1); //LUT has one input port

            const t_block* top_block = find_top_block(atom);

            //Add inputs adding connections
            std::vector<Arc> timing_arcs;
            for(int pin_idx = 0; pin_idx < pb_graph_node->num_input_pins[0]; pin_idx++) {
                int cluster_pin_idx = pb_graph_node->input_pins[0][pin_idx].pin_count_in_cluster; //Unique pin index in cluster
                int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

                std::string net;
                if(atom_net_idx == OPEN) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected to a net
                    
                    //Look up the tnode associated with this pin (used for delay calculation)
                    int tnode_id = find_tnode(atom, cluster_pin_idx);

                    net = make_inst_wire(atom_net_idx, tnode_id, inst_name, PortType::IN, 0, pin_idx);

                    //Record the timing arc
                    assert(tnode[tnode_id].num_edges == 1);
                    float delay = tnode[tnode_id].out_edges[0].Tdel;
                    Arc timing_arc("in", pin_idx, "out", 0, delay);

                    timing_arcs.push_back(timing_arc);
                }
                port_conns["in"].push_back(net);
            }

            //Add the single output connection
            {
                assert(pb_graph_node->num_output_ports == 1); //LUT has one output port
                assert(pb_graph_node->num_output_pins[0] == 1); //LUT has one output pin
                int cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
                int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx; //Connected net in atom netlist

                std::string net;
                if(atom_net_idx == OPEN) {
                    //Disconnected
                    net = "";
                } else {
                    //Connected to a net

                    //Look up the tnode associated with this pin (used for delay calculation)
                    int tnode_id = find_tnode(atom, cluster_pin_idx);

                    net = make_inst_wire(atom_net_idx, tnode_id, inst_name, PortType::OUT, 0, 0);
                }
                port_conns["out"].push_back(net);
            }

            auto inst = std::make_shared<LutInst>(lut_size, lut_mask, inst_name, port_conns, timing_arcs);

            return inst;
        }

        //Returns an Instance object representing the Latch
        std::shared_ptr<Instance> make_latch_instance(const t_pb* atom)  {
            std::string inst_name = join_identifier("latch", atom->name);

            const t_block* top_block = find_top_block(atom);
            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
             
            //We expect a single input, output and clock ports
            assert(pb_graph_node->num_input_ports == 1);
            assert(pb_graph_node->num_output_ports == 1);
            assert(pb_graph_node->num_clock_ports == 1);
            assert(pb_graph_node->num_input_pins[0] == 1);
            assert(pb_graph_node->num_output_pins[0] == 1);
            assert(pb_graph_node->num_clock_pins[0] == 1);

            //The connections
            std::map<std::string,std::string> port_conns;

            //Input (D)
            int input_cluster_pin_idx = pb_graph_node->input_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
            int input_atom_net_idx = top_block->pb_route[input_cluster_pin_idx].atom_net_idx; //Connected net in atom netlist
            std::string input_net = make_inst_wire(input_atom_net_idx, find_tnode(atom, input_cluster_pin_idx), inst_name, PortType::IN, 0, 0);
            port_conns["D"] = input_net;

            double tsu = pb_graph_node->input_pins[0][0].tsu_tco;

            //Output (Q)
            int output_cluster_pin_idx = pb_graph_node->output_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
            int output_atom_net_idx = top_block->pb_route[output_cluster_pin_idx].atom_net_idx; //Connected net in atom netlist
            std::string output_net = make_inst_wire(output_atom_net_idx, find_tnode(atom, output_cluster_pin_idx), inst_name, PortType::OUT, 0, 0);
            port_conns["Q"] = output_net;

            double tcq = pb_graph_node->output_pins[0][0].tsu_tco;

            //Clock (control)
            int control_cluster_pin_idx = pb_graph_node->clock_pins[0][0].pin_count_in_cluster; //Unique pin index in cluster
            int control_atom_net_idx = top_block->pb_route[control_cluster_pin_idx].atom_net_idx; //Connected net in atom netlist
            std::string control_net = make_inst_wire(control_atom_net_idx, find_tnode(atom, control_cluster_pin_idx), inst_name, PortType::CLOCK, 0, 0);
            port_conns["clock"] = control_net;

            //VPR currently doesn't store enough information to determine these attributes,
            //for now assume reasonable defaults.
            LatchInst::Type type = LatchInst::Type::RISING_EDGE;
            LogicVal init_value = LogicVal::FALSE;

            return std::make_shared<LatchInst>(inst_name, port_conns, type, init_value, tcq, tsu);
        }

        //Returns an Instance object representing the RAM
        // Note that the primtive interface to dual and single port rams is nearly identical,
        // so we using a single function to handle both
        std::shared_ptr<Instance> make_ram_instance(const t_pb* atom)  {
            const t_block* top_block = find_top_block(atom);
            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
            const t_pb_type* pb_type = pb_graph_node->pb_type;

            assert(pb_type->class_type == MEMORY_CLASS);

            std::string type = pb_type->model->name;
            std::string inst_name = join_identifier(type, atom->name);
            std::map<std::string,std::string> params;
            std::map<std::string,std::vector<std::string>> input_port_conns;
            std::map<std::string,std::vector<std::string>> output_port_conns;
            std::vector<Arc> timing_arcs;
            std::map<std::string,double> ports_tsu;
            std::map<std::string,double> ports_tcq;

            params["ADDR_WIDTH"] = "0";
            params["DATA_WIDTH"] = "0";

            //Process the input ports
            for(int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {

                for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                    const t_port* port = pin->port; 
                    std::string port_class = port->port_class;

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected
                        net = "";
                    } else {
                        //Connected
                        net = make_inst_wire(atom_net_idx, find_tnode(atom, cluster_pin_idx), inst_name, PortType::IN, iport, ipin);
                    }


                    //RAMs use a port class specification to identify the purpose of each port
                    std::string port_name;
                    if(port_class == "address") {
                        port_name = "addr";
                        params["ADDR_WIDTH"] = std::to_string(port->num_pins);
                    } else if(port_class == "address1") {
                        port_name = "addr1";
                        params["ADDR_WIDTH"] = std::to_string(port->num_pins);
                    } else if(port_class == "address2") {
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
                        vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                    }

                    input_port_conns[port_name].push_back(net);
                    ports_tsu[port_name] = pin->tsu_tco;
                }
            }

            //Process the output ports
            for(int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {

                for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                    const t_port* port = pin->port; 
                    std::string port_class = port->port_class;

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected
                        net = "";
                    } else {
                        net = make_inst_wire(atom_net_idx, find_tnode(atom, cluster_pin_idx), inst_name, PortType::OUT, iport, ipin);
                    }

                    std::string port_name;
                    if(port_class == "data_out") {
                        port_name = "out";
                    } else if(port_class == "data_out1") {
                        port_name = "out1";
                    } else if(port_class == "data_out2") {
                        port_name = "out2";
                    } else {
                        vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                    }
                    output_port_conns[port_name].push_back(net);
                    ports_tcq[port_name] = pin->tsu_tco;
                }
            }

            //Process the clock ports
            for(int iport = 0; iport < pb_graph_node->num_clock_ports; ++iport) {
                assert(pb_graph_node->num_clock_pins[iport] == 1); //Expect a single clock port

                for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->clock_pins[iport][ipin];
                    const t_port* port = pin->port; 
                    std::string port_class = port->port_class;

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    assert(atom_net_idx != OPEN); //Must have a clock

                    std::string net = make_inst_wire(atom_net_idx, find_tnode(atom, cluster_pin_idx), inst_name, PortType::CLOCK, iport, ipin);

                    if(port_class == "clock") {
                        assert(pb_graph_node->num_clock_pins[iport] == 1); //Expect a single clock pin
                        input_port_conns["clock"].push_back(net);
                    } else {
                        vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                                    "Unrecognized input port class '%s' for primitive '%s' (%s)\n", port_class.c_str(), atom->name, pb_type->name);
                    }
                }
            }

            return std::make_shared<BlackBoxInst>(type, inst_name, params, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_tcq);
        }

        //Returns an Instance object representing a Multiplier
        std::shared_ptr<Instance> make_multiply_instance(const t_pb* atom)  {
            const t_block* top_block = find_top_block(atom);
            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
            const t_pb_type* pb_type = pb_graph_node->pb_type;

            std::string type_name = pb_type->model->name;
            std::string inst_name = join_identifier(type_name, atom->name);
            std::map<std::string,std::string> params;
            std::map<std::string,std::vector<std::string>> input_port_conns;
            std::map<std::string,std::vector<std::string>> output_port_conns;
            std::vector<Arc> timing_arcs;
            std::map<std::string,double> ports_tsu;
            std::map<std::string,double> ports_tcq;

            params["WIDTH"] = "0";

            //Delay matrix[sink_tnode] -> tuple of source_port_name, pin index, delay
            std::map<int,std::vector<std::tuple<std::string,int,double>>> tnode_delay_matrix;

            //Process the input ports
            for(int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
                for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                    const t_port* port = pin->port; 
                    params["WIDTH"] = std::to_string(port->num_pins); //Assume same width on all ports

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected
                        net = "";
                    } else {
                        //Connected
                        auto inode = find_tnode(atom, cluster_pin_idx);
                        net = make_inst_wire(atom_net_idx, inode, inst_name, PortType::IN, iport, ipin);

                        //Delays
                        //
                        //We record the souce sink tnodes and thier delays here since the timing graph only
                        //has forward edges
                        for(int iedge = 0; iedge < tnode[inode].num_edges; iedge++) {
                            auto& edge = tnode[inode].out_edges[iedge];
                            auto key = edge.to_node;
                            tnode_delay_matrix[key].emplace_back(port->name, ipin, edge.Tdel);
                        }
                    }

                    input_port_conns[port->name].push_back(net);

                }
            }

            //Process the output ports
            for(int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {

                for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                    const t_port* port = pin->port; 

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected
                        net = "";
                    } else {
                        //Connected

                        auto inode = find_tnode(atom, cluster_pin_idx);
                        net = make_inst_wire(atom_net_idx, inode, inst_name, PortType::OUT, iport, ipin);

                        //Record the timing arcs
                        for(auto& data_tuple : tnode_delay_matrix[inode]) {
                            auto src_name = get<0>(data_tuple);
                            auto src_ipin = get<1>(data_tuple);
                            auto delay = get<2>(data_tuple);
                            timing_arcs.emplace_back(src_name, src_ipin, port->name, ipin, delay);
                        }
                    }

                    output_port_conns[port->name].push_back(net);

                }
            }

            assert(pb_graph_node->num_clock_ports == 0); //No clocks

            return std::make_shared<BlackBoxInst>(type_name, inst_name, params, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_tcq);
        }

        //Returns an Instance object representing an Adder
        std::shared_ptr<Instance> make_adder_instance(const t_pb* atom)  {
            const t_block* top_block = find_top_block(atom);
            const t_pb_graph_node* pb_graph_node = atom->pb_graph_node;
            const t_pb_type* pb_type = pb_graph_node->pb_type;

            std::string type_name = pb_type->model->name;
            std::string inst_name = join_identifier(type_name, atom->name);
            std::map<std::string,std::string> params;
            std::map<std::string,std::vector<std::string>> input_port_conns;
            std::map<std::string,std::vector<std::string>> output_port_conns;
            std::vector<Arc> timing_arcs;
            std::map<std::string,double> ports_tsu;
            std::map<std::string,double> ports_tcq;

            params["WIDTH"] = "0";

            //Delay matrix[sink_tnode] -> tuple of source_port_name, pin index, delay
            std::map<int,std::vector<std::tuple<std::string,int,double>>> tnode_delay_matrix;

            //Process the input ports
            for(int iport = 0; iport < pb_graph_node->num_input_ports; ++iport) {
                for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->input_pins[iport][ipin];
                    const t_port* port = pin->port; 

                    if(port->name == std::string("a") || port->name == std::string("b")) {
                        params["WIDTH"] = std::to_string(port->num_pins); //Assume same width on all ports
                    }

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected

                    } else {
                        //Connected
                        auto inode = find_tnode(atom, cluster_pin_idx);
                        net = make_inst_wire(atom_net_idx, inode, inst_name, PortType::IN, iport, ipin);

                        //Delays
                        //
                        //We record the souce sink tnodes and thier delays here since the timing graph only
                        //has forward edges
                        for(int iedge = 0; iedge < tnode[inode].num_edges; iedge++) {
                            auto& edge = tnode[inode].out_edges[iedge];
                            auto key = edge.to_node;
                            tnode_delay_matrix[key].emplace_back(port->name, ipin, edge.Tdel);
                        }
                    }

                    input_port_conns[port->name].push_back(net);

                }
            }

            //Process the output ports
            for(int iport = 0; iport < pb_graph_node->num_output_ports; ++iport) {

                for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ++ipin) {
                    const t_pb_graph_pin* pin = &pb_graph_node->output_pins[iport][ipin];
                    const t_port* port = pin->port; 

                    int cluster_pin_idx = pin->pin_count_in_cluster;
                    int atom_net_idx = top_block->pb_route[cluster_pin_idx].atom_net_idx;

                    std::string net;
                    if(atom_net_idx == OPEN) {
                        //Disconnected
                        net = "";
                    } else {
                        //Connected
                        auto inode = find_tnode(atom, cluster_pin_idx);
                        net = make_inst_wire(atom_net_idx, inode, inst_name, PortType::OUT, iport, ipin);

                        //Record the timing arcs
                        for(auto& data_tuple : tnode_delay_matrix[inode]) {
                            auto src_name = get<0>(data_tuple);
                            auto src_ipin = get<1>(data_tuple);
                            auto delay = get<2>(data_tuple);
                            timing_arcs.emplace_back(src_name, src_ipin, port->name, ipin, delay);
                        }
                    }

                    output_port_conns[port->name].push_back(net);
                }
            }

            return std::make_shared<BlackBoxInst>(type_name, inst_name, params, input_port_conns, output_port_conns, timing_arcs, ports_tsu, ports_tcq);
        }

        //Returns the netlist block associated with the given pb
        const t_block* find_top_block(const t_pb* curr) {
            const t_pb* top_pb = find_top_cb(curr); 

            for(int i = 0; i < num_blocks; i++) {
                if(block[i].pb == top_pb) {
                    return &block[i];
                }
            }
            assert(false);
        }

        //Returns the top complex block which contains the given pb
        const t_pb* find_top_cb(const t_pb* curr) {
            //Walk up through the pb graph until curr
            //has no parent, at which point it will be the top pb
            const t_pb* parent = curr->parent_pb;
            while(parent != nullptr) {
                curr = parent;
                parent = curr->parent_pb;
            }
            return curr;
        }

        //Returns the tnode ID of the given atom's connected cluster pin
        int find_tnode(const t_pb* atom, int cluster_pin_idx) {

            int clb_index = logical_block[atom->logical_block].clb_index;
            int tnode_id = pin_id_to_tnode_lookup_[clb_index][cluster_pin_idx];

            assert(tnode_id != OPEN);

            return tnode_id;
        }

        //Returns a LogicVec representing the LUT mask of the given LUT atom
        LogicVec load_lut_mask(size_t num_inputs, //LUT size
                              const t_pb* atom) { //LUT primitive
            const t_model* model = logical_block[atom->logical_block].model;
            assert(model->name == std::string("names"));

#ifdef DEBUG_LUT_MASK
            std::cout << "Loading LUT mask for: " << atom->name << std::endl;
#endif

            //Figure out how the inputs were permuted (compared to the input netlist)
            std::vector<int> permute = determine_lut_permutation(num_inputs, atom);

            //VPR stores the truth table as in BLIF
            //Each row of the table (i.e. a c-string) is stored in a linked list 
            //
            //The c-string is the literal row from BLIF, e.g. "0 1" for an inverter, "11 1" for an AND2
            t_linked_vptr* names_row_ptr = logical_block[atom->logical_block].truth_table;
            
            //Determine the truth (output value) for this .names
            bool encoding_on_set = names_encodes_on_set(names_row_ptr);

            //Initialize LUT mask
            int lut_bits = std::pow(2, num_inputs);
            LogicVec lut_mask;
            if(encoding_on_set) {
                //Encoding the ON set, so the 'background' value for unspecified
                //minterms is FALSE
                lut_mask = LogicVec(lut_bits, LogicVal::FALSE); 
            } else {
                //Encoding the OFF set, so the 'background' value for unspecified
                //minterms is TRUE
                lut_mask = LogicVec(lut_bits, LogicVal::TRUE); 
            }

            //Process each row of the .names truth table
            while(names_row_ptr != nullptr) {

                //Get the input values defining this row of the truth table
                LogicVec input_values = names_row_to_logic_vec(static_cast<const char*>(names_row_ptr->data_vptr), num_inputs, encoding_on_set);

                //Apply any LUT input rotations
                auto permuted_input_values = input_values;
                permuted_input_values.rotate(permute);

#ifdef DEBUG_LUT_MASK
                std::cout << "\t" << names_row << " = "<< input_values << ":" << encoding_on_set;

                std::cout << " -> " << permuted_input_values << ":" << encoding_on_set<< std::endl;
#endif

                //Get the minterm numbers representing the logic function, while
                //expand any don't cares in the input values
                for(size_t minterm : permuted_input_values.minterms()) {
#ifdef DEBUG_LUT_MASK
                    std::cout << "\tSetting minterm : " << minterm << " to " << encoding_on_set << std::endl;
#endif
                    //Set the appropraite lut mask entry
                    lut_mask[minterm] = (encoding_on_set) ? LogicVal::TRUE : LogicVal::FALSE;
                }
                
                //Advance to the next row
                names_row_ptr = names_row_ptr->next;
            }

#ifdef DEBUG_LUT_MASK
            std::cout << "\tLUT_MASK: " << lut_mask << std::endl;
#endif

            return lut_mask;
        }

        //Helper function for load_lut_mask() which determines how the LUT inputs were
        //permuted compared to the input BLIF
        //
        //  Since the LUT inputs may have been rotated from the input blif specification we need to
        //  figure out this permutation to reflect the physical implementation connectivity.
        //  
        //  We return a permutation map (which is a list of swaps from index to index)
        //  which is then applied to do the rotation of the lutmask
        std::vector<int> determine_lut_permutation(size_t num_inputs, const t_pb* atom) {
            std::vector<int> permute(num_inputs, OPEN);

#ifdef DEBUG_LUT_MASK
            std::cout << "\tInit Permute: {";
            for(size_t i = 0; i < permute.size(); i++) {
                std::cout << permute[i] << ", ";
            }
            std::cout << "}" << std::endl;
#endif

            //Determine the permutation
            //
            //We walk through the logical inputs to this atom (i.e. in the original truth table/netlist)
            //and find the corresponding input in the implementation atom (i.e. in the current netlist)
            for(size_t i = 0; i < num_inputs; i++) {
                int logical_net = logical_block[atom->logical_block].input_nets[0][i]; //The original net in the input netlist

                if(logical_net != OPEN) {
                    int atom_input_net = OPEN;
                    size_t j;
                    for(j = 0; j < num_inputs; j++) {
                        atom_input_net = find_atom_input_logical_net(atom, j); //The net currently connected to input j

                        if(logical_net == atom_input_net) {
#ifdef DEBUG_LUT_MASK
                            std::cout << "\tLogic net " << logical_net << " (" << g_atoms_nlist.net[logical_net].name << ") "
                            std::cout << "atom lut input " << i << " -> impl lut input " << j << std::endl;
#endif
                            break;
                        }
                    }
                    assert(atom_input_net == logical_net);

                    permute[j] = i;
                }
            }

            //Fill in any missing values in the permutation (i.e. zeros)
            std::set<int> perm_indicies(permute.begin(), permute.end());
            size_t unused_index = 0;
            for(size_t i = 0; i < permute.size(); i++) {
                if(permute[i] == OPEN) {
                    while(perm_indicies.count(unused_index)) {
                        unused_index++;
                    }
                    permute[i] = unused_index;
                    perm_indicies.insert(unused_index);
                }
            }

#ifdef DEBUG_LUT_MASK
            std::cout << "\tPermute: {";
            for(size_t k = 0; k < permute.size(); k++) {
                std::cout << permute[k] << ", ";
            }
            std::cout << "}" << std::endl;


            std::cout << "\tBLIF = Input ->  Rotated" << std::endl;
            std::cout << "\t------------------------" << std::endl;
#endif
            return permute;
        }

        //Helper function for load_lut_mask() which determines if the
        //names is encodeing the ON (returns true) or OFF (returns false)
        //set.
        bool names_encodes_on_set(t_linked_vptr* names_row_ptr) {
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
            if(names_row_ptr) {
                //Determine whether the truth table stores the ON or OFF set
                //
                //  In blif, the 'output values' of a .names must be either '1' or '0', and must be consistent
                //  within a single .names -- that is a single .names can encode either the ON or OFF set 
                //  (of which only one will be encoded in a single .names)
                //
                const std::string names_first_row = (const char*) names_row_ptr->data_vptr;
                auto names_first_row_output_iter = names_first_row.end() - 1;

                if(*names_first_row_output_iter == '1') {
                    encoding_on_set = true; 
                } else if (*names_first_row_output_iter == '0') {
                    encoding_on_set = false; 
                } else {
                    vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                                "Invalid .names truth-table character '%c'. Must be one of '1', '0' or '-'. \n", *names_first_row_output_iter);
                }
            }

            return encoding_on_set;
        }

        //Helper function for load_lut_mask()
        //
        //Converts the given names_row string to a LogicVec
        LogicVec names_row_to_logic_vec(const std::string names_row, size_t num_inputs, bool encoding_on_set) {
            //Get an iterator to the last character (i.e. the output value)
            auto output_val_iter = names_row.end() - 1;

            //Sanity-check, the 2nd last character should be a space
            auto space_iter = names_row.end() - 2;
            assert(*space_iter == ' ');
            
            //Extract the truth (output value) for this row
            if(*output_val_iter == '1') {
                assert(encoding_on_set);
            } else if (*output_val_iter == '0') {
                assert(!encoding_on_set);
            } else {
                vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                            "Invalid .names encoding both ON and OFF set\n");
            }

            //Extract the input values for this row
            LogicVec input_values(num_inputs, LogicVal::FALSE);
            size_t i = 0;
            //Walk through each input in the input cube for this row
            while(names_row[i] != ' ') {
                LogicVal input_val = LogicVal::UNKOWN;
                if(names_row[i] == '1') {
                    input_val = LogicVal::TRUE; 
                } else if (names_row[i] == '0') {
                    input_val = LogicVal::FALSE; 
                } else if (names_row[i] == '-') {
                    input_val = LogicVal::DONTCARE; 
                } else {
                    vpr_throw(VPR_ERROR_IMPL_NETLIST_WRITER, __FILE__, __LINE__,
                                "Invalid .names truth-table character '%c'. Must be one of '1', '0' or '-'. \n", names_row[i]);
                }

                input_values[i] =  input_val;
                i++;
            }
            return input_values;
        }


        //Returns the total number of input pins on the given pb
        int find_num_inputs(const t_pb* pb) {
            int count = 0;
            for(int i = 0; i < pb->pb_graph_node->num_input_ports; i++) {
                count += pb->pb_graph_node->num_input_pins[i];
            }
            return count;
        }

        //Returns the logical net ID 
        int find_atom_input_logical_net(const t_pb* atom, int atom_input_idx) {
            const t_pb_graph_node* pb_node = atom->pb_graph_node;

            int cluster_pin_idx = pb_node->input_pins[0][atom_input_idx].pin_count_in_cluster;
            const t_block* top_clb = find_top_block(atom);
            int atom_net_idx = top_clb->pb_route[cluster_pin_idx].atom_net_idx;
            return atom_net_idx;
        }


        //Returns the name of the routing segment between two wires
        std::string interconnect_name(std::string driver_wire, std::string sink_wire) {
            std::string name = join_identifier("routing_segment", driver_wire);
            name = join_identifier(name, "to");
            name = join_identifier(name, sink_wire);

            return name;
        }


    private: //Data
        std::string top_module_name_; //Name of the top level module (i.e. the circuit)
        std::vector<std::string> inputs_; //Name of circuit inputs
        std::vector<std::string> outputs_; //Name of circuit outputs
        std::vector<Assignment> assignments_; //Set of assignments (i.e. net-to-net connections)
        std::vector<std::shared_ptr<Instance>> cell_instances_; //Set of cell instances

        //Drivers of logical nets. 
        // Key: logic net id, Value: pair of wire_name and tnode_id
        std::map<int, std::pair<std::string,int>> logical_net_drivers_; 

        //Sinks of logical nets. 
        // Key: logical net id, Value: vector wire_name tnode_id pairs
        std::map<int, std::vector<std::pair<std::string,int>>> logical_net_sinks_; 
        std::map<std::string, float> logical_net_sink_delays_;

        //Look-up from pins to tnodes
        int** pin_id_to_tnode_lookup_;

        //Output streams
        std::ostream& verilog_os_;
        std::ostream& blif_os_;
        std::ostream& sdf_os_;
};

//
// Externally Accessible Functions
//

//Main routing for this file. See netlist_writer.h for details.
void netlist_writer(const std::string basename) {
    std::string verilog_filename = basename+ "_post_synthesis.v";
    std::string blif_filename = basename + "_post_synthesis.blif";
    std::string sdf_filename = basename + "_post_synthesis.sdf";

    vpr_printf_info("Writting Implementation Netlist: %s\n", verilog_filename.c_str());
    vpr_printf_info("Writting Implementation Netlist: %s\n", blif_filename.c_str());
    vpr_printf_info("Writting Implementation SDF    : %s\n", sdf_filename.c_str());

    std::ofstream verilog_os(verilog_filename);
    std::ofstream blif_os(blif_filename);
    std::ofstream sdf_os(sdf_filename);

    NetlistWriterVisitor visitor(verilog_os, blif_os, sdf_os);

    NetlistWalker nl_walker(visitor);

    nl_walker.walk();
}

//
// File-scope function implementations
//

//Output operator for LogicVal
std::ostream& operator<<(std::ostream& os, LogicVal val) {
    if(val == LogicVal::FALSE) os << "0";
    else if (val == LogicVal::TRUE) os << "1";
    else if (val == LogicVal::DONTCARE) os << "-";
    else if (val == LogicVal::UNKOWN) os << "x";
    else if (val == LogicVal::HIGHZ) os << "z";
    else assert(false);
    return os;
}


//Returns a blank string for indenting the given depth
std::string indent(size_t depth) {
    std::string indent_ = "    ";
    std::string new_indent;
    for(size_t i = 0; i < depth; ++i) {
        new_indent += indent_;
    }
    return new_indent;
}

//Returns the delay in pico-seconds from a floating point delay
double get_delay_ps(double delay_sec) {
    return delay_sec * 1e12; //Scale to picoseconds
}

//Returns the delay in pico-seconds from source_tnode to sink_tnode
double get_delay_ps(int source_tnode, int sink_tnode) {
    float source_delay = tnode[source_tnode].T_arr;
    float sink_delay = tnode[sink_tnode].T_arr;

    float delay_sec = sink_delay - source_delay;

    return get_delay_ps(delay_sec);
}

//Returns the name of a unique unconnected net
std::string create_unconn_net(size_t& unconn_count) {
    //We increment unconn_count by reference so each
    //call generates a unique name
    return "__vpr__unconn" + std::to_string(unconn_count++);
}

//Pretty-Prints a blif port to the given output stream
//
//  Handles special cases like multi-bit and disconnected ports
void print_blif_port(std::ostream& os, size_t& unconn_count, const std::string& port_name, const std::vector<std::string>& nets, int depth) {
    if(nets.size() == 1) {
        //If only a single bit port, don't include port indexing
        os << indent(depth) << port_name << "=";
        if(nets[0].empty()) {
            os << create_unconn_net(unconn_count);
        } else {
            os << nets[0];
        }
    } else {
        //Include bit indexing for multi-bit ports
        for(int ipin = 0; ipin < (int) nets.size(); ++ipin) {
            os << indent(depth) << port_name << "[" << ipin << "]" << "=";

            if(nets[ipin].empty()) {
                os << create_unconn_net(unconn_count);
            } else {
                os << nets[ipin];
            }
            if(ipin != (int) nets.size() - 1) {
                os << " " << "\\" << "\n";
            }
        }
    }
}

//Pretty-Prints a verilog port to the given output stream
//
//  Handles special cases like multi-bit and disconnected ports
void print_verilog_port(std::ostream& os, const std::string& port_name, const std::vector<std::string>& nets, PortType type, int depth) {
    //Port name
    os << indent(depth) << "." << port_name << "(";

    //Pins
    if(nets.size() == 1) {
        //Single-bit port
        if(nets[0].empty()) {
            //Disconnected
            if(type == PortType::IN || type == PortType::CLOCK) {
                os << "1'b0";
            } else {
                assert(type == PortType::OUT);
                os << "";
            }
        } else {
            //Connected
            os << escape_verilog_identifier(nets[0]);
        }
    } else {
        //A multi-bit port, we explicitly concat the single-bit nets to build the port,
        //taking care to print MSB on left and LSB on right
        os << "{" << "\n";
        for(int ipin = (int) nets.size() - 1; ipin >= 0; --ipin) { //Reverse order to match endianess
            os << indent(depth+1);
            if(nets[ipin].empty()) {
                //Disconnected
                if(type == PortType::IN || type == PortType::CLOCK) {
                    os << "1'b0";
                } else {
                    assert(type == PortType::OUT);
                    os << "";
                }
            } else {
                //Connected
                os << escape_verilog_identifier(nets[ipin]) ;
            }
            if(ipin != 0) {
                os << ",";
                os << "\n";
            }
        }
        os << "}";
    }
    os << ")";
}

//Escapes the given identifier to be safe for verilog
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

//Returns true if c is categorized as a special character in SDF
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
    //Not that the spec defines _ (decimal code 95) and $ (decimal code 36) 
    //as non-special alphanumeric characters. 
    //
    //However it inconsistently also lists $ in the list of special characters.
    //Since the spec allows for non-special characters to be escaped (they are treated
    //normally), we treat $ as a special character to be safe.
    //
    //Note that the spec appears to have rendering errors in the PDF availble
    //on IEEE Xplore, listing the 'LEFT-POINTING DOUBLE ANGLE QUOTATION MARK' 
    //character (decimal code 171) in place of the APOSTROPHE character ' 
    //with decimal code 39 in the special character list. We assume code 39
    if((c >= 33 && c <= 35) ||
       (c == 36) || // $
       (c >= 37 && c <= 47) ||
       (c >= 58 && c <= 64) ||
       (c >= 91 && c <= 94) ||
       (c == 96) ||
       (c >= 123 && c <= 126)) {
        return true;
    }

    return false;
}

//Escapes the given identifier to be safe for sdf
std::string escape_sdf_identifier(const std::string identifier) {
    //SDF allows escaped characters
    //
    //We look at each character in the string and escape it if it is
    //a special character
    std::string escaped_name;

    for(char c : identifier) {
        if(is_special_sdf_char(c)) {
            //Escape the special character
            escaped_name += '\\';
        }
        escaped_name += c;
    }

    return escaped_name;
}

//Joins two identifier strings
std::string join_identifier(std::string lhs, std::string rhs) {
    return lhs + '_' + rhs;
}
