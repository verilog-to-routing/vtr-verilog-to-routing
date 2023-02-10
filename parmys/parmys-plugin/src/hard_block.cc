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
#include <stdlib.h>

#include "hard_block.h"
#include "memory.h"
#include "netlist_utils.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"

#include "kernel/yosys.h"

#include "../parmys_utils.hpp"

STRING_CACHE *hard_block_names = NULL;

void cache_hard_block_names();
void register_hb_port_size(t_model_ports *hb_ports, int size);

void register_hb_port_size(t_model_ports *hb_ports, int size)
{
    if (hb_ports)
        hb_ports->size = size;
    /***
     * else
     *	TODO error
     */
}

t_model_ports *get_model_port(t_model_ports *ports, const char *name)
{
    while (ports && strcmp(ports->name, name))
        ports = ports->next;

    return ports;
}

void cache_hard_block_names()
{
    t_model *hard_blocks = NULL;

    hard_blocks = Arch.models;
    hard_block_names = sc_new_string_cache();
    while (hard_blocks) {
        int sc_spot = sc_add_string(hard_block_names, hard_blocks->name);
        hard_block_names->data[sc_spot] = (void *)hard_blocks;
        hard_blocks = hard_blocks->next;
    }
}

void register_hard_blocks()
{
    cache_hard_block_names();
    single_port_rams = find_hard_block(SINGLE_PORT_RAM_string);
    dual_port_rams = find_hard_block(DUAL_PORT_RAM_string);

    if (single_port_rams) {
        if (configuration.split_memory_width) {
            register_hb_port_size(get_model_port(single_port_rams->inputs, "data"), 1);

            register_hb_port_size(get_model_port(single_port_rams->outputs, "out"), 1);
        }

        register_hb_port_size(get_model_port(single_port_rams->inputs, "addr"), get_sp_ram_split_depth());
    }

    if (dual_port_rams) {
        if (configuration.split_memory_width) {
            register_hb_port_size(get_model_port(dual_port_rams->inputs, "data1"), 1);
            register_hb_port_size(get_model_port(dual_port_rams->inputs, "data2"), 1);

            register_hb_port_size(get_model_port(dual_port_rams->outputs, "out1"), 1);
            register_hb_port_size(get_model_port(dual_port_rams->outputs, "out2"), 1);
        }

        int split_depth = get_dp_ram_split_depth();

        register_hb_port_size(get_model_port(dual_port_rams->inputs, "addr1"), split_depth);
        register_hb_port_size(get_model_port(dual_port_rams->inputs, "addr2"), split_depth);
    }
}

t_model *find_hard_block(const char *name)
{
    t_model *hard_blocks;

    hard_blocks = Arch.models;
    while (hard_blocks)
        if (!strcmp(hard_blocks->name, name))
            return hard_blocks;
        else
            hard_blocks = hard_blocks->next;

    return NULL;
}

void cell_hard_block(nnode_t *node, Yosys::Module *module, netlist_t *netlist, Yosys::Design *design)
{
    int index, port;

    /* Assert that every hard block has at least an input and output */
    oassert(node->input_port_sizes[0] > 0);
    oassert(node->output_port_sizes[0] > 0);

    Yosys::IdString celltype = Yosys::RTLIL::escape_id(node->related_ast_node->identifier_node->types.identifier);
    Yosys::RTLIL::Cell *cell = module->addCell(NEW_ID, celltype);

    Yosys::hashlib::dict<Yosys::RTLIL::IdString, Yosys::hashlib::dict<int, Yosys::SigBit>> cell_wideports_cache;

    /* print the input port mappings */
    port = index = 0;
    for (int i = 0; i < node->num_input_pins; i++) {
        /* Check that the input pin is driven */
        if (node->input_pins[i]->net->num_driver_pins == 0 && node->input_pins[i]->net != netlist->zero_net &&
            node->input_pins[i]->net != netlist->one_net && node->input_pins[i]->net != netlist->pad_net) {
            warning_message(NETLIST, node->loc, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
            add_fanout_pin_to_net(netlist->zero_net, node->input_pins[i]);
        } else if (node->input_pins[i]->net->num_driver_pins > 1) {
            error_message(NETLIST, node->loc, "Multiple (%d) driver pins not supported in hard block definition\n",
                          node->input_pins[i]->net->num_driver_pins);
        }
        std::string p, q;

        if (node->input_port_sizes[port] == 1) {
            p = node->input_pins[i]->mapping;
            if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
                q = node->input_pins[i]->net->driver_pins[0]->name;
            else
                q = node->input_pins[i]->net->driver_pins[0]->node->name;
        } else {
            p = Yosys::stringf("%s[%d]", node->input_pins[i]->mapping, index);
            if (node->input_pins[i]->net->driver_pins[0]->name != NULL)
                q = node->input_pins[i]->net->driver_pins[0]->name;
            else
                q = node->input_pins[i]->net->driver_pins[0]->node->name;
        }

        std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(p);
        if (wp.first.empty())
            cell->setPort(Yosys::RTLIL::escape_id(p), to_wire(q, module));
        else
            cell_wideports_cache[wp.first][wp.second] = to_wire(q, module);

        index++;
        if (node->input_port_sizes[port] == index) {
            index = 0;
            port++;
        }
    }

    /* print the output port mappings */
    port = index = 0;
    for (int i = 0; i < node->num_output_pins; i++) {
        std::string p, q;
        if (node->output_port_sizes[port] != 1) {
            p = Yosys::stringf("%s[%d]", node->output_pins[i]->mapping, index);
            q = node->output_pins[i]->name;
        } else {
            p = node->output_pins[i]->mapping;
            q = node->output_pins[i]->name;
        }

        std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(p);
        if (wp.first.empty())
            cell->setPort(Yosys::RTLIL::escape_id(p), to_wire(q, module));
        else
            cell_wideports_cache[wp.first][wp.second] = to_wire(q, module);

        index++;
        if (node->output_port_sizes[port] == index) {
            index = 0;
            port++;
        }
    }

    handle_cell_wideports_cache(&cell_wideports_cache, design, module, cell);

    for (auto &param : node->cell_parameters) {
        cell->parameters[Yosys::RTLIL::IdString(param.first)] = Yosys::Const(param.second);
    }

    return;
}

void output_hard_blocks_yosys(Yosys::Design *design)
{
    t_model_ports *hb_ports;
    t_model *hard_blocks;

    hard_blocks = Arch.models;
    while (hard_blocks != NULL) {
        if (hard_blocks->used == 1) /* Hard Block is utilized */
        {
            // IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in add_the_blackbox_for_adds and
            // add_the_blackbox_for_mults)
            if (strcmp(hard_blocks->name, "adder") == 0 || strcmp(hard_blocks->name, "multiply") == 0) {
                hard_blocks = hard_blocks->next;
                break;
            }

            Yosys::RTLIL::Module *module = nullptr;

            Yosys::hashlib::dict<Yosys::RTLIL::IdString, std::pair<int, bool>> wideports_cache;

            module = new Yosys::RTLIL::Module;
            module->name = Yosys::RTLIL::escape_id(hard_blocks->name);

            if (design->module(module->name))
                Yosys::log_error("Duplicate definition of module %s!\n", Yosys::log_id(module->name));
            design->add(module);

            hb_ports = hard_blocks->inputs;
            while (hb_ports != NULL) {
                for (int i = 0; i < hb_ports->size; i++) {
                    std::string w_name;
                    if (hb_ports->size == 1)
                        w_name = hb_ports->name;
                    else
                        w_name = Yosys::stringf("%s[%d]", hb_ports->name, i);

                    Yosys::RTLIL::Wire *wire = to_wire(w_name, module);
                    wire->port_input = true;

                    std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(w_name);
                    if (!wp.first.empty() && wp.second >= 0) {
                        wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
                        wideports_cache[wp.first].second = true;
                    }
                }

                hb_ports = hb_ports->next;
            }

            // fprintf(out, "\n.outputs");
            hb_ports = hard_blocks->outputs;
            while (hb_ports != NULL) {
                for (int i = 0; i < hb_ports->size; i++) {
                    std::string w_name;
                    if (hb_ports->size == 1)
                        w_name = hb_ports->name;
                    else
                        w_name = Yosys::stringf("%s[%d]", hb_ports->name, i);

                    Yosys::RTLIL::Wire *wire = to_wire(w_name, module);
                    wire->port_output = true;

                    std::pair<Yosys::RTLIL::IdString, int> wp = wideports_split(w_name);
                    if (!wp.first.empty() && wp.second >= 0) {
                        wideports_cache[wp.first].first = std::max(wideports_cache[wp.first].first, wp.second + 1);
                        wideports_cache[wp.first].second = false;
                    }
                }

                hb_ports = hb_ports->next;
            }

            handle_wideports_cache(&wideports_cache, module);

            module->fixup_ports();
            wideports_cache.clear();

            module->attributes[Yosys::ID::blackbox] = Yosys::RTLIL::Const(1);
        }

        hard_blocks = hard_blocks->next;
    }

    return;
}

void instantiate_hard_block(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
    int port, index;

    port = index = 0;
    /* Give names to the output pins */
    for (int i = 0; i < node->num_output_pins; i++) {
        if (node->output_pins[i]->name == NULL)
            node->output_pins[i]->name = make_full_ref_name(node->name, NULL, NULL, node->output_pins[i]->mapping, i);
        // node->output_pins[i]->name = make_full_ref_name(node->name, NULL, NULL, node->output_pins[i]->mapping,
        // (configuration.elaborator_type == elaborator_e::_YOSYS) ? i : -1); //@TODO

        index++;
        if (node->output_port_sizes[port] == index) {
            index = 0;
            port++;
        }
    }

    node->traverse_visited = mark;
    return;
}
