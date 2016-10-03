#include "netlist2_utils.h"
#include <map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <iterator>

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"

std::vector<AtomBlockId> identify_buffer_luts(const AtomNetlist& netlist);
bool is_buffer_lut(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_block(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_input(const AtomNetlist& netlist, const AtomBlockId blk);
bool is_removable_output(const AtomNetlist& netlist, const AtomBlockId blk);
void remove_buffer_lut(AtomNetlist& netlist, AtomBlockId blk);

std::string make_unconn(size_t& unconn_count, AtomPinType type);


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
            printf("\tDriver Block: '%s' Driver Pin: '%s[%u]'\n", netlist.block_name(pin_blk).c_str(), netlist.port_name(port).c_str(), netlist.pin_port_bit(driver_pin));
        } else {
            printf("\tNo Driver\n");
        }

        for(AtomPinId sink_pin : sinks) {
            VTR_ASSERT(sink_pin);
            AtomPortId port = netlist.pin_port(sink_pin);
            AtomBlockId pin_blk = netlist.pin_block(sink_pin);
            AtomBlockId port_blk = netlist.port_block(port);
            VTR_ASSERT(pin_blk == port_blk);
            printf("\tSink Block: '%s' Sink Pin: '%s[%u]'\n", netlist.block_name(pin_blk).c_str(), netlist.port_name(port).c_str(), netlist.pin_port_bit(sink_pin));
        }
    }
}

void print_netlist_as_blif(FILE* f, const AtomNetlist& netlist) {
    constexpr const char* INDENT = "    ";
    size_t unconn_count = 0;

    fprintf(f, "#Atom netlist generated by VPR\n");

    fprintf(f, ".model %s\n", netlist.netlist_name().c_str());

    {
        std::vector<AtomBlockId> inputs;
        for(auto blk_id : netlist.blocks()) {
            if(netlist.block_type(blk_id) == AtomBlockType::INPAD) {
                inputs.push_back(blk_id);
            }
        }
        fprintf(f, ".inputs \\\n");
        for(size_t i = 0; i < inputs.size(); ++i) {
            fprintf(f, "%s%s", INDENT, netlist.block_name(inputs[i]).c_str());

            if(i != inputs.size() - 1) {
                fprintf(f, " \\\n");
            }
        }
        fprintf(f, "\n");
    }

    {
        std::vector<AtomBlockId> outputs;
        for(auto blk_id : netlist.blocks()) {
            if(netlist.block_type(blk_id) == AtomBlockType::OUTPAD) {
                outputs.push_back(blk_id);
            }
        }
        fprintf(f, ".outputs \\\n");
        for(size_t i = 0; i < outputs.size(); ++i) {
            auto input_ports = netlist.block_input_ports(outputs[i]);
            VTR_ASSERT(input_ports.size() == 1);
            VTR_ASSERT(netlist.port_width(*input_ports.begin()) == 1);

            fprintf(f, "%s%s", INDENT, netlist.block_name(outputs[i]).c_str()+4);

            if(i != outputs.size() - 1) {
                fprintf(f, " \\\n");
            }
        }
        fprintf(f, "\n");
    }

    fprintf(f, "\n");

    //Latch
    for(auto blk_id : netlist.blocks()) {
        if(netlist.block_type(blk_id) == AtomBlockType::SEQUENTIAL) {
            const t_model* blk_model = netlist.block_model(blk_id);
            if(blk_model->name != std::string("latch")) continue;


            //Nets
            std::string d_net;
            std::string q_net;
            std::string clk_net;

            //Determine the nets
            auto input_ports = netlist.block_input_ports(blk_id);
            auto output_ports = netlist.block_output_ports(blk_id);
            auto clock_ports = netlist.block_clock_ports(blk_id);
            VTR_ASSERT(input_ports.size() == 1);
            VTR_ASSERT(output_ports.size() == 1);
            VTR_ASSERT(clock_ports.size() == 1);

            for(auto ports : {input_ports, output_ports, clock_ports}) {
                for(AtomPortId port_id : ports) {
                    auto pins = netlist.port_pins(port_id);
                    VTR_ASSERT(pins.size() == 1);
                    for(auto in_pin_id : pins) {
                        auto net_id = netlist.pin_net(in_pin_id);
                        if(netlist.port_name(port_id) == "D") {
                            d_net = netlist.net_name(net_id);

                        } else if(netlist.port_name(port_id) == "Q") {
                            q_net = netlist.net_name(net_id);

                        } else if(netlist.port_name(port_id) == "clk") {
                            clk_net = netlist.net_name(net_id);

                        } else {
                            VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Unrecognzied latch port '%s'", netlist.port_name(port_id).c_str());
                        }
                    }
                }
            }

            //Latch type: VPR always assumes rising edge
            auto type = "re"; 

            //Latch initial value
            int init_val = 3; //Unkown or unspecified
            //The initial value is stored as a single value in the truth table
            const auto& so_cover = netlist.block_truth_table(blk_id);
            VTR_ASSERT(so_cover.size() == 1); //Only one row
            VTR_ASSERT(so_cover[0].size() == 1); //Only one column
            switch(so_cover[0][0]) {
                case vtr::LogicValue::TRUE: init_val = 1; break;
                case vtr::LogicValue::FALSE: init_val = 0; break;
                case vtr::LogicValue::DONT_CARE: init_val = 2; break;
                case vtr::LogicValue::UNKOWN: init_val = 3; break;
                default: VTR_ASSERT_MSG(false, "Unrecognzied latch initial state");
            }

            fprintf(f, ".latch %s %s %s %s %d\n", d_net.c_str(), q_net.c_str(), type, clk_net.c_str(), init_val);

            fprintf(f, "\n");
        }
    }

    //Names
    for(auto blk_id : netlist.blocks()) {
        if(netlist.block_type(blk_id) == AtomBlockType::COMBINATIONAL) {
            const t_model* blk_model = netlist.block_model(blk_id);
            if(blk_model->name != std::string("names")) continue;


            std::vector<AtomNetId> nets;

            //Collect Inputs
            auto input_ports = netlist.block_input_ports(blk_id);
            VTR_ASSERT(input_ports.size() <= 1);
            for(auto port_id : input_ports) {
                for(auto in_pin_id : netlist.port_pins(port_id)) {
                    auto net_id = netlist.pin_net(in_pin_id);
                    nets.push_back(net_id);
                }
            }

            //Collect Outputs
            auto output_ports = netlist.block_output_ports(blk_id);
            VTR_ASSERT(output_ports.size() == 1);
            auto out_pins = netlist.port_pins(*output_ports.begin());
            VTR_ASSERT(out_pins.size() == 1);

            auto out_net_id = netlist.pin_net(*out_pins.begin());
            nets.push_back(out_net_id);

            fprintf(f, ".names ");
            for(size_t i = 0; i < nets.size(); ++i) {
                auto net_id = nets[i];

                fprintf(f, "%s", netlist.net_name(net_id).c_str());

                if(i != nets.size() - 1) {
                    fprintf(f, " ");
                }

            }
            fprintf(f, "\n");



            //Print the truth table
            for(auto row : netlist.block_truth_table(blk_id)) {
                for(size_t i = 0; i < row.size(); ++i) {
                    //Space between input and output columns
                    if(i == row.size() - 1) {
                        fprintf(f, " ");
                    }

                    switch(row[i]) {
                        case vtr::LogicValue::TRUE:         fprintf(f, "1"); break;
                        case vtr::LogicValue::FALSE:        fprintf(f, "0"); break;
                        case vtr::LogicValue::DONT_CARE:    fprintf(f, "-"); break;
                        default: VTR_ASSERT_MSG(false, "Valid single-output cover logic value");
                    }
                }
                fprintf(f, "\n");
            }
            fprintf(f, "\n");
        }
    }

    //Subckt

    std::set<const t_model*> subckt_models;
    for(auto blk_id : netlist.blocks()) {
        const t_model* blk_model = netlist.block_model(blk_id);
        if(blk_model->name == std::string("latch")
            || blk_model->name == std::string("names")
            || blk_model->name == std::string("input")
            || blk_model->name == std::string("output")) {
            continue;
        }

        //Must be a subckt
        subckt_models.insert(blk_model);
        
        std::vector<AtomPortId> ports;
        for(auto port_group : {netlist.block_input_ports(blk_id), netlist.block_output_ports(blk_id), netlist.block_clock_ports(blk_id)}) {
            for(auto port_id : port_group) {
                VTR_ASSERT(netlist.port_width(port_id) > 0);
                ports.push_back(port_id);
            }
        }


        fprintf(f, ".subckt %s \\\n", blk_model->name);
        for(size_t i = 0; i < ports.size(); i++) {
            auto width = netlist.port_width(ports[i]);
            for(size_t j = 0; j < width; ++j) {
                fprintf(f, "%s%s", INDENT, netlist.port_name(ports[i]).c_str());
                if(width != 1) {
                    fprintf(f, "[%zu]", j);
                }
                fprintf(f, "=");
                
                auto net_id = netlist.block_net(ports[i], j);
                if(net_id) {
                    fprintf(f, "%s", netlist.net_name(net_id).c_str());
                } else {
                    AtomPortType port_type = netlist.port_type(ports[i]);

                    AtomPinType pin_type;
                    switch(port_type) {
                        case AtomPortType::INPUT: //fallthrough
                        case AtomPortType::CLOCK: pin_type = AtomPinType::SINK; break;
                        case AtomPortType::OUTPUT: pin_type = AtomPinType::DRIVER; break;
                        default: VTR_ASSERT(false);
                    }
                    fprintf(f, "%s", make_unconn(unconn_count, pin_type).c_str());
                }

                if(i != ports.size() - 1 || j != width - 1) {
                    fprintf(f, " \\\n");
                }
            }
        }

        fprintf(f, "\n");

        fprintf(f, "\n");
    }
    
    fprintf(f, ".end\n"); //Main model
    fprintf(f, "\n");

    //The subkct models
    for(const t_model* model : subckt_models) {
        fprintf(f, ".model %s\n", model->name);

        fprintf(f, ".inputs");
        const t_model_ports* port = model->inputs;
        while(port) {
            VTR_ASSERT(port->size >= 0);
            if(port->size == 1) {
                fprintf(f, " \\\n");
                fprintf(f, "%s%s", INDENT, port->name);
            } else {
                for(int i = 0; i < port->size; ++i) {
                    fprintf(f, " \\\n");
                    fprintf(f, "%s%s[%d]", INDENT, port->name, i);
                }
            }
            port = port->next;
        }

        fprintf(f, "\n");
        fprintf(f, ".outputs");
        port = model->outputs;
        while(port) {
            VTR_ASSERT(port->size >= 0);
            if(port->size == 1) {
                fprintf(f, " \\\n");
                fprintf(f, "%s%s", INDENT, port->name);
            } else {
                for(int i = 0; i < port->size; ++i) {
                    fprintf(f, " \\\n");
                    fprintf(f, "%s%s[%d]", INDENT, port->name, i);
                }
            }
            port = port->next;
        }
        fprintf(f, "\n");

        fprintf(f, ".blackbox\n");
        fprintf(f, ".end\n");

        fprintf(f, "\n");
    }
}

void absorb_buffer_luts(AtomNetlist& netlist) {
    //First we look through the netlist to find LUTs with identity logic functions
    //we then remove those luts, replacing the net's they drove with the inputs to the
    //buffer lut

    //Find buffer luts
    auto buffer_luts = identify_buffer_luts(netlist);

    vtr::printf_info("Absorbing %zu LUT buffers\n", buffer_luts.size());

    //Remove the buffer luts
    for(auto blk : buffer_luts) {
        remove_buffer_lut(netlist, blk);
    }

    //TODO: absorb inverter LUTs?
}

std::vector<AtomBlockId> identify_buffer_luts(const AtomNetlist& netlist) {
    std::vector<AtomBlockId> buffer_luts;
    for(auto blk : netlist.blocks()) {
        if(is_buffer_lut(netlist, blk)) {
            /*vtr::printf_warning(__FILE__, __LINE__, "%s is a lut buffer and will be absorbed\n", netlist.block_name(blk).c_str());*/
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

    auto input_pin = *netlist.port_pins(input_port).begin(); //i.e. pin 2
    auto output_pin = *netlist.port_pins(output_port).begin(); //i.e. pin m+1

    auto input_net = netlist.pin_net(input_pin);
    auto output_net = netlist.pin_net(output_pin);

    //Collect the new driver and sink pins
    AtomPinId new_driver = netlist.net_driver(input_net);
    VTR_ASSERT(netlist.pin_type(new_driver) == AtomPinType::DRIVER);

    std::vector<AtomPinId> new_sinks;
    auto input_sinks = netlist.net_sinks(input_net); 
    auto output_sinks = netlist.net_sinks(output_net); 

    //We don't copy the input pin (i.e. pin 2)
    std::copy_if(input_sinks.begin(), input_sinks.end(), std::back_inserter(new_sinks), [input_pin](AtomPinId id) {
        return id != input_pin;
    });
    //Since we are copying sinks we don't include the output driver (i.e. pin m+1)
    std::copy(output_sinks.begin(), output_sinks.end(), std::back_inserter(new_sinks));

    VTR_ASSERT(new_sinks.size() == input_sinks.size() + output_sinks.size() - 1);

    //We now need to determine the name of the 'new' net
    //
    // We need to be careful about this name since a net pin could be
    // a Primary-Input/Primary-Output, and we don't want to change PI/PO names (for equivalance checking)
    //
    //Check if we have any PI/POs in the new net's pins
    // Note that the driver can only (potentially) be an INPAD, and the sinks only (potentially) OUTPADs
    AtomBlockType driver_block_type = netlist.block_type(netlist.pin_block(new_driver));
    bool driver_is_pi = (driver_block_type == AtomBlockType::INPAD);
    bool po_in_sinks = std::any_of(new_sinks.begin(), new_sinks.end(), [&](AtomPinId pin_id) {
        VTR_ASSERT(netlist.pin_type(pin_id) == AtomPinType::SINK);
        AtomBlockId blk_id = netlist.pin_block(pin_id);
        return netlist.block_type(blk_id) == AtomBlockType::OUTPAD;
    });

    std::string new_net_name;
    if(!driver_is_pi && !po_in_sinks) {
        //No PIs or POs, we can choose arbitarily in this case
        new_net_name = netlist.net_name(output_net);

    } else if(driver_is_pi && !po_in_sinks) {
        //Must use the input name to perserve primary-input name
        new_net_name = netlist.net_name(input_net);

    } else if(!driver_is_pi && po_in_sinks) {
        //Must use the output name to perserve primary-output name
        new_net_name = netlist.net_name(output_net);

    } else {
        VTR_ASSERT(driver_is_pi && po_in_sinks);
        //This is a buffered connection from a primary input, to primary output
        //TODO: consider implications of removing these...

        //Do not remove such buffers
        return;
    }

    size_t initial_input_net_pins = netlist.net_pins(input_net).size();

    //Remove the buffer
    //
    // Note that this removes pins 2 and m+1
    netlist.remove_block(blk);
    VTR_ASSERT(netlist.net_pins(input_net).size() == initial_input_net_pins - 1); //Should have removed pin 2
    VTR_ASSERT(netlist.net_driver(output_net) == AtomPinId::INVALID()); //Should have removed pin m+1

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
    size_t nets_swept = 0;
    size_t blocks_swept = 0;
    size_t inputs_swept = 0;
    size_t outputs_swept = 0;

    //We perform multiple passes of sweeping, since sweeping something may
    //enable more things to be swept afterward.
    //
    //We keep sweeping until nothing else is removed
    size_t pass_nets_swept;
    size_t pass_blocks_swept;
    size_t pass_inputs_swept;
    size_t pass_outputs_swept;
    do {
        pass_nets_swept = 0;
        pass_blocks_swept = 0;
        pass_inputs_swept = 0;
        pass_outputs_swept = 0;

        pass_nets_swept += sweep_nets(netlist);
        pass_blocks_swept += sweep_blocks(netlist);
        if(sweep_ios) {
            pass_inputs_swept += sweep_inputs(netlist);
            pass_outputs_swept += sweep_outputs(netlist);
        }

        nets_swept += pass_nets_swept;
        blocks_swept += pass_blocks_swept;
        inputs_swept += pass_outputs_swept;
        outputs_swept += pass_outputs_swept;
    } while(pass_nets_swept != 0 || pass_blocks_swept != 0 
            || pass_inputs_swept != 0 || pass_outputs_swept != 0);

    vtr::printf_info("Swept net(s)   : %zu\n", nets_swept);
    vtr::printf_info("Swept block(s) : %zu\n", blocks_swept);
    vtr::printf_info("Swept input(s) : %zu\n", inputs_swept);
    vtr::printf_info("Swept output(s): %zu\n", outputs_swept);

    return nets_swept + blocks_swept + inputs_swept + outputs_swept;
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
        /*vtr::printf_warning(__FILE__, __LINE__, "Sweeping block '%s'\n", netlist.block_name(blk_id).c_str());*/
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
        /*vtr::printf_warning(__FILE__, __LINE__, "Sweeping primary input '%s'\n", netlist.block_name(blk_id).c_str());*/
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
            /*vtr::printf_warning(__FILE__, __LINE__, "Sweeping primary output '%s'\n", netlist.block_name(blk_id).c_str());*/
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
            //No driver
            /*vtr::printf_warning(__FILE__, __LINE__, "Net '%s' has no driver and will be removed\n", netlist.net_name(net_id).c_str());*/
            nets_to_remove.insert(net_id);
        }
        if(netlist.net_sinks(net_id).size() == 0) {
            //No sinks
            /*vtr::printf_warning(__FILE__, __LINE__, "Net '%s' has no sinks and will be removed\n", netlist.net_name(net_id).c_str());*/
            nets_to_remove.insert(net_id);
        }
    }

    for(auto net_id : nets_to_remove) {
        netlist.remove_net(net_id);
    }

    return nets_to_remove.size();
}

std::string make_unconn(size_t& unconn_count, AtomPinType /*pin_type*/) {
#if 0
    if(pin_type == AtomPinType::DRIVER) {
        return std::string("unconn") + std::to_string(unconn_count++);
    } else {
        return std::string("unconn");
    }
#else
    return std::string("unconn") + std::to_string(unconn_count++);
#endif
}
