/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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
 */

#include <sstream> //std::stringstream

#include "verilog.h"
#include "odin_globals.h"
#include "hard_blocks.h"

#include "vtr_util.h"

verilog::writer::writer()
    : generic_writer() {
    this->models_declaration = sc_new_string_cache();
}

verilog::writer::~writer() {
    if (this->models_declaration)
        sc_free_string_cache(this->models_declaration);
}

inline void verilog::writer::_create_file(const char* file_name, const file_type_e file_type) {
    // validate the file_name pointer
    oassert(file_name);
    // validate the file type
    if (file_type != VERILOG)
        error_message(UTIL, unknown_location,
                      "verilog back-end entity cannot create file types(%d) other than verilog", file_type);
    // create the verilog file and set it as the output file
    this->output_file = create_verilog(file_name);
}

void verilog::writer::_write(const netlist_t* netlist) {
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
FILE* verilog::writer::create_verilog(const char* file_name) {
    FILE* out = NULL;

    /* open the file for output */
    out = fopen(file_name, "w");

    if (out == NULL) {
        error_message(UTIL, unknown_location, "Could not open output file %s\n", file_name);
    }
    return (out);
}
