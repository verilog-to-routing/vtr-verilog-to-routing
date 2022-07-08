/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * @file: includes the definition of VERILOG Writer class to write a 
 * given netlist in a Verilog file. In addition to the netlist, the
 * target architecture hardblocks(DSPs) can be outputed as a blackbox.
 * With that said, only the DSPs' declaration are printed.
 */

#include <sstream> //std::stringstream

#include "Verilog.hpp"
#include "odin_globals.h"
#include "hard_blocks.h"
#include "vtr_util.h"

Verilog::Writer::Writer()
    : GenericWriter() {
    this->models_declaration = sc_new_string_cache();
}

Verilog::Writer::~Writer() {
    if (this->models_declaration)
        sc_free_string_cache(this->models_declaration);
}

inline void Verilog::Writer::_create_file(const char* file_name, const file_type_e file_type) {
    // validate the file_name pointer
    oassert(file_name);
    // validate the file type
    if (file_type != _VERILOG)
        error_message(UTIL, unknown_location,
                      "Verilog back-end entity cannot create file types(%d) other than Verilog", file_type);
    // create the Verilog file and set it as the output file
    this->output_file = create_verilog(file_name);
}

void Verilog::Writer::_write(const netlist_t* netlist) {
    // to write the top module and netlist components
    if (netlist) {
        /* [TODO] */
    }

    // print out the rest od models, including DSPs in the target architecture
    t_model* model = Arch.models;

    while (model) {
        int sc_spot;
        if ((sc_spot = sc_lookup_string(this->models_declaration, model->name)) != -1) {
            fprintf(this->output_file, "%s", (char*)this->models_declaration->data[sc_spot]);
            fflush(this->output_file);
        }
        model = model->next;
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: create_verilog)
 * 
 * @brief initiate a new output file stream
 * 
 * @param file_name the path to the verilog file
 * 
 * @return an output stream to the verilog file
 *-------------------------------------------------------------------------------------------
 */
FILE* Verilog::Writer::create_verilog(const char* file_name) {
    FILE* out = NULL;

    /* open the file for output */
    out = fopen(file_name, "w");

    if (out == NULL) {
        error_message(UTIL, unknown_location, "Could not open output file %s\n", file_name);
    }
    return (out);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: declare_blackbox)
 * 
 * @brief find the corresponding blackbox with the given 
 * name in the given target arhitecture, then add its 
 * Verilog declartion to this->models_declaration string cache.
 * 
 * @param bb_name the blackbox(DSP) name
 * 
 * @return a long value, which is representing the index of 
 * the declartion in models string cache. Will return -1 if 
 * a DSP with the given name does not exist in the architecture.
 *-------------------------------------------------------------------------------------------
 */
long Verilog::Writer::declare_blackbox(const char* bb_name) {
    /* to validate the blackbox name */
    oassert(bb_name);

    t_model* bb = find_hard_block(bb_name);
    if (bb == NULL) {
        error_message(UTIL, unknown_location,
                      "Odin-II failed to find DSP module \"%s\" in the target device.", bb_name);
    }

    std::stringstream bb_declaration;

    // need to specify "(* blackbox *)" tag if Yosys
    // is going to elaborate the Verilog file
    if (elaborator_e::_YOSYS) {
        bb_declaration << BLACKBOX_ATTR << NEWLINE;
    }

    bb_declaration << MODULE << TAB << bb_name << OPEN_PARENTHESIS << std::endl;
    bb_declaration << declare_ports(bb) << std::endl;
    bb_declaration << CLOSE_PARENTHESIS << SEMICOLON << std::endl;
    bb_declaration << HARD_BLOCK_COMMENT << std::endl;
    bb_declaration << END_MODULE << NEWLINE << std::endl;

    int sc_spot;
    if ((sc_spot = sc_add_string(this->models_declaration, bb->name)) != -1) {
        this->models_declaration->data[sc_spot] = (void*)vtr::strdup(bb_declaration.str().c_str());
        return (sc_spot);
    }

    return (-1);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: declare_ports)
 * 
 * @brief generate a string that includes the declaration 
 * of input/output ports of a given t_model
 * 
 * @param model the DSP t_model pointer
 * 
 * @return a string value including the declaration of all 
 * input/output ports related to the given DSP model
 *-------------------------------------------------------------------------------------------
 */
std::string Verilog::Writer::declare_ports(t_model* model) {
    /* to validate the model pointer */
    oassert(model);

    std::stringstream input_stream;
    t_model_ports* input_port = model->inputs;
    while (input_port) {
        input_stream << TAB
                     << INPUT_PORT << TAB
                     << OPEN_SQUARE_BRACKET
                     << input_port->size << COLON << "0"
                     << CLOSE_SQUARE_BRACKET
                     << TAB << input_port->name
                     << COMMA << std::endl;

        // move forward until the end of input ports' list
        input_port = input_port->next;
    }

    std::stringstream output_stream;
    t_model_ports* output_port = model->outputs;
    while (output_port) {
        output_stream << TAB
                      << OUTPUT_PORT << TAB
                      << OPEN_SQUARE_BRACKET
                      << output_port->size << COLON << "0"
                      << CLOSE_SQUARE_BRACKET
                      << TAB << output_port->name
                      << COMMA << std::endl;

        // move forward until the end of output ports' list
        output_port = output_port->next;
    }

    std::string input_str = input_stream.str();
    std::string output_str = output_stream.str();

    // check the value of input/output ports declaration
    // to trim extra last semicolon if required
    std::stringstream ports_declaration;
    if (!input_stream.str().empty() && output_stream.str().empty()) {
        input_str[input_str.find_last_not_of(COMMA) - 1] = '\0';
        ports_declaration << input_str;
    } else if (!output_stream.str().empty()) {
        if (!input_stream.str().empty())
            ports_declaration << input_str;

        ports_declaration << output_str.substr(0, output_str.find_last_not_of(COMMA) - 1);
    }

    // return the string value
    return (ports_declaration.str());
}
