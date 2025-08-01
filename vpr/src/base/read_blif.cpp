/**
 * @file
 * @brief BLIF Netlist Loader
 *
 * This loader handles loading a post-technology mapping fully flattened (i.e not
 * hierarchical) netlist in Berkely Logic Interchange Format (BLIF) file, and
 * builds a netlist data structure (AtomNetlist) from it.
 *
 * BLIF text parsing is handled by the blifparse library, while this file is responsible
 * for creating the netlist data structure.
 *
 * The main object of interest is the BlifAllocCallback struct, which implements the
 * blifparse callback interface.  The callback methods are then called when basic blif
 * primitives are encountered by the parser.  The callback methods then create the associated
 * netlist data structures.
 */
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <cctype> //std::isdigit
#include <regex>  //std::regex
#include <optional>

#include "blifparse.hpp"
#include "atom_netlist.h"

#include "logic_types.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_logic.h"
#include "vtr_digest.h"

#include "vpr_error.h"
#include "read_blif.h"
#include "hash.h"

vtr::LogicValue to_vtr_logic_value(blifparse::LogicValue);

struct BlifAllocCallback : public blifparse::Callback {
  public:
    BlifAllocCallback(e_circuit_format blif_format, AtomNetlist& main_netlist, const std::string netlist_id, const LogicalModels& models)
        : main_netlist_(main_netlist)
        , netlist_id_(netlist_id)
        , models_(models)
        , blif_format_(blif_format) {
        VTR_ASSERT(blif_format_ == e_circuit_format::BLIF
                   || blif_format_ == e_circuit_format::EBLIF);
    }

    static constexpr const char* OUTPAD_NAME_PREFIX = "out:";

  public: //Callback interface
    void start_parse() override {}

    void finish_parse() override {
        //When parsing is finished we *move* the main netlist
        //into the user object. This ensures we never have two copies
        //(consuming twice the memory).
        size_t main_netlist_idx = determine_main_netlist_index();
        main_netlist_ = std::move(blif_models_[main_netlist_idx]);
    }

    void begin_model(std::string model_name) override {
        //Create a new model, and set it's name

        blif_models_.emplace_back(model_name, netlist_id_);
        blif_models_black_box_.emplace_back(false);
        ended_ = false;
        set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
    }

    void inputs(std::vector<std::string> input_names) override {
        LogicalModelId blk_model_id = LogicalModels::MODEL_INPUT_ID;
        const t_model& blk_model = models_.get_model(blk_model_id);

        VTR_ASSERT_MSG(!blk_model.inputs, "Inpad model has an input port");
        VTR_ASSERT_MSG(blk_model.outputs, "Inpad model has no output port");
        VTR_ASSERT_MSG(blk_model.outputs->size == 1, "Inpad model has non-single-bit output port");
        VTR_ASSERT_MSG(!blk_model.outputs->next, "Inpad model has multiple output ports");

        std::string pin_name = blk_model.outputs->name;
        for (const auto& input : input_names) {
            AtomBlockId blk_id = curr_model().create_block(input, blk_model_id);
            AtomPortId port_id = curr_model().create_port(blk_id, blk_model.outputs);
            AtomNetId net_id = curr_model().create_net(input);
            curr_model().create_pin(port_id, 0, net_id, PinType::DRIVER);
        }
        set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
    }

    void outputs(std::vector<std::string> output_names) override {
        LogicalModelId blk_model_id = LogicalModels::MODEL_OUTPUT_ID;
        const t_model& blk_model = models_.get_model(blk_model_id);

        VTR_ASSERT_MSG(!blk_model.outputs, "Outpad model has an output port");
        VTR_ASSERT_MSG(blk_model.inputs, "Outpad model has no input port");
        VTR_ASSERT_MSG(blk_model.inputs->size == 1, "Outpad model has non-single-bit input port");
        VTR_ASSERT_MSG(!blk_model.inputs->next, "Outpad model has multiple input ports");

        std::string pin_name = blk_model.inputs->name;
        for (const auto& output : output_names) {
            //Since we name blocks based on their drivers we need to uniquify outpad names,
            //which we do with a prefix
            AtomBlockId blk_id = curr_model().create_block(OUTPAD_NAME_PREFIX + output, blk_model_id);
            AtomPortId port_id = curr_model().create_port(blk_id, blk_model.inputs);
            AtomNetId net_id = curr_model().create_net(output);
            curr_model().create_pin(port_id, 0, net_id, PinType::SINK);
        }
        set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
    }

    void names(std::vector<std::string> nets, std::vector<std::vector<blifparse::LogicValue>> so_cover) override {
        LogicalModelId blk_model_id = LogicalModels::MODEL_NAMES_ID;
        const t_model& blk_model = models_.get_model(blk_model_id);

        VTR_ASSERT_MSG(nets.size() > 0, "BLIF .names has no connections");

        VTR_ASSERT_MSG(blk_model.inputs, ".names model has no input port");
        VTR_ASSERT_MSG(!blk_model.inputs->next, ".names model has multiple input ports");
        if (static_cast<int>(nets.size()) - 1 > blk_model.inputs->size) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "BLIF .names input size (%zu) greater than .names model input size (%d)",
                      nets.size() - 1, blk_model.inputs->size);
        }

        VTR_ASSERT_MSG(blk_model.outputs, ".names has no output port");
        VTR_ASSERT_MSG(!blk_model.outputs->next, ".names model has multiple output ports");
        VTR_ASSERT_MSG(blk_model.outputs->size == 1, ".names model has non-single-bit output");

        //Convert the single-output cover to a netlist truth table
        AtomNetlist::TruthTable truth_table;
        for (const auto& row : so_cover) {
            truth_table.emplace_back();
            for (auto val : row) {
                truth_table[truth_table.size() - 1].push_back(to_vtr_logic_value(val));
            }
        }

        AtomBlockId blk_id = curr_model().create_block(nets[nets.size() - 1], blk_model_id, truth_table);
        set_curr_block(blk_id);

        //Create inputs
        AtomPortId input_port_id = curr_model().create_port(blk_id, blk_model.inputs);
        for (size_t i = 0; i < nets.size() - 1; ++i) {
            AtomNetId net_id = curr_model().create_net(nets[i]);

            curr_model().create_pin(input_port_id, i, net_id, PinType::SINK);
        }

        //Figure out if the output is a constant generator
        bool output_is_const = false;
        if (truth_table.empty()
            || (truth_table.size() == 1 && truth_table[0].size() == 1 && truth_table[0][0] == vtr::LogicValue::FALSE)) {
            //An empty truth table in BLIF corresponds to a constant-zero
            //  e.g.
            //
            //  #gnd is a constant 0 generator
            //  .names gnd
            //
            //An single entry truth table with value '0' also corresponds to a constant-zero
            //  e.g.
            //
            //  #gnd2 is a constant 0 generator
            //  .names gnd2
            //  0
            //
            output_is_const = true;
            VTR_LOG("Found constant-zero generator '%s'\n", nets[nets.size() - 1].c_str());
        } else if (truth_table.size() == 1 && truth_table[0].size() == 1 && truth_table[0][0] == vtr::LogicValue::TRUE) {
            //A single-entry truth table with value '1' in BLIF corresponds to a constant-one
            //  e.g.
            //
            //  #vcc is a constant 1 generator
            //  .names vcc
            //  1
            //
            output_is_const = true;
            VTR_LOG("Found constant-one generator '%s'\n", nets[nets.size() - 1].c_str());
        }

        //Create output
        AtomNetId net_id = curr_model().create_net(nets[nets.size() - 1]);
        AtomPortId output_port_id = curr_model().create_port(blk_id, blk_model.outputs);
        curr_model().create_pin(output_port_id, 0, net_id, PinType::DRIVER, output_is_const);
    }

    void latch(std::string input, std::string output, blifparse::LatchType type, std::string control, blifparse::LogicValue init) override {
        if (type == blifparse::LatchType::UNSPECIFIED) {
            VTR_LOGF_WARN(filename_.c_str(), lineno_, "Treating latch '%s' of unspecified type as rising edge triggered\n", output.c_str());
        } else if (type != blifparse::LatchType::RISING_EDGE) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Only rising edge latches supported\n");
        }

        if (control.empty()) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Latch must have a clock\n");
        }

        LogicalModelId blk_model_id = LogicalModels::MODEL_LATCH_ID;
        const t_model& blk_model = models_.get_model(blk_model_id);

        VTR_ASSERT_MSG(blk_model.inputs, "Has one input port");
        VTR_ASSERT_MSG(blk_model.inputs->next, "Has two input port");
        VTR_ASSERT_MSG(!blk_model.inputs->next->next, "Has no more than two input port");
        VTR_ASSERT_MSG(blk_model.outputs, "Has one output port");
        VTR_ASSERT_MSG(!blk_model.outputs->next, "Has no more than one input port");

        const t_model_ports* d_model_port = blk_model.inputs;
        const t_model_ports* clk_model_port = blk_model.inputs->next;
        const t_model_ports* q_model_port = blk_model.outputs;

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

        AtomBlockId blk_id = curr_model().create_block(output, blk_model_id, truth_table);
        set_curr_block(blk_id);

        //The input
        AtomPortId d_port_id = curr_model().create_port(blk_id, d_model_port);
        AtomNetId d_net_id = curr_model().create_net(input);
        curr_model().create_pin(d_port_id, 0, d_net_id, PinType::SINK);

        //The output
        AtomPortId q_port_id = curr_model().create_port(blk_id, q_model_port);
        AtomNetId q_net_id = curr_model().create_net(output);
        curr_model().create_pin(q_port_id, 0, q_net_id, PinType::DRIVER);

        //The clock
        AtomPortId clk_port_id = curr_model().create_port(blk_id, clk_model_port);
        AtomNetId clk_net_id = curr_model().create_net(control);
        curr_model().create_pin(clk_port_id, 0, clk_net_id, PinType::SINK);
    }

    void subckt(std::string subckt_model, std::vector<std::string> ports, std::vector<std::string> nets) override {
        VTR_ASSERT(ports.size() == nets.size());

        LogicalModelId blk_model_id = models_.get_model_by_name(subckt_model);
        const t_model& blk_model = models_.get_model(blk_model_id);

        //We name the subckt based on the net it's first output pin drives
        std::string subckt_name;
        for (size_t i = 0; i < ports.size(); ++i) {
            const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
            VTR_ASSERT(model_port);

            if (model_port->dir == OUT_PORT) {
                subckt_name = nets[i];
                break;
            }
        }

        if (subckt_name.empty()) {
            //We need to name the block uniquely ourselves since there is no connected output
            subckt_name = unique_subckt_name();

            //Since this is unusual, warn the user
            VTR_LOGF_WARN(filename_.c_str(), lineno_,
                          "Subckt of type '%s' at %s:%d has no output pins, and has been named '%s'\n",
                          blk_model.name, filename_.c_str(), lineno_, subckt_name.c_str());
        }

        //The name for every block should be unique, check that there is no name conflict
        AtomBlockId blk_id = curr_model().find_block(subckt_name);
        if (blk_id) {
            LogicalModelId conflicting_model = curr_model().block_model(blk_id);
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
                      "Duplicate blocks named '%s' found in netlist."
                      " Existing block of type '%s' conflicts with subckt of type '%s'.",
                      subckt_name.c_str(), models_.get_model(conflicting_model).name, subckt_model.c_str());
        }

        //Create the block
        blk_id = curr_model().create_block(subckt_name, blk_model_id);
        set_curr_block(blk_id);

        for (size_t i = 0; i < ports.size(); ++i) {
            //Check for consistency between model and ports
            const t_model_ports* model_port = find_model_port(blk_model, ports[i]);
            VTR_ASSERT(model_port);

            //Determine the pin type
            PinType pin_type = PinType::SINK;
            if (model_port->dir == OUT_PORT) {
                pin_type = PinType::DRIVER;
            } else {
                VTR_ASSERT_MSG(model_port->dir == IN_PORT, "Unexpected port type");
            }

            //Make the port
            std::string port_base;
            size_t port_bit;
            std::tie(port_base, port_bit) = split_index(ports[i]);

            AtomPortId port_id = curr_model().create_port(blk_id, find_model_port(blk_model, port_base));

            //Make the net
            AtomNetId net_id = curr_model().create_net(nets[i]);

            //Make the pin
            curr_model().create_pin(port_id, port_bit, net_id, pin_type);
        }
    }

    void blackbox() override {
        //We treat black-boxes as netlists during parsing so they should contain
        //only inpads/outpads
        for (const auto& blk_id : curr_model().blocks()) {
            auto blk_type = curr_model().block_type(blk_id);
            if (!(blk_type == AtomBlockType::INPAD || blk_type == AtomBlockType::OUTPAD)) {
                vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Unexpected primitives in blackbox model");
            }
        }
        set_curr_model_blackbox(true);
        set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
    }

    void end_model() override {
        if (ended_) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Unexpected .end");
        }

        merge_conn_nets();

        //Mark as ended
        ended_ = true;

        set_curr_block(AtomBlockId::INVALID()); //This statement doesn't define a block, so mark invalid
    }

    //BLIF Extensions
    void conn(std::string src, std::string dst) override {
        if (blif_format_ != e_circuit_format::EBLIF) {
            parse_error(lineno_, ".conn", "Supported only in extended BLIF format");
        }

        //We allow the .conn to create the nets if they don't exist,
        //however typically they will have already been defined.
        AtomNetId driver_net = curr_model().create_net(src);
        AtomNetId sink_net = curr_model().create_net(dst);

        //We eventually need to merge the driver and sink nets,
        //however we must defer that until all the net drivers
        //and sinks have been created (otherwise they may not
        //be properly merged depending on where the .conn is
        //delcared).
        //
        //As a result we record the nets to merge and do the actual merging at
        //the end of the .model
        curr_nets_to_merge_.emplace_back(driver_net, sink_net);

        set_curr_block(AtomBlockId::INVALID());
    }

    void cname(std::string cell_name) override {
        if (blif_format_ != e_circuit_format::EBLIF) {
            parse_error(lineno_, ".cname", "Supported only in extended BLIF format");
        }

        //Re-name the block
        curr_model().set_block_name(curr_block(), cell_name);
    }

    void attr(std::string name, std::string value) override {
        if (blif_format_ != e_circuit_format::EBLIF) {
            parse_error(lineno_, ".attr", "Supported only in extended BLIF format");
        }

        curr_model().set_block_attr(curr_block(), name, value);
    }

    void param(std::string name, std::string value) override {
        if (blif_format_ != e_circuit_format::EBLIF) {
            parse_error(lineno_, ".param", "Supported only in extended BLIF format");
        }

        // Validate the parameter value
        bool is_valid = is_string_param(value) || is_binary_param(value) || is_real_param(value);

        if (!is_valid) {
            parse_error(lineno_, ".param", "Incorrect parameter value specification");
        }

        curr_model().set_block_param(curr_block(), name, value);
    }

    //Utilities
    void filename(std::string fname) override { filename_ = fname; }

    void lineno(int line_num) override { lineno_ = line_num; }

    void parse_error(const int curr_lineno, const std::string& near_text, const std::string& msg) override {
        vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), curr_lineno,
                  "Error in blif file near '%s': %s\n", near_text.c_str(), msg.c_str());
    }

  public:
    //Retrieve the netlist
    size_t determine_main_netlist_index() {
        //Look through all the models loaded, to find the one which is non-blackbox (i.e. has real blocks
        //and is not a blackbox).  To check for errors we look at all models, even if we've already
        //found a non-blackbox model.

        std::optional<size_t> top_model_idx; //Not valid yet

        for (size_t i = 0; i < blif_models_.size(); ++i) {
            if (!blif_models_black_box_[i]) {
                //A non-blackbox model
                if (!top_model_idx.has_value()) {
                    // Top model is first non-blackbox model found
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

        if (!top_model_idx.has_value()) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
                      "No non-blackbox models found. The main model must not be a blackbox.");
        }

        //Return the main model
        return top_model_idx.value();
    }

  private:
    const t_model_ports* find_model_port(const t_model& blk_model, const std::string& port_name) {
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
        for (const t_model_ports* ports : {blk_model.inputs, blk_model.outputs}) {
            const t_model_ports* curr_port = ports;
            while (curr_port) {
                if (trimmed_port_name == curr_port->name) {
                    //Found a matching port, we need to verify the index
                    if (bit_index < curr_port->size) {
                        //Valid port index
                        return curr_port;
                    } else {
                        //Out of range
                        vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
                                  "Port '%s' on architecture model '%s' exceeds port width (%d bits)\n",
                                  port_name.c_str(), blk_model.name, curr_port->size);
                    }
                }
                curr_port = curr_port->next;
            }
        }

        //No match
        vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_,
                  "Found no matching port '%s' on architecture model '%s'\n",
                  port_name.c_str(), blk_model.name);
        return nullptr;
    }

    /** @brief Splits the index off a signal name and returns the base signal name
     *         (excluding the index) and the index as an integer. For example
     *
     * "my_signal_name[2]"   -> "my_signal_name", 2
     */
    std::pair<std::string, int> split_index(const std::string& signal_name) {
        int bit_index = 0;

        std::string trimmed_signal_name = signal_name;

        auto iter = --signal_name.end(); //Initialized to the last char
        if (*iter == ']') {
            //The name may end in an index
            //
            //To verify, we iterate back through the string until we find
            //an open square brackets, or non digit character
            --iter; //Advance past ']'
            while (iter != signal_name.begin() && std::isdigit(*iter))
                --iter;

            //We are at the first non-digit character from the end (or the beginning of the string)
            if (*iter == '[') {
                //We have a valid index in the open range (iter, --signal_name.end())
                std::string index_str(iter + 1, --signal_name.end());

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

    ///@brief Retieves a reference to the currently active .model
    AtomNetlist& curr_model() {
        if (blif_models_.empty() || ended_) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "Expected .model");
        }

        return blif_models_[blif_models_.size() - 1];
    }

    void set_curr_model_blackbox(bool val) {
        VTR_ASSERT(blif_models_.size() == blif_models_black_box_.size());
        blif_models_black_box_[blif_models_black_box_.size() - 1] = val;
    }

    bool verify_blackbox_model(AtomNetlist& blif_model) {
        LogicalModelId arch_model_id = models_.get_model_by_name(blif_model.netlist_name());

        if(!arch_model_id.is_valid()) {
            vpr_throw(VPR_ERROR_BLIF_F, filename_.c_str(), lineno_, "BLIF model '%s' has no equivalent architecture model.",
                    blif_model.netlist_name().c_str());
        }

        const t_model& arch_model = models_.get_model(arch_model_id);

        //Verify each port on the model
        //
        // We parse each .model as its own netlist so the IOs
        // get converted to blocks
        for (auto blk_id : blif_model.blocks()) {
            //Check that the port directions match
            if (blif_model.block_type(blk_id) == AtomBlockType::INPAD) {
                const auto& input_name = blif_model.block_name(blk_id);

                //Find model port already verifies the port widths
                const t_model_ports* arch_model_port = find_model_port(arch_model, input_name);
                VTR_ASSERT(arch_model_port);
                VTR_ASSERT(arch_model_port->dir == IN_PORT);

            } else {
                VTR_ASSERT(blif_model.block_type(blk_id) == AtomBlockType::OUTPAD);

                const auto& raw_output_name = blif_model.block_name(blk_id);

                std::string output_name = vtr::replace_first(raw_output_name, OUTPAD_NAME_PREFIX, "");

                //Find model port already verifies the port widths
                const t_model_ports* arch_model_port = find_model_port(arch_model, output_name);
                VTR_ASSERT(arch_model_port);

                VTR_ASSERT(arch_model_port->dir == OUT_PORT);
            }
        }
        return true;
    }

    ///@brief Returns a different unique subck name each time it is called
    std::string unique_subckt_name() {
        return "unnamed_subckt" + std::to_string(unique_subckt_name_counter_++);
    }

    /** @brief Sets the current block which is being processed
     *
     * Used to determine which block a .cname, .param, .attr apply to
     */
    void set_curr_block(AtomBlockId blk) {
        curr_block_ = blk;
    }

    ///@brief Gets the current block which is being processed
    AtomBlockId curr_block() const {
        return curr_block_;
    }

    /**
     * @brief Merges all the recorded net pairs which need to be merged
     *
     * This should only be called at the end of a .model to ensure that
     * all the associated driver/sink pins have been declared and connected
     * to their nets
     */
    void merge_conn_nets() {
        for (auto net_pair : curr_nets_to_merge_) {
            curr_model().merge_nets(net_pair.first, net_pair.second);
        }

        curr_nets_to_merge_.clear();
    }

  private:
    bool ended_ = true; //Initially no active .model
    std::string filename_ = "";
    int lineno_ = -1;

    std::vector<AtomNetlist> blif_models_;
    std::vector<bool> blif_models_black_box_;

    AtomNetlist& main_netlist_;    ///<User object we fill
    const std::string netlist_id_; ///<Unique identifier based on the contents of the blif file
    const LogicalModels& models_;

    size_t unique_subckt_name_counter_ = 0;

    AtomBlockId curr_block_;
    std::vector<std::pair<AtomNetId, AtomNetId>> curr_nets_to_merge_;

    e_circuit_format blif_format_ = e_circuit_format::BLIF;
};

vtr::LogicValue to_vtr_logic_value(blifparse::LogicValue val) {
    vtr::LogicValue new_val = vtr::LogicValue::UNKOWN;
    switch (val) {
        case blifparse::LogicValue::TRUE:
            new_val = vtr::LogicValue::TRUE;
            break;
        case blifparse::LogicValue::FALSE:
            new_val = vtr::LogicValue::FALSE;
            break;
        case blifparse::LogicValue::DONT_CARE:
            new_val = vtr::LogicValue::DONT_CARE;
            break;
        case blifparse::LogicValue::UNKOWN:
            new_val = vtr::LogicValue::UNKOWN;
            break;
        default:
            VTR_ASSERT_OPT_MSG(false, "Unknown logic value");
    }
    return new_val;
}

bool is_string_param(const std::string& param) {
    // Empty param is considered a string
    if (param.empty()) {
        return true;
    }

    // There have to be at least 2 characters (the quotes)
    if (param.length() < 2) {
        return false;
    }

    // The first and the last characters must be quotes
    size_t len = param.length();
    if (param[0] != '"' || param[len - 1] != '"') {
        return false;
    }

    // There mustn't be any other quotes except for escaped ones
    for (size_t i = 1; i < (len - 1); ++i) {
        if (param[i] == '"' && param[i - 1] != '\\') {
            return false;
        }
    }

    // This is a string param
    return true;
}

bool is_binary_param(const std::string& param) {
    // Must be non-empty
    if (param.empty()) {
        return false;
    }

    // The string must contain only '0' and '1'
    for (size_t i = 0; i < param.length(); ++i) {
        if (param[i] != '0' && param[i] != '1') {
            return false;
        }
    }

    // This is a binary word param
    return true;
}

bool is_real_param(const std::string& param) {
    /* Must be non-empty */
    if (param.empty()) {
        return false;
    }

    // The string must match the regular expression
    const std::regex real_number_expr("[+-]?([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)");
    if (!std::regex_match(param, real_number_expr)) {
        return false;
    }

    // This is a real number param
    return true;
}

AtomNetlist read_blif(e_circuit_format circuit_format,
                      const char* blif_file,
                      const LogicalModels& models) {
    AtomNetlist netlist;
    std::string netlist_id = vtr::secure_digest_file(blif_file);

    BlifAllocCallback alloc_callback(circuit_format, netlist, netlist_id, models);
    blifparse::blif_parse_filename(blif_file, alloc_callback);

    return netlist;
}
