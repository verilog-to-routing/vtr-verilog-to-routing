#include "netlist2_utils.h"
#include <map>
#include <unordered_set>

#include "vtr_assert.h"
#include "vtr_log.h"

std::vector<AtomBlockId> identify_buffer_luts(const AtomNetlist& netlist);
bool is_buffer_lut(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_block(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_input(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_output(const AtomNetlist& netlist, const AtomBlockId blk);
void remove_buffer_lut(AtomNetlist& netlist, AtomBlockId blk);


void print_netlist(FILE* f, const AtomNetlist& netlist) {

    //Build a map of the blocks by type
    std::multimap<AtomBlockType,AtomBlockId> blocks_by_type;
    for(AtomBlockId blk_id : netlist.blocks()) {
        if(blk_id) {
            blocks_by_type.insert({netlist.block_type(blk_id), blk_id});
        }
    }

    //Iterating through the map ensures blocks of the same type are printed together
    for(auto kv : blocks_by_type) {
        AtomBlockType type = kv.first;
        AtomBlockId blk_id = kv.second;
        const t_model* model = netlist.block_model(blk_id);

        //Print the block model type and type
        fprintf(f, "Block '%s'", model->name);
        fprintf(f, " (");
        switch(type) {
            case AtomBlockType::INPAD : fprintf(f, "INPAD"); break;
            case AtomBlockType::OUTPAD: fprintf(f, "OUTPAD"); break;
            case AtomBlockType::COMBINATIONAL: fprintf(f, "COMBINATIONAL"); break;
            case AtomBlockType::SEQUENTIAL: fprintf(f, "SEQUENTIAL"); break;
            default: VTR_ASSERT_MSG(false, "Recognzied AtomBlockType");
        }
        fprintf(f, "):");
        //Print block name
        fprintf(f, " %s\n", netlist.block_name(blk_id).c_str());

        //Print input ports
        for(auto input_port : netlist.block_input_ports(blk_id)) {
            auto pins = netlist.port_pins(input_port);
            fprintf(f, "\tInput (%zu bits)\n", pins.size());
            size_t i = 0;
            for(auto pin : pins) {
                fprintf(f, "\t\t%s[%zu] <-", netlist.port_name(input_port).c_str(), i);
                if(pin) {
                    auto net = netlist.pin_net(pin);
                    if(net) {
                        fprintf(f, " %s", netlist.net_name(net).c_str());
                    } else {
                        fprintf(f, " ");
                    }
                } else {
                    fprintf(f, " ");
                }
                fprintf(f, "\n");
                i++;
            }
        }

        //Print output ports
        for(auto output_port : netlist.block_output_ports(blk_id)) {
            auto pins = netlist.port_pins(output_port);
            fprintf(f, "\tOutput (%zu bits)\n", pins.size());
            size_t i = 0;
            for(auto pin : pins) {
                fprintf(f, "\t\t%s[%zu] ->", netlist.port_name(output_port).c_str(), i);
                if(pin) {
                    auto net = netlist.pin_net(pin);
                    if(net) {
                        fprintf(f, " %s", netlist.net_name(net).c_str());
                    } else {
                        fprintf(f, " ");
                    }
                } else {
                    fprintf(f, " ");
                }
                fprintf(f, "\n");
                i++;
            }
        }

        //Print clock ports
        for(auto clock_port : netlist.block_clock_ports(blk_id)) {
            auto pins = netlist.port_pins(clock_port);
            fprintf(f, "\tClock (%zu bits)\n", pins.size());
            size_t i = 0;
            for(auto pin : pins) {
                fprintf(f, "\t\t%s[%zu] <-", netlist.port_name(clock_port).c_str(), i);
                if(pin) {
                    fprintf(f, " %s", netlist.net_name(netlist.pin_net(pin)).c_str());
                } else {
                    fprintf(f, " ");
                }
                fprintf(f, "\n");
                i++;
            }
        }
    }

    //Print out per-net information
    for(auto net_id : netlist.nets()) {
        if(!net_id) continue;

        auto sinks = netlist.net_sinks(net_id);
        //Net name and fanout
        fprintf(f, "Net '%s' (fanout %zu)\n", netlist.net_name(net_id).c_str(), sinks.size());

        AtomPinId driver_pin = netlist.net_driver(net_id);
        if(driver_pin) {
            AtomPortId port = netlist.pin_port(driver_pin);
            AtomBlockId pin_blk = netlist.pin_block(driver_pin);
            AtomBlockId port_blk = netlist.port_block(port);
            VTR_ASSERT(pin_blk == port_blk);
            printf("\tDriver Block: '%s' Driver Pin: '%s[%zu]'\n", netlist.block_name(pin_blk).c_str(), netlist.port_name(port).c_str(), netlist.pin_port_bit(driver_pin));
        } else {
            printf("\tNo Driver\n");
        }

        for(AtomPinId sink_pin : sinks) {
            VTR_ASSERT(sink_pin);
            AtomPortId port = netlist.pin_port(sink_pin);
            AtomBlockId pin_blk = netlist.pin_block(sink_pin);
            AtomBlockId port_blk = netlist.port_block(port);
            VTR_ASSERT(pin_blk == port_blk);
            printf("\tSink Block: '%s' Sink Pin: '%s[%zu]'\n", netlist.block_name(pin_blk).c_str(), netlist.port_name(port).c_str(), netlist.pin_port_bit(sink_pin));
        }
    }
}

void absorb_buffer_luts(AtomNetlist& netlist) {
    //First we look through the netlist to find LUTs with identity logic functions
    //we then remove those luts, replacing the net's they drove with the inputs to the
    //buffer lut

    //Find buffer luts
    auto buffer_luts = identify_buffer_luts(netlist);

    //Remove the buffer luts
    for(auto blk : buffer_luts) {
        remove_buffer_lut(netlist, blk);
    }
}

std::vector<AtomBlockId> identify_buffer_luts(const AtomNetlist& netlist) {
    std::vector<AtomBlockId> buffer_luts;
    for(auto blk : netlist.blocks()) {
        if(is_buffer_lut(netlist, blk)) {
            printf("%s is a lut buffer and will be absorbed\n", netlist.block_name(blk).c_str());
            buffer_luts.push_back(blk);
        }
    }
    return buffer_luts;
}

bool is_buffer_lut(const AtomNetlist& netlist, const AtomBlockId blk) {
    if(netlist.block_type(blk) == AtomBlockType::COMBINATIONAL) {
        const t_model* blk_model = netlist.block_model(blk);
        if(blk_model->name == std::string("names")) {
            
            auto input_ports = netlist.block_input_ports(blk);
            auto output_ports = netlist.block_output_ports(blk);

            //Buffer LUTs have a single input port and a single output port
            if(input_ports.size() == 1 && output_ports.size() == 1) {
                //Get the ports
                auto input_port = *input_ports.begin();
                auto output_port = *output_ports.begin();

                //Collect the pins
                auto input_pins = netlist.port_pins(input_port);
                auto output_pins = netlist.port_pins(output_port);

                //Count the number of connected input pins
                size_t connected_input_pins = 0;
                for(auto input_pin : input_pins) {
                    if(input_pin && netlist.pin_net(input_pin)) {
                        ++connected_input_pins;
                    }
                }

                //Count the number of connected output pins
                size_t connected_output_pins = 0;
                for(auto output_pin : output_pins) {
                    if(output_pin && netlist.pin_net(output_pin)) {
                        ++connected_output_pins;
                    }
                }

                //Both ports must be single bit
                if(connected_input_pins == 1 && connected_output_pins == 1) {
                    //It is a single-input single-output LUT, we now 
                    //inspect it's truth table
                    //
                    const auto& truth_table = netlist.block_truth_table(blk);

                    VTR_ASSERT_MSG(truth_table.size() == 1, "One truth-table row");
                    VTR_ASSERT_MSG(truth_table[0].size() == 2, "Two truth-table row entries");

                    //Check for valid buffer logic functions
                    // A LUT is a buffer provided it has the identity logic
                    // function and a single input. For example:
                    // 
                    // .names in_buf out_buf
                    // 1 1
                    // 
                    // and
                    // 
                    // .names int_buf out_buf
                    // 0 0
                    // 
                    // both implement logical identity.
                    if((truth_table[0][0] == vtr::LogicValue::TRUE && truth_table[0][1] == vtr::LogicValue::TRUE)
                        || (truth_table[0][0] == vtr::LogicValue::FALSE && truth_table[0][1] == vtr::LogicValue::FALSE)) {

                        //It is a buffer LUT
                        return true;
                    }
                }

            }
        }
    }
    return false;
}

void remove_buffer_lut(AtomNetlist& netlist, AtomBlockId blk) {
    //General net connectivity, numbers equal pin ids
    //
    // 1  in    2 ----- m+1  out
    // --------->| buf |---------> m+2
    //      |     -----     |
    //      |               |
    //      |--> 3          |----> m+3
    //      |               |
    //      | ...           |   ...
    //      |               |
    //      |--> m          |----> m+k+1
    //
    //On the input net we have a single driver (pin 1) and sinks (pins 2 through m)
    //On the output net we have a single driver (pin m+1) and sinks (pins m+2 through m+k+1)
    //
    //The resulting connectivity after removing the buffer is:
    //
    // 1            in
    // --------------------------> m+2
    //      |               |
    //      |               |
    //      |--> 3          |----> m+3
    //      |               |
    //      | ...           |   ...
    //      |               |
    //      |--> m          |----> m+k+1
    // 
    //
    //We remove the buffer and fix-up the connectivity using the following steps
    //  - Remove the buffer (this also removes pins 2 and m+1 from the 'in' and 'out' nets)
    //  - Copy the pins left on 'in' and 'out' nets
    //  - Remove the 'in' and 'out' nets (this sets the pin's associated net to invalid)
    //  - We create a new net using the pins we copied, setting pin 1 as the driver and
    //    all other pins as sinks

    //Find the input and output nets
    auto input_port = *netlist.block_input_ports(blk).begin();
    auto output_port = *netlist.block_output_ports(blk).begin();

    auto input_pin = *netlist.port_pins(input_port).begin();
    auto output_pin = *netlist.port_pins(output_port).begin();

    auto input_net = netlist.pin_net(input_pin);
    auto output_net = netlist.pin_net(output_pin);

    auto new_net_name = netlist.net_name(input_net); //Save the original name

    size_t initial_input_net_pins = netlist.net_pins(input_net).size();

    //Remove the buffer
    //
    // Note that this removes pins 2 and m+1
    netlist.remove_block(blk);

    VTR_ASSERT(netlist.net_pins(input_net).size() == initial_input_net_pins - 1); //Should have removed pin 2
    VTR_ASSERT(netlist.net_driver(output_net) == AtomPinId::INVALID()); //Should have removed pin m+1

    //Collect the remaining pins
    AtomPinId new_driver = netlist.net_driver(input_net);
    std::vector<AtomPinId> new_sinks;
    for(auto net : {input_net, output_net}) {
        for(auto sink : netlist.net_sinks(net)) {
            new_sinks.push_back(sink);
        }
    }

    //Remove the nets
    netlist.remove_net(input_net);
    netlist.remove_net(output_net);

    //Create the new merged net
    netlist.add_net(new_net_name, new_driver, new_sinks);
}

bool is_removable_block(const AtomNetlist& netlist, const AtomBlockId blk_id) {
    //Any block with no fanout is removable
    for(AtomPortId out_port_id : netlist.block_output_ports(blk_id)) {
        for(AtomPinId pin_id : netlist.port_pins(out_port_id)) {
            if(!pin_id) continue;
            AtomNetId net_id = netlist.pin_net(pin_id);
            if(net_id) {
                //There is a valid output net
                return false; 
            }
        }
    }
    return true;
}

bool is_removable_input(const AtomNetlist& netlist, const AtomBlockId blk_id) {
    AtomBlockType type = netlist.block_type(blk_id);

    //Only return true if an INPAD
    if(type != AtomBlockType::INPAD) return false;

    return is_removable_block(netlist, blk_id);
}

bool is_removable_output(const AtomNetlist& netlist, const AtomBlockId blk_id) {
    AtomBlockType type = netlist.block_type(blk_id);

    //Only return true if an INPAD
    if(type != AtomBlockType::OUTPAD) return false;

    //An output is only removable if it has no fan-in
    for(AtomPortId in_port_id : netlist.block_input_ports(blk_id)) {
        for(AtomPinId pin_id : netlist.port_pins(in_port_id)) {
            if(!pin_id) continue;
            AtomNetId net_id = netlist.pin_net(pin_id);
            if(net_id) {
                //There is a valid output net
                return false; 
            }
        }
    }
    return true;
}

size_t sweep_iterative(AtomNetlist& netlist, bool sweep_ios) {
    size_t total_swept_items = 0;
    size_t pass_swept_items = 0;

    //We perform multiple passes of sweeping, since sweeping something may
    //enable more things to be swept afterward.
    //
    //We keep sweeping until nothing else is removed
    do {
        pass_swept_items = 0;
        pass_swept_items += sweep_nets(netlist);
        pass_swept_items += sweep_blocks(netlist);
        if(sweep_ios) {
            pass_swept_items += sweep_inputs(netlist);
            pass_swept_items += sweep_outputs(netlist);
        }
        total_swept_items += pass_swept_items;
    } while(pass_swept_items != 0);

    return total_swept_items;
}

size_t sweep_blocks(AtomNetlist& netlist) {
    //Identify any blocks (not inputs or outputs) for removal
    std::unordered_set<AtomBlockId> blocks_to_remove;
    for(auto blk_id : netlist.blocks()) {
        if(!blk_id) continue;

        AtomBlockType type = netlist.block_type(blk_id);

        //Don't remove inpads/outpads here, we have seperate sweep functions for these
        if(type == AtomBlockType::INPAD || type == AtomBlockType::OUTPAD) continue;

        //We remove any blocks with no fanout 
        if(is_removable_block(netlist, blk_id)) {
            blocks_to_remove.insert(blk_id);
        }
    }

    //Remove them
    for(auto blk_id : blocks_to_remove) {
        vtr::printf_warning(__FILE__, __LINE__, "Sweeping block '%s'\n", netlist.block_name(blk_id).c_str());
        netlist.remove_block(blk_id);
    }

    return blocks_to_remove.size();
}

size_t sweep_inputs(AtomNetlist& netlist) {
    //Identify any inputs for removal
    std::unordered_set<AtomBlockId> inputs_to_remove;
    for(auto blk_id : netlist.blocks()) {
        if(!blk_id) continue;

        if(is_removable_input(netlist, blk_id)) {
            inputs_to_remove.insert(blk_id);
        }
    }

    //Remove them
    for(auto blk_id : inputs_to_remove) {
        vtr::printf_warning(__FILE__, __LINE__, "Sweeping primary input '%s'\n", netlist.block_name(blk_id).c_str());
        netlist.remove_block(blk_id);
    }

    return inputs_to_remove.size();
}

size_t sweep_outputs(AtomNetlist& netlist) {
    //Identify any outputs for removal
    std::unordered_set<AtomBlockId> outputs_to_remove;
    for(auto blk_id : netlist.blocks()) {
        if(!blk_id) continue;

        if(is_removable_output(netlist, blk_id)) {
            vtr::printf_warning(__FILE__, __LINE__, "Sweeping primary output '%s'\n", netlist.block_name(blk_id).c_str());
            outputs_to_remove.insert(blk_id);
        }
    }

    //Remove them
    for(auto blk_id : outputs_to_remove) {
        netlist.remove_block(blk_id);
    }

    return outputs_to_remove.size();
}

size_t sweep_nets(AtomNetlist& netlist) {
    //Find any nets with no fanout or no driver, and remove them

    std::unordered_set<AtomNetId> nets_to_remove;
    for(auto net_id : netlist.nets()) {
        if(!net_id) continue;

        if(!netlist.net_driver(net_id)) {
            vtr::printf_warning(__FILE__, __LINE__, "Net '%s' has no driver and will be removed\n", netlist.net_name(net_id).c_str());
            nets_to_remove.insert(net_id);
        }
        if(netlist.net_sinks(net_id).size() == 0) {
            vtr::printf_warning(__FILE__, __LINE__, "Net '%s' has no sinks and will be removed\n", netlist.net_name(net_id).c_str());
            nets_to_remove.insert(net_id);
        }
    }

    for(auto net_id : nets_to_remove) {
        netlist.remove_net(net_id);
    }

    return nets_to_remove.size();
}
