#include <set>

#include "vtr_log.h"
#include "arch_error.h"
#include "arch_check.h"

bool check_model_clocks(t_model* model, const char* file, uint32_t line) {
    //Collect the ports identified as clocks
    std::set<std::string> clocks;
    for (t_model_ports* ports : {model->inputs, model->outputs}) {
        for (t_model_ports* port = ports; port != nullptr; port = port->next) {
            if (port->is_clock) {
                clocks.insert(port->name);
            }
        }
    }

    //Check that any clock references on the ports are to identified clock ports
    for (t_model_ports* ports : {model->inputs, model->outputs}) {
        for (t_model_ports* port = ports; port != nullptr; port = port->next) {
            if (!port->clock.empty() && !clocks.count(port->clock)) {
                archfpga_throw(file, line,
                               "No matching clock port '%s' on model '%s', required for port '%s'",
                               port->clock.c_str(), model->name, port->name);
            }
        }
    }
    return true;
}

bool check_model_combinational_sinks(const t_model* model, const char* file, uint32_t line) {
    //Outputs should have no combinational sinks
    for (t_model_ports* port = model->outputs; port != nullptr; port = port->next) {
        if (port->combinational_sink_ports.size() != 0) {
            archfpga_throw(file, line,
                           "Model '%s' output port '%s' can not have combinational sink ports",
                           model->name, port->name);
        }
    }

    //Record the output ports
    std::map<std::string, t_model_ports*> output_ports;
    for (t_model_ports* port = model->outputs; port != nullptr; port = port->next) {
        output_ports.insert({port->name, port});
    }

    for (t_model_ports* port = model->inputs; port != nullptr; port = port->next) {
        for (const std::string& sink_port_name : port->combinational_sink_ports) {
            //Check that the input port combinational sinks are all outputs
            if (!output_ports.count(sink_port_name)) {
                archfpga_throw(file, line,
                               "Model '%s' input port '%s' can not be combinationally connected to '%s' (not an output port of the model)",
                               model->name, port->name, sink_port_name.c_str());
            }

            //Check that any output combinational sinks are not clocks
            t_model_ports* sink_port = output_ports[sink_port_name];
            VTR_ASSERT(sink_port);
            if (sink_port->is_clock) {
                archfpga_throw(file, line,
                               "Model '%s' output port '%s' can not be both: a clock source (is_clock=\"%d\"),"
                               " and combinationally connected to input port '%s' (acting as a clock buffer).",
                               model->name, sink_port->name, sink_port->is_clock, port->name);
            }
        }
    }

    return true;
}

void warn_model_missing_timing(const t_model* model, const char* file, uint32_t line) {
    //Check whether there are missing edges and warn the user
    std::set<std::string> comb_connected_outputs;
    for (t_model_ports* port = model->inputs; port != nullptr; port = port->next) {
        if (port->clock.empty()                       //Not sequential
            && port->combinational_sink_ports.empty() //Doesn't drive any combinational outputs
            && !port->is_clock                        //Not an input clock
        ) {
            VTR_LOGF_WARN(file, line,
                          "Model '%s' input port '%s' has no timing specification (no clock specified to create a sequential input port, not combinationally connected to any outputs, not a clock input)\n", model->name, port->name);
        }

        comb_connected_outputs.insert(port->combinational_sink_ports.begin(), port->combinational_sink_ports.end());
    }

    for (t_model_ports* port = model->outputs; port != nullptr; port = port->next) {
        if (port->clock.empty()                          //Not sequential
            && !comb_connected_outputs.count(port->name) //Not combinationally drivven
            && !port->is_clock                           //Not an output clock
        ) {
            VTR_LOGF_WARN(file, line,
                          "Model '%s' output port '%s' has no timing specification (no clock specified to create a sequential output port, not combinationally connected to any inputs, not a clock output)\n", model->name, port->name);
        }
    }
}

void check_port_direct_mappings(t_physical_tile_type_ptr physical_tile, t_sub_tile* sub_tile, t_logical_block_type_ptr logical_block) {
    auto pb_type = logical_block->pb_type;

    if (pb_type->num_pins > (sub_tile->num_phy_pins / sub_tile->capacity.total())) {
        archfpga_throw(__FILE__, __LINE__,
                       "Logical Block (%s) has more pins than the Sub Tile (%s).\n",
                       logical_block->name, sub_tile->name);
    }

    auto& pin_direct_maps = physical_tile->tile_block_pin_directs_map.at(logical_block->index);
    auto pin_direct_map = pin_direct_maps.at(sub_tile->index);

    if (pb_type->num_pins != (int)pin_direct_map.size()) {
        archfpga_throw(__FILE__, __LINE__,
                       "Logical block (%s) and Sub tile (%s) have a different number of ports.\n",
                       logical_block->name, physical_tile->name);
    }

    for (auto pin_map : pin_direct_map) {
        auto block_port = get_port_by_pin(logical_block, pin_map.first.pin);

        auto sub_tile_port = get_port_by_pin(sub_tile, pin_map.second.pin);

        VTR_ASSERT(block_port != nullptr);
        VTR_ASSERT(sub_tile_port != nullptr);

        if (sub_tile_port->type != block_port->type
            || sub_tile_port->num_pins != block_port->num_pins
            || sub_tile_port->equivalent != block_port->equivalent) {
            archfpga_throw(__FILE__, __LINE__,
                           "Logical block (%s) and Physical tile (%s) do not have equivalent port specifications. Sub tile port %s, logical block port %s\n",
                           logical_block->name, sub_tile->name, sub_tile_port->name, block_port->name);
        }
    }
}

bool check_leaf_pb_model_timing_consistency(const t_pb_type* pb_type, const t_arch& arch) {
    //Normalize the blif model name to match the model name
    // by removing the leading '.' (.latch, .inputs, .names etc.)
    // by removing the leading '.subckt'
    VTR_ASSERT(pb_type->blif_model);
    std::string blif_model = pb_type->blif_model;
    std::string subckt = ".subckt ";
    auto pos = blif_model.find(subckt);
    if (pos != std::string::npos) {
        blif_model = blif_model.substr(pos + subckt.size());
    }

    //Find the matching model
    const t_model* model = nullptr;

    for (const t_model* models : {arch.models, arch.model_library}) {
        for (model = models; model != nullptr; model = model->next) {
            if (std::string(model->name) == blif_model) {
                break;
            }
        }
        if (model != nullptr) {
            break;
        }
    }
    if (model == nullptr) {
        archfpga_throw(get_arch_file_name(), -1,
                       "Unable to find model for blif_model '%s' found on pb_type '%s'",
                       blif_model.c_str(), pb_type->name);
    }

    //Now that we have the model we can compare the timing annotations

    //Check from the pb_type's delay annotations match the model
    //
    //  This ensures that the pb_types' delay annotations are consistent with the model
    for (int i = 0; i < pb_type->num_annotations; ++i) {
        const t_pin_to_pin_annotation* annot = &pb_type->annotations[i];

        if (annot->type == E_ANNOT_PIN_TO_PIN_DELAY) {
            //Check that any combinational delays specified match the 'combinational_sinks_ports' in the model

            if (annot->clock) {
                //Sequential annotation, check that the clock on the specified port matches the model

                //Annotations always put the pin in the input_pins field
                VTR_ASSERT(annot->input_pins);
                for (const std::string& input_pin : vtr::split(annot->input_pins)) {
                    InstPort annot_port(input_pin);
                    for (const std::string& clock : vtr::split(annot->clock)) {
                        InstPort annot_clock(clock);

                        //Find the model port
                        const t_model_ports* model_port = nullptr;
                        for (const t_model_ports* ports : {model->inputs, model->outputs}) {
                            for (const t_model_ports* port = ports; port != nullptr; port = port->next) {
                                if (port->name == annot_port.port_name()) {
                                    model_port = port;
                                    break;
                                }
                            }
                            if (model_port != nullptr) break;
                        }
                        if (model_port == nullptr) {
                            archfpga_throw(get_arch_file_name(), annot->line_num,
                                           "Failed to find port '%s' on '%s' for sequential delay annotation",
                                           annot_port.port_name().c_str(), annot_port.instance_name().c_str());
                        }

                        //Check that the clock matches the model definition
                        std::string model_clock = model_port->clock;
                        if (model_clock.empty()) {
                            archfpga_throw(get_arch_file_name(), annot->line_num,
                                           "<pb_type> timing-annotation/<model> mismatch on port '%s' of model '%s', model specifies"
                                           " no clock but timing annotation specifies '%s'",
                                           annot_port.port_name().c_str(), model->name, annot_clock.port_name().c_str());
                        }
                        if (model_port->clock != annot_clock.port_name()) {
                            archfpga_throw(get_arch_file_name(), annot->line_num,
                                           "<pb_type> timing-annotation/<model> mismatch on port '%s' of model '%s', model specifies"
                                           " clock as '%s' but timing annotation specifies '%s'",
                                           annot_port.port_name().c_str(), model->name, model_clock.c_str(), annot_clock.port_name().c_str());
                        }
                    }
                }

            } else if (annot->input_pins && annot->output_pins) {
                //Combinational annotation
                VTR_ASSERT_MSG(!annot->clock, "Combinational annotations should have no clock");
                for (const std::string& input_pin : vtr::split(annot->input_pins)) {
                    InstPort annot_in(input_pin);
                    for (const std::string& output_pin : vtr::split(annot->output_pins)) {
                        InstPort annot_out(output_pin);

                        //Find the input model port
                        const t_model_ports* model_port = nullptr;
                        for (const t_model_ports* port = model->inputs; port != nullptr; port = port->next) {
                            if (port->name == annot_in.port_name()) {
                                model_port = port;
                                break;
                            }
                        }

                        if (model_port == nullptr) {
                            archfpga_throw(get_arch_file_name(), annot->line_num,
                                           "Failed to find port '%s' on '%s' for combinational delay annotation",
                                           annot_in.port_name().c_str(), annot_in.instance_name().c_str());
                        }

                        //Check that the output port is listed in the model's combinational sinks
                        auto b = model_port->combinational_sink_ports.begin();
                        auto e = model_port->combinational_sink_ports.end();
                        auto iter = std::find(b, e, annot_out.port_name());
                        if (iter == e) {
                            archfpga_throw(get_arch_file_name(), annot->line_num,
                                           "<pb_type> timing-annotation/<model> mismatch on port '%s' of model '%s', timing annotation"
                                           " specifies combinational connection to port '%s' but the connection does not exist in the model",
                                           model_port->name, model->name, annot_out.port_name().c_str());
                        }
                    }
                }
            } else {
                throw ArchFpgaError("Unrecognized delay annotation");
            }
        }
    }

    //Build a list of combinationally connected sinks
    std::set<std::string> comb_connected_outputs;
    for (t_model_ports* model_ports : {model->inputs, model->outputs}) {
        for (t_model_ports* model_port = model_ports; model_port != nullptr; model_port = model_port->next) {
            comb_connected_outputs.insert(model_port->combinational_sink_ports.begin(), model_port->combinational_sink_ports.end());
        }
    }

    //Check from the model to pb_type's delay annotations
    //
    //  This ensures that the pb_type has annotations for all delays/values
    //  required by the model
    for (t_model_ports* model_ports : {model->inputs, model->outputs}) {
        for (t_model_ports* model_port = model_ports; model_port != nullptr; model_port = model_port->next) {
            //If the model port has no timing specification don't check anything (e.g. architectures with no timing info)
            if (model_port->clock.empty()
                && model_port->combinational_sink_ports.empty()
                && !comb_connected_outputs.count(model_port->name)) {
                continue;
            }

            if (!model_port->clock.empty()) {
                //Sequential port

                if (model_port->dir == IN_PORT) {
                    //Sequential inputs must have a T_setup or T_hold
                    if (find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) == nullptr
                        && find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_THOLD) == nullptr) {
                        std::stringstream msg;
                        msg << "<pb_type> '" << pb_type->name << "' timing-annotation/<model> mismatch on";
                        msg << " port '" << model_port->name << "' of model '" << model->name << "',";
                        msg << " port is a sequential input but has neither T_setup nor T_hold specified";

                        if (is_library_model(model)) {
                            //Only warn if timing info is missing from a library model (e.g. .names/.latch on a non-timing architecture)
                            VTR_LOGF_WARN(get_arch_file_name(), -1, "%s\n", msg.str().c_str());
                        } else {
                            archfpga_throw(get_arch_file_name(), -1, msg.str().c_str());
                        }
                    }

                    if (!model_port->combinational_sink_ports.empty()) {
                        //Sequential input with internal combinational connectsion it must also have T_clock_to_Q
                        if (find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) == nullptr
                            && find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN) == nullptr) {
                            std::stringstream msg;
                            msg << "<pb_type> '" << pb_type->name << "' timing-annotation/<model> mismatch on";
                            msg << " port '" << model_port->name << "' of model '" << model->name << "',";
                            msg << " port is a sequential input with internal combinational connects but has neither";
                            msg << " min nor max T_clock_to_Q specified";

                            if (is_library_model(model)) {
                                //Only warn if timing info is missing from a library model (e.g. .names/.latch on a non-timing architecture)
                                VTR_LOGF_WARN(get_arch_file_name(), -1, "%s\n", msg.str().c_str());
                            } else {
                                archfpga_throw(get_arch_file_name(), -1, msg.str().c_str());
                            }
                        }
                    }

                } else {
                    VTR_ASSERT(model_port->dir == OUT_PORT);
                    //Sequential outputs must have T_clock_to_Q
                    if (find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX) == nullptr
                        && find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN) == nullptr) {
                        std::stringstream msg;
                        msg << "<pb_type> '" << pb_type->name << "' timing-annotation/<model> mismatch on";
                        msg << " port '" << model_port->name << "' of model '" << model->name << "',";
                        msg << " port is a sequential output but has neither min nor max T_clock_to_Q specified";

                        if (is_library_model(model)) {
                            //Only warn if timing info is missing from a library model (e.g. .names/.latch on a non-timing architecture)
                            VTR_LOGF_WARN(get_arch_file_name(), -1, "%s\n", msg.str().c_str());
                        } else {
                            archfpga_throw(get_arch_file_name(), -1, msg.str().c_str());
                        }
                    }

                    if (comb_connected_outputs.count(model_port->name)) {
                        //Sequential output with internal combinational connectison must have T_setup/T_hold
                        if (find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_TSETUP) == nullptr
                            && find_sequential_annotation(pb_type, model_port, E_ANNOT_PIN_TO_PIN_DELAY_THOLD) == nullptr) {
                            std::stringstream msg;
                            msg << "<pb_type> '" << pb_type->name << "' timing-annotation/<model> mismatch on";
                            msg << " port '" << model_port->name << "' of model '" << model->name << "',";
                            msg << " port is a sequential output with internal combinational connections but has";
                            msg << " neither T_setup nor T_hold specified";

                            if (is_library_model(model)) {
                                //Only warn if timing info is missing from a library model (e.g. .names/.latch on a non-timing architecture)
                                VTR_LOGF_WARN(get_arch_file_name(), -1, "%s\n", msg.str().c_str());
                            } else {
                                archfpga_throw(get_arch_file_name(), -1, msg.str().c_str());
                            }
                        }
                    }
                }
            }

            //Check that combinationally connected inputs/outputs have combinational delays between them
            if (model_port->dir == IN_PORT) {
                for (const auto& sink_port : model_port->combinational_sink_ports) {
                    if (find_combinational_annotation(pb_type, model_port->name, sink_port) == nullptr) {
                        std::stringstream msg;
                        msg << "<pb_type> '" << pb_type->name << "' timing-annotation/<model> mismatch on";
                        msg << " port '" << model_port->name << "' of model '" << model->name << "',";
                        msg << " input port '" << model_port->name << "' has combinational connections to";
                        msg << " port '" << sink_port.c_str() << "'; specified in model, but no combinational delays found on pb_type";

                        if (is_library_model(model)) {
                            //Only warn if timing info is missing from a library model (e.g. .names/.latch on a non-timing architecture)
                            VTR_LOGF_WARN(get_arch_file_name(), -1, "%s\n", msg.str().c_str());
                        } else {
                            archfpga_throw(get_arch_file_name(), -1, msg.str().c_str());
                        }
                    }
                }
            }
        }
    }

    return true;
}

void check_models(t_arch* arch) {
    for (t_model* model = arch->models; model != nullptr; model = model->next) {
        if (model->pb_types == nullptr) {
            archfpga_throw(get_arch_file_name(), 0,
                           "No pb_type found for model %s\n", model->name);
        }

        int clk_count, input_count, output_count;
        clk_count = input_count = output_count = 0;
        for (auto ports : {model->inputs, model->outputs}) {
            for (auto port = ports; port != nullptr; port = port->next) {
                int index;
                switch (port->dir) {
                    case IN_PORT:
                        index = port->is_clock ? clk_count++ : input_count++;
                        break;
                    case OUT_PORT:
                        index = output_count++;
                        break;
                    default:
                        archfpga_throw(get_arch_file_name(), 0,
                                       "Port %s of model %s, has an unrecognized type %s\n", port->name, model->name);
                }

                port->index = index;
            }
        }
    }
}
