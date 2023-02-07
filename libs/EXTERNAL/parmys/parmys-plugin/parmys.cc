/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "kernel/celltypes.h"
#include "kernel/yosys.h"

#include <regex>

#include "netlist_utils.h"
#include "odin_globals.h"
#include "odin_ii.h"
#include "odin_util.h"

#include "vtr_memory.h"
#include "vtr_path.h"
#include "vtr_util.h"

#include "partial_map.h"

#include "netlist_visualizer.h"

#include "parmys_resolve.hpp"

#include "adder.h"
#include "arch_util.h"
#include "block_memory.h"
#include "hard_block.h"
#include "memory.h"
#include "multiplier.h"
#include "netlist_cleanup.h"
#include "netlist_statistic.h"
#include "netlist_visualizer.h"
#include "read_xml_config_file.h"
#include "subtractor.h"

#include "ast_util.h"
#include "parmys_update.hpp"
#include "parmys_utils.hpp"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

#define GND_NAME "$false"
#define VCC_NAME "$true"
#define HBPAD_NAME "$undef"

struct Bbox {
    std::string name;
    std::vector<std::string> inputs, outputs;
};

CellTypes ct;

struct ParMYSPass : public Pass {

    static void hook_up_nets(netlist_t *odin_netlist, Hashtable *output_nets_hash)
    {
        for (int i = 0; i < odin_netlist->num_internal_nodes; i++) {
            nnode_t *node = odin_netlist->internal_nodes[i];
            hook_up_node(node, output_nets_hash);
        }

        for (int i = 0; i < odin_netlist->num_ff_nodes; i++) {
            nnode_t *node = odin_netlist->ff_nodes[i];
            hook_up_node(node, output_nets_hash);
        }

        for (int i = 0; i < odin_netlist->num_top_output_nodes; i++) {
            nnode_t *node = odin_netlist->top_output_nodes[i];
            hook_up_node(node, output_nets_hash);
        }
    }

    static void hook_up_node(nnode_t *node, Hashtable *output_nets_hash)
    {
        for (int j = 0; j < node->num_input_pins; j++) {
            npin_t *input_pin = node->input_pins[j];

            nnet_t *output_net = (nnet_t *)output_nets_hash->get(input_pin->name);

            if (!output_net) {
                log_error("Error: Could not hook up the pin %s: not available, related node: %s.", input_pin->name, node->name);
            }
            add_fanout_pin_to_net(output_net, input_pin);
        }
    }

    static void build_top_output_node(const char *name_str, netlist_t *odin_netlist)
    {
        nnode_t *new_node = allocate_nnode(my_location);
        new_node->related_ast_node = NULL;
        new_node->type = OUTPUT_NODE;
        new_node->name = vtr::strdup(name_str);
        allocate_more_input_pins(new_node, 1);
        add_input_port_information(new_node, 1);

        npin_t *new_pin = allocate_npin();
        new_pin->name = vtr::strdup(name_str);
        add_input_pin_to_node(new_node, new_pin, 0);

        odin_netlist->top_output_nodes =
          (nnode_t **)vtr::realloc(odin_netlist->top_output_nodes, sizeof(nnode_t *) * (odin_netlist->num_top_output_nodes + 1));
        odin_netlist->top_output_nodes[odin_netlist->num_top_output_nodes++] = new_node;
    }

    static void build_top_input_node(const char *name_str, netlist_t *odin_netlist, Hashtable *output_nets_hash)
    {
        loc_t my_loc;
        nnode_t *new_node = allocate_nnode(my_loc);

        new_node->related_ast_node = NULL;
        new_node->type = INPUT_NODE;

        new_node->name = vtr::strdup(name_str);

        allocate_more_output_pins(new_node, 1);
        add_output_port_information(new_node, 1);

        npin_t *new_pin = allocate_npin();
        new_pin->name = vtr::strdup(name_str);
        new_pin->type = OUTPUT;

        add_output_pin_to_node(new_node, new_pin, 0);

        nnet_t *new_net = allocate_nnet();
        new_net->name = vtr::strdup(name_str);

        add_driver_pin_to_net(new_net, new_pin);

        odin_netlist->top_input_nodes =
          (nnode_t **)vtr::realloc(odin_netlist->top_input_nodes, sizeof(nnode_t *) * (odin_netlist->num_top_input_nodes + 1));
        odin_netlist->top_input_nodes[odin_netlist->num_top_input_nodes++] = new_node;

        output_nets_hash->add(name_str, new_net);
    }

    static void create_top_driver_nets(netlist_t *odin_netlist, Hashtable *output_nets_hash)
    {
        npin_t *new_pin;

        /* ZERO net */
        odin_netlist->zero_net = allocate_nnet();
        odin_netlist->gnd_node = allocate_nnode(unknown_location);
        odin_netlist->gnd_node->type = GND_NODE;
        allocate_more_output_pins(odin_netlist->gnd_node, 1);
        add_output_port_information(odin_netlist->gnd_node, 1);
        new_pin = allocate_npin();
        add_output_pin_to_node(odin_netlist->gnd_node, new_pin, 0);
        add_driver_pin_to_net(odin_netlist->zero_net, new_pin);

        /*ONE net*/
        odin_netlist->one_net = allocate_nnet();
        odin_netlist->vcc_node = allocate_nnode(unknown_location);
        odin_netlist->vcc_node->type = VCC_NODE;
        allocate_more_output_pins(odin_netlist->vcc_node, 1);
        add_output_port_information(odin_netlist->vcc_node, 1);
        new_pin = allocate_npin();
        add_output_pin_to_node(odin_netlist->vcc_node, new_pin, 0);
        add_driver_pin_to_net(odin_netlist->one_net, new_pin);

        /* Pad net */
        odin_netlist->pad_net = allocate_nnet();
        odin_netlist->pad_node = allocate_nnode(unknown_location);
        odin_netlist->pad_node->type = PAD_NODE;
        allocate_more_output_pins(odin_netlist->pad_node, 1);
        add_output_port_information(odin_netlist->pad_node, 1);
        new_pin = allocate_npin();
        add_output_pin_to_node(odin_netlist->pad_node, new_pin, 0);
        add_driver_pin_to_net(odin_netlist->pad_net, new_pin);

        /* CREATE the driver for the ZERO */
        odin_netlist->zero_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, zero_string, -1);
        output_nets_hash->add(GND_NAME, odin_netlist->zero_net);

        /* CREATE the driver for the ONE and store twice */
        odin_netlist->one_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, one_string, -1);
        output_nets_hash->add(VCC_NAME, odin_netlist->one_net);

        /* CREATE the driver for the PAD */
        odin_netlist->pad_net->name = make_full_ref_name(odin_netlist->identifier, NULL, NULL, pad_string, -1);
        output_nets_hash->add(HBPAD_NAME, odin_netlist->pad_net);

        odin_netlist->vcc_node->name = vtr::strdup(VCC_NAME);
        odin_netlist->gnd_node->name = vtr::strdup(GND_NAME);
        odin_netlist->pad_node->name = vtr::strdup(HBPAD_NAME);
    }

    static char *sig_full_ref_name_sig(RTLIL::SigBit sig, pool<SigBit> &cstr_bits_seen)
    {

        cstr_bits_seen.insert(sig);

        if (sig.wire == NULL) {
            if (sig == RTLIL::State::S0)
                return vtr::strdup(GND_NAME);
            else if (sig == RTLIL::State::S1)
                return vtr::strdup(VCC_NAME);
            else
                return vtr::strdup(HBPAD_NAME);
        } else {
            std::string str = RTLIL::unescape_id(sig.wire->name);
            if (sig.wire->width == 1)
                return make_full_ref_name(NULL, NULL, NULL, str.c_str(), -1);
            else {
                int idx = sig.wire->upto ? sig.wire->start_offset + sig.wire->width - sig.offset - 1 : sig.wire->start_offset + sig.offset;
                return make_full_ref_name(NULL, NULL, NULL, str.c_str(), idx);
            }
        }
    }

    static void map_input_port(const RTLIL::IdString &mapping, SigSpec in_port, nnode_t *node, pool<SigBit> &cstr_bits_seen)
    {

        int base_pin_idx = node->num_input_pins;

        allocate_more_input_pins(node, in_port.size());
        add_input_port_information(node, in_port.size());

        for (int i = 0; i < in_port.size(); i++) {

            char *in_pin_name = sig_full_ref_name_sig(in_port[i], cstr_bits_seen);

            npin_t *in_pin = allocate_npin();
            in_pin->name = vtr::strdup(in_pin_name);
            in_pin->mapping = vtr::strdup(RTLIL::unescape_id(mapping).c_str());
            add_input_pin_to_node(node, in_pin, base_pin_idx + i);

            vtr::free(in_pin_name);
        }
    }

    static void map_output_port(const RTLIL::IdString &mapping, SigSpec out_port, nnode_t *node, Hashtable *output_nets_hash,
                                pool<SigBit> &cstr_bits_seen)
    {

        int base_pin_idx = node->num_output_pins;

        allocate_more_output_pins(node, out_port.size()); //?
        add_output_port_information(node, out_port.size());

        /*add name information and a net(driver) for the output */

        for (int i = 0; i < out_port.size(); i++) {
            npin_t *out_pin = allocate_npin();
            out_pin->name = NULL;
            out_pin->mapping = vtr::strdup(RTLIL::unescape_id(mapping).c_str());
            add_output_pin_to_node(node, out_pin, base_pin_idx + i);

            char *output_pin_name = sig_full_ref_name_sig(out_port[i], cstr_bits_seen);
            nnet_t *out_net = (nnet_t *)output_nets_hash->get(output_pin_name);
            if (out_net == nullptr) {
                out_net = allocate_nnet();
                out_net->name = vtr::strdup(output_pin_name);
                output_nets_hash->add(output_pin_name, out_net);
            }
            add_driver_pin_to_net(out_net, out_pin);

            vtr::free(output_pin_name);
        }
    }

    static bool is_param_required(operation_list op)
    {
        switch (op) {
        case (SL):
        case (SR):
        case (ASL):
        case (ASR):
        case (DLATCH):
        case (ADLATCH):
        case (SETCLR):
        case (SDFF):
        case (DFFE):
        case (SDFFE):
        case (SDFFCE):
        case (DFFSR):
        case (DFFSRE):
        case (SPRAM):
        case (DPRAM):
        case (YMEM):
        case (YMEM2):
        case (FF_NODE):
            return true;
        default:
            return false;
        }
    }

    static operation_list from_yosys_type(RTLIL::IdString type)
    {
        if (type == ID($add)) {
            return ADD;
        }
        if (type == ID($mem)) {
            return YMEM;
        }
        if (type == ID($mem_v2)) {
            return YMEM2;
        }
        if (type == ID($mul)) {
            return MULTIPLY;
        }
        if (type == ID($sub)) {
            return MINUS;
        }
        if (type == ID(LUT_K)) {
            return SKIP;
        }
        if (type == ID(DFF)) {
            return FF_NODE;
        }
        if (type == ID(fpga_interconnect)) {
            return operation_list_END;
        }
        if (type == ID(mux)) {
            return SMUX_2;
        }
        if (type == ID(adder)) {
            return ADD;
        }
        if (type == ID(multiply)) {
            return MULTIPLY;
        }
        if (type == ID(single_port_ram)) {
            return SPRAM;
        }
        if (type == ID(dual_port_ram)) {
            return DPRAM;
        }

        if (RTLIL::builtin_ff_cell_types().count(type)) {
            return SKIP;
        }

        if (ct.cell_known(type)) {
            return SKIP;
        }

        return NO_OP;
    }

    static netlist_t *to_netlist(RTLIL::Module *top_module, RTLIL::Design *design)
    {
        ct.setup();

        std::vector<RTLIL::Module *> mod_list;

        std::vector<RTLIL::Module *> black_list;

        std::string top_module_name;
        if (top_module_name.empty())
            for (auto module : design->modules())
                if (module->get_bool_attribute(ID::top))
                    top_module_name = module->name.str();

        for (auto module : design->modules()) {

            if (module->processes.size() != 0) {
                log_error("Found unmapped processes in module %s: unmapped processes are not supported in parmys pass!\n", log_id(module->name));
            }

            if (module->memories.size() != 0) {
                log_error("Found unmapped memories in module %s: unmapped memories are not supported in parmys pass!\n", log_id(module->name));
            }

            if (module->name == RTLIL::escape_id(top_module_name)) {
                top_module_name.clear();
                continue;
            }

            mod_list.push_back(module);
        }

        pool<SigBit> cstr_bits_seen;

        netlist_t *odin_netlist = allocate_netlist();
        odin_netlist->design = design;
        Hashtable *output_nets_hash = new Hashtable();
        odin_netlist->identifier = vtr::strdup(log_id(top_module->name));

        create_top_driver_nets(odin_netlist, output_nets_hash);

        // build_top_input_node(DEFAULT_CLOCK_NAME, odin_netlist, output_nets_hash);

        std::map<int, RTLIL::Wire *> inputs, outputs;

        for (auto wire : top_module->wires()) {
            if (wire->port_input)
                inputs[wire->port_id] = wire;
            if (wire->port_output)
                outputs[wire->port_id] = wire;
        }

        for (auto &it : inputs) {
            RTLIL::Wire *wire = it.second;
            for (int i = 0; i < wire->width; i++) {
                char *name_string = sig_full_ref_name_sig(RTLIL::SigBit(wire, i), cstr_bits_seen);
                build_top_input_node(name_string, odin_netlist, output_nets_hash);
                vtr::free(name_string);
            }
        }

        for (auto &it : outputs) {
            RTLIL::Wire *wire = it.second;
            for (int i = 0; i < wire->width; i++) {
                char *name_string = sig_full_ref_name_sig(RTLIL::SigBit(wire, i), cstr_bits_seen);
                build_top_output_node(name_string, odin_netlist);
                vtr::free(name_string);
            }
        }

        long hard_id = 0;
        for (auto cell : top_module->cells()) {

            nnode_t *new_node = allocate_nnode(my_location);

            for (auto &param : cell->parameters) {
                new_node->cell_parameters[RTLIL::IdString(param.first)] = Const(param.second);
            }

            new_node->related_ast_node = NULL;

            // new_node->type = yosys_subckt_strmap[str(cell->type).c_str()];
            new_node->type = from_yosys_type(cell->type);

            // check primitive node type is alreday mapped before or not (blackboxed)
            if (new_node->type == SPRAM || new_node->type == DPRAM || new_node->type == ADD || new_node->type == MULTIPLY) {
                if (design->module(cell->type) != nullptr && design->module(cell->type)->get_blackbox_attribute()) {
                    new_node->type = SKIP;
                }
            }

            if (new_node->type == NO_OP) {

                /**
                 *  according to ast.cc:1657-1663
                 *
                 * 	std::string modname;
                 *	if (parameters.size() == 0)
                 *		modname = stripped_name;
                 *	else if (para_info.size() > 60)
                 *		modname = "$paramod$" + sha1(para_info) + stripped_name;
                 *	else
                 *		modname = "$paramod" + stripped_name + para_info;
                 */

                if (cell->type.begins_with("$paramod$")) // e.g. $paramod$b509a885304d9c8c49f505bb9d0e99a9fb676562\dual_port_ram
                {
                    std::regex regex("^\\$paramod\\$\\w+\\\\(\\w+)$");
                    std::smatch m;
                    std::string modname(str(cell->type));
                    if (regex_match(modname, m, regex)) {
                        new_node->type = yosys_subckt_strmap[m.str(1).c_str()];
                    }
                } else if (cell->type.begins_with("$paramod\\")) // e.g. $paramod\dual_port_ram\ADDR_WIDTH?4'0100\DATA_WIDTH?4'0101
                {
                    std::regex regex("^\\$paramod\\\\(\\w+)(\\\\\\S+)*$");
                    std::smatch m;
                    std::string modname(str(cell->type));
                    if (regex_match(modname, m, regex)) {
                        new_node->type = yosys_subckt_strmap[m.str(1).c_str()];
                    }
                } else if (design->module(cell->type)->get_blackbox_attribute()) {
                    new_node->type = SKIP;
                } else {
                    new_node->type = HARD_IP;
                    t_model *hb_model = find_hard_block(str(cell->type).c_str());
                    if (hb_model) {
                        hb_model->used = 1;
                    }
                    std::string modname(str(cell->type));
                    // Create a fake ast node.
                    if (new_node->type == HARD_IP) {
                        new_node->related_ast_node = create_node_w_type(HARD_BLOCK, my_location);
                        new_node->related_ast_node->children = (ast_node_t **)vtr::calloc(1, sizeof(ast_node_t *));
                        new_node->related_ast_node->identifier_node = create_tree_node_id(vtr::strdup(modname.c_str()), my_location);
                    }
                }
            }

            if (new_node->type == SKIP) {
                std::string modname(str(cell->type));
                // fake ast node.
                new_node->related_ast_node = create_node_w_type(HARD_BLOCK, my_location);
                new_node->related_ast_node->children = (ast_node_t **)vtr::calloc(1, sizeof(ast_node_t *));
                new_node->related_ast_node->identifier_node = create_tree_node_id(vtr::strdup(modname.c_str()), my_location);
            }

            for (auto &conn : cell->connections()) {

                if (cell->input(conn.first) && conn.second.size() > 0) {
                    map_input_port(conn.first, conn.second, new_node, cstr_bits_seen);
                }

                if (cell->output(conn.first) && conn.second.size() > 0) {
                    map_output_port(conn.first, conn.second, new_node, output_nets_hash, cstr_bits_seen);
                }
            }

            if (is_param_required(new_node->type)) {

                if (cell->hasParam(ID::SRST_VALUE)) {
                    auto value = vtr::strdup(cell->getParam(ID::SRST_VALUE).as_string().c_str());
                    new_node->attributes->sreset_value = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::ARST_VALUE)) {
                    auto value = vtr::strdup(cell->getParam(ID::ARST_VALUE).as_string().c_str());
                    new_node->attributes->areset_value = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::OFFSET)) {
                    auto value = vtr::strdup(cell->getParam(ID::OFFSET).as_string().c_str());
                    new_node->attributes->offset = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::SIZE)) {
                    auto value = vtr::strdup(cell->getParam(ID::SIZE).as_string().c_str());
                    new_node->attributes->size = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::WIDTH)) {
                    auto value = vtr::strdup(cell->getParam(ID::WIDTH).as_string().c_str());
                    new_node->attributes->DBITS = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::RD_PORTS)) {
                    auto value = vtr::strdup(cell->getParam(ID::RD_PORTS).as_string().c_str());
                    new_node->attributes->RD_PORTS = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::WR_PORTS)) {
                    auto value = vtr::strdup(cell->getParam(ID::WR_PORTS).as_string().c_str());
                    new_node->attributes->WR_PORTS = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::ABITS)) {
                    auto value = vtr::strdup(cell->getParam(ID::ABITS).as_string().c_str());
                    new_node->attributes->ABITS = std::bitset<sizeof(long) * 8>(value).to_ulong();
                    vtr::free(value);
                }

                if (cell->hasParam(ID::MEMID)) {
                    auto value = vtr::strdup(cell->getParam(ID::MEMID).as_string().c_str());
                    RTLIL::IdString ids = cell->getParam(ID::MEMID).decode_string();
                    new_node->attributes->memory_id = vtr::strdup(RTLIL::unescape_id(ids).c_str());
                    vtr::free(value);
                }

                if (cell->hasParam(ID::A_SIGNED)) {
                    new_node->attributes->port_a_signed = cell->getParam(ID::A_SIGNED).as_bool() ? SIGNED : UNSIGNED;
                }

                if (cell->hasParam(ID::B_SIGNED)) {
                    new_node->attributes->port_b_signed = cell->getParam(ID::B_SIGNED).as_bool() ? SIGNED : UNSIGNED;
                }

                if (cell->hasParam(ID::CLK_POLARITY)) {
                    new_node->attributes->clk_edge_type =
                      cell->getParam(ID::CLK_POLARITY).as_bool() ? RISING_EDGE_SENSITIVITY : FALLING_EDGE_SENSITIVITY;
                }

                if (cell->hasParam(ID::CLR_POLARITY)) {
                    new_node->attributes->clr_polarity =
                      cell->getParam(ID::CLR_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::SET_POLARITY)) {
                    new_node->attributes->set_polarity =
                      cell->getParam(ID::SET_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::EN_POLARITY)) {
                    new_node->attributes->enable_polarity =
                      cell->getParam(ID::EN_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::ARST_POLARITY)) {
                    new_node->attributes->areset_polarity =
                      cell->getParam(ID::ARST_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::SRST_POLARITY)) {
                    new_node->attributes->sreset_polarity =
                      cell->getParam(ID::SRST_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::RD_CLK_ENABLE)) {
                    new_node->attributes->RD_CLK_ENABLE =
                      cell->getParam(ID::RD_CLK_ENABLE).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::WR_CLK_ENABLE)) {
                    new_node->attributes->WR_CLK_ENABLE =
                      cell->getParam(ID::WR_CLK_ENABLE).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::RD_CLK_POLARITY)) {
                    new_node->attributes->RD_CLK_POLARITY =
                      cell->getParam(ID::RD_CLK_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }

                if (cell->hasParam(ID::WR_CLK_POLARITY)) {
                    new_node->attributes->WR_CLK_POLARITY =
                      cell->getParam(ID::WR_CLK_POLARITY).as_bool() ? ACTIVE_HIGH_SENSITIVITY : ACTIVE_LOW_SENSITIVITY;
                }
            }

            if (new_node->type == SMUX_2) {
                new_node->name = vtr::strdup(new_node->output_pins[0]->net->name);
            } else {
                new_node->name = vtr::strdup(
                  stringf("%s~%ld", (((new_node->type == HARD_IP /*|| new_node->type == SKIP*/) ? "\\" : "") + str(cell->type)).c_str(), hard_id++)
                    .c_str());
            }

            /*add this node to blif_netlist as an internal node */
            odin_netlist->internal_nodes =
              (nnode_t **)vtr::realloc(odin_netlist->internal_nodes, sizeof(nnode_t *) * (odin_netlist->num_internal_nodes + 1));
            odin_netlist->internal_nodes[odin_netlist->num_internal_nodes++] = new_node;
        }

        // add intermediate buffer nodes
        for (auto &conn : top_module->connections())
            for (int i = 0; i < conn.first.size(); i++) {
                SigBit lhs_bit = conn.first[i];
                SigBit rhs_bit = conn.second[i];

                if (cstr_bits_seen.count(lhs_bit) == 0)
                    continue;

                nnode_t *buf_node = allocate_nnode(my_location);

                buf_node->related_ast_node = NULL;

                buf_node->type = BUF_NODE;

                allocate_more_input_pins(buf_node, 1);
                add_input_port_information(buf_node, 1);

                char *in_pin_name = sig_full_ref_name_sig(rhs_bit, cstr_bits_seen);
                npin_t *in_pin = allocate_npin();
                in_pin->name = vtr::strdup(in_pin_name);
                in_pin->type = INPUT;
                add_input_pin_to_node(buf_node, in_pin, 0);

                vtr::free(in_pin_name);

                allocate_more_output_pins(buf_node, 1);
                add_output_port_information(buf_node, 1);

                npin_t *out_pin = allocate_npin();
                out_pin->name = NULL;
                add_output_pin_to_node(buf_node, out_pin, 0);

                char *output_pin_name = sig_full_ref_name_sig(lhs_bit, cstr_bits_seen);
                nnet_t *out_net = (nnet_t *)output_nets_hash->get(output_pin_name);
                if (out_net == nullptr) {
                    out_net = allocate_nnet();
                    out_net->name = vtr::strdup(output_pin_name);
                    output_nets_hash->add(output_pin_name, out_net);
                }
                add_driver_pin_to_net(out_net, out_pin);

                buf_node->name = vtr::strdup(output_pin_name);

                odin_netlist->internal_nodes =
                  (nnode_t **)vtr::realloc(odin_netlist->internal_nodes, sizeof(nnode_t *) * (odin_netlist->num_internal_nodes + 1));
                odin_netlist->internal_nodes[odin_netlist->num_internal_nodes++] = buf_node;

                vtr::free(output_pin_name);
            }

        hook_up_nets(odin_netlist, output_nets_hash);

        delete output_nets_hash;
        return odin_netlist;
    }

    void get_physical_luts(std::vector<t_pb_type *> &pb_lut_list, t_mode *mode)
    {
        for (int i = 0; i < mode->num_pb_type_children; i++) {
            get_physical_luts(pb_lut_list, &mode->pb_type_children[i]);
        }
    }

    void get_physical_luts(std::vector<t_pb_type *> &pb_lut_list, t_pb_type *pb_type)
    {
        if (pb_type) {
            if (pb_type->class_type == LUT_CLASS) {
                pb_lut_list.push_back(pb_type);
            } else {
                for (int i = 0; i < pb_type->num_modes; i++) {
                    get_physical_luts(pb_lut_list, &pb_type->modes[i]);
                }
            }
        }
    }

    void set_physical_lut_size(std::vector<t_logical_block_type> &logical_block_types)
    {
        std::vector<t_pb_type *> pb_lut_list;

        for (t_logical_block_type &logical_block : logical_block_types) {
            if (logical_block.index != EMPTY_TYPE_INDEX) {
                get_physical_luts(pb_lut_list, logical_block.pb_type);
            }
        }
        for (t_pb_type *pb_lut : pb_lut_list) {
            if (pb_lut) {
                if (pb_lut->num_input_pins < physical_lut_size || physical_lut_size < 1) {
                    physical_lut_size = pb_lut->num_input_pins;
                }
            }
        }
    }

    static void elaborate(netlist_t *odin_netlist)
    {
        double elaboration_time = wall_time();

        /* Perform any initialization routines here */
        find_hard_multipliers();
        find_hard_adders();
        // find_hard_adders_for_sub();
        register_hard_blocks();

        resolve_top(odin_netlist);

        elaboration_time = wall_time() - elaboration_time;
        log("\nElaboration Time: ");
        log_time(elaboration_time);
        log("\n--------------------------------------------------------------------\n");
    }

    static void optimization(netlist_t *odin_netlist)
    {
        double optimization_time = wall_time();

        if (odin_netlist) {
            /* point for all netlist optimizations. */
            log("Performing Optimization on the Netlist\n");
            if (hard_multipliers) {
                /* Perform a splitting of the multipliers for hard block mults */
                reduce_operations(odin_netlist, MULTIPLY);
                iterate_multipliers(odin_netlist);
                clean_multipliers();
            }

            if (block_memories_info.read_only_memory_list || block_memories_info.block_memory_list) {
                /* Perform a hard block registration and splitting in width for Yosys generated memory blocks */
                iterate_block_memories(odin_netlist);
                free_block_memories();
            }

            if (single_port_rams || dual_port_rams) {
                /* Perform a splitting of any hard block memories */
                iterate_memories(odin_netlist);
                free_memory_lists();
            }

            if (hard_adders) {
                /* Perform a splitting of the adders for hard block add */
                reduce_operations(odin_netlist, ADD);
                iterate_adders(odin_netlist);
                clean_adders();

                /* Perform a splitting of the adders for hard block sub */
                reduce_operations(odin_netlist, MINUS);
                iterate_adders_for_sub(odin_netlist);
                clean_adders_for_sub();
            }
        }

        optimization_time = wall_time() - optimization_time;
        log("\nOptimization Time: ");
        log_time(optimization_time);
        log("\n--------------------------------------------------------------------\n");
    }

    static void techmap(netlist_t *odin_netlist)
    {
        double techmap_time = wall_time();

        if (odin_netlist) {
            /* point where we convert netlist to FPGA or other hardware target compatible format */
            log("Performing Partial Technology Mapping to the target device\n");
            partial_map_top(odin_netlist);
            mixer->perform_optimizations(odin_netlist);

            /* Find any unused logic in the netlist and remove it */
            remove_unused_logic(odin_netlist);
        }

        techmap_time = wall_time() - techmap_time;
        log("\nTechmap Time: ");
        log_time(techmap_time);
        log("\n--------------------------------------------------------------------\n");
    }

    static void report(netlist_t *odin_netlist)
    {

        if (odin_netlist) {

            report_mult_distribution();
            report_add_distribution();
            report_sub_distribution();

            compute_statistics(odin_netlist, true);
        }
    }

    static void log_time(double time) { log("%.1fms", time * 1000); }

    ParMYSPass() : Pass("parmys", "ODIN_II partial mapper for Yosys") {}
    void help() override
    {
        log("\n");
        log("    -a ARCHITECTURE_FILE\n");
        log("        VTR FPGA architecture description file (XML)\n");
        log("\n");
        log("    -c XML_CONFIGURATION_FILE\n");
        log("        Configuration file\n");
        log("\n");
        log("    -top top_module\n");
        log("        set the specified module as design top module\n");
        log("\n");
        log("    -nopass\n");
        log("        No additional passes will be executed.\n");
        log("\n");
        log("    -exact_mults int_value\n");
        log("        To enable mixing hard block and soft logic implementation of adders\n");
        log("\n");
        log("    -mults_ratio float_value\n");
        log("        To enable mixing hard block and soft logic implementation of adders\n");
        log("\n");
        log("    -vtr_prim\n");
        log("        loads vtr primitives as modules, if the design uses vtr prmitives then this flag is mandatory for first run\n");
        log("\n");
        log("    -viz\n");
        log("        visualizes the netlist at 3 different stages: elaborated, optimized, and mapped.\n");
        log("\n");
    }
    void execute(std::vector<std::string> args, RTLIL::Design *design) override
    {
        bool flag_arch_file = false;
        bool flag_config_file = false;
        bool flag_load_vtr_primitives = false;
        bool flag_no_pass = false;
        bool flag_visualize = false;
        std::string arch_file_path;
        std::string config_file_path;
        std::string top_module_name;
        std::string DEFAULT_OUTPUT(".");

        global_args.exact_mults = -1;
        global_args.mults_ratio = -1.0;

        log_header(design, "Starting parmys pass.\n");

        size_t argidx;
        for (argidx = 1; argidx < args.size(); argidx++) {
            if (args[argidx] == "-a" && argidx + 1 < args.size()) {
                arch_file_path = args[++argidx];
                flag_arch_file = true;
                continue;
            }
            if (args[argidx] == "-c" && argidx + 1 < args.size()) {
                config_file_path = args[++argidx];
                flag_config_file = true;
                continue;
            }
            if (args[argidx] == "-top" && argidx + 1 < args.size()) {
                top_module_name = args[++argidx];
                continue;
            }
            if (args[argidx] == "-vtr_prim") {
                flag_load_vtr_primitives = true;
                continue;
            }
            if (args[argidx] == "-viz") {
                flag_visualize = true;
                continue;
            }
            if (args[argidx] == "-nopass") {
                flag_no_pass = true;
                continue;
            }
            if (args[argidx] == "-exact_mults" && argidx + 1 < args.size()) {
                global_args.exact_mults = atoi(args[++argidx].c_str());
                continue;
            }
            if (args[argidx] == "-mults_ratio" && argidx + 1 < args.size()) {
                global_args.mults_ratio = atof(args[++argidx].c_str());
                continue;
            }
        }
        extra_args(args, argidx, design);

        std::vector<t_physical_tile_type> physical_tile_types;
        std::vector<t_logical_block_type> logical_block_types;

        try {
            /* Some initialization */
            one_string = vtr::strdup(ONE_VCC_CNS);
            zero_string = vtr::strdup(ZERO_GND_ZERO);
            pad_string = vtr::strdup(ZERO_PAD_ZERO);

        } catch (vtr::VtrError &vtr_error) {
            log_error("Odin failed to initialize %s with exit code%d\n", vtr_error.what(), ERROR_INITIALIZATION);
        }

        mixer = new HardSoftLogicMixer();
        set_default_config();

        if (global_args.mults_ratio >= 0.0 && global_args.mults_ratio <= 1.0) {
            delete mixer->_opts[MULTIPLY];
            mixer->_opts[MULTIPLY] = new MultsOpt(global_args.mults_ratio);
        } else if (global_args.exact_mults >= 0) {
            delete mixer->_opts[MULTIPLY];
            mixer->_opts[MULTIPLY] = new MultsOpt(global_args.exact_mults);
        }

        configuration.coarsen = true;

        /* read the confirguration file .. get options presets the config values just in case theyr'e not read in with config file */
        if (flag_config_file) {
            log("Reading Configuration file\n");
            try {
                read_config_file(config_file_path.c_str());
            } catch (vtr::VtrError &vtr_error) {
                log_error("Odin Failed Reading Configuration file %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_CONFIG);
            }
        }

        if (flag_arch_file) {
            log("Architecture: %s\n", vtr::basename(arch_file_path).c_str());

            log("Reading FPGA Architecture file\n");
            try {
                XmlReadArch(arch_file_path.c_str(), false, &Arch, physical_tile_types, logical_block_types);
                set_physical_lut_size(logical_block_types);
            } catch (vtr::VtrError &vtr_error) {
                log_error("Odin Failed to load architecture file: %s with exit code%d at line: %ld\n", vtr_error.what(), ERROR_PARSE_ARCH,
                          vtr_error.line());
            }
        }
        log("Using Lut input width of: %d\n", physical_lut_size);

        if (!flag_no_pass) {

            if (flag_load_vtr_primitives) {
                Pass::call(design, "read_verilog -nomem2reg +/parmys/vtr_primitives.v");
                Pass::call(design, "setattr -mod -set keep_hierarchy 1 single_port_ram");
                Pass::call(design, "setattr -mod -set keep_hierarchy 1 dual_port_ram");
            }

            Pass::call(design, "parmys_arch -a " + arch_file_path);

            if (top_module_name.empty()) {
                Pass::call(design, "hierarchy -check -auto-top -purge_lib");
            } else {
                Pass::call(design, "hierarchy -check -top " + top_module_name);
            }

            Pass::call(design, "proc -norom");
            Pass::call(design, "fsm");
            Pass::call(design, "opt");
            Pass::call(design, "wreduce");
            Pass::call(design, "memory -norom");
            Pass::call(design, "check");
            Pass::call(design, "flatten");
            Pass::call(design, "opt -full");
        }

        if (design->top_module()->processes.size() != 0) {
            log_error("Found unmapped processes in top module %s: unmapped processes are not supported in parmys pass!\n",
                      log_id(design->top_module()->name));
        }

        if (design->top_module()->memories.size() != 0) {
            log_error("Found unmapped memories in module %s: unmapped memories are not supported in parmys pass!\n",
                      log_id(design->top_module()->name));
        }

        design->sort();

        log("--------------------------------------------------------------------\n");
        log("Creating Odin-II Netlist from Design\n");

        std::vector<Bbox> black_boxes;

        for (auto bb_module : design->modules()) {
            if (bb_module->get_bool_attribute(ID::blackbox)) {

                Bbox bb;

                bb.name = str(bb_module->name);

                std::map<int, RTLIL::Wire *> inputs, outputs;

                for (auto wire : bb_module->wires()) {
                    if (wire->port_input)
                        inputs[wire->port_id] = wire;
                    if (wire->port_output)
                        outputs[wire->port_id] = wire;
                }

                for (auto &it : inputs) {
                    RTLIL::Wire *wire = it.second;
                    for (int i = 0; i < wire->width; i++)
                        bb.inputs.push_back(str(RTLIL::SigSpec(wire, i)));
                }

                for (auto &it : outputs) {
                    RTLIL::Wire *wire = it.second;
                    for (int i = 0; i < wire->width; i++)
                        bb.outputs.push_back(str(RTLIL::SigSpec(wire, i)));
                }

                black_boxes.push_back(bb);
            }
        }

        netlist_t *transformed = to_netlist(design->top_module(), design);

        double synthesis_time = wall_time();

        log("--------------------------------------------------------------------\n");
        log("High-level Synthesis Begin\n");

        /* Performing elaboration for input digital circuits */
        try {
            elaborate(transformed);
            log("Successful Elaboration of the design by Odin-II\n");
            if (flag_visualize) {
                graphVizOutputNetlist(DEFAULT_OUTPUT, "netlist.elaborated.net", 111, transformed);
                log("Successful visualization of the elaborated netlist\n");
            }
        } catch (vtr::VtrError &vtr_error) {
            log_error("Odin-II Failed to parse Verilog / load BLIF file: %s with exit code:%d \n", vtr_error.what(), ERROR_ELABORATION);
        }

        /* Performing netlist optimizations */
        try {
            optimization(transformed);
            log("Successful Optimization of netlist by Odin-II\n");
            if (flag_visualize) {
                graphVizOutputNetlist(DEFAULT_OUTPUT, "netlist.optimized.net", 222, transformed);
                log("Successful visualization of the optimized netlist\n");
            }
        } catch (vtr::VtrError &vtr_error) {
            log_error("Odin-II Failed to perform netlist optimization %s with exit code:%d \n", vtr_error.what(), ERROR_OPTIMIZATION);
        }

        /* Performaing partial tech. map to the target device */
        try {
            techmap(transformed);
            log("Successful Partial Technology Mapping by Odin-II\n");
            if (flag_visualize) {
                graphVizOutputNetlist(DEFAULT_OUTPUT, "netlist.mapped.net", 333, transformed);
                log("Successful visualization of the mapped netlist\n");
            }
        } catch (vtr::VtrError &vtr_error) {
            log_error("Odin-II Failed to perform partial mapping to target device %s with exit code:%d \n", vtr_error.what(), ERROR_TECHMAP);
        }

        synthesis_time = wall_time() - synthesis_time;

        log("\nTotal Synthesis Time: ");
        log_time(synthesis_time);
        log("\n--------------------------------------------------------------------\n");
        report(transformed);
        log("\n--------------------------------------------------------------------\n");

        log("Updating the Design\n");
        Pass::call(design, "delete");

        for (auto module : design->modules()) {
            design->remove(module);
        }

        for (auto bb_module : black_boxes) {
            Module *module = nullptr;
            hashlib::dict<IdString, std::pair<int, bool>> wideports_cache;

            module = new Module;
            module->name = RTLIL::escape_id(bb_module.name);

            if (design->module(module->name)) {
                log_error("Duplicate definition of module %s!\n", log_id(module->name));
            }

            design->add(module);

            for (auto b_wire : bb_module.inputs) {
                RTLIL::Wire *wire = to_wire(b_wire, module);
                wire->port_input = true;
                std::pair<RTLIL::IdString, int> wp = wideports_split(RTLIL::unescape_id(b_wire));
                if (!wp.first.empty() && wp.second >= 0) {
                    wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
                    wideports_cache[wp.first].second = true;
                }
            }

            for (auto b_wire : bb_module.outputs) {
                RTLIL::Wire *wire = to_wire(RTLIL::unescape_id(b_wire), module);
                wire->port_output = true;
                std::pair<RTLIL::IdString, int> wp = wideports_split(RTLIL::unescape_id(b_wire));
                if (!wp.first.empty() && wp.second >= 0) {
                    wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
                    wideports_cache[wp.first].second = false;
                }
            }

            handle_wideports_cache(&wideports_cache, module);

            module->fixup_ports();
            wideports_cache.clear();

            module->attributes[ID::blackbox] = RTLIL::Const(1);
        }

        update_design(design, transformed);

        if (!flag_no_pass) {
            if (top_module_name.empty()) {
                Pass::call(design, "hierarchy -check -auto-top -purge_lib");
            } else {
                Pass::call(design, "hierarchy -check -top " + top_module_name);
            }
        }

        log("--------------------------------------------------------------------\n");

        free_netlist(transformed);

        if (Arch.models) {
            free_arch(&Arch);
            Arch.models = nullptr;
        }

        free_type_descriptors(logical_block_types);
        free_type_descriptors(physical_tile_types);

        vtr::free(transformed);

        if (one_string) {
            vtr::free(one_string);
        }
        if (zero_string) {
            vtr::free(zero_string);
        }
        if (pad_string) {
            vtr::free(pad_string);
        }

        log("parmys pass finished.\n");
    }
} ParMYSPass;

PRIVATE_NAMESPACE_END