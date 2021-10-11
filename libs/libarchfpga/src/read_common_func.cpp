#include <set>

#include "vtr_log.h"
#include "arch_error.h"

#include "read_common_func.h"

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
