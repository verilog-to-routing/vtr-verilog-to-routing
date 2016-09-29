#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
using namespace std;

#include "blifparse.hpp"
#include "netlist2.h"
#include "netlist2_utils.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_list.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "read_blif.h"
#include "arch_types.h"
#include "ReadOptions.h"
#include "hash.h"

/* PRINT_PIN_NETS */

struct s_model_stats {
	const t_model * model;
	int count;
};

#define MAX_ATOM_PARSE 200000000

/* This source file will read in a FLAT blif netlist consisting     *
 * of .inputs, .outputs, .names and .latch commands.  It currently   *
 * does not handle hierarchical blif files.  Hierarchical            *
 * blif files can be flattened via the read_blif and write_blif      *
 * commands of sis.  LUT circuits should only have .names commands;  *
 * there should be no gates.  This parser performs limited error     *
 * checking concerning the consistency of the netlist it obtains.    *
 * .inputs and .outputs statements must be given; this parser does   *
 * not infer primary inputs and outputs from non-driven and fanout   *
 * free nodes.  This parser can be extended to do this if necessary, *
 * or the sis read_blif and write_blif commands can be used to put a *
 * netlist into the standard format.                                 *
 * V. Betz, August 25, 1994.                                         *
 * Added more error checking, March 30, 1995, V. Betz                */

static int *num_driver, *temp_num_pins;
static int *logical_block_input_count, *logical_block_output_count;
static int num_blif_models;
static int num_luts = 0, num_latches = 0, num_subckts = 0;

/* # of .input, .output, .model and .end lines */
static int ilines, olines, model_lines, endlines;
static struct s_hash **blif_hash;
static char *model = NULL;
static FILE *blif;

static int add_vpack_net(char *ptr, int type, int bnum, int bport, int bpin,
		bool is_global, bool doall);
static void get_blif_tok(char *buffer, bool doall, bool *done,
		bool *add_truth_table, const t_model* inpad_model,
		const t_model* outpad_model, const t_model* logic_model,
		const t_model* latch_model, const t_model* user_models);
static void init_parse(bool doall, bool init_vpack_net_power);
static t_model_ports* find_clock_port(const t_model* model);
static int check_blocks();
static int check_net(bool sweep_hanging_nets_and_inputs);
static void free_parse(void);
static void io_line(int in_or_out, bool doall, const t_model *io_model);
static bool add_lut(bool doall, const t_model *logic_model);
static void add_latch(bool doall, const t_model *latch_model);
static void add_subckt(bool doall, const t_model *user_models);
static void check_and_count_models(bool doall, const char* model_name,
		const t_model* user_models);
static void load_default_models(const t_model *library_models,
		const t_model** inpad_model, const t_model** outpad_model,
		const t_model** logic_model, const t_model** latch_model);
static void read_activity(char * activity_file);
static void read_blif(const char *blif_file, bool sweep_hanging_nets_and_inputs,
		const t_model *user_models, const t_model *library_models,
		bool read_activity_file, char * activity_file);

static void read_blif2(const char *blif_file, bool sweep_hanging_nets_and_inputs,
		const t_model *user_models, const t_model *library_models,
		bool read_activity_file, char * activity_file);

static void do_absorb_buffer_luts(void);
static void compress_netlist(void);
static void show_blif_stats(const t_model *user_models, const t_model *library_models);
static bool add_activity_to_net(char * net_name, float probability,
		float density);

void blif_error(int lineno, std::string near_text, std::string msg);

void blif_error(int lineno, std::string near_text, std::string msg) {
    vpr_throw(VPR_ERROR_BLIF_F, "", lineno,
            "Error in blif file near '%s': %s\n", near_text.c_str(), msg.c_str());
}

vtr::LogicValue to_vtr_logic_value(blifparse::LogicValue);

struct BlifCountCallback : public blifparse::Callback {
    public:
        void start_model(std::string /*model_name*/) override { ++num_models; }
        void inputs(std::vector<std::string> /*inputs*/) override { ++num_inputs; }
        void outputs(std::vector<std::string> /*outputs*/) override { ++num_outputs; }

        void names(std::vector<std::string> /*nets*/, std::vector<std::vector<blifparse::LogicValue>> /*so_cover*/) override { ++num_names; }
        void latch(std::string /*input*/, std::string /*output*/, blifparse::LatchType /*type*/, std::string /*control*/, blifparse::LogicValue /*init*/) override { ++num_latches; }
        void subckt(std::string /*model*/, std::vector<std::string> /*ports*/, std::vector<std::string> /*nets*/) override { ++num_subckts; }
        void blackbox() override {}

        void end_model() override {}

        void filename(std::string /*fname*/) override {}
        void lineno(int /*line_num*/) override {}

        size_t num_models = 0;
        size_t num_inputs = 0;
        size_t num_outputs = 0;
        size_t num_names = 0;
        size_t num_latches = 0;
        size_t num_subckts = 0;
};

struct BlifAllocCallback : public blifparse::Callback {
    public:
        BlifAllocCallback(const t_model* user_models, const t_model* library_models)
            : user_arch_models_(user_models) 
            , library_arch_models_(library_models) {}

        static constexpr const char* OUTPAD_NAME_PREFIX = "out:";

    public: //Callback interface
        void start_model(std::string model_name) override { 
            //Create a new model, and set it's name
            blif_models_.emplace_back(model_name);
            blif_models_black_box_.emplace_back(false);
            ended_ = false;
        }

        void inputs(std::vector<std::string> input_names) override {
            const t_model* blk_model = find_model("input");

            VTR_ASSERT_MSG(!blk_model->inputs, "Inpad model has an input port");
            VTR_ASSERT_MSG(blk_model->outputs, "Inpad model has no output port");
            VTR_ASSERT_MSG(blk_model->outputs->size == 1, "Inpad model has non-single-bit output port");
            VTR_ASSERT_MSG(!blk_model->outputs->next, "Inpad model has multiple output ports");

            std::string pin_name = blk_model->outputs->name;
            for(const auto& input : input_names) {
                AtomBlockId blk_id = curr_model().create_block(input, AtomBlockType::INPAD, blk_model);
                AtomPortId port_id = curr_model().create_port(blk_id, blk_model->outputs->name);
                AtomNetId net_id = curr_model().create_net(input);
                curr_model().create_pin(port_id, 0, net_id, AtomPinType::DRIVER);
            }
        }

        void outputs(std::vector<std::string> output_names) override { 
            const t_model* blk_model = find_model("output");

            VTR_ASSERT_MSG(!blk_model->outputs, "Outpad model has an output port");
            VTR_ASSERT_MSG(blk_model->inputs, "Outpad model has no input port");
            VTR_ASSERT_MSG(blk_model->inputs->size == 1, "Outpad model has non-single-bit input port");
            VTR_ASSERT_MSG(!blk_model->inputs->next, "Outpad model has multiple input ports");

            std::string pin_name = blk_model->inputs->name;
            for(const auto& output : output_names) {
                //Since we name blocks based on thier drivers we need to uniquify outpad names,
                //which we do with a prefix
                AtomBlockId blk_id = curr_model().create_block(OUTPAD_NAME_PREFIX + output, AtomBlockType::OUTPAD, blk_model);
                AtomPortId port_id = curr_model().create_port(blk_id, blk_model->inputs->name);
                AtomNetId net_id = curr_model().create_net(output);
                curr_model().create_pin(port_id, 0, net_id, AtomPinType::SINK);
            }
        }

        void names(std::vector<std::string> nets, std::vector<std::vector<blifparse::LogicValue>> so_cover) override { 
            const t_model* blk_model = find_model("names");

            VTR_ASSERT_MSG(nets.size() > 0, "BLIF .names has no connections");
            
            VTR_ASSERT_MSG(blk_model->inputs, ".names model has no input port");
            VTR_ASSERT_MSG(!blk_model->inputs->next, ".names model has multiple input ports");
            VTR_ASSERT_MSG(blk_model->inputs->size >= static_cast<int>(nets.size()) - 1, ".names model does not match blif .names input size");

            VTR_ASSERT_MSG(blk_model->outputs, ".names has no output port");
            VTR_ASSERT_MSG(!blk_model->outputs->next, ".names model has multiple output ports");
            VTR_ASSERT_MSG(blk_model->outputs->size == 1, ".names model has non-single-bit output");

            //Convert the single-output cover to a netlist truth table
            AtomNetlist::TruthTable truth_table;
            for(const auto& row : so_cover) {
                truth_table.emplace_back();
                for(auto val : row) {
                    truth_table[truth_table.size()-1].push_back(to_vtr_logic_value(val));
                }
            }

            AtomBlockId blk_id = curr_model().create_block(nets[nets.size()-1], AtomBlockType::COMBINATIONAL, blk_model, truth_table);

            //Create inputs
            AtomPortId input_port_id = curr_model().create_port(blk_id, blk_model->inputs->name);
            for(size_t i = 0; i < nets.size() - 1; ++i) {
                AtomNetId net_id = curr_model().create_net(nets[i]);

                curr_model().create_pin(input_port_id, i, net_id, AtomPinType::SINK);
            }

            //Create output
            AtomNetId net_id = curr_model().create_net(nets[nets.size()-1]);
            AtomPortId output_port_id = curr_model().create_port(blk_id, blk_model->outputs->name);
            curr_model().create_pin(output_port_id, 0, net_id, AtomPinType::DRIVER);
        }

        void latch(std::string input, std::string output, blifparse::LatchType type, std::string control, blifparse::LogicValue init) override {
            if(type == blifparse::LatchType::UNSPECIFIED) {
                vtr::printf_warning(filename_.c_str(), lineno_, "Treating latch '%s' of unspecified type as rising edge triggered\n", output.c_str());
            } else if(type != blifparse::LatchType::RISING_EDGE) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Only rising edge latches supported\n");
            }
            
            const t_model* blk_model = find_model("latch");

            VTR_ASSERT_MSG(blk_model->inputs, "Has one input port");
            VTR_ASSERT_MSG(blk_model->inputs->next, "Has two input port");
            VTR_ASSERT_MSG(!blk_model->inputs->next->next, "Has no more than two input port");
            VTR_ASSERT_MSG(blk_model->outputs, "Has one output port");
            VTR_ASSERT_MSG(!blk_model->outputs->next, "Has no more than one input port");

            const t_model_ports* d_model_port = blk_model->inputs;
            const t_model_ports* clk_model_port = blk_model->inputs->next;
            const t_model_ports* q_model_port = blk_model->outputs;

            VTR_ASSERT(d_model_port->name == std::string("D"));
            VTR_ASSERT(clk_model_port->name == std::string("clk"));
            VTR_ASSERT(q_model_port->name == std::string("Q"));
            VTR_ASSERT(d_model_port->size == 1);
            VTR_ASSERT(clk_model_port->size == 1);
            VTR_ASSERT(q_model_port->size == 1);
            VTR_ASSERT(clk_model_port->is_clock);

            //We set the initital value as a single entry in the 'truth_table' field
            AtomNetlist::TruthTable truth_table(1);
            truth_table[0].push_back(to_vtr_logic_value(init));

            AtomBlockId blk_id = curr_model().create_block(output, AtomBlockType::SEQUENTIAL, blk_model, truth_table);

            //The input
            AtomPortId d_port_id = curr_model().create_port(blk_id, d_model_port->name);
            AtomNetId d_net_id = curr_model().create_net(input);
            curr_model().create_pin(d_port_id, 0, d_net_id, AtomPinType::SINK);

            //The output
            AtomPortId q_port_id = curr_model().create_port(blk_id, q_model_port->name);
            AtomNetId q_net_id = curr_model().create_net(output);
            curr_model().create_pin(q_port_id, 0, q_net_id, AtomPinType::DRIVER);

            //The clock
            AtomPortId clk_port_id = curr_model().create_port(blk_id, clk_model_port->name);
            AtomNetId clk_net_id = curr_model().create_net(control);
            curr_model().create_pin(clk_port_id, 0, clk_net_id, AtomPinType::SINK);
        }

        void subckt(std::string subckt_model, std::vector<std::string> ports, std::vector<std::string> nets) override {
            VTR_ASSERT(ports.size() == nets.size());

            const t_model* blk_model = find_model(subckt_model);

            std::string first_output_name;
            for(size_t i = 0; i < ports.size(); ++i) {
                const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
                VTR_ASSERT(model_port);

                //Determine the pin type
                if(model_port->dir == OUT_PORT) {
                    first_output_name = nets[i];
                    break;
                }
            }
            //We must have an output in-order to name the subckt
            // Also intuitively the subckt can be swept if it has no outside effect
            if(first_output_name.empty()) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Found no output pin on .subckt '%s'",
                          subckt_model.c_str());
            }


            AtomBlockType blk_type = determine_block_type(blk_model);

            AtomBlockId blk_id = curr_model().create_block(first_output_name, blk_type, blk_model);


            for(size_t i = 0; i < ports.size(); ++i) {
                //Check for consistency between model and ports
                const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
                VTR_ASSERT(model_port);

                //Determine the pin type
                AtomPinType pin_type = AtomPinType::SINK;
                if(model_port->dir == OUT_PORT) {
                    pin_type = AtomPinType::DRIVER;
                } else {
                    VTR_ASSERT_MSG(model_port->dir == IN_PORT, "Unexpected port type");
                }

                //Make the port
                std::string port_base;
                size_t port_bit;
                std::tie(port_base, port_bit) = split_index(ports[i]);

                AtomPortId port_id = curr_model().create_port(blk_id, port_base);

                //Make the net
                AtomNetId net_id = curr_model().create_net(nets[i]);

                //Make the pin
                curr_model().create_pin(port_id, port_bit, net_id, pin_type);
            }
        }

        void blackbox() override {
            //We treat black-boxes as netlists during parsing so they should contain
            //only inpads/outpads
            for(const auto& blk_id : curr_model().blocks()) {
                auto blk_type = curr_model().block_type(blk_id);
                if(!(blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD)) {
                    vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Unexpected primitives in blackbox model");
                }
            }
            set_curr_model_blackbox(true);
        }

        void end_model() override {
            if(ended_) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Unexpected .end");
            }
            ended_ = true;
        }

        void filename(std::string fname) override { filename_ = fname; }

        void lineno(int line_num) override { lineno_ = line_num; }
    public:
        //Retrieve the netlist
        AtomNetlist netlist() { 
            //Look through all the models loaded, to find the one which is non-blackbox (i.e. has real blocks
            //and is not a blackbox)
            int top_model_idx = -1; //Not valid

            for(int i = 0; i < static_cast<int>(blif_models_.size()); ++i) {
                if(!blif_models_black_box_[i]) {
                    //A non-blackbox model
                    if(top_model_idx == -1) {
                        //This is the top model
                        top_model_idx = i;
                    } else {
                        //We already have a top model
                        vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, 
                                "Found multiple models with primitives. "
                                "Only one model can contain primitives, the others must be blackboxes.");
                    }
                } else {
                    //Verify blackbox models match the architecture
                    verify_blackbox_model(blif_models_[i]);
                }
            }

            if(top_model_idx == -1) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, 
                        "No non-blackbox models found. The main model must not be a blackbox.");
            } else {
                //Return the main model
                return blif_models_[top_model_idx];
            }
        }

    private:
        const t_model* find_model(std::string name) {
            const t_model* arch_model = nullptr;
            for(const t_model* arch_models : {user_arch_models_, library_arch_models_}) {
                arch_model = arch_models;
                while(arch_model) {
                    if(name == arch_model->name) {
                        //Found it
                        break;
                    }
                    arch_model = arch_model->next;
                }
                if(arch_model) {
                    //Found it
                    break;
                }
            }
            if(!arch_model) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Failed to find matching architecture model for '%s'\n",
                          name.c_str());
            }
            return arch_model;
        }

        AtomBlockType determine_block_type(const t_model* blk_model) {
            if(blk_model->name == std::string("input")) {
                return AtomBlockType::INPAD;
            } else if(blk_model->name == std::string("output")) {
                return AtomBlockType::OUTPAD;
            } else {
                //Determine if combinational or sequential.
                // We loop through the inputs looking for clocks
                const t_model_ports* port = blk_model->inputs;
                size_t clk_count = 0;

                while(port) {
                    if(port->is_clock) {
                        ++clk_count;
                    }
                    port = port->next;
                }

                if(clk_count == 0) {
                    return AtomBlockType::COMBINATIONAL;
                } else if (clk_count == 1) {
                    return AtomBlockType::SEQUENTIAL;
                } else {
                    VTR_ASSERT(clk_count > 1);
                    vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Primitive '%s' has multiple clocks (currently unsupported)\n",
                              blk_model->name);
                }
            }
            VTR_ASSERT(false);
        }

        const t_model_ports* find_model_port(const t_model* blk_model, std::string port_name) {
            //We need to handle both single, and multi-bit port names
            //
            //By convention multi-bit port names have the bit index stored in square brackets
            //at the end of the name. For example:
            //
            //   my_signal_name[2]
            //
            //indicates the 2nd bit of the port 'my_signal_name'.
            std::string trimmed_port_name;
            int bit_index;

            //Extract the index bit
            std::tie(trimmed_port_name, bit_index) = split_index(port_name);
             
            //We now have the valid bit index and port_name is the name excluding the index info
            VTR_ASSERT(bit_index >= 0);

            //We now look through all the ports on the model looking for the matching port
            for(const t_model_ports* ports : {blk_model->inputs, blk_model->outputs}) {

                const t_model_ports* curr_port = ports;
                while(curr_port) {
                    if(trimmed_port_name == curr_port->name) {
                        //Found a matching port, we need to verify the index
                        if(bit_index < curr_port->size) {
                            //Valid port index
                            return curr_port;
                        } else {
                            //Out of range
                            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, 
                                     "Port '%s' on architecture model '%s' exceeds port width (%d bits)\n",
                                      port_name.c_str(), blk_model->name, curr_port->size);
                        }
                    }
                    curr_port = curr_port->next;
                }
            }

            //No match
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, 
                     "Found no matching port '%s' on architecture model '%s'\n",
                      port_name.c_str(), blk_model->name);
        }

        //Splits the index off a signal name and returns the base signal name (excluding
        //the index) and the index as an integer. For example
        //
        //  "my_signal_name[2]"   -> "my_signal_name", 2
        std::pair<std::string, int> split_index(const std::string& signal_name) {
            int bit_index = 0;

            std::string trimmed_signal_name = signal_name;

            auto iter = --signal_name.end(); //Initialized to the last char
            if(*iter == ']') {
                //The name may end in an index
                //
                //To verify, we iterate back through the string until we find
                //an open square brackets, or non digit character
                --iter; //Advance past ']'
                while(iter != signal_name.begin() && std::isdigit(*iter)) --iter;

                //We are at the first non-digit character from the end (or the beginning of the string)
                if(*iter == '[') {
                    //We have a valid index in the open range (iter, --signal_name.end())
                    std::string index_str(iter+1, --signal_name.end());

                    //Convert to an integer
                    std::stringstream ss(index_str);
                    ss >> bit_index;
                    VTR_ASSERT_MSG(!ss.fail() && ss.eof(), "Failed to extract signal index");

                    //Trim the signal name to exclude the final index
                    trimmed_signal_name = std::string(signal_name.begin(), iter);
                }
            }
            return std::make_pair(trimmed_signal_name, bit_index);
        }

        //Retieves a reference to the currently active .model
        AtomNetlist& curr_model() { 
            if(blif_models_.empty() || ended_) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Expected .model");
            }

            return blif_models_[blif_models_.size()-1]; 
        }

        void set_curr_model_blackbox(bool val) {
            VTR_ASSERT(blif_models_.size() == blif_models_black_box_.size());
            blif_models_black_box_[blif_models_black_box_.size()-1] = val;
        }

        bool verify_blackbox_model(AtomNetlist& blif_model) {
            const t_model* arch_model = find_model(blif_model.netlist_name());

            //Verify each port on the model
            //
            // We parse each .model as it's own netlist so the IOs
            // get converted to blocks
            for(auto blk_id : blif_model.blocks()) {


                //Check that the port directions match
                if(blif_model.block_type(blk_id) == AtomBlockType::INPAD) {

                    const auto& input_name = blif_model.block_name(blk_id);

                    //Find model port already verifies the port widths
                    const t_model_ports* arch_model_port = find_model_port(arch_model, input_name);
                    VTR_ASSERT(arch_model_port);
                    VTR_ASSERT(arch_model_port->dir == IN_PORT);

                } else {
                    VTR_ASSERT(blif_model.block_type(blk_id) == AtomBlockType::OUTPAD);

                    auto raw_output_name = blif_model.block_name(blk_id);

                    std::string output_name = vtr::replace_first(raw_output_name, OUTPAD_NAME_PREFIX, "");

                    //Find model port already verifies the port widths
                    const t_model_ports* arch_model_port = find_model_port(arch_model, output_name);
                    VTR_ASSERT(arch_model_port);

                    VTR_ASSERT(arch_model_port->dir == OUT_PORT);
                }
            }
            return true;
        }

    private:
        bool ended_ = true; //Initially no active .model
        std::string filename_;
        int lineno_;

        std::vector<AtomNetlist> blif_models_;
        std::vector<bool> blif_models_black_box_;

        const t_model* user_arch_models_;
        const t_model* library_arch_models_;
};


vtr::LogicValue to_vtr_logic_value(blifparse::LogicValue val) {
    vtr::LogicValue new_val;
    switch(val) {
        case blifparse::LogicValue::TRUE: new_val = vtr::LogicValue::TRUE; break;
        case blifparse::LogicValue::FALSE: new_val = vtr::LogicValue::FALSE; break;
        case blifparse::LogicValue::DONT_CARE: new_val = vtr::LogicValue::DONT_CARE; break;
        case blifparse::LogicValue::UNKOWN: new_val = vtr::LogicValue::UNKOWN; break;
        default: VTR_ASSERT_MSG(false, "Unkown logic value");
    }
    return new_val;
}

static void read_blif2(const char *blif_file, bool sweep_hanging_nets_and_inputs,
		const t_model *user_models, const t_model *library_models,
		bool read_activity_file, char * activity_file) {

/*
 *    FILE* blif_f = vtr::fopen(blif_file, "r");
 *    if (blif_f == NULL) {
 *        vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
 *                "Failed to open blif file '%s'.\n", blif_file);
 *    }
 *
 */
    AtomNetlist netlist;
    {
        vtr::ScopedPrintTimer t1("Load BLIF");

        //Throw VPR errors instead of using libblifparse default error
        blifparse::set_blif_error_handler(blif_error);

        BlifAllocCallback alloc_callback(user_models, library_models);
        blifparse::blif_parse_filename(blif_file, alloc_callback);

        netlist = alloc_callback.netlist();
    }

    {
        vtr::ScopedPrintTimer t2("Verify BLIF");
        netlist.verify();
    }

    {
        vtr::ScopedPrintTimer t2("Clean BLIF");
        
        //Clean-up lut buffers
        absorb_buffer_luts(netlist);

        //Remove the special 'unconn' net
        AtomNetId unconn_net_id = netlist.find_net("unconn");
        if(unconn_net_id) {
            netlist.remove_net(unconn_net_id);
        }

        //Sweep unused logic/nets/inputs/outputs
        sweep_iterative(netlist, true);
    }

    {
        vtr::ScopedPrintTimer t2("Compress BLIF");

        //Compress the netlist to clean-out invalid entries
        netlist.compress();
    }
    {
        vtr::ScopedPrintTimer t2("Verify BLIF");

        netlist.verify();
    }

    {
        vtr::ScopedPrintTimer t2("Print BLIF");
        print_netlist(stdout, netlist);
    }
    std::exit(1);
}

static void read_blif(const char *blif_file, bool sweep_hanging_nets_and_inputs,
		const t_model *user_models, const t_model *library_models,
		bool read_activity_file, char * activity_file) {
	char buffer[vtr::BUFSIZE];
	bool done;
	bool add_truth_table;
	const t_model *inpad_model, *outpad_model, *logic_model, *latch_model;
    int error_count = 0;

	blif = vtr::fopen(blif_file, "r");
	if (blif == NULL) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"Failed to open blif file '%s'.\n", blif_file);
	}
	load_default_models(library_models, &inpad_model, &outpad_model,
			&logic_model, &latch_model);

	/* doall = false means do a counting pass, doall = true means allocate and load data structures */
#if 0
    for (bool doall : { false, true }) { // not supported before g++ 4.6
#else
	for (int doall_int = 0; doall_int <= 1; doall_int++) {
		bool doall = bool(doall_int);
#endif
		init_parse(doall, read_activity_file);
		done = false;
		add_truth_table = false;
		model_lines = 0;
		while (vtr::fgets(buffer, vtr::BUFSIZE, blif) != NULL) {
			get_blif_tok(buffer, doall, &done, &add_truth_table, inpad_model,
					outpad_model, logic_model, latch_model, user_models);
		}
		rewind(blif); /* Start at beginning of file again */
	}

	/*checks how well the hash function is performing*/
#ifdef VERBOSE
	get_hash_stats(blif_hash, "blif_hash");
#endif

	fclose(blif);

    /* Check the netlist data structures for errors */
    error_count += check_blocks();
	error_count += check_net(sweep_hanging_nets_and_inputs);
	if (error_count != 0) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"Found %d fatal errors in the input netlist.\n", error_count);
	}


	/* Read activity file */
	if (read_activity_file) {
		read_activity(activity_file);
	}
	free_parse();
}

static void init_parse(bool doall, bool init_vpack_net_power) {

	/* Allocates and initializes the data structures needed for the parse. */

	int i;
	struct s_hash *h_ptr;

	if (!doall) { /* Initialization before first (counting) pass */
		num_logical_nets = 0;
		blif_hash = (struct s_hash **) vtr::calloc(sizeof(struct s_hash *),
		HASHSIZE);
	}
	/* Allocate memory for second (load) pass */
	else {
		vpack_net = (struct s_net *) vtr::calloc(num_logical_nets,
				sizeof(struct s_net));
		if (init_vpack_net_power) {
			vpack_net_power = (t_net_power *) vtr::calloc(num_logical_nets,
					sizeof(t_net_power));
		}
		logical_block = (struct s_logical_block *) vtr::calloc(num_logical_blocks,
				sizeof(struct s_logical_block));
		num_driver = (int *) vtr::malloc(num_logical_nets * sizeof(int));
		temp_num_pins = (int *) vtr::malloc(num_logical_nets * sizeof(int));

		logical_block_input_count = (int *) vtr::calloc(num_logical_blocks,
				sizeof(int));
		logical_block_output_count = (int *) vtr::calloc(num_logical_blocks,
				sizeof(int));

		for (i = 0; i < num_logical_nets; i++) {
			num_driver[i] = 0;
			vpack_net[i].num_sinks = 0;
			vpack_net[i].name = NULL;
			vpack_net[i].node_block = NULL;
			vpack_net[i].node_block_port = NULL;
			vpack_net[i].node_block_pin = NULL;
			vpack_net[i].is_routed = false;
			vpack_net[i].is_fixed = false;
			vpack_net[i].is_global = false;
		}

		for (i = 0; i < num_logical_blocks; i++) {
			logical_block[i].index = i;
		}

		for (i = 0; i < HASHSIZE; i++) {
			h_ptr = blif_hash[i];
			while (h_ptr != NULL) {
				vpack_net[h_ptr->index].node_block = (int *) vtr::malloc(
						h_ptr->count * sizeof(int));
				vpack_net[h_ptr->index].node_block_port = (int *) vtr::malloc(
						h_ptr->count * sizeof(int));
				vpack_net[h_ptr->index].node_block_pin = (int *) vtr::malloc(
						h_ptr->count * sizeof(int));

				/* For avoiding assigning values beyond end of pins array. */
				temp_num_pins[h_ptr->index] = h_ptr->count;
				vpack_net[h_ptr->index].name = vtr::strdup(h_ptr->name);
				h_ptr = h_ptr->next;
			}
		}
#ifdef PRINT_PIN_NETS
		vtr::printf_info("i\ttemp_num_pins\n");
		for (i = 0;i < num_logical_nets;i++) {
			vtr::printf_info("%d\t%d\n",i,temp_num_pins[i]);
		}
		vtr::printf_info("num_logical_nets %d\n", num_logical_nets);
#endif
	}

	/* Initializations for both passes. */

	ilines = 0;
	olines = 0;
	model_lines = 0;
	endlines = 0;
	num_p_inputs = 0;
	num_p_outputs = 0;
	num_luts = 0;
	num_latches = 0;
	num_logical_blocks = 0;
	num_blif_models = 0;
	num_subckts = 0;
}

static void get_blif_tok(char *buffer, bool doall, bool *done,
		bool *add_truth_table, const t_model* inpad_model,
		const t_model* outpad_model, const t_model* logic_model,
		const t_model* latch_model, const t_model* user_models) {

	/* Figures out which, if any token is at the start of this line and *
	 * takes the appropriate action.                                    */

#define BLIF_TOKENS " \t\n"
	char *ptr;
	char *fn;
	vtr::t_linked_vptr *data;

	ptr = vtr::strtok(buffer, TOKENS, blif, buffer);
	if (ptr == NULL)
		return;

	if (*add_truth_table) {
		if (ptr[0] == '0' || ptr[0] == '1' || ptr[0] == '-') {
			data = (vtr::t_linked_vptr*) vtr::malloc(
					sizeof(vtr::t_linked_vptr));
			fn = ptr;
			ptr = vtr::strtok(NULL, BLIF_TOKENS, blif, buffer);
			if (!ptr || strlen(ptr) != 1) {
				if (strlen(fn) == 1) {
					/* constant generator */
					data->next =
							logical_block[num_logical_blocks - 1].truth_table;
					data->data_vptr = vtr::malloc(strlen(fn) + 4);
					sprintf((char*) (data->data_vptr), " %s", fn);
					logical_block[num_logical_blocks - 1].truth_table = data;
					ptr = fn;
				} else {
					vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
							"Unknown truth table data %s %s.\n", fn, ptr);
				}
			} else {
				data->next = logical_block[num_logical_blocks - 1].truth_table;
				data->data_vptr = vtr::malloc(strlen(fn) + 3);
				sprintf((char*) data->data_vptr, "%s %s", fn, ptr);
				logical_block[num_logical_blocks - 1].truth_table = data;
			}
		}
	}

	if (strcmp(ptr, ".names") == 0) {
		*add_truth_table = false;
		*add_truth_table = add_lut(doall, logic_model);
		return;
	}

	if (strcmp(ptr, ".latch") == 0) {
		*add_truth_table = false;
		add_latch(doall, latch_model);
		return;
	}

	if (strcmp(ptr, ".model") == 0) {
		*add_truth_table = false;
		ptr = vtr::strtok(NULL, TOKENS, blif, buffer);
		if (doall) {
			if (ptr != NULL) {
				if (model != NULL) {
					free(model);
				}
				model = (char *) vtr::malloc((strlen(ptr) + 1) * sizeof(char));
				strcpy(model, ptr);
				if (blif_circuit_name == NULL) {
					blif_circuit_name = vtr::strdup(model);
				}
			} else {
				if (model != NULL) {
					free(model);
				}
				model = (char *) vtr::malloc(sizeof(char));
				model[0] = '\0';
			}
		}

		if (model_lines > 0 && ptr != NULL) {
			check_and_count_models(doall, ptr, user_models);
		} else {
			dum_parse(buffer);
		}
		model_lines++;
		return;
	}

	if (strcmp(ptr, ".inputs") == 0) {
		*add_truth_table = false;
		/* packing can only one fully defined model */
		if (model_lines == 1) {
			io_line(DRIVER, doall, inpad_model);
			*done = true;
		}
		if (doall)
			ilines++; /* Error checking only */
		return;
	}

	if (strcmp(ptr, ".outputs") == 0) {
		*add_truth_table = false;
		/* packing can only one fully defined model */
		if (model_lines == 1) {
			io_line(RECEIVER, doall, outpad_model);
			*done = true;
		}
		if (doall)
			olines++; /* Make sure only one .output line */
		/* For error checking only */
		return;
	}
	if (strcmp(ptr, ".end") == 0) {
		*add_truth_table = false;
		if (doall) {
			endlines++; /* Error checking only */
		}
		return;
	}

	if (strcmp(ptr, ".subckt") == 0) {
		*add_truth_table = false;
		add_subckt(doall, user_models);
	}

	/* Could have numbers following a .names command, so not matching any *
	 * of the tokens above is not an error.                               */

}

void dum_parse(char *buf) {

	/* Continue parsing to the end of this (possibly continued) line. */

	while (vtr::strtok(NULL, TOKENS, blif, buf) != NULL)
		;
}

static bool add_lut(bool doall, const t_model *logic_model) {

	/* Adds a LUT as VPACK_COMB from (.names) currently being parsed to the logical_block array.  Adds *
	 * its pins to the nets data structure by calling add_vpack_net.  If doall is *
	 * zero this is a counting pass; if it is 1 this is the final (loading) *
	 * pass.                                                                */

	char *ptr, **saved_names, buf[vtr::BUFSIZE];
	int i, j, output_net_index;

	saved_names = vtr::alloc_matrix<char>(0, logic_model->inputs->size, 0, vtr::BUFSIZE - 1);

	num_logical_blocks++;

	/* Count # nets connecting */
	i = 0;
	while ((ptr = vtr::strtok(NULL, TOKENS, blif, buf)) != NULL) {
		if (i > logic_model->inputs->size) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"[LINE %d] .names %s ... %s has a LUT size that exceeds the maximum LUT size (%d) of the architecture.\n",
					vtr::get_file_line_number_of_last_opened_file(), saved_names[0], ptr,
					logic_model->inputs->size);
		}
		strcpy(saved_names[i], ptr);
		i++;
	}
	output_net_index = i - 1;
	if (strcmp(saved_names[output_net_index], "unconn") == 0) {
		/* unconn is a keyword to pad unused pins, ignore this block */
        vtr::free_matrix(saved_names, 0, logic_model->inputs->size, 0);
		num_logical_blocks--;
		return false;
	}

	if (!doall) { /* Counting pass only ... */
		for (j = 0; j <= output_net_index; j++)
			/* On this pass it doesn't matter if RECEIVER or DRIVER.  Just checking if in hash.  [0] should be DRIVER */
			add_vpack_net(saved_names[j], RECEIVER, num_logical_blocks - 1, 0,
					j, false, doall);
        vtr::free_matrix(saved_names, 0, logic_model->inputs->size, 0);
		return false;
	}

	logical_block[num_logical_blocks - 1].model = logic_model;

	if (output_net_index > logic_model->inputs->size) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"LUT size of %d in .blif file is too big for FPGA which has a maximum LUT size of %d.\n",
				output_net_index, logic_model->inputs->size);
	}
	VTR_ASSERT(logic_model->inputs->next == NULL);
	VTR_ASSERT(logic_model->outputs->next == NULL);
	VTR_ASSERT(logic_model->outputs->size == 1);

	logical_block[num_logical_blocks - 1].input_nets = (int **) vtr::malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].output_nets = (int **) vtr::malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].clock_net = OPEN;

	logical_block[num_logical_blocks - 1].input_nets[0] = (int *) vtr::malloc(
			logic_model->inputs->size * sizeof(int));
	logical_block[num_logical_blocks - 1].output_nets[0] = (int *) vtr::malloc(
			sizeof(int));

	logical_block[num_logical_blocks - 1].type = VPACK_COMB;
	for (i = 0; i < output_net_index; i++) /* Do inputs */
		logical_block[num_logical_blocks - 1].input_nets[0][i] = add_vpack_net(
				saved_names[i], RECEIVER, num_logical_blocks - 1, 0, i, false,
				doall);
	logical_block[num_logical_blocks - 1].output_nets[0][0] = add_vpack_net(
			saved_names[output_net_index], DRIVER, num_logical_blocks - 1, 0, 0,
			false, doall);

	for (i = output_net_index; i < logic_model->inputs->size; i++)
		logical_block[num_logical_blocks - 1].input_nets[0][i] = OPEN;

	logical_block[num_logical_blocks - 1].name = vtr::strdup(
			saved_names[output_net_index]);
	logical_block[num_logical_blocks - 1].truth_table = NULL;
	num_luts++;

    vtr::free_matrix(saved_names, 0, logic_model->inputs->size, 0);
	return doall;
}

static void add_latch(bool doall, const t_model *latch_model) {

	/* Adds the flipflop (.latch) currently being parsed to the logical_block array.  *
	 * Adds its pins to the nets data structure by calling add_vpack_net.  If doall *
	 * is zero this is a counting pass; if it is 1 this is the final          * 
	 * (loading) pass.  Blif format for a latch is:                           *
	 * .latch <input> <output> <type (latch on)> <control (clock)> <init_val> *
	 * The latch pins are in .nets 0 to 2 in the order: Q D CLOCK.            */

	char *ptr, buf[vtr::BUFSIZE], saved_names[6][vtr::BUFSIZE];
	int i;

	num_logical_blocks++;

	/* Count # parameters, making sure we don't go over 6 (avoids memory corr.) */
	/* Note that we can't rely on the tokens being around unless we copy them.  */

	for (i = 0; i < 6; i++) {
		ptr = vtr::strtok(NULL, TOKENS, blif, buf);
		if (ptr == NULL)
			break;
        if(strlen(ptr) + 1 < vtr::BUFSIZE) {
            strcpy(saved_names[i], ptr);
        } else {
            vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
                    ".latch parameter exceeded buffer length of %zu characters.\n",
                    vtr::BUFSIZE);

        }
	}

	if (i != 5) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				".latch does not have 5 parameters.\n"
						"Check netlist, line %d.\n", vtr::get_file_line_number_of_last_opened_file());
	}

	if (!doall) { /* If only a counting pass ... */
		add_vpack_net(saved_names[0], RECEIVER, num_logical_blocks - 1, 0, 0,
				false, doall); /* D */
		add_vpack_net(saved_names[1], DRIVER, num_logical_blocks - 1, 0, 0,
				false, doall); /* Q */
		add_vpack_net(saved_names[3], RECEIVER, num_logical_blocks - 1, 0, 0,
				true, doall); /* Clock */
		return;
	}

	logical_block[num_logical_blocks - 1].model = latch_model;
	logical_block[num_logical_blocks - 1].type = VPACK_LATCH;

	logical_block[num_logical_blocks - 1].input_nets = (int **) vtr::malloc(
			sizeof(int*));
	logical_block[num_logical_blocks - 1].output_nets = (int **) vtr::malloc(
			sizeof(int*));

	logical_block[num_logical_blocks - 1].input_nets[0] = (int *) vtr::malloc(
			sizeof(int));
	logical_block[num_logical_blocks - 1].output_nets[0] = (int *) vtr::malloc(
			sizeof(int));

	logical_block[num_logical_blocks - 1].output_nets[0][0] = add_vpack_net(
			saved_names[1], DRIVER, num_logical_blocks - 1, 0, 0, false, doall); /* Q */
	logical_block[num_logical_blocks - 1].input_nets[0][0] = add_vpack_net(
			saved_names[0], RECEIVER, num_logical_blocks - 1, 0, 0, false,
			doall); /* D */
	logical_block[num_logical_blocks - 1].clock_net = add_vpack_net(
			saved_names[3], RECEIVER, num_logical_blocks - 1, 0, 0, true,
			doall); /* Clock */

	logical_block[num_logical_blocks - 1].name = vtr::strdup(saved_names[1]);
	logical_block[num_logical_blocks - 1].truth_table = NULL;
	num_latches++;
}

static void add_subckt(bool doall, const t_model *user_models) {
	char *ptr;
	char *close_bracket;
	char subckt_name[vtr::BUFSIZE];
	char buf[vtr::BUFSIZE];
	//fpos_t current_subckt_pos;
	int i, j, iparse;
	int subckt_index_signals = 0;
	char **subckt_signal_name = NULL;
	char *port_name, *pin_number;
	char **circuit_signal_name = NULL;
	char *subckt_logical_block_name = NULL;
	short toggle = 0;
	int input_net_count, output_net_count, input_port_count, output_port_count;
	const t_model *cur_model;
	t_model_ports *port;
	bool found_subckt_signal;

	num_logical_blocks++;
	num_subckts++;

	/* now we have to find the matching subckt */
	/* find the name we are looking for */
    ptr = vtr::strtok(NULL, TOKENS, blif, buf);
    if(strlen(ptr) + 1 <= vtr::BUFSIZE) {
        strcpy(subckt_name, ptr);
    } else {
        vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
                "subckt name exceeded buffer size of %zu characters",
                vtr::BUFSIZE);
    }
	/* get all the signals in the form z=r */
	iparse = 0;
	while (iparse < MAX_ATOM_PARSE) {
		iparse++;
		/* Assumption is that it will be "signal1, =, signal1b, spacing, and repeat" */
		ptr = vtr::strtok(NULL, " \t\n=", blif, buf);

		if (ptr == NULL && toggle == 0)
			break;
		else if (ptr == NULL && toggle == 1) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"subckt %s formed incorrectly with signal=signal at %s.\n",
					subckt_name, buf);
		} else if (toggle == 0) {
			/* ELSE - parse in one or the other */
			/* allocate a new spot for both the circuit_signal name and the subckt_signal name */
			subckt_signal_name = (char**) vtr::realloc(subckt_signal_name,
					(subckt_index_signals + 1) * sizeof(char*));
			circuit_signal_name = (char**) vtr::realloc(circuit_signal_name,
					(subckt_index_signals + 1) * sizeof(char*));

			/* copy in the subckt_signal name */
			subckt_signal_name[subckt_index_signals] = vtr::strdup(ptr);

			toggle = 1;
		} else if (toggle == 1) {
			/* copy in the circuit_signal name */
			circuit_signal_name[subckt_index_signals] = vtr::strdup(ptr);
			if (!doall) {
				/* Counting pass, does not matter if driver or receiver and pin number does not matter */
				add_vpack_net(circuit_signal_name[subckt_index_signals],
						RECEIVER, num_logical_blocks - 1, 0, 0, false, doall);
			}

			toggle = 0;
			subckt_index_signals++;
		}
	}
	VTR_ASSERT(iparse < MAX_ATOM_PARSE);
	/* record the position of the parse so far so when we resume we will move to the next item */
	//if (fgetpos(blif, &current_subckt_pos) != 0) {
	//	vtr::printf_error(__FILE__, __LINE__, "In file pointer read - read_blif.c\n");
	//	exit(-1);
	//}
	input_net_count = 0;
	output_net_count = 0;

	if (doall) {
		/* get the matching model to this subckt */

		cur_model = user_models;
		while (cur_model != NULL) {
			if (strcmp(cur_model->name, subckt_name) == 0) {
				break;
			}
			cur_model = cur_model->next;
		}
		if (cur_model == NULL) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"Did not find matching model to subckt %s.\n", subckt_name);
		}

		/* IF - do all then we need to allocate a string to hold all the subckt info */

		/* initialize the logical_block structure */

		/* record model info */
		logical_block[num_logical_blocks - 1].model = cur_model;

		/* allocate space for inputs and initialize all input nets to OPEN */
		input_port_count = 0;
		port = cur_model->inputs;
		while (port) {
			if (!port->is_clock) {
				input_port_count++;
			}
			port = port->next;
		}
		logical_block[num_logical_blocks - 1].input_nets = (int**) vtr::malloc(
				input_port_count * sizeof(int *));
		logical_block[num_logical_blocks - 1].input_pin_names = (char***)vtr::calloc(input_port_count, sizeof(char **));

		port = cur_model->inputs;
		while (port) {
			if (port->is_clock) {
				/* Clock ports are different from regular input ports, skip */
				port = port->next;
				continue;
			}
			VTR_ASSERT(port->size >= 0);
			logical_block[num_logical_blocks - 1].input_nets[port->index] =
					(int*) vtr::malloc(port->size * sizeof(int));
			logical_block[num_logical_blocks - 1].input_pin_names[port->index] = (char**)vtr::calloc(port->size, sizeof(char *));

			for (j = 0; j < port->size; j++) {
				logical_block[num_logical_blocks - 1].input_nets[port->index][j] =
						OPEN;
			}
			port = port->next;
		}
		VTR_ASSERT(port == NULL || (port->is_clock && port->next == NULL));

		/* allocate space for outputs and initialize all output nets to OPEN */
		output_port_count = 0;
		port = cur_model->outputs;
		while (port) {
			port = port->next;
			output_port_count++;
		}
		logical_block[num_logical_blocks - 1].output_nets = (int**) vtr::malloc(
				output_port_count * sizeof(int *));

		logical_block[num_logical_blocks - 1].output_pin_names = (char***)vtr::calloc(output_port_count, sizeof(char **));

		port = cur_model->outputs;
		while (port) {
			VTR_ASSERT(port->size >= 0);
			logical_block[num_logical_blocks - 1].output_nets[port->index] =
					(int*) vtr::malloc(port->size * sizeof(int));
			logical_block[num_logical_blocks - 1].output_pin_names[port->index] = (char**)vtr::calloc(port->size, sizeof(char *));
			for (j = 0; j < port->size; j++) {
				logical_block[num_logical_blocks - 1].output_nets[port->index][j] =
						OPEN;
			}
			port = port->next;
		}
		VTR_ASSERT(port == NULL);

		/* initialize clock data */
		logical_block[num_logical_blocks - 1].clock_net = OPEN;

		logical_block[num_logical_blocks - 1].type = VPACK_COMB;
		logical_block[num_logical_blocks - 1].truth_table = NULL;
		logical_block[num_logical_blocks - 1].name = NULL;

		/* setup the index signal if open or not */

		for (i = 0; i < subckt_index_signals; i++) {
			found_subckt_signal = false;
			/* determine the port name and the pin_number of the subckt */
			port_name = vtr::strdup(subckt_signal_name[i]);
			pin_number = strrchr(port_name, '[');

            bool free_pin_number = false;
			if (pin_number == NULL) {
				pin_number = vtr::strdup("0"); /* default to 0 */
                free_pin_number = true;
			} else {
				/* The pin numbering is port_name[pin_number] so need to go one to the right of [ then NULL out ] */
				*pin_number = '\0';
				pin_number++;
				close_bracket = pin_number;
				while (*close_bracket != '\0' && *close_bracket != ']') {
					close_bracket++;
				}
				*close_bracket = '\0';
			}

			port = cur_model->inputs;
			while (port) {
				if (strcmp(port_name, port->name) == 0) {
					if (found_subckt_signal) {
						vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
								"Two instances of %s subckt signal found in subckt %s.\n",
								subckt_signal_name[i], subckt_name);
					}
					found_subckt_signal = true;
					if (port->is_clock) {
						VTR_ASSERT(
								logical_block[num_logical_blocks - 1].clock_net
										== OPEN);
						VTR_ASSERT(vtr::atoi(pin_number) == 0);
						logical_block[num_logical_blocks - 1].clock_net =
								add_vpack_net(circuit_signal_name[i], RECEIVER,
										num_logical_blocks - 1, port->index,
										vtr::atoi(pin_number), true, doall);
						VTR_ASSERT(logical_block[num_logical_blocks - 1].clock_pin_name == NULL);
						logical_block[num_logical_blocks - 1].clock_pin_name = vtr::strdup(circuit_signal_name[i]);
					} else {
						logical_block[num_logical_blocks - 1].input_nets[port->index][vtr::atoi(
								pin_number)] = add_vpack_net(
								circuit_signal_name[i], RECEIVER,
								num_logical_blocks - 1, port->index,
								vtr::atoi(pin_number), false, doall);
						logical_block[num_logical_blocks - 1].input_pin_names[port->index][vtr::atoi(
							pin_number)] = vtr::strdup(circuit_signal_name[i]);
						input_net_count++;
					}
				}
				port = port->next;
			}

			port = cur_model->outputs;
			while (port) {
				if (strcmp(port_name, port->name) == 0) {
					if (found_subckt_signal) {
						vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
								"Two instances of %s subckt signal found in subckt %s.\n",
								subckt_signal_name[i], subckt_name);
					}
					found_subckt_signal = true;
					logical_block[num_logical_blocks - 1].output_nets[port->index][vtr::atoi(
							pin_number)] = add_vpack_net(circuit_signal_name[i],
							DRIVER, num_logical_blocks - 1, port->index,
							vtr::atoi(pin_number), false, doall);
					if (subckt_logical_block_name == NULL
							&& circuit_signal_name[i] != NULL) {
						subckt_logical_block_name = circuit_signal_name[i];
					}

					logical_block[num_logical_blocks - 1].output_pin_names[port->index][vtr::atoi(
						pin_number)] = vtr::strdup(circuit_signal_name[i]);
					output_net_count++;
				}
				port = port->next;
			}

			/* record the name to be first output net parsed */
			if (logical_block[num_logical_blocks - 1].name == NULL) {
				logical_block[num_logical_blocks - 1].name = vtr::strdup(
						subckt_logical_block_name);
			}

			if (!found_subckt_signal) {
				vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
						"Unknown subckt port %s.\n", subckt_signal_name[i]);
			}
			free(port_name);

            if(free_pin_number) {
                free(pin_number);
            }
		}
	}

	for (i = 0; i < subckt_index_signals; i++) {
		free(subckt_signal_name[i]);
		free(circuit_signal_name[i]);
	}
	free(subckt_signal_name);
	free(circuit_signal_name);

	/* now that you've done the analysis, move the file pointer back */
	//if (fsetpos(blif, &current_subckt_pos) != 0) {
	//	vtr::printf_error(__FILE__, __LINE__, "In moving back file pointer - read_blif.c\n");
	//	exit(-1);
	//}
}

static void io_line(int in_or_out, bool doall, const t_model *io_model) {

	/* Adds an input or output logical_block to the logical_block data structures.           *
	 * in_or_out:  DRIVER for input, RECEIVER for output.                    *
	 * doall:  1 for final pass when structures are loaded.  0 for           *
	 * first pass when hash table is built and pins, nets, etc. are counted. */

	char *ptr;
	char buf2[vtr::BUFSIZE];
	int nindex, len, iparse;

	iparse = 0;
	while (iparse < MAX_ATOM_PARSE) {
		iparse++;
		ptr = vtr::strtok(NULL, TOKENS, blif, buf2);
		if (ptr == NULL)
			return;
		num_logical_blocks++;

		nindex = add_vpack_net(ptr, in_or_out, num_logical_blocks - 1, 0, 0,
				false, doall);
		/* zero offset indexing */
		if (!doall)
			continue; /* Just counting things when doall == 0 */

		logical_block[num_logical_blocks - 1].clock_net = OPEN;
		logical_block[num_logical_blocks - 1].input_nets = NULL;
		logical_block[num_logical_blocks - 1].output_nets = NULL;
		logical_block[num_logical_blocks - 1].model = io_model;

		len = strlen(ptr);
		if (in_or_out == RECEIVER) { /* output pads need out: prefix 
		 * to make names unique from LUTs */
			logical_block[num_logical_blocks - 1].name = (char *) vtr::malloc(
					(len + 1 + 4) * sizeof(char)); /* Space for out: at start */
			strcpy(logical_block[num_logical_blocks - 1].name, "out:");
			strcat(logical_block[num_logical_blocks - 1].name, ptr);
			logical_block[num_logical_blocks - 1].input_nets =
					(int **) vtr::malloc(sizeof(int*));
			logical_block[num_logical_blocks - 1].input_nets[0] =
					(int *) vtr::malloc(sizeof(int));
			logical_block[num_logical_blocks - 1].input_nets[0][0] = OPEN;
		} else {
			VTR_ASSERT(in_or_out == DRIVER);
			logical_block[num_logical_blocks - 1].name = (char *) vtr::malloc(
					(len + 1) * sizeof(char));
			strcpy(logical_block[num_logical_blocks - 1].name, ptr);
			logical_block[num_logical_blocks - 1].output_nets =
					(int **) vtr::malloc(sizeof(int*));
			logical_block[num_logical_blocks - 1].output_nets[0] =
					(int *) vtr::malloc(sizeof(int));
			logical_block[num_logical_blocks - 1].output_nets[0][0] = OPEN;
		}

		if (in_or_out == DRIVER) { /* processing .inputs line */
			num_p_inputs++;
			logical_block[num_logical_blocks - 1].type = VPACK_INPAD;
			logical_block[num_logical_blocks - 1].output_nets[0][0] = nindex;
		} else { /* processing .outputs line */
			num_p_outputs++;
			logical_block[num_logical_blocks - 1].type = VPACK_OUTPAD;
			logical_block[num_logical_blocks - 1].input_nets[0][0] = nindex;
		}
		logical_block[num_logical_blocks - 1].truth_table = NULL;
	}
	VTR_ASSERT(iparse < MAX_ATOM_PARSE);
}

static void check_and_count_models(bool doall, const char* model_name,
		const t_model *user_models) {
	fpos_t start_pos;
	const t_model *user_model;

	num_blif_models++;
	if (doall) {
		/* get start position to do two passes on model */
		if (fgetpos(blif, &start_pos) != 0) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"in file pointer read - read_blif.c\n");
		}

		/* get corresponding architecture model */
		user_model = user_models;
		while (user_model) {
			if (0 == strcmp(model_name, user_model->name)) {
				break;
			}
			user_model = user_model->next;
		}
		if (user_model == NULL) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"No corresponding model %s in architecture description.\n",
					model_name);
		}

		/* check ports */
	}
}

static int add_vpack_net(char *ptr, int type, int bnum, int bport, int bpin,
		bool is_global, bool doall) {

	/* This routine is given a vpack_net name in *ptr, either DRIVER or RECEIVER *
	 * specifying whether the logical_block number (bnum) and the output pin (bpin) is driving this   *
	 * vpack_net or in the fan-out and doall, which is 0 for the counting pass   *
	 * and 1 for the loading pass.  It updates the vpack_net data structure and  *
	 * returns the vpack_net number so the calling routine can update the logical_block  *
	 * data structure.                                                     */

	struct s_hash *h_ptr, *prev_ptr;
	int index, j, nindex;

	if (strcmp(ptr, "open") == 0) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"net name \"open\" is a reserved keyword in VPR.");
	}

	if (strcmp(ptr, "unconn") == 0) {
		return OPEN;
	}
	index = hash_value(ptr);

	if (doall) {
		if (type == RECEIVER && !is_global) {
			logical_block_input_count[bnum]++;
		} else if (type == DRIVER) {
			logical_block_output_count[bnum]++;
		}
	}

	h_ptr = blif_hash[index];
	prev_ptr = h_ptr;

	while (h_ptr != NULL) {
		if (strcmp(h_ptr->name, ptr) == 0) { /* Net already in hash table */
			nindex = h_ptr->index;

			if (!doall) { /* Counting pass only */
				(h_ptr->count)++;
				return (nindex);
			}

			if (type == DRIVER) {
				num_driver[nindex]++;
				j = 0; /* Driver always in position 0 of pinlist */
			} else {
				vpack_net[nindex].num_sinks++;
				if ((num_driver[nindex] < 0) || (num_driver[nindex] > 1)) {
					vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
							"Number of drivers for net #%d (%s) has %d drivers.\n",
							nindex, ptr, num_driver[index]);
				}
				j = vpack_net[nindex].num_sinks;

				/* num_driver is the number of signal drivers of this vpack_net. *
				 * should always be zero or 1 unless the netlist is bad.   */
				if ((vpack_net[nindex].num_sinks - num_driver[nindex])
						>= temp_num_pins[nindex]) {
					vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
							"Net #%d (%s) has no driver and will cause memory corruption.\n",
							nindex, ptr);
				}
			}
			vpack_net[nindex].node_block[j] = bnum;
			vpack_net[nindex].node_block_port[j] = bport;
			vpack_net[nindex].node_block_pin[j] = bpin;
			vpack_net[nindex].is_global = is_global;
			return (nindex);
		}
		prev_ptr = h_ptr;
		h_ptr = h_ptr->next;
	}

	/* Net was not in the hash table. */

	if (doall == 1) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"in add_vpack_net: The second (load) pass could not find vpack_net %s in the symbol table.\n",
				ptr);
	}

	/* Add the vpack_net (only counting pass will add nets to symbol table). */

	num_logical_nets++;
	h_ptr = (struct s_hash *) vtr::malloc(sizeof(struct s_hash));
	if (prev_ptr == NULL) {
		blif_hash[index] = h_ptr;
	} else {
		prev_ptr->next = h_ptr;
	}
	h_ptr->next = NULL;
	h_ptr->index = num_logical_nets - 1;
	h_ptr->count = 1;
	h_ptr->name = vtr::strdup(ptr);
	return (h_ptr->index);
}

void echo_input(const char *blif_file, const char *echo_file, const t_model *library_models) {

	/* Echo back the netlist data structures to file input.echo to *
	 * allow the user to look at the internal state of the program *
	 * and check the parsing.                                      */

	int i, j;
	FILE *fp;
	t_model_ports *port;
	const t_model *latch_model;
	const t_model *logic_model;
	const t_model *cur;
	int *lut_distribution;
	int num_absorbable_latch;
	int inet;

	cur = library_models;
	logic_model = latch_model = NULL;
	while (cur) {
		if (strcmp(cur->name, MODEL_LOGIC) == 0) {
			logic_model = cur;
			VTR_ASSERT(logic_model->inputs->next == NULL);
		} else if (strcmp(cur->name, MODEL_LATCH) == 0) {
			latch_model = cur;
			VTR_ASSERT(latch_model->inputs->size == 1);
		}
		cur = cur->next;
	}
	
	VTR_ASSERT(logic_model != NULL);
	lut_distribution = (int*) vtr::calloc(logic_model->inputs[0].size + 1,
			sizeof(int));
	num_absorbable_latch = 0;
	for (i = 0; i < num_logical_blocks; i++) {
		if (!logical_block)
			continue;

		if (logical_block[i].model == logic_model) {
			//if (logic_model == NULL)
			//	continue;
			for (j = 0; j < logic_model->inputs[0].size; j++) {
				if (logical_block[i].input_nets[0][j] == OPEN) {
					break;
				}
			}
			lut_distribution[j]++;
		} else if (logical_block[i].model == latch_model) {
			if (latch_model == NULL)
				continue;
			inet = logical_block[i].input_nets[0][0];
			if (vpack_net[inet].num_sinks == 1
					&& logical_block[vpack_net[inet].node_block[0]].model
							== logic_model) {
				num_absorbable_latch++;
			}
		}
	}

	vtr::printf_info("Input netlist file: '%s', model: %s\n", blif_file, model);
	vtr::printf_info("Primary inputs: %d, primary outputs: %d\n", num_p_inputs,
			num_p_outputs);
	vtr::printf_info("LUTs: %d, latches: %d, subckts: %d\n", num_luts,
			num_latches, num_subckts);
	vtr::printf_info("# standard absorbable latches: %d\n",
			num_absorbable_latch);
	vtr::printf_info("\t");
	for (i = 0; i < logic_model->inputs[0].size + 1; i++) {
		if (i > 0)
			vtr::printf_direct(", ");
		vtr::printf_direct("LUT size %d = %d", i, lut_distribution[i]);
	}
	vtr::printf_direct("\n");
	vtr::printf_info("Total blocks: %d, total nets: %d\n", num_logical_blocks,
			num_logical_nets);

	fp = vtr::fopen(echo_file, "w");

	fprintf(fp, "Input netlist file: '%s', model: %s\n", blif_file, model);
	fprintf(fp,
			"num_p_inputs: %d, num_p_outputs: %d, num_luts: %d, num_latches: %d\n",
			num_p_inputs, num_p_outputs, num_luts, num_latches);
	fprintf(fp, "num_logical_blocks: %d, num_logical_nets: %d\n",
			num_logical_blocks, num_logical_nets);

	fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\tRecvs.\n");
	for (i = 0; i < num_logical_nets; i++) {
		if (!vpack_net)
			continue;

		fprintf(fp, "\n%d\t%s\t", i, vpack_net[i].name);
		if (strlen(vpack_net[i].name) < 8)
			fprintf(fp, "\t"); /* Name field is 16 chars wide */
		fprintf(fp, "%d", vpack_net[i].num_sinks + 1);
		for (j = 0; j <= vpack_net[i].num_sinks; j++)
			fprintf(fp, "\t(%d,%d,%d)", vpack_net[i].node_block[j],
					vpack_net[i].node_block_port[j],
					vpack_net[i].node_block_pin[j]);
	}

	fprintf(fp, "\n\nBlocks\t\tBlock type legend:\n");
	fprintf(fp, "\t\tINPAD = %d\tOUTPAD = %d\n", VPACK_INPAD, VPACK_OUTPAD);
	fprintf(fp, "\t\tCOMB = %d\tLATCH = %d\n", VPACK_COMB, VPACK_LATCH);
	fprintf(fp, "\t\tEMPTY = %d\n", VPACK_EMPTY);

	for (i = 0; i < num_logical_blocks; i++) {
		if (!logical_block)
			continue;

		fprintf(fp, "\nblock %d %s ", i, logical_block[i].name);
		fprintf(fp, "\ttype: %d ", logical_block[i].type);
		fprintf(fp, "\tmodel name: %s\n", logical_block[i].model->name);

		port = logical_block[i].model->inputs;

		while (port) {
			fprintf(fp, "\tinput port: %s \t", port->name);
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].input_nets[port->index][j] == OPEN)
					fprintf(fp, "OPEN ");
				else
					fprintf(fp, "%d ",
							logical_block[i].input_nets[port->index][j]);
			}
			fprintf(fp, "\n");
			port = port->next;
		}

		port = logical_block[i].model->outputs;
		while (port) {
			fprintf(fp, "\toutput port: %s \t", port->name);
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].output_nets[port->index][j] == OPEN) {
					fprintf(fp, "OPEN ");
				} else {
					fprintf(fp, "%d ",
							logical_block[i].output_nets[port->index][j]);
				}
			}
			fprintf(fp, "\n");
			port = port->next;
		}

		fprintf(fp, "\tclock net: %d\n", logical_block[i].clock_net);
	}
	fclose(fp);

	free(lut_distribution);
}

/* load default vpack models (inpad, outpad, logic) */
static void load_default_models(const t_model *library_models,
		const t_model** inpad_model, const t_model** outpad_model,
		const t_model** logic_model, const t_model** latch_model) {
	const t_model *cur_model;
	cur_model = library_models;
	*inpad_model = *outpad_model = *logic_model = *latch_model = NULL;
	while (cur_model) {
		if (strcmp(MODEL_INPUT, cur_model->name) == 0) {
			VTR_ASSERT(cur_model->inputs == NULL);
			VTR_ASSERT(cur_model->outputs->next == NULL);
			VTR_ASSERT(cur_model->outputs->size == 1);
			*inpad_model = cur_model;
		} else if (strcmp(MODEL_OUTPUT, cur_model->name) == 0) {
			VTR_ASSERT(cur_model->outputs == NULL);
			VTR_ASSERT(cur_model->inputs->next == NULL);
			VTR_ASSERT(cur_model->inputs->size == 1);
			*outpad_model = cur_model;
		} else if (strcmp(MODEL_LOGIC, cur_model->name) == 0) {
			VTR_ASSERT(cur_model->inputs->next == NULL);
			VTR_ASSERT(cur_model->outputs->next == NULL);
			VTR_ASSERT(cur_model->outputs->size == 1);
			*logic_model = cur_model;
		} else if (strcmp(MODEL_LATCH, cur_model->name) == 0) {
			VTR_ASSERT(cur_model->outputs->next == NULL);
			VTR_ASSERT(cur_model->outputs->size == 1);
			*latch_model = cur_model;
		} else {
			VTR_ASSERT(0);
		}
		cur_model = cur_model->next;
	}
}

static t_model_ports* find_clock_port(const t_model* block_model) {
    t_model_ports* port = block_model->inputs;
    while(port != NULL && !port->is_clock) {
        port = port->next;
    }
    return port;
}

static int check_blocks() {

    int error_count = 0;
    for(int iblk = 0; iblk < num_logical_blocks; iblk++) {
        const t_model* block_model = logical_block[iblk].model;

        //Check if this type has a clock port
        t_model_ports* clk_port = find_clock_port(block_model);

        if(clk_port) {
            //This block type/model has a clock port
            
            // It must have a valid clock net, or else the delay
            // calculator asserts on an obscure condition. Better
            // to warn the user now
            if(logical_block[iblk].clock_net == OPEN) {
                vtr::printf_error(__FILE__, __LINE__, "Block '%s' (%s) at index %d has an invalid clock net.\n", logical_block[iblk].name, block_model->name, iblk);
                error_count++;
            }
        }

    }

    return error_count;
}

static int check_net(bool sweep_hanging_nets_and_inputs) {

	/* Checks the input netlist for obvious errors. */

	int i, j, k, error, iblk, ipin, iport, inet, L_check_net;
	bool found;
	int count_inputs, count_outputs;
	int explicit_vpack_models;
	t_model_ports *port;
	vtr::t_linked_vptr *p_io_removed;
	int removed_nets;
	int count_unconn_blocks;

	explicit_vpack_models = num_blif_models + 1;

	error = 0;
	removed_nets = 0;

	if (ilines != explicit_vpack_models) {
		vtr::printf_error(__FILE__, __LINE__,
				"Found %d .inputs lines; expected %d.\n", ilines,
				explicit_vpack_models);
		error++;
	}

	if (olines != explicit_vpack_models) {
		vtr::printf_error(__FILE__, __LINE__,
				"Found %d .outputs lines; expected %d.\n", olines,
				explicit_vpack_models);
		error++;
	}

	if (model_lines != explicit_vpack_models) {
		vtr::printf_error(__FILE__, __LINE__,
				"Found %d .model lines; expected %d.\n", model_lines,
				num_blif_models + 1);
		error++;
	}

	if (endlines != explicit_vpack_models) {
		vtr::printf_error(__FILE__, __LINE__,
				"Found %d .end lines; expected %d.\n", endlines,
				explicit_vpack_models);
		error++;
	}
	for (i = 0; i < num_logical_nets; i++) {

		if (num_driver[i] != 1) {
			vtr::printf_error(__FILE__, __LINE__,
					"vpack_net %s has %d signals driving it.\n",
					vpack_net[i].name, num_driver[i]);
			error++;
		}

		if (vpack_net[i].num_sinks == 0) {

			/* If this is an input pad, it is unused and I just remove it with  *
			 * a warning message.  Lots of the mcnc circuits have this problem. 

			 Also, subckts from ODIN often have unused driven nets
			 */

			iblk = vpack_net[i].node_block[0];
			iport = vpack_net[i].node_block_port[0];
			ipin = vpack_net[i].node_block_pin[0];

			VTR_ASSERT((vpack_net[i].num_sinks - num_driver[i]) == -1);

			/* All nets should connect to inputs of block except output pads */
			if (logical_block[iblk].type != VPACK_OUTPAD) {
				if (sweep_hanging_nets_and_inputs) {
					removed_nets++;
					vpack_net[i].node_block[0] = OPEN;
					vpack_net[i].node_block_port[0] = OPEN;
					vpack_net[i].node_block_pin[0] = OPEN;
					logical_block[iblk].output_nets[iport][ipin] = OPEN;
					logical_block_output_count[iblk]--;
				} else {
					vtr::printf_warning(__FILE__, __LINE__,
							"vpack_net %s has no fanout.\n", vpack_net[i].name);
				}
				continue;
			}
		}

		if (strcmp(vpack_net[i].name, "open") == 0
				|| strcmp(vpack_net[i].name, "unconn") == 0) {
			vtr::printf_error(__FILE__, __LINE__,
					"vpack_net #%d has the reserved name %s.\n", i,
					vpack_net[i].name);
			error++;
		}

		for (j = 0; j <= vpack_net[i].num_sinks; j++) {
			iblk = vpack_net[i].node_block[j];
			iport = vpack_net[i].node_block_port[j];
			ipin = vpack_net[i].node_block_pin[j];
			if (ipin == OPEN) {
				/* Clocks are not connected to regular pins on a block hence open */
				L_check_net = logical_block[iblk].clock_net;
				if (L_check_net != i) {
					vtr::printf_error(__FILE__, __LINE__,
							"Clock net for block %s #%d is net %s #%d but connecting net is %s #%d.\n",
							logical_block[iblk].name, iblk,
							vpack_net[L_check_net].name, L_check_net,
							vpack_net[i].name, i);
					error++;
				}

			} else {
				if (j == 0) {
					L_check_net = logical_block[iblk].output_nets[iport][ipin];
					if (L_check_net != i) {
						vtr::printf_error(__FILE__, __LINE__,
								"Output net for block %s #%d is net %s #%d but connecting net is %s #%d.\n",
								logical_block[iblk].name, iblk,
								vpack_net[L_check_net].name, L_check_net,
								vpack_net[i].name, i);
						error++;
					}
				} else {
					if (vpack_net[i].is_global) {
						L_check_net = logical_block[iblk].clock_net;
					} else {
						L_check_net =
								logical_block[iblk].input_nets[iport][ipin];
					}
					if (L_check_net != i) {
						vtr::printf_error(__FILE__, __LINE__,
								"You have a signal that enters both clock ports and normal input ports.\n"
										"Input net for block %s #%d is net %s #%d but connecting net is %s #%d.\n",
								logical_block[iblk].name, iblk,
								vpack_net[L_check_net].name, L_check_net,
								vpack_net[i].name, i);
						error++;
					}
				}
			}
		}
	}
	vtr::printf_info("Swept away %d nets with no fanout.\n", removed_nets);
	count_unconn_blocks = 0;
	for (i = 0; i < num_logical_blocks; i++) {
		/* This block has no output and is not an output pad so it has no use, hence we remove it */
		if ((logical_block_output_count[i] == 0)
				&& (logical_block[i].type != VPACK_OUTPAD)) {
			vtr::printf_warning(__FILE__, __LINE__,
					"logical_block %s #%d has no fanout.\n",
					logical_block[i].name, i);
			if (sweep_hanging_nets_and_inputs
					&& (logical_block[i].type == VPACK_INPAD)) {
				logical_block[i].type = VPACK_EMPTY;
				vtr::printf_info("Removing input.\n");
				p_io_removed = (vtr::t_linked_vptr*) vtr::malloc(
						sizeof(vtr::t_linked_vptr));
				p_io_removed->data_vptr = vtr::strdup(logical_block[i].name);
				p_io_removed->next = circuit_p_io_removed;
				circuit_p_io_removed = p_io_removed;
				continue;
			} else {
				count_unconn_blocks++;
				vtr::printf_warning(__FILE__, __LINE__,
						"Sweep hanging nodes in your logic synthesis tool because VPR can not do this yet.\n");
			}
		}
		count_inputs = 0;
		count_outputs = 0;
		port = logical_block[i].model->inputs;
		while (port) {
			if (port->is_clock) {
				port = port->next;
				continue;
			}

			for (j = 0; j < port->size; j++) {
				if (logical_block[i].input_nets[port->index][j] == OPEN)
					continue;
				count_inputs++;
				inet = logical_block[i].input_nets[port->index][j];
				found = false;
				for (k = 1; k <= vpack_net[inet].num_sinks; k++) {
					if (vpack_net[inet].node_block[k] == i) {
						if (vpack_net[inet].node_block_port[k] == port->index) {
							if (vpack_net[inet].node_block_pin[k] == j) {
								found = true;
							}
						}
					}
				}
				VTR_ASSERT(found == true);
			}
			port = port->next;
		}
		VTR_ASSERT(count_inputs == logical_block_input_count[i]);
		logical_block[i].used_input_pins = count_inputs;

		port = logical_block[i].model->outputs;
		while (port) {
			for (j = 0; j < port->size; j++) {
				if (logical_block[i].output_nets[port->index][j] == OPEN)
					continue;
				count_outputs++;
				inet = logical_block[i].output_nets[port->index][j];
				vpack_net[inet].is_const_gen = false;
				if (count_inputs == 0 && logical_block[i].type != VPACK_INPAD
						&& logical_block[i].type != VPACK_OUTPAD
						&& logical_block[i].clock_net == OPEN) {
					vtr::printf_info("Net is a constant generator: %s.\n",
							vpack_net[inet].name);
					vpack_net[inet].is_const_gen = true;
				}
				found = false;
				if (vpack_net[inet].node_block[0] == i) {
					if (vpack_net[inet].node_block_port[0] == port->index) {
						if (vpack_net[inet].node_block_pin[0] == j) {
							found = true;
						}
					}
				}
				VTR_ASSERT(found == true);
			}
			port = port->next;
		}
		VTR_ASSERT(count_outputs == logical_block_output_count[i]);

		if (logical_block[i].type == VPACK_LATCH) {
			if (logical_block_input_count[i] != 1) {
				vtr::printf_error(__FILE__, __LINE__,
						"Latch #%d with output %s has %d input pin(s), expected one (D).\n",
						i, logical_block[i].name, logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 1) {
				vtr::printf_error(__FILE__, __LINE__,
						"Latch #%d with output %s has %d output pin(s), expected one (Q).\n",
						i, logical_block[i].name,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net == OPEN) {
				vtr::printf_error(__FILE__, __LINE__,
						"Latch #%d with output %s has no clock.\n", i,
						logical_block[i].name);
				error++;
			}
		}

		else if (logical_block[i].type == VPACK_INPAD) {
			if (logical_block_input_count[i] != 0) {
				vtr::printf_error(__FILE__, __LINE__,
						"IO inpad logical_block #%d name %s of type %d" "has %d input pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 1) {
				vtr::printf_error(__FILE__, __LINE__,
						"IO inpad logical_block #%d name %s of type %d" "has %d output pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net != OPEN) {
				vtr::printf_error(__FILE__, __LINE__,
						"IO inpad #%d with output %s has clock.\n", i,
						logical_block[i].name);
				error++;
			}
		} else if (logical_block[i].type == VPACK_OUTPAD) {
			if (logical_block_input_count[i] != 1) {
				vtr::printf_error(__FILE__, __LINE__,
						"io outpad logical_block #%d name %s of type %d" "has %d input pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_input_count[i]);
				error++;
			}
			if (logical_block_output_count[i] != 0) {
				vtr::printf_error(__FILE__, __LINE__,
						"io outpad logical_block #%d name %s of type %d" "has %d output pins.\n",
						i, logical_block[i].name, logical_block[i].type,
						logical_block_output_count[i]);
				error++;
			}
			if (logical_block[i].clock_net != OPEN) {
				vtr::printf_error(__FILE__, __LINE__,
						"io outpad #%d with name %s has clock.\n", i,
						logical_block[i].name);
				error++;
			}
		} else if (logical_block[i].type == VPACK_COMB) {
			if (logical_block_input_count[i] <= 0) {
				vtr::printf_warning(__FILE__, __LINE__,
						"logical_block #%d with output %s has only %d pin.\n",
						i, logical_block[i].name, logical_block_input_count[i]);

				if (logical_block_input_count[i] < 0) {
					error++;
				} else {
					if (logical_block_output_count[i] > 0) {
						vtr::printf_warning(__FILE__, __LINE__,
								"Block contains output -- may be a constant generator.\n");
					} else {
						vtr::printf_warning(__FILE__, __LINE__,
								"Block contains no output.\n");
					}
				}
			}

			if (strcmp(logical_block[i].model->name, MODEL_LOGIC) == 0) {
				if (logical_block_output_count[i] != 1) {
					vtr::printf_warning(__FILE__, __LINE__,
							"Logical_block #%d name %s of model %s has %d output pins instead of 1.\n",
							i, logical_block[i].name,
							logical_block[i].model->name,
							logical_block_output_count[i]);
				}
			}
		} else {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"Unknown type for logical_block #%d %s.\n", i,
					logical_block[i].name);
		}
	}

	if (count_unconn_blocks > 0) {
		vtr::printf_info("%d unconnected blocks in input netlist.\n",
				count_unconn_blocks);
	}

    return error;
}

static void free_parse(void) {

	/* Release memory needed only during blif network parsing. */

	int i;
	struct s_hash *h_ptr, *temp_ptr;

	for (i = 0; i < HASHSIZE; i++) {
		h_ptr = blif_hash[i];
		while (h_ptr != NULL) {
			free((void *) h_ptr->name);
			temp_ptr = h_ptr->next;
			free((void *) h_ptr);
			h_ptr = temp_ptr;
		}
	}
	free((void *) num_driver);
	free((void *) blif_hash);
	free((void *) temp_num_pins);
}

static void do_absorb_buffer_luts(void) {
	/* This routine uses a simple pattern matching algorithm to remove buffer LUTs where possible (single-input LUTs that are programmed to be a wire) */

	int bnum, in_blk, out_blk, ipin, out_net, in_net;
	int removed = 0;

	/* Pin ordering for the clb blocks (1 VPACK_LUT + 1 FF in each logical_block) is      *
	 * output, n VPACK_LUT inputs, clock input.                                   */

	for (bnum = 0; bnum < num_logical_blocks; bnum++) {
		if (strcmp(logical_block[bnum].model->name, "names") == 0) {
			if (logical_block[bnum].truth_table != NULL
					&& logical_block[bnum].truth_table->data_vptr) {
				if (strcmp("0 0",
						(char*) logical_block[bnum].truth_table->data_vptr) == 0
						|| strcmp("1 1",
								(char*) logical_block[bnum].truth_table->data_vptr)
								== 0) {
					for (ipin = 0;
							ipin < logical_block[bnum].model->inputs->size;
							ipin++) {
						if (logical_block[bnum].input_nets[0][ipin] == OPEN)
							break;
					}
					VTR_ASSERT(ipin == 1);

					VTR_ASSERT(logical_block[bnum].clock_net == OPEN);
					VTR_ASSERT(logical_block[bnum].model->inputs->next == NULL);
					VTR_ASSERT(logical_block[bnum].model->outputs->size == 1);
					VTR_ASSERT(logical_block[bnum].model->outputs->next == NULL);

					in_net = logical_block[bnum].input_nets[0][0]; /* Net driving the buffer */
					out_net = logical_block[bnum].output_nets[0][0]; /* Net the buffer us driving */
					out_blk = vpack_net[out_net].node_block[1];
					in_blk = vpack_net[in_net].node_block[0];

					VTR_ASSERT(in_net != OPEN);
					VTR_ASSERT(out_net != OPEN);
					VTR_ASSERT(out_blk != OPEN);
					VTR_ASSERT(in_blk != OPEN);

					/* TODO: Make this handle general cases, due to time reasons I can only handle buffers with single outputs */
					if (vpack_net[out_net].num_sinks == 1) {
						for (ipin = 1; ipin <= vpack_net[in_net].num_sinks;
								ipin++) {
							if (vpack_net[in_net].node_block[ipin] == bnum) {
								break;
							}
						}
						VTR_ASSERT(ipin <= vpack_net[in_net].num_sinks);

						vpack_net[in_net].node_block[ipin] =
								vpack_net[out_net].node_block[1]; /* New output */
						vpack_net[in_net].node_block_port[ipin] =
								vpack_net[out_net].node_block_port[1];
						vpack_net[in_net].node_block_pin[ipin] =
								vpack_net[out_net].node_block_pin[1];

						VTR_ASSERT(
								logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]]
										== out_net);
						logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]] =
								in_net;

						vpack_net[out_net].node_block[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_pin[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_port[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].num_sinks = 0; /* This vpack_net disappears; mark. */

						logical_block[bnum].type = VPACK_EMPTY; /* Mark logical_block that had LUT */

						/* error checking */
						for (ipin = 0; ipin <= vpack_net[out_net].num_sinks;
								ipin++) {
							VTR_ASSERT(vpack_net[out_net].node_block[ipin] != bnum);
						}
						removed++;
					}
				}
			}
		}
	}
	vtr::printf_info("Removed %d LUT buffers.\n", removed);
}

static void compress_netlist(void) {

	/* This routine removes all the VPACK_EMPTY blocks and OPEN nets that *
	 * may have been left behind post synthesis.  After this    *
	 * routine, all the VPACK blocks that exist in the netlist     *
	 * are in a contiguous list with no unused spots.  The same     *
	 * goes for the list of nets.  This means that blocks and nets  *
	 * have to be renumbered somewhat.                              */

	int inet, iblk, index, ipin, new_num_nets, new_num_blocks, i;
	int *net_remap, *block_remap;
	int L_num_nets;
	t_model_ports *port;
	vtr::t_linked_vptr *tvptr, *next;

	new_num_nets = 0;
	new_num_blocks = 0;
	net_remap = (int *) vtr::malloc(num_logical_nets * sizeof(int));
	block_remap = (int *) vtr::malloc(num_logical_blocks * sizeof(int));

	for (inet = 0; inet < num_logical_nets; inet++) {
		if (vpack_net[inet].node_block[0] != OPEN) {
			net_remap[inet] = new_num_nets;
			new_num_nets++;
		} else {
			net_remap[inet] = OPEN;
		}
	}

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (logical_block[iblk].type != VPACK_EMPTY) {
			block_remap[iblk] = new_num_blocks;
			new_num_blocks++;
		} else {
			block_remap[iblk] = OPEN;
		}
	}

	if (new_num_nets != num_logical_nets
			|| new_num_blocks != num_logical_blocks) {

		for (inet = 0; inet < num_logical_nets; inet++) {
			if (vpack_net[inet].node_block[0] != OPEN) {
				index = net_remap[inet];
				vpack_net[index] = vpack_net[inet];
				if (vpack_net_power) {
					vpack_net_power[index] = vpack_net_power[inet];
				}
				for (ipin = 0; ipin <= vpack_net[index].num_sinks; ipin++) {
					vpack_net[index].node_block[ipin] =
							block_remap[vpack_net[index].node_block[ipin]];
				}
			} else {
				free(vpack_net[inet].name);
				free(vpack_net[inet].node_block);
				free(vpack_net[inet].node_block_port);
				free(vpack_net[inet].node_block_pin);
			}
		}

		num_logical_nets = new_num_nets;
		vpack_net = (struct s_net *) vtr::realloc(vpack_net,
				num_logical_nets * sizeof(struct s_net));
		vpack_net_power = (t_net_power *) vtr::realloc(vpack_net_power,
				num_logical_nets * sizeof(t_net_power));

		for (iblk = 0; iblk < num_logical_blocks; iblk++) {
			if (logical_block[iblk].type != VPACK_EMPTY) {
				index = block_remap[iblk];
				if (index != iblk) {
					logical_block[index] = logical_block[iblk];
					logical_block[index].index = index; /* array index moved */
				}

				L_num_nets = 0;
				port = logical_block[index].model->inputs;
				while (port) {
					for (ipin = 0; ipin < port->size; ipin++) {
						if (port->is_clock) {
							VTR_ASSERT(
									port->size == 1 && port->index == 0
											&& ipin == 0);
							if (logical_block[index].clock_net == OPEN)
								continue;
							logical_block[index].clock_net =
									net_remap[logical_block[index].clock_net];
						} else {
							if (logical_block[index].input_nets[port->index][ipin]
									== OPEN)
								continue;
							logical_block[index].input_nets[port->index][ipin] =
									net_remap[logical_block[index].input_nets[port->index][ipin]];
						}
						L_num_nets++;
					}
					port = port->next;
				}

				port = logical_block[index].model->outputs;
				while (port) {
					for (ipin = 0; ipin < port->size; ipin++) {
						if (logical_block[index].output_nets[port->index][ipin]
								== OPEN)
							continue;
						logical_block[index].output_nets[port->index][ipin] =
								net_remap[logical_block[index].output_nets[port->index][ipin]];
						L_num_nets++;
					}
					port = port->next;
				}
			}

			else {
				free(logical_block[iblk].name);
				port = logical_block[iblk].model->inputs;
				i = 0;
				while (port) {
					if (!port->is_clock) {
						if (logical_block[iblk].input_nets) {
							if (logical_block[iblk].input_nets[i]) {
								free(logical_block[iblk].input_nets[i]);
								logical_block[iblk].input_nets[i] = NULL;
							}
						}
						i++;
					}
					port = port->next;
				}
				if (logical_block[iblk].input_nets)
					free(logical_block[iblk].input_nets);
				port = logical_block[iblk].model->outputs;
				i = 0;
				while (port) {
					if (logical_block[iblk].output_nets) {
						if (logical_block[iblk].output_nets[i]) {
							free(logical_block[iblk].output_nets[i]);
							logical_block[iblk].output_nets[i] = NULL;
						}
					}
					i++;
					port = port->next;
				}
				if (logical_block[iblk].output_nets)
					free(logical_block[iblk].output_nets);
				tvptr = logical_block[iblk].truth_table;
				while (tvptr != NULL) {
					if (tvptr->data_vptr)
						free(tvptr->data_vptr);
					next = tvptr->next;
					free(tvptr);
					tvptr = next;
				}
			}
		}

		vtr::printf_info("Sweeped away %d nodes.\n",
				num_logical_blocks - new_num_blocks);

		num_logical_blocks = new_num_blocks;
		logical_block = (struct s_logical_block *) vtr::realloc(logical_block,
				num_logical_blocks * sizeof(struct s_logical_block));
	}

	/* Now I have to recompute the number of primary inputs and outputs, since *
	 * some inputs may have been unused and been removed.  No real need to     *
	 * recount primary outputs -- it's just done as defensive coding.          */

	num_p_inputs = 0;
	num_p_outputs = 0;

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (logical_block[iblk].type == VPACK_INPAD)
			num_p_inputs++;
		else if (logical_block[iblk].type == VPACK_OUTPAD)
			num_p_outputs++;
	}

	free(net_remap);
	free(block_remap);
}

/* Read blif file and perform basic sweep/accounting on it
 * - power_opts: Power options, can be NULL
 */
void read_and_process_blif(const char *blif_file,
		bool sweep_hanging_nets_and_inputs, bool absorb_buffer_luts,
        const t_model *user_models,
		const t_model *library_models, bool read_activity_file,
		char * activity_file) {

	/* begin parsing blif input file */
#if 0
	read_blif(blif_file, sweep_hanging_nets_and_inputs, user_models,
			library_models, read_activity_file, activity_file);
#else
	read_blif2(blif_file, sweep_hanging_nets_and_inputs, user_models,
			library_models, read_activity_file, activity_file);
#endif

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_BLIF_INPUT)) {
		echo_input(blif_file, getEchoFileName(E_ECHO_BLIF_INPUT),
				library_models);
	}

    if(absorb_buffer_luts) {
        do_absorb_buffer_luts();
    }
	compress_netlist(); /* remove unused inputs */

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_COMPRESSED_NETLIST)) {
		echo_input(blif_file, getEchoFileName(E_ECHO_COMPRESSED_NETLIST),
				library_models);
	}

	//Added August 2013, Daniel Chen for loading flattened netlist into new data structures
	load_global_net_from_array(vpack_net, num_logical_nets, &g_atoms_nlist);
	//echo_global_nlist_net(&g_atoms_nlist, vpack_net);

	/* NB:  It's important to mark clocks and such *after* compressing the   *
	 * netlist because the vpack_net numbers, etc. may be changed by removing      *
	 * unused inputs .  */

	show_blif_stats(user_models, library_models);
	free(logical_block_input_count);
	free(logical_block_output_count);
	free(model);
	logical_block_input_count = NULL;
	logical_block_output_count = NULL;
	model = NULL;
}

/* Output blif statistics */
static void show_blif_stats(const t_model *user_models, const t_model *library_models) {
	struct s_model_stats *model_stats;
	struct s_model_stats *lut_model;
	int num_model_stats;
	const t_model *cur;
	int MAX_LUT_INPUTS;
	int i, j, iblk, ipin, num_pins;
	int *num_lut_of_size;

	/* Store data structure for all models in FPGA */
	num_model_stats = 0;

	cur = library_models;
	while (cur) {
		num_model_stats++;
		cur = cur->next;
	}

	cur = user_models;
	while (cur) {
		num_model_stats++;
		cur = cur->next;
	}

	model_stats = (struct s_model_stats*) vtr::calloc(num_model_stats,
			sizeof(struct s_model_stats));

	num_model_stats = 0;

	lut_model = NULL;
	cur = library_models;
	while (cur) {
		model_stats[num_model_stats].model = cur;
		if (strcmp(cur->name, "names") == 0) {
			lut_model = &model_stats[num_model_stats];
		}
		num_model_stats++;
		cur = cur->next;
	}

	cur = user_models;
	while (cur) {
		model_stats[num_model_stats].model = cur;
		num_model_stats++;
		cur = cur->next;
	}

	/* Gather statistics from circuit */
	MAX_LUT_INPUTS = 0;
	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (strcmp(logical_block[iblk].model->name, "names") == 0) {
			MAX_LUT_INPUTS = logical_block[iblk].model->inputs->size;
			break;
		}
	}
	num_lut_of_size = (int*) vtr::calloc(MAX_LUT_INPUTS + 1, sizeof(int));

	for (i = 0; i < num_logical_blocks; i++) {
		for (j = 0; j < num_model_stats; j++) {
			if (logical_block[i].model == model_stats[j].model) {
				break;
			}
		}
		VTR_ASSERT(j < num_model_stats);
		model_stats[j].count++;
		if (&model_stats[j] == lut_model) {
			num_pins = 0;
			for (ipin = 0; ipin < logical_block[i].model->inputs->size;
					ipin++) {
				if (logical_block[i].input_nets[0][ipin] != OPEN) {
					num_pins++;
				}
			}
			num_lut_of_size[num_pins]++;
		}
	}

	/* Print blif circuit stats */

	vtr::printf_info("BLIF circuit stats:\n");

	for (i = 0; i <= MAX_LUT_INPUTS; i++) {
		vtr::printf_info("\t%d LUTs of size %d\n", num_lut_of_size[i], i);
	}
	for (i = 0; i < num_model_stats; i++) {
		vtr::printf_info("\t%d of type %s\n", model_stats[i].count,
				model_stats[i].model->name);
	}

	free(model_stats);
	free(num_lut_of_size);
}

static void read_activity(char * activity_file) {
	int net_idx;
	bool fail;
	char buf[vtr::BUFSIZE];
	char * ptr;
	char * word1;
	char * word2;
	char * word3;

	FILE * act_file_hdl;

	if (num_logical_nets == 0) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"Error reading activity file.  Must read netlist first\n");
	}

	for (net_idx = 0; net_idx < num_logical_nets; net_idx++) {
		vpack_net_power[net_idx].probability = -1.0;
		vpack_net_power[net_idx].density = -1.0;
	}

	act_file_hdl = vtr::fopen(activity_file, "r");
	if (act_file_hdl == NULL) {
		vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
				"Error: could not open activity file: %s\n", activity_file);
	}

	fail = false;
	ptr = vtr::fgets(buf, vtr::BUFSIZE, act_file_hdl);
	while (ptr != NULL) {
		word1 = strtok(buf, TOKENS);
		word2 = strtok(NULL, TOKENS);
		word3 = strtok(NULL, TOKENS);
		//printf("word1:%s|word2:%s|word3:%s\n", word1, word2, word3);
		fail |= add_activity_to_net(word1, atof(word2), atof(word3));

		ptr = vtr::fgets(buf, vtr::BUFSIZE, act_file_hdl);
	}
	fclose(act_file_hdl);

	/* Make sure all nets have an activity value */
	for (net_idx = 0; net_idx < num_logical_nets; net_idx++) {
		if (vpack_net_power[net_idx].probability < 0.0
				|| vpack_net_power[net_idx].density < 0.0) {
			vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
					"Error: Activity file does not contain signal %s\n",
					vpack_net[net_idx].name);
			fail = true;
		}
	}
}

bool add_activity_to_net(char * net_name, float probability, float density) {
	int hash_idx, net_idx;
	struct s_hash * h_ptr;

	hash_idx = hash_value(net_name);
	h_ptr = blif_hash[hash_idx];

	while (h_ptr != NULL) {
		if (strcmp(h_ptr->name, net_name) == 0) {
			net_idx = h_ptr->index;
			vpack_net_power[net_idx].probability = probability;
			vpack_net_power[net_idx].density = density;
			return false;
		}
		h_ptr = h_ptr->next;
	}

	printf(
			"Error: net %s found in activity file, but it does not exist in the .blif file.\n",
			net_name);
	return true;
}
